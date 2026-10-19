#!/usr/bin/env python3
"""Host tests for the C2V format/player, and for the MJPEG pipeline fed with real ffmpeg output."""
import os, random, shutil, subprocess, sys, tempfile
import numpy as np
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, os.path.join(HERE, '..', '..', 'tools'))
import c2vtool as C
TMP = tempfile.mkdtemp(); fails = 0
def check(c, msg):
    global fails
    if not c: print('FAIL:', msg); fails += 1
def run(*a): return subprocess.run([str(x) for x in a], capture_output=True, text=True)
class A:                                   # encoder options
    def __init__(s, **k):
        s.fit = 'fit'; s.dither = 'bayer'; s.brightness = 0; s.contrast = 1.0; s.gamma = 1.0; s.invert = False; s.bg = 0; s.fps = 15.0
        s.__dict__.update(k)

# ---- RLE codec ----
rng = random.Random(1)
cases = [bytes(3024), b'\xff' * 3024, bytes([0, 255]) * 1512, bytes(rng.randrange(256) for _ in range(3024)),
         bytes(200) + b'\xff' * 300 + bytes(rng.randrange(256) for _ in range(100)) + bytes(2424)]
for _ in range(50):
    b = bytearray()
    while len(b) < 3024:
        b += bytes([rng.choice([0, 255, rng.randrange(256)])]) * rng.choice([1, 1, 2, 5, 70, 130, 300])
    cases.append(bytes(b[:3024]))
for c in cases: check(C.rle_decode(C.rle_encode(c)) == c, 'rle roundtrip')
check(len(C.rle_encode(bytes(3024))) <= 48, 'zero frame compresses')

# ---- synthetic GIF -> C2V ----
frames = []
for i in range(30):
    im = Image.new('RGB', (128, 64)); d = ImageDraw.Draw(im)
    for x in range(128): d.line([x, 0, x, 63], fill=(x * 2, x * 2, x * 2))
    d.ellipse([5 + i * 3, 15, 35 + i * 3, 45], fill=(255, 255, 255), outline=(0, 0, 0))
    if i == 10: im = Image.new('RGB', (128, 64), (255, 255, 255))       # flash
    frames.append(im)
frames[20] = frames[19]                                                  # exact repeat
gif = os.path.join(TMP, 'a.gif'); frames[0].save(gif, save_all=True, append_images=frames[1:], duration=100, loop=0)
c2v = os.path.join(TMP, 'a.c2v')
r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'encode', gif, '-o', c2v); print(r.stdout.strip()); check(r.returncode == 0, 'encode gif ' + r.stderr)
fps, ref = C.read_c2v(c2v); N = len(ref); check(N == 29 and abs(fps - 9.67) < 0.01, f'gif fps/frames {fps} {N}')       # Pillow merges the duplicate GIF frame
a = A(fps=fps); lut = C.make_lut(a)
src = [C.pack(C.quantise(g, a, lut)) for g, _ in C.frames_from(gif, a)]
check(ref == src, 'c2v is a lossless container for the quantised frames')
check(os.path.getsize(c2v) < N * 3024 * 0.5, f'compression {os.path.getsize(c2v)}')
raw = b''.join(ref)
for trickle in (1 << 30, 97, 1):
    out = os.path.join(TMP, 'o.bin'); r = run(os.path.join(HERE, 'c2vplay'), c2v, trickle, 0, out)
    check(f'shown={N}' in r.stdout and 'finished=1' in r.stdout and 'failed=0' in r.stdout, f'play trickle {trickle}: {r.stdout}')
    check(open(out, 'rb').read() == raw, f'frames identical, trickle {trickle}')
out = os.path.join(TMP, 'o.bin'); r = run(os.path.join(HERE, 'c2vplay'), c2v, 61, 1, out); check(open(out, 'rb').read() == raw * 3 and f'shown={3 * N}' in r.stdout, 'looping 3 passes: ' + r.stdout)
r = run(os.path.join(HERE, 'c2vplay'), c2v, 1 << 30, 0, out, 0); check('fps=9.7' in r.stdout and f'shown={N}' in r.stdout and 'finished=1' in r.stdout, 'paced at file rate: ' + r.stdout)
now = int(r.stdout.split('now=')[1]); check((N - 1) * 100 <= now <= (N + 1) * 110, f'paced playback follows the clock: simulated {now} ms for {N} frames at 9.67 fps')
r = run(os.path.join(HERE, 'c2vplay'), c2v, 1 << 30, 0, out, 5); check('fps=5.00' in r.stdout, 'fps override')
# paced playback must not run faster than the clock: 30 frames at 10 fps need ~2.9 s of simulated time (7 ms steps -> many idle calls)
data = open(c2v, 'rb').read()
open(os.path.join(TMP, 'trunc.c2v'), 'wb').write(data[:len(data) // 2]); r = run(os.path.join(HERE, 'c2vplay'), os.path.join(TMP, 'trunc.c2v'), 1 << 30, 0, out)
check('failed=0' in r.stdout and 'finished=0' in r.stdout, 'truncated stream just stalls: ' + r.stdout)
bad = bytearray(data); bad[0] = ord('X'); open(os.path.join(TMP, 'bad.c2v'), 'wb').write(bytes(bad)); r = run(os.path.join(HERE, 'c2vplay'), os.path.join(TMP, 'bad.c2v'), 1 << 30, 0, out); check('failed=1' in r.stdout, 'bad magic: ' + r.stdout)
bad = bytearray(data); bad[14] = 0xFF; bad[15] = 0xFF; open(os.path.join(TMP, 'bad2.c2v'), 'wb').write(bytes(bad)); r = run(os.path.join(HERE, 'c2vplay'), os.path.join(TMP, 'bad2.c2v'), 1 << 30, 0, out); check('failed=1' in r.stdout, 'absurd frame length: ' + r.stdout)
for i in range(40):                                                       # random corruption: never crash, never hang
    bad = bytearray(data)
    for _ in range(8): bad[random.randrange(14, len(bad))] = random.randrange(256)
    open(os.path.join(TMP, 'fz.c2v'), 'wb').write(bytes(bad)); r = subprocess.run([os.path.join(HERE, 'c2vplay'), os.path.join(TMP, 'fz.c2v'), '1073741824', '0', out], capture_output=True, timeout=60)
    check(r.returncode == 0, f'fuzz {i} crashed')
r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'decode', c2v, '-o', os.path.join(TMP, 'p.gif')); check(r.returncode == 0 and os.path.getsize(os.path.join(TMP, 'p.gif')) > 1000, 'decode to gif')
r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'info', c2v); check(f'{N} frames' in r.stdout, 'info ' + r.stdout)
# a folder of images
os.makedirs(os.path.join(TMP, 'seq')); 
for i, f in enumerate(frames[:5]): f.save(os.path.join(TMP, 'seq', f'{i:03d}.png'))
r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'encode', os.path.join(TMP, 'seq'), '-o', os.path.join(TMP, 's.c2v'), '--fps', '8', '--dither', 'floyd'); check(r.returncode == 0 and C.read_c2v(os.path.join(TMP, 's.c2v'))[0] == 8.0, 'image folder + floyd')
# repeated frames use the 'same as previous' type (no payload); players must handle them
for f in os.listdir(os.path.join(TMP, 'seq')): os.remove(os.path.join(TMP, 'seq', f))
for i, f in enumerate([frames[0], frames[0], frames[1], frames[1], frames[1], frames[2]]): f.save(os.path.join(TMP, 'seq', f'{i:03d}.png'))
r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'encode', os.path.join(TMP, 'seq'), '-o', os.path.join(TMP, 'r.c2v'), '--fps', '8'); print(r.stdout.strip())
check('[0, 1, 2, 3]' in r.stdout, 'type stats ' + r.stdout)
fps_r, ref_r = C.read_c2v(os.path.join(TMP, 'r.c2v')); check(len(ref_r) == 6 and ref_r[0] == ref_r[1] and ref_r[2] == ref_r[3] == ref_r[4], 'repeats decode')
r = run(os.path.join(HERE, 'c2vplay'), os.path.join(TMP, 'r.c2v'), 1, 0, out); check(open(out, 'rb').read() == b''.join(ref_r) and 'finished=1' in r.stdout, 'player handles repeated frames: ' + r.stdout)

# ---- ffmpeg-made video ----
if shutil.which('ffmpeg'):
    mp4 = os.path.join(TMP, 't.avi')
    r = run('ffmpeg', '-v', 'error', '-f', 'lavfi', '-i', 'testsrc2=size=320x180:rate=15:duration=2', '-c:v', 'mpeg4', '-y', mp4); check(r.returncode == 0, 'ffmpeg make video ' + r.stderr)
    r = run(sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'encode', mp4, '-o', os.path.join(TMP, 'v.c2v'), '--fps', '10'); print(r.stdout.strip(), r.stderr.strip()[:200])
    check(r.returncode == 0, 'encode video via ffmpeg')
    if r.returncode == 0:
        fps, fr = C.read_c2v(os.path.join(TMP, 'v.c2v')); check(18 <= len(fr) <= 21 and fps == 10.0, f'video frames {len(fr)}')
        check(len(set(fr)) > 10 and C.unpack(fr[5]).mean() > 0.2, 'video frames have content')
    # real MJPEG files straight from ffmpeg: several sizes / subsamplings / quality
    for name, size, pix, q in (('420', '384x126', 'yuvj420p', 5), ('444', '640x360', 'yuvj444p', 3), ('422', '1280x720', 'yuvj422p', 8), ('odd', '333x211', 'yuvj420p', 4)):
        mj = os.path.join(TMP, f'{name}.mjpeg')
        r = run('ffmpeg', '-v', 'error', '-f', 'lavfi', '-i', f'testsrc2=size={size}:rate=15:duration=2', '-c:v', 'mjpeg', '-pix_fmt', pix, '-q:v', str(q), '-y', mj); check(r.returncode == 0, 'ffmpeg mjpeg ' + r.stderr)
        out = os.path.join(TMP, f'{name}.bin'); r = run(os.path.join(HERE, 'mjpegplay'), mj, out, 1, 1)
        check('shown=30 errors=0' in r.stdout, f'mjpeg {name}: {r.stdout.strip()}')
        got = open(out, 'rb').read(); n = len(got) // 3024
        blob = open(mj, 'rb').read(); jp = [b'\xff\xd8\xff' + p for p in blob.split(b'\xff\xd8\xff')[1:]]
        check(len(jp) == 30, f'{name}: {len(jp)} jpegs')
        a = A(fit='fit', dither='bayer'); lut = C.make_lut(a); diffs = []
        for i in (0, 7, 14, 29):
            im = Image.open(__import__('io').BytesIO(jp[i])); lv = C.quantise(C.fit_image(im, a), a, lut)
            dev = C.unpack(got[i * 3024:(i + 1) * 3024]); diffs.append((lv != dev).mean())
            check(abs(float(lv.mean()) - float(dev.mean())) < 0.08, f'{name} frame {i} tone {lv.mean():.3f} vs {dev.mean():.3f}')
        print(f'mjpeg {name:4s} {size:9s} {pix}: differing pixels vs Pillow pipeline: ' + ' '.join(f'{d * 100:.1f}%' for d in diffs))
        check(max(diffs) < 0.16, f'{name}: level mismatch {max(diffs):.3f}')
else:
    print('ffmpeg not found: video tests skipped')
print('ALL OK (2)' if not fails else f'{fails} FAILED'); sys.exit(1 if fails else 0)
