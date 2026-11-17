// EssentialWatch - Man Overboard (MOB) link to EssentialSail on the tablet, over Wi-Fi (UDP).
// Protocol: EssentialSail "MOB / Watch Protocol" specification, version 1.
//   $MOB,1,<id>,<seq>,<state>,<lat>,<lon>,<fixage>,<hr>,<batt>[,<name>],<sig>*<cs>   (watch -> tablet)
//   $MOA,<id>,<seq>,<sig>*<cs>                                                         (tablet -> watch, acknowledgement)
// The watch has no GPS and no heart rate sensor: position, fix age and heart rate are always empty (the tablet then
// uses the boat position for the alarm).  Meshtastic is only reachable through a gateway that forwards these same
// bytes, so the "server address" can also be the address of such a gateway.
// Included by the main sketch after the settings and the battery state.
#pragma once
#include <WiFiUdp.h>
#include "mbedtls/md.h"
#include "gps.h"

enum MobSt : uint8_t { MOB_IDLE, MOB_ALARM, MOB_CANCEL };

#define MOB_NORMAL_MS   5500UL    // "I am alive" (P) and, once acknowledged, A messages
#define MOB_RETRY_MS    1000UL    // A or C without acknowledgement yet: same seq, every second
#define MOB_NAME_MS     60000UL   // owner's name message
#define MOB_FAIL_MS     10000UL   // no acknowledgement this long after raising: "Not delivered"
#define MOB_SEQ_AHEAD   100       // the stored counter is kept this far ahead of the last number sent
#define MOB_MAX_LINE    120       // the tablet drops longer lines

struct MobCtl {
  MobSt st = MOB_IDLE;
  uint16_t seq = 0;            // last sequence number used
  uint16_t limit = 0;          // value stored in flash (always ahead of seq)
  uint16_t pendSeq = 0xFFFF;   // A or C message waiting for its acknowledgement, 0xFFFF = none
  String pendLine;             // the A or C line waiting for its acknowledgement (resent as it is)
  bool pendAcked = false;
  bool ackedSinceRaise = false;
  uint32_t raiseMs = 0, lastSend = 0, lastNameMs = 0, lastAckMs = 0, cancelArmUntil = 0;
  uint32_t nSent = 0, nAck = 0;
  int8_t okNow = -1;           // -1 unknown, 0 not ready (configuration or Wi-Fi), 1 ready
  WiFiUDP udp;
  bool udpOpen = false;
} mob;

// ---------- helpers ----------
static int mobHexVal(char c) {
  return c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1));
}
static bool mobHexKey(const String &hex, uint8_t *out) {
  if (hex.length() != 64) return false;
  for (int i = 0; i < 32; i++) {
    int x = mobHexVal(hex[i * 2]), y = mobHexVal(hex[i * 2 + 1]);
    if (x < 0 || y < 0) return false;
    out[i] = (uint8_t)(x * 16 + y);
  }
  return true;
}
static bool mobIdOk(const String &s) {
  if (s.length() < 4 || s.length() > 8) return false;
  for (char c : s) if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return false;
  return true;
}
// configured (address, id, key), independent from the Wi-Fi state
bool mobConfigured() {
  uint8_t k[32];
  return cfg.mobHost.length() && mobIdOk(cfg.mobId) && mobHexKey(cfg.mobKey, k);
}

// first 4 bytes of HMAC-SHA256(key, data) as 8 lowercase hex characters
static String mobHmac8(const uint8_t *key, const String &data) {
  uint8_t out[32];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key, 32, (const uint8_t *)data.c_str(), data.length(), out);
  char b[9];
  snprintf(b, sizeof b, "%02x%02x%02x%02x", out[0], out[1], out[2], out[3]);
  return String(b);
}
static String mobChecksum(const String &inner) {
  uint8_t c = 0;
  for (size_t i = 0; i < inner.length(); i++) c ^= (uint8_t)inner[i];
  char b[3];
  snprintf(b, sizeof b, "%02X", c);
  return String(b);
}

// Name in UTF-8, at most maxChars characters, percent-encoded (A-Z a-z 0-9 . _ - stay, every other byte is %XX).
static String mobEncodeName(const String &name, size_t maxChars) {
  String out;
  size_t chars = 0;
  for (size_t i = 0; i < name.length() && chars < maxChars;) {
    uint8_t c = (uint8_t)name[i];
    size_t n = c < 0x80 ? 1 : (c >= 0xF0 ? 4 : (c >= 0xE0 ? 3 : (c >= 0xC0 ? 2 : 1)));
    if (i + n > name.length()) break;
    for (size_t k = 0; k < n; k++) {
      uint8_t b = (uint8_t)name[i + k];
      if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9') || b == '.' || b == '_' || b == '-') out += (char)b;
      else { char h[4]; snprintf(h, sizeof h, "%%%02X", b); out += h; }
    }
    i += n;
    chars++;
  }
  return out;
}

// Builds one line (with CR LF). state: 'A' 'P' 'C' 'N'; batt < 0 = empty; name used for 'N' only.
// For 'N' the name is shortened (whole characters) until the line fits the 120-byte limit; empty name -> "" (not sent).
// lat/lon (decimal degrees, NAN = empty) and fixage (seconds, -1 = empty) come from the watch GPS (gps.h) when it has one; hr is always empty.
String mobBuildLine(const uint8_t *key, const String &id, uint16_t seq, char state, int batt, const String &name,
                    double lat = NAN, double lon = NAN, int fixage = -1) {
  char head[96];
  bool pos = !isnan(lat) && !isnan(lon) && fabs(lat) <= 90 && fabs(lon) <= 180;
  if (pos) snprintf(head, sizeof head, "MOB,1,%s,%04u,%c,%.5f,%.5f,", id.c_str(), (unsigned)seq, state, lat, lon);
  else snprintf(head, sizeof head, "MOB,1,%s,%04u,%c,,,", id.c_str(), (unsigned)seq, state);
  String body = head;
  if (pos && fixage >= 0 && fixage <= 999) body += String(fixage);
  body += ",,";   // end of fixage, hr empty
  if (batt >= 0 && batt <= 100) body += String(batt);
  if (state == 'N') {
    size_t maxc = 24;
    for (;;) {
      String enc = mobEncodeName(name, maxc);
      if (!enc.length()) return "";
      String b2 = body + "," + enc;
      if (b2.length() + 13 <= MOB_MAX_LINE) { body = b2; break; }   // "$" + body + "," + sig(8) + "*" + cs(2) = body + 13
      if (maxc == 0) return "";
      maxc--;
    }
  }
  String inner = body + "," + mobHmac8(key, body);
  return "$" + inner + "*" + mobChecksum(inner) + "\r\n";
}

// Verifies "$MOA,<id>,<seq>,<sig>*<cs>"; returns the sequence number or -1.
static int mobCheckAck(const uint8_t *key, const String &id, String line) {
  line.trim();
  int star = line.lastIndexOf('*');
  if (!line.startsWith("$MOA,") || star < 0 || line.length() != (unsigned)star + 3) return -1;
  String inner = line.substring(1, star);
  if (!mobChecksum(inner).equalsIgnoreCase(line.substring(star + 1))) return -1;
  int c1 = inner.indexOf(','), c2 = inner.indexOf(',', c1 + 1), c3 = inner.indexOf(',', c2 + 1);
  if (c1 < 0 || c2 < 0 || c3 < 0 || inner.indexOf(',', c3 + 1) >= 0) return -1;
  if (inner.substring(c1 + 1, c2) != id) return -1;
  if (!mobHmac8(key, inner.substring(0, c3)).equalsIgnoreCase(inner.substring(c3 + 1))) return -1;
  return inner.substring(c2 + 1, c3).toInt();
}

// ---------- sequence number (kept in flash, written once every MOB_SEQ_AHEAD messages) ----------
static void mobStoreLimit() {
  Preferences p;
  if (p.begin("mob", false)) { p.putUShort("seq", mob.limit); p.end(); }
}
void mobInit() {
  Preferences p;
  uint16_t s = 0;
  if (p.begin("mob", true)) { s = p.getUShort("seq", 0); p.end(); }
  mob.seq = s % 10000;   // the stored value is ahead of every number sent before the reset
  mob.limit = (mob.seq + MOB_SEQ_AHEAD) % 10000;
  mobStoreLimit();
  mob.pendSeq = 0xFFFF;
}
static uint16_t mobNextSeq() {
  mob.seq = (mob.seq + 1) % 10000;
  if (mob.seq == mob.limit) { mob.limit = (mob.seq + MOB_SEQ_AHEAD) % 10000; mobStoreLimit(); }
  return mob.seq;
}

// ---------- sending ----------
static bool mobWifi() { return WiFi.status() == WL_CONNECTED; }
bool mobReady() { return cfg.mobOn && mobWifi() && mobConfigured(); }

static void mobSendLine(const String &line) {
  if (!line.length()) return;
  if (!mob.udpOpen) { mob.udpOpen = mob.udp.begin(0); if (!mob.udpOpen) return; }   // any free local port: the reply comes back to it
  if (mob.udp.beginPacket(cfg.mobHost.c_str(), cfg.mobPort)) {
    mob.udp.write((const uint8_t *)line.c_str(), line.length());
    mob.udp.endPacket();
    mob.nSent++;
  }
  mob.lastSend = millis();
}

static int mobBattNow() { return (batt.ok && batt.present && batt.pct >= 0) ? batt.pct : -1; }

// one new message (new sequence number); returns the number used
static uint16_t mobSend(char state) {
  uint8_t key[32];
  if (!mobHexKey(cfg.mobKey, key)) return 0xFFFF;
  uint16_t s = mobNextSeq();
  int ga = gpsAge();   // the position of the wearer, if the watch has a GPS with a fix (not for the name message)
  bool usePos = state != 'N' && ga >= 0;
  String line = mobBuildLine(key, cfg.mobId, s, state, mobBattNow(), cfg.mobOwner,
                             usePos ? gps.lat : NAN, usePos ? gps.lon : NAN, usePos ? ga : -1);
  if (state == 'A' || state == 'C') mob.pendLine = line;   // a retransmission repeats exactly this line
  mobSendLine(line);
  return s;
}
// the same message again (same line, same sequence number): only for A and C while waiting for the acknowledgement
static void mobResend() {
  mobSendLine(mob.pendLine);
}

static void mobSendName() {
  if (cfg.mobOwner.length()) mobSend('N');
  mob.lastNameMs = millis();
}

// ---------- actions from the watch ----------
void mobRaise() {
  if (mob.st != MOB_IDLE) return;
  mob.st = MOB_ALARM;
  mob.raiseMs = millis();
  mob.ackedSinceRaise = false;
  mob.pendAcked = false;
  mob.cancelArmUntil = 0;
  mob.pendSeq = 0xFFFF;   // mobLoop sends it (at once if everything is ready, otherwise as soon as it can)
  mob.lastNameMs = 0;     // the name goes out first, so the tablet knows it before the alarm
}
void mobCancel() {
  if (mob.st != MOB_ALARM) return;
  mob.st = MOB_CANCEL;
  mob.cancelArmUntil = 0;
  mob.pendSeq = 0xFFFF;
  mob.pendAcked = false;
}
bool mobCancelArmed() { return mob.cancelArmUntil && (int32_t)(mob.cancelArmUntil - millis()) > 0; }
// Long press on the MOB page: raises the alarm. When raised, the first long press arms the cancel, the second confirms it.
void mobHold() {
  if (mob.st == MOB_IDLE) mobRaise();
  else if (mob.st == MOB_ALARM) {
    if (mobCancelArmed()) mobCancel();
    else mob.cancelArmUntil = millis() + 6000;
  }
}

// state for the page: 0 idle, 1 delivering, 2 delivered, 3 not delivered, 4 cancelling
int mobUiState() {
  if (mob.st == MOB_IDLE) return 0;
  if (mob.st == MOB_CANCEL) return 4;
  uint32_t now = millis();
  if (mob.ackedSinceRaise) return (now - mob.lastAckMs < 20000) ? 2 : 3;   // acknowledgements stopped coming: not delivered
  return (now - mob.raiseMs >= MOB_FAIL_MS) ? 3 : 1;
}

// ---------- boat -> watch: $MOI (distance, bearing, siren) and our $MOK confirmation (specification, section 5b) ----------
//   $MOI,<ver>,<id>,<seq>,<dist m>,<bearing to the boat>,<siren 0/1>,<age s>,<boatlat>,<boatlon>,<sig>*<cs>
//   $MOK,<id>,<seq>,<sig>*<cs>      the watch confirms every accepted $MOI
#define MOI_STALE_MS   15000UL    // no $MOI this long: the numbers are greyed out and their age is shown
#define MOI_SIREN_MS   12000UL    // the siren stops when no message asks for it for this long (the tablet repeats every 5 s)
#define MOI_MUTE_MS    20000UL    // a tap mutes the siren for this long
#define MOI_RESET_MS   60000UL    // silence this long: the tablet probably restarted and its counter starts again

struct MoiMsg { uint16_t seq; int dist, brg, siren, age; double blat, blon; };   // dist, brg, age: -1 = unknown
struct MoiState {
  bool have = false;
  MoiMsg m = {0, -1, -1, 0, -1, NAN, NAN};
  uint32_t rxMs = 0, sirenMs = 0, muteUntil = 0, nAcc = 0, nDrop = 0;
} moi;

static String mobSplitField(const String &s, int idx, int *count = nullptr);   // not used outside: see below
static String mobSplitField(const String &s, int idx, int *count) {
  int n = 0, from = 0;
  String out;
  for (;;) {
    int c = s.indexOf(',', from);
    String f = c < 0 ? s.substring(from) : s.substring(from, c);
    if (n == idx) out = f;
    n++;
    if (c < 0) break;
    from = c + 1;
  }
  if (count) *count = n;
  return out;
}
static bool mobAllDigits(const String &s) {
  if (!s.length()) return false;
  for (size_t i = 0; i < s.length(); i++) if (s[i] < '0' || s[i] > '9') return false;
  return true;
}
static int mobOptInt(const String &s, int lo, int hi) {   // empty or out of range -> -1 (the value is just not used)
  if (!mobAllDigits(s) || s.length() > 6) return -1;
  long v = s.toInt();
  return (v >= lo && v <= hi) ? (int)v : -1;
}
static double mobOptCoord(const String &s, double lim) {
  if (!s.length()) return NAN;
  double v = s.toDouble();
  return (fabs(v) <= lim && (v != 0 || s[0] == '0')) ? v : NAN;
}

// Checks one $MOI line for this watch: checksum, version, own id, signature, field formats. The order of the sequence
// numbers is checked by the caller.
static bool mobParseMoi(const uint8_t *key, const String &id, String line, MoiMsg &m) {
  line.trim();
  int star = line.lastIndexOf('*');
  if (!line.startsWith("$MOI,") || star < 0 || line.length() != (unsigned)star + 3 || line.length() > MOB_MAX_LINE) return false;
  String inner = line.substring(1, star);
  if (!mobChecksum(inner).equalsIgnoreCase(line.substring(star + 1))) return false;
  int n = 0;
  String last = mobSplitField(inner, 0, &n);   // only the number of fields is wanted here
  (void)last;
  if (n < 11) return false;   // MOI + 10 fields; unknown extra fields before sig are tolerated (the signature covers them)
  if (mobSplitField(inner, 1) != "1" || mobSplitField(inner, 2) != id) return false;
  int lastComma = inner.lastIndexOf(',');
  if (!mobHmac8(key, inner.substring(0, lastComma)).equalsIgnoreCase(inner.substring(lastComma + 1))) return false;
  String sq = mobSplitField(inner, 3), sr = mobSplitField(inner, 6);
  if (sq.length() != 4 || !mobAllDigits(sq) || (sr != "0" && sr != "1")) return false;
  m.seq = sq.toInt();
  m.siren = sr.toInt();
  m.dist = mobOptInt(mobSplitField(inner, 4), 0, 99999);
  m.brg = mobOptInt(mobSplitField(inner, 5), 0, 359);
  m.age = mobOptInt(mobSplitField(inner, 7), 0, 999);
  m.blat = mobOptCoord(mobSplitField(inner, 8), 90);
  m.blon = mobOptCoord(mobSplitField(inner, 9), 180);
  return true;
}

static String mobBuildMok(const uint8_t *key, const String &id, uint16_t seq) {
  char b[32];
  snprintf(b, sizeof b, "MOK,%s,%04u", id.c_str(), (unsigned)seq);
  String body = b;
  String inner = body + "," + mobHmac8(key, body);
  return "$" + inner + "*" + mobChecksum(inner) + "\r\n";
}

static void mobHandleMoi(const uint8_t *key, const String &line, IPAddress rip, uint16_t rport) {
  MoiMsg m;
  if (!mobParseMoi(key, cfg.mobId, line, m)) { moi.nDrop++; return; }   // not for us, unsigned or damaged: silently dropped
  uint32_t now = millis();
  bool dup = false;
  if (moi.have && now - moi.rxMs < MOI_RESET_MS) {   // same rule as 9.2: accepted if 1-4999 ahead of the last one
    int d = ((int)m.seq - (int)moi.m.seq + 10000) % 10000;
    if (d == 0) dup = true;
    else if (d > 4999) { moi.nDrop++; return; }
  }
  mob.udp.beginPacket(rip, rport);   // $MOK for every accepted message (again for a duplicate: the first one may be lost)
  String mok = mobBuildMok(key, cfg.mobId, m.seq);
  mob.udp.write((const uint8_t *)mok.c_str(), mok.length());
  mob.udp.endPacket();
  if (dup) return;
  moi.m = m;
  moi.have = true;
  moi.rxMs = now;
  moi.nAcc++;
  if (m.siren) moi.sirenMs = now; else { moi.sirenMs = 0; moi.muteUntil = 0; }
  dirty = true;
}

bool mobMoiFresh() { return moi.have && millis() - moi.rxMs < MOI_STALE_MS; }
bool mobSirenMuted() { return moi.muteUntil && (int32_t)(moi.muteUntil - millis()) > 0; }
// the siren sounds while the latest accepted message asks for it (and is not older than 12 s) and it is not muted
bool mobSirenWanted() { return cfg.mobOn && moi.have && moi.m.siren == 1 && moi.sirenMs && millis() - moi.sirenMs < MOI_SIREN_MS; }
bool mobSirenOn() { return mobSirenWanted() && !mobSirenMuted(); }
void mobMute() { if (mobSirenWanted()) { moi.muteUntil = millis() + MOI_MUTE_MS; dirty = true; } }

// ---------- main loop (called at every loop) ----------
void mobLoop() {
  bool rdy = mobReady();
  mob.okNow = rdy ? 1 : 0;
  if (!cfg.mobOn) {   // switched off: no alarm state is kept (the MOB page is gone too)
    mob.st = MOB_IDLE; mob.pendSeq = 0xFFFF; mob.lastNameMs = 0;
    if (mob.udpOpen) { mob.udp.stop(); mob.udpOpen = false; }
    return;
  }
  if (!rdy) return;   // an alarm already raised stays raised and is sent as soon as Wi-Fi and configuration allow it
  uint32_t now = millis();

  // acknowledgements
  if (mob.udpOpen) {
    uint8_t key[32];
    int n;
    while ((n = mob.udp.parsePacket()) > 0) {
      char buf[160];
      IPAddress rip = mob.udp.remoteIP();   // where the tablet's line came from: $MOK goes back there
      uint16_t rport = mob.udp.remotePort();
      int r = mob.udp.read((uint8_t *)buf, min(n, (int)sizeof buf - 1));
      if (r <= 0) break;
      buf[r] = 0;
      if (!mobHexKey(cfg.mobKey, key)) break;
      if (strncmp(buf, "$MOI,", 5) == 0) { mobHandleMoi(key, String(buf), rip, rport); continue; }   // boat -> watch (5b)
      int s = mobCheckAck(key, cfg.mobId, String(buf));
      if (s < 0) continue;   // wrong signature, id or format: not an acknowledgement
      mob.nAck++;
      mob.lastAckMs = now;
      if (mob.pendSeq != 0xFFFF && s == mob.pendSeq) {
        mob.pendAcked = true;
        if (mob.st == MOB_ALARM) mob.ackedSinceRaise = true;
      }
      dirty = true;
    }
  }

  switch (mob.st) {
    case MOB_ALARM:
      if (mob.pendSeq == 0xFFFF) { mobSendName(); mob.pendSeq = mobSend('A'); mob.pendAcked = false; }
      else if (!mob.ackedSinceRaise) { if (now - mob.lastSend >= MOB_RETRY_MS) mobResend(); }   // same number until the tablet answers
      else if (now - mob.lastSend >= MOB_NORMAL_MS) { mob.pendSeq = mobSend('A'); mob.pendAcked = false; }   // then a new number every 5-6 s
      break;
    case MOB_CANCEL:
      if (mob.pendSeq == 0xFFFF) { mob.pendSeq = mobSend('C'); mob.pendAcked = false; }
      else if (!mob.pendAcked) { if (now - mob.lastSend >= MOB_RETRY_MS) mobResend(); }
      else { mob.st = MOB_IDLE; mob.pendSeq = 0xFFFF; mob.lastSend = now - MOB_NORMAL_MS; dirty = true; }   // acknowledged: back to "alive" messages
      break;
    default:
      if (!mob.lastNameMs || now - mob.lastNameMs >= MOB_NAME_MS) mobSendName();
      else if (now - mob.lastSend >= MOB_NORMAL_MS) mobSend('P');
      break;
  }
}
