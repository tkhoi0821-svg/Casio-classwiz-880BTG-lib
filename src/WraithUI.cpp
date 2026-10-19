#include "WraithUI.h"
#include "CasioTween.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------- creation ----------------
uint8_t WraithUI::alloc(uint8_t type, uint8_t scr, int16_t x, int16_t y, int16_t w, int16_t h) {
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++)
    if (_w[i].type == T_FREE) {
      W& k = _w[i];
      k = W();
      k.type = type; k.screen = scr; k.x = x; k.y = y; k.w = w; k.h = h;
      _dirty = true;
      if (_focus == NONE && scr == _screen && focusable(k)) _focus = i;
      return i;
    }
  return NONE;
}
static void copyTxt(char* d, const char* s) { if (!s) s = ""; strncpy(d, s, 23); d[23] = 0; }

uint8_t WraithUI::label(uint8_t s, int16_t x, int16_t y, int16_t w, const char* t, Align a) {
  uint8_t i = alloc(T_LABEL, s, x, y, w, 0); if (i != NONE) { copyTxt(_w[i].txt, t); _w[i].align = a; } return i;
}
uint8_t WraithUI::button(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* t) {
  uint8_t i = alloc(T_BUTTON, s, x, y, w, h); if (i != NONE) copyTxt(_w[i].txt, t); return i;
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
uint8_t WraithUI::checkbox(uint8_t s, int16_t x, int16_t y, int16_t w, const char* t, bool c) {
  uint8_t i = alloc(T_CHECK, s, x, y, w, 9); if (i != NONE) { copyTxt(_w[i].txt, t); _w[i].val = c; } return i;
}
uint8_t WraithUI::list(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, const char* const* items, uint8_t n) {
  uint8_t i = alloc(T_LIST, s, x, y, w, h); if (i != NONE) { _w[i].items = items; _w[i].n = n; } return i;
}
uint8_t WraithUI::gauge(uint8_t s, int16_t cx, int16_t cy, int16_t r, int16_t mn, int16_t mx, int16_t v) {
  uint8_t i = alloc(T_GAUGE, s, cx, cy, r, 0);
  if (i != NONE) { _w[i].mn = mn; _w[i].mx = mx; _w[i].val = v; _w[i].disp = (int32_t)v * 16; } return i;
}
uint8_t WraithUI::chart(uint8_t s, int16_t x, int16_t y, int16_t w, int16_t h, int16_t* buf, uint16_t n, int16_t vmin, int16_t vmax) {
  uint8_t i = alloc(T_CHART, s, x, y, w, h);
  if (i != NONE) { _w[i].buf = buf; _w[i].bn = n; _w[i].mn = vmin; _w[i].mx = vmax; } return i;
}

// ---------------- state ----------------
void WraithUI::setText(uint8_t id, const char* t) {
  if (id < WRAITH_MAX_WIDGETS && _w[id].type != T_FREE) { copyTxt(_w[id].txt, t); _dirty = true; }
}
void WraithUI::setValue(uint8_t id, int16_t v) {
  if (id >= WRAITH_MAX_WIDGETS || _w[id].type == T_FREE) return;
  W& w = _w[id];
  if (v < w.mn) v = w.mn;
  if (v > w.mx) v = w.mx;
  w.val = v; _dirty = true;
}
int16_t WraithUI::value(uint8_t id) const { return id < WRAITH_MAX_WIDGETS ? _w[id].val : 0; }
void WraithUI::chartPush(uint8_t id, int16_t s) {
  if (id >= WRAITH_MAX_WIDGETS || _w[id].type != T_CHART || !_w[id].buf || !_w[id].bn) return;
  memmove(_w[id].buf, _w[id].buf + 1, (_w[id].bn - 1) * sizeof(int16_t));
  _w[id].buf[_w[id].bn - 1] = s; _dirty = true;
}
void WraithUI::setVisible(uint8_t id, bool v) {
  if (id < WRAITH_MAX_WIDGETS) { _w[id].visible = v; _dirty = true; if (!v && _focus == id) moveFocus(1); }
}
bool WraithUI::focusable(const W& w) const {
  return w.visible && (w.type == T_BUTTON || w.type == T_SLIDER || w.type == T_TOGGLE || w.type == T_CHECK || w.type == T_LIST);
}
void WraithUI::focus(uint8_t id) {
  if (id < WRAITH_MAX_WIDGETS && _w[id].screen == _screen && focusable(_w[id])) { _focus = id; _dirty = true; }
}
void WraithUI::moveFocus(int dir) {
  uint8_t cur = _focus == NONE ? (dir > 0 ? WRAITH_MAX_WIDGETS - 1 : 0) : _focus;
  for (uint8_t k = 1; k <= WRAITH_MAX_WIDGETS; k++) {
    int i = ((int)cur + dir * k) % WRAITH_MAX_WIDGETS; if (i < 0) i += WRAITH_MAX_WIDGETS;
    if (_w[i].screen == _screen && focusable(_w[i])) { _focus = i; _dirty = true; return; }
  }
}

// ---------------- screens ----------------
void WraithUI::switchScreen(uint8_t s) {
  _prev = _screen; _screen = s; _focus = NONE; _flashId = NONE;
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) if (_w[i].screen == s && focusable(_w[i])) { _focus = i; break; }
  _dirty = true;
}
void WraithUI::show(uint8_t s, Trans t, uint16_t ms) {
  if (s == _screen && t == TR_NONE) return;
  if (t == TR_CRT) {                       // blocking: the old picture collapses, then the new screen appears
    _lcd.fxCrtOff(ms); switchScreen(s); return;
  }
  if (t >= TR_SLIDE_LEFT) {                // snapshot both screens, then slide them in update()
    draw(); _lcd.readFrame2bpp(_snapOld);
    switchScreen(s); draw(); _lcd.readFrame2bpp(_snapNew);
    _tr = t; _trStart = _last; _trDur = ms; _trSwitched = true;
    return;
  }
  if (t == TR_MELT) { _tr = t; _trNew = s; _trStart = _last; _trDur = ms; _trSwitched = false; return; }
  if (t != TR_NONE) { _tr = t; _trStart = _last; _trDur = ms; }
  switchScreen(s);
}

// ---------------- input ----------------
void WraithUI::listKeepVisible(W& w) {
  int16_t rowH = _lcd.fontHeight() + 1, rows = (w.h - 2) / rowH; if (rows < 1) rows = 1;
  if (w.val < w.top) w.top = w.val;
  if (w.val >= w.top + rows) w.top = w.val - rows + 1;
}
void WraithUI::key(Key k) {
  W* f = _focus != NONE ? &_w[_focus] : nullptr;
  _dirty = true;
  switch (k) {
    case KEY_UP: case KEY_DOWN: {
      int d = k == KEY_UP ? -1 : 1;
      if (f && f->type == T_LIST && f->val + d >= 0 && f->val + d < f->n) { f->val += d; useFont(*f); listKeepVisible(*f); _lcd.clearUFonts(); }
      else moveFocus(d);
      break;
    }
    case KEY_LEFT: case KEY_RIGHT: {
      int d = k == KEY_LEFT ? -1 : 1;
      if (f && f->type == T_SLIDER) {
        int32_t v = f->val + d * f->step;
        if (v < f->mn) v = f->mn;
        if (v > f->mx) v = f->mx;
        if (v != f->val) { f->val = v; emit(_focus, v); }
      } else if (f && (f->type == T_TOGGLE || f->type == T_CHECK)) { f->val = !f->val; emit(_focus, f->val); }
      else moveFocus(d);
      break;
    }
    case KEY_OK:
      if (!f) break;
      if (f->type == T_BUTTON) { _flashId = _focus; _flashUntil = _last + 120; emit(_focus, 1); }
      else if (f->type == T_TOGGLE || f->type == T_CHECK) { f->val = !f->val; emit(_focus, f->val); }
      else if (f->type == T_LIST) emit(_focus, f->val);
      break;
    case KEY_BACK:
      if (_backHandler) _backHandler(); else show(_prev);
      break;
  }
}

// ---------------- drawing ----------------
void WraithUI::useFont(const W& w) {
  _lcd.clearUFonts();
  const UFont* f = w.fontSet ? w.font : _defFont;
  if (!f) return;
  _lcd.addUFont(f);
  for (uint8_t i = 0; i < _nFb; i++) if (_fb[i] != f) _lcd.addUFont(_fb[i]);
}
void WraithUI::textIn(const char* s, int16_t x, int16_t y, int16_t w, Align a) {
  int16_t tw = _lcd.textWidth(s), tx = x;
  if (a == CENTER) tx = x + (w - tw) / 2; else if (a == RIGHT) tx = x + w - tw;
  _lcd.setCursor(tx < 0 ? 0 : tx, y);
  _lcd.print(s);
}

void WraithUI::drawWidget(uint8_t id, bool foc) {
  W& w = _w[id];
  useFont(w);
  _lcd.setTextColor(3); _lcd.setTextSize(1);
  int16_t fh = _lcd.fontHeight();
  switch (w.type) {
    case T_LABEL: textIn(w.txt, w.x, w.y, w.w, (Align)w.align); break;
    case T_BUTTON: {
      bool inv = foc || id == _flashId;
      if (inv) { _lcd.fillRoundRect(w.x, w.y, w.w, w.h, 3, 3); _lcd.setTextColor(0); }
      else _lcd.drawRoundRect(w.x, w.y, w.w, w.h, 3, 3);
      if (id == _flashId && foc) _lcd.drawRoundRect(w.x + 1, w.y + 1, w.w - 2, w.h - 2, 2, 0);
      textIn(w.txt, w.x, w.y + (w.h - fh) / 2, w.w, CENTER);
      break;
    }
    case T_BAR: {
      _lcd.drawRect(w.x, w.y, w.w, w.h, 3);
      int32_t range = w.mx - w.mn; if (range <= 0) break;
      int32_t fill = ((w.disp - (int32_t)w.mn * 16) * (w.w - 4)) / (range * 16);
      if (fill < 0) fill = 0;
      if (fill > w.w - 4) fill = w.w - 4;
      _lcd.fillRect(w.x + 2, w.y + 2, fill, w.h - 4, 3);
      break;
    }
    case T_SLIDER: {
      int16_t my = w.y + w.h / 2, range = w.mx - w.mn; if (range <= 0) range = 1;
      int16_t kx = w.x + 4 + (int16_t)(((w.disp - (int32_t)w.mn * 16) * (w.w - 8)) / ((int32_t)range * 16));
      _lcd.fillRect(w.x, my - 1, w.w, 3, 1);
      _lcd.fillRect(w.x, my - 1, kx - w.x, 3, 3);
      _lcd.fillCircle(kx, my, 3, 3);
      if (foc) _lcd.drawCircle(kx, my, 5, 3);
      break;
    }
    case T_TOGGLE: {
      bool on = w.val;
      if (on) _lcd.fillRoundRect(w.x, w.y, w.w, w.h, 5, 3); else _lcd.drawRoundRect(w.x, w.y, w.w, w.h, 5, 3);
      _lcd.fillCircle(on ? w.x + w.w - 6 : w.x + 5, w.y + 5, 3, on ? 0 : 3);
      if (foc) _lcd.drawRoundRect(w.x - 2, w.y - 2, w.w + 4, w.h + 4, 6, 3);
      break;
    }
    case T_CHECK: {
      _lcd.drawRect(w.x, w.y, 9, 9, 3);
      if (w.val) { _lcd.drawLine(w.x + 2, w.y + 4, w.x + 3, w.y + 6, 3); _lcd.drawLine(w.x + 3, w.y + 6, w.x + 7, w.y + 2, 3); }
      textIn(w.txt, w.x + 13, w.y + (9 - fh) / 2, w.w - 13, LEFT);
      if (foc) _lcd.drawRect(w.x - 2, w.y - 2, w.w + 4, (fh > 9 ? fh : 9) + 4, 3);
      break;
    }
    case T_LIST: {
      int16_t rowH = fh + 1, rows = (w.h - 2) / rowH; if (rows < 1) rows = 1;
      _lcd.drawRect(w.x, w.y, w.w, w.h, foc ? 3 : 1);
      for (int16_t r = 0; r < rows && w.top + r < w.n; r++) {
        int16_t idx = w.top + r, ry = w.y + 1 + r * rowH;
        bool sel = idx == w.val;
        if (sel) { _lcd.fillRect(w.x + 1, ry, w.w - 4, rowH, foc ? 3 : 1); _lcd.setTextColor(foc ? 0 : 3); }
        else _lcd.setTextColor(3);
        _lcd.setCursor(w.x + 3, ry + 1); _lcd.print(w.items[idx]);
      }
      if (w.n > rows) {                                           // scrollbar
        int16_t th = (w.h - 2) * rows / w.n, ty = w.y + 1 + (w.h - 2 - th) * w.top / (w.n - rows);
        _lcd.fillRect(w.x + w.w - 3, ty, 2, th, 3);
      }
      break;
    }
    case T_GAUGE: {
      int16_t r = w.w, range = w.mx - w.mn; if (range <= 0) range = 1;
      int32_t pct = ((w.disp - (int32_t)w.mn * 16) * 100) / ((int32_t)range * 16);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      _lcd.fillArc(w.x, w.y, r - 6, r, 180, 360, 1);
      if (pct > 0) _lcd.fillArc(w.x, w.y, r - 6, r, 180, 180 + (int16_t)(pct * 180 / 100), 3);
      char b[16]; snprintf(b, sizeof(b), "%d", (int)((w.disp + 8) / 16));
      textIn(b, w.x - r, w.y - fh - 1, 2 * r, CENTER);
      break;
    }
    case T_CHART:
      if (w.buf) _lcd.drawGraph(w.x, w.y, w.w, w.h, w.buf, w.bn, w.mn, w.mx, 3);
      break;
  }
}

void WraithUI::draw() {
  _lcd.clear();
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++)
    if (_w[i].type != T_FREE && _w[i].screen == _screen && _w[i].visible) drawWidget(i, i == _focus);
  _lcd.clearUFonts();
  _lcd.setTextColor(3);
}

bool WraithUI::update(uint32_t now) {
  uint32_t dt = _last ? now - _last : 0;
  if (dt > 100) dt = 100;
  _last = now;
  bool anim = false;
  for (uint8_t i = 0; i < WRAITH_MAX_WIDGETS; i++) {            // tween bars, sliders and gauges
    W& w = _w[i];
    if (w.type != T_BAR && w.type != T_GAUGE && w.type != T_SLIDER) continue;
    int32_t tgt = (int32_t)w.val * 16, diff = tgt - w.disp;
    if (!diff) continue;
    int32_t step = diff * (int32_t)(dt ? dt : 1) / 110;
    if (!step) step = diff > 0 ? 1 : -1;                       // never stall on rounding
    w.disp += step;
    if (abs(tgt - w.disp) < 8 || w.type == T_SLIDER) w.disp = tgt;
    anim = true;
  }
  if (_flashId != NONE) { if ((int32_t)(now - _flashUntil) >= 0) { _flashId = NONE; } anim = true; }

  if (_tr != TR_NONE) {
    if (!_trStart) _trStart = now;
    float p = (float)(now - _trStart) / (float)(_trDur ? _trDur : 1);
    if (p >= 1.0f) {
      if (_tr == TR_MELT && !_trSwitched) switchScreen(_trNew);
      _tr = TR_NONE;
      draw(); _lcd.display(); _dirty = false; return true;
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
  if (!_dirty && !anim) return false;
  draw();
  _lcd.display();
  _dirty = false;
  return true;
}
