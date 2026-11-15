# EssentialWatch

A marine instrument watch: NMEA 0183 wind / speed / depth, AIS collision warning, clock and battery,
on the **Waveshare ESP32-S3-Touch-AMOLED-2.06** (410x502 AMOLED, FT3168 touch).
Ported from EssentialView (e-ink). Languages: English, French, Spanish (watch menu Settings > Language, or the web page).

## Pages (swipe left / right)

1. **Clock** - analog dial, date. Time from NTP, GPS (NMEA RMC/ZDA) or the battery-backed RTC.
2. **Wind** - compass dial (apparent wind = blue triangle, true wind = yellow triangle), AWS / TWS in the centre.
3. **SOG / Depth** - speed over ground and depth; the depth turns red below the minimum depth.
4. **AIS** - up to 5 boats in collision course or very close, with the distance next to the name. Red = collision course
   (CPA < 0.3 nm within 15 min) or closer than 0.3 nm. Names are decoded from AIS messages 5, 19 and 24.
5. **Battery** - battery level (top) and screen brightness (bottom: touch or drag to set it).

"SEGNALE PERSO" (orange watermark) appears when no NMEA data has arrived for a few seconds.
Double tap anywhere = day / night (red) theme. Long press = menu.

## Buttons

| Button | Action |
|---|---|
| **PWR** (short press) | screen on / off |
| **PWR** (hold 4 s) | power the watch off (a single click on PWR switches it on again) |
| **BOOT** | snooze the alarm for 5 minutes |

## Alarms

A collision warning (or a depth below the minimum depth) shows a flashing red frame, jumps to the AIS (or depth) page and
wakes the screen. BOOT snoozes it for 5 minutes. The board has a vibration motor on **GPIO18** (the default, per the Waveshare schematic); the pin can be changed on the web page ("Vibration motor GPIO", with a
"Test vibration" button) or with `VIB_PIN` in `EssentialViewS3/config.h`. It pulses (300 ms every 1.5 s) while the alarm is active; BOOT stops it.

## Simulator

`tools/watch_sim/` is a pretend boat for the watch: NMEA server with AIS targets and scenarios (coastal, Corsica collisions,
shallow shelf, storm, collision / shallow-water / signal-lost drills) and a panel that drives the real watch (live screen, keys, status).
See `tools/watch_sim/README.md`.

## Web page (port 80)

Login admin with **Digest authentication** (the password is never sent in clear text; Basic is refused). The factory password is `admin`
and **must be changed at the first login**: until then the page only accepts a new password. After 5 wrong passwords a client is locked
out for 30 s (doubling each time). The page is still plain HTTP (no encryption of the data). Without Wi-Fi the board starts the AP "EssentialView Network" at 192.168.4.1.
Settings: Wi-Fi, NMEA server (TCP, with a "Search NMEA server" network scan), minimum depth, time zone, **date and time**, orientation, name, password.
The screen is shown live with the buttons Right, Left, Double Tap, Power and Boot.

**Firmware update:** *Search firmware* lists the firmware published as **GitHub Releases** of this repository
(latest first), shows the changelog of the selected one and installs it: the watch downloads the file from GitHub itself
(HTTPS, SHA-256 checked), so it needs a Wi-Fi with Internet. Nothing is read from the `release/` folder.
Publishing a firmware (always with its changelog in the release notes):

```
gh release create vX.Y.Z release/EssentialWatch-fw-X.Y.Z-s3.bin --title "EssentialWatch X.Y.Z" --notes-file notes.md
```

## Build

- Firmware: `EssentialViewS3/` (Arduino, esp32 core 3.3.x, FQBN
  `esp32:esp32:esp32s3:PSRAM=opi,FlashSize=32M,PartitionScheme=app13M_data7M_32MB,FlashMode=qio,USBMode=hwcdc,CDCOnBoot=cdc`)
- Libraries: `libraries/` (Waveshare fork of GFX Library for Arduino)
- `arduino-cli compile --fqbn <FQBN above> --libraries libraries EssentialViewS3`
- Fonts: `python tools/make_fonts.py`; icons: `python tools/make_icons.py` (Material icons as used in EssentialSail)
- NMEA is received over Wi-Fi (TCP); the board has no RS485. Screen corner radius: `PANEL_CORNER_R` in `config.h`.

## License

The original code in this repository is released under the [MIT License](LICENSE).

Third-party components keep their own licenses and are **not** covered by the MIT license above:

- `libraries/GFX_Library_for_Arduino/` - Waveshare fork, see `libraries/LICENSE-waveshare.txt`.
- `EssentialViewS3/es_icons.h` - icons from Material Icons (Apache License 2.0), the same ones used in EssentialSail.
- `assets/fonts/DejaVuSans*.ttf` - DejaVu fonts (free license).
- `assets/fonts/Essentialsail*.ttf` and the generated `EssentialViewS3/es_fonts.h` - Essentialsail fonts, created by and copyright of Davide Schiavon, released under the [SIL Open Font License 1.1](assets/fonts/OFL.txt). The few symbols taken from DejaVu Sans keep the DejaVu license.

## Contributing

Contributions are welcome: open an issue to discuss an idea or a bug, or send a pull request.
By submitting a contribution you agree that it is released under the MIT License.
