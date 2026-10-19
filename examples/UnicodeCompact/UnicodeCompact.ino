// Compact antialiased Unicode fonts (GrayKazu): 15 px cells with 4 gray levels, FOUR lines on the 63 px screen.
// Compared with the crisp 16 px Unifont fonts (CasioFont_VI.h ...) they are smaller per glyph but softer;
// use whichever reads better on your panel. Save this file as UTF-8.
#include <CasioST75256.h>
#include <CasioFont_VI_15.h>      // DejaVu Sans, ASCII + Vietnamese            (~8 KB flash)
#include <CasioFont_TH_15.h>      // Loma, ASCII + Thai (marks stack, tone marks rise over vowels) (~6 KB)
#include <CasioFont_ZH_CN_15.h>   // Noto Sans CJK SC, 4100 glyphs                  (~245 KB)
// #include <CasioFont_ZH_TW_15.h> // Noto Sans CJK TC, 5800 glyphs                 (~350 KB)

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

void setup() {
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_TH_15); lcd.addUFont(&UFONT_ZH_CN_15);
  lcd.clear(); lcd.setCursor(0, 0);
  lcd.println("Tiếng Việt: Ấ ệ ử Ớ");
  lcd.println("สวัสดีครับ ที่นี่ ผู้ป");
  lcd.println("你好，世界！中文字体");
  lcd.print("Trộn: abc 你好 สวัสดี");
  lcd.display();
  // The glyphs are blended over whatever is behind them, so these also work:
  //   lcd.setTextColor(0, 3);           // white text on a black background
  //   lcd.fillRect(...); lcd.print(..)  // text over gray shapes
}
void loop() {}
