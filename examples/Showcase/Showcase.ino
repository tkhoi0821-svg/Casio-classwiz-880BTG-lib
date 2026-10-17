// v0.3.0 showcase (GrayKazu): dither gradient, progress bar, 2-bit bitmap, rotation, invertRect.
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

// 8x8 image, 2 bits per pixel (2 bytes per row): a little gray diamond
const uint8_t DIAMOND[] PROGMEM = {
  0x00,0x00, 0x03,0xC0, 0x0F,0xF0, 0x3F,0xFC, 0x3F,0xFC, 0x0F,0xF0, 0x03,0xC0, 0x00,0x00
};

void drawScene() {
  lcd.clear();
  lcd.drawRoundRect(0, 0, lcd.width(), lcd.height(), 3);
  lcd.setCursor(4, 4);  lcd.print("Rot "); lcd.print(lcd.getRotation());
  lcd.printRight("v0.3", lcd.width() - 5, 4);
  for (int i = 0; i <= 16; i++)                       // 17-step gradient
    lcd.fillRectDither(4 + i * 3, 16, 3, 10, i, 0, 3);
  lcd.drawProgressBar(4, 30, 50, 9, 65);
  lcd.drawBitmap2bpp(4, 44, DIAMOND, 8, 8, 0);
  lcd.invertRect(16, 44, 20, 8);
  lcd.display();
}

void setup() {
  lcd.begin(4000000);          // 4 MHz; lower it if the picture is noisy
  lcd.setBlockWrite(true);     // one SPI transaction per column run
}

void loop() {
  for (uint8_t r = 0; r < 4; r++) {
    lcd.setRotation(r);
    drawScene();
    delay(2000);
  }
}
