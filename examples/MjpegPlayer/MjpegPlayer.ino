// MJPEG video on the 192x63 LCD (GrayKazu): plays /video.mjpeg from LittleFS in 4 gray levels.
//
// Make the video on your PC (any resolution >= 192 px wide works, ~2x the LCD, e.g. 384 px, is a good size):
//   ffmpeg -i input.mp4 -an -vf "scale=384:-2,fps=15" -c:v mjpeg -q:v 6 video.mjpeg
// For a fixed look, add "eq=contrast=1.2" to the -vf chain. Upload video.mjpeg to LittleFS (data folder / upload tool).
// The same player works with any Stream: a WiFiClient reading an ESP32-CAM /stream, Serial, an SD File...
#include <LittleFS.h>
#include <CasioST75256.h>
#include <CasioMjpeg.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);

static uint8_t frameBuf[24 * 1024];            // must hold the biggest JPEG frame of your video
File video;
CasioMjpegPlayer* player;

void openVideo() {
  if (video) video.close();
  video = LittleFS.open("/video.mjpeg", "r");
  if (player) player->reset();
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  openVideo();
  player = new CasioMjpegPlayer(lcd, video, frameBuf, sizeof(frameBuf));
  player->setFps(15);
  player->decoder().setFit(CasioJpeg::FIT);              // STRETCH / FIT (letterbox) / FILL (crop)
  player->decoder().setDither(CasioJpeg::DITHER_BAYER);  // BAYER = no flicker; FLOYD = smoother stills; NONE = fastest
  player->decoder().setContrast(1.15f);
  // player->decoder().setGamma(1.3f); player->decoder().setInvert(true); player->decoder().setBackground(3);
}

void loop() {
  player->update(millis());
  if (!video.available()) openVideo();                   // loop forever
  static uint32_t last = 0;
  if (millis() - last > 2000) {                          // stats
    last = millis();
    Serial.printf("shown %u  skipped %u  errors %u  decode %u ms\n",
                  player->shown(), player->skipped(), player->errors(), player->lastDecodeMs());
  }
}
