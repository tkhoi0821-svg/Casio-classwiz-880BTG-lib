// host harness: jpgdec in.jpg fit(0..2) dither(0..2) out_prefix  -> writes <prefix>.raw (3024 B) and <prefix>.gray (12096 B)
#include <Arduino.h>
#include "CasioJpeg.h"
#include <stdio.h>
#include <vector>
int g_a0 = 0, PIN_A0 = 10, PIN_CS = 14;
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "rb"); std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back(c); fclose(f);
  static CasioJpeg j; j.setFit((CasioJpeg::Fit)atoi(argv[2])); j.setDither((CasioJpeg::Dither)atoi(argv[3]));
  if (argc > 5) j.setBackground(atoi(argv[5]));
  uint8_t out[CasioJpeg::FRAME_BYTES]; memset(out, 0xAA, sizeof(out));
  int r = j.decode(d.data(), d.size(), out);
  printf("%d %d %d %d\n", r, j.srcWidth(), j.srcHeight(), j.scaleShift());
  if (r == 0) { char p[256]; snprintf(p, 256, "%s.raw", argv[4]); f = fopen(p, "wb"); fwrite(out, 1, sizeof(out), f); fclose(f);
                snprintf(p, 256, "%s.gray", argv[4]); f = fopen(p, "wb"); fwrite(j.gray(), 1, 12096, f); fclose(f); }
  else { for (size_t i = 0; i < sizeof(out); i++) if (out[i] != 0xAA) { printf("OUT WRITTEN ON ERROR\n"); break; } }
  return 0;
}
