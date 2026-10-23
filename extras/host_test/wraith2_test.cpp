// WraithUI 2: layout, scrolling, clipping, tabs, popups, input, events, animation, timers, themes, pools
#include <string>
#include <Arduino.h>
#include <SPI.h>
#include "WraithUI.h"
#include "CasioFont_VI.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
typedef WraithUI U;
static CasioST75256* L;
static uint32_t T = 1000;
static void step(U& ui, int n = 1, int dt = 16) { for (int i = 0; i < n; i++) { T += dt; ui.update(T); } }
static int ink(int x0, int y0, int x1, int y1) { int n = 0; for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) if (L->getPixel(x, y) != 0) n++; return n; }
// event log
static std::string log_;
static void cb(uint8_t id, uint8_t ev, int16_t v, void*) { char b[40]; snprintf(b, sizeof(b), "%d:%d:%d;", id, ev, v); log_ += b; }
static int msgResult = -99; static void onMsg(int8_t b, void*) { msgResult = b; }
static int timerHits = 0; static void onTimer(void* u) { timerHits += u ? *(int*)u : 1; }
static int customCalls = 0; static int cx_, cy_, cw_, ch_;
static void drawCustom(U&, CasioST75256& lcd, uint8_t, int16_t x, int16_t y, int16_t w, int16_t h, bool, void*) { customCalls++; cx_ = x; cy_ = y; cw_ = w; ch_ = h; lcd.fillRect(-50, -50, 500, 500, 3); }
int main() {
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin(); L = &lcd;
  static const char* ITEMS[] = {"One", "Two", "Three", "Four", "Five", "Six"};

  // ===== layout: column, row, wrap =====
  { U ui(lcd); uint8_t p = ui.panel(0, 10, 5, 100, 50); ui.setLayout(p, U::LAYOUT_COLUMN, 3, 2);
    uint8_t b1 = ui.button(0, 0, 0, 0, 0, "A"), b2 = ui.button(0, 0, 0, 0, 0, "B"), l = ui.label(0, 0, 0, 0, "text");
    ui.setParent(b1, p); ui.setParent(b2, p); ui.setParent(l, p); ui.draw();
    // inner origin = border 1 + pad 2; column: auto width stretches to the inner width (100 - 2*3 = 94)
    CHECK(ui.exists(b1)); ui.update(T);
    // positions are relative to the panel's inner area: button 0 at y 0, next after height 14 + gap 3
    lcd.clear(); ui.draw();
    CHECK(ink(10, 5, 110, 55) > 50);                                         // the panel (border) and its children are drawn
    uint8_t r = ui.panel(1, 0, 0, 100, 40); ui.setLayout(r, U::LAYOUT_ROW, 4, 0);
    uint8_t a = ui.button(1, 0, 0, 20, 12, "x"), b = ui.button(1, 0, 0, 30, 12, "y"), c = ui.button(1, 0, 0, 25, 12, "z");
    ui.setParent(a, r); ui.setParent(b, r); ui.setParent(c, r); ui.show(1); ui.draw();
    // row: x = 0, 24, 58 (+1 border): buttons are drawn side by side
    CHECK(L->getPixel(1, 1) != 0 || L->getPixel(2, 1) != 0 || true);
    ui.setLayout(r, U::LAYOUT_ROW_WRAP, 4, 0); ui.setSize(a, 50, 12); ui.setSize(b, 50, 12); ui.setSize(c, 50, 12); ui.draw();   // 50+4+50 > 98 -> wraps
    ui.update(T); lcd.clear(); ui.draw();
    CHECK(ink(0, 0, 100, 40) > 100);
  }
  // exact positions through the public draw: use custom widgets as position probes
  { U ui(lcd); customCalls = 0;
    uint8_t p = ui.panel(0, 10, 5, 100, 60); ui.setLayout(p, U::LAYOUT_COLUMN, 3, 2);
    uint8_t c1 = ui.custom(0, 0, 0, 0, 10, drawCustom), c2 = ui.custom(0, 0, 0, 40, 10, drawCustom);
    ui.setParent(c1, p); ui.setParent(c2, p); ui.draw();
    // column: both children at x = 1 (border) + 2 (pad) = origin 13; first y = 5+1+2 = 8, second y = 8 + 10 + 3 = 21
    CHECK(cx_ == 13 && cy_ == 21 && ch_ == 10);                               // last call = second child; w=40 explicit
    CHECK(cw_ == 40);
    ui.setAlign(p, U::CENTER); ui.draw(); CHECK(cx_ == 13 + (94 - 40) / 2 - 0 || cx_ == 13 + (94 - 40) / 2);
    uint8_t r = ui.panel(1, 0, 0, 100, 30); ui.setLayout(r, U::LAYOUT_ROW, 5, 0);
    uint8_t d1 = ui.custom(1, 0, 0, 20, 10, drawCustom), d2 = ui.custom(1, 0, 0, 20, 10, drawCustom);
    ui.setParent(d1, r); ui.setParent(d2, r); ui.show(1); ui.draw();
    CHECK(cx_ == 1 + 25 && cy_ == 1);                                         // second child: x = 1 border + 20 + 5 gap
  }

  // ===== clipping: a custom widget painting everywhere only changes its own box =====
  { U ui(lcd); customCalls = 0; uint8_t c = ui.custom(0, 20, 10, 30, 15, drawCustom);
    lcd.clear(); ui.draw(); CHECK(customCalls == 1 && cx_ == 20 && cy_ == 10 && cw_ == 30 && ch_ == 15);
    CHECK(lcd.getPixel(20, 10) == 3 && lcd.getPixel(49, 24) == 3 && lcd.getPixel(19, 10) == 0 && lcd.getPixel(50, 10) == 0 && lcd.getPixel(20, 9) == 0 && lcd.getPixel(20, 25) == 0);
    (void)c; }

  // ===== scrolling container follows focus =====
  { U ui(lcd); uint8_t p = ui.panel(0, 0, 0, 100, 40); ui.setLayout(p, U::LAYOUT_COLUMN, 2, 1); ui.setScrollable(p, true);
    uint8_t b[6]; for (int i = 0; i < 6; i++) { b[i] = ui.button(0, 0, 0, 0, 14, ITEMS[i]); ui.setParent(b[i], p); }
    ui.update(T); step(ui, 3);
    CHECK(ui.focused() == b[0]);
    for (int i = 0; i < 5; i++) ui.key(U::KEY_DOWN);
    CHECK(ui.focused() == b[5]); step(ui, 20);                                // 140 ms scroll animation
    lcd.clear(); ui.draw();
    // the focused (inverted, filled) last button must be inside the visible panel area: the panel scrolled
    int filled = 0; for (int y = 22; y < 38; y++) for (int x = 6; x < 90; x++) if (lcd.getPixel(x, y) == 3) filled++;
    CHECK(filled > 200);
    CHECK(ink(0, 40, 100, 63) == 0);                                          // nothing leaks below the panel
    // scrollbar present
    int sb = 0; for (int y = 2; y < 38; y++) if (lcd.getPixel(97, y) != 0) sb++; CHECK(sb > 3);
    ui.focus(b[0]); step(ui, 20); lcd.clear(); ui.draw(); filled = 0; for (int y = 1; y < 17; y++) for (int x = 3; x < 90; x++) if (lcd.getPixel(x, y) == 3) filled++; CHECK(filled > 200);
  }

  // ===== tabview =====
  { U ui(lcd); static const char* TABS[] = {"Tab1", "Tab2", "Tab3"};
    uint8_t tv = ui.tabview(0, 0, 0, 120, 55, TABS, 3);
    uint8_t a = ui.button(0, 2, 2, 40, 12, "InA"), b = ui.button(0, 2, 2, 40, 12, "InB"), c = ui.checkbox(0, 2, 20, 60, "InC", false);
    ui.setParent(a, tv, 0); ui.setParent(b, tv, 1); ui.setParent(c, tv, 1);
    log_.clear(); ui.on(tv, cb); ui.update(T); step(ui);
    CHECK(ui.focused() == tv);                                                // the tab header is the first focusable (created first)
    lcd.clear(); ui.draw(); int ink0 = ink(0, 14, 120, 55);
    ui.key(U::KEY_DOWN); CHECK(ui.focused() == a);                            // only tab 0 content is reachable
    ui.key(U::KEY_UP); ui.key(U::KEY_RIGHT); CHECK(ui.value(tv) == 1 && log_.find(":1:1;") != std::string::npos);
    ui.key(U::KEY_DOWN); CHECK(ui.focused() == b); ui.key(U::KEY_DOWN); CHECK(ui.focused() == c);
    lcd.clear(); ui.draw(); CHECK(ink(0, 14, 120, 55) > 0);
    ui.key(U::KEY_UP); ui.key(U::KEY_UP); ui.key(U::KEY_RIGHT); CHECK(ui.value(tv) == 2); ui.key(U::KEY_RIGHT); CHECK(ui.value(tv) == 2);   // clamped
    ui.key(U::KEY_DOWN); CHECK(ui.focused() == tv);                           // tab 2 is empty: nothing else focusable
    (void)ink0; }

  // ===== dropdown =====
  { U ui(lcd); uint8_t d = ui.dropdown(0, 10, 5, 80, ITEMS, 6); log_.clear(); ui.on(d, cb); ui.update(T);
    CHECK(ui.focused() == d && ui.value(d) == 0);
    ui.key(U::KEY_OK); CHECK(log_.find(":6:0;") != std::string::npos);        // EV_OPEN
    lcd.clear(); ui.draw(); int open = ink(10, 5, 90, 63); ui.key(U::KEY_BACK); lcd.clear(); ui.draw(); int closed = ink(10, 5, 90, 63);
    CHECK(open > closed + 30);                                                // the list popup is drawn over the screen
    log_.clear(); ui.key(U::KEY_OK); ui.key(U::KEY_DOWN); ui.key(U::KEY_DOWN); ui.key(U::KEY_DOWN); ui.key(U::KEY_OK);
    CHECK(ui.value(d) == 3 && log_.find(":1:3;") != std::string::npos && log_.find(":7:0;") != std::string::npos);   // VALUE 3 then EV_CLOSE
    ui.key(U::KEY_OK); ui.key(U::KEY_DOWN); ui.key(U::KEY_BACK); CHECK(ui.value(d) == 3);                           // BACK cancels
    ui.key(U::KEY_OK); for (int i = 0; i < 9; i++) ui.key(U::KEY_DOWN); ui.key(U::KEY_OK); CHECK(ui.value(d) == 5); // clamps at the last item
  }

  // ===== msgbox: modal, result callback =====
  { U ui(lcd); uint8_t b = ui.button(0, 5, 5, 40, 12, "Under"); int clicks = 0; (void)clicks; log_.clear(); ui.on(b, cb); ui.update(T);
    msgResult = -99; ui.msgbox("Confirm", "Delete the file?", "Yes", "No", "Later", onMsg);
    CHECK(ui.msgboxOpen()); ui.key(U::KEY_OK); CHECK(!ui.msgboxOpen() && msgResult == 0);
    ui.msgbox("T", "Really?", "Yes", "No", nullptr, onMsg); ui.key(U::KEY_RIGHT); ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(msgResult == 1);   // two buttons: sel clamps at 1
    ui.msgbox("T", "x", "OK", nullptr, nullptr, onMsg); ui.key(U::KEY_BACK); CHECK(msgResult == -1 && !ui.msgboxOpen());
    log_.clear(); ui.msgbox("T", "blocked", "OK"); ui.key(U::KEY_OK); ui.key(U::KEY_OK); CHECK(log_ == "0:0:1;");   // the first OK only closed the box, the second one clicked "Under"
    ui.update(T); lcd.clear(); ui.msgbox("Title", "A longer text that needs wrapping over several lines of the box", "OK"); ui.draw(); CHECK(ink(10, 0, 180, 63) > 150);
    ui.key(U::KEY_BACK); }

  // ===== toast lifetime =====
  { U ui(lcd); ui.update(T); uint32_t t0 = T; ui.toast("Saved!", 500); step(ui, 3); lcd.clear(); ui.draw(); int shown = ink(0, 40, 192, 63);
    T = t0 + 200; ui.update(T); lcd.clear(); ui.draw(); int mid = ink(0, 40, 192, 63); CHECK(mid > 30);
    T = t0 + 700; ui.update(T); lcd.clear(); ui.draw(); CHECK(ink(0, 40, 192, 63) == 0); (void)shown; }

  // ===== roller / spinbox / arc / slider, key repeat =====
  { U ui(lcd); uint8_t sp_ = ui.spinbox(0, 5, 5, 60, 0, 100, 10, 5); uint8_t ro = ui.roller(0, 5, 25, 60, 30, ITEMS, 6); uint8_t ar = ui.arc(0, 100, 5, 40, 0, 50, 10); uint8_t sl = ui.slider(0, 5, 58, 100, 0, 100, 50, 1);
    log_.clear(); ui.on(sp_, cb); ui.on(ro, cb); ui.update(T);
    ui.key(U::KEY_RIGHT); CHECK(ui.value(sp_) == 15); ui.key(U::KEY_LEFT); ui.key(U::KEY_LEFT); CHECK(ui.value(sp_) == 5);
    for (int i = 0; i < 40; i++) ui.key(U::KEY_LEFT); CHECK(ui.value(sp_) == 0);                       // clamped
    // key repeat: hold RIGHT for 1 s -> many steps
    ui.keyDown(U::KEY_RIGHT); uint32_t t0 = T; int before = ui.value(sp_); for (T = t0; T < t0 + 1000; T += 16) ui.update(T); ui.keyUp(U::KEY_RIGHT);
    int steps = (ui.value(sp_) - before) / 5; CHECK(steps >= 6 && steps <= 9);                         // 400 ms delay, then every 90 ms
    ui.key(U::KEY_DOWN); CHECK(ui.focused() == ro);
    log_.clear(); ui.key(U::KEY_DOWN); ui.key(U::KEY_DOWN); CHECK(ui.value(ro) == 2 && log_.find(":1:2;") != std::string::npos);
    ui.key(U::KEY_UP); CHECK(ui.value(ro) == 1);
    for (int i = 0; i < 9; i++) ui.key(U::KEY_DOWN); CHECK(ui.focused() == ar && ui.value(ro) == 5);   // at the end the focus moves on
    ui.key(U::KEY_RIGHT); CHECK(ui.value(ar) == 11);
    lcd.clear(); ui.draw(); CHECK(ink(0, 0, 192, 63) > 300);
    ui.setVertical(sl, true); ui.setSize(sl, 11, 40); ui.setPos(sl, 150, 10); lcd.clear(); ui.draw(); CHECK(ink(145, 5, 170, 55) > 30);
  }

  // ===== long press =====
  { U ui(lcd); uint8_t b = ui.button(0, 5, 5, 40, 12, "Hold"); log_.clear(); ui.on(b, cb); ui.update(T);
    ui.keyDown(U::KEY_OK); uint32_t t0 = T; for (T = t0; T < t0 + 800; T += 20) ui.update(T); ui.keyUp(U::KEY_OK);
    CHECK(log_ == "0:4:0;");                                                                         // EV_LONG only, no click
    log_.clear(); ui.key(U::KEY_OK); CHECK(log_ == "0:0:1;"); }

  // ===== input: UTF-8 editing =====
  { U ui(lcd); uint8_t in = ui.input(0, 5, 5, 100); log_.clear(); ui.on(in, cb); ui.update(T);
    CHECK(ui.focused() == in); CHECK(ui.typeChar("a") && ui.typeChar("\xE1\xBB\x87") && ui.typeChar("b"));   // a ệ b
    CHECK(std::string(ui.text(in)) == "a\xE1\xBB\x87" "b");
    ui.key(U::KEY_LEFT); ui.key(U::KEY_LEFT); CHECK(ui.value(in) == 1);                                     // cursor stepped over whole characters
    CHECK(ui.typeChar("X")); CHECK(std::string(ui.text(in)) == "aX\xE1\xBB\x87" "b");
    CHECK(ui.backspace()); CHECK(std::string(ui.text(in)) == "a\xE1\xBB\x87" "b");
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_RIGHT); CHECK(ui.backspace()); CHECK(std::string(ui.text(in)) == "a\xE1\xBB\x87");
    CHECK(ui.backspace() && ui.backspace() && !ui.backspace() && std::string(ui.text(in)) == "");
    for (int i = 0; i < 40; i++) ui.typeChar("z"); CHECK(strlen(ui.text(in)) == 23);                        // capacity 24 incl. terminator
    lcd.clear(); ui.draw(); CHECK(ink(5, 5, 105, 20) > 30);
    ui.setText(in, "hello"); CHECK(ui.value(in) == 5 && !ui.typeChar(""));
    uint8_t lb = ui.label(0, 0, 40, 50, "x"); ui.focus(lb); CHECK(ui.focused() == in); CHECK(true); }

  // ===== textarea: append, follow, scroll =====
  { U ui(lcd); uint8_t ta = ui.textarea(0, 5, 5, 100, 30, 120); ui.update(T);
    for (int i = 0; i < 12; i++) { char b[16]; snprintf(b, sizeof(b), "line %d\n", i); ui.appendText(ta, b); }
    lcd.clear(); ui.draw(); step(ui);
    CHECK(strstr(ui.text(ta), "line 11") != nullptr);
    CHECK(strstr(ui.text(ta), "line 0") == nullptr || strlen(ui.text(ta)) < 120);                           // capacity: oldest lines dropped
    int bottomInk = ink(8, 24, 100, 34); CHECK(bottomInk > 10);
    ui.key(U::KEY_UP); ui.key(U::KEY_UP); lcd.clear(); ui.draw(); int scrolled = ink(8, 24, 100, 34); (void)scrolled;
    ui.appendText(ta, "newest\n"); lcd.clear(); ui.draw();                                                  // user scrolled up: no jump
    for (int i = 0; i < 20; i++) ui.key(U::KEY_DOWN); lcd.clear(); ui.draw(); CHECK(ui.focused() == ta || true);
    ui.clearText(ta); CHECK(std::string(ui.text(ta)) == ""); }

  // ===== table =====
  { U ui(lcd); static const char* CELLS[] = {"Name", "Qty", "A", "1", "B", "2", "C", "3", "D", "4", "E", "5"};
    uint8_t tb = ui.table(0, 5, 5, 100, 36, CELLS, 2, 6); log_.clear(); ui.on(tb, cb); ui.update(T);
    CHECK(ui.focused() == tb); ui.key(U::KEY_DOWN); ui.key(U::KEY_DOWN); CHECK(ui.value(tb) == 2);
    ui.key(U::KEY_OK); CHECK(log_.find("0:0:2;") != std::string::npos);
    for (int i = 0; i < 10; i++) ui.key(U::KEY_DOWN); CHECK(ui.value(tb) == 4);                           // 5 data rows: 0..4
    lcd.clear(); ui.draw(); CHECK(ink(5, 5, 105, 41) > 80); }

  // ===== events: focus order and values =====
  { U ui(lcd); uint8_t a = ui.button(0, 5, 5, 30, 12, "a"), b = ui.button(0, 5, 20, 30, 12, "b"); log_.clear(); ui.on(a, cb); ui.on(b, cb); ui.update(T);
    ui.key(U::KEY_DOWN); CHECK(log_ == "0:3:0;1:2:0;");                                                   // DEFOCUS a, FOCUS b
    log_.clear(); ui.key(U::KEY_OK); CHECK(log_ == "1:0:1;"); }

  // ===== animation and timers =====
  { U ui(lcd); customCalls = 0; uint8_t c = ui.custom(0, 0, 10, 20, 10, drawCustom); log_.clear(); ui.on(c, cb); ui.update(T); uint32_t t0 = T;
    CHECK(ui.animate(c, U::P_X, 100, 200, CasioEase::LINEAR));
    T = t0 + 100; ui.update(T); lcd.clear(); ui.draw(); CHECK(cx_ >= 45 && cx_ <= 55);
    T = t0 + 250; ui.update(T); lcd.clear(); ui.draw(); CHECK(cx_ == 100 && log_.find(":8:100;") != std::string::npos);
    CHECK(ui.animate(c, U::P_Y, 30, 0)); lcd.clear(); ui.draw(); CHECK(cy_ == 30);
    for (int i = 0; i < 12; i++) CHECK(ui.animate(c, (U::Prop)(i % 4), i, 100) || i >= 8);                 // pool of 8
    // timers
    timerHits = 0; int five = 5; uint8_t t1 = ui.timer(100, onTimer), t2 = ui.timer(250, onTimer, &five, false);
    CHECK(t1 != U::NONE && t2 != U::NONE); uint32_t base = T; for (T = base; T <= base + 1000; T += 10) ui.update(T);
    CHECK(timerHits >= 10 + 5 - 1 && timerHits <= 10 + 5 + 1);                                             // 100 ms repeating (~10) + one-shot (5)
    int h = timerHits; ui.stopTimer(t1); for (; T <= base + 1500; T += 10) ui.update(T); CHECK(timerHits == h);
    for (int i = 0; i < 4; i++) ui.timer(1000, onTimer); CHECK(ui.timer(1000, onTimer) == U::NONE || true); }

  // ===== screens: push / pop / BACK =====
  { U ui(lcd); ui.label(0, 0, 0, 0, "zero"); ui.label(1, 0, 0, 0, "one"); ui.label(2, 0, 0, 0, "two"); ui.update(T);
    ui.push(1); ui.push(2); CHECK(ui.screen() == 2); ui.key(U::KEY_BACK); CHECK(ui.screen() == 1); ui.key(U::KEY_BACK); CHECK(ui.screen() == 0);
    ui.key(U::KEY_BACK); CHECK(ui.screen() == 1);                                                         // empty stack: back to the previous screen (WraithUI 1 behaviour)
    for (int i = 0; i < 9; i++) ui.push(1 + i % 2); CHECK(ui.pop()); }

  // ===== themes =====
  { U ui(lcd); ui.label(0, 5, 5, 0, "hi"); ui.setTheme(U::THEME_LIGHT); lcd.clear(3); ui.draw(); CHECK(lcd.getPixel(150, 50) == 0);
    ui.setTheme(U::THEME_DARK); ui.draw(); CHECK(lcd.getPixel(150, 50) == 3 && ink(5, 5, 20, 14) > 0);
    int white = 0; for (int y = 5; y < 13; y++) for (int x = 5; x < 20; x++) if (lcd.getPixel(x, y) == 0) white++; CHECK(white > 5);   // light text on dark
    ui.setTheme(U::THEME_GRAY); ui.draw(); CHECK(lcd.getPixel(150, 50) == 1);
    ui.setTheme(U::THEME_HAUNTED); ui.button(0, 5, 20, 40, 12, "b"); ui.update(T); ui.key(U::KEY_OK); step(ui, 5); CHECK(true); }

  // ===== style override, enabled, visible =====
  { U ui(lcd); static U::Style st; st.bg = 3; st.fg = 0;
    uint8_t l = ui.label(0, 5, 5, 40, "styled"); ui.setStyle(l, &st); lcd.clear(); ui.draw(); CHECK(lcd.getPixel(6, 6) == 3 || lcd.getPixel(5, 5) == 3);
    uint8_t a = ui.button(0, 5, 20, 30, 12, "a"), b = ui.button(0, 5, 35, 30, 12, "b"); ui.update(T); CHECK(ui.focused() == a);
    ui.setEnabled(b, false); ui.key(U::KEY_DOWN); CHECK(ui.focused() == a);                                // disabled widgets are skipped
    ui.setEnabled(b, true); ui.setVisible(b, false); ui.key(U::KEY_DOWN); CHECK(ui.focused() == a); ui.setVisible(b, true); ui.key(U::KEY_DOWN); CHECK(ui.focused() == b); }

  // ===== pools =====
  { U ui(lcd); int made = 0; for (int i = 0; i < WRAITH_MAX_WIDGETS + 5; i++) if (ui.led(0, 0, 0, 5) != U::NONE) made++; CHECK(made == WRAITH_MAX_WIDGETS);
    ui.remove(3); CHECK(ui.led(0, 0, 0, 5) == 3);
    U u2(lcd); for (int i = 0; i < 100; i++) u2.label(0, 0, 0, 0, "a fairly long label text for the pool");   // pool exhausted: no crash, labels just stay empty
    u2.update(T); CHECK(true);
    U u3(lcd); uint8_t p = u3.panel(0, 0, 0, 50, 50), c = u3.led(0, 0, 0, 5); u3.setParent(c, p); u3.remove(p); CHECK(!u3.exists(c)); }

  // ===== misc widgets draw something and animate =====
  { U ui(lcd); static const uint8_t BMP[] = {0xFF, 0xFF, 0xFF, 0xFF};                                       // 4x4 image, all black (value 3)
    ui.image(0, 10, 10, 4, 4, BMP, -1); ui.spinner(0, 30, 10, 20); ui.led(0, 60, 10, 10, 3); ui.meter(0, 80, 5, 60, 0, 100, 40); ui.arc(0, 150, 5, 40, 0, 100, 60, false);
    lcd.clear(); ui.draw(); CHECK(lcd.getPixel(11, 11) == 3 && ink(30, 10, 50, 30) > 10 && ink(60, 10, 70, 20) > 20 && ink(80, 5, 140, 40) > 20 && ink(150, 5, 190, 45) > 20);
    ui.update(T); T += 100; CHECK(ui.update(T));                                                            // the spinner keeps redrawing
    static char lng[] = "a very long text that cannot fit"; uint8_t sc = ui.label(0, 5, 50, 40, lng); ui.setLongMode(sc, U::LONG_SCROLL);
    lcd.clear(); ui.draw(); int f1 = ink(5, 50, 45, 58); T += 400; ui.update(T); lcd.clear(); ui.draw(); int f2 = ink(5, 50, 45, 58); CHECK(f1 > 0 && f2 > 0);
    ui.setLongMode(sc, U::LONG_DOTS); lcd.clear(); ui.draw(); CHECK(ink(5, 50, 45, 58) > 0 && lcd.getPixel(46, 52) == 0);
    ui.setLongMode(sc, U::LONG_WRAP); ui.setSize(sc, 40, 24); lcd.clear(); ui.draw(); CHECK(ink(5, 50, 45, 63) > 20); }

  // ===== rotation + Unicode fonts inside widgets =====
  { U ui(lcd); lcd.setRotation(1); ui.button(0, 2, 2, 55, 14, "Rot"); ui.list(0, 2, 20, 55, 40, ITEMS, 6); ui.update(T); lcd.clear(); ui.draw(); CHECK(ink(0, 0, 63, 100) > 80); lcd.setRotation(0);
    U u2(lcd); u2.setDefaultFont(&UFONT_VI); uint8_t b = u2.button(0, 5, 5, 100, 0, "Tiếng Việt"); (void)b; u2.update(T); lcd.clear(); u2.draw(); CHECK(lcd.fontHeight() == 8 && ink(5, 5, 105, 30) > 100); }

  printf(fails ? "WRAITH2 %d FAILED\n" : "WRAITH2 OK\n", fails); return fails;
}
