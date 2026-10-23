#include "WraithUI.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static inline int16_t imin(int16_t a, int16_t b) { return a < b ? a : b; }
static inline int16_t imax(int16_t a, int16_t b) { return a > b ? a : b; }
static char g_empty[1] = {0};

// ================= theme =================
void WraithUI::setTheme(Theme t) {
  _theme = t;
  //            bg fg trk acc pnl bdr fbg ffg dis rad
  static const Colors L = {0, 3, 1, 3, 0, 3, 3, 0, 1, 3}, D = {3, 0, 2, 0, 3, 0, 0, 3, 2, 3},
                      G = {1, 3, 0, 3, 0, 3, 3, 0, 2, 3}, H = {3, 0, 2, 0, 3, 0, 0, 3, 2, 0};
  _c = t == THEME_DARK ? D : t == THEME_GRAY ? G : t == THEME_HAUNTED ? H : L;
  _dirty = true;
}

WraithUI::Colors WraithUI::colors(const W& w) const {
  Colors c = _c;
  if (const Style* s = w.style) {
    if (s->fg >= 0)          { c.fg = s->fg & 3; c.accent = s->fg & 3; }
    if (s->bg >= 0)          c.panel = s->bg > 4 ? 4 : s->bg;          // 4 = transparent
    if (s->borderColor >= 0) c.border = s->borderColor & 3;
    if (s->radius >= 0)      c.radius = s->radius;
  }
  return c;
}

// ================= text pool =================
char* WraithUI::allocText(uint16_t cap) {
  if (_poolUsed + cap > WRAITH_TEXT_POOL) return nullptr;
  char* p = _pool + _poolUsed; _poolUsed += cap; memset(p, 0, cap); return p;
}
static void copyUtf8(char* d, uint16_t cap, const char* s) {
  size_t n = strlen(s);
  if (n >= cap) { n = cap - 1; while (n > 0 && ((uint8_t)s[n] & 0xC0) == 0x80) n--; }   // never cut inside a UTF-8 sequence
  memcpy(d, s, n); d[n] = 0;
}
void WraithUI::initText(W& w, const char* t, uint16_t cap) {
  if (!t) t = "";
  uint16_t need = strlen(t) + 1;
  if (cap == 0) cap = need < 24 ? 24 : need;
  w.txt = allocText(cap);
  if (!w.txt) { w.txt = g_empty; w.cap = 0; return; }
  w.cap = cap; copyUtf8(w.txt, cap, t);
}
void WraithUI::setText(uint8_t id, const char* t) {
  if (!alive(id)) return;
  W& w = _w[id]; if (!t) t = "";
  if (!w.txt || w.cap == 0) { initText(w, t, 0); }
  else copyUtf8(w.txt, w.cap, t);
  if (w.type == T_INPUT) w.val = strlen(w.txt);
  _dirty = _layoutDirty = true;
}
void WraithUI::setTextRef(uint8_t id, const char* t) {
  if (!alive(id)) return;
  _w[id].txt = (char*)(t ? t : g_empty); _w[id].cap = 0; _dirty = _layoutDirty = true;
}
const char* WraithUI::text(uint8_t id) const { return alive(id) && _w[id].txt ? _w[id].txt : ""; }
void WraithUI::clearText(uint8_t id) { if (alive(id) && _w[id].cap) { _w[id].txt[0] = 0; _w[id].top = 0; _w[id].val = 0; _w[id].flags |= F_FOLLOW; _dirty = true; } }
void WraithUI::appendText(uint8_t id, const char* t) {
  if (!alive(id) || !t || !_w[id].cap) return;
  W& w = _w[id]; size_t add = strlen(t), len = strlen(w.txt);
  if (add >= w.cap) { t += add - (w.cap - 1); while (((uint8_t)*t & 0xC0) == 0x80) t++; add = strlen(t); }
  if (len + add >= w.cap) {                                       // full: drop the oldest lines (or bytes) to make room
    size_t drop = len + add - (w.cap - 1), cut = drop;
    while (w.txt[cut] && w.txt[cut] != '\n') cut++;
    if (w.txt[cut] == '\n') cut++; else { cut = drop; while (((uint8_t)w.txt[cut] & 0xC0) == 0x80) cut++; }
    memmove(w.txt, w.txt + cut, len - cut + 1); len -= cut;
  }
  memcpy(w.txt + len, t, add); w.txt[len + add] = 0;
  _dirty = true;
}

// ================= creation =================
uint8_t WraithUI::alloc(uint8_t type, uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h) {
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++)
    if (_w[i].type == T_FREE) {
      W& k = _w[i]; k = W();
      k.type = type; k.screen = scr; k.x = x; k.y = y; k.w = w; k.h = h;
      k.flags = F_VISIBLE | F_ENABLED; k.autosz = (w == 0 ? 1 : 0) | (h == 0 ? 2 : 0);
      _dirty = _layoutDirty = true;
      return i;
    }
  return NONE;
}
uint8_t WraithUI::panel(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h) { return alloc(T_PANEL, s, x, y, w, h); }
uint8_t WraithUI::label(uint8_t s, int16_t x, int16_t y, int16_t w, const char* t, Align a, uint16_t cap) {
  uint8_t i = alloc(T_LABEL, s, x, y, w, 0); if (i != NONE) { initText(_w[i], t, cap); _w[i].align = a; } return i;
}
uint8_t WraithUI::button(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* t, uint16_t cap) {
  uint8_t i = alloc(T_BUTTON, s, x, y, w, h); if (i != NONE) { initText(_w[i], t, cap); _w[i].align = CENTER; } return i;
}
uint8_t WraithUI::bar(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, int16_t v, int16_t mn, int16_t mx) {
  uint8_t i = alloc(T_BAR, s, x, y, w, h);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].disp = (int32_t)v * 16; } return i;
}
uint8_t WraithUI::slider(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t v, int16_t st) {
  uint8_t i = alloc(T_SLIDER, s, x, y, w, 11);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].step = st ? st : 1; _w[i].disp = (int32_t)v * 16; } return i;
}
uint8_t WraithUI::toggle(uint8_t s, int16_t x, int16_t y, bool on) {
  uint8_t i = alloc(T_TOGGLE, s, x, y, 22, 11); if (i != NONE) _w[i].val = on; return i;
}
uint8_t WraithUI::checkbox(uint8_t s, int16_t x, int16_t y, int16_t w, const char* t, bool c, uint16_t cap) {
  uint8_t i = alloc(T_CHECK, s, x, y, w, 9); if (i != NONE) { initText(_w[i], t, cap); _w[i].val = c; } return i;
}
uint8_t WraithUI::list(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n) {
  uint8_t i = alloc(T_LIST, s, x, y, w, h); if (i != NONE) { _w[i].p1 = items; _w[i].n = n; } return i;
}
uint8_t WraithUI::gauge(uint8_t s, int16_t cx, int16_t cy, int16_t r, int16_t mn, int16_t mx, int16_t v) {
  uint8_t i = alloc(T_GAUGE, s, cx, cy, r, r);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].disp = (int32_t)v * 16; _w[i].autosz = 0; } return i;
}
uint8_t WraithUI::chart(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, int16_t* buf, uint16_t n, int16_t vmin, int16_t vmax) {
  uint8_t i = alloc(T_CHART, s, x, y, w, h);
  if (i != NONE) { _w[i].p1 = buf; _w[i].n16 = n; _w[i].mn = vmin; _w[i].mx = vmax; } return i;
}
uint8_t WraithUI::textarea(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t cap) {
  uint8_t i = alloc(T_TEXTAREA, s, x, y, w, h);
  if (i != NONE) { initText(_w[i], "", cap); _w[i].flags |= F_INTERACT | F_FOLLOW; } return i;
}
uint8_t WraithUI::input(uint8_t s, int16_t x, int16_t y, int16_t w, uint16_t cap) {
  uint8_t i = alloc(T_INPUT, s, x, y, w, 0); if (i != NONE) initText(_w[i], "", cap); return i;
}
uint8_t WraithUI::dropdown(uint8_t s, int16_t x, int16_t y, int16_t w, const char* const* items, uint8_t n) {
  uint8_t i = alloc(T_DROPDOWN, s, x, y, w, 0); if (i != NONE) { _w[i].p1 = items; _w[i].n = n; } return i;
}
uint8_t WraithUI::roller(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n) {
  uint8_t i = alloc(T_ROLLER, s, x, y, w, h); if (i != NONE) { _w[i].p1 = items; _w[i].n = n; } return i;
}
uint8_t WraithUI::spinbox(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t v, int16_t st) {
  uint8_t i = alloc(T_SPINBOX, s, x, y, w, 0); if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].step = st ? st : 1; } return i;
}
uint8_t WraithUI::tabview(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* tabs, uint8_t n) {
  uint8_t i = alloc(T_TABVIEW, s, x, y, w, h); if (i != NONE) { _w[i].p1 = tabs; _w[i].n = n; } return i;
}
uint8_t WraithUI::arc(uint8_t s, int16_t x, int16_t y, int16_t size, int16_t mn, int16_t mx, int16_t v, bool inter) {
  uint8_t i = alloc(T_ARC, s, x, y, size, size);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].disp = (int32_t)v * 16; if (inter) _w[i].flags |= F_INTERACT; } return i;
}
uint8_t WraithUI::meter(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t mn, int16_t mx, int16_t v, uint8_t ticks) {
  uint8_t i = alloc(T_METER, s, x, y, w, w / 2 + 10);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].disp = (int32_t)v * 16; _w[i].n = ticks < 2 ? 2 : ticks; } return i;
}
uint8_t WraithUI::led(uint8_t s, int16_t x, int16_t y, int16_t size, uint8_t level) {
  uint8_t i = alloc(T_LED, s, x, y, size, size); if (i != NONE) _w[i].val = level > 3 ? 3 : level; return i;
}
uint8_t WraithUI::spinner(uint8_t s, int16_t x, int16_t y, int16_t size) { return alloc(T_SPINNER, s, x, y, size, size); }
uint8_t WraithUI::image(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t* bmp, int16_t transparent) {
  uint8_t i = alloc(T_IMAGE, s, x, y, w, h); if (i != NONE) { _w[i].p1 = bmp; _w[i].mn = transparent; } return i;
}
uint8_t WraithUI::table(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* cells, uint8_t cols, uint8_t rows, bool header) {
  uint8_t i = alloc(T_TABLE, s, x, y, w, h);
  if (i != NONE) { _w[i].p1 = cells; _w[i].cols = cols ? cols : 1; _w[i].n = rows; _w[i].flags |= F_INTERACT | (header ? F_HEADER : 0); } return i;
}
uint8_t WraithUI::custom(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, DrawCb draw, void* user, bool focusable) {
  uint8_t i = alloc(T_CUSTOM, s, x, y, w, h); if (i != NONE) { _w[i].p1 = (const void*)draw; _w[i].user = user; if (focusable) _w[i].flags |= F_INTERACT; } return i;
}

// ================= properties =================
void WraithUI::setParent(uint8_t child, uint8_t parent, uint8_t tab) {
  if (!alive(child) || !alive(parent) || child == parent || !isContainer(_w[parent])) return;
  _w[child].parent = parent; _w[child].tab = tab; _w[child].screen = _w[parent].screen;
  _dirty = _layoutDirty = true;
}
void WraithUI::setLayout(uint8_t c, Layout l, uint8_t gap, uint8_t pad) {
  if (!alive(c)) return;
  _w[c].layout = l; _w[c].gap = gap; _w[c].pad = pad; _dirty = _layoutDirty = true;
}
void WraithUI::setScrollable(uint8_t c, bool on) {
  if (!alive(c)) return;
  if (on) _w[c].flags |= F_SCROLL; else { _w[c].flags &= ~F_SCROLL; _w[c].sx = _w[c].sy = 0; } _dirty = _layoutDirty = true;
}
void WraithUI::scrollTo(uint8_t c, int16_t x, int16_t y, uint16_t ms) {
  if (!alive(c)) return;
  if (ms) { animate(c, P_SCROLL_X, x, ms); animate(c, P_SCROLL_Y, y, ms); }
  else { _w[c].sx = x; _w[c].sy = y; clampScroll(_w[c]); _dirty = true; }
}
void WraithUI::setLongMode(uint8_t id, LongMode m) { if (alive(id)) { _w[id].longMode = m; _dirty = _layoutDirty = true; } }
void WraithUI::setAlign(uint8_t id, Align a) { if (alive(id)) { _w[id].align = a; _dirty = _layoutDirty = true; } }
void WraithUI::setValue(uint8_t id, int16_t v) {
  if (!alive(id)) return;
  W& w = _w[id];
  if (w.type == T_INPUT || w.type == T_TEXTAREA) return;
  if (v < w.mn) v = w.mn;
  if (v > w.mx) v = w.mx;
  if (w.type == T_LIST || w.type == T_DROPDOWN || w.type == T_ROLLER || w.type == T_TABLE) { v = imax(0, imin(v, (int16_t)w.n - 1 - ((w.flags & F_HEADER) ? 1 : 0))); if (w.type == T_LIST) listKeepVisible(w); }
  if (w.type == T_TABVIEW) v = imax(0, imin(v, (int16_t)w.n - 1));
  w.val = v; _dirty = true;
  if (w.type == T_TABVIEW || w.type == T_LED || w.type == T_TOGGLE || w.type == T_CHECK) _layoutDirty = true;
}
int16_t WraithUI::value(uint8_t id) const { return alive(id) ? _w[id].val : 0; }
void WraithUI::setRange(uint8_t id, int16_t mn, int16_t mx, int16_t st) {
  if (!alive(id)) return;
  _w[id].mn = mn; _w[id].mx = mx; if (st) _w[id].step = st;
  if (_w[id].val < mn) _w[id].val = mn;
  if (_w[id].val > mx) _w[id].val = mx;
  _dirty = true;
}
void WraithUI::setItems(uint8_t id, const char* const* items, uint8_t n) {
  if (!alive(id)) return;
  _w[id].p1 = items; _w[id].n = n; if (_w[id].val >= n) _w[id].val = n ? n - 1 : 0; _dirty = _layoutDirty = true;
}
void WraithUI::chartPush(uint8_t id, int16_t s) {
  if (!alive(id) || _w[id].type != T_CHART || !_w[id].p1 || !_w[id].n16) return;
  int16_t* b = (int16_t*)_w[id].p1;
  memmove(b, b + 1, (_w[id].n16 - 1) * sizeof(int16_t)); b[_w[id].n16 - 1] = s; _dirty = true;
}
void WraithUI::setVertical(uint8_t id, bool v) { if (alive(id)) { if (v) _w[id].flags |= F_VERT; else _w[id].flags &= ~F_VERT; _dirty = true; } }
void WraithUI::setVisible(uint8_t id, bool v) {
  if (!alive(id)) return;
  if (v) _w[id].flags |= F_VISIBLE;
  else _w[id].flags &= ~F_VISIBLE;
  _dirty = _layoutDirty = true;
}
void WraithUI::setEnabled(uint8_t id, bool e) {
  if (!alive(id)) return;
  if (e) _w[id].flags |= F_ENABLED; else _w[id].flags &= ~F_ENABLED; _dirty = true;
}
void WraithUI::setPos(uint8_t id, int16_t x, int16_t y) { if (alive(id)) { _w[id].x = x; _w[id].y = y; _dirty = true; if (_w[id].parent != NONE) _layoutDirty = true; } }
void WraithUI::setSize(uint8_t id, int16_t w, int16_t h) {
  if (!alive(id)) return;
  _w[id].w = w; _w[id].h = h; _w[id].autosz = (w == 0 ? 1 : 0) | (h == 0 ? 2 : 0); _dirty = _layoutDirty = true;
}
void WraithUI::setStyle(uint8_t id, const Style* s) { if (alive(id)) { _w[id].style = s; _dirty = _layoutDirty = true; } }
void WraithUI::remove(uint8_t id) {
  if (!alive(id)) return;
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) if (_w[i].type != T_FREE && _w[i].parent == id) remove(i);
  for (uint8_t a = 0; a < 8; a++) if (_an[a].id == id) _an[a].id = NONE;
  if (_focus == id) _focus = NONE;
  if (_ddOpen == id) _ddOpen = NONE;
  _w[id] = W(); _dirty = _layoutDirty = true;
}

// ================= visibility and focus =================
bool WraithUI::shown(uint8_t id) const {
  uint8_t c = id;
  for (uint8_t depth = 0; depth < 8 && c < WRAITH_MAX_WIDGETS; depth++) {
    const W& k = _w[c];
    if (k.type == T_FREE || !(k.flags & F_VISIBLE)) return false;
    if (k.parent == NONE) return k.screen == _screen;
    const W& p = _w[k.parent];
    if (p.type == T_TABVIEW && k.tab != NONE && k.tab != p.val) return false;
    c = k.parent;
  }
  return false;
}
bool WraithUI::focusable(uint8_t id) const {
  if (!alive(id) || !shown(id) || !(_w[id].flags & F_ENABLED)) return false;
  switch (_w[id].type) {
    case T_BUTTON: case T_SLIDER: case T_TOGGLE: case T_CHECK: case T_LIST: case T_DROPDOWN: case T_ROLLER:
    case T_SPINBOX: case T_TABVIEW: case T_INPUT: return true;
    case T_TEXTAREA: case T_TABLE: case T_ARC: case T_CUSTOM: return (_w[id].flags & F_INTERACT) != 0;
    default: return false;
  }
}
void WraithUI::setFocus(uint8_t id, bool announce) {
  if (id == _focus) return;
  uint8_t old = _focus; _focus = id; _dirty = true;
  if (announce && old != NONE && alive(old)) fire(old, EV_DEFOCUS, 0);
  if (id != NONE) { ensureVisible(id); if (announce) fire(id, EV_FOCUS, 0); }
  if (_theme == THEME_HAUNTED && announce) _glitchUntil = _now + 140;
}
void WraithUI::focus(uint8_t id) { if (focusable(id)) setFocus(id); }
void WraithUI::ensureFocus() {
  if (_focus != NONE && focusable(_focus)) return;
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) if (focusable(i)) { setFocus(i, false); return; }
  _focus = NONE;
}
void WraithUI::moveFocus(int dir) {
  uint8_t cur = _focus == NONE ? (dir > 0 ? WRAITH_MAX_WIDGETS - 1 : 0) : _focus;
  for (uint8_t k = 1; k <= WRAITH_MAX_WIDGETS; k++) {
    int i = ((int)cur + dir * k) % WRAITH_MAX_WIDGETS; if (i < 0) i += WRAITH_MAX_WIDGETS;
    if (focusable(i)) { setFocus(i); return; }
  }
}
void WraithUI::fire(uint8_t id, uint8_t ev, int16_t v) { if (alive(id) && _w[id].cb) _w[id].cb(id, ev, v, _w[id].user); }

// ================= layout and scrolling =================
int16_t WraithUI::hdrH(const W& w) { useFont(w); return _lcd.fontHeight() + 4; }
void WraithUI::innerRect(const W& w, int16_t* ix, int16_t* iy, int16_t* iw, int16_t* ih) const {
  if (w.type == T_TABVIEW) {
    int16_t hh = const_cast<WraithUI*>(this)->hdrH(w);
    *ix = 1; *iy = hh; *iw = w.w - 2; *ih = w.h - hh - 1;
  } else {
    int16_t b = (w.style && w.style->border >= 0) ? w.style->border : 1;
    int16_t p = (w.style && w.style->pad >= 0) ? w.style->pad : w.pad;
    *ix = *iy = b + p; *iw = w.w - 2 * (b + p); *ih = w.h - 2 * (b + p);
  }
}
void WraithUI::clampScroll(W& w) {
  if (!(w.flags & F_SCROLL)) { w.sx = w.sy = 0; return; }
  int16_t ix, iy, iw, ih; innerRect(w, &ix, &iy, &iw, &ih);
  int16_t mx = imax(0, w.cw - iw), my = imax(0, w.ch - ih);
  if (w.sx > mx) w.sx = mx;
  if (w.sx < 0) w.sx = 0;
  if (w.sy > my) w.sy = my;
  if (w.sy < 0) w.sy = 0;
}
int16_t WraithUI::naturalW(W& w) {
  if (w.w > 0 && !(w.autosz & 1)) return w.w;
  useFont(w);
  switch (w.type) {
    case T_LABEL: return _lcd.textWidth(text(&w - _w));
    case T_BUTTON: return _lcd.textWidth(text(&w - _w)) + 10;
    case T_CHECK: return _lcd.textWidth(text(&w - _w)) + 14;
    case T_TOGGLE: return 22;
    default: return w.w > 0 ? w.w : 20;
  }
}
int16_t WraithUI::naturalH(W& w, int16_t cw) {
  if (w.h > 0 && !(w.autosz & 2)) return w.h;
  useFont(w);
  int16_t fh = _lcd.fontHeight();
  switch (w.type) {
    case T_LABEL: return (w.longMode == LONG_WRAP && cw > 0) ? (int16_t)(_lcd.measureWrapped(text(&w - _w), cw) * fh) : fh;
    case T_BUTTON: case T_INPUT: case T_DROPDOWN: case T_SPINBOX: return fh + 6;
    case T_CHECK: return imax(9, fh);
    case T_TOGGLE: return 11;
    default: return w.h > 0 ? w.h : fh;
  }
}
void WraithUI::contentExtents(uint8_t pid) {
  W& p = _w[pid]; int16_t cw = 0, ch = 0;
  for (uint8_t c = 0; c < WRAITH_MAX_WIDGETS; c++) {
    const W& k = _w[c]; if (k.type == T_FREE || k.parent != pid || !(k.flags & F_VISIBLE)) continue;
    if (k.type == T_GAUGE) continue;
    cw = imax(cw, k.x + k.w); ch = imax(ch, k.y + k.h);
  }
  p.cw = cw; p.ch = ch; clampScroll(p);
}
void WraithUI::layoutContainer(uint8_t pid) {
  W& p = _w[pid];
  int16_t ix, iy, iw, ih; innerRect(p, &ix, &iy, &iw, &ih);
  if (p.layout == LAYOUT_NONE) return;
  int groups = p.type == T_TABVIEW ? p.n : 0;
  for (int g = -1; g < groups; g++) {
    uint8_t sel = g < 0 ? NONE : (uint8_t)g;
    int16_t cx = 0, cy = 0, rowH = 0;
    for (uint8_t c = 0; c < WRAITH_MAX_WIDGETS; c++) {
      W& k = _w[c]; if (k.type == T_FREE || k.parent != pid || k.tab != sel || !(k.flags & F_VISIBLE)) continue;
      bool aw = k.autosz & 1, ah = k.autosz & 2;
      int16_t cw, chh;
      if (p.layout == LAYOUT_COLUMN) {
        cw = aw ? iw : k.w; chh = ah ? naturalH(k, cw) : k.h;
        int16_t x = p.align == CENTER ? (iw - cw) / 2 : p.align == RIGHT ? iw - cw : 0;
        k.x = x; k.y = cy; cy += chh + p.gap;
      } else {
        cw = aw ? naturalW(k) : k.w; chh = ah ? (k.type == T_PANEL ? ih : naturalH(k, cw)) : k.h;
        if (p.layout == LAYOUT_ROW_WRAP && cx > 0 && cx + cw > iw) { cx = 0; cy += rowH + p.gap; rowH = 0; }
        int16_t y = p.layout == LAYOUT_ROW_WRAP ? cy : (p.align == CENTER ? (ih - chh) / 2 : p.align == RIGHT ? ih - chh : 0);
        k.x = cx; k.y = y; cx += cw + p.gap; rowH = imax(rowH, chh);
      }
      k.w = cw; k.h = chh;
    }
  }
}
void WraithUI::layoutTree(uint8_t id) {
  W& w = _w[id];
  if (!isContainer(w)) return;
  layoutContainer(id);
  for (uint8_t c = 0; c < WRAITH_MAX_WIDGETS; c++) if (_w[c].type != T_FREE && _w[c].parent == id) layoutTree(c);
  contentExtents(id);
}
void WraithUI::layoutAll() {
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) {           // resolve automatic sizes of widgets that no layout will place
    W& k = _w[i]; if (k.type == T_FREE || !k.autosz) continue;
    if (k.autosz & 1) k.w = naturalW(k);
    if (k.autosz & 2) k.h = naturalH(k, k.w);
  }
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) if (_w[i].type != T_FREE && _w[i].parent == NONE) layoutTree(i);
  _lcd.clearUFonts();
  _layoutDirty = false;
}
void WraithUI::ensureVisible(uint8_t id) {
  if (_layoutDirty) layoutAll();
  uint8_t c = id;
  for (uint8_t depth = 0; depth < 6 && alive(c); depth++) {
    const W& k = _w[c]; uint8_t p = k.parent; if (p == NONE) break;
    W& P = _w[p];
    if (P.flags & F_SCROLL) {
      int16_t ix, iy, iw, ih; innerRect(P, &ix, &iy, &iw, &ih);
      int16_t ty = P.sy, tx = P.sx;
      if (k.y < P.sy) ty = k.y; else if (k.y + k.h > P.sy + ih) ty = k.y + k.h - ih;
      if (k.x < P.sx) tx = k.x; else if (k.x + k.w > P.sx + iw) tx = k.x + k.w - iw;
      if (ty != P.sy) animate(p, P_SCROLL_Y, ty, 140);
      if (tx != P.sx) animate(p, P_SCROLL_X, tx, 140);
    }
    c = p;
  }
}

// ================= animation, timers =================
int16_t WraithUI::getProp(const W& w, uint8_t p) const {
  switch (p) { case P_X: return w.x; case P_Y: return w.y; case P_W: return w.w; case P_H: return w.h;
               case P_VALUE: return w.val; case P_SCROLL_X: return w.sx; default: return w.sy; }
}
void WraithUI::setProp(uint8_t id, uint8_t p, int16_t v) {
  W& w = _w[id];
  switch (p) {
    case P_X: w.x = v; break; case P_Y: w.y = v; break;
    case P_W: w.w = v; w.autosz &= ~1; _layoutDirty = true; break;
    case P_H: w.h = v; w.autosz &= ~2; _layoutDirty = true; break;
    case P_VALUE: if (v < w.mn) v = w.mn;
                  if (v > w.mx) v = w.mx;
                  w.val = v; w.disp = (int32_t)v * 16; break;
    case P_SCROLL_X: w.sx = v; clampScroll(w); break;
    default: w.sy = v; clampScroll(w); break;
  }
  _dirty = true;
}
bool WraithUI::animate(uint8_t id, Prop p, int16_t to, uint16_t ms, CasioEase::Ease e) {
  if (!alive(id)) return false;
  if (ms == 0) { setProp(id, p, to); return true; }
  int slot = -1;
  for (int a = 0; a < 8; a++) { if (_an[a].id == id && _an[a].prop == p) { slot = a; break; } if (slot < 0 && _an[a].id == NONE) slot = a; }
  if (slot < 0) return false;
  Anim& n = _an[slot]; n.id = id; n.prop = p; n.ease = e; n.from = getProp(_w[id], p); n.to = to; n.t0 = _now; n.dur = ms;
  return true;
}
void WraithUI::stepAnims(uint32_t now) {
  for (int a = 0; a < 8; a++) {
    Anim& n = _an[a]; if (n.id == NONE) continue;
    if (!alive(n.id)) { n.id = NONE; continue; }
    float t = (float)(now - n.t0) / (float)(n.dur ? n.dur : 1);
    if (t >= 1.0f) { uint8_t id = n.id; setProp(id, n.prop, n.to); n.id = NONE; fire(id, EV_ANIM_DONE, n.to); }
    else setProp(n.id, n.prop, (int16_t)(n.from + (n.to - n.from) * CasioEase::apply((CasioEase::Ease)n.ease, t)));
  }
}
uint8_t WraithUI::timer(uint16_t period, TimerCb cb, void* user, bool repeat) {
  for (uint8_t i = 0; i < 4; i++) if (!_tm[i].cb) { _tm[i].cb = cb; _tm[i].user = user; _tm[i].period = period; _tm[i].repeat = repeat; _tm[i].last = _now; return i; }
  return NONE;
}
void WraithUI::stepTimers(uint32_t now) {
  for (uint8_t i = 0; i < 4; i++) {
    Tmr& t = _tm[i];
    if (t.cb && (uint32_t)(now - t.last) >= t.period) {
      TimerCb cb = t.cb; void* u = t.user; t.last = now;
      if (!t.repeat) t.cb = nullptr;
      cb(u);
    }
  }
}

// ================= screens =================
void WraithUI::switchScreen(uint8_t s) {
  _prev = _screen; _screen = s; _focus = NONE; _flashId = NONE; _pressed = NONE; _ddOpen = NONE;
  _dirty = _layoutDirty = true;
  ensureFocus();
  if (_loadHandler) _loadHandler(s);
}
void WraithUI::show(uint8_t s, Trans t, uint16_t ms) {
  if (s == _screen && t == TR_NONE) return;
  if (t == TR_CRT) { _lcd.fxCrtOff(ms); switchScreen(s); return; }     // blocking: the old picture collapses, then the new screen appears
  if (t >= TR_SLIDE_LEFT) {                                           // snapshot both screens, then slide them in update()
    draw(); _lcd.readFrame2bpp(_snapOld);
    switchScreen(s); draw(); _lcd.readFrame2bpp(_snapNew);
    _tr = t; _trStart = _last; _trDur = ms; _trSwitched = true;
    return;
  }
  if (t == TR_MELT) { _tr = t; _trNew = s; _trStart = _last; _trDur = ms; _trSwitched = false; return; }
  if (t != TR_NONE) { _tr = t; _trStart = _last; _trDur = ms; }
  switchScreen(s);
}
void WraithUI::push(uint8_t s, Trans t, uint16_t ms) { if (_sp < 6) _stack[_sp++] = _screen; show(s, t, ms); }
bool WraithUI::pop(Trans t, uint16_t ms) { if (!_sp) return false; show(_stack[--_sp], t, ms); return true; }

// ================= popups =================
void WraithUI::msgbox(const char* title, const char* text, const char* b1, const char* b2, const char* b3, MsgCb cb, void* user) {
  Msg& m = _mb; m = Msg();
  copyUtf8(m.title, sizeof(m.title), title ? title : ""); copyUtf8(m.text, sizeof(m.text), text ? text : "");
  const char* bs[3] = {b1, b2, b3};
  for (int i = 0; i < 3; i++) if (bs[i]) { copyUtf8(m.b[m.nb], sizeof(m.b[0]), bs[i]); m.nb++; }
  if (!m.nb) { copyUtf8(m.b[0], sizeof(m.b[0]), "OK"); m.nb = 1; }
  m.cb = cb; m.user = user; m.active = true; closeDropdown(); _dirty = true;
}
void WraithUI::closeMsg(int8_t result) {
  MsgCb cb = _mb.cb; void* u = _mb.user; _mb.active = false; _dirty = true;
  if (cb) cb(result, u);
}
void WraithUI::mbKey(Key k) {
  _dirty = true;
  if (k == KEY_LEFT || k == KEY_UP) { if (_mb.sel > 0) _mb.sel--; }
  else if (k == KEY_RIGHT || k == KEY_DOWN) { if (_mb.sel < (int8_t)_mb.nb - 1) _mb.sel++; }
  else if (k == KEY_OK) closeMsg(_mb.sel);
  else if (k == KEY_BACK) closeMsg(-1);
}
void WraithUI::toast(const char* text, uint16_t ms) {
  copyUtf8(_toast.text, sizeof(_toast.text), text ? text : ""); _toast.t0 = _now; _toast.until = _now + ms; _toast.active = true; _dirty = true;
}
void WraithUI::openDropdown(uint8_t id) { _ddOpen = id; _ddSel = _w[id].val; fire(id, EV_OPEN, 0); _dirty = true; }
void WraithUI::closeDropdown() { if (_ddOpen != NONE) { uint8_t id = _ddOpen; _ddOpen = NONE; fire(id, EV_CLOSE, 0); _dirty = true; } }
void WraithUI::ddKey(Key k) {
  W& d = _w[_ddOpen]; _dirty = true;
  if (k == KEY_UP) { if (_ddSel > 0) _ddSel--; }
  else if (k == KEY_DOWN) { if (_ddSel < (int16_t)d.n - 1) _ddSel++; }
  else if (k == KEY_OK) { uint8_t id = _ddOpen; _w[id].val = _ddSel; closeDropdown(); emitValue(id, _w[id].val); }
  else if (k == KEY_BACK) closeDropdown();
}

// ================= input =================
bool WraithUI::repeatable(uint8_t t) {
  return t == T_SLIDER || t == T_ARC || t == T_SPINBOX || t == T_ROLLER || t == T_LIST || t == T_TEXTAREA || t == T_TABLE;
}
void WraithUI::listKeepVisible(W& w) {
  useFont(w);
  int16_t rowH = _lcd.fontHeight() + 1, rows = (w.h - 2) / rowH; if (rows < 1) rows = 1;
  if (w.val < w.top) w.top = w.val;
  if (w.val >= w.top + rows) w.top = w.val - rows + 1;
  _lcd.clearUFonts();
}
int16_t WraithUI::textLines(W& w) { useFont(w); int16_t n = _lcd.measureWrapped(text(&w - _w), w.w - 5); return n; }

void WraithUI::handleKey(Key k) {
  _dirty = true;
  W* f = (_focus != NONE && alive(_focus)) ? &_w[_focus] : nullptr;
  int d = (k == KEY_UP || k == KEY_LEFT) ? -1 : 1;
  if (!f) { if (k != KEY_OK && k != KEY_BACK) moveFocus(d); return; }
  uint8_t id = _focus;
  bool vert = (k == KEY_UP || k == KEY_DOWN);
  switch (f->type) {
    case T_LIST: case T_TABLE:
      if (vert) {
        int16_t maxv = (int16_t)f->n - 1 - ((f->type == T_TABLE && (f->flags & F_HEADER)) ? 1 : 0);
        if (f->val + d >= 0 && f->val + d <= maxv) { f->val += d; if (f->type == T_LIST) listKeepVisible(*f); } else moveFocus(d);
      } else moveFocus(d);
      return;
    case T_ROLLER:
      if (vert) { int16_t v = f->val + d; if (v >= 0 && v < f->n) { f->val = v; emitValue(id, v); } else moveFocus(d); } else moveFocus(d);
      return;
    case T_TEXTAREA:
      if (vert) {
        int16_t lines = textLines(*f); useFont(*f); int16_t vis = (f->h - 2) / _lcd.fontHeight(); _lcd.clearUFonts();
        int16_t maxTop = imax(0, lines - vis);
        if (d < 0 && f->top > 0) { f->top--; f->flags &= ~F_FOLLOW; }
        else if (d > 0 && f->top < maxTop) { f->top++; if (f->top >= maxTop) f->flags |= F_FOLLOW; }
        else moveFocus(d);
      } else moveFocus(d);
      return;
    case T_SLIDER: case T_ARC: case T_SPINBOX:
      if (!vert) {
        int32_t v = f->val + d * f->step; if (v < f->mn) v = f->mn; if (v > f->mx) v = f->mx;
        if (v != f->val) { f->val = v; emitValue(id, v); }
      } else moveFocus(d);
      return;
    case T_TOGGLE: case T_CHECK:
      if (!vert) { f->val = !f->val; emitValue(id, f->val); } else moveFocus(d);
      return;
    case T_TABVIEW:
      if (!vert) { int16_t v = f->val + d; if (v >= 0 && v < f->n) { f->val = v; _layoutDirty = true; ensureFocus(); emitValue(id, v); } } else moveFocus(d);
      return;
    case T_INPUT:
      if (!vert) {
        int16_t c = f->val;
        if (d < 0) { if (c > 0) { c--; while (c > 0 && ((uint8_t)f->txt[c] & 0xC0) == 0x80) c--; } }
        else if (f->txt[c]) { c++; while (f->txt[c] && ((uint8_t)f->txt[c] & 0xC0) == 0x80) c++; }
        f->val = c;
      } else moveFocus(d);
      return;
    default: moveFocus(d); return;
  }
}

void WraithUI::activate(uint8_t id) {
  W& w = _w[id];
  switch (w.type) {
    case T_BUTTON: _flashId = id; _flashUntil = _now + 120; emitClick(id, 1); break;
    case T_TOGGLE: case T_CHECK: w.val = !w.val; emitValue(id, w.val); break;
    case T_LIST: case T_ROLLER: case T_TABLE: emitClick(id, w.val); break;
    case T_DROPDOWN: openDropdown(id); break;
    case T_INPUT: case T_SPINBOX: case T_CUSTOM: case T_ARC: emitClick(id, w.val); break;
    default: break;
  }
  if (_theme == THEME_HAUNTED) _glitchUntil = _now + 140;
}

void WraithUI::doBack() {
  if (_mb.active) { closeMsg(-1); return; }
  if (_ddOpen != NONE) { closeDropdown(); return; }
  if (_backHandler) { _backHandler(); return; }
  if (!pop()) show(_prev);
}

void WraithUI::keyDown(Key k) {
  _dirty = true;
  if (_mb.active) { mbKey(k); return; }
  if (_ddOpen != NONE) { ddKey(k); return; }
  if (k == KEY_BACK) { doBack(); return; }
  if (k == KEY_OK) { _pressed = _focus; _okSince = _now; _longFired = false; return; }
  _held = k; _heldSince = _now; _lastRep = _now;
  handleKey(k);
}
void WraithUI::keyUp(Key k) {
  if (k == KEY_OK) {
    if (_pressed != NONE && !_longFired && _pressed == _focus && alive(_focus)) activate(_focus);
    _pressed = NONE; _dirty = true;
  } else if ((int8_t)k == _held) _held = -1;
}
bool WraithUI::typeChar(const char* s) {
  if (!s || !*s || _focus == NONE || !alive(_focus) || _w[_focus].type != T_INPUT) return false;
  W& w = _w[_focus]; size_t n = strlen(s), len = strlen(w.txt);
  if (len + n >= w.cap) return false;
  memmove(w.txt + w.val + n, w.txt + w.val, len - w.val + 1); memcpy(w.txt + w.val, s, n); w.val += n;
  _dirty = true; fire(_focus, EV_CHAR, (int16_t)(uint8_t)s[0]); emitValue(_focus, (int16_t)(len + n));
  return true;
}
bool WraithUI::backspace() {
  if (_focus == NONE || !alive(_focus) || _w[_focus].type != T_INPUT || _w[_focus].val == 0) return false;
  W& w = _w[_focus]; int c = w.val, e = c; c--;
  while (c > 0 && ((uint8_t)w.txt[c] & 0xC0) == 0x80) c--;
  memmove(w.txt + c, w.txt + e, strlen(w.txt + e) + 1); w.val = c;
  _dirty = true; emitValue(_focus, (int16_t)strlen(w.txt));
  return true;
}

// ================= frame loop =================
void WraithUI::draw() {
  if (_layoutDirty) layoutAll();
  ensureFocus();
  _needAnim = false;
  _lcd.clearClipRect();
  _lcd.clear(_c.bg);
  Rect full = {0, 0, (int16_t)(_lcd.width() - 1), (int16_t)(_lcd.height() - 1)};
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++)
    if (_w[i].type != T_FREE && _w[i].parent == NONE && _w[i].screen == _screen) drawTree(i, 0, 0, full);
  _curClip = full; setClip(full);
  drawPopups();
  _lcd.clearClipRect(); _lcd.clearUFonts(); _lcd.setTextColor(3); _lcd.setTextSize(1);
  if (_theme == THEME_HAUNTED && _now < _glitchUntil) { _lcd.fxGlitch(90); _lcd.fxNoise(14, 2); _needAnim = true; }
}

bool WraithUI::update(uint32_t now) {
  uint32_t dt = _last ? now - _last : 0;
  if (dt > 100) dt = 100;
  _last = now; _now = now;
  stepTimers(now); stepAnims(now);
  bool anim = false;
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) {            // tween bars, sliders, gauges, arcs, meters
    W& w = _w[i];
    if (w.type != T_BAR && w.type != T_GAUGE && w.type != T_SLIDER && w.type != T_ARC && w.type != T_METER) continue;
    int32_t tgt = (int32_t)w.val * 16, diff = tgt - w.disp;
    if (!diff) continue;
    int32_t step = diff * (int32_t)(dt ? dt : 1) / 110;
    if (!step) step = diff > 0 ? 1 : -1;                         // never stall on rounding
    w.disp += step;
    if (abs(tgt - w.disp) < 8 || w.type == T_SLIDER) w.disp = tgt;
    anim = true;
  }
  if (_flashId != NONE) { if ((int32_t)(now - _flashUntil) >= 0) _flashId = NONE; anim = true; }
  if (_pressed != NONE && !_longFired && _pressed == _focus && now - _okSince >= 600) {   // long press
    _longFired = true; fire(_pressed, EV_LONG, 0); _dirty = true;
  }
  if (_held >= 0 && _focus != NONE && alive(_focus) && repeatable(_w[_focus].type) && now - _heldSince >= 400 && now - _lastRep >= 90) {
    _lastRep = now; handleKey((Key)_held);
  }
  if (_toast.active) { if ((int32_t)(now - _toast.until) >= 0) { _toast.active = false; _dirty = true; } else anim = true; }
  if (_theme == THEME_HAUNTED && now < _glitchUntil) anim = true;

  if (_tr != TR_NONE) {
    if (!_trStart) _trStart = now;
    float p = (float)(now - _trStart) / (float)(_trDur ? _trDur : 1);
    if (p >= 1.0f) {
      if (_tr == TR_MELT && !_trSwitched) switchScreen(_trNew);
      _tr = TR_NONE;
      draw(); _lcd.display(); _dirty = false; _lastDraw = now; return true;
    }
    float q = 1.0f - p;
    if (_tr == TR_GLITCH)      { draw(); _lcd.fxGlitch((uint8_t)(255 * q)); _lcd.fxNoise((uint8_t)(90 * q), 2); }
    else if (_tr == TR_STATIC) { draw(); _lcd.fxNoise((uint8_t)(255 * q), 3); }
    else if (_tr >= TR_SLIDE_LEFT) {
      float e = CasioEase::apply(CasioEase::OUT_CUBIC, p);
      int16_t W = _lcd.width(), H = _lcd.height();
      int16_t dx = (_tr == TR_SLIDE_LEFT) ? -(int16_t)(e * W) : (_tr == TR_SLIDE_RIGHT) ? (int16_t)(e * W) : 0;
      int16_t dy = (_tr == TR_SLIDE_UP) ? -(int16_t)(e * H) : (_tr == TR_SLIDE_DOWN) ? (int16_t)(e * H) : 0;
      int16_t nx = dx + ((_tr == TR_SLIDE_LEFT) ? W : (_tr == TR_SLIDE_RIGHT) ? -W : 0);
      int16_t ny = dy + ((_tr == TR_SLIDE_UP) ? H : (_tr == TR_SLIDE_DOWN) ? -H : 0);
      _lcd.clear();
      _lcd.drawBitmap2bpp(dx, dy, _snapOld, W, H);
      _lcd.drawBitmap2bpp(nx, ny, _snapNew, W, H);
    }
    else if (_tr == TR_MELT) {
      if (p < 0.5f) _lcd.fxMelt(3);                             // old picture melts away
      else {
        if (!_trSwitched) { switchScreen(_trNew); _trSwitched = true; }
        draw(); _lcd.fxNoise((uint8_t)(255 * (1.0f - (p - 0.5f) * 2)), 3);
      }
    }
    _lcd.display();
    return true;
  }
  if (!_dirty && !anim) {
    if (!(_needAnim && now - _lastDraw >= 33)) return false;     // spinner / caret / marquee: ~30 fps
  }
  draw();
  _lcd.display();
  _dirty = false; _lastDraw = now;
  return true;
}
