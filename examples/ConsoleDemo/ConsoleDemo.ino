// CasioConsole demo (GrayKazu): a scrolling log window with word wrap, UTF-8 and scrollback.
// In the Serial monitor: type text + Enter to log it; send "u" / "d" (alone) to scroll back / forward.
#include <CasioST75256.h>
#include <CasioConsole.h>
#include <CasioFont_VI.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
CasioConsole con(lcd, 0, 0, 192, 63);                  // x, y, w, h of the text window

void setup() {
  Serial.begin(115200);
  lcd.begin(4000000);
  lcd.setBlockWrite(true);
  lcd.addUFont(&UFONT_VI);                             // remove this line for the small 5x7 font (8 lines of 31 chars)
  con.showCursor(true);
  con.println("Khởi động xong!");
  con.println("Gõ chữ vào Serial Monitor.");
}

void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n'); s.trim();
    if (s == "u") con.scrollBack(1);
    else if (s == "d") con.scrollBack(-1);
    else con.println(s);                               // new output jumps back to the bottom
  }
  con.draw();
  lcd.display();                                       // only the changed columns are sent
  delay(30);
}
