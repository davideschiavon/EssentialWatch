// EssentialWatch - optional GPS of the watch: a GPS module (NMEA 0183, 3.3 V) wired to a free pad of the board.
// The board itself has no GPS. Set the RX pad (GPIO) and the baud rate on the web page; -1 = no GPS (default).
// The position is the position of the WEARER: it goes into the MOB messages (mob.h) and is shown on the web page.
// Included by the main sketch after the settings.
#pragma once
#include <math.h>

struct GpsState {
  bool on = false;          // UART open
  bool fix = false;         // a valid position was received
  double lat = NAN, lon = NAN;
  uint32_t fixMs = 0;       // millis() of the last valid position
  uint32_t lines = 0, bad = 0;   // sentences received / with a bad checksum
  int sats = -1;
} gps;

static String gpsBuf;

// "ddmm.mmmm" / "dddmm.mmmm" + hemisphere -> decimal degrees (NAN if malformed)
static double gpsCoord(const String &v, const String &hem, int degDigits) {
  if (v.length() < (unsigned)degDigits + 3 || (hem != "N" && hem != "S" && hem != "E" && hem != "W")) return NAN;
  for (size_t i = 0; i < v.length(); i++) if (!isdigit((unsigned char)v[i]) && v[i] != '.') return NAN;
  double d = v.substring(0, degDigits).toDouble(), m = v.substring(degDigits).toDouble();
  if (m >= 60) return NAN;
  double r = d + m / 60.0;
  return (hem == "S" || hem == "W") ? -r : r;
}

static String gpsField(const String &s, int n) {   // n-th comma separated field
  int from = 0;
  for (int i = 0; i < n; i++) { from = s.indexOf(',', from); if (from < 0) return ""; from++; }
  int to = s.indexOf(',', from);
  return to < 0 ? s.substring(from) : s.substring(from, to);
}

// One NMEA sentence ("$GPRMC,...*hh"); returns true if it carried a valid position.
bool gpsFeed(String line) {
  line.trim();
  int star = line.lastIndexOf('*');
  if (line.length() < 12 || line[0] != '$' || star < 0 || line.length() != (unsigned)star + 3) { gps.bad++; return false; }
  uint8_t c = 0;
  for (int i = 1; i < star; i++) c ^= (uint8_t)line[i];
  char b[3];
  snprintf(b, sizeof b, "%02X", c);
  if (!line.substring(star + 1).equalsIgnoreCase(b)) { gps.bad++; return false; }
  gps.lines++;
  String body = line.substring(1, star);
  String type = body.substring(2, 5);   // talker (GP, GN, GL...) + RMC / GGA
  double la = NAN, lo = NAN;
  if (type == "RMC") {   // $xxRMC,time,A,lat,N,lon,E,...   (A = valid)
    if (gpsField(body, 2) != "A") return false;
    la = gpsCoord(gpsField(body, 3), gpsField(body, 4), 2);
    lo = gpsCoord(gpsField(body, 5), gpsField(body, 6), 3);
  } else if (type == "GGA") {   // $xxGGA,time,lat,N,lon,E,quality,sats,...   (quality > 0 = fix)
    if (gpsField(body, 6).toInt() <= 0) return false;
    la = gpsCoord(gpsField(body, 2), gpsField(body, 3), 2);
    lo = gpsCoord(gpsField(body, 4), gpsField(body, 5), 3);
    String sv = gpsField(body, 7);
    if (sv.length()) gps.sats = sv.toInt();
  } else return false;
  if (isnan(la) || isnan(lo) || fabs(la) > 90 || fabs(lo) > 180) return false;
  gps.lat = la; gps.lon = lo; gps.fix = true; gps.fixMs = millis();
  return true;
}

void gpsBegin() {   // (re)opens the UART according to the settings
  if (gps.on) { Serial2.end(); gps.on = false; }
  gps.fix = false; gps.sats = -1; gpsBuf = "";
  if (cfg.gpsRx >= 0) { Serial2.begin(cfg.gpsBaud, SERIAL_8N1, cfg.gpsRx, -1); gps.on = true; }
}

void gpsLoop() {
  if (!gps.on) return;
  int n = 0;
  while (Serial2.available() && n++ < 256) {
    char ch = (char)Serial2.read();
    if (ch == '\n') { if (gpsBuf.length()) gpsFeed(gpsBuf); gpsBuf = ""; }
    else if (ch != '\r' && gpsBuf.length() < 120) gpsBuf += ch;
    else if (gpsBuf.length() >= 120) gpsBuf = "";
  }
}

// age of the position in seconds, -1 = no position (older than 999 s counts as none)
int gpsAge() {
  if (!gps.fix) return -1;
  uint32_t a = (millis() - gps.fixMs) / 1000;
  return a > 999 ? -1 : (int)a;
}
