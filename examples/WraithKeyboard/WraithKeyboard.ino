// WraithUI keyboard + symbol icons (GrayKazu): pick a Wi-Fi network, type its password on the on-screen keyboard.
// Keys from the Serial monitor: w/s = up/down, a/d = left/right, e = OK, q = back.
// Icons come from the symbol font: SYM_WIFI, SYM_BATTERY_FULL ... (docs/SYMBOLS.md lists all 53), added AFTER the text font.
#include <CasioST75256.h>
#include <WraithUI.h>
#include <CasioFont_VI_15.h>
#include <CasioFont_SYM_15.h>
#include <CasioSymbols.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
WraithUI ui(lcd);

enum { SCR_HOME, SCR_PASS };
const char* NETS[] = {"MyHomeWiFi", "Cafe Guest", "Office-5G", "Hidden network"};
uint8_t idStatus, idList, idInput, idKeys;
int chosen = 0;

void onNet(uint8_t id, uint8_t ev, int16_t row, void*) {
  if (ev != WraithUI::EV_CLICK) return;
  chosen = row;
  ui.setText(idInput, "");
  ui.push(SCR_PASS, WraithUI::TR_SLIDE_LEFT, 300);
  ui.focus(idKeys);
}
void onKeys(uint8_t id, uint8_t ev, int16_t, void*) {
  if (ev != WraithUI::EV_CLICK) return;                 // the OK key of the keyboard
  char msg[40]; snprintf(msg, sizeof(msg), "%s: %u chars", NETS[chosen], (unsigned)strlen(ui.text(idInput)));
  ui.toast(msg, 1800);
  ui.pop(WraithUI::TR_SLIDE_RIGHT, 300);
}

void setup() {
  Serial.begin(115200);
  lcd.begin(4000000); lcd.setBlockWrite(true);
  ui.addFallbackFont(&UFONT_SYM_15);                    // icons are found after the text font's own glyphs

  idStatus = ui.label(SCR_HOME, 2, 0, 188, SYM_WIFI " Home  " SYM_BATTERY_FULL " 100%  " SYM_CHARGE, WraithUI::LEFT, 64);
  ui.setFont(idStatus, &UFONT_VI_15);                   // this label uses the 15 px font; the list below the built-in 5x7
  idList = ui.list(SCR_HOME, 2, 17, 188, 44, NETS, 4);  ui.on(idList, onNet);

  idInput = ui.input(SCR_PASS, 0, 0, 192, 40);
  idKeys = ui.keyboard(SCR_PASS, 0, 16, 192, 45, idInput);  ui.on(idKeys, onKeys);
}

void loop() {
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'w': ui.key(WraithUI::KEY_UP); break;    case 's': ui.key(WraithUI::KEY_DOWN); break;
      case 'a': ui.key(WraithUI::KEY_LEFT); break;  case 'd': ui.key(WraithUI::KEY_RIGHT); break;
      case 'e': ui.key(WraithUI::KEY_OK); break;    case 'q': ui.key(WraithUI::KEY_BACK); break;
    }
  }
  ui.update(millis());
}
