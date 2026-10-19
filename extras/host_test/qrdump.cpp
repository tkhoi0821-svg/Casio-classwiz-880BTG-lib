// host harness: qrdump <ecc 0..3> <boost 0/1> <minver> <text|@file>  -> "ver size mask ecc" then one row of 0/1 per line
#include <Arduino.h>
#include <SPI.h>
#include "CasioQR.h"
#include <stdio.h>
#include <string>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main(int, char** argv) {
  std::string t = argv[4];
  if (t[0] == '@') { FILE* f = fopen(argv[4] + 1, "rb"); t.clear(); int c; while ((c = fgetc(f)) != EOF) t += (char)c; fclose(f); }
  static CasioQR qr;
  if (!qr.encode(t.c_str(), (CasioQR::Ecc)atoi(argv[1]), atoi(argv[2]), atoi(argv[3]))) { printf("FAIL\n"); return 0; }
  printf("%d %d %d %d\n", qr.version(), qr.size(), qr.mask(), qr.ecc());
  for (int y = 0; y < qr.size(); y++) { for (int x = 0; x < qr.size(); x++) putchar(qr.module(x, y) ? '1' : '0'); putchar('\n'); }
}
