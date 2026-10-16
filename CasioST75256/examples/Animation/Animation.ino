// Animation demo (GrayKazu): bouncing ball + FPS counter.
// display() only sends the pages that changed, so small animations stay fast.
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

int bx = 20, by = 30, dx = 2, dy = 1;
unsigned long lastFps = 0;
int frames = 0, fps = 0;

void setup() {
  lcd.begin();
  lcd.clear();
  lcd.display();
}

void loop() {
  lcd.clear();
  lcd.drawRoundRect(0, 0, 192, 63, 4);
  lcd.fillCircle(bx, by, 5);
  lcd.fillTriangle(150, 50, 165, 50, 157, 38, 2);

  char buf[16];
  snprintf(buf, sizeof(buf), "FPS %d", fps);
  lcd.setTextSize(1);
  lcd.setCursor(6, 6);
  lcd.print(buf);
  lcd.printCentered("GrayKazu", 52);
  lcd.display();

  bx += dx; by += dy;
  if (bx < 7 || bx > 185) dx = -dx;
  if (by < 7 || by > 55)  dy = -dy;

  frames++;
  if (millis() - lastFps >= 1000) { fps = frames; frames = 0; lastFps = millis(); }
}
