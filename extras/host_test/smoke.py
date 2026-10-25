#!/usr/bin/env python3
"""Compile every example sketch against host stubs (-std=gnu++11, ASan+UBSan), run setup() and a few loop()s, check it draws."""
import glob, os, shutil, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__)); os.chdir(HERE)
EX = os.path.join(HERE, '..', '..', 'examples'); SRC = os.path.join(HERE, '..', '..', 'src')
os.makedirs('smoke_data', exist_ok=True)
# media for the video examples
if shutil.which('ffmpeg'):
    subprocess.run(['ffmpeg', '-v', 'error', '-f', 'lavfi', '-i', 'testsrc2=size=384x126:rate=15:duration=2', '-c:v', 'mjpeg', '-q:v', '6', '-y', 'smoke_data/video.mjpeg'], check=True)
    subprocess.run([sys.executable, os.path.join(HERE, '..', '..', 'tools', 'c2vtool.py'), 'encode', 'smoke_data/video.mjpeg', '-o', 'smoke_data/clip.c2v', '--fps', '10'], check=True, capture_output=True)
# sketch -> (iterations, serial input, minimum inked pixels)
PLAN = {
  'Basic': (3, '', 30), 'Animation': (40, '', 5), 'IconSweep': (2, '', 0), 'Showcase': (4, '', 200), 'Dashboard': (60, '', 200),
  'Haunted': (1, '', 0), 'UnicodeDemo': (2, '', 100), 'UnicodeCompact': (1, '', 200), 'Fonts': (1, '', 0),
  'WraithDemo': (150, 'sddeewaq', 100), 'WraithShowcase': (250, 'esdesdsdaqes', 100), 'WraithKeyboard': (200, 'eeddesqq', 50), 'ConsoleDemo': (30, 'hello console\\nxin chao the gioi\\nu\\nd\\n', 100),
  'QrDemo': (2, '', 500), 'MjpegPlayer': (400, '', 300), 'C2vPlayer': (400, '', 300),
}
FLAGS = ['-std=gnu++11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=undefined', '-Wall', '-Wno-unused-variable', '-Wno-unused-parameter',
         '-I', 'examples_stub', '-I', '.', '-I', SRC]
LIB = [os.path.join(SRC, f) for f in ('CasioST75256.cpp', 'CasioJpeg.cpp', 'CasioMjpeg.cpp', 'CasioC2v.cpp', 'CasioConsole.cpp', 'CasioQR.cpp', 'WraithUI.cpp', 'WraithWidgets.cpp')]
fails = 0
for d in sorted(os.listdir(EX)):
    ino = os.path.join(EX, d, d + '.ino')
    if not os.path.exists(ino): continue
    iters, feed, need = PLAN.get(d, (3, '', 0))
    exe = f'/tmp/smoke_{d}'
    r = subprocess.run(['g++'] + FLAGS + [f'-DSKETCH="{ino}"', 'smoke.cpp'] + LIB + ['-o', exe], capture_output=True, text=True)
    if r.returncode:
        print(f'FAIL compile {d}:\n{r.stderr[:1500]}'); fails += 1; continue
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    r = subprocess.run([exe, str(iters), feed], capture_output=True, text=True, env=env, timeout=300)
    line = [l for l in r.stderr.split('\n') if l.startswith('SMOKE')]
    ink = int(line[0].split('ink=')[1].split()[0]) if line else -1
    status = 'ok' if r.returncode == 0 and line and ink >= need else 'FAIL'
    if status == 'FAIL': fails += 1
    print(f'{status:4s} {d:15s} {line[0] if line else r.stderr[-600:]}')
print('ALL EXAMPLES OK' if not fails else f'{fails} EXAMPLE(S) FAILED'); sys.exit(1 if fails else 0)
