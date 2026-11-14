#pragma once
// ---------------------------------------------------------------
//  EssentialView S3 - configurazione
//  Scheda: Waveshare ESP32-S3-Touch-AMOLED-2.06
//  (ESP32-S3, AMOLED 2,06" 410x502 QSPI CO5300, touch FT3168, PMU AXP2101)
// ---------------------------------------------------------------

#define FW_VERSION "2.0.0-s3"

// Risoluzione logica dell'interfaccia (uguale al pannello, in verticale)
#define SCR_W 410
#define SCR_H 502

// Pin della scheda (da pin_config.h di Waveshare)
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
#define WAKE_PIN 0   // tasto BOOT: riaccende dopo "Power off"

// Valori di fabbrica. Senza rete WiFi (SSID vuoto) il dispositivo parte in modalita' configurazione:
// rete aperta SETUP_AP_NAME, pagina http://192.168.4.1, login admin / admin.
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASS ""
#define DEFAULT_NMEA_HOST ""      // IP del server NMEA, es. "192.168.2.144"
#define DEFAULT_NMEA_PORT 10110   // comuni: 10110, 2000, 4242

// NMEA via cavo: la scheda non ha RS485 a bordo, quindi solo Wi-Fi (server NMEA TCP).
#define NMEA_WIRED 0
#define NMEA_RX_PIN 18
#define NMEA_TX_PIN 17
#define NMEA_BAUD 4800

// Seriale USB (log + banco di prova sul PC: tasti virtuali). Stesso baud del firmware e-ink.
#define SERIAL_BAUD 460800

// Primo avvio (nessuna rete salvata): rete WiFi APERTA con questo nome
#define SETUP_AP_NAME "EssentialView Network"
// Rete di emergenza "EssentialView-XXXX" (con password): se la rete di bordo salvata non e' raggiungibile
#define CONFIG_AP_PASS "essentialview"
#define CONFIG_AP_AFTER_MS 30000
// Login della pagina web: utente fisso, password modificabile (da web, tool o USB)
#define WEB_USER "admin"
#define WEB_DEFAULT_PASS "admin"
#define DEFAULT_DEVICE_NAME "essentialview"

// Orientamento di default: 0 = orizzontale, 2 = ruotato di 180 gradi
#define SCREEN_ROTATION 0

// Aggiornamento schermo
#define REFRESH_MS      250    // controllo di ridisegno (il disegno avviene solo se i dati cambiano)
#define FULL_REDRAW_MS  5000   // ridisegno forzato ogni tot ms anche senza cambiamenti
#define DATA_STALE_MS   6000   // dopo questo tempo senza dati mostra "--"

// Tocco
#define LONG_PRESS_MS    1000  // pressione lunga (menu, conferma)
#define REPEAT_DELAY_MS  500   // UP/DOWN tenuti premuti: ripetizione dopo
#define REPEAT_EVERY_MS  130   // ... ogni quanto
#define SWIPE_MIN_PX     70   // scorrimento minimo per cambiare pagina
#define SWIPE_MAX_MS     900   // ... entro questo tempo
#define TAP_SLOP_PX      14    // oltre questa distanza non e' piu' un tocco fermo

#define NUM_PAGES 2
