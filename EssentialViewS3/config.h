#pragma once
// ---------------------------------------------------------------
//  EssentialView S3 - configurazione
//  Scheda: Waveshare ESP32-S3-Touch-AMOLED-2.06
//  (ESP32-S3, AMOLED 2,06" 410x502 QSPI CO5300, touch FT3168, PMU AXP2101)
// ---------------------------------------------------------------

#define FW_VERSION "2.2.1-s3"

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
#define DOUBLE_TAP_MS    400   // due tocchi entro questo tempo = giorno/notte
#define DOUBLE_TAP_PX    50    // ... e entro questa distanza
#define TAP_SLOP_PX      14    // oltre questa distanza non e' piu' un tocco fermo

#define NUM_PAGES 5
#define PAGE_AIS  3
#define PAGE_BATT 4

// Pulsanti fisici: BOOT = GPIO0 (basso = premuto), PWR = GPIO10 "SYS_OUT" (alto = premuto, secondo Waveshare)
#define BOOT_PIN 0
#define PWR_PIN  10
#define PWR_OFF_HOLD_MS 4000   // PWR tenuto premuto: spegne l'orologio (PMU AXP2101)
#define SNOOZE_MS 300000UL     // BOOT: silenzia l'allarme collisione per 5 minuti

// Allarme collisione: la scheda NON ha un motorino di vibrazione. Se ne colleghi uno (con transistor) a un GPIO libero,
// metti qui il numero del pin: vibra a impulsi mentre l'allarme e' attivo. -1 = nessun motorino (resta l'allarme a schermo).
#define VIB_PIN -1
#define ALERT_PULSE_MS 300
#define ALERT_EVERY_MS 1500

// PMU AXP2101 e RTC PCF85063 sul bus I2C condiviso con il touch
#define PMU_ADDR 0x34
#define RTC_ADDR 0x51

// Pagina AIS: stesse soglie di EssentialSail (CPA < 0,3 nm con TCPA < 15 min = rotta di collisione)
#define AIS_MAX_ROWS   5       // barche mostrate
#define AIS_CPA_NM     0.3f    // zona di guardia CPA
#define AIS_TCPA_MIN   15.0f   // zona di guardia TCPA
#define AIS_CLOSE_NM   0.3f    // sotto questa distanza la barca e' "molto vicina" (rossa anche se non converge)
#define AIS_WATCH_NM   3.0f    // oltre questa distanza (e senza rischio) la barca non viene elencata
#define AIS_STALE_MS   300000UL // posizione piu' vecchia di 5 minuti: barca ignorata

// Raggio degli angoli del pannello AMOLED (px): le schede vicine agli angoli lo seguono. Regola a occhio se serve.
#define PANEL_CORNER_R 110
