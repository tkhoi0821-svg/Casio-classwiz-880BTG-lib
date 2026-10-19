# CasioST75256 1.0.0

Author: GrayKazu

Arduino / ESP32 library for the 192x63 LCD (ST75256-compatible) of the fake Casio fx-880BTG and its icon row:
a 4-gray framebuffer with partial refresh, text in Vietnamese / Thai / Chinese, shapes, effects, a widget toolkit,
QR codes and video playback.

> **Status:** developed and tested on a host PC (unit tests, fuzzing, sanitizers, OpenCV, ffmpeg). Only the core driver
> (init, drawing, 4-gray, icon row as a whole) has been tried on a real unit. Everything else is **not verified on
> hardware yet**, see [Verified vs not verified](#verified-vs-not-verified).

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
| Text | built-in 5x7, Adafruit-GFX-format fonts (`setFont`), UTF-8 Unicode fonts (`addUFont`), `printCentered/printRight/getTextBounds/printf` | same |
| Unicode fonts | Vietnamese, Thai, Simplified and Traditional Chinese, in two families: crisp 16 px (`CasioFont_VI.h`, `_TH`, `_ZH_CN`, `_ZH_TW`) and compact antialiased 15 px, 4 lines on screen (`CasioFont_VI_15.h` ...) | `CasioFont_*.h` |
| Effects | ghost trails, static, scanlines, glitch, melt, flicker, heartbeat, CRT-off, cursed text | `fx*`, `printCursed` |
| UI | `WraithUI`: label, button, bar, slider, toggle, checkbox, list, gauge, chart; screens, focus navigation, tweening, transitions (glitch, static, melt, CRT, slide) | `WraithUI.h` |
| Console | scrolling terminal window with scrollback, UTF-8 aware | `CasioConsole.h` |
| Animation | 10 easing functions and a tween | `CasioTween.h` |
| QR codes | versions 1-10, numeric/alphanumeric/byte, all ECC levels, Wi-Fi helper | `CasioQR.h` |
| Video | `CasioJpeg` (JPEG -> 192x63 2 bpp), `CasioMjpegPlayer` (any `Stream`), `CasioC2vPlayer` (pre-baked, tiny, no JPEG decoding) | `CasioJpeg.h`, `CasioMjpeg.h`, `CasioC2v.h` |
| PC tools | `tools/img2c.py` (image -> array), `tools/font2c.py` (font -> header, 1-bit or antialiased), `tools/c2vtool.py` (video/GIF -> .c2v and back) | `tools/` |

All the public functions are listed in `keywords.txt` (Arduino IDE highlighting) and commented in the headers. The
examples folder has 14 sketches: `Basic`, `Animation`, `IconSweep`, `Showcase`, `Dashboard`, `Haunted`, `Fonts`,
`UnicodeDemo`, `UnicodeCompact`, `WraithDemo`, `ConsoleDemo`, `QrDemo`, `MjpegPlayer`, `C2vPlayer`.

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
| `WraithUI` (24 widgets + 2 slide snapshots) | 8.3 KB |
| `CasioC2vPlayer` | 9.2 KB |
| `CasioJpeg` (+ 48 KB heap on the first decode) | 30 KB |
| `CasioMjpegPlayer` (+ your frame buffer, 16-32 KB typical) | 33.7 KB |

Flash for fonts: VI 16 px 25 KB, TH 16 px 19 KB, ZH_CN 16 px 129 KB, ZH_TW 16 px 182 KB; VI 15 px 8 KB, TH 15 px 6 KB,
ZH_CN 15 px 245 KB, ZH_TW 15 px 350 KB (data only; unused headers cost nothing).

## Verified vs not verified
**Tried on a real unit** (ESP32-S3, SPI mode 0, 1 MHz): init, drawing, 4-gray, the icon row as a whole.

**Verified on a host PC only** (`extras/host_test`, run `./build.sh`; needs g++, Python 3, Pillow, numpy; ffmpeg and OpenCV enable the video and QR tests):
- framebuffer diffing (the fake LCD RAM always equals the framebuffer after `display()`), rotation, shapes, text, 1-bit/2-bit blending, UTF-8, Thai mark stacking
- WraithUI navigation/events/tweening/transitions (slide frames compared pixel by pixel), console, easing
- JPEG decoder against Pillow on 12 synthetic images and against ffmpeg-made MJPEG (4:2:0, 4:4:4, 4:2:2, odd size: about 1 % of the 4-gray pixels differ), plus truncated / garbage / bit-flipped input
- MJPEG stream splitter (1-byte chunks, multipart junk, EXIF thumbnails, oversized frames), C2V round trip, looping, pacing, 40 corrupted-file fuzz runs
- QR codes: all 40 full-capacity version x ECC combinations decode with OpenCV, plus modes, UTF-8, Wi-Fi escaping, damage tolerance
- everything under AddressSanitizer + UBSan; sources build with `-std=gnu++11`; all 14 examples compile and run against stubs

**Not verified on hardware:** `setVop()` / `fadeTo()` / `fxFlicker()` / `fxHeartbeat()` (the contrast range), `setDisplayInverse()`,
`sleep()`, `setBlockWrite(true)`, SPI above 1 MHz (try 4 MHz and lower it if the picture is noisy), the icon name table
(use `IconSweep`, then `defineIcon`), legibility of the antialiased 15 px fonts on the real panel, scanning a QR code off the
real LCD, and **all timing**: decode/frame rates were only measured on a PC. The init sequence is copied from sniffed
frames; `0x6E` and the delays around the OTP commands (E0/E3/E1) are unverified.

## Limits worth knowing
- Unicode fonts are BMP only, no right-to-left, no automatic Thai word-breaking; marks are positioned by a fixed rule, not full shaping.
- QR codes stop at version 10 (what fits 63 px at 1 px per module).
- JPEG: baseline only, luma only (that is the point), no progressive.
- No DMA / asynchronous SPI: `display()` blocks while it sends.

## License
Library code: see `LICENSE`. The font headers contain glyph data derived from third-party fonts with their own licenses:
`FONTS-LICENSE.md`.

History: `CHANGELOG.md`.
