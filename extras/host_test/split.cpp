// host harness: split in.bin chunk -> prints "<len> <hash>" per frame found, then dropped count
#include <Arduino.h>
#include <SPI.h>
#include "CasioMjpeg.h"
#include <stdio.h>
#include <vector>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "rb"); std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back(c); fclose(f);
  Stream s; s.data = d.data(); s.size = d.size(); s.trickle = atoi(argv[2]);
  size_t cap = argc > 3 ? atoi(argv[3]) : 65536; std::vector<uint8_t> bv(cap); uint8_t* buf = bv.data();
  CasioMjpegReader r(s, buf, cap); size_t len; int idle = 0;
  while (idle < 3) {
    if (r.poll(&len)) { uint32_t h = 2166136261u; for (size_t i = 0; i < len; i++) h = (h ^ buf[i]) * 16777619u; printf("%zu %u\n", len, h); idle = 0; }
    else { if (s.pos >= s.size) idle++; }
  }
  printf("dropped %u\n", r.dropped());
}
