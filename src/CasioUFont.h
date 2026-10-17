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
  uint32_t off;   // byte offset into the bitmap
};
struct UFont {
  const UGlyph*  glyphs;
  const uint8_t* bitmap;   // per glyph: `height` rows of ceil(w/8) bytes, MSB first
  uint16_t count;
  uint8_t  height;         // cell height; the cursor y is the TOP of the cell
};
