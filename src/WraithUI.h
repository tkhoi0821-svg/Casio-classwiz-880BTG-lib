// WraithUI 2 - retained-mode UI toolkit for CasioST75256 (key-driven, no touch, no malloc).   Author: GrayKazu
//
// Widgets : label, button, bar, slider, toggle, checkbox, list, gauge, chart (v1) and panel, textarea, input, dropdown,
//           roller, spinbox, tabview, arc, meter, led, spinner, image, table, custom (v2)
// Popups  : dropdown list, msgbox (modal, up to 3 buttons), toast
// Layout  : containers (panel / tabview) with column / row / wrapping-row layout, padding, gaps, scrolling that follows focus
// Style   : 4 themes (light, dark, gray, haunted) + per-widget Style overrides + per-widget fonts (UFont, GFX)
// Input   : keyDown/keyUp/key (up, down, left, right, ok, back), long press, key repeat, typeChar/backspace for text input
// Logic   : per-widget event callbacks, animations (position, size, value, scroll), timers, screen stack (push/pop),
//           screen transitions built from the HAUNTED FX (glitch, static, melt, CRT) plus slides
// Fixed pools: WRAITH_MAX_WIDGETS widgets, WRAITH_TEXT_POOL bytes of text, 8 animations, 4 timers.
#pragma once
#include "CasioST75256.h"
#include "CasioTween.h"

#ifndef WRAITH_MAX_WIDGETS
#define WRAITH_MAX_WIDGETS 40
#endif
#ifndef WRAITH_TEXT_POOL
#define WRAITH_TEXT_POOL 2048
#endif

class WraithUI {
 public:
  enum Key   { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK };
  enum Trans { TR_NONE, TR_GLITCH, TR_STATIC, TR_MELT, TR_CRT,
               TR_SLIDE_LEFT, TR_SLIDE_RIGHT, TR_SLIDE_UP, TR_SLIDE_DOWN };   // slide: the new screen pushes the old one out
  enum Align { LEFT, CENTER, RIGHT };
  // events delivered to a widget's callback (see on()); the global onEvent() handler only sees EV_CLICK and EV_VALUE
  enum Ev : uint8_t { EV_CLICK, EV_VALUE, EV_FOCUS, EV_DEFOCUS, EV_LONG, EV_CHAR, EV_OPEN, EV_CLOSE, EV_ANIM_DONE };
  enum Layout : uint8_t { LAYOUT_NONE, LAYOUT_COLUMN, LAYOUT_ROW, LAYOUT_ROW_WRAP };
  enum LongMode : uint8_t { LONG_CLIP, LONG_DOTS, LONG_WRAP, LONG_SCROLL };     // label text wider than the label
  enum Prop : uint8_t { P_X, P_Y, P_W, P_H, P_VALUE, P_SCROLL_X, P_SCROLL_Y };  // animatable properties
  enum Theme : uint8_t { THEME_LIGHT, THEME_DARK, THEME_GRAY, THEME_HAUNTED };
  static const uint8_t NONE = 0xFF;                       // "no widget" / "no parent" / "no tab" / pool full

  typedef void (*Handler)(uint8_t id, int16_t value);                         // global: click (button 1, list index) / value
  typedef void (*EventCb)(uint8_t id, uint8_t ev, int16_t value, void* user); // per widget, see on()
  typedef void (*DrawCb)(WraithUI& ui, CasioST75256& lcd, uint8_t id, int16_t x, int16_t y, int16_t w, int16_t h,
                         bool focused, void* user);                          // custom widget; drawing is clipped to its box
  typedef void (*TimerCb)(void* user);
  typedef void (*MsgCb)(int8_t button, void* user);                           // msgbox result: button index, -1 = cancelled
  // Per-widget look. -1 = inherit from the theme. fg/bg/borderColor are gray levels 0 (white) .. 3 (black), bg 4 = transparent.
  struct Style { int8_t fg = -1, bg = -1, border = -1, borderColor = -1, radius = -1, pad = -1; };

  explicit WraithUI(CasioST75256& lcd) : _lcd(lcd) { setTheme(THEME_LIGHT); }

  // ================= creating widgets (all return the widget id, or NONE when the pool is full) =================
  // scr = screen number the widget belongs to. x/y = top-left in pixels, relative to the parent's content area when
  // the widget has a parent (see setParent). w/h = 0 means "automatic" where that makes sense (labels, buttons).
  uint8_t panel   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h);                  // container, optional border/fill
  uint8_t label   (uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, Align a = LEFT, uint16_t cap = 0);
  uint8_t button  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* text, uint16_t cap = 0);
  uint8_t bar     (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t value = 0, int16_t mn = 0, int16_t mx = 100);
  uint8_t slider  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value, int16_t step = 1);
  uint8_t toggle  (uint8_t scr, int16_t x, int16_t y, bool on = false);
  uint8_t checkbox(uint8_t scr, int16_t x, int16_t y, int16_t w, const char* text, bool checked = false, uint16_t cap = 0);
  uint8_t list    (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n);
  uint8_t gauge   (uint8_t scr, int16_t cx, int16_t cy, int16_t r, int16_t mn, int16_t mx, int16_t value = 0);   // x,y = CENTER of the half ring
  uint8_t chart   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, int16_t* buf, uint16_t n, int16_t vmin, int16_t vmax);
  // --- new in WraithUI 2 ---
  uint8_t textarea(uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t cap = 192);   // wrapped, scrollable text / log
  uint8_t input   (uint8_t scr, int16_t x, int16_t y, int16_t w, uint16_t cap = 24);               // one-line editor (typeChar/backspace)
  uint8_t dropdown(uint8_t scr, int16_t x, int16_t y, int16_t w, const char* const* items, uint8_t n);
  uint8_t roller  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n);   // picker, selected row centered
  uint8_t spinbox (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value, int16_t step = 1);
  uint8_t tabview (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* tabs, uint8_t n);   // children: setParent(child, tabview, tabIndex)
  uint8_t arc     (uint8_t scr, int16_t x, int16_t y, int16_t size, int16_t mn, int16_t mx, int16_t value = 0, bool interactive = true);
  uint8_t meter   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t value = 0, uint8_t ticks = 5);   // dial with ticks + needle
  uint8_t led     (uint8_t scr, int16_t x, int16_t y, int16_t size, uint8_t level = 3);          // level 0 (off) .. 3
  uint8_t spinner (uint8_t scr, int16_t x, int16_t y, int16_t size);                             // loading indicator
  uint8_t image   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* bmp2bpp, int16_t transparent = -1);
  uint8_t table   (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* cells, uint8_t cols, uint8_t rows, bool header = true);
  uint8_t custom  (uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h, DrawCb draw, void* user = nullptr, bool focusable = false);

  // ================= widget properties =================
  void    setParent(uint8_t child, uint8_t parent, uint8_t tab = NONE);   // parent = panel or tabview; tab = tab index (tabview only)
  void    setLayout(uint8_t container, Layout l, uint8_t gap = 2, uint8_t pad = 2);
  void    setScrollable(uint8_t container, bool on);                       // vertical + horizontal; follows focus; scrollbars
  void    scrollTo(uint8_t container, int16_t x, int16_t y, uint16_t animMs = 0);
  void    setText(uint8_t id, const char* text);                           // copied; truncated to the widget's capacity
  void    setTextRef(uint8_t id, const char* text);                        // NOT copied: must stay valid (flash strings, long texts)
  const char* text(uint8_t id) const;
  void    appendText(uint8_t id, const char* text);                        // textarea: add text, follow the end if the view was at the end
  void    clearText(uint8_t id);
  void    setLongMode(uint8_t label, LongMode m);
  void    setAlign(uint8_t id, Align a);                                   // text alignment (label, button, input, dropdown...) / cross-axis of layouts
  void    setValue(uint8_t id, int16_t v);                                 // clamped; bar/gauge/slider animate toward it
  int16_t value(uint8_t id) const;
  void    setRange(uint8_t id, int16_t mn, int16_t mx, int16_t step = 0);
  void    setItems(uint8_t id, const char* const* items, uint8_t n);       // list, dropdown, roller, tab titles
  void    chartPush(uint8_t id, int16_t sample);
  void    setVertical(uint8_t id, bool v);                                 // bar / slider orientation
  void    setVisible(uint8_t id, bool v);
  void    setEnabled(uint8_t id, bool e);                                  // disabled widgets are drawn gray and skipped by focus
  void    setPos(uint8_t id, int16_t x, int16_t y);
  void    setSize(uint8_t id, int16_t w, int16_t h);
  void    setStyle(uint8_t id, const Style* s);                            // s must outlive the widget (nullptr = theme)
  void    setFont(uint8_t id, const UFont* f)  { if (id < WRAITH_MAX_WIDGETS) { _w[id].font = f; _w[id].flags |= F_FONTSET; _dirty = _layoutDirty = true; } }
  void    focus(uint8_t id);
  uint8_t focused() { ensureFocus(); return _focus; }
  bool    exists(uint8_t id) const { return id < WRAITH_MAX_WIDGETS && _w[id].type != T_FREE; }
  void    remove(uint8_t id);                                              // frees the slot (and children); its text stays in the pool

  // ================= theme and fonts =================
  void setTheme(Theme t);
  Theme theme() const { return _theme; }
  void setDefaultFont(const UFont* f)          { _defFont = f; _dirty = _layoutDirty = true; }
  void addFallbackFont(const UFont* f)         { if (_nFb < 2) _fb[_nFb++] = f; }   // glyph fallback after a widget's own font

  // ================= events =================
  void onEvent(Handler h)       { _handler = h; }                          // global, compatible with WraithUI 1
  void on(uint8_t id, EventCb cb, void* user = nullptr) { if (id < WRAITH_MAX_WIDGETS) { _w[id].cb = cb; _w[id].user = user; } }
  void onBack(void (*h)())      { _backHandler = h; }                      // KEY_BACK when nothing else handles it
  void onScreenLoad(void (*h)(uint8_t screen)) { _loadHandler = h; }

  // ================= screens =================
  void show(uint8_t screen, Trans t = TR_NONE, uint16_t ms = 450);
  void push(uint8_t screen, Trans t = TR_NONE, uint16_t ms = 450);         // remember the current screen (depth 6) ...
  bool pop(Trans t = TR_NONE, uint16_t ms = 450);                          // ... and come back to it; KEY_BACK does this by default
  uint8_t screen() const { return _screen; }

  // ================= popups =================
  void msgbox(const char* title, const char* text, const char* b1, const char* b2 = nullptr, const char* b3 = nullptr,
              MsgCb cb = nullptr, void* user = nullptr);                   // modal until a button is pressed (BACK = cancel, -1)
  bool msgboxOpen() const { return _mb.active; }
  void toast(const char* text, uint16_t ms = 1500);
  void closeDropdown();

  // ================= animation and timers =================
  // Move a property to `to` over `ms` milliseconds. EV_ANIM_DONE is sent to the widget when it ends. Returns false if the pool (8) is full.
  bool animate(uint8_t id, Prop p, int16_t to, uint16_t ms, CasioEase::Ease e = CasioEase::OUT_CUBIC);
  uint8_t timer(uint16_t periodMs, TimerCb cb, void* user = nullptr, bool repeat = true);   // returns 0..3 or NONE
  void stopTimer(uint8_t t) { if (t < 4) _tm[t].cb = nullptr; }

  // ================= input and frame loop =================
  void keyDown(Key k);                                                     // long press (OK held 600 ms -> EV_LONG) and key repeat need keyUp
  void keyUp(Key k);
  void key(Key k) { keyDown(k); keyUp(k); }                                // a complete press
  bool typeChar(const char* utf8);                                         // into the focused input; returns false if nothing took it
  bool backspace();
  // Call from loop() with millis(). Redraws and calls lcd.display() only when something changed (animations ~30 fps);
  // returns true if a frame was sent.
  bool update(uint32_t nowMs);
  void draw();                                                             // render the current screen into the framebuffer only

 private:
  enum Type : uint8_t { T_FREE, T_LABEL, T_BUTTON, T_BAR, T_SLIDER, T_TOGGLE, T_CHECK, T_LIST, T_GAUGE, T_CHART, T_PANEL, T_TEXTAREA,
                        T_INPUT, T_DROPDOWN, T_ROLLER, T_SPINBOX, T_TABVIEW, T_ARC, T_METER, T_LED, T_SPINNER, T_IMAGE, T_TABLE, T_CUSTOM };
  enum : uint8_t { F_VISIBLE = 1, F_ENABLED = 2, F_FONTSET = 4, F_SCROLL = 8, F_VERT = 16, F_FOLLOW = 32, F_INTERACT = 64, F_HEADER = 128 };
  struct W {
    uint8_t type = T_FREE, screen = 0, parent = NONE, tab = NONE, flags = 0, align = 0, longMode = 0, layout = 0;
    uint8_t gap = 0, pad = 0, n = 0, top = 0, cols = 0, autosz = 0;      // autosz: bit0 = width automatic, bit1 = height automatic
    int16_t x = 0, y = 0, w = 0, h = 0, val = 0, mn = 0, mx = 100, step = 1;
    int16_t sx = 0, sy = 0, cw = 0, ch = 0, ax = 0, ay = 0;     // scroll offset, content size, absolute position from the last draw
    int32_t disp = 0;                                           // tweened value x16
    const UFont* font = nullptr; const Style* style = nullptr;
    char* txt = nullptr; uint16_t cap = 0;
    const void* p1 = nullptr; uint16_t n16 = 0;                 // items / chart buffer / bitmap / draw callback
    EventCb cb = nullptr; void* user = nullptr;
  };
  struct Anim { uint8_t id = NONE, prop = 0, ease = 0; int16_t from = 0, to = 0; uint32_t t0 = 0; uint16_t dur = 0; };
  struct Tmr { TimerCb cb = nullptr; void* user = nullptr; uint32_t last = 0; uint16_t period = 0; bool repeat = true; };
  struct Msg { bool active = false; int8_t sel = 0; uint8_t nb = 0; char title[24], text[100], b[3][12]; MsgCb cb = nullptr; void* user = nullptr; };
  struct Colors { uint8_t bg, fg, track, accent, panel, border, fbg, ffg, dis, radius; };
  struct Rect { int16_t x0, y0, x1, y1; };                      // inclusive

  uint8_t alloc(uint8_t type, uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h);
  void    initText(W& w, const char* t, uint16_t cap);
  char*   allocText(uint16_t cap);
  bool    alive(uint8_t id) const { return id < WRAITH_MAX_WIDGETS && _w[id].type != T_FREE; }
  bool    isContainer(const W& w) const { return w.type == T_PANEL || w.type == T_TABVIEW; }
  bool    shown(uint8_t id) const;                              // visible, on the current screen, ancestors visible, tab active
  bool    focusable(uint8_t id) const;
  void    ensureFocus();
  void    moveFocus(int dir);
  void    setFocus(uint8_t id, bool announce = true);
  void    ensureVisible(uint8_t id);
  void    fire(uint8_t id, uint8_t ev, int16_t v);
  void    emitValue(uint8_t id, int16_t v) { fire(id, EV_VALUE, v); if (_handler) _handler(id, v); }
  void    emitClick(uint8_t id, int16_t v) { fire(id, EV_CLICK, v); if (_handler) _handler(id, v); }
  void    handleKey(Key k);
  void    activate(uint8_t id);
  void    doBack();
  void    mbKey(Key k);
  void    ddKey(Key k);
  void    closeMsg(int8_t result);
  void    openDropdown(uint8_t id);
  void    layoutAll();
  void    layoutTree(uint8_t id);
  void    drawScrollbars(const W& w, int16_t ax, int16_t ay);
  int16_t textLines(W& w);
  int16_t prefixWidth(const W& w, int bytes);
  Rect    pushClip(int16_t x, int16_t y, int16_t w, int16_t h);
  void    popClip(const Rect& r) { _curClip = r; setClip(r); }
  static bool repeatable(uint8_t type);
  void    layoutContainer(uint8_t id);
  void    contentExtents(uint8_t id);
  void    innerRect(const W& w, int16_t* ix, int16_t* iy, int16_t* iw, int16_t* ih) const;
  int16_t hdrH(const W& w);
  int16_t naturalW(W& w);
  int16_t naturalH(W& w, int16_t cw);
  void    clampScroll(W& w);
  int16_t getProp(const W& w, uint8_t p) const;
  void    setProp(uint8_t id, uint8_t p, int16_t v);
  void    stepAnims(uint32_t now);
  void    stepTimers(uint32_t now);
  Colors  colors(const W& w) const;
  void    useFont(const W& w);
  void    textIn(const char* s, int16_t x, int16_t y, int16_t w, Align a);
  void    drawTree(uint8_t id, int16_t ox, int16_t oy, Rect clip);
  void    drawWidget(uint8_t id, int16_t ax, int16_t ay, bool foc);
  void    drawPopups();
  void    dimScreen();
  void    listKeepVisible(W& w);
  void    switchScreen(uint8_t s);
  void    setClip(const Rect& r) { _lcd.setClipRect(r.x0, r.y0, r.x1 - r.x0 + 1, r.y1 - r.y0 + 1); }

  CasioST75256& _lcd;
  W _w[WRAITH_MAX_WIDGETS];
  char _pool[WRAITH_TEXT_POOL]; uint16_t _poolUsed = 0;
  Anim _an[8]; Tmr _tm[4];
  Msg _mb;
  struct { char text[40]; uint32_t t0 = 0, until = 0; bool active = false; } _toast;
  Colors _c; Theme _theme = THEME_LIGHT; Rect _curClip = {0, 0, 191, 62};
  const UFont* _defFont = nullptr; const UFont* _fb[2] = {nullptr, nullptr}; uint8_t _nFb = 0;
  uint8_t _screen = 0, _prev = 0, _focus = NONE, _trNew = 0, _stack[6]; uint8_t _sp = 0;
  uint8_t _ddOpen = NONE; int16_t _ddSel = 0;
  Handler _handler = nullptr; void (*_backHandler)() = nullptr; void (*_loadHandler)(uint8_t) = nullptr;
  bool _dirty = true, _layoutDirty = true, _trSwitched = false, _needAnim = false;
  Trans _tr = TR_NONE;
  uint32_t _last = 0, _now = 0, _lastDraw = 0, _trStart = 0, _flashUntil = 0, _glitchUntil = 0, _okSince = 0, _heldSince = 0, _lastRep = 0;
  uint16_t _trDur = 0;
  uint8_t _flashId = NONE, _pressed = NONE; int8_t _held = -1; bool _longFired = false;
  uint8_t _snapOld[3072], _snapNew[3072];                       // frames for the slide transitions
};
