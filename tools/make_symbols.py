#!/usr/bin/env python3
"""Generate the CasioST75256 symbol (icon) font: vector icons rendered at any size, antialiased to 4 gray levels.

  python make_symbols.py --size 12 --cell 15 --top 1 --name UFONT_SYM_15 --out ../src/CasioFont_SYM_15.h
  python make_symbols.py --list                       (names)
  python make_symbols.py --sheet ../docs/symbols.png  (picture of every icon)

Icons live in the Unicode private use area from U+E000. The matching UTF-8 string macros (SYM_WIFI ...) are in
src/CasioSymbols.h. Put the symbol font AFTER your text font: lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_SYM_15);
then lcd.print(SYM_WIFI " Home");
"""
import argparse, math, sys
from PIL import Image, ImageDraw

SS = 8    # supersampling

class Canvas:
    def __init__(self, size):
        self.n = size; self.k = size * SS / 12.0
        self.im = Image.new('L', (size * SS, size * SS), 0); self.d = ImageDraw.Draw(self.im)
    def p(self, x, y): return (x * self.k, y * self.k)
    def w(self, v): return max(1, int(round(v * self.k)))
    def line(self, pts, w=1.3, fill=255):
        P = [self.p(*q) for q in pts]; wd = self.w(w)
        self.d.line(P, fill=fill, width=wd, joint='curve')
        for q in P: self.d.ellipse([q[0] - wd / 2, q[1] - wd / 2, q[0] + wd / 2, q[1] + wd / 2], fill=fill)
    def poly(self, pts, fill=255): self.d.polygon([self.p(*q) for q in pts], fill=fill)
    def polyline_closed(self, pts, w=1.2, fill=255): self.line(list(pts) + [pts[0]], w, fill)
    def rect(self, x0, y0, x1, y1, fill=255): self.d.rectangle([x0 * self.k, y0 * self.k, x1 * self.k - 1, y1 * self.k - 1], fill=fill)
    def box(self, x0, y0, x1, y1, w=1.2, fill=255):
        self.rect(x0, y0, x1, y1, fill); self.rect(x0 + w, y0 + w, x1 - w, y1 - w, 0)
    def disc(self, cx, cy, r, fill=255): self.d.ellipse([(cx - r) * self.k, (cy - r) * self.k, (cx + r) * self.k, (cy + r) * self.k], fill=fill)
    def ring(self, cx, cy, r, w=1.2, fill=255): self.disc(cx, cy, r, fill); self.disc(cx, cy, r - w, 0)
    def arc(self, cx, cy, r, a0, a1, w=1.2, fill=255):
        self.d.arc([(cx - r) * self.k, (cy - r) * self.k, (cx + r) * self.k, (cy + r) * self.k], a0, a1, fill=fill, width=self.w(w))
    def result(self):
        small = self.im.resize((self.n, self.n), Image.BOX)
        return [[(small.getpixel((x, y)) * 3 + 127) // 255 for x in range(self.n)] for y in range(self.n)]

def star(c, cx, cy, ro, ri):
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5; r = ro if i % 2 == 0 else ri
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    c.poly(pts)

def battery(c, level):
    c.box(0.8, 3.2, 10.2, 8.8, 1.0); c.rect(10.2, 5, 11.4, 7)
    for i in range(level): c.rect(2.0 + i * 2.7, 4.4, 2.0 + i * 2.7 + 2.0, 7.6)

def speaker(c):
    c.poly([(1, 4.4), (3.6, 4.4), (6.6, 1.8), (6.6, 10.2), (3.6, 7.6), (1, 7.6)])

ICONS = []
def icon(name):
    def deco(f): ICONS.append((name, f)); return f
    return deco

@icon('WIFI')
def _(c):
    for r in (9.0, 6.0, 3.0): c.arc(6, 10, r, 227, 313, 1.25)
    c.disc(6, 10, 1.1)
@icon('BLUETOOTH')
def _(c): c.line([(3.2, 3.6), (8.6, 8.4), (6, 11), (6, 1), (8.6, 3.6), (3.2, 8.4)], 1.1)
@icon('BATTERY_EMPTY')
def _(c): battery(c, 0)
@icon('BATTERY_1')
def _(c): battery(c, 1)
@icon('BATTERY_2')
def _(c): battery(c, 2)
@icon('BATTERY_FULL')
def _(c): battery(c, 3)
@icon('CHARGE')
def _(c): c.poly([(7.2, 0.6), (2.6, 6.6), (5.6, 6.6), (4.6, 11.4), (9.6, 5.0), (6.6, 5.0)])
@icon('LEFT')
def _(c): c.line([(7.6, 1.8), (3.2, 6), (7.6, 10.2)], 1.7)
@icon('RIGHT')
def _(c): c.line([(4.4, 1.8), (8.8, 6), (4.4, 10.2)], 1.7)
@icon('UP')
def _(c): c.line([(1.8, 7.6), (6, 3.2), (10.2, 7.6)], 1.7)
@icon('DOWN')
def _(c): c.line([(1.8, 4.4), (6, 8.8), (10.2, 4.4)], 1.7)
@icon('PLAY')
def _(c): c.poly([(2.8, 1.4), (10.2, 6), (2.8, 10.6)])
@icon('PAUSE')
def _(c): c.rect(2.4, 1.6, 5.0, 10.4); c.rect(7.0, 1.6, 9.6, 10.4)
@icon('STOP')
def _(c): c.rect(2.2, 2.2, 9.8, 9.8)
@icon('HOME')
def _(c):
    c.line([(0.8, 6.0), (6, 1.4), (11.2, 6.0)], 1.3); c.box(2.4, 5.6, 9.6, 11.0, 1.1); c.rect(5.0, 7.6, 7.0, 11.0)
@icon('SETTINGS')
def _(c):
    c.ring(6, 6, 3.3, 1.5)
    for i in range(8):
        a = i * math.pi / 4; c.line([(6 + 3.4 * math.cos(a), 6 + 3.4 * math.sin(a)), (6 + 5.4 * math.cos(a), 6 + 5.4 * math.sin(a))], 1.6)
@icon('CHECK')
def _(c): c.line([(1.8, 6.4), (4.8, 9.4), (10.4, 2.6)], 1.8)
@icon('CLOSE')
def _(c): c.line([(2.4, 2.4), (9.6, 9.6)], 1.7); c.line([(9.6, 2.4), (2.4, 9.6)], 1.7)
@icon('PLUS')
def _(c): c.line([(6, 1.8), (6, 10.2)], 1.8); c.line([(1.8, 6), (10.2, 6)], 1.8)
@icon('MINUS')
def _(c): c.line([(1.8, 6), (10.2, 6)], 1.8)
@icon('WARNING')
def _(c): c.polyline_closed([(6, 1.0), (11.2, 10.6), (0.8, 10.6)], 1.2); c.line([(6, 4.4), (6, 7.4)], 1.4); c.disc(6, 9.0, 0.8)
@icon('INFO')
def _(c): c.ring(6, 6, 5.2, 1.1); c.disc(6, 3.6, 0.85); c.line([(6, 5.6), (6, 9)], 1.4)
@icon('BELL')
def _(c): c.arc(6, 5.2, 3.6, 180, 360, 1.3); c.poly([(2.4, 5.2), (9.6, 5.2), (10.6, 9.0), (1.4, 9.0)]); c.rect(1.4, 8.4, 10.6, 9.4); c.disc(6, 10.4, 1.0); c.rect(5.4, 0.8, 6.6, 2.0)
@icon('LOCK')
def _(c): c.arc(6, 5, 2.6, 180, 360, 1.4); c.line([(3.4, 5), (3.4, 5.6)], 1.4); c.line([(8.6, 5), (8.6, 5.6)], 1.4); c.rect(1.8, 5.4, 10.2, 11.2)
@icon('UNLOCK')
def _(c): c.arc(8.2, 4.6, 2.4, 180, 360, 1.4); c.line([(5.8, 4.6), (5.8, 5.6)], 1.4); c.rect(1.8, 5.4, 10.2, 11.2)
@icon('MAIL')
def _(c): c.box(0.8, 2.4, 11.2, 9.8, 1.1); c.line([(1.0, 2.8), (6, 6.8), (11.0, 2.8)], 1.1)
@icon('HEART')
def _(c): c.disc(3.9, 3.9, 2.8); c.disc(8.1, 3.9, 2.8); c.poly([(1.2, 4.9), (10.8, 4.9), (6, 11)])
@icon('STAR')
def _(c): star(c, 6, 6.4, 5.6, 2.4)
@icon('SUN')
def _(c):
    c.disc(6, 6, 2.5)
    for i in range(8):
        a = i * math.pi / 4; c.line([(6 + 3.8 * math.cos(a), 6 + 3.8 * math.sin(a)), (6 + 5.4 * math.cos(a), 6 + 5.4 * math.sin(a))], 1.2)
@icon('MOON')
def _(c): c.disc(6, 6, 5.0); c.disc(8.6, 4.4, 4.0, 0)
@icon('TRASH')
def _(c):
    c.line([(1.8, 3.0), (10.2, 3.0)], 1.3); c.rect(4.4, 1.2, 7.6, 2.4)
    c.polyline_closed([(2.8, 3.6), (9.2, 3.6), (8.5, 11.0), (3.5, 11.0)], 1.1); c.line([(6, 5), (6, 9.6)], 1.0)
@icon('EDIT')
def _(c): c.line([(2.6, 9.4), (9.0, 3.0)], 2.6); c.poly([(1.2, 10.8), (2.0, 8.2), (3.8, 10.0)]); c.line([(8.6, 1.6), (10.4, 3.4)], 1.0)
@icon('REFRESH')
def _(c): c.arc(6, 6, 4.2, 40, 340, 1.5); c.poly([(8.8, 0.6), (11.4, 4.6), (6.6, 4.4)])
@icon('SEARCH')
def _(c): c.ring(4.9, 4.9, 3.6, 1.4); c.line([(7.6, 7.6), (10.8, 10.8)], 2.0)
@icon('MENU')
def _(c):
    for y in (2.8, 6.0, 9.2): c.line([(1.8, y), (10.2, y)], 1.5)
@icon('POWER')
def _(c): c.arc(6, 6.6, 4.4, 300, 240, 1.5); c.line([(6, 0.8), (6, 6.2)], 1.7)
@icon('VOLUME')
def _(c): speaker(c); c.arc(6.8, 6, 2.7, -55, 55, 1.2); c.arc(6.8, 6, 5.0, -55, 55, 1.2)
@icon('MUTE')
def _(c): speaker(c); c.line([(8.0, 4.2), (11.0, 7.8)], 1.4); c.line([(11.0, 4.2), (8.0, 7.8)], 1.4)
@icon('CLOCK')
def _(c): c.ring(6, 6, 5.2, 1.1); c.line([(6, 6), (6, 3)], 1.2); c.line([(6, 6), (8.4, 7.4)], 1.2)
@icon('FILE')
def _(c): c.polyline_closed([(2.2, 0.8), (7.2, 0.8), (9.8, 3.4), (9.8, 11.2), (2.2, 11.2)], 1.1); c.line([(7.0, 0.8), (7.0, 3.6), (9.8, 3.6)], 1.0); c.line([(4, 6.2), (8, 6.2)], 1.0); c.line([(4, 8.6), (8, 8.6)], 1.0)
@icon('FOLDER')
def _(c): c.polyline_closed([(0.8, 2.6), (4.4, 2.6), (5.8, 4.0), (11.2, 4.0), (11.2, 10.4), (0.8, 10.4)], 1.2)
@icon('DOWNLOAD')
def _(c): c.line([(6, 1.0), (6, 7.6)], 1.6); c.line([(3.0, 5.2), (6, 8.2), (9.0, 5.2)], 1.6); c.line([(1.4, 10.4), (10.6, 10.4)], 1.5)
@icon('UPLOAD')
def _(c): c.line([(6, 9.0), (6, 2.4)], 1.6); c.line([(3.0, 4.8), (6, 1.8), (9.0, 4.8)], 1.6); c.line([(1.4, 10.4), (10.6, 10.4)], 1.5)
@icon('IMAGE')
def _(c): c.box(0.8, 1.6, 11.2, 10.4, 1.0); c.poly([(1.6, 9.6), (4.6, 5.0), (6.6, 7.6), (8.0, 6.0), (10.4, 9.6)]); c.disc(8.6, 3.8, 1.0)
@icon('USER')
def _(c): c.disc(6, 3.6, 2.4); c.arc(6, 11.2, 4.6, 180, 360, 1.8); c.rect(1.4, 10.4, 10.6, 11.4)
@icon('PIN')
def _(c): c.disc(6, 4.4, 3.6); c.poly([(2.8, 6.2), (9.2, 6.2), (6, 11.6)]); c.disc(6, 4.4, 1.3, 0)
@icon('SIGNAL')
def _(c):
    for i, h in enumerate((2.4, 4.8, 7.2, 9.6)): c.rect(1.4 + i * 2.5, 11 - h, 1.4 + i * 2.5 + 1.8, 11)
@icon('CLOUD')
def _(c): c.disc(4.0, 7.2, 2.8); c.disc(7.4, 5.6, 3.4); c.disc(9.2, 7.8, 2.2); c.rect(4.0, 7.4, 9.2, 10.0)
@icon('CALENDAR')
def _(c): c.box(1.0, 2.0, 11.0, 11.0, 1.0); c.rect(1.0, 2.0, 11.0, 4.4); c.rect(3.0, 0.8, 4.0, 3.0); c.rect(8.0, 0.8, 9.0, 3.0)
@icon('MUSIC')
def _(c): c.line([(4.6, 8.8), (4.6, 1.8), (10.0, 1.0), (10.0, 7.8)], 1.3); c.disc(3.4, 9.0, 2.0); c.disc(8.8, 8.2, 2.0)
@icon('KEYBOARD')
def _(c):
    c.box(0.6, 2.6, 11.4, 9.6, 1.0)
    for y in (4.4, 6.2): 
        for x in (2.4, 4.6, 6.8, 9.0): c.rect(x, y, x + 1.0, y + 0.9)
    c.rect(3.2, 7.8, 8.8, 8.6)
@icon('LIGHT')
def _(c): c.disc(6, 4.6, 3.6); c.rect(4.4, 8.4, 7.6, 9.4); c.rect(4.8, 9.8, 7.2, 10.8)
@icon('BAN')
def _(c): c.ring(6, 6, 5.2, 1.3); c.line([(2.6, 9.4), (9.4, 2.6)], 1.4)

def render(size):
    out = []
    for name, f in ICONS:
        c = Canvas(size); f(c); out.append((name, c.result()))
    return out

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--size', type=int, default=12); ap.add_argument('--cell', type=int, default=15)
    ap.add_argument('--top', type=int, default=1); ap.add_argument('--gap', type=int, default=1); ap.add_argument('--name', default='UFONT_SYM_15')
    ap.add_argument('--out'); ap.add_argument('--macros'); ap.add_argument('--list', action='store_true'); ap.add_argument('--sheet'); ap.add_argument('--preview')
    a = ap.parse_args()
    if a.list:
        for i, (n, _) in enumerate(ICONS): print(f'U+{0xE000 + i:04X} SYM_{n}')
        return
    if a.macros:
        with open(a.macros, 'w') as f:
            f.write('// UTF-8 strings for the symbol font icons (generated by tools/make_symbols.py). Use together with a symbol font:\n')
            f.write('//   lcd.addUFont(&UFONT_VI_15); lcd.addUFont(&UFONT_SYM_15);  lcd.print(SYM_WIFI " Home");\n#pragma once\n')
            for i, (n, _) in enumerate(ICONS):
                cp = 0xE000 + i; b = [0xE0 | (cp >> 12), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F)]
                f.write(f'#define SYM_{n:<14} "' + ''.join(f'\\x{x:02X}' for x in b) + f'"   // U+{cp:04X}\n')
        print('wrote', a.macros); return
    icons = render(a.size)
    if a.preview:
        for n, px in icons:
            if n == a.preview.upper():
                for r in px: print(''.join(' .+#'[v] for v in r))
        return
    if a.sheet:
        S = 6; cols = 10; rows = (len(icons) + cols - 1) // cols; pad = 4
        im = Image.new('L', (cols * (a.size * S + pad * 2), rows * (a.size * S + pad * 2)), 255)
        for i, (n, px) in enumerate(render(12)):
            big = Image.new('L', (12, 12)); big.putdata([255 - v * 85 for r in px for v in r])
            im.paste(big.resize((12 * S, 12 * S), Image.NEAREST), ((i % cols) * (a.size * S + pad * 2) + pad, (i // cols) * (a.size * S + pad * 2) + pad))
        im.save(a.sheet); print('wrote', a.sheet); return
    if not a.out: sys.exit('--out required')
    w = a.size; adv = w + a.gap; bpr = (w + 3) // 4; bmp = bytearray(); glyphs = []
    for i, (n, px) in enumerate(icons):
        glyphs.append((0xE000 + i, w, adv, len(bmp)))
        for y in range(a.cell):
            row = px[y - a.top] if 0 <= y - a.top < w else [0] * w
            for bx in range(bpr):
                b = 0
                for k in range(4):
                    x = bx * 4 + k; b = (b << 2) | (row[x] if x < w else 0)
                bmp.append(b)
    with open(a.out, 'w') as f:
        f.write(f'// Generated by tools/make_symbols.py: {len(glyphs)} vector icons, {a.size}px, cell height {a.cell}, {len(bmp)} bytes, 2-bit antialiased\n')
        f.write('// Original artwork of this library (MIT, see LICENSE). Add AFTER your text font; strings: CasioSymbols.h\n#pragma once\n#include "CasioUFont.h"\n\n')
        f.write(f'static const uint8_t {a.name}_BMP[] PROGMEM = {{\n')
        for i in range(0, len(bmp), 24): f.write('  ' + ','.join(f'0x{b:02X}' for b in bmp[i:i + 24]) + ',\n')
        f.write('};\n' + f'static const UGlyph {a.name}_GLYPHS[] PROGMEM = {{\n')
        for i in range(0, len(glyphs), 6): f.write('  ' + ' '.join(f'{{0x{c:04X},{ww},{ad},{o}}},' for c, ww, ad, o in glyphs[i:i + 6]) + '\n')
        f.write('};\n' + f'static const UFont {a.name} = {{ {a.name}_GLYPHS, {a.name}_BMP, {len(glyphs)}, {a.cell}, 2 }};\n')
    print('wrote', a.out, len(glyphs), 'icons')
if __name__ == '__main__': main()
