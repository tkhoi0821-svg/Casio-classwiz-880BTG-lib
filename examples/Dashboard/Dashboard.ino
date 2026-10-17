// v0.4 dashboard demo (GrayKazu): arc gauge, live graph, printf, thick line, contrast fade.
#include <CasioST75256.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

int16_t samples[48];
uint8_t head = 0;

void setup() {
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  for (uint8_t i = 0; i < 48; i++) samples[i] = 50;
}

void loop() {
  static float t = 0;
  t += 0.07f;
  int value = 50 + (int)(45 * sinf(t));                 // fake sensor 5..95
  samples[head] = value; head = (head + 1) % 48;
  int16_t ordered[48];
  for (uint8_t i = 0; i < 48; i++) ordered[i] = samples[(head + i) % 48];

  lcd.clear();
  // gauge: light track, dark value arc, needle
  const int16_t cx = 32, cy = 46;
  lcd.fillArc(cx, cy, 20, 27, 180, 360, 1);
  lcd.fillArc(cx, cy, 20, 27, 180, 180 + value * 180 / 100, 3);
  float rad = (180 + value * 1.8f) * 0.0174533f;
  lcd.drawThickLine(cx, cy, cx + (int)(17 * cosf(rad)), cy + (int)(17 * sinf(rad)), 2);
  lcd.setCursor(cx - 9, 52); lcd.printf("%3d%%", value);
  // graph on the right
  lcd.drawGraph(76, 4, 112, 40, ordered, 48, 0, 100);
  lcd.setCursor(76, 50); lcd.printf("t=%.1f", t);
  lcd.display();
}
