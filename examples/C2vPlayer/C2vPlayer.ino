// Pre-baked video (.c2v) on the 192x63 LCD (GrayKazu). No JPEG decoding: a frame costs microseconds to unpack.
//
// On your PC (needs Python + Pillow + numpy, and ffmpeg for video files):
//   python tools/c2vtool.py encode input.mp4 -o clip.c2v --fps 15 --fit fit --dither bayer
//   python tools/c2vtool.py encode anim.gif  -o clip.c2v          (frame rate is taken from the GIF)
//   python tools/c2vtool.py decode clip.c2v  -o preview.gif       (see what the LCD will show)
// Upload clip.c2v to LittleFS, then:
#include <LittleFS.h>
#include <CasioST75256.h>
#include <CasioC2v.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
File clip;
CasioC2vPlayer* player;

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  clip = LittleFS.open("/clip.c2v", "r");
  player = new CasioC2vPlayer(lcd, clip);
  player->setLoop(true, [](Stream& s) { return ((File&)s).seek(0); });   // rewind function = loop forever
  // player->setFps(30);   // override the file's rate;  setFps(-1) = as fast as possible
}

void loop() {
  player->update(millis());
  if (player->failed()) { Serial.println("bad .c2v file"); delay(1000); }
}
