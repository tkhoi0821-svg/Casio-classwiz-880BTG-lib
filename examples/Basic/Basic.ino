// Basic demo for CasioST75256 (by GrayKazu) (ESP32-S3 wiring from the notes: CS=14, A0=10, RST=11, SCK=12, SDA=13)
// Keep the calculator's own MCU in reset so it does not fight the bus.
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);                                   // ESP32-S3; on other boards use &SPI
CasioST75256 lcd(/*cs*/14, /*a0*/10, /*rst*/11, /*sck*/12, /*sda*/13, &lcdSpi);

void setup() {
  lcd.begin();
  lcd.clear();

  lcd.setTextSize(2);
  lcd.setCursor(4, 2);
  lcd.print("Hello!");

  lcd.setTextSize(1);
  lcd.setCursor(4, 22);
  lcd.print("ST75256 192x63");

  lcd.drawRoundRect(0, 0, 192, 63, 4);
  lcd.drawCircle(160, 40, 14);
  lcd.fillRect(100, 34, 30, 14, 2);        // gray
  for (int i = 0; i < 4; i++) lcd.fillRect(4 + i * 24, 44, 20, 14, i);  // 4 gray levels
  lcd.display();
}

void loop() {
  // blink the whole icon row once per second
  lcd.iconFill();  lcd.display(); delay(1000);
  lcd.iconClear(); lcd.display(); delay(1000);
}
