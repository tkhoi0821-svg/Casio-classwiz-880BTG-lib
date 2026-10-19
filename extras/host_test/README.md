# Host tests

Run everything on a PC, no hardware needed:

    ./build.sh            # normal build + all tests
    SAN=1 ./build.sh      # same with AddressSanitizer + UndefinedBehaviorSanitizer
    python3 smoke.py      # compile (-std=gnu++11) and run every example sketch against host stubs

Needs g++, Python 3, Pillow and numpy. ffmpeg enables the video tests, OpenCV (`cv2`) the QR decode tests; tests that need
a missing tool are skipped with a message. `examples_stub/` and the `Arduino.h`/`Print.h`/`SPI.h` stubs here are just
enough of the Arduino/ESP32 API to build the library and the examples on a PC. Generated binaries and `smoke_data/` are
not part of the library; delete them freely.
