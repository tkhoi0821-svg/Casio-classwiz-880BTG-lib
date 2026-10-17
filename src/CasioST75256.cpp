#include "CasioST75256.h"
#include <math.h>

// Classic 5x7 font, ASCII 32..126, 5 bytes per glyph (column-major, bit0 = top row)
static const uint8_t FONT5X7[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
  0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x56,0x20,0x50, 0x00,0x08,0x07,0x03,0x00,
  0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x2A,0x1C,0x7F,0x1C,0x2A, 0x08,0x08,0x3E,0x08,0x08,
  0x00,0x80,0x70,0x30,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x00,0x60,0x60,0x00, 0x20,0x10,0x08,0x04,0x02,
  0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46, 0x21,0x41,0x49,0x4D,0x33,
  0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x31, 0x41,0x21,0x11,0x09,0x07,
  0x36,0x49,0x49,0x49,0x36, 0x46,0x49,0x49,0x29,0x1E, 0x00,0x00,0x14,0x00,0x00, 0x00,0x40,0x34,0x00,0x00,
  0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x59,0x09,0x06,
  0x3E,0x41,0x5D,0x59,0x4E, 0x7C,0x12,0x11,0x12,0x7C, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
  0x7F,0x41,0x41,0x41,0x3E, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x41,0x51,0x73,
  0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
  0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x1C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
  0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x26,0x49,0x49,0x49,0x32,
  0x03,0x01,0x7F,0x01,0x03, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
  0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03, 0x61,0x59,0x49,0x4D,0x43, 0x00,0x7F,0x41,0x41,0x00,
  0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x7F,0x00, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
  0x00,0x03,0x07,0x08,0x00, 0x20,0x54,0x54,0x78,0x40, 0x7F,0x28,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x28,
  0x38,0x44,0x44,0x28,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x00,0x08,0x7E,0x09,0x02, 0x18,0xA4,0xA4,0x9C,0x78,
  0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x40,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
  0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x78,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
  0xFC,0x18,0x24,0x24,0x18, 0x18,0x24,0x24,0x18,0xFC, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x24,
  0x04,0x04,0x3F,0x44,0x24, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
  0x44,0x28,0x10,0x28,0x44, 0x4C,0x90,0x90,0x90,0x7C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
  0x00,0x00,0x77,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x02,0x01,0x02,0x04,0x02
};

static inline void swap16(int16_t& a, int16_t& b) { int16_t t = a; a = b; b = t; }

CasioST75256::CasioST75256(int8_t cs, int8_t a0, int8_t rst, int8_t sck, int8_t sda, SPIClass* spi)
    : _cs(cs), _a0(a0), _rst(rst), _sck(sck), _sda(sda), _spi(spi) {
  memset(fb, 0, sizeof(fb));
  memset(sent, 0, sizeof(sent));
  for (uint8_t i = 0; i < MAX_ICONS; i++) { _iconX0[i] = -1; _iconX1[i] = -1; }
}

// ---------------- low level ----------------
void CasioST75256::tx(uint8_t b, bool data) {
  digitalWrite(_a0, (data ^ _a0Inv) ? HIGH : LOW);   // A0 high = data, low = command
  digitalWrite(_cs, LOW);
  _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
  _spi->transfer(b);
  _spi->endTransaction();
  digitalWrite(_cs, HIGH);
}

void CasioST75256::initSequence() {
  // Hardware reset
  digitalWrite(_rst, HIGH); delay(10);
  digitalWrite(_rst, LOW);  delay(20);
  digitalWrite(_rst, HIGH); delay(100);

  // Frame #0 of the captured init
  writeCommand(0x30);
  writeCommand(0x6E);                       // kept as captured, meaning not verified
  writeCommand(0x31);                       // extension 2
  writeCommand(0xD7); writeData(0x9F);      // auto read control
  writeCommand(0xE0); writeData(0x00); delay(20);   // OTP WR/RD control
  writeCommand(0xE3); delay(30);            // OTP read
  writeCommand(0xE1);                       // OTP control out
  writeCommand(0x30);
  writeCommand(0x94); delay(20);            // sleep out
  writeCommand(0xAE);                       // display off

  // Frame #1 of the captured init
  writeCommand(0x20); writeData(0x0B); delay(20);               // power control
  writeCommand(0x81); writeData(0x3B); writeData(0x01);         // Vop = 0x7B
  writeCommand(0x31);
  writeCommand(0x20);                                           // gray levels
  static const uint8_t gray[16] = {0x00,0x00,0x00,0x0D,0x0D,0x0D,0x00,0x00,
                                   0x16,0x00,0x00,0x16,0x16,0x16,0x00,0x00};
  for (uint8_t i = 0; i < 16; i++) writeData(gray[i]);
  writeCommand(0xF0); writeData(0x08); writeData(0x08); writeData(0x0B); writeData(0x0D);  // frame rate
  writeCommand(0x32); writeData(0x00); writeData(0x01); writeData(0x05);                   // analog
  writeCommand(0x51); writeData(0xFA);                                                     // booster
  writeCommand(0x30);
  writeCommand(0xF0); writeData(0x11);                          // 4-gray mode
  writeCommand(0xCA); writeData(0x00); writeData(0x3F); writeData(0x00);   // 64 duty
  writeCommand(0xBC); writeData(0x02);                          // scan direction
  writeCommand(0xA6);                                           // normal display
}

bool CasioST75256::begin(uint32_t spiHz, bool clearScreen) {
  _hz = spiHz;
  pinMode(_a0, OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
#if defined(ARDUINO_ARCH_ESP32)
  _spi->begin(_sck, -1, _sda, -1);
#else
  _spi->begin();
#endif
  initSequence();
  if (clearScreen) memset(fb, 0, sizeof(fb));
  display(true);             // write RAM before turning the display on
  writeCommand(0xAF);
  return true;
}

// ---------------- panel control ----------------
void CasioST75256::setDisplayInverse(bool inv) { writeCommand(0x30); writeCommand(inv ? 0xA7 : 0xA6); }
void CasioST75256::displayOn(bool on)          { writeCommand(0x30); writeCommand(on ? 0xAF : 0xAE); }
void CasioST75256::sleep(bool on)              { writeCommand(0x30); writeCommand(on ? 0x95 : 0x94); }
void CasioST75256::setVop(uint16_t vop) {
  if (vop > 511) vop = 511;
  writeCommand(0x30);
  writeCommand(0x81); writeData(vop & 0x3F); writeData((vop >> 6) & 0x07);
}

// Send columns c0..c1 (inclusive) of one page, then remember them as "on the LCD".
void CasioST75256::sendRun(uint8_t page, int16_t c0, int16_t c1) {
  writeCommand(0x75); writeData(page); writeData(page);
  writeCommand(0x15); writeData(_xOff + c0); writeData(_xOff + c1);
  writeCommand(0x5C);
  const uint8_t* src = &fb[page][c0];
  int16_t n = c1 - c0 + 1;
  if (_block) {
    digitalWrite(_a0, _a0Inv ? LOW : HIGH);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(SPISettings(_hz, MSBFIRST, SPI_MODE0));
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    _spi->writeBytes(src, n);
#else
    for (int16_t i = 0; i < n; i++) _spi->transfer(src[i]);
#endif
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  } else {
    for (int16_t i = 0; i < n; i++) writeData(src[i]);
  }
  memcpy(&sent[page][c0], src, n);
}

void CasioST75256::display(bool all) {
  const int16_t GAP = 6;   // merge runs closer than this: cheaper than re-sending 3 commands
  if (all || _forceAll) {
    writeCommand(0x30);
    for (uint8_t p = 0; p < 16; p++) sendRun(p, 0, WIDTH - 1);
    _forceAll = false;
    return;
  }
  bool started = false;
  for (uint8_t p = 0; p < 16; p++) {
    int16_t x = 0;
    while (x < WIDTH) {
      if (fb[p][x] == sent[p][x]) { x++; continue; }
      int16_t start = x, last = x;
      x++;
      while (x < WIDTH && x - last <= GAP) {
        if (fb[p][x] != sent[p][x]) last = x;
        x++;
      }
      if (!started) { writeCommand(0x30); started = true; }
      sendRun(p, start, last);
      x = last + 1;
    }
  }
}

// ---------------- framebuffer ----------------
void CasioST75256::setRam(int16_t col, int16_t row, uint8_t c) {
  if (col < 0 || col >= WIDTH || row < 0 || row >= 64) return;
  uint8_t& b = fb[row >> 2][col];
  uint8_t sh = (3 - (row & 3)) * 2;
  b = (b & ~(3 << sh)) | ((c & 3) << sh);
}
uint8_t CasioST75256::getRam(int16_t col, int16_t row) {
  if (col < 0 || col >= WIDTH || row < 0 || row >= 64) return 0;
  return (fb[row >> 2][col] >> ((3 - (row & 3)) * 2)) & 3;
}

// logical (rotated) coordinates -> physical RAM; caller guarantees x,y are inside width() x height()
void CasioST75256::put(int16_t x, int16_t y, uint8_t color) {
  if (_invertColors) color = 3 - (color & 3);
  int16_t px, py;
  switch (_rot) {
    default:
    case 0: px = x;             py = y;              break;
    case 1: px = WIDTH - 1 - y; py = x;              break;
    case 2: px = WIDTH - 1 - x; py = HEIGHT - 1 - y; break;
    case 3: px = y;             py = HEIGHT - 1 - x; break;
  }
  if (_mirrorX) px = WIDTH - 1 - px;
  if (_mirrorY) py = HEIGHT - 1 - py;
  setRam(px, py, color);
}

void CasioST75256::drawPixel(int16_t x, int16_t y, uint8_t color) {
  if (x < 0 || x >= width() || y < 0 || y >= height()) return;
  put(x, y, color);
}
uint8_t CasioST75256::getPixel(int16_t x, int16_t y) {
  if (x < 0 || x >= width() || y < 0 || y >= height()) return 0;
  int16_t px, py;
  switch (_rot) {
    default:
    case 0: px = x;             py = y;              break;
    case 1: px = WIDTH - 1 - y; py = x;              break;
    case 2: px = WIDTH - 1 - x; py = HEIGHT - 1 - y; break;
    case 3: px = y;             py = HEIGHT - 1 - x; break;
  }
  if (_mirrorX) px = WIDTH - 1 - px;
  if (_mirrorY) py = HEIGHT - 1 - py;
  uint8_t c = getRam(px, py);
  return _invertColors ? 3 - c : c;
}

void CasioST75256::clear(uint8_t color) {
  if (_invertColors) color = 3 - (color & 3);
  uint8_t pattern = (color & 3) * 0x55;   // same 2-bit value in all four pixels of a byte
  for (uint8_t p = 0; p < 16; p++)
    for (int16_t x = 0; x < WIDTH; x++) {
      if (p < 15) fb[p][x] = pattern;
      else fb[p][x] = (pattern & 0xFC) | (fb[p][x] & 0x03);   // keep the icon row (row 64)
    }
  _cx = _cy = 0;
}

// ---------------- shapes ----------------
void CasioST75256::drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
  if (w <= 0 || y < 0 || y >= height()) return;
  int16_t x1 = x + w - 1;
  if (x < 0) x = 0;
  if (x1 >= width()) x1 = width() - 1;
  for (int16_t i = x; i <= x1; i++) put(i, y, color);
}
void CasioST75256::drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color) {
  if (h <= 0 || x < 0 || x >= width()) return;
  int16_t y1 = y + h - 1;
  if (y < 0) y = 0;
  if (y1 >= height()) y1 = height() - 1;
  for (int16_t j = y; j <= y1; j++) put(x, j, color);
}

void CasioST75256::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
  if (y0 == y1) { if (x0 > x1) swap16(x0, x1); drawFastHLine(x0, y0, x1 - x0 + 1, color); return; }
  if (x0 == x1) { if (y0 > y1) swap16(y0, y1); drawFastVLine(x0, y0, y1 - y0 + 1, color); return; }
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;
  while (true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
void CasioST75256::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}
void CasioST75256::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  int16_t x1 = x + w - 1, y1 = y + h - 1;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x1 >= width())  x1 = width() - 1;
  if (y1 >= height()) y1 = height() - 1;
  for (int16_t j = y; j <= y1; j++)
    for (int16_t i = x; i <= x1; i++) put(i, j, color);
}
void CasioST75256::drawCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color) {
  int16_t x = r, y = 0, err = 1 - r;
  while (x >= y) {
    drawPixel(cx + x, cy + y, color); drawPixel(cx - x, cy + y, color);
    drawPixel(cx + x, cy - y, color); drawPixel(cx - x, cy - y, color);
    drawPixel(cx + y, cy + x, color); drawPixel(cx - y, cy + x, color);
    drawPixel(cx + y, cy - x, color); drawPixel(cx - y, cy - x, color);
    y++;
    if (err < 0) err += 2 * y + 1;
    else { x--; err += 2 * (y - x) + 1; }
  }
}
void CasioST75256::fillCircle(int16_t cx, int16_t cy, int16_t r, uint8_t color) {
  if (r < 0) return;
  int32_t r2 = (int32_t)r * r;
  for (int16_t j = -r; j <= r; j++) {
    int32_t v = r2 - (int32_t)j * j;
    int16_t d = (int16_t)sqrtf((float)v);
    while ((int32_t)(d + 1) * (d + 1) <= v) d++;     // exact integer floor(sqrt(v))
    while ((int32_t)d * d > v) d--;
    drawFastHLine(cx - d, cy + j, 2 * d + 1, color);
  }
}
void CasioST75256::drawBitmap(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, uint8_t color, int16_t bg) {
  int16_t bytesPerRow = (w + 7) / 8;
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++) {
      if (pgm_read_byte(&bmp[j * bytesPerRow + i / 8]) & (0x80 >> (i & 7))) drawPixel(x + i, y + j, color);
      else if (bg >= 0) drawPixel(x + i, y + j, (uint8_t)bg);
    }
}
void CasioST75256::drawBitmap2bpp(int16_t x, int16_t y, const uint8_t* bmp, int16_t w, int16_t h, int16_t transparent) {
  int16_t bytesPerRow = (w + 3) / 4;
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++) {
      uint8_t b = pgm_read_byte(&bmp[j * bytesPerRow + i / 4]);
      uint8_t v = (b >> (6 - 2 * (i & 3))) & 3;
      if (transparent >= 0 && v == (uint8_t)transparent) continue;
      drawPixel(x + i, y + j, v);
    }
}

// ---------------- v0.3.0 extras ----------------
void CasioST75256::fillRectDither(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t level, uint8_t c0, uint8_t c1) {
  static const uint8_t bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++)
      drawPixel(x + i, y + j, bayer[(y + j) & 3][(x + i) & 3] < level ? c1 : c0);
}
void CasioST75256::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint8_t color) {
  if (percent > 100) percent = 100;
  drawRect(x, y, w, h, color);
  int16_t iw = w - 4;
  if (iw <= 0 || h <= 4) return;
  fillRect(x + 2, y + 2, (int16_t)((int32_t)iw * percent / 100), h - 4, color);
}
void CasioST75256::invertRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  for (int16_t j = 0; j < h; j++)
    for (int16_t i = 0; i < w; i++) {
      int16_t px = x + i, py = y + j;
      if (px < 0 || px >= width() || py < 0 || py >= height()) continue;
      drawPixel(px, py, 3 - getPixel(px, py));
    }
}

// mask bits: 1 = top-right, 2 = top-left, 4 = bottom-left, 8 = bottom-right
void CasioST75256::drawCircleQuad(int16_t cx, int16_t cy, int16_t r, uint8_t m, uint8_t color) {
  int16_t x = r, y = 0, err = 1 - r;
  while (x >= y) {
    if (m & 1) { drawPixel(cx + x, cy - y, color); drawPixel(cx + y, cy - x, color); }
    if (m & 2) { drawPixel(cx - x, cy - y, color); drawPixel(cx - y, cy - x, color); }
    if (m & 4) { drawPixel(cx - x, cy + y, color); drawPixel(cx - y, cy + x, color); }
    if (m & 8) { drawPixel(cx + x, cy + y, color); drawPixel(cx + y, cy + x, color); }
    y++;
    if (err < 0) err += 2 * y + 1;
    else { x--; err += 2 * (y - x) + 1; }
  }
}

void CasioST75256::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  if (r > w / 2) r = w / 2;
  if (r > h / 2) r = h / 2;
  if (r < 0) r = 0;
  drawFastHLine(x + r, y, w - 2 * r, color);
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
  drawFastVLine(x, y + r, h - 2 * r, color);
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
  if (r > 0) {
    drawCircleQuad(x + r, y + r, r, 2, color);
    drawCircleQuad(x + w - 1 - r, y + r, r, 1, color);
    drawCircleQuad(x + r, y + h - 1 - r, r, 4, color);
    drawCircleQuad(x + w - 1 - r, y + h - 1 - r, r, 8, color);
  }
}

void CasioST75256::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  if (r > w / 2) r = w / 2;
  if (r > h / 2) r = h / 2;
  if (r < 0) r = 0;
  for (int16_t j = 0; j < h; j++) {
    int16_t dy = -1;
    if (j < r) dy = r - 1 - j;
    else if (j >= h - r) dy = j - (h - r);
    int16_t inset = 0;
    if (dy >= 0) {
      float d = dy + 0.5f;
      int16_t span = (int16_t)(sqrtf((float)r * r - d * d) + 0.5f);
      inset = r - span;
    }
    drawFastHLine(x + inset, y + j, w - 2 * inset, color);
  }
}

void CasioST75256::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color) {
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
}

// Scanline fill (much faster than testing every pixel of the bounding box).
void CasioST75256::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color) {
  if (y0 > y1) { swap16(y0, y1); swap16(x0, x1); }
  if (y1 > y2) { swap16(y2, y1); swap16(x2, x1); }
  if (y0 > y1) { swap16(y0, y1); swap16(x0, x1); }
  if (y0 == y2) {   // all on one row
    int16_t a = x0, b = x0;
    if (x1 < a) a = x1; else if (x1 > b) b = x1;
    if (x2 < a) a = x2; else if (x2 > b) b = x2;
    drawFastHLine(a, y0, b - a + 1, color);
    return;
  }
  int32_t dx01 = x1 - x0, dy01 = y1 - y0;
  int32_t dx02 = x2 - x0, dy02 = y2 - y0;
  int32_t dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;
  int16_t last = (y1 == y2) ? y1 : y1 - 1;
  int16_t y;
  for (y = y0; y <= last; y++) {
    int16_t a = x0 + sa / dy01;
    int16_t b = x0 + sb / dy02;
    sa += dx01; sb += dx02;
    if (a > b) swap16(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    int16_t a = x1 + sa / dy12;
    int16_t b = x0 + sb / dy02;
    sa += dx12; sb += dx02;
    if (a > b) swap16(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
  drawTriangle(x0, y0, x1, y1, x2, y2, color);   // keep the edges solid
}

// ---------------- text ----------------
void CasioST75256::drawChar(int16_t x, int16_t y, char c, uint8_t fg, int16_t bg, uint8_t size) {
  if (c < 32 || c > 126) c = '?';
  const uint8_t* g = &FONT5X7[(c - 32) * 5];
  for (int8_t i = 0; i < 5; i++) {
    uint8_t col = pgm_read_byte(&g[i]);
    for (int8_t j = 0; j < 8; j++) {
      if (col & (1 << j)) fillRect(x + i * size, y + j * size, size, size, fg);
      else if (bg >= 0)   fillRect(x + i * size, y + j * size, size, size, (uint8_t)bg);
    }
  }
  if (bg >= 0) fillRect(x + 5 * size, y, size, 8 * size, (uint8_t)bg);
}
size_t CasioST75256::write(uint8_t c) {
  if (c == '\n') { _cx = 0; _cy += 8 * _ts; return 1; }
  if (c == '\r') return 1;
  if (_wrap && _cx + 6 * _ts > width()) { _cx = 0; _cy += 8 * _ts; }   // wrap
  drawChar(_cx, _cy, (char)c, _fg, _bg, _ts);
  _cx += 6 * _ts;
  return 1;
}
int16_t CasioST75256::textWidth(const char* s) const {
  int16_t n = strlen(s);
  return n ? n * 6 * _ts - _ts : 0;
}
void CasioST75256::printCentered(const char* s, int16_t y) {
  int16_t x = (width() - textWidth(s)) / 2;
  if (x < 0) x = 0;
  setCursor(x, y);
  print(s);
}
void CasioST75256::printRight(const char* s, int16_t xRight, int16_t y) {
  int16_t x = xRight - textWidth(s) + 1;
  if (x < 0) x = 0;
  setCursor(x, y);
  print(s);
}

// ---------------- icon row (RAM row 64) ----------------
void CasioST75256::iconSet(int16_t x0, int16_t x1, bool on) {
  if (x0 > x1) swap16(x0, x1);
  for (int16_t x = x0; x <= x1; x++) {
    if (x < 0 || x >= WIDTH) continue;
    int16_t col = _mirrorX ? (WIDTH - 1 - x) : x;
    setRam(col, 63, on ? 3 : 0);
  }
}
void CasioST75256::iconClear() { iconSet(0, WIDTH - 1, false); }
void CasioST75256::iconFill()  { iconSet(0, WIDTH - 1, true); }

void CasioST75256::defineIcon(uint8_t id, int16_t x0, int16_t x1) {
  if (id >= MAX_ICONS) return;
  _iconX0[id] = x0; _iconX1[id] = x1;
}
void CasioST75256::icon(uint8_t id, bool on) {
  if (id >= MAX_ICONS || _iconX0[id] < 0) return;
  iconSet(_iconX0[id], _iconX1[id], on);
}
