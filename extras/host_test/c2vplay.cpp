// host harness: c2vplay file trickle loop(0/1) out.bin [fps]  -> plays unpaced (or at fps), dumps every shown frame
#include <Arduino.h>
#include <SPI.h>
#include "CasioC2v.h"
#include <stdio.h>
#include <vector>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "rb"); std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back(c); fclose(f);
  Stream s; s.data = d.data(); s.size = d.size(); s.trickle = atoi(argv[2]);
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  CasioC2vPlayer p(lcd, s);
  bool loop = atoi(argv[3]);
  p.setLoop(loop, [](Stream& st) { st.pos = 0; return true; });
  p.setFps(argc > 5 ? atof(argv[5]) : -1);
  FILE* out = fopen(argv[4], "wb");
  uint32_t now = 0; size_t want = loop ? 3 : 1; int guard = 0;
  unsigned target = 0;
  { // total frames to capture: header count * passes
    target = (d[10] | d[11] << 8 | d[12] << 16 | d[13] << 24) * want;
  }
  while ((!loop || p.shown() < target) && !p.finished() && !p.failed() && guard++ < 2000000) {
    if (p.update(now)) fwrite(p.frame(), 1, 3024, out);
    now += 7;
  }
  fclose(out);
  printf("shown=%u skipped=%u finished=%d failed=%d fps=%.2f count=%u now=%u\n", p.shown(), p.skipped(), p.finished(), p.failed(), p.fps(), p.frameCount(), now);
}
