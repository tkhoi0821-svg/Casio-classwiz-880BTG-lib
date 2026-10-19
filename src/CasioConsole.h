// CasioConsole v0.1 - scrolling text terminal with scrollback for CasioST75256.   Author: GrayKazu
// A Print object: console.println("boot ok"); console.printf-style via snprintf is up to you.
// Word-wraps by pixel width using whatever font the lcd currently has (5x7, GFX font, or a Unicode UFont: UTF-8
// sequences are never split). Keeps the last MAX_LINES lines; scrollBack() looks at older ones.
// draw() renders into the framebuffer (region cleared first); call lcd.display() afterwards.
#pragma once
#include <Arduino.h>
#include "CasioST75256.h"

class CasioConsole : public Print {
 public:
  static const uint8_t MAX_LINES = 24;       // ring of logical lines
  static const uint8_t LINE_BYTES = 80;      // bytes per line (long lines wrap earlier if needed)

  // Region in logical lcd coordinates; w/h <= 0 means "to the right/bottom edge".
  explicit CasioConsole(CasioST75256& lcd, int16_t x = 0, int16_t y = 0, int16_t w = 0, int16_t h = 0)
      : _lcd(lcd), _x(x), _y(y), _w(w), _h(h) { clear(); }

  size_t write(uint8_t c) override;
  using Print::write;
  void clear();
  void setColor(uint8_t fg)       { _fg = fg; }
  void showCursor(bool on)        { _cursor = on; }          // underscore after the last character
  void scrollBack(int16_t lines); // >0 = look at older lines, <0 = newer; clamped. New output snaps back to the bottom.
  uint16_t lineCount() const      { return _n; }
  const char* line(uint16_t i) const;                        // i = 0 oldest kept line, lineCount()-1 = current line
  void draw();

 private:
  void newLine();
  int16_t regionW() const;
  int16_t regionH() const;

  CasioST75256& _lcd;
  int16_t _x, _y, _w, _h;
  char _l[MAX_LINES][LINE_BYTES + 1];
  uint8_t _len[MAX_LINES];
  uint8_t _head = 0;       // ring index of the oldest line
  uint8_t _n = 1;          // number of lines in use (the last one is being written)
  uint8_t _pending = 0;    // UTF-8 continuation bytes still expected
  uint8_t _fg = 3;
  bool _cursor = false;
  int16_t _off = 0;
};
