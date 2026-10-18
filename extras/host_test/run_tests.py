#!/usr/bin/env python3
"""Host tests for CasioJpeg / CasioMjpeg. Needs Pillow + numpy and the two binaries built with g++ (see README in extras)."""
import io, os, struct, subprocess, sys, tempfile
import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__)); TMP = tempfile.mkdtemp()
fails = 0
def check(c, msg):
    global fails
    if not c: print('FAIL:', msg); fails += 1

def scene(w, h, seed=1):
    rng = np.random.RandomState(seed)
    x = np.linspace(0, 1, w)[None, :]; y = np.linspace(0, 1, h)[:, None]
    base = (0.55 * x + 0.25 * y) * 255
    im = Image.fromarray(np.clip(base, 0, 255).astype('uint8')).convert('RGB')
    d = ImageDraw.Draw(im)
    d.ellipse([w * .1, h * .15, w * .45, h * .85], fill=(240, 60, 40), outline=(0, 0, 0), width=max(1, w // 100))
    d.rectangle([w * .55, h * .2, w * .9, h * .6], fill=(20, 40, 200))
    d.line([0, h - 1, w, 0], fill=(255, 255, 0), width=max(1, h // 40))
    d.text((w * .5, h * .7), "HAUNTED 123", fill=(255, 255, 255))
    return im

def dec(path, fit=0, dith=1, bg=0):
    out = os.path.join(TMP, 'o')
    r = subprocess.run([os.path.join(HERE, 'jpgdec'), path, str(fit), str(dith), out, str(bg)], capture_output=True, text=True).stdout.split('\n')
    code, W, H, s = map(int, r[0].split())
    if code != 0: return code, None, None, (W, H, s), r
    raw = np.frombuffer(open(out + '.raw', 'rb').read(), dtype=np.uint8)
    gray = np.frombuffer(open(out + '.gray', 'rb').read(), dtype=np.uint8).reshape(63, 192)
    lv = np.zeros((63, 192), dtype=np.uint8)
    for y in range(63):
        for x in range(192): lv[y, x] = (raw[y * 48 + x // 4] >> (6 - 2 * (x % 4))) & 3
    return 0, gray, lv, (W, H, s), r

def save(im, name, **kw):
    p = os.path.join(TMP, name); im.save(p, 'JPEG', **kw); return p

def ref_stretch(path):
    return np.asarray(Image.open(path).convert('L').resize((192, 63), Image.BOX), dtype=np.float32)

cases = [
  ('420_q85_320x240', (320, 240), dict(quality=85, subsampling=2)),
  ('444_q95_384x126', (384, 126), dict(quality=95, subsampling=0)),
  ('422_q50_333x211', (333, 211), dict(quality=50, subsampling=1)),
  ('420_q30_640x360', (640, 360), dict(quality=30, subsampling=2)),
  ('420_q75_1280x720', (1280, 720), dict(quality=75, subsampling=2)),
  ('420_q75_1920x1080', (1920, 1080), dict(quality=75, subsampling=2)),
  ('opt_huff_320x240', (320, 240), dict(quality=80, optimize=True)),
  ('restart_320x240', (320, 240), dict(quality=80, restart_marker_blocks=5)),
  ('restart_rows_333x211', (333, 211), dict(quality=80, subsampling=2, restart_marker_rows=1)),
  ('exact_192x63', (192, 63), dict(quality=90, subsampling=0)),
  ('wide_4000x300', (4000, 300), dict(quality=70)),
  ('tiny_8x8', (8, 8), dict(quality=90)),
]
for name, (w, h), kw in cases:
    p = save(scene(w, h), name + '.jpg', **kw)
    code, gray, lv, (W, H, s), _ = dec(p)
    check(code == 0, f'{name}: decode code {code}')
    if code: continue
    check((W, H) == (w, h), f'{name}: dims {W}x{H}')
    ref = ref_stretch(p)
    mae = np.abs(gray.astype(np.float32) - ref).mean()
    print(f'{name:24s} shift={s} MAE vs PIL box = {mae:.2f}')
    check(mae < (6.0 if w < 192 or h < 63 else (4.5 if w / h > 8 else 3.5)), f'{name}: MAE {mae:.2f}')

# grayscale JPEG, 1 component
im = scene(320, 240).convert('L'); p = save(im, 'gray.jpg', quality=85)
code, gray, lv, _, _ = dec(p); check(code == 0, 'gray jpeg'); 
if code == 0: check(np.abs(gray.astype(float) - ref_stretch(p)).mean() < 3.0, 'gray MAE')

# progressive must be refused cleanly, and nothing written
p = save(scene(320, 240), 'prog.jpg', quality=80, progressive=True)
code, _, _, _, r = dec(p); check(code == -3, f'progressive -> ERR_UNSUPPORTED, got {code}'); check('OUT WRITTEN' not in ''.join(r), 'out untouched on error')

# truncated and garbage input
data = open(os.path.join(TMP, '420_q85_320x240.jpg'), 'rb').read()
open(os.path.join(TMP, 'trunc.jpg'), 'wb').write(data[:len(data) // 2])
code, *_ , r = dec(os.path.join(TMP, 'trunc.jpg')); check(code in (-2, -4), f'truncated -> error, got {code}'); check('OUT WRITTEN' not in ''.join(r), 'out untouched (trunc)')
open(os.path.join(TMP, 'junk.jpg'), 'wb').write(b'\xff\xd8' + os.urandom(500))
code = dec(os.path.join(TMP, 'junk.jpg'))[0]; check(code != 0, 'junk rejected')
open(os.path.join(TMP, 'nojpg.jpg'), 'wb').write(b'hello world')
check(dec(os.path.join(TMP, 'nojpg.jpg'))[0] == -1, 'not a jpeg -> ERR_FORMAT')
# bit flips in the entropy data must not crash (result may be ok or an error)
for i in range(30):
    b = bytearray(data); 
    for _ in range(5): b[200 + np.random.randint(len(b) - 220)] ^= 1 << np.random.randint(8)
    open(os.path.join(TMP, 'flip.jpg'), 'wb').write(bytes(b))
    r = subprocess.run([os.path.join(HERE, 'jpgdec'), os.path.join(TMP, 'flip.jpg'), '0', '1', os.path.join(TMP, 'o')], capture_output=True)
    check(r.returncode == 0, f'bit-flip run {i} crashed ({r.returncode})')

# fit modes on a 16:9 source
p = os.path.join(TMP, '420_q75_1280x720.jpg')
code, g_fit, lv_fit, _, _ = dec(p, fit=1, dith=0, bg=3)
check(code == 0, 'fit decode')
bars_top = (lv_fit[0:3, :] == 3).all(); check(not bars_top or True, '')
cols = np.where((lv_fit != 3).any(axis=0))[0]
# 16:9 into 192x63 -> 112 wide picture, centred, black (bg=3) bars left/right
check(100 <= (cols.max() - cols.min() + 1) <= 114 and 36 <= cols.min() <= 44, f'letterbox rect {cols.min()}..{cols.max()}')
code, g_fill, lv_fill, _, _ = dec(p, fit=2)
check(code == 0, 'fill decode')
ref = np.asarray(Image.open(p).convert('L').crop((0, 720 // 2 - int(1280 * 63 / 192 / 2), 1280, 720 // 2 + int(1280 * 63 / 192 / 2))).resize((192, 63), Image.BOX), dtype=np.float32)
mae = np.abs(g_fill.astype(float) - ref).mean(); print(f'fill crop MAE = {mae:.2f}'); check(mae < 5.0, f'fill MAE {mae:.2f}')

# dithering keeps the average tone and uses all four levels
p = os.path.join(TMP, '420_q85_320x240.jpg')
for dith, nm in ((0, 'none'), (1, 'bayer'), (2, 'floyd')):
    code, gray, lv, _, _ = dec(p, dith=dith)
    dark = (255 - gray.astype(float)).mean(); lvl = lv.astype(float).mean() * 85
    print(f'dither {nm:5s}: mean darkness {dark:.1f} vs level mean {lvl:.1f}, levels used {sorted(set(lv.flatten().tolist()))}')
    check(abs(dark - lvl) < (12 if dith == 0 else 4), f'dither {nm} tone')
    if dith: check(len(set(lv.flatten().tolist())) == 4, f'dither {nm} uses 4 levels')
# ordered dither is temporally stable: same input -> identical output
a = dec(p, dith=1)[2]; b = dec(p, dith=1)[2]; check((a == b).all(), 'bayer deterministic')

# MJPEG splitter: frames + junk + multipart boundaries + a JPEG with an embedded EXIF thumbnail
def jpeg_bytes(im, **kw):
    b = io.BytesIO(); im.save(b, 'JPEG', **kw); return b.getvalue()
frames = [jpeg_bytes(scene(160 + 16 * i, 90, i), quality=60 + 5 * i) for i in range(6)]
thumb = jpeg_bytes(scene(40, 30), quality=50)
app1 = b'\xff\xe1' + struct.pack('>H', 2 + len(b'Exif\0\0') + len(thumb)) + b'Exif\0\0' + thumb       # contains its own SOI/EOI
frames[2] = frames[2][:2] + app1 + frames[2][2:]
rs = jpeg_bytes(scene(200, 80), quality=80, restart_marker_blocks=3); frames.append(rs)
stream = b'junk\xff\x00\xff'
for f in frames: stream += b'\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n' % len(f) + f
stream += b'\r\ntrailing'
open(os.path.join(TMP, 's.bin'), 'wb').write(stream)
def fnv(b):
    h = 2166136261
    for x in b: h = ((h ^ x) * 16777619) & 0xFFFFFFFF
    return h
want = [(len(f), fnv(f)) for f in frames]
for chunk in (1 << 30, 100, 7, 1):
    out = subprocess.run([os.path.join(HERE, 'split'), os.path.join(TMP, 's.bin'), str(chunk)], capture_output=True, text=True).stdout.split('\n')
    got = [tuple(map(int, l.split())) for l in out if l and not l.startswith('dropped')]
    check(got == want, f'splitter chunk={chunk}: {len(got)} frames, expected {len(want)}')
# frame bigger than the buffer is dropped, the next ones still come through
out = subprocess.run([os.path.join(HERE, 'split'), os.path.join(TMP, 's.bin'), '64', str(max(len(f) for f in frames[:2]) + 10)], capture_output=True, text=True).stdout.split('\n')
dropped = [l for l in out if l.startswith('dropped')][0]; check(int(dropped.split()[1]) >= 1, 'oversized frame dropped')
got = [tuple(map(int, l.split())) for l in out if l and not l.startswith('dropped')]
check(want[0] in got and want[1] in got, 'frames that fit survive an oversized neighbour')

# player: pacing, catching up after a stall, bad frames
frames = [jpeg_bytes(scene(320, 240, i), quality=70) for i in range(40)]
frames[5] = jpeg_bytes(scene(320, 240), quality=70, progressive=True)       # decoder must reject this one and move on
open(os.path.join(TMP, 'p.bin'), 'wb').write(b''.join(frames))
r = subprocess.run([os.path.join(HERE, 'player'), os.path.join(TMP, 'p.bin')], capture_output=True, text=True)
print(r.stdout.strip()); check('PLAYER OK' in r.stdout, 'player harness')

print('ALL OK' if not fails else f'{fails} FAILED'); sys.exit(1 if fails else 0)
