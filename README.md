# CasioST75256 1.2.0

Author: GrayKazu

Arduino / ESP32 library for the 192x63 LCD (ST75256-compatible) of the fake Casio fx-880BTG and its icon row:
a 4-gray framebuffer with partial refresh, text in Vietnamese / Thai / Chinese, shapes, effects, a widget toolkit,
QR codes and video playback.

> **Status:** the author runs every release on a real unit (ESP32-S3) and reports it stable and light on resources
> through 1.0. The 1.1 and 1.2 additions (WraithUI 2, keyboard, icons, big fonts, word wrap, clip rectangle, font scanner) are verified on a host PC (unit tests,
> fuzzing, sanitizers, OpenCV, ffmpeg) and still need your run on the unit. See [Verified](#verified).

## Install
Arduino IDE: Sketch > Include Library > Add .ZIP Library, pick `CasioST75256.zip`. PlatformIO: `library.json` is included.
Needs the ESP32 Arduino core (the examples use `FSPI`, `LittleFS`, `esp_random`). The driver itself also builds with
`-std=gnu++11` and on other boards that have `SPIClass` (then `sck`/`sda` arguments are ignored).

## Wiring (ESP32-S3 used in the tests)
CS=GPIO14, A0=GPIO10, RST=GPIO11, SCK=GPIO12, SDA=GPIO13. Keep the calculator's own MCU in reset while the ESP drives the LCD.

## Quick start
```cpp
#include <CasioST75256.h>
SPIClass lcdSpi(FSPI);                              // elsewhere pass &SPI
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);      // cs, a0, rst, sck, sda
void setup() {
  lcd.begin();                                      // 1 MHz; lcd.begin(4000000) + lcd.setBlockWrite(true) for speed
  lcd.clear();
  lcd.setCursor(4, 4); lcd.print("Hello");
  lcd.fillCircle(150, 30, 12, 2);
  lcd.iconFill();                                   // light the icon row
  lcd.display();                                    // sends only what changed since the last call
}
void loop() {}
```

## Screen facts
- 192 columns (RAM column offset 0x40), 63 dot rows, plus the icon row (row 64 of the same RAM).
- 4 gray levels: 0 = white .. 3 = black. Coordinates are (0,0) top-left, `setRotation(0..3)` for portrait.
- Default orientation mirrorX = false, mirrorY = true (matches the unit it was tested on): `setMirror(x, y)` before `begin()`.
- `display()` keeps a copy of what the LCD holds and sends only the column runs that differ, so `clear()` + redraw every
  frame is cheap and an unchanged frame sends nothing.

## What is in the box
| Area | What you get | Header |
|---|---|---|
| Core | shapes, lines, round rects, triangles, arcs, thick lines, dithered fills, 1-bit and 2-bit bitmaps, scroll, invert, icon row (`defineIcon`), rotation, `readFrame2bpp` | `CasioST75256.h` |
| Text | built-in 5x7, Adafruit-GFX-format fonts (`setFont`), UTF-8 Unicode fonts (`addUFont`), `printCentered/printRight/getTextBounds/printf`, **word wrap** `printWrapped` (spaces, hyphens, CJK, never splits a letter from its marks), `setClipRect`, `dumpFrame` | same |
| Unicode fonts | Vietnamese, Thai, Simplified and Traditional Chinese: crisp 16 px (`CasioFont_VI.h`, `_TH`, `_ZH_CN`, `_ZH_TW`), compact antialiased 15 px with 4 lines on screen (`CasioFont_VI_15.h` ...), plus `CasioFont_VI_24.h` (24 px titles), `CasioFont_VI_15B.h` (bold) and `CasioFont_NUM_36.h` (36 px digits for clocks and readouts), and **53 vector icons** you can put inside text (`CasioFont_SYM_15.h` / `_16` / `_26`, strings `SYM_WIFI` ... in `CasioSymbols.h`, list in `docs/SYMBOLS.md`) | `CasioFont_*.h` |
| Effects | ghost trails, static, scanlines, glitch, melt, flicker, heartbeat, CRT-off, cursed text | `fx*`, `printCursed` |
| UI | **WraithUI 2**: 24 widgets (label, button, bar, slider, toggle, checkbox, list, gauge, chart, panel, textarea, input, dropdown, roller, spinbox, tabview, arc, meter, led, spinner, image, table, **keyboard**, custom), containers with column / row / wrap layout and scrolling, themes and styles, popups (dropdown, msgbox, toast), events, animations, timers, screen stack, transitions | `WraithUI.h`, `docs/WRAITHUI.md` |
| Console | scrolling terminal window with scrollback, UTF-8 aware | `CasioConsole.h` |
| Animation | 10 easing functions and a tween | `CasioTween.h` |
| QR codes | versions 1-10, numeric/alphanumeric/byte, all ECC levels, Wi-Fi helper | `CasioQR.h` |
| Video | `CasioJpeg` (JPEG -> 192x63 2 bpp), `CasioMjpegPlayer` (any `Stream`), `CasioC2vPlayer` (pre-baked, tiny, no JPEG decoding) | `CasioJpeg.h`, `CasioMjpeg.h`, `CasioC2v.h` |
| PC tools | `tools/img2c.py` (image -> array), `tools/font2c.py` (font -> header, 1-bit or antialiased; `--scan` keeps only the characters your sketch uses, so any size / any script costs a few hundred bytes), `tools/make_symbols.py` (icons at any size), `tools/c2vtool.py` (video/GIF -> .c2v and back), `tools/font_sizes.py` (flash cost of each font header), `tools/gen_api.py` | `tools/` |

API reference: `docs/API.md` (generated from the headers by `tools/gen_api.py`), WraithUI guide: `docs/WRAITHUI.md`,
Arduino IDE highlighting: `keywords.txt`. The examples folder has 16 sketches: `Basic`, `Animation`, `IconSweep`, `Showcase`, `Dashboard`, `Haunted`, `Fonts`,
`UnicodeDemo`, `UnicodeCompact`, `WraithDemo`, `WraithShowcase`, `WraithKeyboard`, `ConsoleDemo`, `QrDemo`, `MjpegPlayer`, `C2vPlayer`.

### Unicode text in three lines
```cpp
#include <CasioFont_VI_15.h>
#include <CasioFont_TH_15.h>
#include <CasioFont_ZH_CN_15.h>
lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_TH_15); lcd.addUFont(&UFONT_ZH_CN_15);   // searched in this order
lcd.print("Tiếng Việt / สวัสดี / 你好");                                                      // save the sketch as UTF-8
```
Only the font headers you `#include` take flash. Vietnamese must be precomposed (NFC). Missing glyphs print `?`.
**Read `FONTS-LICENSE.md` before you distribute firmware that contains these fonts.**

### Video
```
ffmpeg -i in.mp4 -an -vf "scale=384:-2,fps=15" -c:v mjpeg -q:v 6 video.mjpeg     # MJPEG, decoded on the ESP
python tools/c2vtool.py encode in.mp4 -o clip.c2v --fps 15                         # pre-baked, a few hundred bytes/frame
```
Both players read any Arduino `Stream` (LittleFS/SD `File`, `WiFiClient`, Serial), never block, keep the frame rate by
skipping late frames, and can loop (C2V). See `examples/MjpegPlayer` and `examples/C2vPlayer`.
`CasioJpeg` decodes baseline/extended-sequential Huffman JPEG, luma only, with reduced-size IDCT for big sources; it
refuses progressive and other unsupported files with an error code and never overwrites the output on error.

## Memory (approximate, 32-bit target)
| Object | RAM |
|---|---|
| `CasioST75256` (framebuffer + diff copy) | 6.4 KB |
| `CasioConsole` | 2 KB |
| `CasioQR` | 1.5 KB |
| `WraithUI` (40 widgets, 2 KB text pool, 2 slide snapshots; `WRAITH_MAX_WIDGETS` / `WRAITH_TEXT_POOL` shrink it) | about 12 KB |
| `CasioC2vPlayer` | 9.2 KB |
| `CasioJpeg` (+ 48 KB heap on the first decode) | 30 KB |
| `CasioMjpegPlayer` (+ your frame buffer, 16-32 KB typical) | 33.7 KB |

Flash for fonts (data only, glyph tables included; an unused header costs nothing, `python tools/font_sizes.py` prints this table): VI 16 px 6 KB, TH 16 px 4 KB, ZH_CN 16 px 158 KB, ZH_TW 16 px 224 KB; VI 15 px 10 KB, VI 15 px bold 10 KB, TH 15 px 8 KB, ZH_CN 15 px 270 KB, ZH_TW 15 px 383 KB, VI 24 px 24 KB, digits 36 px 24 KB, icons 3-9 KB per size.

## Verified
**On the real unit (author's report):** every release up to 1.0 runs stably and lightly on the ESP32-S3 unit.

**On a host PC** (`extras/host_test`, `./build.sh`; needs g++, Python 3, Pillow, numpy; ffmpeg and OpenCV enable the video and QR tests):
- framebuffer diffing (the fake LCD RAM always equals the framebuffer after `display()`), rotation, shapes, text, 1-bit/2-bit blending, UTF-8, Thai mark stacking, word wrap, clip rectangle
- WraithUI 2 (plus the keyboard widget and the 53 icons, each checked to draw only inside its own cell): layout positions (probed with custom widgets), scrolling that follows focus, clipping, tabs, dropdown/msgbox/toast, key repeat and long press, UTF-8 input editing, text area, table, events, animations, timers, screen stack, themes, pools; the WraithUI 1 tests still pass unchanged
- JPEG decoder against Pillow and against ffmpeg-made MJPEG, MJPEG splitter, C2V round trip and fuzzing, QR codes against OpenCV (all 40 version x ECC combinations)
- everything under AddressSanitizer + UBSan; sources build with `-std=gnu++11`; all 16 examples compile and run against stubs (`smoke.py`)

I have no per-feature measurements from the real unit (frame times, contrast range, SPI limits, how the 15 px fonts or a scanned
QR code look). The init sequence is copied from sniffed frames; `0x6E` and the delays around the OTP commands (E0/E3/E1) are unverified.

## Limits worth knowing
- Vietnamese capitals with two stacked marks (e.g. Ẩ Ẫ Ể Ổ at 12 px) lose the top pixel row in `CasioFont_VI_15.h`: the cell is 15 px tall so that 4 lines fit. `tools/font2c.py` prints a warning with the exact glyphs when you generate your own.
- WraithUI has no touch input and no content-sized containers (give panels an explicit size); popups are one at a time.
- Unicode fonts are BMP only, no right-to-left, no automatic Thai word-breaking; marks are positioned by a fixed rule, not full shaping.
- QR codes stop at version 10 (what fits 63 px at 1 px per module).
- JPEG: baseline only, luma only (that is the point), no progressive.
- No DMA / asynchronous SPI: `display()` blocks while it sends.

## License
Library code: see `LICENSE`. The font headers contain glyph data derived from third-party fonts with their own licenses:
`FONTS-LICENSE.md`.

History: `CHANGELOG.md`.
