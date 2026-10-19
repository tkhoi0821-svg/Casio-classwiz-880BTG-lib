// QR codes on the 192x63 LCD (GrayKazu): a Wi-Fi sign-in code and a URL, alternating, with a caption.
#include <CasioST75256.h>
#include <CasioQR.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
CasioQR qr;

void show(bool wifi) {
  lcd.clear();
  bool ok = wifi ? qr.wifi("MyHomeWiFi", "s3cret;pass")        // special characters are escaped for you
                 : qr.encode("https://example.com/hello", CasioQR::ECC_L);
  if (!ok) { lcd.setCursor(0, 0); lcd.print("text too long"); lcd.display(); return; }
  uint8_t scale = qr.bestScale(63, 63, 2);                      // keep the left 63 px square for the code
  qr.draw(lcd, 0, (63 - (qr.size() + 4) * scale) / 2, scale, 2);
  lcd.setCursor(76, 8);  lcd.print(wifi ? "Wi-Fi" : "Website");
  lcd.setCursor(76, 24); lcd.print("version "); lcd.print(qr.version());
  lcd.setCursor(76, 36); lcd.print("scale "); lcd.print(scale); lcd.print(" px");
  lcd.display();
}

void setup() {
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
}
void loop() {
  static bool wifi = true;
  show(wifi); wifi = !wifi;
  delay(5000);
  // qr.drawCentered(lcd);   // alternative: biggest code that fits, centered
}
