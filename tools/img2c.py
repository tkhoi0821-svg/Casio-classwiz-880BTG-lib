#!/usr/bin/env python3
"""Convert an image to a C array for CasioST75256 (needs Pillow: pip install pillow).

  python img2c.py logo.png --name LOGO --width 48 --bits 2 > logo.h

--bits 1 : 1-bit mask for drawBitmap()         (dark pixel = 1)
--bits 2 : 4-gray image for drawBitmap2bpp()   (0 = white ... 3 = black)
--dither : Floyd-Steinberg dithering instead of plain thresholds
--invert : swap dark/light
"""
import argparse, sys
from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument('image'); ap.add_argument('--name', default='IMG')
ap.add_argument('--width', type=int); ap.add_argument('--height', type=int)
ap.add_argument('--bits', type=int, choices=(1, 2), default=2)
ap.add_argument('--dither', action='store_true'); ap.add_argument('--invert', action='store_true')
a = ap.parse_args()

im = Image.open(a.image).convert('RGBA')
bg = Image.new('RGBA', im.size, (255, 255, 255, 255)); bg.alpha_composite(im)
im = bg.convert('L')
if a.width or a.height:
    w = a.width or round(im.width * a.height / im.height)
    h = a.height or round(im.height * w / im.width)
    im = im.resize((w, h), Image.LANCZOS)
w, h = im.size
levels = 2 if a.bits == 1 else 4
px = [[255 - im.getpixel((x, y)) for x in range(w)] for y in range(h)]   # 0 = white .. 255 = black
if a.invert: px = [[255 - v for v in r] for r in px]
step = 255 / (levels - 1)
q = [[0] * w for _ in range(h)]
for y in range(h):
    for x in range(w):
        old = px[y][x]
        new = max(0, min(levels - 1, round(old / step)))
        q[y][x] = new
        if a.dither:
            err = old - new * step
            for dx, dy, f in ((1, 0, 7/16), (-1, 1, 3/16), (0, 1, 5/16), (1, 1, 1/16)):
                if 0 <= x + dx < w and y + dy < h: px[y + dy][x + dx] += err * f
per = 8 if a.bits == 1 else 4
data = []
for row in q:
    for i in range(0, w, per):
        b = 0
        for k in range(per):
            v = row[i + k] if i + k < w else 0
            b = (b << a.bits) | (v if a.bits == 2 else (1 if v else 0))
        data.append(b)
n = a.name
print(f'// {w}x{h}, {a.bits}-bit, from {a.image}')
print(f'#define {n}_W {w}\n#define {n}_H {h}')
print(f'const uint8_t {n}[] PROGMEM = {{')
for i in range(0, len(data), 16):
    print('  ' + ','.join(f'0x{b:02X}' for b in data[i:i+16]) + ',')
print('};')
