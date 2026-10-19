// host harness: mjpegplay file out.bin [fit] [dither] -> plays an MJPEG file unpaced, dumps each frame
#include <Arduino.h>
#include <SPI.h>
#include "CasioMjpeg.h"
#include <stdio.h>
#include <vector>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "rb"); std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back(c); fclose(f);
  Stream s; s.data = d.data(); s.size = d.size(); s.trickle = 4096;
  std::vector<uint8_t> buf(1 << 20);
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  CasioMjpegPlayer p(lcd, s, buf.data(), buf.size()); p.setFps(0);
  p.decoder().setFit((CasioJpeg::Fit)(argc > 3 ? atoi(argv[3]) : 1)); p.decoder().setDither((CasioJpeg::Dither)(argc > 4 ? atoi(argv[4]) : 1));
  FILE* out = fopen(argv[2], "wb"); uint32_t now = 0; int idle = 0;
  while (idle < 3) { if (p.update(now)) { fwrite(p.frame(), 1, 3024, out); idle = 0; } else if (s.pos >= s.size) idle++; now += 5; }
  fclose(out); printf("shown=%u errors=%u\n", p.shown(), p.errors());
}
