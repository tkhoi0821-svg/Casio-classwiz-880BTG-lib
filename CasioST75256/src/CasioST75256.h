// CasioST75256 v0.2.0 - driver for the 192x63 LCD of the fake Casio fx-880BTG (ST75256-compatible)
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

class CasioST75256 : public Print {
 public:
  static const int16_t WIDTH  = 192;
  static const int16_t HEIGHT = 63;   // dot rows; the 64th row is the icon row

  // cs, a0, rst = control pins.
  // sck/sda are only used on ESP32 (custom SPI pins). On other boards use the default SPI pins.
  CasioST75256(int8_t cs, int8_t a0, int8_t rst,
               int8_t sck = -1, int8_t sda = -1, SPIClass* spi = &SPI);

  // Reset, init, clear, display ON. Call after setMirror()/setColumnOffset() if you change them.
  bool begin(uint32_t spiHz = 1000000, bool clearScreen = true);

  // Send the framebuffer to the LCD. Only pages changed since the last call are sent
  // (pass true to force a full refresh).
  void display(bool all = false);
  void markAllDirty() { _dirty = 0xFFFF; }
  const uint8_t* frameBuffer() const { return &fb[0][0]; }   // 16 pages x 192 columns

  // ---- panel settings ----
  void setColumnOffset(uint8_t off) { _xOff = off; }   // default 0x40
  void setMirror(bool x, bool y)    { _mirrorX = x; _mirrorY = y; }  // default x=false, y=true
  void setA0Inverted(bool inv)      { _a0Inv = inv; }  // debug only
  void setBlockWrite(bool on)       { _block = on; }   // keep CS low for a whole page (faster, less tested)
  void setInvertColors(bool inv)    { _invertColors = inv; }  // software: swap black/white when drawing
  void setDisplayInverse(bool inv);                    // hardware inverse (A6/A7)
  void displayOn(bool on);
  void sleep(bool on);
  void setVop(uint16_t vop);                           // contrast, 0..511, default 123 (0x7B)

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
  // 1-bit bitmap, rows padded to whole bytes, MSB first
  void drawBitmap(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, uint8_t color = 3);

  // ---- text (5x7 font in a 6x8 cell, works with print()/println()) ----
  void setCursor(int16_t x, int16_t y) { _cx = x; _cy = y; }
  void setTextSize(uint8_t s)          { _ts = s ? s : 1; }
  void setTextColor(uint8_t fg, int16_t bg = -1) { _fg = fg; _bg = bg; }  // bg -1 = transparent
  void drawChar(int16_t x, int16_t y, char c, uint8_t fg, int16_t bg, uint8_t size);
  int16_t textWidth(const char* s) const;                 // width in pixels at the current size
  void printCentered(const char* s, int16_t y);           // horizontally centered at row y
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

  // ---- low level ----
  void writeCommand(uint8_t c) { tx(c, false); }
  void writeData(uint8_t d)    { tx(d, true); }

 private:
  void tx(uint8_t b, bool data);
  void initSequence();
  void setRam(int16_t col, int16_t row, uint8_t c);
  uint8_t getRam(int16_t col, int16_t row);
  void drawCircleQuad(int16_t cx, int16_t cy, int16_t r, uint8_t mask, uint8_t color);

  int8_t _cs, _a0, _rst, _sck, _sda;
  SPIClass* _spi;
  uint32_t _hz = 1000000;
  uint8_t _xOff = 0x40;
  bool _mirrorX = false, _mirrorY = true;
  bool _a0Inv = false, _block = false, _invertColors = false;
  int16_t _cx = 0, _cy = 0, _bg = -1;
  uint8_t _ts = 1, _fg = 3;
  uint16_t _dirty = 0xFFFF;               // bit n = page n changed since last display()
  int16_t _iconX0[MAX_ICONS], _iconX1[MAX_ICONS];
  uint8_t fb[16][WIDTH];   // [page][ram column], 4 pixels per byte
};
