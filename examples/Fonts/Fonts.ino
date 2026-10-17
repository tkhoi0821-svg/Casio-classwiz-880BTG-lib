// Custom font demo (GrayKazu). Needs a GFX-format font header, e.g. copy FreeSans9pt7b.h
// from Adafruit_GFX/Fonts into this sketch folder. Without it, delete the font lines.
#include <CasioST75256.h>
#include "FreeSans9pt7b.h"

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

void setup() {
  lcd.begin();
  lcd.clear();
  lcd.setFont(&FreeSans9pt7b);          // cursor y is the baseline now
  lcd.printCentered("Casio", 16);
  lcd.setFont(nullptr);                 // back to the 5x7 font (cursor y = top)
  lcd.printRight("v0.3.1", 187, 28);
  lcd.drawArc(30, 50, 10, 180, 360);    // half circle, top side
  lcd.display();
}
void loop() {}
