#!/usr/bin/env python3
"""Generate docs/API.md from the public parts of the library headers (so it always matches the code).
   python tools/gen_api.py            (run from the library root)"""
import os, re, sys
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
HEADERS = [('CasioST75256.h', 'CasioST75256 (core driver)'), ('WraithUI.h', 'WraithUI (widget toolkit)'), ('CasioConsole.h', 'CasioConsole'),
           ('CasioQR.h', 'CasioQR'), ('CasioJpeg.h', 'CasioJpeg'), ('CasioMjpeg.h', 'CasioMjpegReader / CasioMjpegPlayer'),
           ('CasioC2v.h', 'CasioC2vPlayer'), ('CasioTween.h', 'CasioTween / CasioEase'), ('CasioUFont.h', 'UFont / UGlyph (Unicode font format)'),
           ('gfxfont.h', 'GFXfont (Adafruit GFX font format)')]

def split_comment(line):
    in_s = False
    for i, ch in enumerate(line):
        if ch == '"': in_s = not in_s
        if not in_s and line[i:i + 2] == '//': return line[:i].rstrip(), line[i + 2:].strip()
    return line.rstrip(), ''

def parse(path):
    lines = open(path, encoding='utf-8').read().split('\n')
    head = []
    for l in lines:                                   # leading comment block = file description
        if l.startswith('//'): head.append(l[2:].strip())
        elif l.strip() == '' and head: continue
        elif head or l.startswith('#pragma'): break
    out = []                                           # (kind, text): kind in 'section', 'class', 'item'
    depth = 0; cls_stack = []; active = False; comments = []; decl = ''; trail = []
    i = 0
    while i < len(lines):
        raw = lines[i]; i += 1; s = raw.strip()
        m = re.match(r'^(class|struct)\s+(\w+)(\s*:\s*[^{]*)?\s*\{', s)
        if m and not cls_stack:
            cls_stack.append((m.group(2), m.group(1) == 'struct')); active = m.group(1) == 'struct'; depth = 1
            out.append(('class', m.group(1) + ' ' + m.group(2) + (m.group(3) or ''))); comments = []; continue
        if not cls_stack:
            if re.match(r'^(enum|typedef|struct|namespace|inline|#define)', s) and not s.endswith('{'):
                pass
            continue
        if depth == 1 and re.match(r'^(public|private|protected):', s):
            active = s.startswith('public'); comments = []; decl = ''; continue
        code, com = split_comment(raw)
        if code.strip() == '' and decl == '':
            if com:
                sec = re.match(r'^[-=]{2,}\s*(.+?)\s*[-=]*\s*$', com)
                if sec and active: out.append(('section', sec.group(1))); comments = []
                else: comments.append(com)
            else: comments = []
            continue
        if com: trail.append(com)
        decl = (decl + ' ' + code.strip()).strip()
        depth_now = decl.count('{') - decl.count('}')
        done = decl.endswith(';') or (decl.endswith('}') and depth_now == 0 and '(' in decl and not decl.endswith('};')) or decl.endswith('};')
        if decl.count('(') != decl.count(')'): done = False
        if done and depth_now <= 0:
            d = decl; decl = ''
            allc = comments + trail; comments = []; trail = []
            if d == '};' and depth == 1:
                cls_stack.pop(); depth = 0; active = False; continue
            if active and not d.startswith('using') and not d.startswith('friend'):
                sig = d.split('{')[0].strip().rstrip(';') if re.search(r'\)\s*(const)?\s*(override)?\s*\{', d) else d.rstrip(';')
                sig = re.sub(r'\)\s*:\s*\w+\(.*$', ')', sig)          # drop constructor initialiser lists
                if re.search(r'\(|^enum|^typedef|^static const|^struct', sig):
                    out.append(('item', sig, ' '.join(allc)))
        elif not (decl.endswith(';') or decl.endswith('{') or decl.endswith(',') or decl.endswith(')')) and decl == '':
            pass
    return head, out

md = ['# API reference', '', 'Generated from the headers in `src/` by `tools/gen_api.py` (do not edit by hand). Short guides: `README.md`, `docs/WRAITHUI.md`.', '']
for fn, title in HEADERS:
    p = os.path.join(ROOT, 'src', fn)
    if not os.path.exists(p): continue
    head, items = parse(p)
    md += [f'## {title}', f'`#include <{fn}>`', '']
    if head: md += ['> ' + ' '.join(h for h in head if h)[:900], '']
    for it in items:
        if it[0] == 'class': md += [f'### `{it[1]}`', '']
        elif it[0] == 'section': md += [f'#### {it[1]}', '']
        else:
            sig = re.sub(r'\s+', ' ', it[1]); md.append(f'- `{sig}`' + (f' - {it[2]}' if it[2] else ''))
    md.append('')
os.makedirs(os.path.join(ROOT, 'docs'), exist_ok=True)
open(os.path.join(ROOT, 'docs', 'API.md'), 'w', encoding='utf-8').write('\n'.join(md))
print('wrote docs/API.md', len(md), 'lines')
