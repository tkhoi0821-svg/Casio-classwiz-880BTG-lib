// WraithUI 2: drawing of the widget tree, every widget type and the popup layer.
#include "WraithUI.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static inline int16_t imin(int16_t a, int16_t b) { return a < b ? a : b; }
static inline int16_t imax(int16_t a, int16_t b) { return a > b ? a : b; }
static inline const char* item(const void* p, int i) { return ((const char* const*)p)[i]; }

void WraithUI::useFont(const W& w) {
  _lcd.clearUFonts();
  const UFont* f = (w.flags & F_FONTSET) ? w.font : _defFont;
  if (!f) return;
  _lcd.addUFont(f);
  for (uint8_t i = 0; i < _nFb; i++) if (_fb[i] != f) _lcd.addUFont(_fb[i]);
}
void WraithUI::textIn(const char* s, int16_t x, int16_t y, int16_t w, Align a) {
  int16_t tw = _lcd.textWidth(s), tx = x;
  if (a == CENTER) tx = x + (w - tw) / 2; else if (a == RIGHT) tx = x + w - tw;
  _lcd.setCursor(tx, y);
  _lcd.print(s);
}
int16_t WraithUI::prefixWidth(const W& w, int bytes) {
  char tmp[160]; size_t n = bytes < 159 ? bytes : 159;
  memcpy(tmp, w.txt, n); tmp[n] = 0;
  return _lcd.textWidth(tmp);
}
WraithUI::Rect WraithUI::pushClip(int16_t x, int16_t y, int16_t w, int16_t h) {
  Rect old = _curClip, n = {imax(old.x0, x), imax(old.y0, y), imin(old.x1, (int16_t)(x + w - 1)), imin(old.y1, (int16_t)(y + h - 1))};
  _curClip = n; setClip(n);
  return old;
}

// ---------------- tree ----------------
void WraithUI::drawTree(uint8_t id, int16_t ox, int16_t oy, Rect clip) {
  W& w = _w[id];
  if (!(w.flags & F_VISIBLE)) return;
  int16_t ax = ox + w.x, ay = oy + w.y; w.ax = ax; w.ay = ay;
  if (w.type != T_GAUGE && (ax > clip.x1 || ay > clip.y1 || ax + w.w - 1 < clip.x0 || ay + w.h - 1 < clip.y0)) return;   // outside the clip
  _curClip = clip; setClip(clip);
  drawWidget(id, ax, ay, id == _focus);
  if (isContainer(w)) {
    int16_t ix, iy, iw, ih; innerRect(w, &ix, &iy, &iw, &ih);
    Rect inner = {imax(clip.x0, (int16_t)(ax + ix)), imax(clip.y0, (int16_t)(ay + iy)), imin(clip.x1, (int16_t)(ax + ix + iw - 1)), imin(clip.y1, (int16_t)(ay + iy + ih - 1))};
    int16_t cx = ax + ix - w.sx, cy = ay + iy - w.sy;
    for (uint8_t c = 0; c < WRAITH_MAX_WIDGETS; c++) {
      W& k = _w[c];
      if (k.type == T_FREE || k.parent != id) continue;
      if (w.type == T_TABVIEW && k.tab != NONE && k.tab != w.val) continue;
      drawTree(c, cx, cy, inner);
    }
    _curClip = clip; setClip(clip);
    drawScrollbars(w, ax, ay);
  }
}

void WraithUI::drawScrollbars(const W& w, int16_t ax, int16_t ay) {
  if (!(w.flags & F_SCROLL)) return;
  Colors c = colors(w);
  int16_t ix, iy, iw, ih; innerRect(w, &ix, &iy, &iw, &ih);
  if (w.ch > ih) {
    int16_t x = ax + w.w - 3, y0 = ay + iy, th = imax(4, (int16_t)((int32_t)ih * ih / w.ch)), ty = y0 + (int16_t)((int32_t)(ih - th) * w.sy / (w.ch - ih));
    _lcd.fillRect(x, y0, 1, ih, c.track); _lcd.fillRect(x - 1, ty, 3, th, c.accent);
  }
  if (w.cw > iw) {
    int16_t y = ay + w.h - 3, x0 = ax + ix, tw = imax(4, (int16_t)((int32_t)iw * iw / w.cw)), tx = x0 + (int16_t)((int32_t)(iw - tw) * w.sx / (w.cw - iw));
    _lcd.fillRect(x0, y, iw, 1, c.track); _lcd.fillRect(tx, y - 1, tw, 3, c.accent);
  }
}

// ---------------- widgets ----------------
void WraithUI::drawWidget(uint8_t id, int16_t ax, int16_t ay, bool foc) {
  W& w = _w[id];
  Colors c = colors(w);
  bool dis = !(w.flags & F_ENABLED), pressed = (id == _flashId) || (_pressed == id);
  uint8_t fg = dis ? c.dis : c.fg;
  useFont(w);
  _lcd.setTextSize(1); _lcd.setTextColor(fg); _lcd.setTextWrap(false);
  int16_t fh = _lcd.fontHeight();
  const char* t = text(id);

  switch (w.type) {
    case T_LABEL: {
      if (w.style && w.style->bg >= 0 && w.style->bg < 4) _lcd.fillRect(ax, ay, w.w, w.h, c.panel);
      int16_t tw = _lcd.textWidth(t);
      if (w.longMode == LONG_WRAP) _lcd.printWrapped(t, ax, ay, w.w, 0, (CasioST75256::TextAlign)w.align, false);
      else if (w.w <= 0 || tw <= w.w) textIn(t, ax, ay, w.w > 0 ? w.w : tw, (Align)w.align);
      else if (w.longMode == LONG_DOTS) {
        char tmp[100]; size_t n = strlen(t); if (n > 90) n = 90; memcpy(tmp, t, n); tmp[n] = 0;
        while (n > 0) {
          char t2[104]; memcpy(t2, tmp, n); memcpy(t2 + n, "...", 4);
          if (_lcd.textWidth(t2) <= w.w) { _lcd.setCursor(ax, ay); _lcd.print(t2); break; }
          n--; while (n > 0 && ((uint8_t)tmp[n] & 0xC0) == 0x80) n--;
        }
      } else if (w.longMode == LONG_SCROLL) {
        Rect old = pushClip(ax, ay, w.w, w.h);
        int16_t gap = 16, off = (int16_t)((_now / 35) % (uint32_t)(tw + gap));
        _lcd.setCursor(ax - off, ay); _lcd.print(t); _lcd.setCursor(ax - off + tw + gap, ay); _lcd.print(t);
        popClip(old); _needAnim = true;
      } else { Rect old = pushClip(ax, ay, w.w, w.h); _lcd.setCursor(ax, ay); _lcd.print(t); popClip(old); }
      break;
    }
    case T_BUTTON: {
      bool inv = (foc || pressed) && !dis;
      if (inv) { _lcd.fillRoundRect(ax, ay, w.w, w.h, c.radius, c.fbg); _lcd.setTextColor(c.ffg); }
      else { if (c.panel < 4) _lcd.fillRoundRect(ax, ay, w.w, w.h, c.radius, c.panel); _lcd.drawRoundRect(ax, ay, w.w, w.h, c.radius, dis ? c.dis : c.border); }
      if (pressed && foc && !dis) _lcd.drawRoundRect(ax + 1, ay + 1, w.w - 2, w.h - 2, imax(0, c.radius - 1), c.ffg == 0 ? 0 : c.ffg);
      textIn(t, ax, ay + (w.h - fh) / 2, w.w, (Align)w.align);
      break;
    }
    case T_BAR: {
      _lcd.drawRect(ax, ay, w.w, w.h, fg);
      int32_t range = w.mx - w.mn; if (range <= 0) break;
      int32_t pos = w.disp - (int32_t)w.mn * 16;
      if (w.flags & F_VERT) { int32_t fill = pos * (w.h - 4) / (range * 16); if (fill < 0) fill = 0; if (fill > w.h - 4) fill = w.h - 4; _lcd.fillRect(ax + 2, ay + w.h - 2 - fill, w.w - 4, fill, dis ? c.dis : c.accent); }
      else { int32_t fill = pos * (w.w - 4) / (range * 16); if (fill < 0) fill = 0; if (fill > w.w - 4) fill = w.w - 4; _lcd.fillRect(ax + 2, ay + 2, fill, w.h - 4, dis ? c.dis : c.accent); }
      break;
    }
    case T_SLIDER: {
      int32_t range = w.mx - w.mn; if (range <= 0) range = 1;
      int32_t pos = w.disp - (int32_t)w.mn * 16;
      uint8_t acc = dis ? c.dis : c.accent;
      if (w.flags & F_VERT) {
        int16_t mx = ax + w.w / 2, ky = ay + w.h - 4 - (int16_t)(pos * (w.h - 8) / (range * 16));
        _lcd.fillRect(mx - 1, ay, 3, w.h, c.track); _lcd.fillRect(mx - 1, ky, 3, ay + w.h - ky, acc);
        _lcd.fillCircle(mx, ky, 3, acc); if (foc) _lcd.drawCircle(mx, ky, 5, c.fg);
      } else {
        int16_t my = ay + w.h / 2, kx = ax + 4 + (int16_t)(pos * (w.w - 8) / (range * 16));
        _lcd.fillRect(ax, my - 1, w.w, 3, c.track); _lcd.fillRect(ax, my - 1, kx - ax, 3, acc);
        _lcd.fillCircle(kx, my, 3, acc); if (foc) _lcd.drawCircle(kx, my, 5, c.fg);
      }
      break;
    }
    case T_TOGGLE: {
      bool on = w.val;
      if (on) _lcd.fillRoundRect(ax, ay, w.w, w.h, 5, dis ? c.dis : c.accent); else _lcd.drawRoundRect(ax, ay, w.w, w.h, 5, fg);
      _lcd.fillCircle(on ? ax + w.w - 6 : ax + 5, ay + 5, 3, on ? c.bg : fg);
      if (foc) _lcd.drawRoundRect(ax - 2, ay - 2, w.w + 4, w.h + 4, 6, c.fg);
      break;
    }
    case T_CHECK: {
      _lcd.drawRect(ax, ay, 9, 9, fg);
      if (w.val) { _lcd.drawLine(ax + 2, ay + 4, ax + 3, ay + 6, fg); _lcd.drawLine(ax + 3, ay + 6, ax + 7, ay + 2, fg); }
      textIn(t, ax + 13, ay + (9 - fh) / 2, w.w - 13, LEFT);
      if (foc) _lcd.drawRect(ax - 2, ay - 2, w.w + 4, imax(fh, 9) + 4, c.fg);
      break;
    }
    case T_LIST: {
      int16_t rowH = fh + 1, rows = imax(1, (int16_t)((w.h - 2) / rowH));
      _lcd.drawRect(ax, ay, w.w, w.h, foc ? c.fg : c.track);
      Rect old = pushClip(ax + 1, ay + 1, w.w - 2, w.h - 2);
      for (int16_t r = 0; r < rows && w.top + r < w.n; r++) {
        int16_t idx = w.top + r, ry = ay + 1 + r * rowH;
        if (idx == w.val) { _lcd.fillRect(ax + 1, ry, w.w - 4, rowH, foc ? c.fbg : c.track); _lcd.setTextColor(foc ? c.ffg : c.fg); }
        else _lcd.setTextColor(fg);
        _lcd.setCursor(ax + 3, ry + 1); _lcd.print(item(w.p1, idx));
      }
      popClip(old);
      if (w.n > rows) { int16_t th = (w.h - 2) * rows / w.n, ty = ay + 1 + (w.h - 2 - th) * w.top / (w.n - rows); _lcd.fillRect(ax + w.w - 3, ty, 2, th, c.accent); }
      break;
    }
    case T_GAUGE: {                                           // legacy: ax, ay = centre of the half ring, w.w = radius
      int16_t r = w.w, range = w.mx - w.mn; if (range <= 0) range = 1;
      int32_t pct = ((w.disp - (int32_t)w.mn * 16) * 100) / ((int32_t)range * 16);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      _lcd.fillArc(ax, ay, r - 6, r, 180, 360, c.track);
      if (pct > 0) _lcd.fillArc(ax, ay, r - 6, r, 180, 180 + (int16_t)(pct * 180 / 100), c.accent);
      char b[16]; snprintf(b, sizeof(b), "%d", (int)((w.disp + 8) / 16));
      textIn(b, ax - r, ay - fh - 1, 2 * r, CENTER);
      break;
    }
    case T_CHART: if (w.p1) _lcd.drawGraph(ax, ay, w.w, w.h, (const int16_t*)w.p1, w.n16, w.mn, w.mx, c.fg); break;

    case T_PANEL: {
      int16_t b = (w.style && w.style->border >= 0) ? w.style->border : 1;
      if (c.panel < 4) { if (c.radius) _lcd.fillRoundRect(ax, ay, w.w, w.h, c.radius, c.panel); else _lcd.fillRect(ax, ay, w.w, w.h, c.panel); }
      for (int16_t i = 0; i < b; i++) { if (c.radius) _lcd.drawRoundRect(ax + i, ay + i, w.w - 2 * i, w.h - 2 * i, imax(0, c.radius - i), c.border); else _lcd.drawRect(ax + i, ay + i, w.w - 2 * i, w.h - 2 * i, c.border); }
      break;
    }
    case T_TEXTAREA: {
      _lcd.drawRect(ax, ay, w.w, w.h, foc ? c.fg : c.border); if (foc) _lcd.drawRect(ax + 1, ay + 1, w.w - 2, w.h - 2, c.fg);
      int16_t lines = textLines(w), vis = imax(1, (int16_t)((w.h - 2) / fh)), maxTop = imax(0, lines - vis);
      if ((w.flags & F_FOLLOW) || w.top > maxTop) w.top = (uint8_t)imin(maxTop, 255);
      Rect old = pushClip(ax + 2, ay + 1, w.w - 5, w.h - 2);
      _lcd.setTextColor(fg);
      _lcd.printWrapped(t, ax + 2, ay + 1 - w.top * fh, w.w - 5, 0, CasioST75256::ALIGN_LEFT, false);
      popClip(old);
      if (lines > vis) { int16_t th = imax(4, (int16_t)((w.h - 2) * vis / lines)), ty = ay + 1 + (w.h - 2 - th) * w.top / imax(1, maxTop); _lcd.fillRect(ax + w.w - 3, ty, 2, th, c.accent); }
      break;
    }
    case T_INPUT: {
      _lcd.drawRoundRect(ax, ay, w.w, w.h, imin(c.radius, 2), foc ? c.fg : c.border); if (foc) _lcd.drawRoundRect(ax + 1, ay + 1, w.w - 2, w.h - 2, 0, c.fg);
      int16_t inW = w.w - 6, cwid = prefixWidth(w, w.val), off = cwid > inW - 2 ? cwid - (inW - 2) : 0;
      Rect old = pushClip(ax + 2, ay + 2, w.w - 4, w.h - 4);
      _lcd.setCursor(ax + 3 - off, ay + (w.h - fh) / 2); _lcd.print(t);
      if (foc && ((_now / 500) & 1) == 0) _lcd.drawFastVLine(ax + 3 - off + cwid, ay + 3, w.h - 6, c.fg);
      popClip(old);
      if (foc) _needAnim = true;
      break;
    }
    case T_DROPDOWN: {
      bool open = _ddOpen == id;
      _lcd.drawRoundRect(ax, ay, w.w, w.h, imin(c.radius, 2), (foc || open) ? c.fg : c.border); if (foc || open) _lcd.drawRoundRect(ax + 1, ay + 1, w.w - 2, w.h - 2, 0, c.fg);
      Rect old = pushClip(ax + 2, ay + 2, w.w - 13, w.h - 4);
      _lcd.setCursor(ax + 4, ay + (w.h - fh) / 2); if (w.n) _lcd.print(item(w.p1, imin(w.val, (int16_t)(w.n - 1))));
      popClip(old);
      _lcd.fillTriangle(ax + w.w - 10, ay + w.h / 2 - 2, ax + w.w - 4, ay + w.h / 2 - 2, ax + w.w - 7, ay + w.h / 2 + 2, fg);
      break;
    }
    case T_ROLLER: {
      int16_t rowH = fh + 2, centerY = ay + w.h / 2 - rowH / 2, k = w.h / rowH / 2 + 1;
      _lcd.drawRect(ax, ay, w.w, w.h, foc ? c.fg : c.border);
      Rect old = pushClip(ax + 1, ay + 1, w.w - 2, w.h - 2);
      _lcd.fillRect(ax + 1, centerY, w.w - 2, rowH, foc ? c.fbg : c.track);
      for (int16_t r = -k; r <= k; r++) {
        int16_t idx = w.val + r; if (idx < 0 || idx >= w.n) continue;
        _lcd.setTextColor(r == 0 ? (foc ? c.ffg : c.fg) : (r == 1 || r == -1) ? c.fg : c.dis);
        const char* s = item(w.p1, idx); _lcd.setCursor(ax + (w.w - _lcd.textWidth(s)) / 2, centerY + r * rowH + 1); _lcd.print(s);
      }
      popClip(old);
      break;
    }
    case T_SPINBOX: {
      char b[12]; snprintf(b, sizeof(b), "%d", (int)w.val);
      uint8_t tc = fg;
      if (foc && !dis) { _lcd.fillRoundRect(ax, ay, w.w, w.h, imin(c.radius, 3), c.fbg); tc = c.ffg; } else _lcd.drawRoundRect(ax, ay, w.w, w.h, imin(c.radius, 3), dis ? c.dis : c.border);
      int16_t my = ay + w.h / 2;
      _lcd.fillTriangle(ax + 4, my, ax + 8, my - 3, ax + 8, my + 3, tc); _lcd.fillTriangle(ax + w.w - 5, my, ax + w.w - 9, my - 3, ax + w.w - 9, my + 3, tc);
      _lcd.setTextColor(tc); textIn(b, ax + 10, ay + (w.h - fh) / 2, w.w - 20, CENTER);
      break;
    }
    case T_TABVIEW: {
      int16_t hh = fh + 4, n = imax(1, (int16_t)w.n), tabW = w.w / n;
      for (int16_t i = 0; i < w.n; i++) {
        int16_t tx = ax + i * tabW; bool act = i == w.val; uint8_t tc = act ? fg : c.dis;
        if (act && foc && !dis) { _lcd.fillRect(tx, ay, tabW, hh - 1, c.fbg); tc = c.ffg; } else if (act) _lcd.fillRect(tx + 1, ay + hh - 3, tabW - 2, 2, c.accent);
        _lcd.setTextColor(tc); textIn(item(w.p1, i), tx, ay + (hh - fh) / 2, tabW, CENTER);
      }
      _lcd.drawRect(ax, ay + hh - 1, w.w, w.h - hh + 1, c.border);
      break;
    }
    case T_ARC: {
      int16_t r = w.w / 2, cx = ax + r, cy = ay + r, th = imax(3, (int16_t)(r / 4)), range = w.mx - w.mn; if (range <= 0) range = 1;
      int32_t pct = ((w.disp - (int32_t)w.mn * 16) * 100) / ((int32_t)range * 16);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      int16_t sweep = (int16_t)(pct * 270 / 100);
      _lcd.fillArc(cx, cy, r - th, r, 135, 405, c.track);
      if (sweep > 0) _lcd.fillArc(cx, cy, r - th, r, 135, 135 + sweep, dis ? c.dis : c.accent);
      if (w.flags & F_INTERACT) {
        float a = (135 + sweep) * 0.0174533f; int16_t kx = cx + (int16_t)lroundf((r - th / 2.0f) * cosf(a)), ky = cy + (int16_t)lroundf((r - th / 2.0f) * sinf(a));
        _lcd.fillCircle(kx, ky, th / 2 + 1, c.fg); if (foc) _lcd.drawCircle(kx, ky, th / 2 + 3, c.fg);
      }
      char b[12]; snprintf(b, sizeof(b), "%d", (int)((w.disp + 8) / 16)); textIn(b, ax, cy - fh / 2, w.w, CENTER);
      break;
    }
    case T_METER: {
      int16_t r = w.w / 2 - 1, cx = ax + w.w / 2, cy = ay + r + 1, range = w.mx - w.mn; if (range <= 0) range = 1;
      int32_t pct = ((w.disp - (int32_t)w.mn * 16) * 100) / ((int32_t)range * 16);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      _lcd.fillArc(cx, cy, r - 2, r, 180, 360, c.track);
      for (int i = 0; i < w.n; i++) {
        float a = (180.0f + i * 180.0f / (w.n - 1)) * 0.0174533f, ca = cosf(a), sa = sinf(a);
        _lcd.drawLine(cx + (int16_t)lroundf((r - 6) * ca), cy + (int16_t)lroundf((r - 6) * sa), cx + (int16_t)lroundf(r * ca), cy + (int16_t)lroundf(r * sa), fg);
      }
      float a = (180.0f + pct * 1.8f) * 0.0174533f;
      _lcd.drawThickLine(cx, cy, cx + (int16_t)lroundf((r - 9) * cosf(a)), cy + (int16_t)lroundf((r - 9) * sinf(a)), 2, dis ? c.dis : c.accent);
      _lcd.fillCircle(cx, cy, 2, fg);
      char b[12]; snprintf(b, sizeof(b), "%d", (int)((w.disp + 8) / 16)); textIn(b, ax, cy + 3, w.w, CENTER);
      break;
    }
    case T_LED: {
      int16_t r = w.w / 2; uint8_t L = w.val > 3 ? 3 : w.val;
      uint8_t on = L == 3 ? c.accent : L == 2 ? (c.accent == 3 ? 2 : 1) : c.track;
      if (L == 0) _lcd.drawCircle(ax + r, ay + r, r, fg); else { _lcd.fillCircle(ax + r, ay + r, r, on); _lcd.drawCircle(ax + r, ay + r, r, fg); }
      break;
    }
    case T_SPINNER: {
      int16_t r = w.w / 2, th = imax(2, (int16_t)(r / 3)); int16_t a = (int16_t)((_now / 3) % 360);
      _lcd.fillArc(ax + r, ay + r, r - th, r, 0, 359, c.track); _lcd.fillArc(ax + r, ay + r, r - th, r, a, a + 100, c.accent);
      _needAnim = true;
      break;
    }
    case T_IMAGE: if (w.p1) _lcd.drawBitmap2bpp(ax, ay, (const uint8_t*)w.p1, w.w, w.h, w.mn); break;
    case T_TABLE: {
      int16_t rowH = fh + 2, cols = w.cols ? w.cols : 1, cwid = w.w / cols;
      bool hdr = (w.flags & F_HEADER) != 0;
      int16_t dataRows = (int16_t)w.n - (hdr ? 1 : 0), vis = imax(1, (int16_t)((w.h - 2) / rowH - (hdr ? 1 : 0)));
      if (w.val < w.top) w.top = (uint8_t)w.val;
      if (w.val >= w.top + vis) w.top = (uint8_t)(w.val - vis + 1);
      _lcd.drawRect(ax, ay, w.w, w.h, foc ? c.fg : c.border);
      Rect old = pushClip(ax + 1, ay + 1, w.w - 2, w.h - 2);
      int16_t y = ay + 1;
      const char* const* cells = (const char* const*)w.p1;
      if (hdr) {
        _lcd.fillRect(ax + 1, y, w.w - 2, rowH, c.fg);
        for (int16_t cc = 0; cc < cols; cc++) { _lcd.setTextColor(c.bg); _lcd.setCursor(ax + 3 + cc * cwid, y + 1); _lcd.print(cells[cc]); }
        y += rowH;
      }
      for (int16_t r = 0; r < vis && w.top + r < dataRows; r++, y += rowH) {
        int16_t row = w.top + r + (hdr ? 1 : 0); bool sel = (w.top + r) == w.val;
        if (sel) _lcd.fillRect(ax + 1, y, w.w - 4, rowH, foc ? c.fbg : c.track);
        _lcd.setTextColor(sel && foc ? c.ffg : fg);
        for (int16_t cc = 0; cc < cols; cc++) {
          Rect o2 = pushClip(ax + 1 + cc * cwid, y, cwid - 1, rowH);
          _lcd.setCursor(ax + 3 + cc * cwid, y + 1); _lcd.print(cells[row * cols + cc]); popClip(o2);
        }
      }
      for (int16_t cc = 1; cc < cols; cc++) _lcd.drawFastVLine(ax + cc * cwid, ay + 1, w.h - 2, c.track);
      popClip(old);
      if (dataRows > vis) { int16_t th = imax(4, (int16_t)((w.h - 2) * vis / dataRows)), ty = ay + 1 + (w.h - 2 - th) * w.top / imax(1, dataRows - vis); _lcd.fillRect(ax + w.w - 3, ty, 2, th, c.accent); }
      break;
    }
    case T_KEYBOARD: {
      _lcd.clearUFonts();                                     // the keys always use the built-in 5x7 font
      int16_t cw = w.w / 10, chh = imax((int16_t)9, (int16_t)(w.h / 5)), sel = w.val;
      static const char* LAB[8] = {"", "", "#+=", "", "DEL", "OK", "<", ">"};
      for (int row = 0; row < 5; row++)
        for (int col = 0; col < 10; col++) {
          int s, e; kbRun(w.n, row, col, &s, &e);
          if (col != s) continue;                              // draw each (possibly wide) key once
          char code = kbCode(w.n, row, col); int16_t kx = ax + s * cw, ky = ay + row * chh, kw = (e - s + 1) * cw;
          bool cur = (sel / 10 == row) && (sel % 10 >= s && sel % 10 <= e), on = cur && foc && !dis;
          if (on) _lcd.fillRect(kx, ky, kw, chh, c.fbg); else _lcd.drawRect(kx, ky, kw + 1, chh + 1, cur ? c.fg : c.track);
          uint8_t tc = on ? c.ffg : fg; _lcd.setTextColor(tc);
          int16_t cx = kx + kw / 2, cy = ky + chh / 2;
          if ((uint8_t)code >= 32) { char s2[2] = {code, 0}; _lcd.setCursor(kx + (kw - 5) / 2 + 1, ky + (chh - 7) / 2 + 1); _lcd.print(s2); }
          else if (code == 1) { if (w.n == 1) _lcd.fillTriangle(cx - 3, cy + 2, cx + 3, cy + 2, cx, cy - 3, tc); else _lcd.drawTriangle(cx - 3, cy + 2, cx + 3, cy + 2, cx, cy - 3, tc); }
          else if (code == 3) _lcd.drawFastHLine(cx - kw / 2 + 4, cy + 1, kw - 8, tc);
          else if (code == 6) _lcd.fillTriangle(cx - 2, cy, cx + 2, cy - 3, cx + 2, cy + 3, tc);
          else if (code == 7) _lcd.fillTriangle(cx + 2, cy, cx - 2, cy - 3, cx - 2, cy + 3, tc);
          else { const char* t2 = (code == 2 && w.n == 2) ? "abc" : LAB[(uint8_t)code]; _lcd.setCursor(kx + (kw - (int16_t)(strlen(t2) * 6 - 1)) / 2, ky + (chh - 7) / 2 + 1); _lcd.print(t2); }
        }
      break;
    }
    case T_CUSTOM: {
      Rect old = pushClip(ax, ay, w.w, w.h);
      if (w.p1) ((DrawCb)w.p1)(*this, _lcd, id, ax, ay, w.w, w.h, foc, w.user);
      popClip(old);
      break;
    }
    default: break;
  }
}

// ---------------- popups ----------------
void WraithUI::dimScreen() {
  int16_t W = _lcd.width(), H = _lcd.height();
  for (int16_t y = 0; y < H; y++) for (int16_t x = (y & 1); x < W; x += 2) _lcd.drawPixel(x, y, _c.bg);
}
void WraithUI::drawPopups() {
  int16_t SW = _lcd.width(), H = _lcd.height();
  _lcd.setTextWrap(false);
  if (_ddOpen != NONE && alive(_ddOpen)) {                       // dropdown list
    W& d = _w[_ddOpen]; Colors c = colors(d); useFont(d);
    int16_t fh = _lcd.fontHeight(), rowH = fh + 2, vis = imin((int16_t)d.n, (int16_t)4), hh = vis * rowH + 2, py = d.ay + d.h;
    if (py + hh > H) py = d.ay - hh;
    if (_ddSel < d.top) d.top = (uint8_t)_ddSel;
    if (_ddSel >= d.top + vis) d.top = (uint8_t)(_ddSel - vis + 1);
    _lcd.fillRect(d.ax, py, d.w, hh, c.bg); _lcd.drawRect(d.ax, py, d.w, hh, c.fg);
    for (int16_t r = 0; r < vis && d.top + r < d.n; r++) {
      int16_t idx = d.top + r, ry = py + 1 + r * rowH;
      if (idx == _ddSel) { _lcd.fillRect(d.ax + 1, ry, d.w - 2, rowH, c.fbg); _lcd.setTextColor(c.ffg); } else _lcd.setTextColor(c.fg);
      _lcd.setCursor(d.ax + 3, ry + 1); _lcd.print(item(d.p1, idx));
    }
    if (d.n > vis) { int16_t th = (hh - 2) * vis / d.n, ty = py + 1 + (hh - 2 - th) * d.top / (d.n - vis); _lcd.fillRect(d.ax + d.w - 3, ty, 2, th, c.accent); }
  }
  if (_mb.active) {                                              // modal message box
    W tmp; useFont(tmp);
    int16_t fh = _lcd.fontHeight(), bw = imin((int16_t)(SW - 12), (int16_t)168), innerW = bw - 10, titleH = _mb.title[0] ? fh + 3 : 0, btnH = fh + 6;
    int16_t avail = H - 8 - titleH - btnH - 10, maxL = imax(1, (int16_t)(avail / fh));
    int16_t lines = imin(imin((int16_t)_lcd.measureWrapped(_mb.text, innerW), maxL), (int16_t)4);
    int16_t bh = 4 + titleH + lines * fh + 4 + btnH + 4, bx = (SW - bw) / 2, by = (H - bh) / 2;
    dimScreen();
    _lcd.fillRoundRect(bx, by, bw, bh, _c.radius, _c.bg); _lcd.drawRoundRect(bx, by, bw, bh, _c.radius, _c.fg);
    _lcd.setTextColor(_c.fg);
    if (_mb.title[0]) { _lcd.setCursor(bx + (bw - _lcd.textWidth(_mb.title)) / 2, by + 3); _lcd.print(_mb.title); _lcd.drawFastHLine(bx + 4, by + 3 + fh + 1, bw - 8, _c.fg); }
    _lcd.printWrapped(_mb.text, bx + 5, by + 4 + titleH, innerW, lines, CasioST75256::ALIGN_CENTER, true);
    int16_t nb = _mb.nb, bwid = (innerW - (nb - 1) * 4) / nb, bY = by + bh - btnH - 4;
    for (int i = 0; i < nb; i++) {
      int16_t x = bx + 5 + i * (bwid + 4); bool sel = i == _mb.sel;
      if (sel) { _lcd.fillRoundRect(x, bY, bwid, btnH, imin(_c.radius, 3), _c.fbg); _lcd.setTextColor(_c.ffg); } else { _lcd.drawRoundRect(x, bY, bwid, btnH, imin(_c.radius, 3), _c.fg); _lcd.setTextColor(_c.fg); }
      _lcd.setCursor(x + (bwid - _lcd.textWidth(_mb.b[i])) / 2, bY + 3); _lcd.print(_mb.b[i]);
    }
  }
  if (_toast.active) {                                           // toast
    W tmp; useFont(tmp);
    int16_t fh = _lcd.fontHeight(), tw = imin((int16_t)(_lcd.textWidth(_toast.text) + 10), (int16_t)(SW - 8)), th = fh + 6;
    uint32_t e = _now - _toast.t0, left = _toast.until > _now ? _toast.until - _now : 0;
    float k = 1.0f;
    if (e < 120) k = CasioEase::apply(CasioEase::OUT_CUBIC, e / 120.0f); else if (left < 150) k = CasioEase::apply(CasioEase::OUT_CUBIC, left / 150.0f);
    int16_t x = (SW - tw) / 2, y = H - th - 3 + (int16_t)((1.0f - k) * (th + 6));
    _lcd.fillRoundRect(x, y, tw, th, _c.radius, _c.fg); _lcd.setTextColor(_c.bg);
    Rect old = pushClip(x + 2, y, tw - 4, th); _lcd.setCursor(x + 5, y + 3); _lcd.print(_toast.text); popClip(old);
    _needAnim = true;
  }
  _lcd.setTextWrap(true);
}
