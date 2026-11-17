"""Talks to the EssentialWatch over USB: sends configuration / test commands and prints the answers.

Usage examples (python tools/mob_test/watch_usb.py ...):
    status                      the status JSON, only the MOB / MOI / GPS fields (the pairing result is in CGET, field 'pair')
    CSTATUS                     the whole status JSON line
    CMOBVEC                     reference lines of the specification ($MOB vectors 5, 9, 10)
    CMOIVEC                     $MOI / $MOK vectors (13.3)
    CPAIRVEC                    pairing example (8.5): fixed keys, code 482913, id W7K2
    CPAIR 482913                start pairing with the code shown on the tablet
    CMOBRAISE / CMOBCANCEL      raise / cancel the MOB without the on-screen confirmation (test)
    CSET {"mobon":true,"mobhost":"192.168.2.143","mobid":"W7K2","mobkey":"<64 hex>","mobowner":"Test"}

IMPORTANT
  * Only one program can hold the USB port: close the watch simulator (Tools tab > disconnect) before running this, and
    do not run it while somebody is flashing the watch.
  * Opening the port RESTARTS the watch. The script waits --wait seconds (default 12) so that the Wi-Fi is up again.
    Several commands in one run share one connection (one restart): put them in the same call.
"""
import argparse
import json
import sys
import time

import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM5")
    ap.add_argument("--wait", type=float, default=12.0, help="seconds to wait after opening the port (watch restart + Wi-Fi)")
    ap.add_argument("--reply", type=float, default=2.0, help="seconds to listen after each command")
    ap.add_argument("commands", nargs="+", help="commands; 'status' = short MOB status; others are sent as they are")
    a = ap.parse_args()
    ser = serial.Serial(a.port, 115200, timeout=0.3)
    ser.dtr = False
    ser.rts = False
    time.sleep(a.wait)
    for c in a.commands:
        if c.lower() == "status":
            c = "CSTATUS"
            short = True
        else:
            short = False
        ser.reset_input_buffer()
        ser.write((c + "\n").encode())
        t0 = time.time()
        while time.time() - t0 < a.reply:
            line = ser.readline().decode("utf-8", "replace").strip()
            if not line or line.startswith(("PERF", "WiFi=", "Wind:", "NMEA:")):
                continue
            if short and line.startswith("STATUS"):
                d = json.loads(line[7:])
                keep = {k: v for k, v in d.items() if k.startswith(("mob", "moi", "gps")) or k in ("fw", "wifi", "ip", "batt")}
                print(json.dumps(keep, indent=1))
            else:
                print(line[:400])
    ser.close()


if __name__ == "__main__":
    sys.exit(main())
