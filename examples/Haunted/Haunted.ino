// HAUNTED demo (GrayKazu): ghost trails, static, glitch, melting, cursed text, CRT off.
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

void frame() { lcd.display(); }

void setup() {
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  lcd.fxSeed(esp_random());          // ESP32 hardware RNG; use analogRead(A0) elsewhere
}

void loop() {
  // 1) a ball leaving ghost trails (fxDecay instead of clear)
  lcd.clear();
  for (int i = 0; i < 120; i++) {
    lcd.fxDecay(1);
    int x = 96 + (int)(80 * sinf(i * 0.11f)), y = 31 + (int)(26 * sinf(i * 0.23f));
    lcd.fillCircle(x, y, 4, 3);
    frame();
  }
  // 2) something is wrong with the text
  lcd.clear(); lcd.setTextSize(2);
  for (int i = 0; i < 40; i++) {
    lcd.clear(); lcd.setCursor(20, 20);
    lcd.printCursed("IT SEES YOU", 20 + i * 5);
    lcd.fxGlitch(i * 6); if (i > 20) lcd.fxNoise(i * 2, 2);
    lcd.fxScanlines(2, 1);
    frame(); delay(40);
  }
  lcd.setTextSize(1);
  lcd.fxHeartbeat(3, 50);              // contrast pulses (needs a working setVop)
  // 3) everything melts
  for (int i = 0; i < 60; i++) { lcd.fxMelt(3); frame(); }
  lcd.fxFlicker(6);
  // 4) lights out
  lcd.clear(); lcd.fillRoundRect(40, 14, 112, 34, 6, 3);
  lcd.setTextColor(0); lcd.printCentered("SIGNAL LOST", 27); lcd.setTextColor(3);
  frame(); delay(800);
  lcd.fxCrtOff(500);
  delay(1500);
}
