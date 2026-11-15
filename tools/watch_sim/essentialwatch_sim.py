#!/usr/bin/env python3
"""EssentialWatch simulator: a pretend boat for the EssentialWatch AMOLED watch.

It is an NMEA 0183 TCP server (own boat + AIS targets) with scenarios made for the watch pages:
clock (RMC time), wind, SOG / depth, AIS list with collision warnings, depth alarm, signal-lost watermark.
The window also drives the real watch over its web API: IP address, live screen copy, keys (Right, Left,
Double tap, Power, Boot), battery / alarm status, vibration test.

Scenarios (inspired by the ones of the EssentialSail simulator, re-implemented here):
  Cadaques coastal, Corsica 12 nm (a collision course every 150 s), L'Espiguette shallow shelf, Strong wind and
  drift, plus three watch drills: Collision drill, Shallow-water drill, Signal-lost drill.

Needs Python 3.9+ with Tkinter. Pillow is optional (live screen copy of the watch).
Run:       python essentialwatch_sim.py
Headless:  python essentialwatch_sim.py --headless --scenario corsica --port 10112 [--seconds 120]
Never use the simulated depth or positions for navigation.
"""
import argparse
import base64
import hashlib
import io
import json
import math
import os
import random
import re
import socket
import sys
import queue
import threading
import time
import urllib.error
import urllib.request

# ============================== geometry helpers (nautical miles, knots, degrees) ==============================
def vec(course, speed):
    r = math.radians(course)
    return speed * math.sin(r), speed * math.cos(r)          # east, north

def bearing(dx, dy):
    return math.degrees(math.atan2(dx, dy)) % 360

def norm180(a):
    return (a + 180) % 360 - 180

def nmea(body, start="$"):
    cs = 0
    for ch in body:
        cs ^= ord(ch)
    return f"{start}{body}*{cs:02X}\r\n"

# ============================== AIS encoding (6-bit payload) ==============================
def bits(val, n):
    return format(val & ((1 << n) - 1), f"0{n}b")

def ais_text(s, n):
    out = ""
    for ch in s.upper().ljust(n, "@")[:n]:
        v = ord(ch)
        out += bits(v - 64 if v >= 64 else v, 6)
    return out

def ais_payload(b):
    b += "0" * ((6 - len(b) % 6) % 6)
    out = ""
    for i in range(0, len(b), 6):
        v = int(b[i:i + 6], 2) + 48
        out += chr(v + 8 if v > 87 else v)
    return out

def vdm(pl, frag=1, num=1, seq="", fill=0):
    return nmea(f"AIVDM,{frag},{num},{seq},A,{pl},{fill}", "!")

def ais_position(t, lat, lon):
    """Class A (type 1) or class B (type 18) position report."""
    if t["cls"] == "A":
        b = (bits(1, 6) + bits(0, 2) + bits(t["mmsi"], 30) + bits(0, 4) + bits(0, 8) + bits(int(t["sog"] * 10), 10) + "0" +
             bits(round(lon * 600000), 28) + bits(round(lat * 600000), 27) + bits(int(t["cog"] * 10), 12) + bits(511, 9) +
             bits(60, 6) + bits(0, 2) + bits(0, 3) + "0" + bits(0, 19))
    else:
        b = (bits(18, 6) + bits(0, 2) + bits(t["mmsi"], 30) + bits(0, 8) + bits(int(t["sog"] * 10), 10) + "0" +
             bits(round(lon * 600000), 28) + bits(round(lat * 600000), 27) + bits(int(t["cog"] * 10), 12) + bits(511, 9) +
             bits(60, 6)).ljust(168, "0")
    return vdm(ais_payload(b))

def ais_name(t):
    """Class A: type 5 static data (2 fragments). Class B: type 24 part A."""
    if t["cls"] == "B":
        return vdm(ais_payload(bits(24, 6) + bits(0, 2) + bits(t["mmsi"], 30) + bits(0, 2) + ais_text(t["name"], 20)), fill=2)
    b = (bits(5, 6) + bits(0, 2) + bits(t["mmsi"], 30) + bits(0, 2) + bits(0, 30) + ais_text("IABC", 7) + ais_text(t["name"], 20) +
         bits(60, 8) + bits(0, 30) + bits(1, 4) + bits(0, 20) + bits(0, 8) + ais_text("PORTO", 20) + "0" + "0")
    pl = ais_payload(b)
    return vdm(pl[:50], 1, 2, "3") + vdm(pl[50:], 2, 2, "3")

# ============================== scenarios ==============================
class Scenario:
    def __init__(self, key, title, text, lat, lon, cog, sog, twd, tws, depth, boats, gust=0.0, collision_every=0.0,
                 wind_sweep_s=0.0, yaw=0.0, signal_cycle=None):
        self.key, self.title, self.text = key, title, text
        self.lat, self.lon, self.cog, self.sog = lat, lon, cog, sog
        self.twd, self.tws, self.gust = twd, tws, gust
        self.depth = depth                # function(t) -> metres
        self.boats = boats                # [(name, class, rel bearing from own boat, range nm, cog, sog)]
        self.collision_every, self.wind_sweep_s, self.yaw, self.signal_cycle = collision_every, wind_sweep_s, yaw, signal_cycle

NAMES = ["BLUE HORIZON", "FERRY NEPTUNE", "CROSS TRAFFIC", "SEA BREEZE", "ALPHA", "MISTRAL", "TRAMONTANA", "LA BELLE ETOILE",
         "NORDIC STAR", "OCEAN DREAM"]

SCENARIOS = [
    Scenario("cadaques", "Cadaques coastal",
             "0.3 nm off Cap de Creus, Tramontana 28 kn from the north, steep seabed (18-40 m), four vessels around.",
             42.3200, 3.3100, 160, 6.0, 355, 28, lambda t: 29 + 11 * math.sin(t / 80.0), gust=6,
             boats=[("SEA BREEZE", "B", 60, 1.4, 250, 4.0), ("FERRY NEPTUNE", "A", 280, 2.5, 80, 14.0),
              ("ALPHA", "B", 150, 3.2, 0, 0.0), ("", "A", 20, 4.5, 190, 9.0)]),
    Scenario("corsica", "12 nm off Corsica",
             "Mistral 22 kn, deep water, vessels at different distances; every 150 s one comes close on a collision course.",
             42.2000, 8.3000, 20, 6.5, 300, 22, lambda t: 520.0, gust=4,
             boats=[("BLUE HORIZON", "A", 35, 3.0, 215, 8.0), ("TRAMONTANA", "A", 290, 5.0, 100, 11.0),
              ("", "A", 120, 6.5, 300, 7.0), ("OCEAN DREAM", "A", 200, 8.0, 20, 12.0)], collision_every=150),
    Scenario("espiguette", "L'Espiguette shallow shelf",
             "About 450 m off the beach, a sandy shelf from 5 m down to 1.3 m: the depth alarm triggers below the minimum depth.",
             43.4800, 4.1500, 60, 5.0, 150, 15, lambda t: 3.2 + 1.9 * math.sin(t / 35.0), gust=3,
             boats=[("LA BELLE ETOILE", "B", 330, 0.8, 60, 3.0), ("", "B", 200, 1.5, 0, 0.0), ("NORDIC STAR", "A", 20, 4.0, 200, 10.0)]),
    Scenario("storm", "Strong wind and drift",
             "40 kn gusting 50, the wind goes round all directions in 8 minutes, the boat yaws and drifts.",
             43.1000, 4.0000, 90, 4.0, 270, 40, lambda t: 32.0, gust=10, wind_sweep_s=480, yaw=25,
             boats=[("MISTRAL", "A", 45, 4.0, 225, 9.0)]),
    Scenario("collision", "Drill: collision",
             "Open sea, a collision course every 90 s (CPA 0, TCPA 8 min) plus two harmless targets. Try BOOT = snooze.",
             41.5000, 6.0000, 90, 6.0, 40, 14, lambda t: 80.0, gust=2,
             boats=[("ALPHA", "B", 300, 3.0, 40, 5.0), ("FERRY NEPTUNE", "A", 150, 4.5, 330, 13.0)], collision_every=90),
    Scenario("shallow", "Drill: shallow water",
             "The depth ramps from 8 m down to 1 m and back every 2 minutes: minimum depth alarm, SOG / depth page.",
             43.4800, 4.1500, 60, 5.0, 150, 12, lambda t: 4.5 + 3.5 * math.cos(2 * math.pi * t / 120.0), gust=2, boats=[]),
    Scenario("signal", "Drill: signal lost",
             "Data for 25 s, then silence for 20 s: the orange SEGNALE PERSO watermark appears and goes away again.",
             42.5000, 7.0000, 120, 6.0, 20, 15, lambda t: 25.0, gust=2,
             boats=[("SEA BREEZE", "B", 60, 1.0, 0, 0.0)], signal_cycle=(25, 20)),
]
SCEN = {s.key: s for s in SCENARIOS}


class Sim:
    """The pretend boat. step() once per second; everything is protected by a lock (server thread + window)."""

    def __init__(self):
        self.lock = threading.Lock()
        self.scn = SCENARIOS[1]
        self.manual_depth = None      # metres, None = from the scenario
        self.mute = False             # manual signal loss
        self.tws_user = None          # window sliders override the scenario wind
        self.twd_user = None
        self.next_mmsi = 247100000
        self.reset(self.scn)

    def reset(self, scn):
        with self.lock:
            self.scn, self.t, self.n = scn, 0.0, 0
            self.x = self.y = 0.0
            self.t_collision = 15 - scn.collision_every if scn.collision_every else 0   # the first one comes after 15 s
            self.targets = []
            for name, cls, rb, rng, cog, sog in scn.boats:
                self._add(name, cls, scn.cog + rb, rng, cog, sog)
            self._update_wind()
            self.hdg = scn.cog

    def _add(self, name, cls, brg, rng, cog, sog):
        ex, ey = vec(brg, rng)
        self.next_mmsi += 1
        self.targets.append(dict(mmsi=self.next_mmsi, name=name, cls=cls, x=self.x + ex, y=self.y + ey, cog=cog % 360, sog=sog))

    def _update_wind(self):
        s = self.scn
        twd = s.twd + (360.0 * self.t / s.wind_sweep_s if s.wind_sweep_s else 0)
        gust = s.gust * max(0.0, math.sin(self.t / 7.0) * math.sin(self.t / 3.1))
        self.twd = (self.twd_user if self.twd_user is not None else twd) % 360
        self.tws = (self.tws_user if self.tws_user is not None else s.tws) + gust

    def add_collision(self):
        """A target that meets our boat in 8 minutes on a reciprocal-ish course: CPA 0, TCPA 8 min."""
        with self.lock:
            tm = 8 / 60.0                                            # hours
            c = self.scn.cog + 180 + random.uniform(-40, 40)
            sp = random.uniform(7, 12)
            ox, oy = vec(self.scn.cog, self.scn.sog)
            vx, vy = vec(c, sp)
            px, py = self.x + ox * tm, self.y + oy * tm              # where we will be
            self.next_mmsi += 1
            self.targets.append(dict(mmsi=self.next_mmsi, name=random.choice(NAMES), cls="A", x=px - vx * tm, y=py - vy * tm, cog=c % 360, sog=sp))

    def add_far(self):
        with self.lock:
            self._add(random.choice(NAMES + [""]), random.choice("AB"), random.uniform(0, 360), random.uniform(2, 6),
                      random.uniform(0, 360), random.uniform(0, 12))

    def clear_targets(self):
        with self.lock:
            self.targets = []

    def depth(self):
        return self.manual_depth if self.manual_depth is not None else max(0.2, self.scn.depth(self.t))

    def silent(self):
        c = self.scn.signal_cycle
        return self.mute or bool(c and (self.t % (c[0] + c[1])) >= c[0])

    def step(self, dt=1.0):
        with self.lock:
            s = self.scn
            self.t += dt
            self.n += 1
            ox, oy = vec(s.cog, s.sog)
            if s.yaw:                                                # storm: yaws and drifts to leeward
                self.hdg = (s.cog + s.yaw * math.sin(self.t / 6.0)) % 360
                dx, dy = vec(self.twd + 180, 0.6)
                ox, oy = ox + dx, oy + dy
            else:
                self.hdg = s.cog
            self.x += ox * dt / 3600
            self.y += oy * dt / 3600
            for t in self.targets:
                tx, ty = vec(t["cog"], t["sog"])
                t["x"] += tx * dt / 3600
                t["y"] += ty * dt / 3600
            self.targets = [t for t in self.targets if math.hypot(t["x"] - self.x, t["y"] - self.y) < 9.0]
            self._update_wind()
            if s.collision_every and self.t >= self.t_collision + s.collision_every:
                self.t_collision = self.t
                self.lock.release()
                try:
                    self.add_collision()
                finally:
                    self.lock.acquire()
                self.targets = self.targets[-6:]

    # ---- derived values ----
    def own(self):
        s = self.scn
        ox, oy = vec(s.cog, s.sog)
        sog = math.hypot(ox, oy)
        cog = bearing(ox, oy) if sog > 0.01 else self.hdg
        wx, wy = vec(self.twd + 180, self.tws)                       # wind velocity (blows towards)
        ax, ay = wx - ox, wy - oy                                    # apparent wind velocity
        aws = math.hypot(ax, ay)
        awa = norm180(bearing(-ax, -ay) - self.hdg) if aws > 0.01 else 0.0
        twa = norm180(self.twd - self.hdg)
        return dict(sog=sog, cog=cog, hdg=self.hdg, aws=aws, awa=awa, tws=self.tws, twa=twa, depth=self.depth())

    def latlon(self, x, y):
        s = self.scn
        return s.lat + y / 60.0, s.lon + x / (60.0 * math.cos(math.radians(s.lat)))

    def risk(self, t):
        """(distance nm, bearing, CPA nm, TCPA min or None) of a target as the watch computes it."""
        rx, ry = t["x"] - self.x, t["y"] - self.y
        ox, oy = vec(self.scn.cog, self.scn.sog)
        tx, ty = vec(t["cog"], t["sog"])
        vx, vy = tx - ox, ty - oy
        v2 = vx * vx + vy * vy
        dist = math.hypot(rx, ry)
        if v2 < 1e-6:
            return dist, bearing(rx, ry), dist, None
        tc = -(rx * vx + ry * vy) / v2 * 60.0                        # minutes
        if tc < 0:
            return dist, bearing(rx, ry), dist, None
        cx, cy = rx + vx * tc / 60, ry + vy * tc / 60
        return dist, bearing(rx, ry), math.hypot(cx, cy), tc

    def sentences(self):
        """One second of NMEA: own boat first, AIS after."""
        with self.lock:
            if self.silent():
                return ""
            o = self.own()
            lat, lon = self.latlon(self.x, self.y)
            now = time.gmtime()
            la, lo = abs(lat), abs(lon)
            lat_s = f"{int(la):02d}{(la - int(la)) * 60:07.4f},{'N' if lat >= 0 else 'S'}"
            lon_s = f"{int(lo):03d}{(lo - int(lo)) * 60:07.4f},{'E' if lon >= 0 else 'W'}"
            tm, dt = time.strftime("%H%M%S", now), time.strftime("%d%m%y", now)
            out = nmea(f"GPRMC,{tm}.00,A,{lat_s},{lon_s},{o['sog']:.1f},{o['cog']:.1f},{dt},,,A")
            out += nmea(f"GPVTG,{o['cog']:.1f},T,{o['cog']:.1f},M,{o['sog']:.1f},N,{o['sog'] * 1.852:.1f},K,A")
            out += nmea(f"IIHDT,{o['hdg']:.1f},T")
            out += nmea(f"IIVHW,{o['hdg']:.1f},T,{o['hdg']:.1f},M,{o['sog']:.1f},N,{o['sog'] * 1.852:.1f},K")
            out += nmea(f"IIMWV,{o['awa'] % 360:.1f},R,{o['aws']:.1f},N,A")
            out += nmea(f"IIMWV,{o['twa'] % 360:.1f},T,{o['tws']:.1f},N,A")
            out += nmea(f"IIDPT,{o['depth']:.1f},0.0")
            for t in self.targets:
                la_t, lo_t = self.latlon(t["x"], t["y"])
                out += ais_position(t, la_t, lo_t)
            if self.n % 4 == 1:                                      # names (static data) less often
                for t in self.targets:
                    if t["name"]:
                        out += ais_name(t)
            return out

# ============================== NMEA TCP server ==============================
class Server:
    def __init__(self, sim, log):
        self.sim, self.log = sim, log
        self.sock, self.clients, self.ips, self.running = None, [], {}, False
        self.lock = threading.Lock()

    def start(self, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):   # Windows: a second simulator on the same port gets an error
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        else:
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(("0.0.0.0", port))
        self.sock.listen(8)
        self.running = True
        threading.Thread(target=self._accept, daemon=True).start()
        threading.Thread(target=self._send, daemon=True).start()

    def stop(self):
        self.running = False
        for c in [self.sock] + self.clients:
            try: c.close()
            except Exception: pass
        with self.lock:
            self.clients, self.ips = [], {}

    def _accept(self):
        while self.running:
            try:
                c, addr = self.sock.accept()
            except OSError:
                break
            try:
                c.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                with self.lock:
                    self.clients.append(c)
                    self.ips[c] = addr[0]
                self.log(f"client connected: {addr[0]}")
            except Exception as e:
                self.log(f"client error: {e}")

    def _send(self):
        while self.running:
            try:
                self.sim.step(1.0)
                data = self.sim.sentences().encode("ascii")
            except Exception as e:   # never let one bad step stop the data
                self.log(f"simulation error: {e}")
                data = b""
            with self.lock:
                for c in list(self.clients):
                    try:
                        if data:
                            c.sendall(data)
                    except OSError:
                        self.clients.remove(c)
                        self.log(f"client disconnected: {self.ips.pop(c, '')}")
            time.sleep(1.0)

    def watch_ips(self):
        with self.lock:
            return [self.ips[c] for c in self.clients if c in self.ips]

# ============================== the real watch (web API, HTTP Digest login) ==============================
class WatchApi:
    def __init__(self):
        self.host, self.password, self._dg = "", "", None

    def _challenge(self):
        try:
            urllib.request.urlopen(f"http://{self.host}/api/status", timeout=6)
        except urllib.error.HTTPError as e:
            return dict(re.findall(r'(\w+)="([^"]*)"', e.headers.get("WWW-Authenticate", "")))
        return {}

    def _auth(self, method, uri, fresh):
        d = self._dg
        if fresh or not d or d["host"] != self.host:
            d = self._dg = {"host": self.host, "nc": 0, **self._challenge()}
        d["nc"] += 1
        md5 = lambda t: hashlib.md5(t.encode()).hexdigest()
        nc, cn = f"{d['nc']:08x}", os.urandom(8).hex()
        realm, nonce = d.get("realm", ""), d.get("nonce", "")
        ha1 = md5(f"admin:{realm}:{self.password}")
        resp = md5(f"{ha1}:{nonce}:{nc}:{cn}:auth:{md5(method + ':' + uri)}")
        return (f'Digest username="admin", realm="{realm}", nonce="{nonce}", uri="{uri}", algorithm=MD5, '
                f'response="{resp}", opaque="{d.get("opaque", "")}", qop=auth, nc={nc}, cnonce="{cn}"')

    def request(self, method, path, timeout=10, raw=False):
        for attempt in (0, 1):
            req = urllib.request.Request(f"http://{self.host}{path}", method=method, data=b"" if method == "POST" else None)
            req.add_header("Authorization", self._auth(method, path, attempt == 1))
            try:
                with urllib.request.urlopen(req, timeout=timeout) as r:
                    body = r.read()
                    return body if raw else json.loads(body.decode())
            except urllib.error.HTTPError as e:
                if e.code == 401 and attempt == 0:
                    continue
                if e.code == 401:
                    raise PermissionError("wrong password")
                if e.code == 429:
                    raise PermissionError("too many wrong passwords: locked out for a few minutes")
                try:
                    raise ValueError(json.loads(e.read().decode()).get("error", str(e)))
                except json.JSONDecodeError:
                    raise ValueError(str(e))

    def key(self, k):
        return self.request("POST", "/api/key?k=" + k)

    def status(self):
        return self.request("GET", "/api/status")

    def screen(self):
        return self.request("GET", "/screen.bmp?half=1", timeout=30, raw=True)

# ============================== the real watch over USB (serial commands of the firmware) ==============================
def rgb565_image(w, h, b64):
    """Screen copy sent by the firmware: half size, RGB565 big-endian, base64."""
    from PIL import Image
    raw = base64.b64decode(b64)
    if len(raw) != w * h * 2:   # bytes lost on the USB link: the colours would be shifted
        raise ValueError(f"incomplete copy ({len(raw)} of {w * h * 2} bytes)")
    out = bytearray(w * h * 3)
    for n in range(min(w * h, len(raw) // 2)):
        v = (raw[2 * n] << 8) | raw[2 * n + 1]
        out[3 * n] = ((v >> 11) & 31) * 255 // 31
        out[3 * n + 1] = ((v >> 5) & 63) * 255 // 63
        out[3 * n + 2] = (v & 31) * 255 // 31
    return Image.frombytes("RGB", (w, h), bytes(out))


class SerialLink:
    """USB serial link: commands go out, log lines, status and screen copies come back."""
    B64 = re.compile(r"^[A-Za-z0-9+/=]+$")

    def __init__(self, on_line):
        self.on_line, self.ser, self.ok = on_line, None, False
        self._wlock = threading.Lock()
        self._st, self._st_evt = None, threading.Event()
        self._img, self._img_evt = None, threading.Event()

    def open(self, port):
        import serial
        s = serial.Serial()
        s.port, s.baudrate, s.timeout = port, 115200, 0.2
        s.dtr = s.rts = False   # try not to reset the board when the port opens
        s.open()
        self.ser, self.ok = s, True
        threading.Thread(target=self._read, daemon=True).start()

    def close(self):
        self.ok = False
        try:
            self.ser.close()
        except Exception:
            pass

    def send(self, text):
        if not self.ok:
            raise RuntimeError("USB not connected")
        with self._wlock:
            self.ser.write(text.encode())
            self.ser.flush()

    def _read(self):
        buf, shot = b"", None
        while self.ok:
            try:
                chunk = self.ser.read(4096)
            except Exception as e:
                self.ok = False
                self.on_line(f"USB lost: {e}")
                break
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = raw.decode("utf-8", "replace").strip()
                if shot is not None:
                    if line == "SCR565_END":
                        try:
                            self._img = rgb565_image(shot[0], shot[1], "".join(shot[2]))
                            self._img_evt.set()
                        except Exception as e:
                            self.on_line(f"screen copy damaged: {e}")
                            self._img = None   # the caller keeps the previous picture
                            self._img_evt.set()
                        shot = None
                    elif self.B64.match(line):
                        shot[2].append(line)
                    elif line:
                        self.on_line(line)   # a log line in the middle of the copy
                elif line.startswith("SCR565_BEGIN"):
                    _, w, h = line.split()
                    shot = [int(w), int(h), []]
                elif line.startswith("STATUS "):
                    try:
                        self._st = json.loads(line[7:])
                        self._st_evt.set()
                    except ValueError:
                        pass
                elif line:
                    self.on_line(line)

    def status(self):
        self._st_evt.clear()
        self.send("CSTATUS\n")
        if not self._st_evt.wait(4):
            raise TimeoutError("no answer over USB (the watch may be restarting: wait ~20 s)")
        return self._st

    def screen(self):
        self._img_evt.clear()
        self.send("S")
        if not self._img_evt.wait(20):
            raise TimeoutError("no screen copy over USB")
        return self._img

# ============================== window ==============================
SETTINGS_FILE = os.path.join(os.path.expanduser("~"), ".essentialwatch_sim.json")


def load_settings():
    try:
        with open(SETTINGS_FILE, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def save_settings(d):
    """Remembers the watch IP, its web password and the connection mode (plain text in the user profile)."""
    try:
        with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
            json.dump(d, f)
    except OSError:
        pass


def local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return "127.0.0.1"


def run_gui(args):
    import tkinter as tk
    from tkinter import ttk
    try:
        from PIL import Image, ImageTk
    except ImportError:
        Image = None

    sim = Sim()
    root = tk.Tk()
    ui_q = queue.Queue()   # UI updates from the worker threads (server, USB, HTTP): run by pump() in the Tk thread

    def ui(fn):
        ui_q.put(fn)

    def pump():
        try:
            while True:
                ui_q.get_nowait()()
        except queue.Empty:
            pass
        except Exception as e:
            print("UI error:", e, file=sys.stderr)
        root.after(100, pump)
    root.after(100, pump)
    root.title("EssentialWatch Simulator")
    root.geometry("1100x780")
    logbox = tk.Text(root, height=6, state="disabled", font=("Consolas", 9))
    logbox.pack(side="bottom", fill="x")
    nb = ttk.Notebook(root)
    nb.pack(fill="both", expand=True)
    tab_sim, tab_tools = ttk.Frame(nb), ttk.Frame(nb)
    nb.add(tab_sim, text="  Simulator  ")
    nb.add(tab_tools, text="  Tools  ")

    def log(s):
        def w():
            logbox.config(state="normal")
            logbox.insert("end", time.strftime("%H:%M:%S ") + s + "\n")
            logbox.see("end")
            logbox.config(state="disabled")
        ui(w)

    server = Server(sim, log)
    api = WatchApi()
    pad = dict(padx=6, pady=3)

    # ---- left column: boat and scenario ----
    left = ttk.Frame(tab_sim)
    left.pack(side="left", fill="y", padx=8, pady=8)

    con = ttk.LabelFrame(left, text="NMEA server")
    con.pack(fill="x")
    ttk.Label(con, text=f"This PC: {local_ip()}").grid(row=0, column=0, columnspan=2, sticky="w", **pad)
    ttk.Label(con, text="Port").grid(row=1, column=0, sticky="w", **pad)
    port_v = tk.StringVar(value=str(args.port))
    ttk.Entry(con, textvariable=port_v, width=8).grid(row=1, column=1, sticky="w", **pad)
    srv_btn = ttk.Button(con, text="Start")
    srv_btn.grid(row=2, column=0, columnspan=2, sticky="ew", **pad)
    srv_info = ttk.Label(con, text="stopped")
    srv_info.grid(row=3, column=0, columnspan=2, sticky="w", **pad)

    def toggle_server():
        if server.running:
            server.stop()
            srv_btn.config(text="Start")
            log("server stopped")
        else:
            try:
                server.start(int(port_v.get()))
            except (OSError, ValueError) as e:
                busy = isinstance(e, OSError) and e.errno in (98, 10048, 10013)
                log(f"cannot start: port {port_v.get()} is already used (another simulator still open?)" if busy else f"cannot start: {e}")
                return
            srv_btn.config(text="Stop")
            log(f"server on {local_ip()}:{port_v.get()}")
    srv_btn.config(command=toggle_server)

    sc = ttk.LabelFrame(left, text="Scenario")
    sc.pack(fill="x", pady=6)
    scn_v = tk.StringVar(value=sim.scn.title)
    box = ttk.Combobox(sc, textvariable=scn_v, values=[s.title for s in SCENARIOS], state="readonly", width=30)
    box.pack(fill="x", **pad)
    desc = tk.Label(sc, text=sim.scn.text, wraplength=270, justify="left", fg="#4E6D83")
    desc.pack(fill="x", **pad)

    def pick(_=None):
        s = next(x for x in SCENARIOS if x.title == scn_v.get())
        sim.tws_user = sim.twd_user = sim.manual_depth = None
        sim.mute = False
        sim.reset(s)
        desc.config(text=s.text)
        log(f"scenario: {s.title}")
        sync_sliders()
    box.bind("<<ComboboxSelected>>", pick)

    ctl = ttk.LabelFrame(left, text="Boat")
    ctl.pack(fill="x")
    twd_v, tws_v, dep_v = tk.DoubleVar(), tk.DoubleVar(), tk.DoubleVar(value=8)
    dep_auto = tk.BooleanVar(value=True)

    def slider(row, label, var, lo, hi, cmd):
        ttk.Label(ctl, text=label).grid(row=row, column=0, sticky="w", **pad)
        sc_ = tk.Scale(ctl, variable=var, from_=lo, to=hi, orient="horizontal", length=170, resolution=0.5)
        sc_.grid(row=row, column=1, **pad)
        # only real user moves count (a command= callback would also fire when the program sets the value)
        sc_.bind("<B1-Motion>", cmd)
        sc_.bind("<ButtonRelease-1>", cmd)

    def set_wind(_=None):
        sim.twd_user, sim.tws_user = twd_v.get(), tws_v.get()

    def set_depth(_=None):
        if _ is not None:
            dep_auto.set(False)   # moving the slider takes over from the scenario
        sim.manual_depth = None if dep_auto.get() else dep_v.get()

    slider(0, "Wind from (deg)", twd_v, 0, 359, set_wind)
    slider(1, "Wind speed (kn)", tws_v, 0, 60, set_wind)
    slider(2, "Depth (m)", dep_v, 0.5, 60, set_depth)
    ttk.Checkbutton(ctl, text="Depth from the scenario", variable=dep_auto, command=set_depth).grid(row=3, column=0, columnspan=2, sticky="w", **pad)

    def sync_sliders():
        twd_v.set(round(sim.twd))
        tws_v.set(round(sim.tws))
        dep_auto.set(True)
    sync_sliders()

    act = ttk.LabelFrame(left, text="Traffic and signal")
    act.pack(fill="x", pady=6)
    ttk.Button(act, text="Collision course now", command=lambda: (sim.add_collision(), log("collision target added"))).pack(fill="x", **pad)
    ttk.Button(act, text="Add a harmless target", command=sim.add_far).pack(fill="x", **pad)
    ttk.Button(act, text="Remove all targets", command=sim.clear_targets).pack(fill="x", **pad)
    mute_v = tk.BooleanVar()

    def set_mute():
        sim.mute = mute_v.get()
        log("signal lost: no data sent" if sim.mute else "signal back")
    ttk.Checkbutton(act, text="Signal lost (send nothing)", variable=mute_v, command=set_mute).pack(anchor="w", **pad)

    # ---- right column: targets, watch ----
    right = ttk.Frame(tab_sim)
    right.pack(side="left", fill="both", expand=True, padx=8, pady=8)

    tg = ttk.LabelFrame(right, text="AIS targets (red = the watch shows them in red)")
    tg.pack(fill="x")
    cols = ("name", "mmsi", "dist", "brg", "cpa", "tcpa")
    tree = ttk.Treeview(tg, columns=cols, show="headings", height=6)
    for c, w in zip(cols, (140, 90, 60, 50, 60, 60)):
        tree.heading(c, text=c.upper())
        tree.column(c, width=w, anchor="center")
    tree.tag_configure("red", foreground="#D63A3A")
    tree.pack(fill="x", **pad)
    own_lbl = ttk.Label(right, text="")
    own_lbl.pack(anchor="w", **pad)

    # ---------------- Tools tab: link to the real watch (network or USB), its address, screen mirror, console ----------------
    saved = load_settings()
    mode_v, host_v, pw_v = tk.StringVar(value=saved.get("mode", "net")), tk.StringVar(value=saved.get("host", "")), tk.StringVar(value=saved.get("password", ""))
    if mode_v.get() == "usb":
        mode_v.set("net")   # the USB link must be opened again by hand

    def remember():
        save_settings({"mode": mode_v.get(), "host": host_v.get().strip(), "password": pw_v.get()})
    live_v, busy, dev = tk.BooleanVar(value=True), {"n": False}, {"link": None, "img": None, "ip": ""}
    try:
        from serial.tools import list_ports
    except ImportError:
        list_ports = None

    def console(s):
        def w():
            con_box.config(state="normal")
            con_box.insert("end", s + "\n")
            if int(con_box.index("end-1c").split(".")[0]) > 600:
                con_box.delete("1.0", "100.0")
            con_box.see("end")
            con_box.config(state="disabled")
        ui(w)

    def bg(fn, done):
        def run():
            try:
                r, e = fn(), None
            except Exception as ex:
                r, e = None, ex
            ui(lambda: done(r, e))
        threading.Thread(target=run, daemon=True).start()

    def usb_link():
        ln = dev["link"]
        if not ln or not ln.ok:
            raise RuntimeError("USB not connected: choose the port and press Connect")
        return ln

    def dev_status():
        if mode_v.get() == "usb":
            return usb_link().status()
        api.host, api.password = host_v.get().strip(), pw_v.get()
        if not api.host:
            raise RuntimeError("no watch IP yet")
        return api.status()

    def dev_screen():
        if mode_v.get() == "usb":
            return usb_link().screen()
        api.host, api.password = host_v.get().strip(), pw_v.get()
        img = Image.open(io.BytesIO(api.screen()))
        img.load()
        return img

    USB_KEYS = {"right": "U", "left": "D", "dtap": "B", "power": "P", "vibtest": "Y"}

    def dev_key(k):
        if mode_v.get() == "usb":
            if k not in USB_KEYS:
                raise RuntimeError(f"'{k}' works only over the network")
            usb_link().send(USB_KEYS[k])
            return
        api.host, api.password = host_v.get().strip(), pw_v.get()
        api.key(k)

    # 1. connection
    conn = ttk.LabelFrame(tab_tools, text="1. Connection to the watch")
    conn.pack(fill="x", padx=8, pady=(8, 4))
    r1 = ttk.Frame(conn)
    r1.pack(fill="x")
    net_row, usb_row = ttk.Frame(conn), ttk.Frame(conn)

    def show_mode():
        net_row.pack_forget()
        usb_row.pack_forget()
        (usb_row if mode_v.get() == "usb" else net_row).pack(fill="x")
        mirror_poll(now=True)
    ttk.Radiobutton(r1, text="Network (Wi-Fi)", variable=mode_v, value="net", command=show_mode).pack(side="left", **pad)
    ttk.Radiobutton(r1, text="USB cable", variable=mode_v, value="usb", command=show_mode).pack(side="left", **pad)
    ttk.Label(net_row, text="Watch IP").pack(side="left", **pad)
    ttk.Entry(net_row, textvariable=host_v, width=16).pack(side="left")
    ttk.Label(net_row, text="admin password").pack(side="left", **pad)
    ttk.Entry(net_row, textvariable=pw_v, show="*", width=14).pack(side="left")
    ttk.Label(net_row, text="(the IP fills in by itself when the watch connects to the NMEA server)", foreground="#4E6D83").pack(side="left", **pad)
    ttk.Label(usb_row, text="Port").pack(side="left", **pad)
    port_sel = ttk.Combobox(usb_row, width=44, state="readonly")
    port_sel.pack(side="left")

    def refresh_ports():
        if not list_ports:
            port_sel["values"] = ["pyserial missing: pip install pyserial"]
            port_sel.current(0)
            return
        ps = sorted(list_ports.comports(), key=lambda p: (p.vid != 0x303A, p.device))   # Espressif first
        port_sel["values"] = [f"{p.device} - {p.description}" for p in ps] or ["no serial port found"]
        port_sel.current(0)
    usb_btn = ttk.Button(usb_row, text="Connect", width=12)

    def usb_toggle():
        ln = dev["link"]
        if ln and ln.ok:
            ln.close()
            dev["link"] = None
            usb_btn.config(text="Connect")
            console("USB closed")
            return
        if not list_ports or " - " not in port_sel.get():
            console("USB: no port selected")
            return
        ln = SerialLink(console)
        try:
            ln.open(port_sel.get().split(" - ")[0])
        except Exception as e:
            console(f"USB: cannot open the port: {e}")
            return
        dev["link"] = ln
        usb_btn.config(text="Disconnect")
        console("USB open (if the watch restarts, wait ~20 s)")
        mirror_poll(now=True)
    usb_btn.config(command=usb_toggle)
    ttk.Button(usb_row, text="Refresh", command=refresh_ports).pack(side="left", **pad)
    usb_btn.pack(side="left", **pad)
    refresh_ports()

    # 2. device address
    inf = ttk.LabelFrame(tab_tools, text="2. Device address")
    inf.pack(fill="x", padx=8, pady=4)
    addr_lbl = tk.Label(inf, text="-", font=("Segoe UI", 18, "bold"), anchor="w")
    addr_lbl.pack(fill="x", padx=8, pady=(4, 0))
    det_lbl = ttk.Label(inf, text="not connected", justify="left")
    det_lbl.pack(anchor="w", padx=8, pady=2)
    ib = ttk.Frame(inf)
    ib.pack(fill="x")

    def copy_addr():
        if dev["ip"]:
            root.clipboard_clear()
            root.clipboard_append(dev["ip"])
            console(f"copied {dev['ip']}")

    def use_net():
        if dev["ip"]:
            host_v.set(dev["ip"])
            mode_v.set("net")
            show_mode()

    def point_here():
        try:
            usb_link().send(f"H{local_ip()}:{port_v.get()}\n")
            console(f"told the watch to read NMEA from {local_ip()}:{port_v.get()} (until it restarts)")
        except Exception as e:
            console(str(e))
    ttk.Button(ib, text="Copy address", command=copy_addr).pack(side="left", **pad)
    ttk.Button(ib, text="Use it for the network connection", command=use_net).pack(side="left", **pad)
    ttk.Button(ib, text="Open its web page", command=lambda: dev["ip"] and __import__("webbrowser").open(f"http://{dev['ip']}/")).pack(side="left", **pad)
    ttk.Button(ib, text="Point the watch at this simulator (USB)", command=point_here).pack(side="left", **pad)

    # 3. screen mirror + output
    low = ttk.Frame(tab_tools)
    low.pack(fill="both", expand=True, padx=8, pady=4)
    mir = ttk.LabelFrame(low, text="3. Screen of the watch")
    mir.pack(side="left", fill="y")
    img_lbl = tk.Label(mir, text="live screen copy needs Pillow (pip install pillow)" if not Image else "no image yet", bg="#101820", fg="#ccc", width=34, height=18)
    img_lbl.pack(padx=6, pady=6)
    mb = ttk.Frame(mir)
    mb.pack(fill="x")
    ttk.Checkbutton(mb, text="Live (while this tab is open)", variable=live_v).pack(side="left", **pad)

    def save_shot():
        if not dev["img"]:
            return
        from tkinter import filedialog
        f = filedialog.asksaveasfilename(defaultextension=".png", initialfile="watch_" + time.strftime("%Y%m%d_%H%M%S") + ".png")
        if f:
            dev["img"].save(f)
            console(f"saved {f}")
    ttk.Button(mb, text="Save PNG", command=save_shot).pack(side="left", **pad)
    kb = ttk.Frame(mir)
    kb.pack(fill="x")
    for i, (label, k) in enumerate((("Right", "right"), ("Left", "left"), ("Double tap", "dtap"), ("Power", "power"), ("Snooze", "boot"), ("Vib. test", "vibtest"))):
        ttk.Button(kb, text=label, width=9, command=lambda k=k: bg(lambda: dev_key(k), lambda r, e: e and console(f"key {k}: {e}"))).grid(row=i // 3, column=i % 3, padx=2, pady=2)

    out = ttk.LabelFrame(low, text="Output of the watch (USB log) and commands")
    out.pack(side="left", fill="both", expand=True, padx=(8, 0))
    con_box = tk.Text(out, height=10, state="disabled", font=("Consolas", 9), bg="#101820", fg="#B8D8E8")
    con_box.pack(fill="both", expand=True, padx=4, pady=4)
    cmd_v = tk.StringVar()
    cr = ttk.Frame(out)
    cr.pack(fill="x")
    cmd_e = ttk.Entry(cr, textvariable=cmd_v)
    cmd_e.pack(side="left", fill="x", expand=True, padx=4, pady=4)

    def send_cmd(_=None):
        c = cmd_v.get().strip()
        if not c:
            return
        try:
            usb_link().send(c + "\n")
            console("> " + c)
            cmd_v.set("")
        except Exception as e:
            console(str(e))
    cmd_e.bind("<Return>", send_cmd)
    ttk.Button(cr, text="Send", command=send_cmd).pack(side="left", padx=4)
    ttk.Label(out, text="USB commands: CSTATUS, CGET, S (screen), U D O L B K (buttons), H<ip>:<port> (NMEA server)", foreground="#4E6D83").pack(anchor="w", padx=4)

    def show_result(s, img):
        ip = s.get("ip") or ""
        ap = s.get("ap_ip") or ""
        dev["ip"] = ip or ap
        addr_lbl.config(text=ip or (f"{ap}  (its own Wi-Fi \"{s.get('ap')}\")" if ap else "not on Wi-Fi"))
        d = [f"{s.get('name', '')}  fw {s.get('fw')}", f"Wi-Fi: {s.get('wifi')}" + (f"  \"{s.get('ssid')}\"  {s.get('rssi')} dBm" if ip else ""),
             f"NMEA server: {s.get('host')}:{s.get('port')}  " + ("connected" if s.get("server") else "NOT connected"),
             f"battery {s.get('batt', '--')}%" + (" charging" if s.get("chg") else "")]
        if s.get("alert"):
            d.append("ALARM")
        elif s.get("snooze"):
            d.append(f"snoozed {s['snooze']} s")
        if s.get("vib", -1) >= 0:
            d.append(f"vibration GPIO {s['vib']}" + (" ON" if s.get("vibon") else ""))
        det_lbl.config(text="\n".join(d))
        if ip and not host_v.get():
            host_v.set(ip)
        if img is not None and Image:
            dev["img"] = img
            w, h = img.size
            ph = ImageTk.PhotoImage(img.resize((int(w * 1.5), int(h * 1.5))))
            img_lbl.config(image=ph, text="", width=ph.width(), height=ph.height())
            img_lbl.image = ph

    def mirror_poll(now=False):
        if mirror_poll.job:
            root.after_cancel(mirror_poll.job)
        usb = mode_v.get() == "usb"
        ready = (dev["link"] is not None and dev["link"].ok) if usb else bool(host_v.get().strip())
        on_view = nb.select() == str(tab_tools)   # the copy costs the watch ~0.5 s: only while someone looks at it
        if (now or (live_v.get() and on_view)) and ready and not busy["n"]:
            busy["n"] = True

            def work():
                s = dev_status()
                return s, (dev_screen() if Image else None)

            def done(r, e):
                busy["n"] = False
                if e:
                    det_lbl.config(text=f"{e}")
                else:
                    show_result(*r)
                    remember()
            bg(work, done)
        mirror_poll.job = root.after(3500 if usb else 3000, mirror_poll)
    mirror_poll.job = None
    show_mode()

    def tick():
        with sim.lock:
            o = sim.own()
            rows = [(t, *sim.risk(t)) for t in sim.targets]
            tn = sim.t
        own_lbl.config(text=f"t={int(tn)} s   SOG {o['sog']:.1f} kn  COG {o['cog']:.0f}  HDG {o['hdg']:.0f}   AWA {o['awa']:.0f}  AWS {o['aws']:.1f}   "
                            f"TWA {o['twa']:.0f}  TWS {o['tws']:.1f}   depth {o['depth']:.1f} m" + ("   [SIGNAL LOST]" if sim.silent() else ""))
        tree.delete(*tree.get_children())
        for t, dist, brg, cpa, tcpa in rows:
            if dist > 9:
                continue
            red = dist < 0.3 or (tcpa is not None and cpa < 0.3 and tcpa < 15)
            tree.insert("", "end", tags=("red",) if red else (), values=(t["name"] or "(MMSI only)", t["mmsi"], f"{dist:.2f}", f"{brg:.0f}",
                                                                       f"{cpa:.2f}", f"{tcpa:.1f}" if tcpa is not None else "-"))
        ips = server.watch_ips()
        if ips:
            if not host_v.get():
                host_v.set(ips[0])
            srv_info.config(text=f"watch connected: {ips[0]}")
        else:
            srv_info.config(text="running, no client" if server.running else "stopped")
        root.after(1000, tick)

    tick()
    if not args.no_autostart:
        toggle_server()   # the watch reconnects by itself: no need to press Start
    if args.scenario:
        scn_v.set(SCEN[args.scenario].title)
        pick()
    root.protocol("WM_DELETE_WINDOW", lambda: (remember(), server.stop(), root.destroy()))
    root.mainloop()


def run_headless(args):
    sim = Sim()
    sim.reset(SCEN[args.scenario or "corsica"])
    srv = Server(sim, lambda s: print(time.strftime("%H:%M:%S"), s, flush=True))
    srv.start(args.port)
    print(f"EssentialWatch simulator: {sim.scn.title} on {local_ip()}:{args.port}", flush=True)
    try:
        time.sleep(args.seconds)
    except KeyboardInterrupt:
        pass
    srv.stop()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="EssentialWatch simulator")
    ap.add_argument("--headless", action="store_true", help="no window: NMEA server only")
    ap.add_argument("--scenario", choices=list(SCEN), help="start with this scenario")
    ap.add_argument("--port", type=int, default=10112)
    ap.add_argument("--no-autostart", action="store_true", help="window only: do not start the NMEA server at launch")
    ap.add_argument("--seconds", type=float, default=3600, help="headless run time")
    a = ap.parse_args()
    run_headless(a) if a.headless else run_gui(a)
