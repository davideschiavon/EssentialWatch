// EssentialWatch - pairing with the EssentialSail tablet by a 6-digit code (MOB / Watch Protocol, section 8).
//   $MOP,1,<wpub>,<mac>*<cs>          watch  -> tablet   "pair me"   (UDP broadcast to port 10120)
//   $MOQ,1,<id>,<tpub>,<mac>*<cs>     tablet -> watch    "you are <id>"
// The 6 digits are only a password that authenticates the two P-256 public keys (mac = first 8 bytes of HMAC-SHA256
// keyed with the six ASCII digits). The real 256-bit key comes from ECDH + HKDF-SHA256, so it never travels on the air.
// The tablet's address is learned from the source of its reply. Included after mob.h.
#pragma once
#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"
#include "esp_random.h"

enum PairSt : uint8_t { PAIR_IDLE, PAIR_BUSY, PAIR_OK, PAIR_FAIL };

#define PAIR_WAIT_MS   5000UL    // the watch waits this long for the $MOQ (the same $MOP is repeated every 1.5 s)
#define PAIR_MAX_LINE  300

struct PairCtl {
  PairSt st = PAIR_IDLE;
  String code, wpub, err;
  uint32_t startMs = 0, lastSend = 0;
  mbedtls_ecp_group grp;
  mbedtls_mpi d;            // the ephemeral private key: lives only while the pairing runs
  bool keys = false;
  WiFiUDP udp;
  bool udpOpen = false;
} pr;

static int pairRng(void *, unsigned char *buf, size_t len) { esp_fill_random(buf, len); return 0; }

static void pairFree() {
  if (pr.keys) { mbedtls_mpi_free(&pr.d); mbedtls_ecp_group_free(&pr.grp); pr.keys = false; }
  if (pr.udpOpen) { pr.udp.stop(); pr.udpOpen = false; }
  pr.code = ""; pr.wpub = "";
}

static String pairHex(const uint8_t *b, size_t n) {
  String s;
  char h[3];
  for (size_t i = 0; i < n; i++) { snprintf(h, sizeof h, "%02x", b[i]); s += h; }
  return s;
}
static bool pairUnhex(const String &s, uint8_t *out, size_t n) {
  if (s.length() != n * 2) return false;
  for (size_t i = 0; i < n; i++) {
    int a = mobHexVal(s[i * 2]), b = mobHexVal(s[i * 2 + 1]);
    if (a < 0 || b < 0) return false;
    out[i] = (uint8_t)(a * 16 + b);
  }
  return true;
}

// Loads the P-256 group and creates the key pair: random, or from a fixed scalar (hex) for the reference example.
static bool pairKeygen(const char *fixedHex, String &pubHex) {
  mbedtls_ecp_group_init(&pr.grp);
  mbedtls_mpi_init(&pr.d);
  pr.keys = true;
  mbedtls_ecp_point Q;
  mbedtls_ecp_point_init(&Q);
  bool ok = mbedtls_ecp_group_load(&pr.grp, MBEDTLS_ECP_DP_SECP256R1) == 0;
  if (ok) {
    if (fixedHex) {   // fixed scalar (reference example only): public key = d * G, G = the standard P-256 generator
      static const char *GX = "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";
      static const char *GY = "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";
      uint8_t gb[65];
      gb[0] = 0x04;
      pairUnhex(GX, gb + 1, 32);
      pairUnhex(GY, gb + 33, 32);
      mbedtls_ecp_point G;
      mbedtls_ecp_point_init(&G);
      ok = mbedtls_mpi_read_string(&pr.d, 16, fixedHex) == 0 && mbedtls_ecp_point_read_binary(&pr.grp, &G, gb, 65) == 0 &&
           mbedtls_ecp_mul(&pr.grp, &Q, &pr.d, &G, pairRng, nullptr) == 0;
      mbedtls_ecp_point_free(&G);
    }
    else ok = mbedtls_ecdh_gen_public(&pr.grp, &pr.d, &Q, pairRng, nullptr) == 0;
  }
  if (ok) {
    uint8_t buf[65];
    size_t olen = 0;
    ok = mbedtls_ecp_point_write_binary(&pr.grp, &Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, buf, sizeof buf) == 0 && olen == 65;
    if (ok) pubHex = pairHex(buf, 65);
  }
  mbedtls_ecp_point_free(&Q);
  return ok;
}

// first 8 bytes of HMAC-SHA256, keyed with the six digits of the code, as 16 lowercase hex characters
static String pairMac(const String &code, const String &text) {
  uint8_t out[32];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (const uint8_t *)code.c_str(), code.length(), (const uint8_t *)text.c_str(), text.length(), out);
  return pairHex(out, 8);
}

static String pairLine(const String &inner) { return "$" + inner + "*" + mobChecksum(inner); }   // without CR LF

// "$MOP,1,<wpub>,<mac>*cs"
static String pairRequest(const String &code, const String &wpub) {
  String body = "MOP,1," + wpub;
  return pairLine(body + "," + pairMac(code, body));
}

// Checks the tablet's reply with the typed code and our key pair; on success returns the device id and the 32-byte key.
static bool pairComplete(const String &raw, const String &code, const String &wpub, String &id, uint8_t key[32]) {
  String line = raw;
  line.trim();
  int star = line.lastIndexOf('*');
  if (line.length() > PAIR_MAX_LINE || !line.startsWith("$MOQ,") || star < 0 || line.length() != (unsigned)star + 3) return false;
  String inner = line.substring(1, star);
  if (!mobChecksum(inner).equalsIgnoreCase(line.substring(star + 1))) return false;
  int n = 0;
  mobSplitField(inner, 0, &n);
  if (n != 5 || mobSplitField(inner, 1) != "1") return false;
  id = mobSplitField(inner, 2);
  String tpub = mobSplitField(inner, 3), mac = mobSplitField(inner, 4);
  if (!mobIdOk(id) || !mac.equalsIgnoreCase(pairMac(code, "MOQ,1," + id + "," + tpub + "," + wpub))) return false;   // wrong code or not the tablet
  uint8_t pb[65];
  if (!pr.keys || !pairUnhex(tpub, pb, 65) || pb[0] != 0x04) return false;
  mbedtls_ecp_point Qp;
  mbedtls_ecp_point_init(&Qp);
  mbedtls_mpi z;
  mbedtls_mpi_init(&z);
  uint8_t shared[32];
  bool ok = mbedtls_ecp_point_read_binary(&pr.grp, &Qp, pb, 65) == 0 && mbedtls_ecp_check_pubkey(&pr.grp, &Qp) == 0 &&
            mbedtls_ecdh_compute_shared(&pr.grp, &z, &Qp, &pr.d, pairRng, nullptr) == 0 && mbedtls_mpi_write_binary(&z, shared, 32) == 0;
  mbedtls_ecp_point_free(&Qp);
  mbedtls_mpi_free(&z);
  if (!ok) return false;
  // HKDF-SHA256, one block: salt "essentialsail-watch-1", info = id; key = HMAC(PRK, id || 0x01)
  const char *salt = "essentialsail-watch-1";
  uint8_t prk[32];
  const mbedtls_md_info_t *sha = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_hmac(sha, (const uint8_t *)salt, strlen(salt), shared, 32, prk);
  String info = id + String((char)1);
  mbedtls_md_hmac(sha, prk, 32, (const uint8_t *)info.c_str(), info.length(), key);
  return true;
}

// ---------- actions ----------
// returns "" or an error text
String pairStart(const String &codeIn) {
  String code;
  for (size_t i = 0; i < codeIn.length(); i++) if (codeIn[i] != ' ') code += codeIn[i];   // "482 913" as the tablet shows it
  if (code.length() != 6 || !mobAllDigits(code)) return "pairing code: the 6 digits shown on the tablet";
  if (WiFi.status() != WL_CONNECTED) return "the watch is not connected to a WiFi network";
  if (pr.st == PAIR_BUSY) return "";
  pairFree();
  if (!pairKeygen(nullptr, pr.wpub)) { pairFree(); return "pairing: cannot create the key"; }
  pr.code = code;
  pr.err = "";
  pr.startMs = millis();
  pr.lastSend = 0;
  pr.udpOpen = pr.udp.begin(0);
  pr.st = PAIR_BUSY;
  dirty = true;
  return "";
}

// One target per attempt: the tablet counts every $MOP with a wrong code as a failure (5 close the window), so a wrong
// code must not burn them all in one second. A typed address is used directly; otherwise the attempts alternate between
// the limited broadcast (255.255.255.255) and the directed broadcast of the WiFi network.
static void pairSendRequest() {
  static uint8_t attempt = 0;
  String ln = pairRequest(pr.code, pr.wpub) + "\r\n";
  IPAddress ip = WiFi.localIP(), mask = WiFi.subnetMask();
  IPAddress target;
  if (!(cfg.mobHost.length() && target.fromString(cfg.mobHost))) target = (attempt++ & 1) ? IPAddress((uint32_t)ip | ~(uint32_t)mask) : IPAddress(255, 255, 255, 255);
  if (pr.udp.beginPacket(target, cfg.mobPort ? cfg.mobPort : 10120)) {
    pr.udp.write((const uint8_t *)ln.c_str(), ln.length());
    pr.udp.endPacket();
  }
  pr.lastSend = millis();
}

void pairLoop() {
  if (pr.st != PAIR_BUSY) return;
  uint32_t now = millis();
  if (now - pr.startMs > PAIR_WAIT_MS) { pr.st = PAIR_FAIL; pr.err = "no valid answer"; pairFree(); dirty = true; return; }
  if (!pr.lastSend || now - pr.lastSend >= 1500) pairSendRequest();   // 4 attempts in 5 s (the same $MOP)
  int n;
  while ((n = pr.udp.parsePacket()) > 0) {
    char buf[PAIR_MAX_LINE + 8];
    IPAddress from = pr.udp.remoteIP();
    uint16_t fport = pr.udp.remotePort();
    int r = pr.udp.read((uint8_t *)buf, min(n, (int)sizeof buf - 1));
    if (r <= 0) break;
    buf[r] = 0;
    String id;
    uint8_t key[32];
    if (!pairComplete(String(buf), pr.code, pr.wpub, id, key)) continue;   // not the tablet, or a wrong code: keep waiting
    cfg.mobId = id;
    cfg.mobKey = pairHex(key, 32);
    cfg.mobHost = from.toString();   // the tablet's address is where its reply came from
    cfg.mobPort = fport;
    cfg.mobOn = true;
    saveCfg();
    mob.lastNameMs = 0;   // the owner name goes to the tablet at once
    pairFree();
    pr.st = PAIR_OK;
    dirty = true;
    return;
  }
}

const char *pairStateStr() { return pr.st == PAIR_BUSY ? "busy" : (pr.st == PAIR_OK ? "ok" : (pr.st == PAIR_FAIL ? "fail" : "idle")); }
