// Tests for CasioTween, readFrame2bpp, CasioConsole and WraithUI slide transitions
#include <string>
#include <Arduino.h>
#include <SPI.h>
#include "WraithUI.h"
#include "CasioConsole.h"
#include "CasioTween.h"
#include "CasioFont_VI.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static bool validUtf8(const char* s) {
  while (*s) { uint8_t c = *s++; int n = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
    if (n < 0) return false; while (n--) { if ((*s & 0xC0) != 0x80) return false; s++; } }
  return true;
}
int main() {
  // ---- easing / tween ----
  using namespace CasioEase;
  for (int e = 0; e <= OUT_ELASTIC; e++) { CHECK(apply((Ease)e, 0) == 0); CHECK(fabsf(apply((Ease)e, 1) - 1) < 1e-6f); }
  for (Ease e : {LINEAR, IN_QUAD, OUT_QUAD, INOUT_QUAD, IN_CUBIC, OUT_CUBIC, INOUT_CUBIC}) { float p = 0; bool mono = true; for (int i = 1; i <= 100; i++) { float v = apply(e, i / 100.0f); if (v < p - 1e-6f) mono = false; p = v; } CHECK(mono); }
  { float mx = 0; for (int i = 0; i <= 100; i++) mx = fmaxf(mx, apply(OUT_BACK, i / 100.0f)); CHECK(mx > 1.05f); }
  { float mn = 9, mx = -9; for (int i = 0; i <= 100; i++) { float v = apply(OUT_BOUNCE, i / 100.0f); mn = fminf(mn, v); mx = fmaxf(mx, v); } CHECK(mn >= 0 && mx <= 1.0001f); }
  CHECK(fabsf(apply(INOUT_QUAD, 0.5f) - 0.5f) < 1e-6f && apply(IN_QUAD, 0.5f) < 0.5f && apply(OUT_QUAD, 0.5f) > 0.5f);
  { CasioTween t; t.start(10, 110, 1000, 400, LINEAR); CHECK(t.value(1000) == 10); CHECK(fabsf(t.value(1200) - 60) < 1e-3f); CHECK(t.value(1400) == 110 && t.done(1400) && !t.done(1399)); CHECK(t.value(4000000000u) == 110);
    CasioTween z; z.start(5, 7, 0, 0); CHECK(z.value(0) == 7); }
  // ---- readFrame2bpp round trip, every rotation ----
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  static uint8_t buf[3072], snap[3072];
  for (int r = 0; r < 4; r++) {
    lcd.setRotation(r); lcd.clear(); srand(r); for (int i = 0; i < 800; i++) lcd.drawPixel(rand() % lcd.width(), rand() % lcd.height(), rand() & 3);
    size_t n = lcd.readFrame2bpp(buf); CHECK(n == (size_t)((lcd.width() + 3) / 4) * lcd.height());
    memcpy(snap, lcd.frameBuffer(), 0); static uint8_t fbCopy[3072]; memcpy(fbCopy, lcd.frameBuffer(), 3072);
    lcd.clear(); lcd.drawBitmap2bpp(0, 0, buf, lcd.width(), lcd.height()); CHECK(memcmp(fbCopy, lcd.frameBuffer(), 3072) == 0);
  }
  lcd.setRotation(0); CHECK(lcd.readFrame2bpp(buf) == 3024);
  // ---- console ----
  lcd.clear(); lcd.fillRect(0, 0, 192, 63, 3);
  CasioConsole con(lcd, 10, 10, 60, 24);
  con.print("abcdefghijklmnopqrstuvwxyz0123");
  CHECK(con.lineCount() == 3); CHECK(strcmp(con.line(0), "abcdefghij") == 0 && strcmp(con.line(2), "uvwxyz0123") == 0);
  con.println(); con.println("hi"); CHECK(con.lineCount() == 5 && strcmp(con.line(3), "") != 0);
  con.clear(); CHECK(con.lineCount() == 1 && con.line(0)[0] == 0);
  for (int i = 0; i < 40; i++) { con.print("L"); con.println(i); }
  CHECK(con.lineCount() == CasioConsole::MAX_LINES);
  CHECK(strcmp(con.line(0), "L17") == 0 && strcmp(con.line(22), "L39") == 0 && con.line(23)[0] == 0);
  con.draw();
  bool outside = true; for (int y = 0; y < 63; y++) for (int x = 0; x < 192; x++) { bool in = x >= 10 && x < 70 && y >= 10 && y < 34; if (!in && lcd.getPixel(x, y) != 3) outside = false; }
  CHECK(outside);
  int ink = 0; for (int y = 10; y < 34; y++) for (int x = 10; x < 70; x++) if (lcd.getPixel(x, y) == 3) ink++; CHECK(ink > 50 && ink < 60 * 24 - 100);
  { con.scrollBack(5); con.draw(); static uint8_t a[3072], b[3072]; lcd.readFrame2bpp(a); con.scrollBack(-100); con.draw(); lcd.readFrame2bpp(b); CHECK(memcmp(a, b, 3024) != 0); con.scrollBack(1000); con.draw(); }
  con.print("x"); CHECK(true);                                  // new output snaps back to the bottom
  // UTF-8: never split a character, even when wrapping with a Unicode font
  lcd.addUFont(&UFONT_VI); con.clear();
  CasioConsole c2(lcd, 0, 0, 40, 32);
  c2.print("Việt Nam ấ ệ ử ơ ư Xin chào các bạn"); 
  bool ok = c2.lineCount() > 3; for (int i = 0; i < c2.lineCount(); i++) { ok = ok && validUtf8(c2.line(i)) && lcd.textWidth(c2.line(i)) <= 40; } CHECK(ok);
  { std::string all; for (int i = 0; i < c2.lineCount(); i++) all += c2.line(i); CHECK(all == "Việt Nam ấ ệ ử ơ ư Xin chào các bạn"); }
  lcd.clearUFonts();
  // ---- slide transitions ----
  WraithUI ui(lcd);
  uint8_t b0 = ui.bar(0, 0, 0, 192, 20, 100); uint8_t l1 = ui.label(1, 20, 40, 100, "SECOND"); (void)b0; (void)l1;
  ui.update(1000);
  for (WraithUI::Trans tr : {WraithUI::TR_SLIDE_LEFT, WraithUI::TR_SLIDE_RIGHT, WraithUI::TR_SLIDE_UP, WraithUI::TR_SLIDE_DOWN}) {
    ui.show(0); ui.update(1000); ui.draw(); static uint8_t oldB[3072], newB[3072], mid[3072];
    lcd.readFrame2bpp(oldB);
    ui.show(1); ui.draw(); lcd.readFrame2bpp(newB); ui.show(0); ui.update(2000);       // back to 0 with a known timeline
    ui.update(3000);
    ui.show(1, tr, 400);                                                                // start = last update time (3000)
    ui.update(3200);                                                                    // p = 0.5
    lcd.readFrame2bpp(mid);
    float e = CasioEase::apply(CasioEase::OUT_CUBIC, 0.5f); int W = 192, H = 63;
    int dx = tr == WraithUI::TR_SLIDE_LEFT ? -(int)(e * W) : tr == WraithUI::TR_SLIDE_RIGHT ? (int)(e * W) : 0;
    int dy = tr == WraithUI::TR_SLIDE_UP ? -(int)(e * H) : tr == WraithUI::TR_SLIDE_DOWN ? (int)(e * H) : 0;
    int nx = dx + (tr == WraithUI::TR_SLIDE_LEFT ? W : tr == WraithUI::TR_SLIDE_RIGHT ? -W : 0);
    int ny = dy + (tr == WraithUI::TR_SLIDE_UP ? H : tr == WraithUI::TR_SLIDE_DOWN ? -H : 0);
    auto px = [&](const uint8_t* b, int x, int y) { return (b[y * 48 + (x >> 2)] >> (6 - 2 * (x & 3))) & 3; };
    int bad = 0;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
      int ox = x - dx, oy = y - dy, qx = x - nx, qy = y - ny, want = 0;
      if (ox >= 0 && ox < W && oy >= 0 && oy < H) want = px(oldB, ox, oy); else if (qx >= 0 && qx < W && qy >= 0 && qy < H) want = px(newB, qx, qy);
      if (px(mid, x, y) != want) bad++;
    }
    CHECK(bad == 0);
    ui.update(3500); ui.update(3600);                                                   // finished
    CHECK(ui.screen() == 1);
    static uint8_t fin[3072]; lcd.readFrame2bpp(fin); CHECK(memcmp(fin, newB, 3024) == 0);
  }
  printf(fails ? "EXTRA %d FAILED\n" : "EXTRA OK\n", fails); return fails;
}
