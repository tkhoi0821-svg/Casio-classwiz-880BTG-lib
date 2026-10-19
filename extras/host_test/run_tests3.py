#!/usr/bin/env python3
"""QR generator tests: every version x ECC level at full capacity, all three modes, UTF-8, Wi-Fi escaping - decoded with OpenCV."""
import os, random, subprocess, sys, tempfile
import numpy as np
try:
    import cv2
except ImportError:
    print('OpenCV not installed: QR decode tests skipped'); sys.exit(0)
HERE = os.path.dirname(os.path.abspath(__file__)); TMP = tempfile.mkdtemp(); fails = 0
def check(c, msg):
    global fails
    if not c: print('FAIL:', msg); fails += 1
def matrix(args):
    out = subprocess.run([os.path.join(HERE, args[0])] + [str(a) for a in args[1:]], capture_output=True, text=True).stdout.split('\n')
    if out[0] == 'FAIL': return None
    ver, size, mask, ecc = map(int, out[0].split())
    return ver, size, mask, ecc, np.array([[c == '1' for c in row] for row in out[1:1 + size]])
def decode(m, scale=8, quiet=4):
    n = m.shape[0]; img = np.full(((n + 2 * quiet) * scale,) * 2, 255, np.uint8)
    for y in range(n):
        for x in range(n):
            if m[y, x]: img[(y + quiet) * scale:(y + quiet + 1) * scale, (x + quiet) * scale:(x + quiet + 1) * scale] = 0
    text, pts, _ = cv2.QRCodeDetector().detectAndDecode(img)
    return text
def enc(text, ecc, boost=0, minver=1):
    p = os.path.join(TMP, 'in.txt'); open(p, 'wb').write(text.encode('utf-8'))
    return matrix(['qrdump', ecc, boost, minver, '@' + p])
# capacities (byte mode, data bits only) from the spec: version x ecc
CAP = {0: [17, 32, 53, 78, 106, 134, 154, 192, 230, 271], 1: [14, 26, 42, 62, 84, 106, 122, 152, 180, 213],
       2: [11, 20, 32, 46, 60, 74, 86, 108, 130, 151], 3: [7, 14, 24, 34, 44, 58, 64, 84, 98, 119]}
rng = random.Random(7); alphabet = 'abcdefghijklmnopqrstuvwxyz0123456789 .,:;/?=&-_!@#'
count = 0
for ecc in range(4):
    for v in range(1, 11):
        n = CAP[ecc][v - 1]
        text = ''.join(rng.choice(alphabet) for _ in range(n)); text = 'a' + text[1:]            # force byte mode
        r = enc(text, ecc); check(r is not None, f'v{v} ecc{ecc}: encode failed'); 
        if r is None: continue
        check(r[0] == v and r[3] == ecc, f'v{v} ecc{ecc}: chose version {r[0]} ecc {r[3]}')
        got = decode(r[4]); count += 1
        check(got == text, f'v{v} ecc{ecc} byte x{n}: decoded {got[:30]!r}')
        # one byte more must move to the next version (or be refused at v10)
        r2 = enc(text + 'z', ecc); check((r2 is None) if v == 10 else (r2 is not None and r2[0] == v + 1), f'v{v} ecc{ecc}: capacity+1')
print(f'{count} full-capacity codes (10 versions x 4 ecc levels) decoded')
# numeric / alphanumeric: denser modes pick smaller versions than byte mode would
for ecc in (0, 1, 2, 3):
    num = ''.join(rng.choice('0123456789') for _ in range(100)); r = enc(num, ecc); check(r and decode(r[4]) == num, f'numeric ecc{ecc}')
    al = ''.join(rng.choice('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 $%*+-./:') for _ in range(80)); r = enc(al, ecc); check(r and decode(r[4]) == al, f'alnum ecc{ecc}')
r_num = enc('1' * 100, 0, 0); r_byte = enc('a' * 100, 0, 0); check(r_num[0] < r_byte[0], f'numeric mode is denser ({r_num[0]} vs {r_byte[0]})')
for odd in ('1', '12', '123', '1234', '12345', 'A', 'AB', 'ABC', 'A1B2C'):
    r = enc(odd, 1); check(r and decode(r[4]) == odd, f'tail handling {odd}')
# UTF-8
for t in ('Xin chào thế giới! Tiếng Việt ấ ệ ử', '你好，世界', 'สวัสดีครับ', 'Mixed: abc 你好 xin chào €'):
    r = enc(t, 1); check(r and decode(r[4]) == t, f'utf8 {t}')
# URLs
for t in ('https://example.com', 'https://github.com/user/repo?tab=readme#top', 'mailto:a@b.co', 'tel:+84901234567'):
    r = enc(t, 0); check(r and decode(r[4]) == t, f'url {t}')
# ECC boost keeps the version but raises the level; no boost keeps the requested level
r = enc('HELLO', 0, 1); r0 = enc('HELLO', 0, 0); check(r[0] == r0[0] == 1 and r[3] > 0 and r0[3] == 0, 'boost raises ecc inside the same version')
# minimum version
r = enc('hi', 0, 0, 5); check(r[0] == 5 and decode(r[4]) == 'hi', 'minVersion')
# robustness: error correction really works - damage some modules and still decode (ECC_H, v3)
r = enc('damage test ' * 3, 3, 0); m = r[4].copy(); n = m.shape[0]
for _ in range(10): x, y = rng.randrange(9, n - 9), rng.randrange(9, n - 9); m[y, x] ^= True
check(decode(m) == 'damage test ' * 3, 'ecc_h repairs 10 flipped modules')
# Wi-Fi
def wifi(ssid, pw, typ):
    return matrix(['qrwifi', ssid, pw, typ])
for ssid, pw, typ in (('Home', 'secret123', 'WPA'), ('My;Net', 'p:a,s\\s"x', 'WPA'), ('Cafe Wi-Fi', '', 'nopass'), ('Tiếng Việt', 'mật-khẩu', 'WPA')):
    r = wifi(ssid, pw, typ); check(r is not None, f'wifi {ssid} encode')
    esc = lambda s: ''.join('\\' + c if c in '\\;,:"' else c for c in s)
    want = f'WIFI:T:{typ};S:{esc(ssid)};' + (f'P:{esc(pw)};' if typ != 'nopass' else '') + ';'
    ref = enc(want, 1, 1)
    check(r and ref and (r[4] == ref[4]).all(), f'wifi {ssid}: matrix equals encode(expected string)')
    check(r and decode(r[4]) == want, f'wifi {ssid}: decoded {decode(r[4])!r}')
# the smallest practical sizes still scan at the LCD's real scale (2 px modules, 2-module quiet zone)
r = enc('https://t.co/x', 1); check(decode(r[4], scale=4, quiet=2) == 'https://t.co/x', 'scans with only a 2-module quiet zone')
print('ALL OK (3)' if not fails else f'{fails} FAILED'); sys.exit(1 if fails else 0)
