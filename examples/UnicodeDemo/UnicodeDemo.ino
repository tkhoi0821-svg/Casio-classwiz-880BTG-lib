// Vietnamese + Thai + Chinese on the 192x63 LCD (GrayKazu). Save this file as UTF-8.
// Fonts are 16 px bitmaps (GNU Unifont): 3 lines fit on the screen, 24 half-width / 12 full-width glyphs per line.
#include <CasioST75256.h>
#include <CasioFont_VI.h>      // ASCII + full Vietnamese  (~4 KB flash)
#include <CasioFont_TH.h>      // ASCII + Thai with stacked vowel/tone marks (~3 KB)
#include <CasioFont_ZH_CN.h>   // ASCII + 4100 Simplified Chinese glyphs (~160 KB)
// #include <CasioFont_ZH_TW.h> // Traditional Chinese (~215 KB), add it the same way

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

void setup() {
  lcd.begin();
  lcd.addUFont(&UFONT_VI);       // searched in this order for every character
  lcd.addUFont(&UFONT_TH);
  lcd.addUFont(&UFONT_ZH_CN);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("Tiếng Việt: Ấ ệ ử");
  lcd.println("สวัสดีครับ ที่นี่");
  lcd.print("你好，世界！中文");
  lcd.display();
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 8);
  lcd.printCursed("Xin chào 你好 สวัสดี", 60);   // the haunted text also understands UTF-8
  lcd.fxGlitch(100);
  lcd.display();
  delay(2000);
}
