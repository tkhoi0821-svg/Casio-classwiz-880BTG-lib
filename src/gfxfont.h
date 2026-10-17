// Font structs, binary-compatible with Adafruit GFX fonts (same include guard, so both libraries can coexist).
// Fonts made with Adafruit's fontconvert, or the FreeSans*/FreeMono* headers, work with CasioST75256::setFont().
#ifndef _GFXFONT_H_
#define _GFXFONT_H_
#include <stdint.h>
typedef struct {
  uint16_t bitmapOffset;  // offset into the bitmap array
  uint8_t  width, height; // bitmap size in pixels
  uint8_t  xAdvance;      // cursor advance
  int8_t   xOffset, yOffset;  // offset from the cursor (baseline) to the top-left of the bitmap
} GFXglyph;
typedef struct {
  uint8_t  *bitmap;
  GFXglyph *glyph;
  uint16_t first, last;
  uint8_t  yAdvance;      // line height
} GFXfont;
#endif
