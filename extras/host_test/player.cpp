// host harness for CasioMjpegPlayer: player stream.bin  (40 frames, frame 5 is progressive = undecodable)
#include <Arduino.h>
#include <SPI.h>
#include "CasioMjpeg.h"
#include <stdio.h>
#include <vector>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(int, char** argv) {
  FILE* f = fopen(argv[1], "rb"); std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back(c); fclose(f);
  Stream s; s.data = d.data(); s.size = d.size(); s.trickle = 300;                 // data trickles in like a network stream
  std::vector<uint8_t> buf(65536);
  SPIClass sp; CasioST75256 lcd(14, 10, 11, 12, 13, &sp); lcd.begin();
  CasioMjpegPlayer p(lcd, s, buf.data(), buf.size());
  p.setFps(10);
  uint32_t now = 0; int guard = 0;
  while (!p.update(now) && guard++ < 1000) {}                  // first frame: shown immediately once enough bytes arrived
  CHECK(p.shown() == 1);
  int n = 0; for (int y = 0; y < 63; y++) for (int x = 0; x < 192; x++) if (lcd.getPixel(x, y)) n++;
  CHECK(n > 1000);                                              // a real picture was drawn
  CHECK(p.lastResult() == 0);
  now = 50; CHECK(!p.update(now));                              // not due yet
  now = 100; guard = 0; while (!p.update(now) && guard++ < 1000) {} CHECK(p.shown() == 2);
  // frames 3,4 normal; frame 5 is undecodable
  for (now = 200; now <= 600; now += 100) { guard = 0; while (!p.update(now) && guard++ < 50) {} }
  CHECK(p.errors() == 1);                                       // exactly the progressive frame, playback continued
  uint32_t before = p.shown();
  now = 3000;                                                   // long stall: frames must be skipped, not played in slow motion
  guard = 0; while (!p.update(now) && guard++ < 50) {}
  CHECK(p.skipped() >= 8);
  CHECK(p.shown() == before + 1);
  now += 100; guard = 0; while (!p.update(now) && guard++ < 50) {}
  CHECK(p.shown() == before + 2);                               // back to normal pacing
  p.setFps(0);                                                  // no pacing: every call shows a frame
  uint32_t s0 = p.shown(); guard = 0; while (p.shown() < s0 + 3 && guard++ < 500) p.update(now);
  CHECK(p.shown() >= s0 + 3);
  printf(fails ? "PLAYER %d FAILED\n" : "PLAYER OK (shown %u, skipped %u, errors %u)\n", fails ? fails : p.shown(), p.skipped(), p.errors());
  return fails;
}
