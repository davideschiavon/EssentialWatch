"""Fake tablet for pairing: accepts $MOP while its window is open (code given on the command line), answers $MOQ,
then acknowledges $MOB lines signed with the derived key. Usage: fake_pair_tablet.py <code> <seconds> [id]"""
import hmac, hashlib, socket, sys, time, os
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization

code, secs = sys.argv[1].encode(), float(sys.argv[2])
dev_id = sys.argv[3] if len(sys.argv) > 3 else "T3ST"


def cs(inner):
    c = 0
    for b in inner.encode(): c ^= b
    return f"{c:02X}"


def mac(text): return hmac.new(code, text.encode(), hashlib.sha256).hexdigest()[:16]
def hmac8(key, body): return hmac.new(key, body.encode(), hashlib.sha256).hexdigest()[:8]


s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s.bind(("0.0.0.0", 10120)); s.settimeout(0.3)
t0 = time.time(); fails = 0; key = None; last = None; opened = time.time()
print(f"pairing window open, code {code.decode()}, id {dev_id}", flush=True)
while time.time() - t0 < secs:
    try:
        data, addr = s.recvfrom(512)
    except socket.timeout:
        continue
    line = data.decode("ascii", "replace").strip(); now = time.time() - t0
    if line.startswith("$MOP,"):
        star = line.rfind("*"); inner = line[1:star]; f = inner.split(",")
        if key is not None:
            print(f"{now:6.1f}s MOP ignored (already paired) from {addr}", flush=True); continue
        if cs(inner) != line[star + 1:].upper() or len(f) != 4 or f[1] != "1":
            print(f"{now:6.1f}s MOP bad format", flush=True); continue
        if mac(f"MOP,1,{f[2]}") != f[3]:
            fails += 1; print(f"{now:6.1f}s MOP WRONG CODE (fail {fails}) from {addr[0]}:{addr[1]} - silent", flush=True); continue
        wpub = bytes.fromhex(f[2])
        try:
            wk = ec.EllipticCurvePublicKey.from_encoded_point(ec.SECP256R1(), wpub)
        except Exception as e:
            print(f"{now:6.1f}s bad public key {e}", flush=True); continue
        tk = ec.generate_private_key(ec.SECP256R1())
        tpub = tk.public_key().public_bytes(serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint).hex()
        shared = tk.exchange(ec.ECDH(), wk)
        prk = hmac.new(b"essentialsail-watch-1", shared, hashlib.sha256).digest()
        key = hmac.new(prk, dev_id.encode() + b"\x01", hashlib.sha256).digest()
        body = f"MOQ,1,{dev_id},{tpub},{mac(f'MOQ,1,{dev_id},{tpub},{f[2]}')}"
        s.sendto(f"${body}*{cs(body)}\r\n".encode(), addr)
        print(f"{now:6.1f}s MOP OK from {addr[0]}:{addr[1]} -> MOQ sent, paired as {dev_id}, key {key.hex()[:16]}...", flush=True)
        continue
    if key is None:
        print(f"{now:6.1f}s (not paired yet) {line[:60]}", flush=True); continue
    if line.startswith("$MOB,"):
        star = line.rfind("*"); inner = line[1:star]; f = inner.split(",")
        good = cs(inner) == line[star + 1:].upper() and f[2] == dev_id and hmac8(key, ",".join(f[:-1])) == f[-1]
        if not good:
            print(f"{now:6.1f}s MOB with a WRONG signature: {line[:70]}", flush=True); continue
        seq = int(f[3]); body = f"MOA,{dev_id},{seq:04d}"; inner2 = f"{body},{hmac8(key, body)}"
        s.sendto(f"${inner2}*{cs(inner2)}\r\n".encode(), addr)
        print(f"{now:6.1f}s MOB {f[4]} seq {seq:04d} signature VALID with the derived key, ack sent", flush=True)
