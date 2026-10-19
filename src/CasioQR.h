// CasioQR v0.1 - QR code generator for CasioST75256 (versions 1..10, numeric / alphanumeric / byte modes).   Author: GrayKazu
//   CasioQR qr;
//   if (qr.encode("https://example.com")) qr.drawCentered(lcd);          // picks the biggest scale that fits 192x63
//   lcd.display();
// Version 10 (57x57 modules, up to 271 bytes at ECC_L) is the largest that still fits the 63-pixel-high screen at 1 px per
// module. Short texts give bigger modules, which scan much more easily from a phone: version 1-2 (up to 32 bytes at ECC_L)
// is drawn at 2 px per module with a quiet zone. A Wi-Fi sign-in code: qr.wifi(ssid, password) then drawCentered().
#pragma once
#include <Arduino.h>
#include "CasioST75256.h"

class CasioQR {
 public:
  enum Ecc : uint8_t { ECC_L = 0, ECC_M = 1, ECC_Q = 2, ECC_H = 3 };
  static const uint8_t MAX_VERSION = 10;

  // Encode text (UTF-8 bytes). Picks numeric / alphanumeric / byte mode and the smallest version that fits.
  // boostEcc: raise the error correction level for free when the chosen version has room.
  // Returns false if the text does not fit version MAX_VERSION at the requested level.
  bool encode(const char* text, Ecc ecc = ECC_L, bool boostEcc = true, uint8_t minVersion = 1);
  // "WIFI:T:WPA;S:<ssid>;P:<password>;;" with the required escaping (type: "WPA", "WEP" or "nopass").
  bool wifi(const char* ssid, const char* password, const char* type = "WPA", Ecc ecc = ECC_M);

  uint8_t version() const { return _ver; }                         // 0 = nothing encoded
  uint8_t size() const    { return _ver ? _ver * 4 + 17 : 0; }     // modules per side
  Ecc     ecc() const     { return _ecc; }
  uint8_t mask() const    { return _mask; }
  bool    module(int8_t x, int8_t y) const { return x >= 0 && y >= 0 && x < size() && y < size() && getM(x, y); }
  // Largest integer module size for which size()*scale + 2*quiet*scale fits in availW x availH (0 = does not fit).
  uint8_t bestScale(int16_t availW, int16_t availH, uint8_t quiet = 2) const;
  // Draw at (x, y), the top-left of the quiet zone. quiet = white border in modules (4 is the spec, 2 works on screens).
  void draw(CasioST75256& lcd, int16_t x, int16_t y, uint8_t scale, uint8_t quiet = 2, uint8_t dark = 3, uint8_t light = 0);
  // Centered in the lcd (current rotation) with the best scale; returns the scale used (0 = too big for the screen).
  uint8_t drawCentered(CasioST75256& lcd, uint8_t quiet = 2, uint8_t dark = 3, uint8_t light = 0);
  // Capacity in bytes (byte mode) of a version at an ECC level.
  static uint16_t capacityBytes(uint8_t version, Ecc ecc);

 private:
  static const int MAXN = MAX_VERSION * 4 + 17;
  bool getM(int x, int y) const { int i = y * MAXN + x; return (_mod[i >> 3] >> (i & 7)) & 1; }
  void setM(int x, int y, bool v) { int i = y * MAXN + x; if (v) _mod[i >> 3] |= 1 << (i & 7); else _mod[i >> 3] &= ~(1 << (i & 7)); }
  bool isF(int x, int y) const { int i = y * MAXN + x; return (_fn[i >> 3] >> (i & 7)) & 1; }
  void setF(int x, int y, bool v) { setM(x, y, v); int i = y * MAXN + x; _fn[i >> 3] |= 1 << (i & 7); }
  void drawFunctionPatterns();
  void drawFormatBits(uint8_t mask);
  void drawFinder(int cx, int cy);
  void drawAlign(int cx, int cy);
  void drawCodewords(const uint8_t* data, int len);
  void applyMask(uint8_t mask);
  long penalty() const;
  static int rawModules(int ver);
  static int dataCodewords(int ver, Ecc e);
  int  addEccAndInterleave(const uint8_t* data, uint8_t* out) const;

  uint8_t _ver = 0, _mask = 0; Ecc _ecc = ECC_L;
  uint8_t _mod[(MAXN * MAXN + 7) / 8];
  uint8_t _fn[(MAXN * MAXN + 7) / 8];
  uint8_t _data[290];                                    // data codewords (v10-L: 274)
  uint8_t _cw[350];                                      // data + error correction, interleaved (v10: 346)
};
