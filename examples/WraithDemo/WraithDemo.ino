// WraithUI demo (GrayKazu): menu list, settings with slider/toggle/checkbox/bar, live monitor with gauge + chart.
// Keys come from the Serial monitor: w/s = up/down, a/d = left/right, e = OK, q = back.
// Replace readKey() with your GPIO buttons or the calculator keypad.
#include <CasioST75256.h>
#include <WraithUI.h>
#include <CasioFont_VI.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
WraithUI ui(lcd);

enum { SCR_MENU, SCR_SETTINGS, SCR_MONITOR };
const char* MENU[] = {"Cai dat", "Giam sat", "Ma am"};
int16_t samples[40];
uint8_t idMenu, idContrast, idHaunt, idBar, idGauge, idChart;

void onEvent(uint8_t id, int16_t v) {
  if (id == idMenu) {
    if (v == 0) ui.show(SCR_SETTINGS, WraithUI::TR_GLITCH);
    else if (v == 1) ui.show(SCR_MONITOR, WraithUI::TR_STATIC);
    else ui.show(SCR_MONITOR, WraithUI::TR_MELT, 700);
  } else if (id == idContrast) lcd.setVop(100 + v);          // needs a working setVop()
}

int readKey() {
  if (!Serial.available()) return -1;
  switch (Serial.read()) {
    case 'w': return WraithUI::KEY_UP;    case 's': return WraithUI::KEY_DOWN;
    case 'a': return WraithUI::KEY_LEFT;  case 'd': return WraithUI::KEY_RIGHT;
    case 'e': return WraithUI::KEY_OK;    case 'q': return WraithUI::KEY_BACK;
  }
  return -1;
}

void setup() {
  Serial.begin(115200);
  lcd.begin(4000000); lcd.setBlockWrite(true);

  ui.label(SCR_MENU, 0, 0, 192, "WRAITH UI", WraithUI::CENTER);
  idMenu = ui.list(SCR_MENU, 40, 12, 112, 36, MENU, 3);

  uint8_t lbl = ui.label(SCR_SETTINGS, 2, 0, 120, "Tương phản");
  ui.setFont(lbl, &UFONT_VI);                                 // this label uses the Vietnamese 16 px font
  idContrast = ui.slider(SCR_SETTINGS, 4, 20, 120, 0, 40, 23, 1);
  idHaunt = ui.checkbox(SCR_SETTINGS, 4, 36, 100, "Haunted FX", true);
  ui.toggle(SCR_SETTINGS, 150, 36, true);
  idBar = ui.bar(SCR_SETTINGS, 4, 52, 180, 8, 70);

  idGauge = ui.gauge(SCR_MONITOR, 34, 55, 30, 0, 100);
  idChart = ui.chart(SCR_MONITOR, 80, 4, 108, 54, samples, 40, 0, 100);
  for (auto& s : samples) s = 50;
}

void loop() {
  static uint32_t lastSample = 0;
  int k = readKey();
  if (k >= 0) ui.key((WraithUI::Key)k);
  if (millis() - lastSample > 120) {
    lastSample = millis();
    int v = 50 + (int)(40 * sinf(millis() / 700.0f));
    ui.setValue(idGauge, v);
    ui.chartPush(idChart, v);
  }
  ui.update(millis());                                        // redraws only when something changed
}
