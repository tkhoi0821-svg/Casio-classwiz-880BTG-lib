#include <Arduino.h>
#include <SPI.h>
#include "CasioST75256.h"
#include "scan.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main() {
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin(); lcd.addUFont(&UFONT_SCAN);
  lcd.clear(); lcd.setCursor(0, 0); lcd.print("Hello 你好世界 こんにちは");
  int n = 0; for (int y = 0; y < 63; y++) for (int x = 0; x < 192; x++) if (lcd.getPixel(x, y)) n++;
  printf("count=%d height=%d width_cjk=%d ink=%d\n", UFONT_SCAN.count, UFONT_SCAN.height, lcd.textWidth("你好"), n);
}
