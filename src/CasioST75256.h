// CasioST75256 v0.6.0 - driver for the 192x63 LCD of the fake Casio fx-880BTG (ST75256-compatible)
// Author: GrayKazu
//
// Screen facts (found by sniffing + replaying on a real unit):
//   * 192 visible columns, RAM columns start at offset 0x40
//   * 63 dot rows + 1 icon row (row 64 of the same RAM, page 15)
//   * 4-gray mode, 2 bits per pixel, 4 vertical pixels per byte
//   * SPI mode 0, A0 low = command, A0 high = data
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Print.h>
#include <stdarg.h>
#include "gfxfont.h"
#include "CasioUFont.h"

class CasioST75256 : public Print {
 public:
  static const int16_t WIDTH  = 192;  // physical size (rotation 0)
  static const int16_t HEIGHT = 63;   // dot rows; the 64th row is the icon row

  // cs, a0, rst = control pins.
  // sck/sda are only used on ESP32 (custom SPI pins). On other boards use the default SPI pins.
  CasioST75256(int8_t cs, int8_t a0, int8_t rst,
               int8_t sck = -1, int8_t sda = -1, SPIClass* spi = &SPI);

  // Reset, init, clear, display ON. Call after setMirror()/setColumnOffset() if you change them.
  bool begin(uint32_t spiHz = 1000000, bool clearScreen = true);

  // Send the framebuffer to the LCD. v0.3.0+: the buffer is diffed against what was last sent,
  // and only the changed column runs of each page are transmitted (pass true to force a full refresh).
  void display(bool all = false);
  void markAllDirty() { _forceAll = true; }
  const uint8_t* frameBuffer() const { return &fb[0][0]; }   // 16 pages x 192 columns

  // ---- panel settings ----
  void setSpeed(uint32_t hz)        { _hz = hz; }    // change SPI clock after begin()
  void setColumnOffset(uint8_t off) { _xOff = off; }   // default 0x40
  void setMirror(bool x, bool y)    { _mirrorX = x; _mirrorY = y; }  // default x=false, y=true
  void setA0Inverted(bool inv)      { _a0Inv = inv; }  // debug only
  void setBlockWrite(bool on)       { _block = on; }   // one SPI transaction per column run (faster, less tested)
  void setInvertColors(bool inv)    { _invertColors = inv; }  // software: swap black/white when drawing
  void setDisplayInverse(bool inv);                    // hardware inverse (A6/A7)
  void displayOn(bool on);
  void sleep(bool on);
  void setVop(uint16_t vop);                           // contrast, 0..511, default 123 (0x7B)

  // ---- rotation (software, drawing only; the icon row is not rotated) ----
  // 0 = normal 192x63, 1 = 90 deg clockwise (63x192), 2 = 180, 3 = 270.
  void setRotation(uint8_t r)       { _rot = r & 3; }
  uint8_t getRotation() const       { return _rot; }
  int16_t width() const             { return (_rot & 1) ? HEIGHT : WIDTH; }
  int16_t height() const            { return (_rot & 1) ? WIDTH : HEIGHT; }

  // ---- drawing (color 0 = white .. 3 = black) ----
  void clear(uint8_t color = 0);                       // clears the dot area, keeps the icon row
  void drawPixel(int16_t x, int16_t y, uint8_t color = 3);
  uint8_t getPixel(int16_t x, int16_t y);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color = 3);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = 3);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = 3);
  void drawCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color = 3);
  void fillCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color = 3);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color = 3);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color = 3);
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color = 3);
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color = 3);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color = 3);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color = 3);
  // 1-bit bitmap (PROGMEM ok), rows padded to whole bytes, MSB first. bg -1 = transparent.
  void drawBitmap(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, uint8_t color = 3, int16_t bg = -1);
  // 2-bit (4-gray) bitmap (PROGMEM ok), 4 pixels per byte, MSB first, rows padded to whole bytes.
  // transparent = gray value to skip (-1 = draw everything).
  // Read the dot area back (current rotation) into a packed 2bpp buffer: ((width()+3)/4) bytes per row, height() rows.
  // Returns the byte count (3024 for rotation 0, 3072 for 90/270). Same layout drawBitmap2bpp() reads.
  size_t readFrame2bpp(uint8_t* out);
  void drawBitmap2bpp(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, int16_t transparent = -1);

  // ---- v0.3.0 extras ----
  // Ordered-dither fill: level 0..16 = how many of the 16 cells in a 4x4 block use c1 (rest use c0).
  // With real grays this gives smooth gradients: e.g. fillRectDither(x,y,w,h,8,0,3).
  void fillRectDither(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t level, uint8_t c0 = 0, uint8_t c1 = 3);
  void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint8_t color = 3);
  void invertRect(int16_t x, int16_t y, int16_t w, int16_t h);   // c -> 3 - c inside the rectangle
  // Arc of a circle, angles in degrees: 0 = 3 o'clock, increasing clockwise (y points down).
  void drawArc(int16_t cx, int16_t cy, int16_t r, int16_t startDeg, int16_t endDeg, uint8_t color = 3);
  // Shift the dot area by (dx,dy) pixels in current-rotation coordinates; uncovered area gets `fill`.
  void scroll(int16_t dx, int16_t dy, uint8_t fill = 0);
  // ---- v0.4.0: UI building blocks ----
  // Line with a square brush of `thickness` pixels.
  void drawThickLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t thickness, uint8_t color = 3);
  // Filled ring segment between radii rIn..rOut and angles a0..a1 (same angle convention as drawArc).
  // Gauges: fillArc(cx, cy, 20, 26, 180, 360) draws a half ring; draw a lighter one underneath for the track.
  void fillArc(int16_t cx, int16_t cy, int16_t rIn, int16_t rOut, int16_t a0, int16_t a1, uint8_t color = 3);
  // Line graph of n samples scaled from vmin..vmax into the box (with a frame).
  void drawGraph(int16_t x, int16_t y, int16_t w, int16_t h, const int16_t* data, uint16_t n,
                 int16_t vmin, int16_t vmax, uint8_t color = 3);
  size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));   // print() with a format string
  // Smoothly ramp the contrast (Vop) to `target` over about `ms` milliseconds (blocking).
  void fadeTo(uint16_t target, uint16_t ms);
  uint16_t getVop() const { return _vop; }

  // ---- text (5x7 font in a 6x8 cell, works with print()/println()) ----
  void setCursor(int16_t x, int16_t y) { _cx = x; _cy = y; }
  void setTextSize(uint8_t s)          { _ts = s ? s : 1; }
  void setTextColor(uint8_t fg, int16_t bg = -1) { _fg = fg; _bg = bg; }  // bg -1 = transparent
  void setTextWrap(bool w)             { _wrap = w; }                      // default true
  // Custom font (GFXfont, see gfxfont.h). With a custom font the cursor y is the text BASELINE
  // (like Adafruit GFX); setFont(nullptr) goes back to the built-in 5x7 font (cursor y = top).
  void setFont(const GFXfont* f)       { _font = f; }
  // Unicode fonts (v0.6.0): UTF-8 text with Vietnamese, Thai, Chinese... Include a font header and add it:
  //   #include <CasioFont_VI.h>   ->  lcd.addUFont(&UFONT_VI);   lcd.print("Tiếng Việt");
  // Up to 4 fonts are searched in the order they were added (so you can mix e.g. VI + TH + ZH_CN).
  // Cursor y is the top of the cell. While a font is added it replaces the built-in 5x7 font.
  bool addUFont(const UFont* f);
  void clearUFonts()                   { _nU = 0; _u8n = 0; }
  // Line height in pixels at size 1 for the active font (8 for the built-in font).
  int16_t fontHeight() const           { return _font ? _font->yAdvance : (_nU ? _uf[0]->height : 8); }
  void drawChar(int16_t x, int16_t y, char c, uint8_t fg, int16_t bg, uint8_t size);
  // Bounding box of `s` if printed with the cursor at (x,y) (handles '\n', current font and size).
  void getTextBounds(const char* s, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
  int16_t textWidth(const char* s) const;                 // width in pixels at the current size
  void printCentered(const char* s, int16_t y);           // horizontally centered at row y
  void printRight(const char* s, int16_t xRight, int16_t y);  // right edge of the text at xRight
  size_t write(uint8_t c) override;
  using Print::write;

  // ---- icon row (FIX, SCI, arrows ...): x is the same 0..191 column space as drawing ----
  void iconClear();
  void iconFill();
  void iconSet(int16_t x0, int16_t x1, bool on = true);
  // Name your own icons once you know their columns (see the IconSweep example):
  //   lcd.defineIcon(0, 8, 15);   // id 0 = columns 8..15
  //   lcd.icon(0, true);
  static const uint8_t MAX_ICONS = 32;
  void defineIcon(uint8_t id, int16_t x0, int16_t x1);
  void icon(uint8_t id, bool on = true);

  // ---- HAUNTED FX (v0.5.0): effects that work on the framebuffer, call display() afterwards ----
  void fxSeed(uint32_t s)            { _rng = s ? s : 1; }          // seed the effect RNG (esp_random() etc.)
  // Fade every pixel `amount` gray levels toward white: call each frame instead of clear() for ghost trails.
  void fxDecay(uint8_t amount = 1);
  // TV static: density 0..255 = chance a pixel is replaced by a random gray (<= maxLevel).
  void fxNoise(uint8_t density, uint8_t maxLevel = 3);
  // CRT look: every `step`-th row is dimmed by `dim` gray levels.
  void fxScanlines(uint8_t step = 2, uint8_t dim = 1);
  // Corrupt random horizontal bands (shifted with wrap-around, above 128 some are inverted). 0..255.
  void fxGlitch(uint8_t intensity);
  // Every column slides down by a random 0..strength pixels. Call repeatedly: the picture melts away.
  void fxMelt(uint8_t strength = 2);
  // Contrast flicker like a dying lamp (blocking; needs a working setVop).
  void fxFlicker(uint8_t times = 5, uint16_t dim = 40);
  // Contrast pulse: lub-dub, lub-dub (blocking; needs a working setVop).
  void fxHeartbeat(uint8_t beats = 3, uint16_t amp = 60);
  // Old TV switching off: the picture collapses to a line, then to a dot, then nothing (blocking, clears the screen).
  void fxCrtOff(uint16_t ms = 400);
  // Like print(), but letters jitter and sometimes turn into garbage. intensity 0..255.
  void printCursed(const char* s, uint8_t intensity = 40);

  // ---- low level ----
  void writeCommand(uint8_t c) { tx(c, false); }
  void writeData(uint8_t d)    { tx(d, true); }

 private:
  void tx(uint8_t b, bool data);
  void sendRun(uint8_t page, int16_t c0, int16_t c1);
  void initSequence();
  void setRam(int16_t col, int16_t row, uint8_t c);
  uint8_t getRam(int16_t col, int16_t row);
  void put(int16_t x, int16_t y, uint8_t color);   // logical coords, no clipping
  const UGlyph* uFind(uint16_t cp, const UFont** fo) const;
  void putCodepoint(uint32_t cp);
  void drawUGlyph(int16_t x, int16_t y, const UFont* f, const UGlyph* g, uint8_t fg, int16_t bg, uint8_t size);
  int16_t uTextWidth(const char* s) const;
  void drawCircleQuad(int16_t cx, int16_t cy, int16_t r, uint8_t mask, uint8_t color);

  int8_t _cs, _a0, _rst, _sck, _sda;
  SPIClass* _spi;
  uint32_t _hz = 1000000;
  uint8_t _xOff = 0x40;
  uint8_t _rot = 0;
  uint16_t _vop = 123;
  const UFont* _uf[4];
  uint8_t _nU = 0, _u8n = 0;
  uint32_t _u8cp = 0;
  int16_t _lastX = 0;
  bool _lastMark = false;
  uint32_t _rng = 0x1BADB002;
  uint32_t rnd() { _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5; return _rng; }
  const GFXfont* _font = nullptr;
  bool _mirrorX = false, _mirrorY = true;
  bool _a0Inv = false, _block = false, _invertColors = false, _wrap = true, _forceAll = true;
  int16_t _cx = 0, _cy = 0, _bg = -1;
  uint8_t _ts = 1, _fg = 3;
  int16_t _iconX0[MAX_ICONS], _iconX1[MAX_ICONS];
  uint8_t fb[16][WIDTH];     // [page][ram column], 4 pixels per byte
  uint8_t sent[16][WIDTH];   // what the LCD RAM currently holds (for diffing)
};
