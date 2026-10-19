#include "CasioQR.h"
#include <string.h>

// Error correction codewords per block and number of blocks, [ecc L,M,Q,H][version-1], versions 1..10
static const uint8_t ECC_PER_BLOCK[4][10] = {{7, 10, 15, 20, 26, 18, 20, 24, 30, 18}, {10, 16, 26, 18, 24, 16, 18, 22, 22, 26},
                                             {13, 22, 18, 26, 18, 24, 18, 22, 20, 24}, {17, 28, 22, 16, 22, 28, 26, 26, 24, 28}};
static const uint8_t NUM_BLOCKS[4][10] = {{1, 1, 1, 1, 1, 2, 2, 2, 2, 4}, {1, 1, 1, 2, 2, 4, 4, 4, 5, 5},
                                          {1, 1, 2, 2, 4, 4, 6, 6, 8, 8}, {1, 1, 2, 4, 4, 4, 5, 6, 8, 8}};
static const uint8_t FORMAT_BITS[4] = {1, 0, 3, 2};                  // L, M, Q, H
static const char ALNUM[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

int CasioQR::rawModules(int ver) {
  int r = (16 * ver + 128) * ver + 64;
  if (ver >= 2) { int na = ver / 7 + 2; r -= (25 * na - 10) * na - 55; if (ver >= 7) r -= 36; }
  return r;
}
int CasioQR::dataCodewords(int ver, Ecc e) { return rawModules(ver) / 8 - ECC_PER_BLOCK[e][ver - 1] * NUM_BLOCKS[e][ver - 1]; }
uint16_t CasioQR::capacityBytes(uint8_t ver, Ecc e) {
  if (ver < 1 || ver > MAX_VERSION) return 0;
  int bits = dataCodewords(ver, e) * 8 - 4 - (ver < 10 ? 8 : 16);
  return bits > 0 ? bits / 8 : 0;
}

// ---------------- Reed-Solomon over GF(256), polynomial 0x11D ----------------
static uint8_t gmul(uint8_t x, uint8_t y) {
  uint8_t z = 0;
  for (int i = 7; i >= 0; i--) { z = (uint8_t)((z << 1) ^ ((z >> 7) * 0x11D)); z ^= ((y >> i) & 1) * x; }
  return z;
}
static void rsDivisor(int degree, uint8_t* out) {
  memset(out, 0, degree); out[degree - 1] = 1;
  uint8_t root = 1;
  for (int i = 0; i < degree; i++) {
    for (int j = 0; j < degree; j++) { out[j] = gmul(out[j], root); if (j + 1 < degree) out[j] ^= out[j + 1]; }
    root = gmul(root, 0x02);
  }
}
static void rsRemainder(const uint8_t* data, int len, const uint8_t* div, int degree, uint8_t* out) {
  memset(out, 0, degree);
  for (int k = 0; k < len; k++) {
    uint8_t factor = data[k] ^ out[0];
    memmove(out, out + 1, degree - 1); out[degree - 1] = 0;
    for (int i = 0; i < degree; i++) out[i] ^= gmul(div[i], factor);
  }
}

int CasioQR::addEccAndInterleave(const uint8_t* data, uint8_t* out) const {
  int nb = NUM_BLOCKS[_ecc][_ver - 1], eccLen = ECC_PER_BLOCK[_ecc][_ver - 1];
  int raw = rawModules(_ver) / 8, shortBlocks = nb - raw % nb, shortLen = raw / nb;
  uint8_t div[30], ecc[8][30]; int start[8], dlen[8], k = 0;
  rsDivisor(eccLen, div);
  for (int i = 0; i < nb; i++) {
    dlen[i] = shortLen - eccLen + (i < shortBlocks ? 0 : 1); start[i] = k;
    rsRemainder(data + k, dlen[i], div, eccLen, ecc[i]); k += dlen[i];
  }
  int idx = 0;
  for (int i = 0; i <= shortLen - eccLen; i++)
    for (int j = 0; j < nb; j++) if (i < dlen[j]) out[idx++] = data[start[j] + i];
  for (int i = 0; i < eccLen; i++)
    for (int j = 0; j < nb; j++) out[idx++] = ecc[j][i];
  return idx;
}

// ---------------- function patterns ----------------
void CasioQR::drawFinder(int cx, int cy) {
  int n = size();
  for (int dy = -4; dy <= 4; dy++)
    for (int dx = -4; dx <= 4; dx++) {
      int d = abs(dx) > abs(dy) ? abs(dx) : abs(dy), x = cx + dx, y = cy + dy;
      if (x >= 0 && x < n && y >= 0 && y < n) setF(x, y, d != 2 && d != 4);
    }
}
void CasioQR::drawAlign(int cx, int cy) {
  for (int dy = -2; dy <= 2; dy++)
    for (int dx = -2; dx <= 2; dx++) setF(cx + dx, cy + dy, (abs(dx) > abs(dy) ? abs(dx) : abs(dy)) != 1);
}
void CasioQR::drawFormatBits(uint8_t mask) {
  int n = size();
  int data = FORMAT_BITS[_ecc] << 3 | mask, rem = data;
  for (int i = 0; i < 10; i++) rem = (rem << 1) ^ ((rem >> 9) * 0x537);
  int bits = (data << 10 | rem) ^ 0x5412;
  for (int i = 0; i <= 5; i++) setF(8, i, (bits >> i) & 1);
  setF(8, 7, (bits >> 6) & 1); setF(8, 8, (bits >> 7) & 1); setF(7, 8, (bits >> 8) & 1);
  for (int i = 9; i < 15; i++) setF(14 - i, 8, (bits >> i) & 1);
  for (int i = 0; i < 8; i++) setF(n - 1 - i, 8, (bits >> i) & 1);
  for (int i = 8; i < 15; i++) setF(8, n - 15 + i, (bits >> i) & 1);
  setF(8, n - 8, true);                                  // the always-dark module
}
void CasioQR::drawFunctionPatterns() {
  int n = size();
  memset(_mod, 0, sizeof(_mod)); memset(_fn, 0, sizeof(_fn));
  for (int i = 0; i < n; i++) { setF(6, i, i % 2 == 0); setF(i, 6, i % 2 == 0); }
  drawFinder(3, 3); drawFinder(n - 4, 3); drawFinder(3, n - 4);
  if (_ver >= 2) {
    int na = _ver / 7 + 2, step = (_ver * 4 + na * 2 + 1) / (na * 2 - 2) * 2, pos[8];
    pos[0] = 6;
    for (int i = na - 1, p = n - 7; i >= 1; i--, p -= step) pos[i] = p;
    for (int i = 0; i < na; i++)
      for (int j = 0; j < na; j++) if (!((i == 0 && j == 0) || (i == 0 && j == na - 1) || (i == na - 1 && j == 0))) drawAlign(pos[i], pos[j]);
  }
  drawFormatBits(0);
  if (_ver >= 7) {
    int rem = _ver;
    for (int i = 0; i < 12; i++) rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    long bits = (long)_ver << 12 | rem;
    for (int i = 0; i < 18; i++) { bool c = (bits >> i) & 1; int a = n - 11 + i % 3, b = i / 3; setF(a, b, c); setF(b, a, c); }
  }
}
void CasioQR::drawCodewords(const uint8_t* data, int len) {
  int n = size(), i = 0;
  for (int right = n - 1; right >= 1; right -= 2) {
    if (right == 6) right = 5;
    for (int vert = 0; vert < n; vert++)
      for (int j = 0; j < 2; j++) {
        int x = right - j; bool up = ((right + 1) & 2) == 0; int y = up ? n - 1 - vert : vert;
        if (!isF(x, y) && i < len * 8) { setM(x, y, (data[i >> 3] >> (7 - (i & 7))) & 1); i++; }
      }
  }
}
void CasioQR::applyMask(uint8_t m) {
  int n = size();
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) {
      bool inv;
      switch (m) {
        case 0: inv = (x + y) % 2 == 0; break;
        case 1: inv = y % 2 == 0; break;
        case 2: inv = x % 3 == 0; break;
        case 3: inv = (x + y) % 3 == 0; break;
        case 4: inv = (x / 3 + y / 2) % 2 == 0; break;
        case 5: inv = x * y % 2 + x * y % 3 == 0; break;
        case 6: inv = (x * y % 2 + x * y % 3) % 2 == 0; break;
        default: inv = ((x + y) % 2 + x * y % 3) % 2 == 0; break;
      }
      if (inv && !isF(x, y)) setM(x, y, !getM(x, y));
    }
}
long CasioQR::penalty() const {
  int n = size(); long p = 0;
  for (int pass = 0; pass < 2; pass++)                    // rows then columns: runs of 5+ (N1) and finder-like patterns (N3)
    for (int a = 0; a < n; a++) {
      int run = 1, win = 0;
      bool prev = pass ? getM(a, 0) : getM(0, a);
      for (int b = 0; b < n; b++) {
        bool c = pass ? getM(a, b) : getM(b, a);
        if (b > 0) { if (c == prev) { run++; if (run == 5) p += 3; else if (run > 5) p++; } else run = 1; }
        prev = c;
        win = ((win << 1) | c) & 0x7FF;
        if (b >= 10 && (win == 0x5D0 || win == 0x05D)) p += 40;
      }
    }
  for (int y = 0; y + 1 < n; y++)
    for (int x = 0; x + 1 < n; x++) { bool c = getM(x, y); if (c == getM(x + 1, y) && c == getM(x, y + 1) && c == getM(x + 1, y + 1)) p += 3; }
  long dark = 0; for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) dark += getM(x, y);
  long total = (long)n * n, k = (labs(dark * 20 - total * 10) + total - 1) / total - 1;
  return p + (k > 0 ? k : 0) * 10;
}

// ---------------- encoding ----------------
bool CasioQR::encode(const char* text, Ecc ecc, bool boost, uint8_t minVer) {
  _ver = 0;
  size_t len = strlen(text);
  bool numeric = len > 0, alnum = len > 0;
  for (size_t i = 0; i < len; i++) {
    char c = text[i];
    if (c < '0' || c > '9') numeric = false;
    if (!strchr(ALNUM, c) || c == 0) alnum = false;
  }
  int mode = numeric ? 1 : alnum ? 2 : 4;
  long dataBits = numeric ? (long)len / 3 * 10 + (len % 3 == 0 ? 0 : len % 3 == 1 ? 4 : 7)
                : alnum ? (long)len / 2 * 11 + (len % 2) * 6 : (long)len * 8;
  int ver = minVer < 1 ? 1 : minVer; long need = 0;
  for (;; ver++) {
    if (ver > MAX_VERSION) return false;
    int cc = mode == 1 ? (ver < 10 ? 10 : 12) : mode == 2 ? (ver < 10 ? 9 : 11) : (ver < 10 ? 8 : 16);
    need = 4 + cc + dataBits;
    if ((mode == 4 && ver < 10 && len > 255) || need > (long)dataCodewords(ver, ecc) * 8) continue;
    if (len >= (1u << cc)) continue;
    break;
  }
  if (boost) for (int e = ECC_M; e <= ECC_H; e++) if (e > ecc && need <= (long)dataCodewords(ver, (Ecc)e) * 8) ecc = (Ecc)e;
  _ver = ver; _ecc = ecc;
  int cap = dataCodewords(ver, ecc);
  memset(_data, 0, sizeof(_data));
  long nb = 0;                                           // bits written
  auto put = [&](uint32_t v, int n) { for (int i = n - 1; i >= 0; i--, nb++) if ((v >> i) & 1) _data[nb >> 3] |= 0x80 >> (nb & 7); };
  int cc = mode == 1 ? (ver < 10 ? 10 : 12) : mode == 2 ? (ver < 10 ? 9 : 11) : (ver < 10 ? 8 : 16);
  put(mode, 4); put(len, cc);
  if (mode == 1) {
    for (size_t i = 0; i < len;) { int n = len - i >= 3 ? 3 : (int)(len - i); uint32_t v = 0; for (int k = 0; k < n; k++) v = v * 10 + (text[i + k] - '0'); put(v, n == 3 ? 10 : n == 2 ? 7 : 4); i += n; }
  } else if (mode == 2) {
    for (size_t i = 0; i < len; i += 2) {
      uint32_t a = strchr(ALNUM, text[i]) - ALNUM;
      if (i + 1 < len) { uint32_t b = strchr(ALNUM, text[i + 1]) - ALNUM; put(a * 45 + b, 11); } else put(a, 6);
    }
  } else for (size_t i = 0; i < len; i++) put((uint8_t)text[i], 8);
  long capBits = (long)cap * 8;
  long term = capBits - nb < 4 ? capBits - nb : 4; nb += term;          // terminator
  nb = (nb + 7) / 8 * 8;
  for (uint8_t pad = 0xEC; nb < capBits; pad ^= 0xEC ^ 0x11) { _data[nb >> 3] = pad; nb += 8; }
  addEccAndInterleave(_data, _cw);
  drawFunctionPatterns();
  drawCodewords(_cw, rawModules(ver) / 8);
  long best = 0x7FFFFFFF; uint8_t bm = 0;
  for (uint8_t m = 0; m < 8; m++) {
    applyMask(m); drawFormatBits(m);
    long p = penalty();
    if (p < best) { best = p; bm = m; }
    applyMask(m);                                                         // XOR again = undo
  }
  _mask = bm; applyMask(bm); drawFormatBits(bm);
  return true;
}

bool CasioQR::wifi(const char* ssid, const char* pass, const char* type, Ecc ecc) {
  char buf[200]; size_t o = 0;
  auto add = [&](const char* s, bool esc) { for (; *s && o < sizeof(buf) - 4; s++) { if (esc && strchr("\\;,:\"", *s)) buf[o++] = '\\'; buf[o++] = *s; } };
  add("WIFI:T:", false); add(type, false); add(";S:", false); add(ssid, true);
  if (strcmp(type, "nopass") != 0) { add(";P:", false); add(pass, true); }
  add(";;", false); buf[o] = 0;
  return encode(buf, ecc);
}

// ---------------- drawing ----------------
uint8_t CasioQR::bestScale(int16_t w, int16_t h, uint8_t quiet) const {
  if (!_ver) return 0;
  int total = size() + 2 * quiet, sx = w / total, sy = h / total, s = sx < sy ? sx : sy;
  return s > 0 ? (uint8_t)s : 0;
}
void CasioQR::draw(CasioST75256& lcd, int16_t x, int16_t y, uint8_t scale, uint8_t quiet, uint8_t dark, uint8_t light) {
  if (!_ver || !scale) return;
  int n = size(), total = n + 2 * quiet;
  lcd.fillRect(x, y, total * scale, total * scale, light);
  for (int my = 0; my < n; my++)
    for (int mx = 0; mx < n; mx++)
      if (getM(mx, my)) lcd.fillRect(x + (quiet + mx) * scale, y + (quiet + my) * scale, scale, scale, dark);
}
uint8_t CasioQR::drawCentered(CasioST75256& lcd, uint8_t quiet, uint8_t dark, uint8_t light) {
  uint8_t s = bestScale(lcd.width(), lcd.height(), quiet);
  if (!s) s = bestScale(lcd.width(), lcd.height(), 0), quiet = 0;       // no room for a border: use the whole screen
  if (!s) return 0;
  int total = (size() + 2 * quiet) * s;
  draw(lcd, (lcd.width() - total) / 2, (lcd.height() - total) / 2, s, quiet, dark, light);
  return s;
}
