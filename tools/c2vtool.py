#!/usr/bin/env python3
"""C2V: pre-baked 192x63 4-gray video for CasioST75256 (plays with CasioC2vPlayer, no JPEG decoding on the ESP).

  python c2vtool.py encode input.mp4   -o clip.c2v --fps 15 --fit fit --dither bayer
  python c2vtool.py encode anim.gif    -o clip.c2v
  python c2vtool.py encode frames_dir/ -o clip.c2v --fps 12
  python c2vtool.py decode clip.c2v    -o preview.gif        (also checks the file)
  python c2vtool.py info   clip.c2v

Input: video (needs ffmpeg on PATH), animated GIF/APNG/WebP, or a folder of images. Needs Pillow + numpy.
Look options mirror CasioJpeg: --fit stretch|fit|fill, --dither none|bayer|floyd, --brightness, --contrast, --gamma,
--invert, --bg 0..3 (letterbox level, 0 white .. 3 black).

File format (little endian):
  header  : "C2V1", u16 width=192, u16 height=63, u16 fps*100, u32 frame count            (14 bytes)
  frame   : u16 payload length, u8 type, payload                                          (3 byte header)
    type 0: raw 3024 bytes (63 rows x 48 bytes, 4 px/byte MSB first, 0 white .. 3 black)
    type 1: RLE of the raw frame            type 2: RLE of (frame XOR previous frame)     type 3: same as previous (no payload)
  RLE tokens: 0x80|(n-1) = n zero bytes (1..128); 0x40|(n-1) = n bytes of 0xFF (1..64); (n-1) = n literal bytes follow (1..64)
"""
import argparse, os, struct, subprocess, sys
import numpy as np
from PIL import Image, ImageSequence

W, H, FRAME = 192, 63, 3024
BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]])

# ---------------- RLE ----------------
def rle_encode(b):
    out = bytearray(); i = 0; n = len(b); lit = bytearray()
    def flush():
        nonlocal lit
        for k in range(0, len(lit), 64):
            chunk = lit[k:k + 64]; out.append(len(chunk) - 1); out.extend(chunk)
        lit = bytearray()
    while i < n:
        v = b[i]; j = i
        while j < n and b[j] == v: j += 1
        run = j - i
        if v in (0x00, 0xFF) and run >= 2:
            flush()
            lim = 128 if v == 0 else 64
            while run > 0:
                m = min(run, lim); out.append((0x80 if v == 0 else 0x40) | (m - 1)); run -= m
        else:
            lit.extend(b[i:j])
        i = j
    flush()
    return bytes(out)

def rle_decode(p, size=FRAME):
    out = bytearray(); i = 0
    while i < len(p):
        t = p[i]; i += 1
        if t & 0x80: out.extend(b'\x00' * ((t & 0x7F) + 1))
        elif t & 0x40: out.extend(b'\xFF' * ((t & 0x3F) + 1))
        else:
            n = t + 1
            if i + n > len(p): raise ValueError('literal overruns payload')
            out.extend(p[i:i + n]); i += n
        if len(out) > size: raise ValueError('RLE overruns frame')
    if len(out) != size: raise ValueError('RLE size %d != %d' % (len(out), size))
    return bytes(out)

# ---------------- look ----------------
def make_lut(a):
    v = np.arange(256) / 255.0
    v = (v - 0.5) * a.contrast + 0.5 + a.brightness / 255.0
    v = np.clip(v, 0, 1)
    if a.gamma != 1.0: v = v ** (1.0 / a.gamma)
    return (v * 255 + 0.5).astype(np.uint8)

def quantise(gray, a, lut):
    g = lut[gray]
    d = (g if a.invert else 255 - g).astype(np.int32)             # darkness 0..255
    if a.dither == 'none': lv = (d * 3 + 127) // 255
    elif a.dither == 'bayer':
        b = np.tile(BAYER, (H // 4 + 1, W // 4 + 1))[:H, :W]
        lv = (d * 48 + (((b * 2 + 1) * 255) >> 1)) // (255 * 16)
    else:
        err = np.zeros((H + 1, W + 2), dtype=np.int32); lv = np.zeros((H, W), dtype=np.int32)
        for y in range(H):
            for x in range(W):
                v = min(255, max(0, int(d[y, x]) + int(err[y, x + 1]))); l = min(3, (v * 3 + 127) // 255); lv[y, x] = l
                e = v - l * 85
                err[y, x + 2] += (e * 7) >> 4; err[y + 1, x] += (e * 3) >> 4; err[y + 1, x + 1] += (e * 5) >> 4; err[y + 1, x + 2] += e >> 4
    return np.minimum(lv, 3).astype(np.uint8)

def pack(lv):
    out = bytearray()
    for y in range(H):
        for x in range(0, W, 4):
            out.append((int(lv[y, x]) << 6) | (int(lv[y, x + 1]) << 4) | (int(lv[y, x + 2]) << 2) | int(lv[y, x + 3]))
    return bytes(out)

def unpack(b):
    lv = np.zeros((H, W), dtype=np.uint8)
    for y in range(H):
        for x in range(W): lv[y, x] = (b[y * 48 + (x >> 2)] >> (6 - 2 * (x & 3))) & 3
    return lv

# ---------------- sources ----------------
def fit_image(im, a):
    im = im.convert('L')
    bg = 255 if a.bg == 0 else (170 if a.bg == 1 else (85 if a.bg == 2 else 0))
    if a.fit == 'stretch': return np.asarray(im.resize((W, H), Image.BOX))
    sw, sh = im.size
    s = min(W / sw, H / sh) if a.fit == 'fit' else max(W / sw, H / sh)
    nw, nh = max(1, round(sw * s)), max(1, round(sh * s))
    r = im.resize((nw, nh), Image.BOX if s < 1 else Image.BILINEAR)
    canvas = Image.new('L', (W, H), bg)
    canvas.paste(r, ((W - nw) // 2, (H - nh) // 2))
    return np.asarray(canvas)

def frames_from(path, a):
    if os.path.isdir(path):
        names = sorted(f for f in os.listdir(path) if f.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.gif', '.webp')))
        if not names: sys.exit('no images in ' + path)
        for n in names: yield fit_image(Image.open(os.path.join(path, n)), a), None
    elif path.lower().endswith(('.gif', '.png', '.apng', '.webp')) :
        im = Image.open(path)
        for fr in ImageSequence.Iterator(im): yield fit_image(fr.convert('RGBA').convert('RGB'), a), fr.info.get('duration')
    else:
        # ffmpeg does the scaling, padding and frame-rate conversion; we receive 192x63 gray frames
        vf = {'stretch': f'scale={W}:{H}',
              'fit': f'scale={W}:{H}:force_original_aspect_ratio=decrease,pad={W}:{H}:(ow-iw)/2:(oh-ih)/2:color={"white" if a.bg == 0 else "black" if a.bg == 3 else "gray"}',
              'fill': f'scale={W}:{H}:force_original_aspect_ratio=increase,crop={W}:{H}'}[a.fit]
        cmd = ['ffmpeg', '-v', 'error', '-i', path, '-an', '-vf', f'fps={a.fps},{vf},format=gray', '-f', 'rawvideo', '-pix_fmt', 'gray', '-']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        while True:
            buf = p.stdout.read(W * H)
            if len(buf) < W * H: break
            yield np.frombuffer(buf, dtype=np.uint8).reshape(H, W), None
        p.wait()

# ---------------- encode / decode ----------------
def encode(a):
    lut = make_lut(a); frames = []; durs = []
    for gray, dur in frames_from(a.input, a):
        frames.append(pack(quantise(gray, a, lut))); durs.append(dur)
        if a.max_frames and len(frames) >= a.max_frames: break
    if not frames: sys.exit('no frames decoded')
    fps = a.fps
    if a.fps_given is False and durs and all(d for d in durs): fps = round(1000.0 / (sum(durs) / len(durs)), 2)
    body = bytearray(); prev = bytes(FRAME); stats = [0, 0, 0, 0]
    for f in frames:
        if f == prev: typ, pay = 3, b''
        else:
            cands = [(0, f), (1, rle_encode(f)), (2, rle_encode(bytes(x ^ y for x, y in zip(f, prev))))]
            typ, pay = min(cands, key=lambda c: len(c[1]) if c[0] else FRAME)
        stats[typ] += 1
        body += struct.pack('<HB', len(pay), typ) + pay; prev = f
    data = b'C2V1' + struct.pack('<HHHI', W, H, int(round(fps * 100)), len(frames)) + body
    open(a.output, 'wb').write(data)
    print(f'{len(frames)} frames @ {fps} fps -> {len(data)} bytes ({len(data) / len(frames):.0f} B/frame, raw would be {FRAME}); '
          f'types raw/rle/delta/same = {stats}; {len(data) * 8 / (len(frames) / fps) / 1000:.1f} kbit/s')

def read_c2v(path):
    d = open(path, 'rb').read()
    if d[:4] != b'C2V1': raise ValueError('not a C2V file')
    w, h, fps100, n = struct.unpack('<HHHI', d[4:14]); pos = 14; prev = bytes(FRAME); out = []
    if (w, h) != (W, H): raise ValueError('unsupported size')
    for i in range(n):
        ln, typ = struct.unpack('<HB', d[pos:pos + 3]); pos += 3; pay = d[pos:pos + ln]; pos += ln
        if typ == 0:
            if ln != FRAME: raise ValueError('bad raw frame')
            cur = pay
        elif typ == 1: cur = rle_decode(pay)
        elif typ == 2: cur = bytes(x ^ y for x, y in zip(rle_decode(pay), prev))
        elif typ == 3: cur = prev
        else: raise ValueError('bad frame type')
        out.append(cur); prev = cur
    if pos != len(d): raise ValueError('trailing bytes')
    return fps100 / 100.0, out

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    e = sub.add_parser('encode'); e.add_argument('input'); e.add_argument('-o', '--output', required=True)
    e.add_argument('--fps', type=float, default=None); e.add_argument('--fit', choices=['stretch', 'fit', 'fill'], default='fit')
    e.add_argument('--dither', choices=['none', 'bayer', 'floyd'], default='bayer'); e.add_argument('--brightness', type=int, default=0)
    e.add_argument('--contrast', type=float, default=1.0); e.add_argument('--gamma', type=float, default=1.0)
    e.add_argument('--invert', action='store_true'); e.add_argument('--bg', type=int, default=0, choices=[0, 1, 2, 3]); e.add_argument('--max-frames', type=int, default=0)
    d = sub.add_parser('decode'); d.add_argument('input'); d.add_argument('-o', '--output', required=True)
    i = sub.add_parser('info'); i.add_argument('input')
    a = ap.parse_args()
    if a.cmd == 'encode':
        a.fps_given = a.fps is not None
        if a.fps is None: a.fps = 15.0
        encode(a)
    else:
        fps, frames = read_c2v(a.input)
        if a.cmd == 'info': print(f'{len(frames)} frames, {fps} fps, {len(frames) / fps:.1f} s, {os.path.getsize(a.input)} bytes')
        else:
            ims = [Image.fromarray((255 - unpack(f) * 85).astype(np.uint8)) for f in frames]
            ims[0].save(a.output, save_all=True, append_images=ims[1:], duration=int(1000 / fps), loop=0)
            print('wrote', a.output, f'({len(frames)} frames)')

if __name__ == '__main__': main()
