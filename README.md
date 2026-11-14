# EssentialWatch

EssentialView (NMEA 0183 wind / speed / depth display) ported to the
**Waveshare ESP32-S3-Touch-AMOLED-2.06** (410x502 AMOLED, FT3168 touch).

- Firmware: `EssentialViewS3/` (Arduino, esp32 core 3.3.x, FQBN
  `esp32:esp32:esp32s3:PSRAM=opi,FlashSize=32M,PartitionScheme=app13M_data7M_32MB,FlashMode=qio,USBMode=hwcdc,CDCOnBoot=cdc`)
- Libraries: `libraries/` (Waveshare fork of GFX Library for Arduino)
- Ready-to-flash binary: `release/` (also usable from the web page's firmware update)
- Web configuration page on port 80 (default login admin/admin - change it);
  without Wi-Fi the board starts the AP "EssentialView Network" at 192.168.4.1.
- NMEA is received over Wi-Fi (TCP); the board has no RS485.

Build: `arduino-cli compile --fqbn <FQBN above> --libraries libraries EssentialViewS3`

## License

The original code in this repository is released under the [MIT License](LICENSE).

Third-party components keep their own licenses and are **not** covered by the MIT license above:

- `libraries/GFX_Library_for_Arduino/` - Waveshare fork, see `libraries/LICENSE-waveshare.txt`.
- `assets/fonts/DejaVuSans*.ttf` - DejaVu fonts (free license).
- `assets/fonts/Essentialsail*.ttf` and the generated `EssentialViewS3/es_fonts.h` - Essentialsail fonts, all rights remain with their owner.

## Contributing

Contributions are welcome: open an issue to discuss an idea or a bug, or send a pull request.
By submitting a contribution you agree that it is released under the MIT License.
