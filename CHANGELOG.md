# Changelog

Every version is published as a GitHub Release (firmware .bin + these notes); the watch web page lists them.

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
