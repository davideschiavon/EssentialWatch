#pragma once
// ---------------------------------------------------------------
//  EssentialWatch S3 - configuration
//  Board: Waveshare ESP32-S3-Touch-AMOLED-2.06
//  (ESP32-S3, 2.06" 410x502 QSPI CO5300 AMOLED, FT3168 touch, AXP2101 PMU)
// ---------------------------------------------------------------

#define FW_VERSION "2.3.0-s3"

// Logical resolution of the interface (same as the panel, portrait)
#define SCR_W 410
#define SCR_H 502

// Board pins (from Waveshare pin_config.h)
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 11
#define LCD_CS 12
#define LCD_RESET 8
#define TP_SDA 15
#define TP_SCL 14
#define TP_INT 38
#define TP_RESET 9
#define TP_ADDR 0x38
#define WAKE_PIN 0   // BOOT button: wakes the board after "Power off"

// Factory values. Without a WiFi network (empty SSID) the device starts in configuration mode:
// open network SETUP_AP_NAME, page http://192.168.4.1, login admin / admin.
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASS ""
#define DEFAULT_NMEA_HOST ""      // IP of the NMEA server, e.g. "192.168.2.144"
#define DEFAULT_NMEA_PORT 10110   // common: 10110, 2000, 4242

// Wired NMEA: the board has no on-board RS485, so only WiFi (NMEA TCP server).
#define NMEA_WIRED 0
#define NMEA_RX_PIN 18
#define NMEA_TX_PIN 17
#define NMEA_BAUD 4800

// USB serial (log + PC test bench: virtual keys). Same baud rate as the e-ink firmware.
#define SERIAL_BAUD 460800

// First start (no saved network): OPEN WiFi network with this name
#define SETUP_AP_NAME "EssentialView Network"
// Emergency network "EssentialView-XXXX" (with password): if the saved boat network is not reachable
#define CONFIG_AP_PASS "essentialview"
#define CONFIG_AP_AFTER_MS 30000
// Web page login: fixed user, changeable password (from the web, the tool or USB)
#define WEB_USER "admin"
#define WEB_DEFAULT_PASS "admin"
#define DEFAULT_DEVICE_NAME "essentialview"

// Default orientation: 0 = normal, 2 = rotated by 180 degrees
#define SCREEN_ROTATION 0

// Screen refresh
#define REFRESH_MS      250    // redraw check (the drawing happens only if the data change)
#define FULL_REDRAW_MS  5000   // forced redraw every so many ms even without changes
#define DATA_STALE_MS   6000   // after this time without data it shows "--"

// Touch
#define LONG_PRESS_MS    1000  // long press (menu, confirm)
#define REPEAT_DELAY_MS  500   // UP/DOWN held pressed: repeat after
#define REPEAT_EVERY_MS  130   // ... every
#define SWIPE_MIN_PX     70   // minimum swipe distance to change page
#define SWIPE_MAX_MS     900   // ... within this time
#define DOUBLE_TAP_MS    400   // two taps within this time = day/night
#define DOUBLE_TAP_PX    50    // ... and within this distance
#define TAP_SLOP_PX      14    // beyond this distance it is no longer a still tap

#define NUM_PAGES 5
#define PAGE_AIS  3
#define PAGE_BATT 4

// Physical buttons: BOOT = GPIO0 (low = pressed), PWR = GPIO10 "SYS_OUT" (high = pressed, according to Waveshare)
#define BOOT_PIN 0
#define PWR_PIN  10
#define PWR_OFF_HOLD_MS 4000   // PWR held pressed: powers the watch off (AXP2101 PMU)
#define SNOOZE_MS 300000UL     // BOOT: silences the collision alarm for 5 minutes

// Collision alarm: the board has NO vibration motor. If you wire one (with a transistor) to a free GPIO,
// put the pin number here: it vibrates in pulses while the alarm is active. -1 = no motor (the on-screen alarm remains).
#define VIB_PIN -1
#define ALERT_PULSE_MS 300
#define ALERT_EVERY_MS 1500

// AXP2101 PMU and PCF85063 RTC on the I2C bus shared with the touch
#define PMU_ADDR 0x34
#define RTC_ADDR 0x51

// AIS page: same thresholds as EssentialSail (CPA < 0.3 nm with TCPA < 15 min = collision course)
#define AIS_MAX_ROWS   5       // boats shown
#define AIS_CPA_NM     0.3f    // CPA guard zone
#define AIS_TCPA_MIN   15.0f   // TCPA guard zone
#define AIS_CLOSE_NM   0.3f    // below this distance the boat is "very close" (red even if it is not converging)
#define AIS_WATCH_NM   3.0f    // beyond this distance (and without risk) the boat is not listed
#define AIS_STALE_MS   300000UL // position older than 5 minutes: boat ignored

// Corner radius of the AMOLED panel (px): the tiles near the corners follow it. Adjust by eye if needed.
#define PANEL_CORNER_R 110
