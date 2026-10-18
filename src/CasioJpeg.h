// CasioJpeg v0.1 - baseline JPEG -> 192x63 4-gray (2 bits per pixel) decoder for CasioST75256.   Author: GrayKazu
//
//  * Luma only: chroma blocks are entropy-decoded and skipped, so it is much faster than a full decoder.
//  * Decodes at 1/1, 1/2, 1/4 or 1/8 scale (reduced IDCT) when the source is much bigger than the LCD.
//  * Box-filter resize into 192x63 (stretch / letterbox / crop-fill), then 4-level quantisation with ordered
//    (stable, flicker-free for video) or Floyd-Steinberg dithering.
//  * Output = 63 rows x 48 bytes, 4 pixels per byte MSB first, 0 = white .. 3 = black: exactly what
//    lcd.drawBitmap2bpp(0, 0, buf, 192, 63) draws.
//  * Supports baseline / extended-sequential Huffman JPEG, 1 or 3 components, any common subsampling,
//    restart markers, custom Huffman tables. Not supported: progressive, arithmetic coding, 12-bit, CMYK.
// Uses about 50 KB of heap (one 192x63 accumulator) on the first decode.
#pragma once
#include <Arduino.h>

class CasioJpeg {
 public:
  static const int16_t OUT_W = 192, OUT_H = 63;
  static const size_t  FRAME_BYTES = OUT_W * OUT_H / 4;          // 3024
  enum Fit    : uint8_t { STRETCH, FIT, FILL };                  // ignore aspect / letterbox / crop to fill
  enum Dither : uint8_t { DITHER_NONE, DITHER_BAYER, DITHER_FLOYD };
  enum Result : int8_t { OK = 0, ERR_FORMAT = -1, ERR_TRUNCATED = -2, ERR_UNSUPPORTED = -3,
                         ERR_DATA = -4, ERR_SIZE = -5, ERR_MEMORY = -6 };

  CasioJpeg();
  ~CasioJpeg();

  // Decode one JPEG held in memory into `out` (FRAME_BYTES bytes). `out` is only written on success.
  Result decode(const uint8_t* jpg, size_t len, uint8_t* out);

  // ---- look ----
  void setFit(Fit f)               { _fit = f; }
  void setDither(Dither d)         { _dither = d; }
  void setBrightness(int16_t b)    { _bright = b; _lutDirty = true; }         // -255..255 added to luma
  void setContrast(float c)        { _contrast = c; _lutDirty = true; }       // 1.0 = unchanged
  void setGamma(float g)           { _gamma = g; _lutDirty = true; }          // >1 brightens midtones
  void setInvert(bool i)           { _invert = i; }                           // bright pixels -> black
  void setBackground(uint8_t lvl)  { _bg = lvl & 3; }                         // letterbox bars: 0 white .. 3 black

  // ---- info about the last decoded frame ----
  uint16_t srcWidth()  const { return _W; }
  uint16_t srcHeight() const { return _H; }
  uint8_t  scaleShift() const { return _shift; }                             // IDCT reduction used (0..3)
  const uint8_t* gray() const { return _gray; }                              // 192x63 luma after brightness/contrast/gamma

 private:
  struct Huff { uint8_t bits[17]; uint8_t vals[256]; uint16_t fast[512]; int32_t maxcode[18]; int32_t mincode[18]; int16_t valptr[17]; bool ok; };
  struct Comp { uint8_t id, h, v, tq, td, ta; int32_t pred; };
  struct Bits { const uint8_t* p; const uint8_t* end; uint32_t buf; int cnt; bool marker; int zeros; };

  Result parse(const uint8_t* d, size_t n);
  Result setupMaps();
  Result decodeScan();
  void   finish(uint8_t* out);
  void   buildHuff(Huff& h);
  void   buildLut();
  void   bitsFill(Bits& b);
  int    huffDecode(Bits& b, const Huff& h);
  void   placeBlock(const uint8_t* pix, int N, int X0, int Y0);
  bool   lumaBlock(Bits& b, Comp& c, uint8_t* pix);
  bool   skipBlock(Bits& b, Comp& c);

  Fit _fit = FIT; Dither _dither = DITHER_BAYER;
  int16_t _bright = 0; float _contrast = 1.0f, _gamma = 1.0f; bool _invert = false, _lutDirty = true; uint8_t _bg = 0;
  uint8_t _lut[256];

  Huff _huff[2][4];
  uint16_t _qt[4][64];
  Comp _c[3]; uint8_t _nf = 0, _hmax = 1, _vmax = 1;
  uint16_t _W = 0, _H = 0, _ri = 0;
  const uint8_t* _scan = nullptr; const uint8_t* _end = nullptr;
  uint8_t _shift = 0; int _rw = 0, _rh = 0;
  int16_t _xmap[1024], _ymap[1024];
  uint16_t _xcnt[OUT_W], _ycnt[OUT_H];
  int16_t _rx0 = 0, _ry0 = 0, _rw2 = OUT_W, _rh2 = OUT_H;     // destination rectangle
  uint32_t* _acc = nullptr;
  uint8_t _gray[OUT_W * OUT_H];
  float _ct[4][8][8];                                         // IDCT tables for N = 8, 4, 2, 1
};
