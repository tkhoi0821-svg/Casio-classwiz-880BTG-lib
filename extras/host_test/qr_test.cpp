// QR drawing tests (geometry on the lcd); decoding correctness is checked by run_tests3.py with OpenCV
#include <string>
#include <Arduino.h>
#include <SPI.h>
#include "CasioQR.h"
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\\n", __LINE__, #c); fails++; } } while (0)
int main() {
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  static CasioQR qr;
  CHECK(qr.version() == 0 && qr.size() == 0 && qr.bestScale(192, 63) == 0);
  CHECK(!qr.encode(std::string(600, 'a').c_str()) && qr.version() == 0);              // too long: refused, state stays empty
  CHECK(qr.encode("HELLO WORLD") && qr.version() == 1 && qr.size() == 21);
  CHECK(qr.bestScale(192, 63, 2) == 2 && qr.bestScale(192, 63, 4) == 2 && qr.bestScale(20, 20, 2) == 0);
  // draw at scale 2 with a 2-module quiet zone and compare every pixel with the module matrix
  lcd.clear(); lcd.fillRect(0, 0, 192, 63, 1);
  int s = qr.drawCentered(lcd); CHECK(s == 2);
  int total = (21 + 4) * 2, x0 = (192 - total) / 2, y0 = (63 - total) / 2;
  bool ok = true;
  for (int y = 0; y < total; y++) for (int x = 0; x < total; x++) {
    int mx = x / 2 - 2, my = y / 2 - 2; bool dark = qr.module(mx, my);
    if (lcd.getPixel(x0 + x, y0 + y) != (dark ? 3 : 0)) ok = false;
  }
  CHECK(ok);
  CHECK(lcd.getPixel(x0 - 1, y0 + 10) == 1 && lcd.getPixel(x0 + total, y0 + 10) == 1);   // nothing drawn outside the code area
  // finder pattern corner: dark module at (0,0), (6,0), (0,6) and light at (7,7) separator
  CHECK(qr.module(0, 0) && qr.module(6, 0) && qr.module(0, 6) && !qr.module(7, 7) && qr.module(20, 0) && qr.module(0, 20));
  // biggest version: scale 1, falls back to no quiet zone when 4 modules do not fit
  std::string big(250, 'x'); CHECK(qr.encode(big.c_str(), CasioQR::ECC_L, false) && qr.version() == 10);
  CHECK(qr.bestScale(192, 63, 4) == 0 && qr.bestScale(192, 63, 2) == 1);
  lcd.clear(); CHECK(qr.drawCentered(lcd, 4) == 1);
  CHECK(CasioQR::capacityBytes(1, CasioQR::ECC_L) == 17 && CasioQR::capacityBytes(1, CasioQR::ECC_H) == 7 && CasioQR::capacityBytes(10, CasioQR::ECC_L) == 271 && CasioQR::capacityBytes(11, CasioQR::ECC_L) == 0);
  // rotation: drawn in logical coordinates
  lcd.setRotation(1); lcd.clear(); CHECK(qr.encode("hi") && qr.drawCentered(lcd) >= 1); lcd.setRotation(0);
  printf(fails ? "QR %d FAILED\n" : "QR OK\n", fails); return fails;
}
