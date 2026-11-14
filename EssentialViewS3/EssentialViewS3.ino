// EssentialView S3 - Waveshare ESP32-S3-Touch-AMOLED-2.06 (AMOLED 410x502 colori, touch FT3168)
// Porting del firmware e-ink (firmware/src/main.cpp) per il display a colori:
// - stesse frasi NMEA (WiFi TCP, opzionale RS485), stesse regole delle vele, stessi menu e pagina web;
// - pulsanti fisici sostituiti dal touch: tasti virtuali in basso nei menu,
//   nella dashboard swipe = cambio pagina, pressione lunga = menu, tasto DAY/NIGHT in alto;
// - disegno su buffer fuori schermo (Arduino_Canvas), copiato intero sul pannello: niente sfarfallio.
// Pagina 1: quadrante vento (AWA/TWA) + AWS/TWS/SOG.  Pagina 2: profondita' + storico 30 min + allarme.

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <math.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include "config.h"
#include "es_fonts.h"  // font Essentialsail: tipo EsFont usato dai prototipi generati da Arduino

// Tipi usati dai prototipi generati da Arduino: vanno dichiarati prima di tutto il resto
enum Ev { EV_NONE = 0, EV_SHORT, EV_LONG, EV_REPEAT };
enum Mode { M_DASH, M_MENU, M_EDIT, M_SCAN, M_DIAG, M_SETUP, M_FACTORY };
enum EditT { E_PASS, E_HOST, E_PORT, E_ALARM };

// ============================== Schermo e colori ==========================
Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *lcd = new Arduino_CO5300(bus, LCD_RESET, 0 /* rotazione: verticale */, false /* ips: niente inversione */, SCR_W, SCR_H,
                                         22 /* col_offset1 */, 0, 0, 0);
// Arduino_Canvas alloca con aligned_alloc(), che sull'S3 puo' finire nella RAM interna (troppo piccola per 400 KB).
// Questa versione mette il buffer in PSRAM.
class PsramCanvas : public Arduino_Canvas {
 public:
  PsramCanvas(int16_t w, int16_t h, Arduino_G *out, uint8_t rot) : Arduino_Canvas(w, h, out, 0, 0, rot) {}
  bool initBuffer() {
    const size_t bytes = (size_t)SCR_W * SCR_H * 2;
    _framebuffer = (uint16_t *)heap_caps_aligned_alloc(16, bytes, MALLOC_CAP_SPIRAM);
    if (_framebuffer) memset(_framebuffer, 0, bytes);
    return _framebuffer != nullptr;
  }
};
Arduino_Canvas *cv = nullptr;  // buffer di disegno: si crea in setup()

static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Colori dei colori di essentialsail.com (come il tool PC). Giorno: fondo chiaro.
// Notte: fondo scuro e tonalita' piu' tenui, per non abbagliare di notte.
struct Theme {
  uint16_t bg, card, fg, mfg, border, pri, acc, ok, bad, sail, hull;
};
const Theme DAY_THEME = {
  rgb565(242, 250, 254), rgb565(255, 255, 255), rgb565(6, 35, 61), rgb565(78, 109, 131),
  rgb565(197, 219, 233), rgb565(0, 135, 243), rgb565(0, 170, 170), rgb565(10, 143, 77),
  rgb565(214, 58, 58), rgb565(255, 246, 222), rgb565(0, 27, 58),
};
const Theme NIGHT_THEME = {
  // solo rosso (nessun altro colore), per non rovinare la vista notturna: la distinzione e' nella luminosita'
  rgb565(12, 0, 0), rgb565(30, 3, 3), rgb565(255, 70, 70), rgb565(170, 38, 38),
  rgb565(84, 14, 14), rgb565(225, 38, 38), rgb565(255, 120, 120), rgb565(150, 26, 26),
  rgb565(255, 50, 50), rgb565(58, 8, 8), rgb565(200, 48, 48),
};
Theme TH = DAY_THEME;  // tema attivo (applyTheme)

// ============================== Impostazioni ==============================
Preferences prefs;
struct Settings {
  String ssid, pass, host;
  uint16_t port = 10110;
  float depthAlarm = 2.0f;
  bool night = false;
  String name;        // nome del dispositivo: http://<name>.local e rete di configurazione
  uint8_t rot = 0;    // rotazione schermo: 0 o 2 (180 gradi)
  String webpass;     // password della pagina web (utente WEB_USER), mai vuota
} cfg;

void loadCfg() {
  prefs.begin("essview", true);
  cfg.ssid = prefs.getString("ssid", DEFAULT_WIFI_SSID);
  cfg.pass = prefs.getString("pass", DEFAULT_WIFI_PASS);
  cfg.host = prefs.getString("host", DEFAULT_NMEA_HOST);
  cfg.port = prefs.getUShort("port", DEFAULT_NMEA_PORT);
  cfg.depthAlarm = prefs.getFloat("dalarm", 2.0f);
  cfg.night = prefs.getBool("night", false);
  cfg.name = prefs.getString("name", DEFAULT_DEVICE_NAME);
  cfg.rot = prefs.getUChar("rot", SCREEN_ROTATION);
  cfg.webpass = prefs.getString("webpass", WEB_DEFAULT_PASS);
  if (!cfg.webpass.length()) cfg.webpass = WEB_DEFAULT_PASS;
  prefs.end();
}
void saveCfg() {
  prefs.begin("essview", false);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  prefs.putString("host", cfg.host);
  prefs.putUShort("port", cfg.port);
  prefs.putFloat("dalarm", cfg.depthAlarm);
  prefs.putBool("night", cfg.night);
  prefs.putString("name", cfg.name);
  prefs.putUChar("rot", cfg.rot);
  prefs.putString("webpass", cfg.webpass);
  prefs.end();
}
void applyTheme() { TH = cfg.night ? NIGHT_THEME : DAY_THEME; }

// Font Essentialsail (anti-aliasing) e simbolo del grado in UTF-8
#define DEG "Â°"
#include "es_text.h"

// ============================== Dati di bordo =============================
struct Val {
  float v = NAN;
  uint32_t t = 0;
  void set(float x) { v = x; t = millis(); }
  bool ok(uint32_t maxAge = DATA_STALE_MS) const { return !isnan(v) && (millis() - t) < maxAge; }
};
Val awa, aws, twaS, twsS, sog, stw, hdg, depth, keel;  // gradi, nodi, metri
uint32_t hdtT = 0;  // ultima HDT ricevuta: la prua vera ha la precedenza su HDG/HDM

const int HN = 180;  // 30 minuti a 1 campione / 10 s
float hist[HN];
uint32_t lastHist = 0;

static float normAngle(float a) {
  while (a > 180) a -= 360;
  while (a < -180) a += 360;
  return a;
}

// Vento reale: dal server se c'e', altrimenti calcolato da apparente + velocita' sull'acqua (STW).
// Senza nessuna velocita' non c'e' vento reale: mostrare l'apparente sarebbe sbagliato.
bool getTrue(float &a, float &s) {
  if (twaS.ok(10000) && twsS.ok(10000)) { a = twaS.v; s = twsS.v; return true; }
  if (!aws.ok() || !awa.ok()) return false;
  if (!stw.ok() && !sog.ok()) return false;
  float bs = stw.ok() ? stw.v : sog.v;
  float r = awa.v * DEG_TO_RAD;
  float x = aws.v * cosf(r) - bs, y = aws.v * sinf(r);
  s = sqrtf(x * x + y * y);
  a = atan2f(y, x) * RAD_TO_DEG;
  return true;
}

// ============================== NMEA 0183 =================================
// Sentences: MWV, VWR, VWT (vento) - VTG, RMC (SOG) - VHW (STW) - HDT, HDG, HDM (prua) - DBT, DPT, DBK (fondo)
WiFiClient nmea;
bool srvUp = false;
uint32_t nRx = 0, nOk = 0, nBadCs = 0, nUnknown = 0, nConnTry = 0;
String lastLine;
uint32_t lastConnTry = 0;
String lineBuf;
const float KMH2KN = 0.539957f, MS2KN = 1.943844f, MPH2KN = 0.868976f;

static float toKnots(float v, char unit) {
  switch (unit) {
    case 'M': return v * MS2KN;
    case 'K': return v * KMH2KN;
    case 'S': return v * MPH2KN;
    default: return v;  // 'N' nodi
  }
}

static float wrap360(float d) {
  while (d < 0) d += 360;
  while (d >= 360) d -= 360;
  return d;
}

static bool nmeaChecksumOk(const String &l) {
  if (l.length() < 6 || l[0] != '$') return false;
  int star = l.indexOf('*');
  if (star < 0) return true;  // senza checksum: accetta
  uint8_t c = 0;
  for (int i = 1; i < star; i++) c ^= (uint8_t)l[i];
  return (uint8_t)strtol(l.substring(star + 1, star + 3).c_str(), nullptr, 16) == c;
}

static int splitFields(const String &l, String *f, int maxF) {
  int end = l.indexOf('*');
  if (end < 0) end = l.length();
  int n = 0, start = 1;
  while (n < maxF && start <= end) {
    int c = l.indexOf(',', start);
    if (c < 0 || c > end) c = end;
    f[n++] = l.substring(start, c);
    start = c + 1;
  }
  return n;
}

void handleNmea(const String &raw) {
  nRx++;
  lastLine = raw.substring(0, 70);
  String l = raw;
  int d0 = l.indexOf('$');
  if (d0 > 0) l = l.substring(d0);
  if (!nmeaChecksumOk(l)) { nBadCs++; return; }
  String f[16];
  int n = splitFields(l, f, 16);
  if (n < 2 || f[0].length() < 5) return;
  String t = f[0].substring(f[0].length() - 3);
  auto has = [&](int i) { return i < n && f[i].length() > 0; };
  bool known = (t == "MWV" || t == "VWR" || t == "VWT" || t == "VTG" || t == "RMC" || t == "VHW" || t == "HDT" ||
                t == "HDG" || t == "HDM" || t == "DBT" || t == "DPT" || t == "DBK");
  if (known) nOk++; else nUnknown++;

  if (t == "MWV") {                    // angolo, R/T, velocita', unita', stato
    if (!has(1) || !has(2) || !has(3)) return;
    if (has(5) && f[5][0] != 'A') return;
    float a = normAngle(f[1].toFloat());
    float sp = toKnots(f[3].toFloat(), has(4) ? f[4][0] : 'N');
    if (f[2][0] == 'R') { awa.set(a); aws.set(sp); }
    else { twaS.set(a); twsS.set(sp); }
  } else if (t == "VWR" || t == "VWT") {  // angolo 0-180, L/R, nodi
    if (!has(1) || !has(2) || !has(3)) return;
    float a = f[1].toFloat();
    if (f[2][0] == 'L') a = -a;
    a = normAngle(a);
    float sp = f[3].toFloat();
    if (t == "VWR") { awa.set(a); aws.set(sp); }
    else { twaS.set(a); twsS.set(sp); }
  } else if (t == "VTG") {             // SOG nodi = campo 5
    if (has(5)) sog.set(f[5].toFloat());
  } else if (t == "RMC") {             // SOG nodi = campo 7
    if (has(2) && f[2][0] == 'A' && has(7)) sog.set(f[7].toFloat());
  } else if (t == "VHW") {             // hdgT,T,hdgM,M,stw,N,stw,K
    if (has(5)) stw.set(f[5].toFloat());
    else if (has(7)) stw.set(f[7].toFloat() * KMH2KN);
  } else if (t == "HDT") {
    if (has(1)) { hdg.set(wrap360(f[1].toFloat())); hdtT = millis(); }
  } else if (t == "HDG" || t == "HDM") {  // magnetica: solo se manca la HDT
    if (has(1) && (hdtT == 0 || millis() - hdtT > DATA_STALE_MS)) hdg.set(wrap360(f[1].toFloat()));
  } else if (t == "DBT") {             // piedi, f, metri, M
    if (has(3)) depth.set(f[3].toFloat());
    else if (has(1)) depth.set(f[1].toFloat() * 0.3048f);
  } else if (t == "DPT") {             // profondita' m, offset m
    if (has(1)) {
      float d = f[1].toFloat();
      depth.set(d);
      if (has(2)) {
        float off = f[2].toFloat();
        if (off < 0) keel.set(d + off);  // offset negativo = trasduttore-chiglia
      }
    }
  } else if (t == "DBK") {             // profondita' sotto chiglia, metri = campo 3
    if (has(3)) keel.set(f[3].toFloat());
  }
}

void netLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    srvUp = false;
    nmea.stop();
    return;
  }
  if (!nmea.connected()) {
    srvUp = false;
    uint32_t now = millis();
    if (cfg.host.length() && now - lastConnTry > 3000) {
      lastConnTry = now;
      nConnTry++;
      if (nmea.connect(cfg.host.c_str(), cfg.port, 500)) {
        srvUp = true; lineBuf = "";
        Serial.printf("NMEA: connected to %s:%u\n", cfg.host.c_str(), cfg.port);
      } else {
        Serial.printf("NMEA: connect to %s:%u failed (try %lu)\n", cfg.host.c_str(), cfg.port, (unsigned long)nConnTry);
      }
    }
    return;
  }
  srvUp = true;
  int guard = 0;
  while (nmea.available() && guard++ < 600) {
    char c = (char)nmea.read();
    if (c == '\n' || c == '\r') {
      if (lineBuf.length()) { handleNmea(lineBuf); lineBuf = ""; }
    } else if (lineBuf.length() < 120) {
      lineBuf += c;
    }
  }
}

#if NMEA_WIRED
String serBuf;
void serialLoop() {
  int guard = 0;
  while (Serial1.available() && guard++ < 400) {
    char c = (char)Serial1.read();
    if (c == '\n' || c == '\r') {
      if (serBuf.length()) { handleNmea(serBuf); serBuf = ""; }
    } else if (serBuf.length() < 120) {
      serBuf += c;
    }
  }
}
#endif

// ============================== Tasti virtuali e tocco ====================
// Un tasto: breve al rilascio, lungo dopo LONG_PRESS_MS; UP/DOWN ripetono se tenuti premuti.
struct Btn {
  bool repeat;
  bool stable = false, lastRaw = false, longFired = false;
  uint32_t tChange = 0, tPress = 0, tRep = 0;
  explicit Btn(bool r) : repeat(r) {}
  Ev poll(bool raw) {
    uint32_t now = millis();
    if (raw != lastRaw) { lastRaw = raw; tChange = now; }
    Ev ev = EV_NONE;
    if (now - tChange >= 25 && raw != stable) {
      stable = raw;
      if (stable) {
        tPress = now; tRep = now; longFired = false;
        if (repeat) ev = EV_SHORT;
      } else if (!repeat && !longFired) {
        ev = EV_SHORT;
      }
    } else if (stable) {
      if (repeat) {
        if (now - tPress > REPEAT_DELAY_MS && now - tRep > REPEAT_EVERY_MS) { tRep = now; ev = EV_REPEAT; }
      } else if (!longFired && now - tPress >= LONG_PRESS_MS) {
        longFired = true; ev = EV_LONG;
      }
    }
    return ev;
  }
};
Btn bUp(true), bDown(true), bOk(false), bBack(false);

// server NMEA temporaneo del banco di prova (comando "H"): non viene mai salvato
bool hostTemp = false;
String savedHost;
uint16_t savedPort = 0;
void configCmd(const String &c);  // webcfg.h

// stato del tocco
bool tTouching = false, tWasDown = false, tMoved = false, tLongFired = false;
int tX = 0, tY = 0, tX0 = 0, tY0 = 0;
uint32_t tDown = 0;
int tKey = -1;  // tasto virtuale premuto (0 UP, 1 DOWN, 2 OK, 3 BACK), -1 = nessuno
bool tpOk = false;

const int BAR_H = 40;        // barra di stato in alto (dashboard)
const int HEADER_H = 44;     // intestazione dei menu
const int KEYBAR_Y = 446;    // tasti virtuali in basso (menu, modifica, scansione, diagnostica)
const int NIGHT_X = SCR_W - 92, NIGHT_Y = 6, NIGHT_W = 80, NIGHT_H = 28;

// Touch FT3168 (I2C 0x38): reset, poi lettura dei registri 0x02..0x06 (numero di tocchi, X, Y)
void touchInit() {
  pinMode(TP_RESET, OUTPUT);
  digitalWrite(TP_RESET, LOW);
  delay(20);
  digitalWrite(TP_RESET, HIGH);
  delay(60);
  Wire.begin(TP_SDA, TP_SCL, 400000);
  Wire.beginTransmission(TP_ADDR);
  tpOk = (Wire.endTransmission() == 0);
}

// Legge il touch: coordinate del punto premuto (con rotazione applicata)
void readTouch() {
  tTouching = false;
  if (!tpOk) return;
  Wire.beginTransmission(TP_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom((int)TP_ADDR, 5) != 5) return;
  uint8_t b[5];
  for (int i = 0; i < 5; i++) b[i] = Wire.read();
  if ((b[0] & 0x0F) == 0 || (b[0] & 0x0F) > 5) return;
  int px = ((b[1] & 0x0F) << 8) | b[2];
  int py = ((b[3] & 0x0F) << 8) | b[4];
  if (cfg.rot == 2) { px = SCR_W - 1 - px; py = SCR_H - 1 - py; }
  tX = constrain(px, 0, SCR_W - 1);
  tY = constrain(py, 0, SCR_H - 1);
  tTouching = true;
}

// ============================== Stato UI ==================================
Mode mode = M_DASH;
int page = 0;
int menuSel = 0;
bool dirty = true;
uint32_t lastRender = 0, lastSig = 0;

// editor testo
EditT editT = E_PASS;
String editBuf;
const char *editSet;
int curIdx = 0;
size_t editMax = 63;
const char CS_FULL[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_.,:;!?@#$%&*()+=/\\'\"<>[]{}|~^`";
const char CS_NUM[] = "0123456789.";
const char *EDIT_TITLES[] = {"WiFi password", "NMEA server IP", "Server port", "Depth alarm (m)"};

// scansione WiFi
String scanSsid[12];
int scanRssi[12];
int scanN = 0, scanSel = 0;

// ============================== Helper di disegno =========================
static inline void pol(float cx, float cy, float r, float deg, int &x, int &y) {
  float a = deg * DEG_TO_RAD;
  x = lroundf(cx + r * sinf(a));
  y = lroundf(cy - r * cosf(a));
}

// Font piu' piccolo (per far stare un valore in una scheda)
static const EsFont *smaller(const EsFont *f) {
  if (f == &F_NUM_XL) return &F_NUM_L;
  if (f == &F_NUM_L) return &F_NUM_M;
  if (f == &F_NUM_M) return &F_HEAD_M;
  if (f == &F_HEAD_M) return &F_HEAD;
  if (f == &F_HEAD_L) return &F_HEAD;
  return nullptr;
}

// Scheda: etichetta in alto a sinistra, valore centrato, unita' in basso a destra.
// Il valore usa il font dato e passa a quelli piu' piccoli finche' ci sta nella scheda.
void tile(int x, int y, int w, int h, const char *label, const String &val, const char *unit,
          uint16_t accent, const EsFont *f, uint16_t valCol = 0, bool deg = false) {
  cv->fillRoundRect(x, y, w, h, 10, TH.card);
  cv->drawRoundRect(x, y, w, h, 10, TH.border);
  cv->fillRoundRect(x + 2, y + 10, 4, h - 20, 2, accent);
  tFont(&F_TEXT_S);
  tColor(TH.mfg);
  tAt(x + 14, y + 7 + F_TEXT_S.capH, label);
  int reserve = (unit && *unit) ? 28 : 0;
  if (unit && *unit) { tFont(&F_TEXT); tR(unit, x + w - 8, y + h - 9); }
  String v = deg ? val + DEG : val;
  tFont(f);
  while ((tWidth(v) > w - 22 - reserve || esFont->capH > h - 30) && smaller(esFont)) tFont(smaller(esFont));
  int top = y + 22, bot = y + h - 7;
  tColor(valCol ? valCol : TH.fg);
  tC(v, x + 10 + (w - 10 - reserve) / 2, (top + bot) / 2 + esFont->capH / 2);
}

// Settore di corona circolare (fill con triangoli)
void bandArc(float cx, float cy, float r0, float r1, float a0, float a1, uint16_t col) {
  int x0, y0, x1, y1, x2, y2, x3, y3;
  for (float a = a0; a < a1; a += 2) {
    pol(cx, cy, r0, a, x0, y0); pol(cx, cy, r1, a, x1, y1);
    pol(cx, cy, r0, a + 2, x2, y2); pol(cx, cy, r1, a + 2, x3, y3);
    cv->fillTriangle(x0, y0, x1, y1, x2, y2, col);
    cv->fillTriangle(x1, y1, x2, y2, x3, y3, col);
  }
}

void pointerTri(int cx, int cy, float ang, float rBase, float rTip, float half, uint16_t col) {
  int x0, y0, x1, y1, x2, y2;
  pol(cx, cy, rTip, ang, x0, y0);
  pol(cx, cy, rBase, ang - half, x1, y1);
  pol(cx, cy, rBase, ang + half, x2, y2);
  cv->fillTriangle(x0, y0, x1, y1, x2, y2, col);
}

void dashRadial(int cx, int cy, float ang, float r0, float r1, uint16_t col) {
  int xa, ya, xb, yb;
  for (float r = r0; r > r1; r -= 14) {
    pol(cx, cy, r, ang, xa, ya);
    pol(cx, cy, max(r - 8, r1), ang, xb, yb);
    cv->drawLine(xa, ya, xb, yb, col);
    cv->drawLine(xa + 1, ya, xb + 1, yb, col);
  }
}

String fv(const Val &v, int dec = 1) { return v.ok() ? String(v.v, dec) : String("--"); }

const char *cardinal(float h) {
  static const char *n[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  return n[((int)lroundf(h / 45.0f)) % 8];
}

bool statusLive() { return srvUp && (aws.ok() || depth.ok() || sog.ok()); }

// Tasti virtuali in basso (menu e pagine di servizio)
const char *KEY_LABELS[4] = {"UP", "DOWN", "OK", "BACK"};
void drawKeybar() {
  const int w = SCR_W / 4;
  cv->fillRect(0, KEYBAR_Y - 2, SCR_W, SCR_H - KEYBAR_Y + 2, TH.card);
  cv->drawFastHLine(0, KEYBAR_Y - 2, SCR_W, TH.border);
  tFont(&F_HEAD);
  for (int i = 0; i < 4; i++) {
    bool pressed = tTouching && tKey == i;
    int x = i * w + 5, y = KEYBAR_Y + 7, bw = w - 10, bh = SCR_H - KEYBAR_Y - 17;
    cv->fillRoundRect(x, y, bw, bh, 12, pressed ? TH.pri : TH.bg);
    cv->drawRoundRect(x, y, bw, bh, 12, TH.pri);
    tColor(pressed ? TH.card : TH.pri);
    tCC(KEY_LABELS[i], x + bw / 2, y + bh / 2);
  }
}

// Istruzioni sopra i tasti: una o due righe
void hint(const char *l1, const char *l2 = nullptr) {
  tFont(&F_TEXT_S);
  tColor(TH.mfg);
  if (l2) { tAt(10, KEYBAR_Y - 26, l1); tAt(10, KEYBAR_Y - 8, l2); }
  else tAt(10, KEYBAR_Y - 8, l1);
}

// Barra di stato in alto: [pallino STATO]  [legenda]  [pagina n/N]  [DAY/NIGHT]
void drawStatusBar() {
  cv->fillRect(0, 0, SCR_W, BAR_H, TH.card);
  cv->drawFastHLine(0, BAR_H - 1, SCR_W, TH.border);
  bool live = statusLive();
  const char *st = live ? "LIVE" : (WiFi.status() != WL_CONNECTED ? "NO WIFI" : (srvUp ? "NO DATA" : "NO SERVER"));
  uint16_t sc = live ? TH.ok : TH.bad;
  const int cy = BAR_H / 2;
  tFont(&F_TEXT);
  const int base = cy + F_TEXT.capH / 2;
  if (live) cv->fillCircle(16, cy, 6, sc); else cv->drawCircle(16, cy, 6, sc);
  tColor(sc);
  tAt(28, base, st);
  int cur = 28 + tWidth(st) + 14;
  if (page == 0) {
    tFont(&F_TEXT_S);
    const int b2 = cy + F_TEXT_S.capH / 2;
    cv->fillTriangle(cur, cy + 6, cur + 11, cy + 6, cur + 5, cy - 6, TH.pri);
    tColor(TH.fg);
    tAt(cur + 15, b2, "App.");
    cur += 15 + tWidth("App.") + 10;
    cv->drawCircle(cur + 5, cy, 5, TH.acc);
    cv->drawCircle(cur + 5, cy, 4, TH.acc);
    tAt(cur + 15, b2, "True");
  }
  tFont(&F_TEXT);
  tColor(TH.fg);
  tR(String(page + 1) + "/" + String(NUM_PAGES), NIGHT_X - 8, base);

  // pulsante giorno/notte: mostra il tema a cui si passa
  cv->fillRoundRect(NIGHT_X, NIGHT_Y, NIGHT_W, NIGHT_H, 8, TH.pri);
  tFont(&F_TEXT_S);
  tColor(TH.card);
  tCC(cfg.night ? "DAY" : "NIGHT", NIGHT_X + NIGHT_W / 2, NIGHT_Y + NIGHT_H / 2);
}

// ============================== Vele ======================================
// Regole dal Manuale di vela del Centro Velico Caprera (tav. 9, 29), le stesse di SailTrim.kt:
// - le andature sono riferite al vento APPARENTE (AWA);
// - dalla bolina al lasco le vele tengono circa lo stesso angolo col vento;
// - sotto LUFF_ANGLE (angolo morto) le vele sono in bando e sbattono;
// - oltre WING_ANGLE il genoa va a farfalla sull'altro lato, col tangone;
// - vento leggero = vele grasse, vento forte = vele magre;
// - carrello del genoa: avanti con poco vento, indietro con vento forte, avanti quando si lasca.
#define LUFF_ANGLE 24.0f
#define WING_ANGLE 150.0f

static float mainsailAngle(float awaAbs) { return constrain(8 + (awaAbs - 28) * (77.0f / 122.0f), 8.0f, 85.0f); }
static float genoaAngle(float awaAbs) { return constrain(10 + (awaAbs - 28) * (60.0f / 122.0f), 10.0f, 70.0f); }

// posizione suggerita del carrello del genoa: 1 = tutto avanti ... 5 = tutto indietro, 0 = non si applica
static int genoaCarStep(float awaAbs, float tws) {
  if (awaAbs < 22 || awaAbs > WING_ANGLE) return 0;
  int s = tws < 8 ? 2 : tws < 14 ? 3 : tws < 20 ? 4 : 5;
  if (awaAbs > 60) s--;
  if (awaAbs > 100) s--;
  return constrain(s, 1, 5);
}

// vela vista dall'alto: corda P0-P1, pancia di profondita' depth*corda verso (wx, wy) = dove soffia il vento
static void drawSailShape(float x0, float y0, float x1, float y1, float depth, float wx, float wy) {
  float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
  if (len < 1) return;
  float nx = -dy / len, ny = dx / len;
  if (nx * wx + ny * wy < 0) { nx = -nx; ny = -ny; }
  float qx = (x0 + x1) / 2 + nx * 2 * depth * len, qy = (y0 + y1) / 2 + ny * 2 * depth * len;
  int px = lroundf(x0), py = lroundf(y0);
  for (int i = 1; i <= 24; i++) {
    float t = i / 24.0f, u = 1 - t;
    int x = lroundf(u * u * x0 + 2 * u * t * qx + t * t * x1);
    int y = lroundf(u * u * y0 + 2 * u * t * qy + t * t * y1);
    cv->fillTriangle(lroundf(x0), lroundf(y0), px, py, x, y, TH.sail);  // vela piena
    cv->drawLine(px, py, x, y, TH.fg);
    px = x; py = y;
  }
}

// vela in bando: linea ondulata lungo l'asse
static void drawLuffing(int x, int y0, int y1, int amp) {
  int px = x, py = y0;
  for (int y = y0 + 3; y <= y1; y += 3) {
    int nx = x + lroundf(amp * sinf((y - y0) * 0.35f));
    cv->drawLine(px, py, nx, y, TH.fg);
    cv->drawLine(px + 1, py, nx + 1, y, TH.fg);
    px = nx; py = y;
  }
}

// Disegno delle vele; tutte le misure sono scalate da k (1 = disegno originale da 800x480)
void drawSails(int cx, int cy, float k, bool haveAwa, float aw, bool haveTws, float tws) {
  const float mastY = cy - 25 * k, tackY = cy - 96 * k;
  const float boomLen = 85 * k, genoaLen = 105 * k;
  float a = fabsf(aw);
  int sgn = aw >= 0 ? 1 : -1;  // +1 vento da dritta
  int lee = -sgn;              // lato sottovento sullo schermo (+x = dritta)

  if (!haveAwa || a < LUFF_ANGLE) {
    if (haveAwa) {
      drawLuffing(cx, mastY + 4, mastY + boomLen, 4);
      drawLuffing(cx, tackY + 4, mastY - 6, 3);
    } else {
      cv->drawLine(cx, mastY, cx, mastY + boomLen, TH.fg);
      cv->drawLine(cx, tackY, cx, mastY - 6, TH.fg);
    }
    cv->fillCircle(cx, mastY, 5 * k, TH.fg);
    return;
  }

  // direzione verso cui soffia il vento apparente (coord. schermo, y verso il basso)
  float wr = aw * DEG_TO_RAD, wx = -sinf(wr), wy = cosf(wr);
  // pancia: grassa con vento leggero, magra con vento forte
  float depth = !haveTws ? 0.11f : tws < 8 ? 0.15f : tws < 16 ? 0.11f : 0.07f;

  // genoa (disegnato prima: la randa gli sta sopra)
  bool wing = a > WING_ANGLE;
  float g = wing ? 80 : genoaAngle(a);
  int gside = wing ? sgn : lee;  // a farfalla va sopravvento
  float clx = cx + gside * sinf(g * DEG_TO_RAD) * genoaLen, cly = tackY + cosf(g * DEG_TO_RAD) * genoaLen;
  drawSailShape(cx, tackY, clx, cly, depth, wx, wy);
  if (wing) {  // tangone dall'albero alla bugna
    cv->drawLine(cx, mastY, lroundf(clx), lroundf(cly), TH.fg);
    cv->drawLine(cx, mastY + 1, lroundf(clx), lroundf(cly) + 1, TH.fg);
  }

  // carrello del genoa sulla rotaia sottovento
  int step = haveTws ? genoaCarStep(a, tws) : 0;
  if (step) {
    int tx = cx + lee * 23 * k, ty0 = cy + 5 * k;
    cv->drawLine(tx, ty0, tx, ty0 + 40 * k, TH.fg);
    for (int i = 0; i < 5; i++) cv->drawLine(tx - 3, lroundf(ty0 + i * 10 * k), tx + 3, lroundf(ty0 + i * 10 * k), TH.fg);
    int carY = lroundf(ty0 + (step - 1) * 10 * k);
    cv->fillRect(tx - 4, carY - 4, 9, 9, TH.fg);
    cv->drawLine(lroundf(clx), lroundf(cly), tx, carY, TH.fg);  // scotta
  }

  // randa
  float m = mainsailAngle(a) * DEG_TO_RAD;
  float bx = cx + lee * sinf(m) * boomLen, by = mastY + cosf(m) * boomLen;
  drawSailShape(cx, mastY, bx, by, depth, wx, wy);
  cv->drawLine(cx, mastY, lroundf(bx), lroundf(by), TH.fg);  // boma
  cv->drawLine(cx + 1, mastY, lroundf(bx) + 1, lroundf(by), TH.fg);
  cv->fillCircle(cx, mastY, 6 * k, TH.fg);
  cv->fillCircle(cx, mastY, 3 * k, TH.card);
}

// ============================== Pagina 1: vento ===========================
void pageWind() {
  const int cx = SCR_W / 2, cy = 200;
  const float R = 148, k = R / 228.0f;  // il quadrante e' disegnato in proporzione al vecchio 800x480
  float hd = hdg.ok() ? hdg.v : 0;

  // disco e anello: dritta (scuro) / sinistra (chiaro) sul lato da cui arriva il vento apparente
  cv->fillCircle(cx, cy, R, TH.card);
  cv->drawCircle(cx, cy, R, TH.border);
  cv->drawCircle(cx, cy, R - 1, TH.border);
  if (awa.ok()) {
    if (awa.v >= 0) bandArc(cx, cy, R - 14 * k, R - 2, 5, 175, TH.ok);
    else            bandArc(cx, cy, R - 14 * k, R - 2, -175, -5, TH.bad);
  }
  cv->drawCircle(cx, cy, R - 14 * k, TH.border);

  // tacche e rosa dei venti (ruota con la prua)
  int x0, y0, x1, y1;
  for (int c = 0; c < 360; c += 10) {
    float a = c - hd;
    bool lg = (c % 30 == 0);
    pol(cx, cy, R - 20 * k, a, x0, y0);
    pol(cx, cy, R - 20 * k - (lg ? 18 * k : 9 * k), a, x1, y1);
    cv->drawLine(x0, y0, x1, y1, TH.fg);
    if (lg) cv->drawLine(x0 + 1, y0, x1 + 1, y1, TH.fg);
  }
  for (int c = 0; c < 360; c += 30) {
    float a = c - hd;
    int lx, ly;
    pol(cx, cy, R - 54 * k, a, lx, ly);
    String s;
    bool card = (c % 90 == 0);
    if (c == 0) s = "N"; else if (c == 90) s = "E"; else if (c == 180) s = "S"; else if (c == 270) s = "W";
    else { char b[5]; snprintf(b, sizeof b, "%03d", c); s = b; }
    tFont(card ? &F_HEAD : &F_TEXT);
    tColor(card ? TH.pri : TH.fg);
    tCC(s, lx, ly);
  }

  // barca (fissa, prua in alto)
  static const int8_t hull[][2] = {{0, -100}, {22, -60}, {30, 0}, {26, 60}, {18, 92}, {-18, 92}, {-26, 60}, {-30, 0}, {-22, -60}};
  const int nh = sizeof(hull) / sizeof(hull[0]);
  for (int i = 0; i < nh; i++) {
    int j = (i + 1) % nh;
    int ax = lroundf(cx + hull[i][0] * k), ay = lroundf(cy + hull[i][1] * k);
    int bx = lroundf(cx + hull[j][0] * k), by = lroundf(cy + hull[j][1] * k);
    cv->drawLine(ax, ay, bx, by, TH.hull);
    cv->drawLine(ax + 1, ay, bx + 1, by, TH.hull);
  }

  // vele (stesse regole di SailTrim.kt nell'app)
  float ta0 = 0, ts0 = 0;
  bool haveTrue = getTrue(ta0, ts0);
  drawSails(cx, cy, k, awa.ok(), awa.ok() ? awa.v : 0, haveTrue, ts0);

  // vento apparente: puntatore + linea tratteggiata
  if (awa.ok()) {
    dashRadial(cx, cy, awa.v, R - 52 * k, 112 * k, TH.pri);
    pointerTri(cx, cy, awa.v, R - 16 * k, R - 52 * k, 3.6f, TH.pri);
  }
  // vento reale: anello colorato
  float ta, ts;
  if (getTrue(ta, ts)) {
    int mx, my;
    pol(cx, cy, R - 34 * k, ta, mx, my);
    cv->fillCircle(mx, my, 10, TH.card);
    cv->drawCircle(mx, my, 10, TH.acc);
    cv->drawCircle(mx, my, 9, TH.acc);
    cv->drawCircle(mx, my, 8, TH.acc);
  }

  // prua (heading) al centro in alto
  tFont(&F_TEXT_S);
  tColor(TH.mfg);
  tC("HEADING", cx, cy - 62 - F_HEAD_M.capH - 8);   // appena sopra il valore della prua
  tFont(&F_HEAD_M);
  tColor(TH.fg);
  if (hdg.ok()) {
    char b[5]; snprintf(b, sizeof b, "%03d", ((int)lroundf(hd)) % 360);
    tC(String(b) + DEG + " " + cardinal(hd), cx, cy - 62);
  } else tC("--", cx, cy - 62);

  // schede sotto il quadrante
  tile(10, 356, 192, 68, "AWS  apparent", fv(aws), "kn", TH.pri, &F_NUM_L);
  String tsStr = getTrue(ta, ts) ? String(ts, 1) : String("--");
  tile(208, 356, 192, 68, "TWS  true", tsStr, "kn", TH.acc, &F_NUM_L);
  String awaS = awa.ok() ? String((int)lroundf(fabsf(awa.v))) : String("--");
  String twaStr = getTrue(ta, ts) ? String((int)lroundf(fabsf(ta))) : String("--");
  tile(10, 430, 126, 64, "SOG", fv(sog), "kn", TH.mfg, &F_NUM_M);
  tile(142, 430, 126, 64, "AWA", awaS, "", awa.ok() && awa.v >= 0 ? TH.ok : TH.bad, &F_NUM_M, 0, awa.ok());
  tile(274, 430, 126, 64, "TWA", twaStr, "", getTrue(ta, ts) && ta >= 0 ? TH.ok : TH.bad, &F_NUM_M, 0,
       getTrue(ta, ts));
  drawStatusBar();
}

// ============================== Pagina 2: profondita' =====================
bool depthAlarming() { return depth.ok() && depth.v < cfg.depthAlarm; }

void pageDepth() {
  bool al = depthAlarming();
  tile(10, 48, 390, 150, "DEPTH  below transducer", fv(depth), "m", al ? TH.bad : TH.pri, &F_NUM_XL,
       al ? TH.bad : TH.fg);
  tile(10, 206, 192, 84, "BELOW KEEL", fv(keel), "m", TH.acc, &F_NUM_L);
  String lab = "ALARM  < " + String(cfg.depthAlarm, 1) + " m";
  tile(208, 206, 192, 84, lab.c_str(), al ? String("ALARM") : String("OK"), "", al ? TH.bad : TH.ok, &F_HEAD_L,
       al ? TH.bad : TH.fg);

  // grafico storico
  cv->fillRoundRect(10, 298, 390, 196, 10, TH.card);
  cv->drawRoundRect(10, 298, 390, 196, 10, TH.border);
  float mn = NAN, mx = NAN;
  for (int i = 0; i < HN; i++) if (!isnan(hist[i])) {
    if (isnan(mn) || hist[i] < mn) mn = hist[i];
    if (isnan(mx) || hist[i] > mx) mx = hist[i];
  }
  tFont(&F_TEXT_S);
  tColor(TH.mfg);
  String hdr = "LAST 30 MIN";
  if (!isnan(mn)) hdr += "    MIN " + String(mn, 1) + "   MAX " + String(mx, 1) + " m";
  tAt(24, 322, hdr);

  const int px0 = 50, px1 = 384, py0 = 344, py1 = 450;
  float maxScale = 5;
  if (!isnan(mx)) maxScale = max(5.0f, ceilf(mx * 1.25f));
  if (cfg.depthAlarm > maxScale) maxScale = ceilf(cfg.depthAlarm * 1.2f);
  maxScale = ceilf(maxScale / 2) * 2;  // pari: la tacca di mezzo e' un numero intero
  // griglia (0 in alto = superficie)
  for (int g = 0; g <= 2; g++) {
    int gy = py0 + g * (py1 - py0) / 2;
    for (int gx = px0; gx < px1; gx += 10) cv->drawFastHLine(gx, gy, 5, TH.border);
    tR(String(maxScale * g / 2, 0), px0 - 8, gy + 5);
  }
  // soglia di allarme
  if (cfg.depthAlarm < maxScale) {
    int ay = py0 + (int)lroundf(cfg.depthAlarm / maxScale * (py1 - py0));
    cv->drawFastHLine(px0, ay, px1 - px0, TH.bad);
    cv->drawFastHLine(px0, ay + 1, px1 - px0, TH.bad);
    cv->fillTriangle(px0 - 2, ay, px0 - 11, ay - 5, px0 - 11, ay + 5, TH.bad);
  }
  // curva
  int lx = -1, ly = -1;
  for (int i = 0; i < HN; i++) {
    if (isnan(hist[i])) { lx = -1; continue; }
    int gx = px0 + i * (px1 - px0) / (HN - 1);
    int gy = py0 + (int)lroundf(min(hist[i], maxScale) / maxScale * (py1 - py0));
    if (lx >= 0) {
      cv->drawLine(lx, ly, gx, gy, TH.pri);
      cv->drawLine(lx, ly + 1, gx, gy + 1, TH.pri);
    }
    lx = gx; ly = gy;
  }
  tColor(TH.mfg);
  tAt(px0, 480, "-30 min");
  tC("-15", (px0 + px1) / 2, 480);
  tR("now", px1, 480);
  drawStatusBar();
}

// ============================== Menu / editor =============================
void drawHeader(const char *t) {
  cv->fillRect(0, 0, SCR_W, HEADER_H, TH.pri);
  tFont(&F_HEAD);
  tColor(TH.card);
  tAt(14, HEADER_H / 2 + F_HEAD.capH / 2, t);
}

const char *MENU_LABELS[] = {"WiFi network", "WiFi password", "NMEA server IP", "Port", "Depth alarm",
                             "Diagnostics", "Save and restart", "Restart", "Factory reset",
                             "Power off", "Exit without saving"};
const int MENU_N = 11;

String menuVal(int i) {
  switch (i) {
    case 0: return cfg.ssid.length() ? (cfg.ssid.length() > 16 ? cfg.ssid.substring(0, 15) + "." : cfg.ssid) : String("(none)");
    case 1: { String s; for (size_t k = 0; k < cfg.pass.length() && k < 10; k++) s += '*'; return s.length() ? s : String("(empty)"); }
    case 2: return cfg.host.length() ? cfg.host : String("(empty)");
    case 3: return String(cfg.port);
    case 4: return String(cfg.depthAlarm, 1) + " m";
    default: return "";
  }
}

void drawMenu() {
  drawHeader("SETTINGS");
  for (int i = 0; i < MENU_N; i++) {
    int y = HEADER_H + 6 + i * 32;
    bool sel = (i == menuSel);
    cv->fillRoundRect(10, y, SCR_W - 20, 28, 8, sel ? TH.pri : TH.card);
    if (!sel) cv->drawRoundRect(10, y, SCR_W - 20, 28, 8, TH.border);
    tFont(&F_TEXT);
    tColor(sel ? TH.card : TH.fg);
    tAt(22, y + 14 + F_TEXT.capH / 2, MENU_LABELS[i]);
    tR(menuVal(i), SCR_W - 22, y + 14 + F_TEXT.capH / 2);
  }
  hint("Tap the keys below.  After editing, 'Save and restart'.");
}

void drawEdit() {
  drawHeader(EDIT_TITLES[editT]);
  bool big = editBuf.length() <= 13;
  tFont(big ? &F_HEAD_L : &F_HEAD);
  String shown = editBuf;
  size_t maxShow = big ? 13 : 22;
  if (shown.length() > maxShow) shown = String("...") + shown.substring(shown.length() - maxShow);
  int x = 14, y = 170;
  tColor(TH.fg);
  tAt(x, y, shown);
  int bx = x + (shown.length() ? tWidth(shown) + 6 : 0);
  char c = editSet[curIdx];
  String cs = (c == ' ') ? String("_") : String(c);
  int bw = max(tWidth(cs) + 16, 32), bh = big ? 42 : 32;
  cv->fillRoundRect(bx, y - bh + 9, bw, bh, 6, TH.pri);
  tColor(TH.card);
  tAt(bx + 8, y, cs);
  tFont(&F_TEXT);
  tColor(TH.mfg);
  tAt(14, 230, String("Characters: ") + editBuf.length() + " / " + editMax);
  hint("UP/DOWN: character (hold = fast)   OK: add   BACK: delete",
       "OK hold = confirm      BACK hold = cancel");
}

void drawScan() {
  drawHeader("SELECT WIFI NETWORK");
  if (!scanN) {
    tFont(&F_TEXT);
    tColor(TH.fg);
    tC("No networks found", SCR_W / 2, 200);
  }
  const int rows = 7;
  int first = max(0, min(scanSel - rows / 2, scanN - rows));
  for (int r = 0; r < rows && first + r < scanN; r++) {
    int i = first + r, y = HEADER_H + 6 + r * 48;
    bool sel = (i == scanSel);
    cv->fillRoundRect(10, y, SCR_W - 20, 40, 8, sel ? TH.pri : TH.card);
    if (!sel) cv->drawRoundRect(10, y, SCR_W - 20, 40, 8, TH.border);
    tFont(&F_TEXT);
    tColor(sel ? TH.card : TH.fg);
    String nm = scanSsid[i];
    if (nm.length() > 22) nm = nm.substring(0, 21) + ".";
    tAt(22, y + 20 + F_TEXT.capH / 2, nm);
    tR(String(scanRssi[i]), SCR_W - 22, y + 20 + F_TEXT.capH / 2);
  }
  hint("Choose the boat WiFi, then OK to enter its password.");
}

const char *wifiStatusStr() {
  if (!cfg.ssid.length()) return "setup mode (no boat WiFi set)";
  switch (WiFi.status()) {
    case WL_CONNECTED: return "connected";
    case WL_NO_SSID_AVAIL: return "SSID not found (5 GHz network?)";
    case WL_CONNECT_FAILED: return "connect failed (wrong password?)";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected / connecting";
    case WL_IDLE_STATUS: return "idle";
    default: return "unknown";
  }
}

extern bool apOn;
extern String apName;
void drawDiag() {
  drawHeader("DIAGNOSTICS");
  tFont(&F_TEXT);
  tColor(TH.fg);
  int y = 68;
  const int dy = 22;
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  tAt(12, y, String("WiFi: ") + wifiStatusStr()); y += dy;
  tAt(12, y, String("Network: ") + cfg.ssid); y += dy;
  String ip = wifiOk ? WiFi.localIP().toString() : String("-");
  String rssi = wifiOk ? (String(WiFi.RSSI()) + " dBm") : String("-");
  tAt(12, y, String("IP: ") + ip); y += dy;
  tAt(12, y, String("RSSI: ") + rssi); y += dy;
  String hostS = cfg.host.length() ? cfg.host : String("(not set)");
  tAt(12, y, String("Server: ") + hostS + ":" + String((int)cfg.port)); y += dy;
  tAt(12, y, srvUp ? "   CONNECTED" : "   not connected"); y += dy;
  tAt(12, y, String("Connect tries: ") + String((unsigned long)nConnTry)); y += dy;
  tAt(12, y, String("Sentences: ") + String((unsigned long)nRx) + "   ok " + String((unsigned long)nOk)); y += dy;
  tAt(12, y, String("Bad checksum: ") + String((unsigned long)nBadCs) + "   other " + String((unsigned long)nUnknown));
  y += dy;
  tFont(&F_TEXT_S);
  String last = lastLine.length() ? lastLine : String("(nothing received)");
  if (last.length() > 48) last = last.substring(0, 48);
  tAt(12, y, String("Last: ") + last);
  y += dy;
  tFont(&F_TEXT);
  if (wifiOk) {
    tAt(12, y, String("Web: http://") + ip); y += dy;
    tAt(12, y, String("  or http://") + cfg.name + ".local"); y += dy;
  } else { tAt(12, y, "Web: -"); y += dy; }
  if (apOn) {
    tFont(&F_TEXT_S);
    tAt(12, y, String("Setup WiFi: ") + apName + "  pass " + CONFIG_AP_PASS); y += 18;
    tAt(12, y, String("http://") + WiFi.softAPIP().toString());
  }
  hint("No WiFi: 2.4 GHz networks only.",
       "No sentences: wrong port, UDP-only or busy server.");
}

// Conferma del ripristino di fabbrica (hard reset)
void drawFactory() {
  drawHeader("FACTORY RESET");
  tFont(&F_TEXT);
  tColor(TH.fg);
  int y = 96;
  tAt(14, y, "All settings will be erased:"); y += 32;
  tAt(26, y, "boat WiFi, NMEA server, alarm,"); y += 24;
  tAt(26, y, "name, orientation, night mode,"); y += 24;
  tAt(26, y, "admin password (back to admin)."); y += 40;
  tAt(14, y, "The device restarts in setup mode:"); y += 24;
  tAt(14, y, "open WiFi '" SETUP_AP_NAME "'.");
  hint("OK hold = erase everything and restart.  BACK = cancel.");
}

void factoryReset() {
  Serial.println("Factory reset: erasing settings");
  prefs.begin("essview", false);
  prefs.clear();
  prefs.end();
  Serial.println("Restarting in setup mode");
  delay(300);
  ESP.restart();
}

// Primo avvio: istruzioni per la configurazione dal telefono / PC
void drawSetup() {
  drawHeader("WELCOME - FIRST SETUP");
  tFont(&F_TEXT);
  tColor(TH.fg);
  int y = 82;
  tAt(14, y, "1. On your phone or PC join the"); y += 22;
  tAt(14, y, "    open WiFi network:"); y += 40;
  tFont(&F_HEAD);
  tColor(TH.pri);
  tC(SETUP_AP_NAME, SCR_W / 2, y); y += 38;
  tFont(&F_TEXT);
  tColor(TH.fg);
  tAt(14, y, "2. The setup page opens by itself,"); y += 22;
  tAt(14, y, "    or open  http://192.168.4.1"); y += 32;
  tAt(14, y, "3. Login: admin    Password: admin"); y += 32;
  tAt(14, y, "4. Choose the boat WiFi and the"); y += 22;
  tAt(14, y, "    NMEA server, then Save."); y += 22;
  hint("OK opens the settings here.  BACK = dashboard.");
}

// Messaggio a schermo intero (fuori dal ciclo di disegno normale)
void showMsg(const char *l1, const char *l2 = "") {
  cv->fillScreen(TH.bg);
  tFont(&F_HEAD_L);
  tColor(TH.fg);
  tC(l1, SCR_W / 2, 240);
  tFont(&F_TEXT);
  tColor(TH.mfg);
  tC(l2, SCR_W / 2, 282);
  cv->flush();
}

// ============================== Render ====================================
void render() {
  cv->fillScreen(TH.bg);
  switch (mode) {
    case M_DASH: if (page == 0) pageWind(); else pageDepth(); break;
    case M_MENU: drawMenu(); drawKeybar(); break;
    case M_EDIT: drawEdit(); drawKeybar(); break;
    case M_SCAN: drawScan(); drawKeybar(); break;
    case M_DIAG: drawDiag(); drawKeybar(); break;
    case M_SETUP: drawSetup(); drawKeybar(); break;
    case M_FACTORY: drawFactory(); drawKeybar(); break;
  }
  cv->flush();
  lastRender = millis();
  lastSig = sig();
  dirty = false;
}

uint32_t sig() {
  uint32_t h = 2166136261u;
  auto add = [&](const Val &v, float mul) {
    int32_t i = v.ok() ? (int32_t)lroundf(v.v * mul) : -99999;
    h = (h ^ (uint32_t)i) * 16777619u;
  };
  add(aws, 10); add(awa, 1); add(twaS, 1); add(twsS, 10); add(sog, 10); add(stw, 10); add(hdg, 1); add(depth, 10); add(keel, 10);
  h = (h ^ (uint32_t)(srvUp ? 1 : 0) ^ (uint32_t)(page << 4) ^ (uint32_t)(mode << 6) ^ (uint32_t)(cfg.night << 9)) * 16777619u;
  return h;
}

// Copia dello schermo via USB (comando 'S'): meta' risoluzione, RGB565 in base64, "SCR565_BEGIN w h" ... "SCR565_END".
// Serve per controllare l'aspetto dal PC senza guardare il display.
void sendScreenPreview() {
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint16_t *fb = cv->getFramebuffer();
  char line[80];
  int ln = 0, on = 0;
  uint8_t in[3];
  auto put = [&](uint8_t b) {
    in[on++] = b;
    if (on < 3) return;
    line[ln++] = b64[in[0] >> 2];
    line[ln++] = b64[((in[0] & 3) << 4) | (in[1] >> 4)];
    line[ln++] = b64[((in[1] & 15) << 2) | (in[2] >> 6)];
    line[ln++] = b64[in[2] & 63];
    on = 0;
    if (ln >= 76) { line[ln] = 0; Serial.println(line); ln = 0; }
  };
  Serial.printf("SCR565_BEGIN %d %d\n", SCR_W / 2, SCR_H / 2);
  for (int y = 0; y < SCR_H; y += 2)
    for (int x = 0; x < SCR_W; x += 2) {
      uint16_t p = fb[(uint32_t)y * SCR_W + x];
      put(p >> 8);
      put(p & 255);
    }
  if (on) {  // padding base64
    uint8_t a = in[0], b = on > 1 ? in[1] : 0;
    line[ln++] = b64[a >> 2];
    line[ln++] = b64[((a & 3) << 4) | (b >> 4)];
    line[ln++] = on > 1 ? b64[(b & 15) << 2] : '=';
    line[ln++] = '=';
  }
  if (ln) { line[ln] = 0; Serial.println(line); }
  Serial.println("SCR565_END");
}

// ============================== Banco di prova via USB ====================
// Comandi (un carattere): U/D = UP/DOWN, V/W = UP/DOWN ripetuto, O/L = OK breve/lungo,
// B/K = BACK breve/lungo, P = spegni; "H<ip>:<porta>\n" = server NMEA temporaneo;
// "C..." = configurazione (vedi webcfg.h).
// Il banco di prova del PC non riceve piu' la copia dello schermo (il display ora e' a colori).
void serialCmdLoop() {
  static String cmd;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (cmd.length()) {
      if (c == '\n' || c == '\r') {
        if (cmd[0] == 'C') { configCmd(cmd.substring(1)); cmd = ""; continue; }
        if (cmd[0] == 'N') { handleNmea(cmd.substring(1)); cmd = ""; continue; }   // frase NMEA via USB (prova)
        int colon = cmd.lastIndexOf(':');
        if (colon > 1) {
          if (!hostTemp) { savedHost = cfg.host; savedPort = cfg.port; hostTemp = true; }
          cfg.host = cmd.substring(1, colon);
          cfg.port = (uint16_t)cmd.substring(colon + 1).toInt();
          nmea.stop(); srvUp = false; lastConnTry = 0;
          Serial.printf("HOST %s:%u (temporary)\n", cfg.host.c_str(), cfg.port);
        }
        cmd = "";
      } else if (cmd.length() < 700) cmd += c;
      continue;
    }
    switch (c) {
      case 'U': handleKeys(EV_SHORT, EV_NONE, EV_NONE, EV_NONE); break;
      case 'V': handleKeys(EV_REPEAT, EV_NONE, EV_NONE, EV_NONE); break;
      case 'D': handleKeys(EV_NONE, EV_SHORT, EV_NONE, EV_NONE); break;
      case 'W': handleKeys(EV_NONE, EV_REPEAT, EV_NONE, EV_NONE); break;
      case 'O': handleKeys(EV_NONE, EV_NONE, EV_SHORT, EV_NONE); break;
      case 'L': handleKeys(EV_NONE, EV_NONE, EV_LONG, EV_NONE); break;
      case 'B': handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_SHORT); break;
      case 'K': handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_LONG); break;
      case 'P': powerOff(); break;
      case 'S': sendScreenPreview(); break;
      case 'T': cfg.night = !cfg.night; applyTheme(); dirty = true; break;   // prova: tema giorno/notte senza salvare
      case 'H': cmd = "H"; break;
      case 'C': cmd = "C"; break;
      case 'N': cmd = "N"; break;
    }
  }
}

// ============================== Logica dei menu ===========================
void startEdit(EditT t, bool clear = false) {
  editT = t;
  switch (t) {
    case E_PASS: editSet = CS_FULL; editBuf = clear ? String("") : cfg.pass; editMax = 63; curIdx = 0; break;
    case E_HOST: editSet = CS_NUM; editBuf = cfg.host; editMax = 15; curIdx = 1; break;
    case E_PORT: editSet = CS_NUM; editBuf = String(cfg.port); editMax = 5; curIdx = 3; break;
    case E_ALARM: editSet = CS_NUM; editBuf = String(cfg.depthAlarm, 1); editMax = 5; curIdx = 2; break;
  }
  mode = M_EDIT;
  dirty = true;
}

void finishEdit() {
  switch (editT) {
    case E_PASS: cfg.pass = editBuf; break;
    case E_HOST: cfg.host = editBuf; break;
    case E_PORT: { long p = editBuf.toInt(); cfg.port = (p >= 1 && p <= 65535) ? p : 10110; break; }
    case E_ALARM: cfg.depthAlarm = editBuf.toFloat(); break;
  }
  mode = M_MENU;
  dirty = true;
}

void doScan() {
  showMsg("Scanning WiFi...");
  int n = WiFi.scanNetworks();
  scanN = 0;
  for (int i = 0; i < n && scanN < 12; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (int k = 0; k < scanN; k++) if (scanSsid[k] == s) dup = true;
    if (dup) continue;
    scanSsid[scanN] = s;
    scanRssi[scanN] = WiFi.RSSI(i);
    scanN++;
  }
  WiFi.scanDelete();
  scanSel = 0;
  mode = M_SCAN;
  dirty = true;
}

void handleKeys(Ev eu, Ev ed, Ev eo, Ev eb) {
  switch (mode) {
    case M_DASH:
      if (eu == EV_SHORT) { page = (page + NUM_PAGES - 1) % NUM_PAGES; dirty = true; }
      if (ed == EV_SHORT) { page = (page + 1) % NUM_PAGES; dirty = true; }
      if (eo == EV_LONG) { mode = M_MENU; menuSel = 0; dirty = true; }
      if (eb == EV_SHORT) { cfg.night = !cfg.night; applyTheme(); saveCfg(); dirty = true; }
      break;
    case M_MENU:
      if (eu) menuSel = (menuSel + MENU_N - 1) % MENU_N;
      if (ed) menuSel = (menuSel + 1) % MENU_N;
      if (eo == EV_SHORT) {
        switch (menuSel) {
          case 0: doScan(); break;
          case 1: startEdit(E_PASS); break;
          case 2: startEdit(E_HOST); break;
          case 3: startEdit(E_PORT); break;
          case 4: startEdit(E_ALARM); break;
          case 5: mode = M_DIAG; dirty = true; break;
          case 6: saveCfg(); showMsg("Saved", "Restarting..."); delay(800); ESP.restart(); break;
          case 7: showMsg("Restarting...", ""); delay(300); ESP.restart(); break;
          case 8: mode = M_FACTORY; dirty = true; break;
          case 9: powerOff(); break;
          case 10: loadCfg(); applyTheme(); mode = M_DASH; dirty = true; break;
        }
      }
      if (eb) { loadCfg(); applyTheme(); mode = M_DASH; dirty = true; }
      dirty = true;
      break;
    case M_EDIT: {
      int n = strlen(editSet);
      if (eu) curIdx = (curIdx + (eu == EV_REPEAT ? 2 : 1)) % n;
      if (ed) curIdx = ((curIdx - (ed == EV_REPEAT ? 2 : 1)) % n + n) % n;
      if (eo == EV_SHORT && editBuf.length() < editMax) editBuf += editSet[curIdx];
      if (eo == EV_LONG) finishEdit();
      if (eb == EV_SHORT && editBuf.length()) editBuf.remove(editBuf.length() - 1);
      if (eb == EV_LONG) mode = M_MENU;
      dirty = true;
      break;
    }
    case M_FACTORY:
      if (eo == EV_LONG) factoryReset();
      if (eb) { mode = M_MENU; dirty = true; }
      break;
    case M_SETUP:
      if (eo) { mode = M_MENU; menuSel = 0; dirty = true; }
      if (eb) { mode = M_DASH; dirty = true; }
      break;
    case M_DIAG:
      if (eo == EV_SHORT) dirty = true;
      if (eb) { mode = M_MENU; dirty = true; }
      break;
    case M_SCAN:
      if (eu && scanN) scanSel = (scanSel + scanN - 1) % scanN;
      if (ed && scanN) scanSel = (scanSel + 1) % scanN;
      if (eo == EV_SHORT && scanN) { cfg.ssid = scanSsid[scanSel]; startEdit(E_PASS, true); }
      if (eb) { mode = M_MENU; dirty = true; }
      dirty = true;
      break;
  }
}

// ============================== Tocco: gesti e tasti ======================
// Nella dashboard: swipe = pagina, pressione lunga = menu, tocco su DAY/NIGHT = tema.
// Nei menu: i 4 tasti virtuali in basso (tenendo premuto UP/DOWN ripetono).
void touchEvents(Ev &eu, Ev &ed, Ev &eo, Ev &eb) {
  eu = ed = eo = eb = EV_NONE;
  uint32_t now = millis();
  bool down = tTouching;
  if (down && !tWasDown) {  // inizio del tocco
    tDown = now; tX0 = tX; tY0 = tY; tMoved = false; tLongFired = false;
    tKey = (mode != M_DASH && tY >= KEYBAR_Y) ? min(3, tX / (SCR_W / 4)) : -1;
    dirty = true;
  }
  if (down) {
    if (abs(tX - tX0) > TAP_SLOP_PX || abs(tY - tY0) > TAP_SLOP_PX) tMoved = true;
    if (mode == M_DASH && !tMoved && !tLongFired && now - tDown >= LONG_PRESS_MS) {
      tLongFired = true;
      eo = EV_LONG;
    }
  } else if (tWasDown) {  // fine del tocco
    if (mode == M_DASH) {
      int dx = tX - tX0;
      if (tMoved && abs(dx) >= SWIPE_MIN_PX && now - tDown <= SWIPE_MAX_MS) {
        if (dx < 0) ed = EV_SHORT; else eu = EV_SHORT;   // swipe a sinistra = pagina successiva
      } else if (!tMoved && !tLongFired && tX0 >= NIGHT_X && tX0 < NIGHT_X + NIGHT_W &&
                 tY0 >= NIGHT_Y && tY0 < NIGHT_Y + NIGHT_H) {
        eb = EV_SHORT;
      }
    }
    tKey = -1;
    dirty = true;
  }
  tWasDown = down;
  if (mode != M_DASH) {
    Ev ku = bUp.poll(down && tKey == 0);
    Ev kd = bDown.poll(down && tKey == 1);
    Ev ko = bOk.poll(down && tKey == 2);
    Ev kb = bBack.poll(down && tKey == 3);
    if (ku) eu = ku;
    if (kd) ed = kd;
    if (ko) eo = ko;
    if (kb) eb = kb;
  }
}

// ============================== Spegnimento ===============================
// Schermo spento e deep sleep. Per riaccendere: tasto BOOT o RESET sulla scheda.
void powerOff() {
  Serial.println("Power off");
  nmea.stop();
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  cv->fillScreen(0);
  cv->flush();
  lcd->displayOff();
  delay(200);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 0);  // tasto BOOT (a massa) riaccende
  esp_deep_sleep_start();
}

#include "webcfg.h"

// ============================== Setup / loop ==============================
void haltWithError(const char *message) {
  Serial.println(message);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.printf("\nEssentialView S3 %s - reset reason %d (1 power-on, 3 software, 4 panic, 5-7 watchdog, 9 brownout)\n",
                FW_VERSION, (int)esp_reset_reason());
  loadCfg();
  applyTheme();
  for (int i = 0; i < HN; i++) hist[i] = NAN;

  if (!lcd->begin()) haltWithError("LCD init failed (QSPI CO5300)");
  lcd->setBrightness(255);
  PsramCanvas *pc = new PsramCanvas(SCR_W, SCR_H, lcd, cfg.rot == 2 ? 2 : 0);
  if (!pc->initBuffer()) haltWithError("Canvas: cannot allocate 400 KB in PSRAM (PSRAM = OPI? see README)");
  cv = pc;
  cv->begin(GFX_SKIP_OUTPUT_BEGIN);  // il pannello e' gia' inizializzato sopra: non rifarlo (fallirebbe)

  touchInit();
  if (!tpOk) Serial.println("FT3168 not found: touch disabled");

#if NMEA_WIRED
  Serial1.begin(NMEA_BAUD, SERIAL_8N1, NMEA_RX_PIN, NMEA_TX_PIN);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (cfg.ssid.length()) WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  webSetup();

  if (!cfg.ssid.length()) mode = M_SETUP;  // primo avvio: rete aperta + istruzioni
  else if (!cfg.host.length() && !NMEA_WIRED) { mode = M_MENU; menuSel = 0; }
  render();
  Serial.println("EssentialView S3 ready");
}

void loop() {
  netLoop();
#if NMEA_WIRED
  serialLoop();
#endif
  serialCmdLoop();
  netCfgLoop();

  readTouch();
  Ev eu, ed, eo, eb;
  touchEvents(eu, ed, eo, eb);
  if (eu || ed || eo || eb) handleKeys(eu, ed, eo, eb);

  uint32_t now = millis();
  if (now - lastHist >= 10000) {
    lastHist = now;
    memmove(hist, hist + 1, (HN - 1) * sizeof(float));
    hist[HN - 1] = depth.ok() ? depth.v : NAN;
  }

  static uint32_t lastLog = 0;
  if (now - lastLog >= 5000) {
    lastLog = now;
    Serial.printf("WiFi=%s srv=%d rx=%lu ok=%lu badcs=%lu other=%lu\n", wifiStatusStr(), (int)srvUp,
                  (unsigned long)nRx, (unsigned long)nOk, (unsigned long)nBadCs, (unsigned long)nUnknown);
    float ta, ts;
    bool tr = getTrue(ta, ts);
    Serial.printf("Wind: app %.1f/%.1fkn true %.1f/%.1fkn (%s) stw=%.1f sog=%.1f\n", awa.v, aws.v,
                  tr ? ta : NAN, tr ? ts : NAN,
                  (twaS.ok(10000) && twsS.ok(10000)) ? "server" : (stw.ok() ? "calc STW" : (sog.ok() ? "calc SOG" : "none")),
                  stw.ok() ? stw.v : NAN, sog.ok() ? sog.v : NAN);
  }

  // ridisegno: subito se c'e' un evento, altrimenti quando cambiano i dati (o ogni FULL_REDRAW_MS)
  if (now - lastRender >= 60 && (dirty || (now - lastRender >= REFRESH_MS && sig() != lastSig) ||
                                 now - lastRender >= FULL_REDRAW_MS)) {
    render();
  }
  delay(2);
}
