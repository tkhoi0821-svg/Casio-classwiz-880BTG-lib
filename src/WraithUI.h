// WraithUI v0.1 - tiny retained-mode widget toolkit for CasioST75256 (key-driven, no touch).
// Label, button, bar, slider, toggle, checkbox, list, gauge, chart; several screens; focus navigation;
// smooth value tweening; screen transitions that use the HAUNTED FX (glitch, static, melt, CRT-off).
// No malloc: everything lives in a fixed pool (WRAITH_MAX_WIDGETS widgets). Author: GrayKazu
#pragma once
#include "CasioST75256.h"

#ifndef WRAITH_MAX_WIDGETS
#define WRAITH_MAX_WIDGETS 24
#endif

class WraithUI {
 public:
  enum Key   { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK };
  enum Trans { TR_NONE, TR_GLITCH, TR_STATIC, TR_MELT, TR_CRT };
  enum Align { LEFT, CENTER, RIGHT };
  static const uint8_t NONE = 0xFF;                       // "no widget" / pool full
  typedef void (*Handler)(uint8_t id, int16_t value);     // button: 1, slider: value, toggle/check: 0/1, list: index

  explicit WraithUI(CasioST75256& lcd) : _lcd(lcd) {}

  // ---- widgets: every creator returns the widget id (or NONE). Positions are in pixels, y = top. ----
  uint8_t label   (uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, Align a = LEFT);
  uint8_t button  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* text);
  uint8_t bar     (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t value = 0, int16_t mn = 0, int16_t mx = 100);
  uint8_t slider  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value, int16_t step = 1);
  uint8_t toggle  (uint8_t scr, int16_t x, int16_t y, bool on = false);
  uint8_t checkbox(uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, bool checked = false);
  uint8_t list    (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n);
  uint8_t gauge   (uint8_t scr, int16_t cx, int16_t cy, int16_t r, int16_t mn, int16_t mx, int16_t value = 0);
  uint8_t chart   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t* buf, uint16_t n, int16_t vmin, int16_t vmax);

  void    setText(uint8_t id, const char* text);          // copied (max 23 bytes)
  void    setValue(uint8_t id, int16_t v);                // bar/gauge animate toward it
  int16_t value(uint8_t id) const;
  void    chartPush(uint8_t id, int16_t sample);          // shift the chart left and append
  void    setVisible(uint8_t id, bool v);
  void    focus(uint8_t id);
  uint8_t focused() const { return _focus; }

  // ---- fonts: pass a UFont (CasioFont_*.h) or nullptr for the built-in 5x7 font ----
  void setDefaultFont(const UFont* f)          { _defFont = f; _dirty = true; }
  void setFont(uint8_t id, const UFont* f)     { if (id < WRAITH_MAX_WIDGETS) { _w[id].font = f; _w[id].fontSet = true; _dirty = true; } }
  void addFallbackFont(const UFont* f)         { if (_nFb < 2) _fb[_nFb++] = f; }   // glyph fallback after a widget's own font

  // ---- screens, input, frame loop ----
  void show(uint8_t screen, Trans t = TR_NONE, uint16_t ms = 450);
  uint8_t screen() const { return _screen; }
  void onEvent(Handler h) { _handler = h; }
  void onBack(void (*h)()) { _backHandler = h; }          // KEY_BACK; default = go to the previous screen
  void key(Key k);
  // Call from loop() with millis(). Redraws and calls lcd.display() only when something changed;
  // returns true if the frame was sent.
  bool update(uint32_t nowMs);
  void draw();                                            // render the current screen into the framebuffer only

 private:
  enum Type : uint8_t { T_FREE, T_LABEL, T_BUTTON, T_BAR, T_SLIDER, T_TOGGLE, T_CHECK, T_LIST, T_GAUGE, T_CHART };
  struct W {
    uint8_t type = T_FREE, screen = 0, align = 0, n = 0, top = 0;
    bool visible = true, fontSet = false;
    int16_t x = 0, y = 0, w = 0, h = 0, val = 0, mn = 0, mx = 100, step = 1;
    int32_t disp = 0;                                     // tweened value, x16
    const UFont* font = nullptr;
    const char* const* items = nullptr;
    int16_t* buf = nullptr; uint16_t bn = 0;
    char txt[24] = {0};
  };
  uint8_t alloc(uint8_t type, uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h);
  bool focusable(const W& w) const;
  void moveFocus(int dir);
  void emit(uint8_t id, int16_t v) { if (_handler) _handler(id, v); }
  void useFont(const W& w);
  void drawWidget(uint8_t id, bool focused);
  void textIn(const char* s, int16_t x, int16_t y, int16_t w, Align a);
  void listKeepVisible(W& w);
  void switchScreen(uint8_t s);

  CasioST75256& _lcd;
  W _w[WRAITH_MAX_WIDGETS];
  const UFont* _defFont = nullptr;
  const UFont* _fb[2] = {nullptr, nullptr};
  uint8_t _nFb = 0, _screen = 0, _prev = 0, _focus = NONE, _trNew = 0;
  Handler _handler = nullptr;
  void (*_backHandler)() = nullptr;
  bool _dirty = true, _trSwitched = false;
  Trans _tr = TR_NONE;
  uint32_t _last = 0, _trStart = 0, _flashUntil = 0;
  uint16_t _trDur = 0;
  uint8_t _flashId = NONE;
};
