// Tests for 2-bit antialiased Unicode fonts: exact blending, raised Thai tone marks, mixing with 1-bit fonts
#include <Arduino.h>
#include <SPI.h>
#include "CasioST75256.h"
#include "CasioFont_VI_15.h"
#include "CasioFont_TH_15.h"
#include "CasioFont_VI.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static const UGlyph* find(const UFont& f, uint16_t cp) { for (int i = 0; i < f.count; i++) if (f.glyphs[i].cp == cp) return &f.glyphs[i]; return nullptr; }
static int cov(const UFont& f, const UGlyph* g, int i, int j) { int bpr = (g->w + 3) / 4; return (f.bitmap[g->off + j * bpr + i / 4] >> (6 - 2 * (i & 3))) & 3; }
static int topInk(CasioST75256& l, int x0, int x1, int y0, int y1) { for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) if (l.getPixel(x, y)) return y; return -1; }
static int ink(CasioST75256& l, int x0, int x1, int y0, int y1) { int n = 0; for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) n += l.getPixel(x, y); return n; }
int main() {
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  CHECK(UFONT_VI_15.bpp == 2 && UFONT_VI.bpp == 0 && UFONT_VI_15.height == 15);
  lcd.addUFont(&UFONT_VI_15);
  const UGlyph* A = find(UFONT_VI_15, 'A'); CHECK(A && A->adv >= A->w - 1);
  int total = 0; for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) total += cov(UFONT_VI_15, A, i, j); CHECK(total > 20);
  // 1) black on white, transparent: pixel level == coverage
  lcd.clear(); lcd.setCursor(0, 0); lcd.setTextColor(3); lcd.print("A");
  bool ok = true; for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) if (lcd.getPixel(i, j) != cov(UFONT_VI_15, A, i, j)) ok = false; CHECK(ok);
  // 2) opaque background: fg 3 on bg 1 -> 1 + (2*v*2 + 3)/6 ; v=0 -> bg
  lcd.clear(); lcd.setCursor(0, 0); lcd.setTextColor(3, 1); lcd.print("A"); ok = true;
  for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) { int v = cov(UFONT_VI_15, A, i, j); int want = 1 + (2 * v * 2 + 3) / 6; if (lcd.getPixel(i, j) != want) ok = false; } CHECK(ok);
  // 3) white text on black: v=3 -> 0, v=0 stays black
  lcd.clear(); lcd.fillRect(0, 0, 40, 20, 3); lcd.setCursor(0, 0); lcd.setTextColor(0); lcd.print("A"); ok = true;
  for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) { int v = cov(UFONT_VI_15, A, i, j); int want = 3 + (-3 * v * 2 - 3) / 6; if (lcd.getPixel(i, j) != want) ok = false; }
  CHECK(ok);
  // 4) transparent over a gray background keeps the gray where coverage is 0
  lcd.clear(); lcd.fillRect(0, 0, 40, 20, 2); lcd.setCursor(0, 0); lcd.setTextColor(3); lcd.print("A"); ok = true;
  for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) { int v = cov(UFONT_VI_15, A, i, j); int want = 2 + (1 * v * 2 + 3) / 6; if (lcd.getPixel(i, j) != want) ok = false; } CHECK(ok);
  // 5) scaling x2 repeats each blended pixel as a 2x2 block
  lcd.clear(); lcd.setTextSize(2); lcd.setCursor(0, 0); lcd.setTextColor(3); lcd.print("A"); ok = true;
  for (int j = 0; j < 15; j++) for (int i = 0; i < A->w; i++) { int v = cov(UFONT_VI_15, A, i, j); if (lcd.getPixel(2 * i, 2 * j) != v || lcd.getPixel(2 * i + 1, 2 * j + 1) != v) ok = false; } CHECK(ok);
  lcd.setTextSize(1);
  CHECK(lcd.fontHeight() == 15 && lcd.textWidth("Việt") == (find(UFONT_VI_15, 'V')->adv + find(UFONT_VI_15, 'i')->adv + find(UFONT_VI_15, 0x1EC7)->adv + find(UFONT_VI_15, 't')->adv));
  // Vietnamese stacked marks stay inside the cell (nothing drawn at row >= 15 or above row 0 is impossible): ink exists in the top 3 rows for "Ấ"
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Ấ"); CHECK(topInk(lcd, 0, 12, 0, 15) <= 2);
  // ---- Thai: tone mark over a vowel uses the raised twin ----
  lcd.clearUFonts(); lcd.addUFont(&UFONT_TH_15);
  CHECK(find(UFONT_TH_15, 0x0E48) && find(UFONT_TH_15, 0x0E48 | 0xF000) && find(UFONT_TH_15, 0x0E48)->adv == 0);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("ท่"); int topFlat = topInk(lcd, 0, 14, 0, 15), inkFlat = ink(lcd, 0, 14, 0, 15);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("ที่"); int topRaised = topInk(lcd, 0, 14, 0, 15), inkRaised = ink(lcd, 0, 14, 0, 15);
  printf("thai tone mark top row: flat %d, over vowel %d (ink %d vs %d)\n", topFlat, topRaised, inkFlat, inkRaised);
  CHECK(topRaised >= 0 && topRaised <= topFlat);            // never lower than on a bare consonant
  { lcd.clear(); lcd.setCursor(0, 0); lcd.print("ท\u0E34"); int vowelTop = topInk(lcd, 0, 14, 0, 15);
    lcd.clear(); lcd.setCursor(0, 0); lcd.print("ท\u0E34\u0E48"); int stackedTop = topInk(lcd, 0, 14, 0, 15);
    CHECK(stackedTop < vowelTop); }                           // the tone sits above the vowel instead of on top of it
  // the cursor does not advance for marks
  lcd.setCursor(0, 0); lcd.print("ท"); int cx = lcd.textWidth("ท"); CHECK(lcd.textWidth("ที่") == cx);
  // ---- mixing: 1-bit and 2-bit fonts in one chain ----
  lcd.clearUFonts(); lcd.addUFont(&UFONT_VI); lcd.addUFont(&UFONT_TH_15);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Aท"); CHECK(lcd.textWidth("Aท") == 8 + find(UFONT_TH_15, 0x0E17)->adv);
  CHECK(ink(lcd, 0, 8, 0, 16) > 10 && ink(lcd, 8, 20, 0, 15) > 5);
  printf(fails ? "FONT15 %d FAILED\n" : "FONT15 OK\n", fails); return fails;
}
