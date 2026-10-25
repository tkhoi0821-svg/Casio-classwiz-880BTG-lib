#!/usr/bin/env python3
"""Build a CasioST75256 Unicode bitmap font header from any TTF/OTF (needs Pillow + fontTools).

  python font2c.py --font unifont.otf --charset vi --name UFONT_VI > CasioFont_VI.h
  python font2c.py --font MyFont.ttf --size 12 --baseline 10 --text "ありがとう" --name UFONT_JA > ja.h

Charsets (combine with commas): ascii, vi (Vietnamese), th (Thai), gb2312 (Simplified Chinese, ~4.4k glyphs),
big5 (Traditional Chinese, ~5.5k glyphs). --text / --textfile add your own characters; --scan DIR adds every non-ASCII character that appears in your sketch files
(a font with only the characters you really use: any size, any script, a few hundred bytes of flash).
Glyphs are 1-bit (default) or 2-bit antialiased with --aa (4 gray levels: the LCD blends them over any background),
one cell per glyph, `height` rows tall. --aa options: --index N (font number inside a .ttc), --engine raqm|basic,
--boost F (>1 darkens thin strokes), --width-pad N. Combining marks (category Mn) get adv=0 and are
drawn on top of the previous glyph; use --strip-circle with Unifont, whose marks carry a dotted circle.
"""
import argparse, sys, unicodedata
from PIL import Image, ImageDraw, ImageFont
import numpy as np
from fontTools.ttLib import TTFont

ap = argparse.ArgumentParser()
ap.add_argument('--font', required=True); ap.add_argument('--name', default='UFONT')
ap.add_argument('--size', type=int, default=16); ap.add_argument('--baseline', type=int, default=None,
                help='rows from the top of the cell to the baseline (default: size-2)')
ap.add_argument('--height', type=int, default=None, help='cell height (default: size)')
ap.add_argument('--charset', default=''); ap.add_argument('--text', default=''); ap.add_argument('--textfile')
ap.add_argument('--strip-circle', action='store_true'); ap.add_argument('--threshold', type=int, default=128)
ap.add_argument('--scan', action='append', default=[], help='file or folder: add every non-ASCII character used in its .ino/.h/.cpp/.c/.txt/.md/.json files (UTF-8); repeatable')
ap.add_argument('--aa', action='store_true', help='2-bit antialiased glyphs'); ap.add_argument('--index', type=int, default=0)
ap.add_argument('--engine', choices=['basic', 'raqm'], default='basic'); ap.add_argument('--boost', type=float, default=1.0)
ap.add_argument('--width-pad', type=int, default=0, help='extra advance pixels added to every glyph (letter spacing)')
a = ap.parse_args()
H = a.height or a.size; BASE = a.baseline if a.baseline is not None else a.size - 2

def decode_rows(codec, leads, trails):
    out = []
    for l in leads:
        for t in trails:
            try: out.append(bytes([l, t]).decode(codec))
            except UnicodeDecodeError: pass
    return out

def charset(name):
    if name == 'ascii': return [chr(c) for c in range(0x20, 0x7F)]
    if name == 'vi':
        s = set()
        for b in 'aăâeêioôơuưyAĂÂEÊIOÔƠUƯY':
            for m in ('', '\u0300', '\u0309', '\u0303', '\u0301', '\u0323'):
                n = unicodedata.normalize('NFC', b + m)
                if len(n) == 1: s.add(n)
        s.update('đĐ–—‘’“”…•·°€₫«»×÷')
        return sorted(s)
    if name == 'th': return [chr(c) for c in list(range(0x0E01, 0x0E3B)) + list(range(0x0E3F, 0x0E5C))]
    if name == 'gb2312':
        return decode_rows('gb2312', list(range(0xA1, 0xA4)) + list(range(0xB0, 0xD8)), range(0xA1, 0xFF))
    if name == 'big5':
        return decode_rows('big5', list(range(0xA1, 0xA4)) + list(range(0xA4, 0xC6)),
                           list(range(0x40, 0x7F)) + list(range(0xA1, 0xFF)))
    sys.exit('unknown charset ' + name)

chars = set()
for n in filter(None, a.charset.split(',')): chars.update(charset(n.strip()))
chars.update(a.text)
if a.textfile: chars.update(open(a.textfile, encoding='utf-8').read())
import os
def _scan(path):
    found, files = set(), 0
    paths = []
    if os.path.isdir(path):
        for root, _, names in os.walk(path):
            paths += [os.path.join(root, n) for n in names if n.lower().endswith(('.ino', '.h', '.cpp', '.c', '.txt', '.md', '.json')) and not n.startswith('CasioFont_')]
    else: paths = [path]
    for p in paths:
        try: text = open(p, encoding='utf-8', errors='ignore').read()
        except OSError: continue
        files += 1; found.update(ch for ch in text if ord(ch) > 127 and ord(ch) not in (0xFFFD, 0xFEFF, 0x200B))
    return found, files
for _p in a.scan:
    _f, _n = _scan(_p); chars.update(_f)
    print(f'// scan {_p}: {len(_f)} distinct non-ASCII characters in {_n} file(s)', file=sys.stderr)
chars = {c for c in chars if c not in '\n\r\t' and ord(c) <= 0xFFFF}
if not chars: sys.exit('no characters selected')

tt = TTFont(a.font, fontNumber=a.index); cmap = tt.getBestCmap()
face = ImageFont.truetype(a.font, a.size, index=a.index, layout_engine=ImageFont.Layout.RAQM if a.engine == 'raqm' else ImageFont.Layout.BASIC)
THAI_ABOVE_VOWEL = '\u0E34'; THAI_BASE = '\u0E01'
RAISED = [0x0E47, 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E]     # marks that get a 'raised' twin at cp|0xF000
if a.aa and any(0x0E47 <= ord(c) <= 0x0E4E for c in chars):
    chars = set(chars) | {chr(cp | 0xF000) for cp in RAISED if chr(cp) in chars}
missing = sorted(c for c in chars if ord(c) not in cmap and not (0xFE47 <= ord(c) <= 0xFE4E))
if missing: print('// skipped (not in font):', ''.join(missing)[:200], file=sys.stderr)
chars = sorted(c for c in chars if ord(c) in cmap or (0xFE47 <= ord(c) <= 0xFE4E))

def render_cov(text, w):
    im = Image.new('L', (w + 16, H + 16), 0)
    ImageDraw.Draw(im).text((0 + 8, 8 + BASE), text, font=face, fill=255, anchor='ls', features=['-kern'] if False else None)
    return np.asarray(im, dtype=np.int32)[8:8 + H, 8:8 + w]

def levels(cov):
    cov = np.minimum(255, (cov * a.boost)).astype(np.int32)
    return ((cov * 3 + 127) // 255).tolist()

def render(ch, w):
    im = Image.new('L', (w + 16, H + 16), 0)
    ImageDraw.Draw(im).text((8, 8 + BASE), ch, font=face, fill=255, anchor='ls')
    return [[1 if im.getpixel((8 + x, 8 + y)) >= a.threshold else 0 for x in range(w)] for y in range(H)]

circle = {}
if a.strip_circle and 0x25CC in cmap:
    circle[8] = render('\u25CC', 8)

# diagnostics: ink that would fall above the first row of the cell (stacked diacritics, tall CJK strokes)
_clip = []
for _ch in chars:
    try:
        _b = face.getbbox(chr(ord(_ch) & 0x0FFF) if 0xFE47 <= ord(_ch) <= 0xFE4E else _ch, anchor='ls')
    except Exception:
        continue
    if _b and -_b[1] > BASE and unicodedata.category(_ch) != 'Mn': _clip.append((_ch, -_b[1] - BASE))
if _clip:
    print(f'// WARNING: {len(_clip)} glyph(s) are clipped at the top by up to {max(c for _, c in _clip)} px; raise --baseline/--height or lower --size', file=sys.stderr)
    print('// clipped at the top: ' + ' '.join(f'{c}(-{n})' for c, n in _clip[:12]) + (' ...' if len(_clip) > 12 else ''), file=sys.stderr)
glyphs, bmp = [], bytearray()
base_adv = int(round(face.getlength(THAI_BASE))) if 0x0E01 in cmap else 8
for ch in chars:
    cp = ord(ch)
    raised = 0xFE47 <= cp <= 0xFE4E
    real = chr(cp & 0x0FFF) if raised else ch
    adv = int(round(face.getlength(real)))
    mark = unicodedata.category(real) == 'Mn'
    if a.aa:
        if mark and 0x0E00 <= ord(real) <= 0x0E7F:
            w = max(base_adv, 6)
            stem = THAI_BASE + (THAI_ABOVE_VOWEL if raised else '')
            pair = render_cov(stem + real, max(w, 24)); basec = render_cov(stem, max(w, 24))
            cov = np.clip(pair - basec, 0, 255)[:, :w]
            px = levels(cov); adv = 0
        else:
            if mark: adv = 0
            probe = render_cov(real, 48)
            cols = np.where(probe.max(axis=0) > 40)[0]
            ink_r = int(cols.max()) + 1 if len(cols) else 0
            w = max(adv, ink_r, 1) if not mark else max(ink_r, 6)
            if adv: adv = w + a.width_pad if False else adv + a.width_pad
            px = levels(render_cov(real, w))
        bpr = (w + 3) // 4
        glyphs.append((cp, w, adv if adv or mark else w, len(bmp)))
        for row in px:
            for i in range(bpr):
                b = 0
                for k in range(4): b = (b << 2) | (row[i * 4 + k] if i * 4 + k < w else 0)
                bmp.append(b)
        continue
    adv = int(round(face.getlength(ch)))
    mark = unicodedata.category(ch) == 'Mn'
    w = max(8, (adv + 7) // 8 * 8) if adv else 8
    px = render(ch, w)
    if mark and a.strip_circle and w == 8 and 8 in circle:
        px = [[p & (1 - c) for p, c in zip(r, cr)] for r, cr in zip(px, circle[8])]
    if mark: adv = 0
    elif adv == 0: adv = w
    bpr = (w + 7) // 8
    glyphs.append((cp, w, adv, len(bmp)))
    for row in px:
        for i in range(bpr):
            b = 0
            for k in range(8):
                b = (b << 1) | (row[i * 8 + k] if i * 8 + k < w else 0)
            bmp.append(b)

n = a.name
print(f'// Generated by tools/font2c.py from {a.font.split("/")[-1]}: {len(glyphs)} glyphs, {a.size}px, cell height {H}, {len(bmp)} bytes{", 2-bit antialiased" if a.aa else ""}')
print(f'#pragma once\n#include "CasioUFont.h"\n')
print(f'static const uint8_t {n}_BMP[] PROGMEM = {{')
for i in range(0, len(bmp), 24): print('  ' + ','.join(f'0x{b:02X}' for b in bmp[i:i+24]) + ',')
print('};')
print(f'static const UGlyph {n}_GLYPHS[] PROGMEM = {{')
for i in range(0, len(glyphs), 6):
    print('  ' + ' '.join(f'{{0x{c:04X},{w},{ad},{o}}},' for c, w, ad, o in glyphs[i:i+6]))
print('};')
print(f'static const UFont {n} = {{ {n}_GLYPHS, {n}_BMP, {len(glyphs)}, {H}, {2 if a.aa else 1} }};')
