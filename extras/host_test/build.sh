#!/bin/sh
# Build the host test binaries (needs g++) and run every test (python3 + Pillow + numpy; ffmpeg enables the video tests, OpenCV the QR decode tests).
cd "$(dirname "$0")" || exit 1
S=../../src
# SAN=1 ./build.sh  builds everything with AddressSanitizer + UndefinedBehaviorSanitizer
if [ -n "$SAN" ]; then OPT="-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined"; else OPT="-O2"; fi
CXX="g++ -std=c++17 $OPT -I. -I$S"
ALL="$S/CasioST75256.cpp"
$CXX -o jpgdec jpgdec.cpp $S/CasioJpeg.cpp || exit 1
$CXX -o split split.cpp $S/CasioMjpeg.cpp $S/CasioJpeg.cpp $ALL || exit 1
$CXX -o player player.cpp $S/CasioMjpeg.cpp $S/CasioJpeg.cpp $ALL || exit 1
$CXX -o mjpegplay mjpegplay.cpp $S/CasioMjpeg.cpp $S/CasioJpeg.cpp $ALL || exit 1
$CXX -o c2vplay c2vplay.cpp $S/CasioC2v.cpp $ALL || exit 1
$CXX -o core_test core_test.cpp $ALL || exit 1
$CXX -o unicode_test unicode_test.cpp $ALL || exit 1
$CXX -o wraith_test wraith_test.cpp $S/WraithUI.cpp $S/WraithWidgets.cpp $ALL || exit 1
$CXX -o extra_test extra_test.cpp $S/WraithUI.cpp $S/WraithWidgets.cpp $S/CasioConsole.cpp $ALL || exit 1
$CXX -o wraith2_test wraith2_test.cpp $S/WraithUI.cpp $S/WraithWidgets.cpp $ALL || exit 1
$CXX -o font15_test font15_test.cpp $ALL || exit 1
$CXX -o qr_test qr_test.cpp $S/CasioQR.cpp $ALL || exit 1
$CXX -o qrdump qrdump.cpp $S/CasioQR.cpp $ALL || exit 1
$CXX -o qrwifi qrwifi.cpp $S/CasioQR.cpp $ALL || exit 1
./core_test && ./unicode_test | tail -1 && ./wraith_test && ./wraith2_test && ./extra_test && ./font15_test && ./qr_test || exit 1
python3 run_tests.py && python3 run_tests2.py && python3 run_tests3.py && python3 smoke.py
