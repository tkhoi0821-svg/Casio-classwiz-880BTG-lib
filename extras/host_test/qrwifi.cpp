// host harness: qrwifi <ssid> <pass> <type> -> same output format as qrdump
#include <Arduino.h>
#include <SPI.h>
#include "CasioQR.h"
#include <stdio.h>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14; SPIClass SPI; uint8_t SPIClass::transfer(uint8_t) { return 0; } void csEdge(int) {}
int main(int, char** argv) {
  static CasioQR qr;
  if (!qr.wifi(argv[1], argv[2], argv[3])) { printf("FAIL\n"); return 0; }
  printf("%d %d %d %d\n", qr.version(), qr.size(), qr.mask(), qr.ecc());
  for (int y = 0; y < qr.size(); y++) { for (int x = 0; x < qr.size(); x++) putchar(qr.module(x, y) ? '1' : '0'); putchar('\n'); }
}
