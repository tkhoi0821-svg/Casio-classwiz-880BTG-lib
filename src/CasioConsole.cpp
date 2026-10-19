#include "CasioConsole.h"
#include <string.h>

void CasioConsole::clear() {
  memset(_l, 0, sizeof(_l));
  memset(_len, 0, sizeof(_len));
  _head = 0; _n = 1; _pending = 0; _off = 0;
}
int16_t CasioConsole::regionW() const { return _w > 0 ? _w : _lcd.width() - _x; }
int16_t CasioConsole::regionH() const { return _h > 0 ? _h : _lcd.height() - _y; }

const char* CasioConsole::line(uint16_t i) const {
  if (i >= _n) return "";
  return _l[(_head + i) % MAX_LINES];
}

void CasioConsole::newLine() {
  if (_n < MAX_LINES) _n++;
  else {                                    // ring full: drop the oldest line, reuse its slot
    uint8_t slot = _head;
    _head = (_head + 1) % MAX_LINES;
    _l[slot][0] = 0; _len[slot] = 0;
    return;
  }
  uint8_t slot = (_head + _n - 1) % MAX_LINES;
  _l[slot][0] = 0; _len[slot] = 0;
}

size_t CasioConsole::write(uint8_t c) {
  _off = 0;                                  // new output: follow the bottom again
  if (c == '\r') return 1;
  if (c == '\n') { newLine(); _pending = 0; return 1; }
  if (c == '\t') c = ' ';
  uint8_t cur = (_head + _n - 1) % MAX_LINES;
  if (_len[cur] >= LINE_BYTES - 4) {         // out of bytes: wrap here (keeps room for a UTF-8 sequence)
    if (!_pending) { newLine(); cur = (_head + _n - 1) % MAX_LINES; }
  }
  _l[cur][_len[cur]++] = (char)c;
  _l[cur][_len[cur]] = 0;
  if (_pending) { if ((c & 0xC0) == 0x80) _pending--; else _pending = 0; }
  else if (c >= 0xF0) _pending = 3;
  else if (c >= 0xE0) _pending = 2;
  else if (c >= 0xC0) _pending = 1;
  if (_pending) return 1;                    // wait for the rest of the character before measuring
  _lcd.setTextSize(1);
  if (_len[cur] > 1 && _lcd.textWidth(_l[cur]) > regionW()) {
    int k = _len[cur] - 1;                   // start of the last character
    while (k > 0 && ((uint8_t)_l[cur][k] & 0xC0) == 0x80) k--;
    char tail[8]; int tl = _len[cur] - k;
    if (tl > 7) tl = 7;
    memcpy(tail, _l[cur] + k, tl);
    _l[cur][k] = 0; _len[cur] = k;
    newLine();
    cur = (_head + _n - 1) % MAX_LINES;
    memcpy(_l[cur], tail, tl); _l[cur][tl] = 0; _len[cur] = tl;
  }
  return 1;
}

void CasioConsole::scrollBack(int16_t lines) {
  int16_t fh = _lcd.fontHeight(), rows = regionH() / (fh > 0 ? fh : 8);
  int16_t maxOff = (int16_t)_n - rows; if (maxOff < 0) maxOff = 0;
  _off += lines;
  if (_off < 0) _off = 0;
  if (_off > maxOff) _off = maxOff;
}

void CasioConsole::draw() {
  _lcd.setTextSize(1);
  int16_t fh = _lcd.fontHeight(); if (fh <= 0) fh = 8;
  int16_t rows = regionH() / fh; if (rows < 1) rows = 1;
  _lcd.fillRect(_x, _y, regionW(), regionH(), 0);
  int16_t end = (int16_t)_n - _off, start = end - rows;
  if (start < 0) start = 0;
  _lcd.setTextColor(_fg);
  for (int16_t i = start; i < end; i++) {
    int16_t y = _y + (i - start) * fh;
    const char* s = line(i);
    _lcd.setCursor(_x, y);
    _lcd.setTextWrap(false);
    _lcd.print(s);
    _lcd.setTextWrap(true);
    if (_cursor && i == (int16_t)_n - 1 && _off == 0) {
      int16_t cx = _x + _lcd.textWidth(s) + (s[0] ? 1 : 0);
      _lcd.fillRect(cx, y + fh - 2, 5, 1, _fg);
    }
  }
}
