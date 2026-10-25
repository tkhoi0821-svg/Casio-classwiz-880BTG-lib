// WraithUI keyboard widget and the symbol (icon) fonts
#include <string>
#include <Arduino.h>
#include <SPI.h>
#include "WraithUI.h"
#include "CasioFont_VI_15.h"
#include "CasioFont_SYM_15.h"
#include "CasioFont_SYM_16.h"
#include "CasioFont_SYM_26.h"
#include "CasioSymbols.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
typedef WraithUI U;
static uint32_t T = 1000; static std::string log_;
static void cb(uint8_t id, uint8_t ev, int16_t v, void*) { char b[40]; snprintf(b, sizeof(b), "%d:%d:%d;", id, ev, v); log_ += b; }
static int ink(CasioST75256& l, int x0, int y0, int x1, int y1) { int n = 0; for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) if (l.getPixel(x, y)) n++; return n; }
static void press(U& ui, std::initializer_list<U::Key> ks) { for (U::Key k : ks) ui.key(k); }
int main() {
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  // ===== keyboard =====
  // layout: rows "1234567890" / "qwertyuiop" / "asdfghjkl"+DEL / SHIFT+"zxcvbnm,." / SYM < SPACE(4 wide) > OK(3 wide); the cursor starts on q
  { U ui(lcd); uint8_t in = ui.input(0, 0, 0, 190); uint8_t kb = ui.keyboard(0, 0, 14, 190, 50, in); ui.update(T);
    CHECK(ui.focused() == in); ui.key(U::KEY_DOWN); CHECK(ui.focused() == kb);
    log_.clear(); ui.on(kb, cb);
    ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "q");
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "qwe");
    CHECK(log_.find(":5:") != std::string::npos);                                       // EV_CHAR on the keyboard
    press(ui, {U::KEY_LEFT, U::KEY_LEFT, U::KEY_DOWN, U::KEY_DOWN});                      // (1,0) -> (3,0) = shift
    ui.key(U::KEY_OK);                                                                  // shift on (one-shot)
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "qweZ");  // z -> Z
    ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "qweZz");                       // shift was consumed
    press(ui, {U::KEY_UP});                                                              // (2,1)
    for (int i = 0; i < 8; i++) ui.key(U::KEY_RIGHT);                                    // (2,9) = DEL
    ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "qweZ");
    lcd.clear(); ui.draw(); CHECK(ink(lcd, 0, 14, 190, 64) > 400);                       // the keyboard is drawn
  }
  { U ui(lcd); uint8_t in = ui.input(0, 0, 0, 190); uint8_t kb = ui.keyboard(0, 0, 14, 190, 50, in); ui.update(T); ui.focus(kb); log_.clear(); ui.on(kb, cb);
    for (int i = 0; i < 3; i++) ui.key(U::KEY_DOWN);                                      // (1,0) -> (4,0) = SYM
    ui.key(U::KEY_OK);                                                                  // symbols mode
    for (int i = 0; i < 4; i++) ui.key(U::KEY_UP);                                        // (0,0)
    ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "!");
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "!@");   // (0,1)
    for (int i = 0; i < 4; i++) ui.key(U::KEY_DOWN);                                      // (4,1) = "<"
    ui.key(U::KEY_LEFT); ui.key(U::KEY_OK);                                              // (4,0) SYM again: back to letters
    for (int i = 0; i < 4; i++) ui.key(U::KEY_UP);                                        // (0,0)
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK);                        // (0,2) = '3' in letters mode
    CHECK(std::string(ui.text(in)) == "!@3");
    for (int i = 0; i < 4; i++) ui.key(U::KEY_DOWN);                                      // (4,2) = SPACE (wide key)
    ui.key(U::KEY_LEFT); ui.key(U::KEY_OK); ui.key(U::KEY_OK); CHECK(ui.value(in) == 1);  // "<" twice: cursor 3 -> 1
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(std::string(ui.text(in)) == "! @3");  // space inserted at the cursor
    ui.key(U::KEY_RIGHT); ui.key(U::KEY_OK); CHECK(ui.value(in) == 3);                   // ">" (jumped over the wide space key)
    ui.key(U::KEY_RIGHT); log_.clear(); ui.key(U::KEY_OK); CHECK(log_.find("1:0:1;") != std::string::npos);   // OK key = EV_CLICK on the keyboard
    ui.key(U::KEY_DOWN); CHECK(ui.focused() == in);                                      // bottom edge hands the focus on
    ui.focus(kb); CHECK(ui.typeChar("x") && std::string(ui.text(in)) == "! @x3" && ui.backspace());   // a physical keypad types through the keyboard
  }
  { U ui(lcd); uint8_t kb = ui.keyboard(0, 0, 14, 190, 50); ui.update(T); ui.key(U::KEY_OK); CHECK(!ui.typeChar("a")); (void)kb; }     // no target: nothing breaks

  // ===== symbols =====
  CHECK(UFONT_SYM_15.count == 53 && UFONT_SYM_16.count == 53 && UFONT_SYM_26.count == 53);
  CHECK(UFONT_SYM_15.height == 15 && UFONT_SYM_16.height == 16 && UFONT_SYM_26.height == 26 && UFONT_SYM_15.bpp == 2);
  CHECK(strlen(SYM_WIFI) == 3 && (uint8_t)SYM_WIFI[0] == 0xEE && (uint8_t)SYM_WIFI[1] == 0x80 && (uint8_t)SYM_WIFI[2] == 0x80);
  CHECK((uint8_t)SYM_BLUETOOTH[2] == 0x81 && strcmp(SYM_WIFI, SYM_BAN) != 0);
  { lcd.clearUFonts(); lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_SYM_15);
    lcd.clear(); lcd.setCursor(0, 0); lcd.print(SYM_WIFI " Wi-Fi"); CHECK(ink(lcd, 0, 0, 12, 15) > 15 && ink(lcd, 14, 0, 50, 15) > 30);
    int w1 = lcd.textWidth(SYM_WIFI), w2 = lcd.textWidth("A"); CHECK(w1 == 13 && w2 > 3);
    // every icon draws something inside its own cell and nothing outside
    for (int i = 0; i < 53; i++) { char s[4] = {(char)0xEE, (char)0x80, (char)(0x80 + i), 0}; if (i >= 64) break;
      lcd.clear(); lcd.setCursor(20, 5); lcd.print(s); int in_ = ink(lcd, 20, 5, 33, 20), out = ink(lcd, 0, 0, 20, 63) + ink(lcd, 33, 0, 192, 63) + ink(lcd, 20, 0, 33, 5) + ink(lcd, 20, 20, 33, 63);
      CHECK(in_ >= 8 && out == 0); }
    lcd.clearUFonts(); }
  { U ui(lcd); ui.setDefaultFont(&UFONT_VI_15); ui.addFallbackFont(&UFONT_SYM_15); ui.label(0, 0, 0, 0, SYM_BATTERY_FULL " 100% " SYM_CHARGE);
    ui.update(T); lcd.clear(); ui.draw(); CHECK(ink(lcd, 0, 0, 12, 15) > 15 && ink(lcd, 12, 0, 60, 15) > 30); }
  printf(fails ? "WRAITH3 %d FAILED\n" : "WRAITH3 OK\n", fails); return fails;
}
