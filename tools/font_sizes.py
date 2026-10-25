#!/usr/bin/env python3
"""Flash cost of the font headers (data only; a header you do not #include costs nothing).
   python tools/font_sizes.py [src folder]"""
import glob, os, re, sys
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')
rows = []
for p in sorted(glob.glob(os.path.join(src, 'CasioFont_*.h'))):
    first = open(p, encoding='utf-8').readline()
    m = re.search(r'(\d+) (?:glyphs|vector icons), (\d+)px, cell height (\d+), (\d+) bytes', first)
    if m:
        g, px, h, b = map(int, m.groups()); rows.append((os.path.basename(p), g, px, h, b + 8 * g))
print(f'{"header":28s} {"glyphs":>7s} {"px":>4s} {"cell":>5s} {"flash":>9s}')
for r in rows: print(f'{r[0]:28s} {r[1]:7d} {r[2]:4d} {r[3]:5d} {r[4]/1024:8.1f}K')
print(f'{"all headers together":28s} {"":>7s} {"":>4s} {"":>5s} {sum(r[4] for r in rows)/1024:8.1f}K')
