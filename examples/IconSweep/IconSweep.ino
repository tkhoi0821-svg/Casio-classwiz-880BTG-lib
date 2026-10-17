// Sweeps a lit block along the icon row and prints its column to Serial,
// so you can note which columns belong to FIX, SCI, the arrows ...
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

const int BLOCK_W = 8, STEP = 4;

void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.clear();
  lcd.display();
}

void loop() {
  for (int s = 0; s <= CasioST75256::WIDTH - BLOCK_W; s += STEP) {
    lcd.iconClear();
    lcd.iconSet(s, s + BLOCK_W - 1, true);
    lcd.display();
    Serial.printf("icon block at columns %d..%d\n", s, s + BLOCK_W - 1);
    delay(80);
  }
  lcd.iconClear(); lcd.display(); delay(600);
}
