// EssentialWatch - configuration: web page (port 80), configuration WiFi network, USB commands "C...".
// Included by the main sketch after all the device functions (uses cfg, display, handleKeys, ...).
// The same JSON functions serve the web page and the PC tool (EssentialView Configuration and Test Tool).
#pragma once
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include "mbedtls/sha256.h"
#include <lwip/sockets.h>
#include <fcntl.h>
#include <DNSServer.h>
#include "webpage.h"
#include "web_fonts.h"   // Essentialsail fonts served to the page

WebServer web(80);
DNSServer dns;  // configuration network: every name leads to the page (the phone opens it by itself)
bool apOn = false;
uint32_t apRetryT = 0;
String apName;

// ---------- Common JSON (web + USB) ----------
String cfgJson() {
  JsonDocument d;
  d["name"] = cfg.name;
  d["ssid"] = cfg.ssid;
  d["pass_set"] = cfg.pass.length() > 0;    // the password never leaves the device
  d["host"] = hostTemp ? savedHost : cfg.host;   // always the saved server
  d["port"] = hostTemp ? savedPort : cfg.port;
  d["dalarm"] = serialized(String(cfg.depthAlarm, 1));
  d["cpa"] = serialized(String(cfg.cpaNm, 2));
  d["tcpa"] = serialized(String(cfg.tcpaMin, 0));
  d["lang"] = LANG_CODES[cfg.lang];
  d["night"] = cfg.theme == THEME_NIGHT;
  d["theme"] = cfg.theme;
  d["sleep"] = cfg.sleepSec;
  d["raise"] = cfg.raise;
  d["rot"] = cfg.rot;
  d["tz"] = cfg.tz;
  d["bright"] = cfg.bright;
  d["webuser"] = WEB_USER;
  d["vib"] = cfg.vibPin;
  d["webpass_default"] = cfg.webpass == WEB_DEFAULT_PASS;
  d["mobon"] = cfg.mobOn;
  d["mobhost"] = cfg.mobHost;
  d["mobport"] = cfg.mobPort;
  d["mobid"] = cfg.mobId;
  d["mobkey_set"] = cfg.mobKey.length() == 64;   // the key never leaves the device
  d["pair"] = pairStateStr();   // pairing with a code: idle / busy / ok / fail
  d["mobowner"] = cfg.mobOwner;
  d["gpsrx"] = cfg.gpsRx;
  d["gpsbaud"] = cfg.gpsBaud;
  d["fw"] = FW_VERSION;
  String s;
  serializeJson(d, s);
  return s;
}

// Settings backup (.json file). Contains the WiFi password, NOT the administrator one.
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
  d["cpa"] = serialized(String(cfg.cpaNm, 2));
  d["tcpa"] = serialized(String(cfg.tcpaMin, 0));
  d["lang"] = LANG_CODES[cfg.lang];
  d["night"] = cfg.theme == THEME_NIGHT;
  d["theme"] = cfg.theme;
  d["sleep"] = cfg.sleepSec;
  d["raise"] = cfg.raise;
  d["rot"] = cfg.rot;
  d["tz"] = cfg.tz;
  d["mobon"] = cfg.mobOn;   // the MOB pairing (id, key) is not part of the backup: it is a secret, pair again after a restore
  d["mobhost"] = cfg.mobHost;
  d["mobport"] = cfg.mobPort;
  d["mobowner"] = cfg.mobOwner;
  d["gpsrx"] = cfg.gpsRx;
  d["gpsbaud"] = cfg.gpsBaud;
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
  d["mob"] = cfg.mobOn ? (mob.st == MOB_IDLE ? "idle" : (mob.st == MOB_ALARM ? "alarm" : "cancelling")) : "off";
  d["mobok"] = mob.okNow == 1;   // configured, Wi-Fi up
  d["mobsent"] = mob.nSent; d["mobacks"] = mob.nAck; d["mobseq"] = mob.seq;
  d["mobackage"] = mob.nAck ? (int)((millis() - mob.lastAckMs) / 1000) : -1;
  if (moi.have) {   // boat -> watch: last $MOI from the tablet
    d["moi"] = (int)((millis() - moi.rxMs) / 1000);   // its age in seconds
    d["moidist"] = moi.m.dist; d["moibrg"] = moi.m.brg; d["moisiren"] = moi.m.siren;
  }
  d["moisirenon"] = mobSirenOn(); d["moiacc"] = moi.nAcc; d["moidrop"] = moi.nDrop;
  d["gps"] = gps.on;   // optional GPS module of the watch
  if (gps.on) {
    d["gpsfix"] = gpsAge() >= 0;
    if (gpsAge() >= 0) { d["gpslat"] = serialized(String(gps.lat, 5)); d["gpslon"] = serialized(String(gps.lon, 5)); d["gpsage"] = gpsAge(); }
    d["gpssats"] = gps.sats; d["gpslines"] = gps.lines; d["gpsbad"] = gps.bad;
  }
  d["rx"] = nRx; d["ok"] = nOk; d["badcs"] = nBadCs; d["other"] = nUnknown;
  auto put = [&](const char *k, const Val &v, int dec) { if (v.ok()) d[k] = serialized(String(v.v, dec)); };
  put("awa", awa, 0); put("aws", aws, 1); put("sog", sog, 1); put("stw", stw, 1);
  put("hdg", hdg, 0); put("depth", depth, 1);
  float ta, ts;
  if (getTrue(ta, ts)) { d["twa"] = serialized(String(ta, 0)); d["tws"] = serialized(String(ts, 1)); }
  d["uptime"] = millis() / 1000;
  {  // clock, battery, keys
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
  d["vib"] = cfg.vibPin; d["vibon"] = vibNow;
    d["screen"] = screenOn;
    d["imu"] = imuAddr != 0;
    if (imuAddr) { JsonArray g = d["g"].to<JsonArray>(); for (int i = 0; i < 3; i++) g.add(serialized(String(imuG[i], 2))); }   // acceleration in g (x, y, z)
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

// Applies the keys present in the JSON (missing ones stay as they are), saves to memory.
// "pass"/"webpass" present = new passwords (even empty). Returns "" if ok, otherwise the error.
String applyCfg(const String &body, bool &needRestart, bool fromWeb = false) {
  JsonDocument d;
  if (deserializeJson(d, body)) return "invalid JSON";
  if (hostTemp) {  // a save closes the temporary redirection of the test bench
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
  if (!d["cpa"].isNull()) {
    float a = d["cpa"].as<float>();
    if (a < 0.05f || a > 5) return "collision CPA: 0.05-5 nm";
    n.cpaNm = a;
  }
  if (!d["tcpa"].isNull()) {
    float a = d["tcpa"].as<float>();
    if (a < 1 || a > 120) return "collision TCPA: 1-120 min";
    n.tcpaMin = a;
  }
  if (!d["lang"].isNull()) {
    int l = langFromCode(d["lang"].as<const char *>() ? d["lang"].as<const char *>() : "");
    if (l < 0) return "language: en, fr, es";
    n.lang = l;
  }
  if (!d["sleep"].isNull()) {
    int v = d["sleep"].as<int>();
    if (v < 0 || v > 3600) return "screen timeout: 0 (never) to 3600 s";
    n.sleepSec = v;
  }
  if (!d["raise"].isNull()) n.raise = d["raise"].as<bool>();
  if (!d["night"].isNull()) n.theme = d["night"].as<bool>() ? THEME_NIGHT : THEME_DAY;   // older clients and backups
  if (!d["theme"].isNull()) {
    int t = d["theme"].as<int>();
    if (t < 0 || t >= THEME_COUNT) return "theme: 0 day, 1 elegant black, 2 night";
    n.theme = t;
  }
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
  if (!d["mobon"].isNull()) n.mobOn = d["mobon"].as<bool>();
  if (d["mobhost"].is<const char *>()) {
    String h = d["mobhost"].as<String>();
    h.trim();
    if (h.length() > 63) return "MOB address: up to 63 characters";
    for (char c : h) if (!isalnum((unsigned char)c) && c != '.' && c != '-') return "MOB address: IP address or host name";
    n.mobHost = h;
  }
  if (!d["mobport"].isNull()) {
    long p = d["mobport"].as<long>();
    if (p < 1 || p > 65535) return "MOB port: 1-65535";
    n.mobPort = p;
  }
  if (d["mobid"].is<const char *>()) {
    String v = d["mobid"].as<String>();
    v.trim();
    v.toUpperCase();
    if (v.length() && !mobIdOk(v)) return "MOB device ID: 4-8 characters, A-Z and 0-9";
    n.mobId = v;
  }
  if (d["mobkey"].is<const char *>()) {   // present = new key (empty = remove it)
    String v = d["mobkey"].as<String>();
    v.trim();
    v.toLowerCase();
    uint8_t kb[32];
    if (v.length() && !mobHexKey(v, kb)) return "MOB key: exactly 64 hexadecimal characters";
    n.mobKey = v;
  }
  if (d["mobowner"].is<const char *>()) {
    String v = d["mobowner"].as<String>();
    v.trim();
    size_t chars = 0;
    for (size_t i = 0; i < v.length(); i++) if (((uint8_t)v[i] & 0xC0) != 0x80) chars++;   // UTF-8 characters
    if (chars > 24) return "MOB owner name: up to 24 characters";
    n.mobOwner = v;
  }
  if (d["webpass"].is<const char *>()) {
    String w = d["webpass"].as<String>();
    if (w.length() < 4 || w.length() > 32) return "web password: 4-32 characters";
    if (fromWeb && w == WEB_DEFAULT_PASS) return "choose a password different from the default one";
    n.webpass = w;
  }

  int8_t vibNew = cfg.vibPin;
  if (!d["vib"].isNull()) {   // optional external vibration motor driver: only free GPIOs of the board pads
    int v = d["vib"].as<int>();
    static const int8_t okPins[] = {1, 2, 3, 17, 18, 21, 43, 44, 47, 48};
    bool good = v == -1;
    for (int8_t o : okPins) if (o == v) good = true;
    if (!good) return "vibration pin: -1 (none) or one of GPIO 1 2 3 17 18 21 43 44 47 48";
    vibNew = v;
  }

  if (!d["gpsrx"].isNull()) {   // optional GPS module: only the free pads of the board, never the vibration motor pin
    int v = d["gpsrx"].as<int>();
    static const int8_t okGps[] = {1, 2, 3, 17, 21, 43, 44, 47, 48};
    bool good = v == -1;
    for (int8_t o : okGps) if (o == v) good = true;
    if (!good) return "GPS pin: -1 (none) or one of GPIO 1 2 3 17 21 43 44 47 48";
    if (v >= 0 && v == vibNew) return "GPS pin: already used by the vibration motor";
    n.gpsRx = v;
  }
  if (!d["gpsbaud"].isNull()) {
    uint32_t b = d["gpsbaud"].as<uint32_t>();
    if (b != 4800 && b != 9600 && b != 19200 && b != 38400 && b != 57600 && b != 115200) return "GPS baud rate: 4800, 9600, 19200, 38400, 57600 or 115200";
    n.gpsBaud = b;
  }
  if (n.gpsRx >= 0 && n.gpsRx == vibNew) return "GPS pin: already used by the vibration motor";

  long long newEpoch = 0;
  if (!d["epoch"].isNull()) {
    newEpoch = d["epoch"].as<long long>();
    if (newEpoch < 1700000000LL || newEpoch > 4102444800LL) return "time: invalid date";
  }
  needRestart = n.ssid != cfg.ssid || n.pass != cfg.pass || n.name != cfg.name;
  bool srvChanged = n.host != cfg.host || n.port != cfg.port;
  bool look = n.theme != cfg.theme || n.rot != cfg.rot;
  bool mobChanged = n.mobOwner != cfg.mobOwner || n.mobId != cfg.mobId || n.mobKey != cfg.mobKey || n.mobHost != cfg.mobHost;
  bool gpsChanged = n.gpsRx != cfg.gpsRx || n.gpsBaud != cfg.gpsBaud;
  cfg = n;
  if (gpsChanged) gpsBegin();
  if (mobChanged) mob.lastNameMs = 0;   // the tablet gets the (new) owner name at once
  if (vibNew != cfg.vibPin) vibSetPin(vibNew);   // releases the old pin, drives the new one low
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

// watch keys from the web page: right / left = swipe, dtap = double tap (next theme),
// power = PWR button (screen on/off), boot = BOOT button (alarm snooze)
bool sendKeyName(const String &k) {
  if (!screenOn && k != "power") screenSet(true);
  if (k == "right") handleKeys(EV_SHORT, EV_NONE, EV_NONE, EV_NONE);
  else if (k == "left") handleKeys(EV_NONE, EV_SHORT, EV_NONE, EV_NONE);
  else if (k == "dtap") handleKeys(EV_NONE, EV_NONE, EV_NONE, EV_SHORT);
  else if (k == "power") pwrShortPress();
  else if (k == "boot") snoozeAlarm();
  else if (k == "vibtest") { if (cfg.vibPin < 0) return false; vibTest(); }
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

// ---------- USB commands: "CGET", "CSTATUS", "CSCAN", "CSET {json}", "CRESTART" ----------
void configCmd(const String &c) {
  if (c == "GET") Serial.println("CFG " + cfgJson());
  else if (c == "STATUS") Serial.println("STATUS " + statusJson());
  else if (c == "SCAN") Serial.println("SCAN " + scanJson());
  else if (c == "BACKUP") Serial.println("BACKUP " + backupJson());
  else if (c == "MOBVEC") {   // test: the lines of the specification's vectors 5, 9 and 10 (key 00..1f, id W7K2)
    uint8_t k[32];
    for (int i = 0; i < 32; i++) k[i] = i;
    Serial.print("MOBVEC " + mobBuildLine(k, "W7K2", 422, 'A', 84, ""));
    Serial.print("MOBVEC " + mobBuildLine(k, "W7K2", 426, 'N', 85, "Anna Rossi"));
    Serial.print("MOBVEC " + mobBuildLine(k, "W7K2", 427, 'N', 84, "José"));
  }
  else if (c == "MOIVEC") {   // test: the boat -> watch lines of the specification (13.3), key 00..1f, id W7K2
    uint8_t k[32];
    for (int i = 0; i < 32; i++) k[i] = i;
    const char *lines[] = {"$MOI,1,W7K2,0007,142,287,0,3,45.12345,9.12345,150c82fd*0A", "$MOI,1,W7K2,0008,138,285,1,2,45.12345,9.12345,465212ca*51",
                           "$MOI,1,W7K2,0009,,,1,,,,42af3bc1*59", "$MOI,1,W7K2,0009,,,1,,,,42af3bc2*5A", "$MOI,1,ZZZZ,0009,,,1,,,,42af3bc1*59"};
    for (const char *l : lines) {
      MoiMsg m;
      bool ok = mobParseMoi(k, "W7K2", String(l), m);
      Serial.printf("MOIVEC %s seq=%u dist=%d brg=%d siren=%d age=%d boat=%.5f,%.5f  %s\n", ok ? "ACCEPT" : "REJECT", ok ? m.seq : 0, ok ? m.dist : 0, ok ? m.brg : 0,
                    ok ? m.siren : 0, ok ? m.age : 0, ok ? m.blat : 0.0, ok ? m.blon : 0.0, l);
    }
    Serial.print("MOIVEC MOK for 0008: " + mobBuildMok(k, "W7K2", 8));
  }
  else if (c.startsWith("GPSFEED ")) {   // test: feeds one NMEA sentence to the GPS parser
    bool ok = gpsFeed(c.substring(8));
    Serial.printf("GPSFEED %s fix=%d lat=%.5f lon=%.5f sats=%d\n", ok ? "POSITION" : "no position", (int)gps.fix, gps.lat, gps.lon, gps.sats);
  }
  else if (c.startsWith("PAIR ")) {   // test / USB: start pairing with the code shown on the tablet
    String e = pairStart(c.substring(5));
    Serial.println(e.length() ? "CFG_ERR " + errJson(e) : "CFG_OK " + okJson(false));
  }
  else if (c == "PAIRVEC") {   // test: the reference example of the specification (8.5): fixed keys, code 482913, id W7K2
    pairFree();
    String w;
    bool k = pairKeygen("1111111111111111111111111111111111111111111111111111111111111111", w);
    Serial.println(String("PAIRVEC wpub ") + (k ? w : String("ERROR")));
    String req = pairRequest("482913", w);
    Serial.println("PAIRVEC " + req);
    const String moq = "$MOQ,1,W7K2,04d65a93977caa3d1b081852ff57a79e465f1660577304baead505dd3a48589cf350185e895372df6221ea3a137557e473fddb6755f05bd507c3c533fce9c91285,c079233dcfe766ca*23";
    String id;
    uint8_t key[32];
    bool ok = pairComplete(moq, "482913", w, id, key);
    Serial.println(String("PAIRVEC reply ") + (ok ? "ACCEPT id " + id + " key " + pairHex(key, 32) : String("REJECT")));
    String bad = moq;
    bad.setCharAt(bad.length() - 4, '0');   // damaged mac (and checksum): must be rejected
    Serial.println(String("PAIRVEC damaged reply ") + (pairComplete(bad, "482913", w, id, key) ? "ACCEPT (WRONG)" : "REJECT"));
    Serial.println(String("PAIRVEC wrong code ") + (pairComplete(moq, "482914", w, id, key) ? "ACCEPT (WRONG)" : "REJECT"));
    pairFree();
  }
  else if (c == "MOBRAISE") { mobRaise(); page = PAGE_MOB; dirty = true; Serial.println("CFG_OK " + okJson(false)); }   // test, no confirmation
  else if (c == "MOBCANCEL") { mobCancel(); dirty = true; Serial.println("CFG_OK " + okJson(false)); }
  else if (c == "FACTORY") { Serial.println("CFG_OK " + okJson(true)); Serial.flush(); factoryReset(); }
  else if (c == "RESTART") { Serial.println("CFG_OK " + okJson(false)); Serial.flush(); delay(200); ESP.restart(); }
  else if (c.startsWith("SET ")) {
    bool r = false;
    String e = applyCfg(c.substring(4), r);
    Serial.println(e.length() ? "CFG_ERR " + errJson(e) : "CFG_OK " + okJson(r));
  } else Serial.println("CFG_ERR " + errJson("unknown command"));
}

void sendJson(const String &s, int code = 200) {
  web.sendHeader("Cache-Control", "no-store");
  web.send(code, "application/json", s);
}

// ---------- web page ----------
// Login always required over the network (user admin). Not over USB: physical access, used to recover the password.
// HTTP Digest authentication (RFC 2617, qop=auth): the password never travels on the network, only a hash tied to a
// server nonce. The nonce stays valid for a while (not renewed at every 401), so parallel browser requests do not
// invalidate each other. Wrong logins are counted per client IP and lock that IP out for a growing time.
#define AUTH_REALM "EssentialWatch"
#define AUTH_NONCE_MS 600000UL     // a nonce (and the previous one) is accepted for 10-20 minutes
#define AUTH_MAX_FAILS 5           // wrong logins before the lock-out
#define AUTH_LOCK_MS 30000UL       // first lock-out 30 s, doubling at each new lock-out up to about 16 minutes
enum AuthRes { AUTH_NONE, AUTH_STALE, AUTH_BAD, AUTH_LOCKED, AUTH_OK };
struct AuthTrack { uint32_t ip = 0; uint8_t fails = 0, level = 0; uint32_t lockUntil = 0, seen = 0; };
AuthTrack authTracks[4];
String authNonce, authNoncePrev;
uint32_t authNonceT = 0;

String randHex32() {
  char b[33];
  for (int i = 0; i < 4; i++) snprintf(b + i * 8, 9, "%08x", (unsigned)esp_random());
  return String(b);
}
String md5hex(const String &s) { MD5Builder m; m.begin(); m.add(s); m.calculate(); return m.toString(); }

// value of key=... in the Authorization header (quoted or not); "nonce" must not match "cnonce"
String dgParam(const String &h, const char *key) {
  String k = String(key) + "=";
  int from = 0;
  for (;;) {
    int i = h.indexOf(k, from);
    if (i < 0) return "";
    if (i == 0 || h[i - 1] == ' ' || h[i - 1] == ',') {
      int s = i + k.length();
      if (s < (int)h.length() && h[s] == '"') { int e = h.indexOf('"', s + 1); return e < 0 ? String("") : h.substring(s + 1, e); }
      int e = h.indexOf(',', s);
      if (e < 0) e = h.length();
      String v = h.substring(s, e);
      v.trim();
      return v;
    }
    from = i + 1;
  }
}

AuthTrack *authTrackFor(uint32_t ip) {
  AuthTrack *free_ = &authTracks[0];
  for (auto &t : authTracks) {
    if (t.ip == ip) { t.seen = millis(); return &t; }
    if (t.seen < free_->seen || !t.ip) free_ = &t;
  }
  *free_ = AuthTrack();
  free_->ip = ip; free_->seen = millis();
  return free_;
}

AuthRes digestCheck() {
  if (!web.hasHeader("Authorization")) return AUTH_NONE;
  String h = web.header("Authorization");
  if (!h.startsWith("Digest ")) return AUTH_BAD;   // Basic (clear-text password) is not accepted any more
  h = h.substring(7);
  String user = dgParam(h, "username"), realm = dgParam(h, "realm"), nonce = dgParam(h, "nonce"), uri = dgParam(h, "uri");
  String resp = dgParam(h, "response"), nc = dgParam(h, "nc"), cn = dgParam(h, "cnonce"), qop = dgParam(h, "qop");
  if (!nonce.length() || !resp.length() || !user.length()) return AUTH_BAD;
  if (nonce != authNonce && nonce != authNoncePrev) return AUTH_STALE;
  String path = uri;
  int q = path.indexOf('?');
  if (q >= 0) path = path.substring(0, q);
  if (path != web.uri() || realm != AUTH_REALM) return AUTH_BAD;
  const char *m = web.method() == HTTP_POST ? "POST" : web.method() == HTTP_PUT ? "PUT" : web.method() == HTTP_DELETE ? "DELETE" : "GET";
  String ha1 = md5hex(user + ":" + AUTH_REALM + ":" + cfg.webpass);
  String ha2 = md5hex(String(m) + ":" + uri);
  String expect = qop == "auth" ? md5hex(ha1 + ":" + nonce + ":" + nc + ":" + cn + ":auth:" + ha2) : md5hex(ha1 + ":" + nonce + ":" + ha2);
  resp.toLowerCase();
  bool ok = user.equalsConstantTime(WEB_USER) && resp.equalsConstantTime(expect);
  return ok ? AUTH_OK : AUTH_BAD;
}

// Checks the login of the current request (also counts failures / enforces the lock-out). Sends nothing.
AuthRes authEval() {
  uint32_t now = millis();
  if (authNonce.isEmpty() || now - authNonceT > AUTH_NONCE_MS) { authNoncePrev = authNonce; authNonce = randHex32(); authNonceT = now; }
  AuthTrack *t = authTrackFor((uint32_t)web.client().remoteIP());
  if (t->lockUntil && (int32_t)(t->lockUntil - now) > 0) return AUTH_LOCKED;
  AuthRes r = digestCheck();
  if (r == AUTH_OK) { t->fails = 0; t->level = 0; t->lockUntil = 0; }
  else if (r == AUTH_BAD && ++t->fails >= AUTH_MAX_FAILS) {
    t->fails = 0;
    t->lockUntil = now + (AUTH_LOCK_MS << t->level);
    if (!t->lockUntil) t->lockUntil = 1;
    Serial.printf("Web login: too many wrong passwords, client locked out for %u s\n", (unsigned)((AUTH_LOCK_MS << t->level) / 1000));
    if (t->level < 5) t->level++;
  }
  return r;
}

void authReject(AuthRes r) {
  if (r == AUTH_LOCKED) {
    web.sendHeader("Retry-After", "30");
    web.send(429, "text/plain", "Too many wrong passwords. Try again in a few minutes.");
    return;
  }
  String c = String("Digest realm=\"" AUTH_REALM "\", qop=\"auth\", algorithm=MD5, nonce=\"") + authNonce + "\", opaque=\"essentialwatch\"";
  if (r == AUTH_STALE) c += ", stale=true";
  web.sendHeader("WWW-Authenticate", c);
  web.send(401, "text/plain", "Login required");
}

bool defaultPass() { return cfg.webpass == WEB_DEFAULT_PASS; }
const char *PASS_FIRST_MSG = "change the default administrator password first (Administrator login, then Save)";

// allowDefault: the few pages needed to set the first password work even while the password is still "admin"
bool webAuth(bool allowDefault = false) {
  AuthRes r = authEval();
  if (r != AUTH_OK) { authReject(r); return false; }
  if (!allowDefault && defaultPass()) { sendJson(errJson(PASS_FIRST_MSG), 403); return false; }
  return true;
}

// Screen as a 24-bit BMP read from the RGB565 drawing buffer: 410x502, or 205x251 with ?half=1
// (4 times less data: for continuous updates of the PC tool on a slow WiFi)
void sendScreenBmp() {
  if (!cv) { web.send(503, "text/plain", "no screen buffer"); return; }
  const int step = web.hasArg("half") ? 2 : 1;
  const int W = SCR_W / step, H = SCR_H / step;
  const uint32_t rowB = ((uint32_t)W * 3 + 3) & ~3u;   // BMP rows are aligned to 4 bytes (1232 / 616)
  const uint32_t img = rowB * H, off = 54, total = off + img;
  uint8_t h[54] = {0};
  auto le32 = [&](int i, uint32_t v) { h[i] = v; h[i + 1] = v >> 8; h[i + 2] = v >> 16; h[i + 3] = v >> 24; };
  h[0] = 'B'; h[1] = 'M'; le32(2, total); le32(10, off);
  le32(14, 40); le32(18, W); le32(22, H); h[26] = 1; h[28] = 24; le32(34, img);
  web.sendHeader("Cache-Control", "no-store");
  web.setContentLength(total);
  web.send(200, "image/bmp", "");
  web.sendContent((const char *)h, sizeof h);
  // rows are sent in ~8 KB blocks (about 20% faster than one TCP write per row). The copy still keeps the
  // watch busy for about a second over Wi-Fi: callers should ask for it only when someone is looking.
  const int rowsPer = max(1, (int)(8192 / rowB));
  uint8_t *buf = (uint8_t *)calloc(rowB * rowsPer, 1);   // the alignment bytes stay 0
  if (!buf) return;
  const uint16_t *fb = cv->getFramebuffer();
  int filled = 0;
  for (int y = H - 1; y >= 0; y--) {           // BMP: rows from bottom to top
    const uint16_t *src = fb + (uint32_t)y * step * SCR_W;
    uint8_t *row = buf + filled * rowB;
    for (int x = 0; x < W; x++) {
      uint16_t p = src[x * step];
      uint8_t r = ((p >> 11) & 0x1F) * 255 / 31, g = ((p >> 5) & 0x3F) * 255 / 63, b = (p & 0x1F) * 255 / 31;
      row[x * 3] = b; row[x * 3 + 1] = g; row[x * 3 + 2] = r;
    }
    if (++filled == rowsPer || y == 0) {
      size_t len = (size_t)filled * rowB;
      if (web.client().write(buf, len) != len) break;   // the client is gone: do not keep writing into the void
      filled = 0;
    }
  }
  free(buf);
}

// ---------- firmware update ----------
bool otaAuthOk = false;
AuthRes otaRes = AUTH_NONE;
String otaErr;
size_t otaBytes = 0;

void otaUpload() {
  HTTPUpload &up = web.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaRes = authEval();
    otaAuthOk = otaRes == AUTH_OK && !defaultPass();
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

// Update from the watch itself: downloads the firmware from the GitHub Releases of this project (HTTPS with a verified
// certificate, SHA-256 checked) and installs it. The browser cannot download it (GitHub does not send the CORS headers).
String fwFromUrl(const String &url, const String &sha256hex) {
  const String prefix = "https://github.com/davideschiavon/EssentialWatch/releases/download/";
  if (!url.startsWith(prefix)) return "only firmware from the EssentialWatch GitHub releases can be installed";
  if (WiFi.status() != WL_CONNECTED) return "the watch is not connected to a WiFi network";
  if (!clockValid()) return "the watch clock is not set (needed for HTTPS): set the date and time first";
  showMsg("Downloading firmware", "from GitHub...");
  NetworkClientSecure client;
  client.useBuiltinCACertBundle();   // verify the server certificate chain with the built-in root certificates
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.setUserAgent("EssentialWatch");
  if (!http.begin(client, url)) return "cannot start the download";
  int code = http.GET();
  if (code != 200) {
    String e = "GitHub answered " + String(code) + (code < 0 ? " (" + http.errorToString(code) + ")" : String(""));
    http.end();
    return e;
  }
  int len = http.getSize();
  if (!Update.begin(len > 0 ? (size_t)len : UPDATE_SIZE_UNKNOWN, U_FLASH)) { String e = Update.errorString(); http.end(); return e; }
  WiFiClient *st = http.getStreamPtr();
  mbedtls_sha256_context sh;
  mbedtls_sha256_init(&sh);
  mbedtls_sha256_starts(&sh, 0);
  static uint8_t buf[1024];
  size_t total = 0;
  uint32_t lastData = millis();
  bool first = true;
  String err = "";
  while (http.connected() && (len < 0 || total < (size_t)len)) {
    size_t av = st->available();
    if (!av) {
      if (millis() - lastData > 20000) { err = "download timed out"; break; }
      delay(2);
      continue;
    }
    int n = st->readBytes(buf, min(av, sizeof buf));
    if (n <= 0) continue;
    lastData = millis();
    if (first) { first = false; if (buf[0] != 0xE9) { err = "not an ESP32 firmware file (.bin)"; break; } }
    if (Update.write(buf, n) != (size_t)n) { err = Update.errorString(); break; }
    mbedtls_sha256_update(&sh, buf, n);
    total += n;
  }
  uint8_t hash[32];
  mbedtls_sha256_finish(&sh, hash);
  mbedtls_sha256_free(&sh);
  http.end();
  if (!err.length() && len > 0 && total != (size_t)len) err = "download incomplete";
  if (!err.length() && sha256hex.length()) {
    char hx[65];
    for (int i = 0; i < 32; i++) sprintf(hx + 2 * i, "%02x", hash[i]);
    String want = sha256hex;
    want.toLowerCase();
    if (want != String(hx)) err = "checksum mismatch: the file was rejected";
  }
  if (err.length()) { Update.abort(); return err; }
  if (!Update.end(true)) return Update.errorString();
  Serial.printf("OTA from GitHub: %u bytes written\n", (unsigned)total);
  return "";
}

// ---------- "Search NMEA server": scans the /24 network of the watch ----------
// Stage 1: non-blocking TCP connects to every host on a few common NMEA ports (a small pool of sockets at a time).
// Stage 2: every open port is read for ~2 s; it is a real NMEA server if it sends lines starting with '$' or '!'.
struct NmeaHit { uint8_t host; uint16_t port; bool nmea; char sample[8]; };
static NmeaHit scanHits[16];
static volatile int scanNHits = 0, scanProgress = 0;
static volatile bool scanRunning = false;
static uint8_t scanBase[3] = {0, 0, 0};
static const uint16_t SCAN_PORTS[] = {10110, 10111, 10112, 2000, 4242, 3000};   // common NMEA / gateway ports
// The same scan also finds the EssentialSail tablet: mode 1 looks for the MOB listener (one TCP port, 10120 by default).
// A hit is "verified" when the watch is already paired and the tablet answers a signed "alive" message with a valid $MOA.
static volatile uint8_t scanMode = 0;       // 0 NMEA server, 1 EssentialSail (MOB listener)
static uint16_t scanMobPort = 10120;

static void nmeaScanTask(void *) {
  const bool mobScan = scanMode == 1;
  const uint16_t *ports = mobScan ? &scanMobPort : SCAN_PORTS;
  const int nPorts = mobScan ? 1 : sizeof(SCAN_PORTS) / sizeof(SCAN_PORTS[0]);
  const int total = 254 * nPorts, SLOTS = 5;
  const uint8_t myHost = WiFi.localIP()[3];
  struct Slot { int fd; uint32_t t0; uint8_t host; uint16_t port; } slot[SLOTS];
  for (auto &s : slot) s.fd = -1;
  struct Open { uint8_t host; uint16_t port; };
  static Open opens[48];
  int nOpen = 0, issued = 0, done = 0;
  while (done < total) {
    for (auto &s : slot) {   // refill the free slots
      while (s.fd < 0 && issued < total) {
        uint8_t host = 1 + issued / nPorts;
        uint16_t port = ports[issued % nPorts];
        if (host == myHost) { issued++; done++; continue; }
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) break;   // out of sockets: try again on the next round
        fcntl(fd, F_SETFL, O_NONBLOCK);
        struct sockaddr_in a;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = (uint32_t)IPAddress(scanBase[0], scanBase[1], scanBase[2], host);
        connect(fd, (struct sockaddr *)&a, sizeof a);   // returns EINPROGRESS
        s.fd = fd; s.t0 = millis(); s.host = host; s.port = port;
        issued++;
      }
    }
    fd_set wf;
    FD_ZERO(&wf);
    int maxfd = -1;
    for (auto &s : slot) if (s.fd >= 0) { FD_SET(s.fd, &wf); if (s.fd > maxfd) maxfd = s.fd; }
    if (maxfd >= 0) {
      struct timeval tv = {0, 20000};
      select(maxfd + 1, nullptr, &wf, nullptr, &tv);
    } else {
      vTaskDelay(5);
    }
    for (auto &s : slot) {
      if (s.fd < 0) continue;
      bool fin = false;
      if (FD_ISSET(s.fd, &wf)) {   // connect finished: success only if SO_ERROR is 0 (otherwise refused)
        int err = 0;
        socklen_t l = sizeof err;
        getsockopt(s.fd, SOL_SOCKET, SO_ERROR, &err, &l);
        if (err == 0 && nOpen < 48) opens[nOpen++] = {s.host, s.port};
        fin = true;
      } else if (millis() - s.t0 > 150) {
        fin = true;   // no answer: host absent
      }
      if (fin) { close(s.fd); s.fd = -1; done++; scanProgress = 90 * done / total; }
    }
  }
  for (int i = 0; i < nOpen && scanNHits < 16; i++) {   // stage 2: is it really NMEA?
    WiFiClient c;
    IPAddress ip(scanBase[0], scanBase[1], scanBase[2], opens[i].host);
    NmeaHit h = {opens[i].host, opens[i].port, false, ""};
    uint8_t mkey[32];
    if (mobScan) {   // verification: a signed "alive" message (state P) must come back as a valid $MOA on the same connection
      if (mobIdOk(cfg.mobId) && mobHexKey(cfg.mobKey, mkey) && c.connect(ip, opens[i].port, 800)) {
        uint16_t sq = mobNextSeq();
        String ln = mobBuildLine(mkey, cfg.mobId, sq, 'P', mobBattNow(), "");
        c.print(ln);
        uint32_t t = millis();
        String line;
        while (millis() - t < 1500 && c.connected() && !h.nmea) {
          while (c.available() && !h.nmea) {
            char ch = (char)c.read();
            if (ch == '\n') { if (mobCheckAck(mkey, cfg.mobId, line) == (int)sq) h.nmea = true; line = ""; }
            else if (ch != '\r' && line.length() < 100) line += ch;
          }
          vTaskDelay(10);
        }
        c.stop();
      }
    } else if (c.connect(ip, opens[i].port, 800)) {
      uint32_t t = millis();
      String line;
      while (millis() - t < 2000 && c.connected() && !h.nmea) {
        while (c.available() && !h.nmea) {
          char ch = (char)c.read();
          if (ch == '\n' || ch == '\r') {
            if (line.length() >= 6 && (line[0] == '$' || line[0] == '!')) { h.nmea = true; strncpy(h.sample, line.c_str(), 6); h.sample[6] = 0; }
            line = "";
          } else if (line.length() < 100) {
            line += ch;
          }
        }
        vTaskDelay(10);
      }
      c.stop();
    }
    scanHits[scanNHits++] = h;
    scanProgress = 90 + 10 * (i + 1) / nOpen;
  }
  scanProgress = 100;
  scanRunning = false;
  vTaskDelete(nullptr);
}

String nmeaScanStart(uint8_t mode = 0) {
  if (scanRunning) return "";
  scanMode = mode;
  scanMobPort = cfg.mobPort ? cfg.mobPort : 10120;
  if (WiFi.status() != WL_CONNECTED) return "the watch is not connected to a WiFi network";
  IPAddress me = WiFi.localIP();
  scanBase[0] = me[0]; scanBase[1] = me[1]; scanBase[2] = me[2];
  scanNHits = 0; scanProgress = 0; scanRunning = true;
  xTaskCreatePinnedToCore(nmeaScanTask, "nmeascan", 8192, nullptr, 1, nullptr, 0);
  return "";
}

String nmeaScanJson() {
  JsonDocument d;
  d["running"] = (bool)scanRunning;
  d["progress"] = (int)scanProgress;
  JsonArray a = d["hits"].to<JsonArray>();
  for (int i = 0; i < scanNHits; i++) {
    JsonObject o = a.add<JsonObject>();
    o["ip"] = IPAddress(scanBase[0], scanBase[1], scanBase[2], scanHits[i].host).toString();
    o["port"] = scanHits[i].port;
    o["nmea"] = scanHits[i].nmea;
    o["sample"] = String(scanHits[i].sample);
  }
  String s;
  serializeJson(d, s);
  return s;
}

void webSetup() {
  web.on("/fonts/EssentialsailDisplay-Bold.ttf", HTTP_GET, [] {   // no login: they are public (OFL) font files
    web.sendHeader("Cache-Control", "public, max-age=604800");
    web.send_P(200, "font/ttf", (PGM_P)FONT_DISPLAY, FONT_DISPLAY_LEN);
  });
  web.on("/fonts/EssentialsailText-Regular.ttf", HTTP_GET, [] {
    web.sendHeader("Cache-Control", "public, max-age=604800");
    web.send_P(200, "font/ttf", (PGM_P)FONT_TEXT, FONT_TEXT_LEN);
  });
  web.on("/", HTTP_GET, [] {
    if (!webAuth(true)) return;
    web.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
  });
  web.on("/api/config", HTTP_GET, [] { if (webAuth(true)) sendJson(cfgJson()); });
  web.on("/api/config", HTTP_POST, [] {
    if (!webAuth(true)) return;
    // while the password is still the default one, the only thing that can be saved is a new password
    if (defaultPass() && web.arg("plain").indexOf("\"webpass\"") < 0) { sendJson(errJson(PASS_FIRST_MSG), 403); return; }
    bool r = false;
    String e = applyCfg(web.arg("plain"), r, true);
    sendJson(e.length() ? errJson(e) : okJson(r), e.length() ? 400 : 200);
  });
  web.on("/api/status", HTTP_GET, [] { if (webAuth(true)) sendJson(statusJson()); });
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
  // firmware update (OTA): multipart POST with the firmware.bin file
  web.on("/api/update", HTTP_POST, [] {
    if (!otaAuthOk) { if (otaRes == AUTH_OK) sendJson(errJson(PASS_FIRST_MSG), 403); else authReject(otaRes); return; }
    bool ok = !otaErr.length() && !Update.hasError();
    sendJson(ok ? okJson(true) : errJson(otaErr.length() ? otaErr : String(Update.errorString())), ok ? 200 : 400);
    if (ok) {
      Serial.println("OTA: done, restarting");
      showMsg("Firmware updated", "Restarting...");
      delay(500);
      ESP.restart();
    }
  }, [] { otaUpload(); });
  web.on("/api/nmeascan", HTTP_POST, [] {
    if (!webAuth()) return;
    String e = nmeaScanStart();
    sendJson(e.length() ? errJson(e) : okJson(false), e.length() ? 400 : 200);
  });
  web.on("/api/nmeascan", HTTP_GET, [] { if (webAuth()) sendJson(nmeaScanJson()); });
  web.on("/api/mobscan", HTTP_POST, [] {   // "Search EssentialSail": the same scan, looking for the MOB listener of the tablet
    if (!webAuth()) return;
    String e = nmeaScanStart(1);
    sendJson(e.length() ? errJson(e) : okJson(false), e.length() ? 400 : 200);
  });
  web.on("/api/mobpair", HTTP_POST, [] {   // pairing with the 6-digit code shown on the tablet
    if (!webAuth()) return;
    JsonDocument pd;
    deserializeJson(pd, web.arg("plain"));
    String e = pairStart(pd["code"].as<String>());
    sendJson(e.length() ? errJson(e) : okJson(false), e.length() ? 400 : 200);
  });
  web.on("/api/mobpair", HTTP_GET, [] {
    if (!webAuth()) return;
    JsonDocument pd;
    pd["state"] = pairStateStr();
    if (pr.st == PAIR_OK) { pd["id"] = cfg.mobId; pd["host"] = cfg.mobHost; pd["port"] = cfg.mobPort; }
    String out;
    serializeJson(pd, out);
    sendJson(out);
  });
  web.on("/api/mobscan", HTTP_GET, [] { if (webAuth()) sendJson(nmeaScanJson()); });
  web.on("/api/fwurl", HTTP_POST, [] {
    if (!webAuth()) return;
    JsonDocument d;
    if (deserializeJson(d, web.arg("plain"))) { sendJson(errJson("invalid JSON"), 400); return; }
    String err = fwFromUrl(String(d["url"] | ""), String(d["sha256"] | ""));
    if (err.length()) { dirty = true; sendJson(errJson(err), 400); return; }
    sendJson(okJson(true));
    showMsg("Firmware updated", "Restarting...");
    delay(500);
    ESP.restart();
  });
  web.onNotFound([] {
    if (apOn && web.hostHeader() != WiFi.softAPIP().toString()) {  // "captive portal" requests from phones
      web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
      web.send(302, "text/plain", "");
      return;
    }
    web.send(404, "text/plain", "not found");
  });
  web.begin();
}

// Configuration network: switches on if the boat network is not reachable after CONFIG_AP_AFTER_MS
// (or if there is no SSID), switches off when the boat WiFi is connected and nobody is connected to the AP.
// First start (no saved network): OPEN network SETUP_AP_NAME, access point only.
// Boat network saved but unreachable: emergency network "EssentialView-XXXX" with password.
void startConfigAP() {
  bool first = !cfg.ssid.length();
  if (first) {
    apName = SETUP_AP_NAME;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str());   // open
  } else {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char b[32];
    snprintf(b, sizeof b, "EssentialView-%02X%02X", mac[4], mac[5]);
    apName = b;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName.c_str(), CONFIG_AP_PASS);
    WiFi.setAutoReconnect(false);   // continuous attempts would disturb the AP: retry every 60 s
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
  static uint32_t lastSta = 0;   // last time the boat WiFi was connected
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
