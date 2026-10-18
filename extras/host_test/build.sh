#!/bin/sh
# Build the host test binaries (needs g++) and run the tests (needs python3 + Pillow + numpy).
cd "$(dirname "$0")"
S=../../src
g++ -std=c++17 -O2 -I. -I$S -o jpgdec jpgdec.cpp $S/CasioJpeg.cpp || exit 1
g++ -std=c++17 -O2 -I. -I$S -o split split.cpp $S/CasioMjpeg.cpp $S/CasioJpeg.cpp $S/CasioST75256.cpp || exit 1
g++ -std=c++17 -O2 -I. -I$S -o player player.cpp $S/CasioMjpeg.cpp $S/CasioJpeg.cpp $S/CasioST75256.cpp || exit 1
python3 run_tests.py
