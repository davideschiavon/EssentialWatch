# Changelog

Every version is published as a GitHub Release (firmware .bin + these notes); the watch web page lists them.

## 2.8.0

- **Man overboard (MOB) link to EssentialSail** (Wi-Fi), following the EssentialSail MOB / Watch protocol version 1. Off by default: switch it on in the
  menu ("MOB messages") or on the web page. It adds a **MOB page** (last page): hold the screen to raise the alarm, hold twice to cancel.
  - The watch sends signed messages (HMAC-SHA256) over UDP to the tablet and checks the signed acknowledgement: "alive" every 5.5 s, the alarm
    every second until the tablet answers (then every 5.5 s), "Delivering / Delivered / NOT DELIVERED" on the page. The sequence number survives restarts.
  - **Owner name** (menu "MOB owner name" or web page) is sent to the tablet before the alarm and every minute.
  - **Pairing with the 6-digit code** shown by the tablet (menu "MOB pairing code" or web page, field "Pairing code"): the watch finds the tablet by itself
    (UDP broadcast), the two agree on a secret key (ECDH P-256 + HKDF) that never travels on the air, and the watch stores the tablet address, the device
    ID and the key. The key is never shown again and is not in the settings backup. You can also type the address, device ID and key by hand, or paste a
    pairing link (Settings > MOB Messages > Pairing link on the tablet).
  - **Boat to watch**: the tablet sends the distance and the direction of the boat (shown on the MOB page as an arrow, north up: the watch has no compass,
    and as metres and degrees; greyed out after 15 s) and, when the crew presses "Ring watch", a **siren**: the vibration motor pulses and the screen wakes
    (a tap mutes it for 20 s). The watch confirms every message to the tablet. The board has no speaker output used by the firmware: the siren is the motor.
  - **Search EssentialSail** (web page): scans the Wi-Fi network for the tablet's MOB port; "verified" when the watch is already paired and the tablet answers.
  - Optional **GPS module** (NMEA 0183, 3.3 V) on a free pad (web page: Device > GPS module RX pad and baud rate): the position of the wearer is shown on the
    web page (with a map link) and sent in the MOB messages. The board itself has no GPS.
  - USB test commands: `CMOBVEC`, `CMOIVEC`, `CPAIRVEC` (the reference vectors of the specification), `CPAIR <code>`, `CGPSFEED <nmea>`, `CMOBRAISE`, `CMOBCANCEL`.
- **Web page**: uses the Essentialsail fonts (served by the watch itself, no Internet needed) and square corners everywhere.
- **Fixed (2.7.0)**: the web page was empty (a script error) because two French texts had an unescaped apostrophe ("Veille de l'écran" and the screen
  timeout hint). Everything on the page, in every language, was affected.
- The clock page has a plain background (no gradient or glows).
- Simulator: button **Stop other simulators** (ends the other copies of the simulator still running in the background, which can keep the NMEA or the USB port busy).
- Docs and tools for testing the MOB link together with the EssentialSail side: `docs/MOB_TESTING.md` (test levels, scenario matrix, who touches what, report format),
  `tools/mob_test/` (`watch_usb.py`, `fake_tablet.py`, `fake_pair_tablet.py`).

## 2.7.0

- **Elegant Black theme**, inspired by the dark theme of EssentialSail (EssentialAIS): near-black green background, graphite cards,
  off-white text, orange accent, amber warnings. Three themes now: Day, Elegant Black, Night (red only). Choose it in the watch menu
  (Settings > Theme) or on the web page (Display > Theme); double tap (or BACK on the dashboard) cycles day > Elegant Black > night.
  Old configurations and backups with `night` keep working.
- **Screen timeout**: the screen turns off after 30 s (default) without touch or buttons: Off / 15 s / 30 s / 1 min / 2 min / 5 min
  (menu "Screen timeout", web page Display). It never turns off during a collision or depth alarm, and menus wait at least 2 minutes.
- **Raise to wake**: the QMI8658 accelerometer wakes the screen when the watch is raised to be looked at (screen facing up or down, watch still
  for 0.35 s, after being lowered). A watch lying still on a table does not wake by itself; touch, a button or an alarm always wake it.
  Switch it off in the menu ("Raise to wake") or on the web page.
- Status JSON: `screen` (on/off), `imu` (sensor found) and `g` (acceleration x, y, z in g). USB command `Z` wakes the screen (test).

## 2.6.0

- **Languages**: English, French and Spanish, on the watch (pages, settings, messages, day and month names,
  O for West on the compass) and on the web page. Choose it in the watch menu (Settings > Language, applied at once)
  or on the web page (selector at the top, or Display > Language). The page follows the language of the watch.
  The letters the Essentialsail fonts lack (ç ñ á í ó ú ê â ô ü ¿ ¡ ...) come from DejaVu Sans.
- The "signal lost" watermark is now translated ("SIGNAL LOST", "SIGNAL PERDU", "SEÑAL PERDIDA"); it was always in Italian.
- NMEA server: a connection that stays silent for 15 s is dropped and made again. A server that disappeared without
  closing the connection (PC switched off, simulator closed) left the watch "connected" to nothing, so it never
  reached the new server.
- Fixed (2.4.1): the settings menu had more rows than the screen, so "Power off" and "Exit without saving" were
  hidden under the keys. The menu now scrolls (arrows show more rows).
- Fixed (2.4.1): editing "Collision CPA" / "Collision TCPA" read a title outside its table (garbage or a crash).
- Web page: the vibration hint said the board has no motor (it is on GPIO 18); the screen copy text says 10 s.
- Simulator: a screen copy damaged on the USB link is discarded instead of being shown with shifted colours.
- Simulator: its NMEA server could stop accepting connections (an error in the accepting thread, which wrote to the
  window from a background thread): the port stayed open but the watch could not connect. Window updates now go
  through a queue, client errors are caught, and a second simulator on the same port now reports "port already
  used" instead of sharing it silently.

## 2.5.0

Smoother page changes. Measured on the watch (USB "PERF" log line, every 5 s):

- Dashboard drawing time per frame: clock page 78 -> 23 ms, AIS page 138 -> 63 ms (the panel transfer stays at 44 ms).
  - The background gradient and glows are drawn once per theme and then copied (they were redrawn at every frame).
  - Faster colour blending for the glass tiles and the anti-aliased text (one multiply instead of six per pixel).
  - Glass rectangles walk the framebuffer row by row.
- The NMEA server connection no longer freezes the watch: while the server was unreachable each retry blocked everything for 0.5 s every 3 s. The connection now runs in the background; after 5 failures it retries every 10 s.
- Screen copy (`/screen.bmp`) sent in 8 KB blocks (about 20% faster). The web page now asks for the half-size copy every 10 s (it fetched the full-size one every 5 s, keeping the watch busy for seconds).
- Simulator Tools tab: the live screen copy runs only while the Tools tab is open.
- USB log: a `PERF` line reports drawing, panel transfer, worst loop and network times.

## 2.4.1

- Vibration: the board **does** have a motor. The Waveshare schematic routes it to **GPIO18** ("MOTOR" net), so GPIO18 is now the default `VIB_PIN` (2.4.0 wrongly said the board had no motor). Alarms pulse it 300 ms every 1.5 s; BOOT snoozes it. The web page "Test vibration" button checks it.
- **Collision threshold** in the settings (watch menu "Collision CPA" / "Collision TCPA" and web page): a boat that will pass closer than the CPA (default 0.3 nm) within the TCPA (default 15 min), or is already closer than the CPA, turns red and starts the alarm and the vibration.
- Simulator: new **Tools** tab (USB or network link, device address, screen mirror, USB log); it remembers the watch IP and password.
- USB command `Y` runs the vibration test (used by the simulator Tools tab).

## 2.4.0

- Web login hardening: HTTP **Digest** authentication replaces Basic (the password no longer travels in clear text); clients that only speak Basic (old scripts: use `curl --digest`) are refused.
- The factory password `admin` must be changed at the first login: until then only a new password can be saved (the new password cannot be `admin`).
- After 5 wrong passwords a client IP is locked out for 30 s, doubling at each new lock-out (up to about 16 minutes).
- Vibration: the output pin of an external motor is now set from the web page (GPIO 1, 2, 3, 17, 18, 21, 43, 44, 47, 48) with a "Test vibration" button. The pulses were verified against a simulated collision (300 ms every 1.5 s, stopped by the BOOT snooze). The board itself has no motor.
- Status page JSON reports `vib` (pin) and `vibon` (pulse active).

## 2.3.0

- Web page: **Search NMEA server** scans the WiFi network of the watch (ports 10110, 10111, 10112, 2000, 4242, 3000), checks that the port really sends NMEA sentences and fills in the IP address and port.
- Firmware search: the watch now downloads the firmware from GitHub itself (certificate checked, SHA-256 verified), because browsers cannot download GitHub release files (CORS). The watch needs a WiFi with Internet.
- All code comments are now in English.

## 2.2.1

- Battery page: the percentage inside the battery icon is 50% smaller.

## 2.2.0

**New pages** (swipe left / right)
- Clock: analog dial, small date. Time from NTP, GPS (NMEA RMC/ZDA) or the battery-backed RTC.
- Wind: no more boat drawing; AWS and TWS in the centre, legend (blue triangle = apparent wind, yellow triangle = true wind).
- SOG / Depth: two large widgets (the depth graph was removed).
- AIS: up to 5 boats in collision course or very close, distance next to the name (or MMSI). Red = collision course
  (CPA < 0.3 nm within 15 min) or closer than 0.3 nm. Names decoded from AIS messages 5, 19 and 24.
- Battery: battery icon with the percentage (top) and screen brightness (bottom, touch or drag to set it).

**Look**
- Translucent "glass" widgets over a soft gradient background (the red night theme stays solid, as before).
- Widget corners follow the curved corners of the screen. Title bar removed; orange "SEGNALE PERSO" watermark when the signal is lost.
- Double tap anywhere = day / night theme.

**Alarms and buttons**
- Collision warning and minimum-depth alarm: flashing red frame, jumps to the right page, wakes the screen.
- PWR: short press = screen on / off, hold 4 s = power off. BOOT = snooze the alarm for 5 minutes.
- The board has no vibration motor: set `VIB_PIN` in `config.h` if you wire an external one.

**Web page**
- Title is now EssentialWatch. Buttons: Right, Left, Double Tap, Power, Boot.
- Set the date and time (kept by the RTC); minimum depth setting; battery and time in the status.
- Firmware: "Search firmware" reads the GitHub Releases, drop-down with the latest first and the changelog of the selected one.

## 2.0.0

- First port of EssentialView to the Waveshare ESP32-S3-Touch-AMOLED-2.06 (wind and depth pages, web configuration).
