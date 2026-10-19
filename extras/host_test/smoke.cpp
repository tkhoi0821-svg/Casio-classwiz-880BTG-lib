// Smoke-test harness: includes an example sketch, runs setup() and N x loop(), reports how much was drawn.
//   g++ -DSKETCH='"../../examples/Basic/Basic.ino"' smoke.cpp ...      (run: ./smoke <iterations> [serial input])
#include <Arduino.h>
#include <SPI.h>
unsigned long g_ms = 0;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; }
#include SKETCH
SerialClass Serial; FSClass LittleFS;
int main(int argc, char** argv) {
  srand(1);
  if (argc > 2) { std::string in = argv[2]; for (size_t i = 0; i + 1 < in.size(); i++) if (in[i] == '\\' && in[i + 1] == 'n') { in.replace(i, 2, "\n"); } Serial.feed(in.c_str()); }
  setup();
  int n = argc > 1 ? atoi(argv[1]) : 3;
  for (int i = 0; i < n; i++) loop();
  int ink = 0, grays = 0, icons = 0;
  for (int y = 0; y < 63; y++) for (int x = 0; x < 192; x++) { int p = lcd.getPixel(x, y); if (p) ink++; if (p == 1 || p == 2) grays++; }
  for (int x = 0; x < 192; x++) icons += lcd.frameBuffer()[15 * 192 + x] & 3 ? 1 : 0;
  fprintf(stderr, "SMOKE ink=%d gray=%d iconcols=%d t=%lums\n", ink, grays, icons, g_ms);
}
