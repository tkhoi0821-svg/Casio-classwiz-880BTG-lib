#!/usr/bin/env python3
"""Tools: font2c --scan, make_symbols consistency, font_sizes."""
import os, re, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.join(HERE, '..', '..'); TMP = tempfile.mkdtemp(); fails = 0
def check(c, m):
    global fails
    if not c: print('FAIL:', m); fails += 1
NOTO = '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
if os.path.exists(NOTO):
    d = os.path.join(TMP, 'sk'); os.makedirs(d)
    open(os.path.join(d, 'a.ino'), 'w', encoding='utf-8').write('// Hello 你好世界\nconst char* s = "こんにちは ก \U0001F600";\n')
    open(os.path.join(d, 'b.h'), 'w', encoding='utf-8').write('#define X "世界"\n')
    open(os.path.join(d, 'CasioFont_X.h'), 'w', encoding='utf-8').write('// 龘 must be ignored: generated font header\n')
    r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'font2c.py'), '--font', NOTO, '--index', '2', '--charset', 'ascii', '--scan', d,
                        '--size', '16', '--height', '16', '--baseline', '13', '--aa', '--name', 'UFONT_SCAN'], capture_output=True, text=True)
    check(r.returncode == 0, 'font2c --scan ' + r.stderr[-300:])
    open(os.path.join(TMP, 'scan.h'), 'w').write(r.stdout)
    cps = {int(m, 16) for m in re.findall(r'\{0x([0-9A-F]{4}),\d+,\d+,\d+\}', r.stdout)}
    want = set(range(32, 127)) | {ord(c) for c in '你好世界こんにちは'}
    check(cps == want, f'scan glyph set: extra {sorted(hex(c) for c in cps - want)[:5]} missing {sorted(hex(c) for c in want - cps)[:5]}')
    check(ord('龘') not in cps and 0x1F600 not in cps, 'generated headers and non-BMP are ignored')
    check('scan' in r.stderr and 'files' in r.stderr or 'file(s)' in r.stderr, 'scan summary printed')
    check(len(r.stdout) < 60000, 'only the used characters are stored')
    b = subprocess.run(['g++', '-std=c++17', '-I', HERE, '-I', TMP, '-I', os.path.join(ROOT, 'src'), os.path.join(HERE, 'scanrender.cpp'), os.path.join(ROOT, 'src', 'CasioST75256.cpp'), '-o', os.path.join(TMP, 'sr')], capture_output=True, text=True)
    check(b.returncode == 0, 'compile scanned font ' + b.stderr[-300:])
    if b.returncode == 0:
        o = subprocess.run([os.path.join(TMP, 'sr')], capture_output=True, text=True).stdout
        m = re.search(r'count=(\d+) height=(\d+) width_cjk=(\d+) ink=(\d+)', o)
        check(m and int(m.group(1)) == len(want) and int(m.group(2)) == 16 and int(m.group(3)) >= 28 and int(m.group(4)) > 300, 'scanned font renders: ' + o.strip())
else:
    print('Noto CJK not installed: --scan test skipped')
# symbol macros agree with the generator's list
lst = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'make_symbols.py'), '--list'], capture_output=True, text=True).stdout.strip().split('\n')
mac = re.findall(r'#define (SYM_\w+)\s+"(\\x..\\x..\\x..)"\s+// U\+([0-9A-F]{4})', open(os.path.join(ROOT, 'src', 'CasioSymbols.h')).read())
check(len(lst) == len(mac) == 53, f'symbol count {len(lst)} / {len(mac)}')
for l, (name, esc, cp) in zip(lst, mac):
    n = int(cp, 16); b = [0xE0 | (n >> 12), 0x80 | ((n >> 6) & 0x3F), 0x80 | (n & 0x3F)]
    check(l == f'U+{cp} {name}' and esc == ''.join('\\x%02X' % x for x in b), f'symbol {name}')
r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'font_sizes.py')], capture_output=True, text=True)
check(r.returncode == 0 and 'CasioFont_VI_15.h' in r.stdout and 'all headers together' in r.stdout, 'font_sizes')
r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'gen_api.py')], capture_output=True, text=True)
check(r.returncode == 0, 'gen_api'); api = open(os.path.join(ROOT, 'docs', 'API.md'), encoding='utf-8').read()
for k in ('keyboard(', 'printWrapped', 'msgbox', 'encode(', 'setClipRect', 'animate('): check(k in api, 'API mentions ' + k)
print('ALL OK (4)' if not fails else f'{fails} FAILED'); sys.exit(1 if fails else 0)
