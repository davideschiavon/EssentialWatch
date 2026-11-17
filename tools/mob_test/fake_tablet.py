"""Fake tablet v2: receives $MOB from the watch (ack $MOA) and, once an A arrives, sends $MOI (own counter) to the
address/port the A came from, checking the $MOK that comes back. Usage: fake_tablet2.py <id> <key64> <seconds> [port]"""
import hmac, hashlib, socket, sys, time, urllib.parse


def hmac8(key, body): return hmac.new(key, body.encode(), hashlib.sha256).hexdigest()[:8]


def cs(inner):
    c = 0
    for b in inner.encode(): c ^= b
    return f"{c:02X}"


def sign(key, body):
    inner = f"{body},{hmac8(key, body)}"
    return f"${inner}*{cs(inner)}\r\n"


dev_id, key, secs = sys.argv[1], bytes.fromhex(sys.argv[2]), float(sys.argv[3])
port = int(sys.argv[4]) if len(sys.argv) > 4 else 10120
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("0.0.0.0", port)); s.settimeout(0.3)
t0 = time.time(); last = None
moi_seq = 6; next_moi = None; watch = None; sent = {}
plan = []   # (time offset from first A, siren)
print(f"listening UDP {port}, id {dev_id}", flush=True)
while time.time() - t0 < secs:
    now = time.time() - t0
    if next_moi is not None and time.time() >= next_moi and watch:
        k = len(plan_done)
        siren = 1 if 1 <= k <= 3 else 0   # siren on for the 2nd..4th message, then off
        moi_seq += 1
        body = f"MOI,1,{dev_id},{moi_seq:04d},{142 - 4 * k},{287 - 2 * k},{siren},3,45.12345,9.12345"
        s.sendto(sign(key, body).encode(), watch); sent[moi_seq] = time.time()
        plan_done.append(moi_seq)
        print(f"{now:6.1f}s -> MOI seq {moi_seq:04d} siren {siren} dist {142 - 4 * k}", flush=True)
        next_moi = time.time() + 5 if k < 6 else None
        if k == 3:   # one duplicate of the siren message: the watch must answer with $MOK again but not process it twice
            time.sleep(0.2); s.sendto(sign(key, body).encode(), watch); print("      -> (duplicate re-sent)", flush=True)
    try:
        data, addr = s.recvfrom(512)
    except socket.timeout:
        continue
    line = data.decode("ascii", "replace").strip()
    if line.startswith("$MOK,"):
        inner = line[1:line.rfind("*")]
        p = inner.split(",")
        ok = cs(inner) == line[-2:].upper() and p[1] == dev_id and hmac8(key, ",".join(p[:3])) == p[3]
        sq = int(p[2]); lat = (time.time() - sent[sq]) if sq in sent else -1
        print(f"{now:6.1f}s <- MOK seq {sq:04d} {'VALID' if ok else 'BAD'} ({lat * 1000:.0f} ms after the MOI)", flush=True)
        continue
    if not line.startswith("$MOB,"):
        print(f"{now:6.1f}s ?? {line[:80]}", flush=True); continue
    star = line.rfind("*"); inner = line[1:star]; f = inner.split(",")
    if cs(inner) != line[star + 1:].upper() or f[2] != dev_id or hmac8(key, ",".join(f[:-1])) != f[-1]:
        print(f"{now:6.1f}s DROP {line}", flush=True); continue
    seq = int(f[3]); dup = last is not None and seq == last
    if not dup: last = seq
    name = urllib.parse.unquote(f[10]) if f[4] == "N" else ""
    print(f"{now:6.1f}s {'DUP' if dup else 'ACC'} seq {seq:04d} {f[4]} lat={f[5]!r} lon={f[6]!r} fixage={f[7]!r} batt={f[9]!r} {name}", flush=True)
    body = f"MOA,{dev_id},{seq:04d}"
    s.sendto(sign(key, body).encode(), addr)
    if f[4] == "A" and next_moi is None and not watch:
        watch = addr; plan_done = []; next_moi = time.time() + 1
