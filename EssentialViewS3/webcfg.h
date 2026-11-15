// EssentialView - configurazione: pagina web (porta 80), rete WiFi di configurazione, comandi USB "C...".
// Incluso da main.cpp dopo tutte le funzioni del dispositivo (usa cfg, display, handleKeys, ...).
// Le stesse funzioni JSON servono la pagina web e il PC tool (EssentialView Configuration and Test Tool).
#pragma once
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <DNSServer.h>
#include "webpage.h"

WebServer web(80);
DNSServer dns;  // rete di configurazione: ogni nome porta alla pagina (il telefono la apre da solo)
bool apOn = false;
uint32_t apRetryT = 0;
String apName;

// ---------- JSON comuni (web + USB) ----------
String cfgJson() {
  JsonDocument d;
  d["name"] = cfg.name;
  d["ssid"] = cfg.ssid;
  d["pass_set"] = cfg.pass.length() > 0;    // la password non esce mai dal dispositivo
  d["host"] = hostTemp ? savedHost : cfg.host;   // sempre il server salvato
  d["port"] = hostTemp ? savedPort : cfg.port;
  d["dalarm"] = serialized(String(cfg.depthAlarm, 1));
  d["night"] = cfg.night;
  d["rot"] = cfg.rot;
  d["tz"] = cfg.tz;
  d["bright"] = cfg.bright;
  d["webuser"] = WEB_USER;
  d["webpass_default"] = cfg.webpass == WEB_DEFAULT_PASS;
  d["fw"] = FW_VERSION;
  String s;
  serializeJson(d, s);
  return s;
}

// Backup delle impostazioni (file .json). Contiene la password WiFi, NON quella di amministratore.
String backupJson() {
  JsonDocument d;
  d["type"] = "essentialview-config";
  d["fw"] = FW_VERSION;
  d["name"] = cfg.name;
  d["ssid"] = cfg.ssid;
  d["pass"] = cfg.pass;
  d["host"] = hostTemp ? savedHost : cfg.host;
  d["port"] = hostTemp ? savedPort : cfg.port;
  d["dalarm"] = serialized(String(cfg.depthAlarm, 1));
  d["night"] = cfg.night;
  d["rot"] = cfg.rot;
  d["tz"] = cfg.tz;
  String s;
  serializeJson(d, s);
  return s;
}

String statusJson() {
  JsonDocument d;
  bool ok = WiFi.status() == WL_CONNECTED;
  d["fw"] = FW_VERSION;
  d["name"] = cfg.name;
  d["wifi"] = wifiStatusStr();
  d["ssid"] = cfg.ssid;
  d["ip"] = ok ? WiFi.localIP().toString() : String("");
  d["rssi"] = ok ? WiFi.RSSI() : 0;
  d["ap"] = apOn ? apName : String("");
  d["ap_ip"] = apOn ? WiFi.softAPIP().toString() : String("");
  d["host"] = cfg.host;
  d["port"] = cfg.port;
  d["server"] = srvUp;
  d["rx"] = nRx; d["ok"] = nOk; d["badcs"] = nBadCs; d["other"] = nUnknown;
  auto put = [&](const char *k, const Val &v, int dec) { if (v.ok()) d[k] = serialized(String(v.v, dec)); };
  put("awa", awa, 0); put("aws", aws, 1); put("sog", sog, 1); put("stw", stw, 1);
  put("hdg", hdg, 0); put("depth", depth, 1);
  float ta, ts;
  if (getTrue(ta, ts)) { d["twa"] = serialized(String(ta, 0)); d["tws"] = serialized(String(ts, 1)); }
  d["uptime"] = millis() / 1000;
  {  // orologio, batteria, tasti
    time_t tn = time(nullptr);
    struct tm tmv;
    localtime_r(&tn, &tmv);
    char tb[24];
    strftime(tb, sizeof tb, "%Y-%m-%d %H:%M:%S", &tmv);
    d["time"] = clockValid() ? String(tb) : String("");
    d["epoch"] = (long long)tn;
    d["tsrc"] = clockSource();
    d["pmu"] = pmuOk;
    if (batt.ok && batt.present) {
      if (batt.pct >= 0) d["batt"] = batt.pct;
      d["vbat"] = serialized(String(batt.mv / 1000.0f, 2));
    }
    d["chg"] = batt.charging; d["vbus"] = batt.vbus;
    d["pwr"] = digitalRead(PWR_PIN);
    d["boot"] = digitalRead(BOOT_PIN);
    d["alert"] = alertActive;
    d["snooze"] = snoozed() ? (int)((snoozeUntil - millis()) / 1000) : 0;
    d["screen"] = screenOn;
  }
  d["heap"] = ESP.getFreeHeap();
  String s;
  serializeJson(d, s);
  return s;
}

String scanJson() {
  int n = WiFi.scanNetworks();
  JsonDocument d;
  JsonArray a = d.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (JsonObject o : a) if (s == o["ssid"].as<const char *>()) dup = true;
    if (dup) continue;
    JsonObject o = a.add<JsonObject>();
    o["ssid"] = s;
    o["rssi"] = WiFi.RSSI(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String s;
  serializeJson(d, s);
  return s;
}

// Applica le chiavi presenti nel JSON (le assenti restano come sono), salva in memoria.
// "pass"/"webpass" presenti = nuove password (anche vuote). Ritorna "" se ok, altrimenti l'errore.
String applyCfg(const String &body, bool &needRestart) {
  JsonDocument d;
  if (deserializeJson(d, body)) return "invalid JSON";
  if (hostTemp) {  // un salvataggio chiude il reindirizzamento temporaneo del banco di prova
    cfg.host = savedHost; cfg.port = savedPort; hostTemp = false;
    nmea.stop(); srvUp = false; lastConnTry = 0;
  }
  Settings n = cfg;
  if (d["name"].is<const char *>()) {
    String v = d["name"].as<String>();
    v.toLowerCase();
    if (!v.length() || v.length() > 24) return "name: 1-24 characters";
    for (char c : v) if (!isalnum((unsigned char)c) && c != '-') return "name: only letters, digits and -";
    n.name = v;
  }
  if (d["ssid"].is<const char *>()) n.ssid = d["ssid"].as<String>();
  if (d["pass"].is<const char *>()) n.pass = d["pass"].as<String>();
  if (n.pass.length() && (n.pass.length() < 8 || n.pass.length() > 63)) return "WiFi password: 8-63 characters (empty = open network)";
  if (d["host"].is<const char *>()) n.host = d["host"].as<String>();
  if (!d["port"].isNull()) {
    long p = d["port"].as<long>();
    if (p < 1 || p > 65535) return "port: 1-65535";
    n.port = p;
  }
  if (!d["dalarm"].isNull()) {
    float a = d["dalarm"].as<float>();
    if (a < 0 || a > 100) return "depth alarm: 0-100 m";
    n.depthAlarm = a;
  }
  if (!d["night"].isNull()) n.night = d["night"].as<bool>();
  if (!d["rot"].isNull()) n.rot = d["rot"].as<int>() == 2 ? 2 : 0;
  if (!d["bright"].isNull()) {
    int bv = d["bright"].as<int>();
    if (bv < 5 || bv > 100) return "brightness: 5-100 %";
    n.bright = bv;
  }
  if (d["tz"].is<const char *>()) {
    String z = d["tz"].as<String>();
    z.trim();
    if (!z.length() || z.length() > 63) return "time zone: 1-63 characters (POSIX TZ string)";
    n.tz = z;
  }
  if (d["webpass"].is<const char *>()) {
    String w = d["webpass"].as<String>();
    if (w.length() < 4 || w.length() > 32) return "web password: 4-32 characters";
    n.webpass = w;
  }

  long long newEpoch = 0;
  if (!d["epoch"].isNull()) {
    newEpoch = d["epoch"].as<long long>();
    if (newEpoch < 1700000000LL || newEpoch > 4102444800LL) return "time: invalid date";
  }
  needRestart = n.ssid != cfg.ssid || n.pass != cfg.pass || n.name != cfg.name;
  bool srvChanged = n.host != cfg.host || n.port != cfg.port;
  bool look = n.night != cfg.night || n.rot != cfg.rot;
  cfg = n;
  saveCfg();
  applyTz();
  if (newEpoch) setClockEpoch((time_t)newEpoch);
  if (srvChanged) { nmea.stop(); srvUp = false; lastConnTry = 0; nConnTry = 0; }
  if (n.bright != 0) applyBrightness();
  if (look) { applyTheme(); cv->setRotation(cfg.rot == 2 ? 2 : 0); }
  dirty = true;
  return "";
}

String okJson(bool restart) { return String("{\"ok\":true,\"restart\":") + (restart ? "true" : "false") + "}"; }
String errJson(const String &e) { return String("{\"ok\":false,\"error\":\"") + e + "\"}"; }

// tasti dell'orologio dalla pagina web: right / left = scorrimento, dtap = doppio tocco (giorno/notte),
// power = tasto PWR (schermo acceso/spento), boot = tasto BOOT (snooze allarme)
bool sendKeyName(const String &k) {
  if (!screenOn && k != "power") screenSet(true);
  if (k == "right") handleKeys(EV_SHORT, EV_NONE, EV_NONE, EV_NONE);
  else if (k == "left") handleKeys(EV_NONE, EV_SHORT, EV_NONE, EV_NONE);
  else if (k == "dtap") handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_SHORT);
  else if (k == "power") pwrShortPress();
  else if (k == "boot") snoozeAlarm();
  else return false;
  return true;
}

void sendKeyChar(char k) {
  switch (k) {
    case 'U': handleKeys(EV_SHORT, EV_NONE, EV_NONE, EV_NONE); break;
    case 'D': handleKeys(EV_NONE, EV_SHORT, EV_NONE, EV_NONE); break;
    case 'O': handleKeys(EV_NONE, EV_NONE, EV_SHORT, EV_NONE); break;
    case 'L': handleKeys(EV_NONE, EV_NONE, EV_LONG, EV_NONE); break;
    case 'B': handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_SHORT); break;
    case 'K': handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_LONG); break;
  }
}

// ---------- comandi USB: "CGET", "CSTATUS", "CSCAN", "CSET {json}", "CRESTART" ----------
void configCmd(const String &c) {
  if (c == "GET") Serial.println("CFG " + cfgJson());
  else if (c == "STATUS") Serial.println("STATUS " + statusJson());
  else if (c == "SCAN") Serial.println("SCAN " + scanJson());
  else if (c == "BACKUP") Serial.println("BACKUP " + backupJson());
  else if (c == "FACTORY") { Serial.println("CFG_OK " + okJson(true)); Serial.flush(); factoryReset(); }
  else if (c == "RESTART") { Serial.println("CFG_OK " + okJson(false)); Serial.flush(); delay(200); ESP.restart(); }
  else if (c.startsWith("SET ")) {
    bool r = false;
    String e = applyCfg(c.substring(4), r);
    Serial.println(e.length() ? "CFG_ERR " + errJson(e) : "CFG_OK " + okJson(r));
  } else Serial.println("CFG_ERR " + errJson("unknown command"));
}

// ---------- pagina web ----------
// Login sempre richiesto via rete (utente admin). Via USB no: accesso fisico, serve a recuperare la password.
bool webAuth() {
  if (web.authenticate(WEB_USER, cfg.webpass.c_str())) return true;
  web.requestAuthentication(BASIC_AUTH, "EssentialWatch");
  return false;
}

void sendJson(const String &s, int code = 200) {
  web.sendHeader("Cache-Control", "no-store");
  web.send(code, "application/json", s);
}

// Schermo come BMP a 24 bit letto dal buffer di disegno RGB565: 410x502, oppure 205x251 con ?half=1
// (4 volte meno dati: per l'aggiornamento continuo del tool PC su un Wi-Fi lento)
void sendScreenBmp() {
  if (!cv) { web.send(503, "text/plain", "no screen buffer"); return; }
  const int step = web.hasArg("half") ? 2 : 1;
  const int W = SCR_W / step, H = SCR_H / step;
  const uint32_t rowB = ((uint32_t)W * 3 + 3) & ~3u;   // le righe del BMP sono allineate a 4 byte (1232 / 616)
  const uint32_t img = rowB * H, off = 54, total = off + img;
  uint8_t h[54] = {0};
  auto le32 = [&](int i, uint32_t v) { h[i] = v; h[i + 1] = v >> 8; h[i + 2] = v >> 16; h[i + 3] = v >> 24; };
  h[0] = 'B'; h[1] = 'M'; le32(2, total); le32(10, off);
  le32(14, 40); le32(18, W); le32(22, H); h[26] = 1; h[28] = 24; le32(34, img);
  web.sendHeader("Cache-Control", "no-store");
  web.setContentLength(total);
  web.send(200, "image/bmp", "");
  web.sendContent((const char *)h, sizeof h);
  uint8_t *row = (uint8_t *)calloc(rowB, 1);   // i byte di allineamento restano a 0
  if (!row) return;
  const uint16_t *fb = cv->getFramebuffer();
  for (int y = H - 1; y >= 0; y--) {           // BMP: righe dal basso verso l'alto
    const uint16_t *src = fb + (uint32_t)y * step * SCR_W;
    for (int x = 0; x < W; x++) {
      uint16_t p = src[x * step];
      uint8_t r = ((p >> 11) & 0x1F) * 255 / 31, g = ((p >> 5) & 0x3F) * 255 / 63, b = (p & 0x1F) * 255 / 31;
      row[x * 3] = b; row[x * 3 + 1] = g; row[x * 3 + 2] = r;
    }
    if (web.client().write(row, rowB) != rowB) break;   // il client e' sparito: non restare a scrivere nel vuoto
  }
  free(row);
}

// ---------- aggiornamento firmware ----------
bool otaAuthOk = false;
String otaErr;
size_t otaBytes = 0;

void otaUpload() {
  HTTPUpload &up = web.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaAuthOk = web.authenticate(WEB_USER, cfg.webpass.c_str());
    otaErr = "";
    otaBytes = 0;
    if (!otaAuthOk) return;
    Serial.printf("OTA: start %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) otaErr = Update.errorString();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!otaAuthOk || otaErr.length()) return;
    if (otaBytes == 0 && up.buf[0] != 0xE9) { otaErr = "not an ESP32 firmware file (.bin)"; Update.abort(); return; }
    if (Update.write(up.buf, up.currentSize) != up.currentSize) otaErr = Update.errorString();
    otaBytes += up.currentSize;
  } else if (up.status == UPLOAD_FILE_END) {
    if (!otaAuthOk || otaErr.length()) return;
    if (!Update.end(true)) otaErr = Update.errorString();
    else Serial.printf("OTA: %u bytes written\n", (unsigned)otaBytes);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaErr = "upload aborted";
  }
}

void webSetup() {
  web.on("/", HTTP_GET, [] {
    if (!webAuth()) return;
    web.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
  });
  web.on("/api/config", HTTP_GET, [] { if (webAuth()) sendJson(cfgJson()); });
  web.on("/api/config", HTTP_POST, [] {
    if (!webAuth()) return;
    bool r = false;
    String e = applyCfg(web.arg("plain"), r);
    sendJson(e.length() ? errJson(e) : okJson(r), e.length() ? 400 : 200);
  });
  web.on("/api/status", HTTP_GET, [] { if (webAuth()) sendJson(statusJson()); });
  web.on("/api/scan", HTTP_GET, [] { if (webAuth()) sendJson(scanJson()); });
  web.on("/api/restart", HTTP_POST, [] {
    if (!webAuth()) return;
    sendJson(okJson(false));
    delay(300);
    ESP.restart();
  });
  web.on("/api/backup", HTTP_GET, [] {
    if (!webAuth()) return;
    web.sendHeader("Content-Disposition", String("attachment; filename=\"") + cfg.name + "-config.json\"");
    sendJson(backupJson());
  });
  web.on("/api/factory", HTTP_POST, [] {
    if (!webAuth()) return;
    sendJson(okJson(true));
    delay(300);
    factoryReset();
  });
  web.on("/api/key", HTTP_POST, [] {
    if (!webAuth()) return;
    String k = web.arg("k");
    if (sendKeyName(k)) sendJson(okJson(false));
    else if (k.length() == 1 && strchr("UDOLBK", k[0])) { sendKeyChar(k[0]); sendJson(okJson(false)); }
    else sendJson(errJson("k = right left dtap power boot (or U D O L B K)"), 400);
  });
  web.on("/screen.bmp", HTTP_GET, [] { if (webAuth()) sendScreenBmp(); });
  // aggiornamento firmware (OTA): POST multipart con il file firmware.bin
  web.on("/api/update", HTTP_POST, [] {
    if (!otaAuthOk) { web.requestAuthentication(BASIC_AUTH, "EssentialView"); return; }
    bool ok = !otaErr.length() && !Update.hasError();
    sendJson(ok ? okJson(true) : errJson(otaErr.length() ? otaErr : String(Update.errorString())), ok ? 200 : 400);
    if (ok) {
      Serial.println("OTA: done, restarting");
      showMsg("Firmware updated", "Restarting...");
      delay(500);
      ESP.restart();
    }
  }, [] { otaUpload(); });
  web.onNotFound([] {
    if (apOn && web.hostHeader() != WiFi.softAPIP().toString()) {  // richieste "captive portal" dei telefoni
      web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
      web.send(302, "text/plain", "");
      return;
    }
    web.send(404, "text/plain", "not found");
  });
  web.begin();
}

// Rete di configurazione: si accende se la rete di bordo non e' raggiungibile dopo CONFIG_AP_AFTER_MS
// (o se non c'e' un SSID), si spegne quando il WiFi di bordo e' connesso e nessuno e' collegato all'AP.
// Primo avvio (nessuna rete salvata): rete APERTA SETUP_AP_NAME, solo access point.
// Rete di bordo salvata ma irraggiungibile: rete di emergenza "EssentialView-XXXX" con password.
void startConfigAP() {
  bool first = !cfg.ssid.length();
  if (first) {
    apName = SETUP_AP_NAME;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str());   // aperta
  } else {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char b[32];
    snprintf(b, sizeof b, "EssentialView-%02X%02X", mac[4], mac[5]);
    apName = b;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName.c_str(), CONFIG_AP_PASS);
    WiFi.setAutoReconnect(false);   // i tentativi continui disturberebbero l'AP: riprova ogni 60 s
  }
  dns.start(53, "*", WiFi.softAPIP());
  apOn = true;
  apRetryT = millis();
  Serial.printf("Config WiFi on: %s%s%s  http://%s\n", apName.c_str(), first ? " (open)" : " / ",
                first ? "" : CONFIG_AP_PASS, WiFi.softAPIP().toString().c_str());
  dirty = true;
}

void netCfgLoop() {
  static bool mdnsOn = false;
  static uint32_t lastSta = 0;   // ultima volta che il WiFi di bordo era connesso
  web.handleClient();
  if (apOn) dns.processNextRequest();
  uint32_t now = millis();
  bool sta = WiFi.status() == WL_CONNECTED;
  if (sta) lastSta = now;
  if (!apOn && !sta && (now - lastSta > CONFIG_AP_AFTER_MS || !cfg.ssid.length())) startConfigAP();
  if (apOn) {
    if (sta && WiFi.softAPgetStationNum() == 0) {
      dns.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(true);
      apOn = false;
      Serial.println("Config WiFi off (boat WiFi connected)");
      dirty = true;
    } else if (!sta && cfg.ssid.length() && WiFi.softAPgetStationNum() == 0 && now - apRetryT > 60000) {
      apRetryT = now;
      WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
    }
  }
  if (!mdnsOn && (sta || apOn)) {
    mdnsOn = MDNS.begin(cfg.name.c_str());
    if (mdnsOn) MDNS.addService("http", "tcp", 80);
  }
}
