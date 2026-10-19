// Unicode bitmap font format used by CasioST75256 (build fonts with tools/font2c.py).
#pragma once
#include <stdint.h>
#ifndef PROGMEM
#define PROGMEM
#endif
struct UGlyph {
  uint16_t cp;    // Unicode code point (BMP), the table is sorted by cp
  uint8_t  w;     // bitmap width in pixels (8 or 16...)
  uint8_t  adv;   // cursor advance; 0 = combining mark drawn over the previous glyph (Thai vowels/tones)
                  // (a tone mark that follows another mark uses its 'raised' twin at cp|0xF000 when the font has one)
  uint32_t off;   // byte offset into the bitmap
};
struct UFont {
  const UGlyph*  glyphs;
  const uint8_t* bitmap;   // per glyph: `height` rows of ceil(w/8) bytes, MSB first
  uint16_t count;
  uint8_t  height;         // cell height; the cursor y is the TOP of the cell
  uint8_t  bpp;            // 0/1 = 1-bit glyphs, 2 = 2-bit antialiased (rows of ceil(w/4) bytes, 0 = transparent .. 3 = full ink)
};
