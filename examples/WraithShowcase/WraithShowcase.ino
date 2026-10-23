// WraithUI 2 showcase (GrayKazu): home screen with a menu + live meter, a settings screen with tabs, a text-input screen,
// popups (dropdown, msgbox, toast), themes, slide transitions, timers.
// Keys from the Serial monitor: w/s = up/down, a/d = left/right, e = OK, q = back.
// On the input screen the keys type text instead: Tab = down, Esc = back, Enter = OK, Backspace = delete.
#include <CasioST75256.h>
#include <WraithUI.h>
#include <CasioFont_VI_15.h>

SPIClass lcdSpi(FSPI);
CasioST75256 lcd(14, 10, 11, 12, 13, &lcdSpi);
WraithUI ui(lcd);

enum { SCR_HOME, SCR_SETTINGS, SCR_INPUT };
const char* MENU[]   = {"Settings", "Type text", "Haunt me", "Exit"};
const char* TABS[]   = {"Display", "Audio", "Data"};
const char* THEMES[] = {"Light", "Dark", "Gray", "Haunted"};
const char* CELLS[]  = {"Name", "Value", "Vop", "123", "Rate", "4 MHz", "Fonts", "VI TH ZH", "Tween", "on"};
uint8_t idMenu, idMeter, idContrast, idTheme, idSave, idLog, idInput, idEcho, idSubmit;

void onExit(int8_t button, void*) {
  if (button == 0) { ui.toast("Bye!"); ui.show(SCR_HOME, WraithUI::TR_CRT, 400); }
}
void onMenu(uint8_t id, uint8_t ev, int16_t v, void*) {
  if (ev != WraithUI::EV_CLICK) return;                       // v = the selected row
  if (v == 0) ui.push(SCR_SETTINGS, WraithUI::TR_SLIDE_LEFT, 300);
  else if (v == 1) ui.push(SCR_INPUT, WraithUI::TR_SLIDE_LEFT, 300);
  else if (v == 2) { ui.setTheme(WraithUI::THEME_HAUNTED); ui.toast("boo!"); }
  else ui.msgbox("Exit", "Really leave?", "Yes", "No", nullptr, onExit);
}
void onSettings(uint8_t id, uint8_t ev, int16_t v, void*) {
  if (id == idContrast && ev == WraithUI::EV_VALUE) lcd.setVop(100 + v);
  else if (id == idTheme && ev == WraithUI::EV_VALUE) ui.setTheme((WraithUI::Theme)v);
  else if (id == idSave && ev == WraithUI::EV_CLICK) { ui.appendText(idLog, "saved\n"); ui.toast("Saved!"); }
  else if (id == idSave && ev == WraithUI::EV_LONG) ui.toast("long press");
}
void onInput(uint8_t id, uint8_t ev, int16_t v, void*) {
  if (id == idSubmit && ev == WraithUI::EV_CLICK) {           // Enter on the button: move the text into the log
    ui.appendText(idEcho, ui.text(idInput)); ui.appendText(idEcho, "\n"); ui.setText(idInput, "");
    ui.focus(idInput);
  }
}
void onTick(void*) {                                          // 5 times a second
  ui.setValue(idMeter, 50 + (int)(45 * sinf(millis() / 800.0f)));
}

void setup() {
  Serial.begin(115200);
  lcd.begin(4000000); lcd.setBlockWrite(true);
  ui.setDefaultFont(nullptr);                                 // built-in 5x7 font for widgets...

  // ---- home ----
  uint8_t title = ui.label(SCR_HOME, 2, 0, 0, "Xin chào, Wraith!");
  ui.setFont(title, &UFONT_VI_15);                            // ...and the 15 px Vietnamese font for this label only
  idMenu = ui.list(SCR_HOME, 2, 17, 78, 44, MENU, 4); ui.on(idMenu, onMenu);
  idMeter = ui.meter(SCR_HOME, 106, 17, 70, 0, 100, 50, 5);
  ui.spinner(SCR_HOME, 170, 1, 14);

  // ---- settings: tabs ----
  uint8_t tv = ui.tabview(SCR_SETTINGS, 0, 0, 192, 63, TABS, 3);
  idContrast = ui.slider(SCR_SETTINGS, 4, 6, 120, 0, 40, 23);       ui.setParent(idContrast, tv, 0);
  uint8_t tg = ui.toggle(SCR_SETTINGS, 4, 24);                       ui.setParent(tg, tv, 0);
  idTheme = ui.dropdown(SCR_SETTINGS, 40, 22, 90, THEMES, 4);       ui.setParent(idTheme, tv, 0);
  uint8_t arc = ui.arc(SCR_SETTINGS, 6, 2, 44, 0, 100, 40);         ui.setParent(arc, tv, 1);
  uint8_t sb = ui.spinbox(SCR_SETTINGS, 70, 8, 70, 0, 99, 7, 1);    ui.setParent(sb, tv, 1);
  uint8_t led = ui.led(SCR_SETTINGS, 150, 10, 10, 3);                ui.setParent(led, tv, 1);
  uint8_t tbl = ui.table(SCR_SETTINGS, 2, 2, 110, 44, CELLS, 2, 5); ui.setParent(tbl, tv, 2);
  idLog = ui.textarea(SCR_SETTINGS, 116, 2, 72, 30, 120);            ui.setParent(idLog, tv, 2);
  idSave = ui.button(SCR_SETTINGS, 116, 34, 72, 14, "Save");         ui.setParent(idSave, tv, 2);
  ui.on(idContrast, onSettings); ui.on(idTheme, onSettings); ui.on(idSave, onSettings);

  // ---- text input ----
  uint8_t lab = ui.label(SCR_INPUT, 4, 2, 0, "Type something:");
  idInput = ui.input(SCR_INPUT, 4, 12, 150, 40);
  idSubmit = ui.button(SCR_INPUT, 158, 12, 30, 0, "OK");
  idEcho = ui.textarea(SCR_INPUT, 4, 28, 184, 34, 160);
  ui.on(idSubmit, onInput); (void)lab;

  ui.timer(200, onTick);
}

void loop() {
  while (Serial.available()) {
    int c = Serial.read();
    bool typing = ui.screen() == SCR_INPUT && ui.focused() == idInput;
    if (typing) {
      if (c == '\t') ui.key(WraithUI::KEY_DOWN);
      else if (c == 27) ui.key(WraithUI::KEY_BACK);
      else if (c == '\r' || c == '\n') ui.key(WraithUI::KEY_OK);
      else if (c == 8 || c == 127) ui.backspace();
      else { char s[2] = {(char)c, 0}; ui.typeChar(s); }
    } else switch (c) {
      case 'w': ui.key(WraithUI::KEY_UP); break;    case 's': ui.key(WraithUI::KEY_DOWN); break;
      case 'a': ui.key(WraithUI::KEY_LEFT); break;  case 'd': ui.key(WraithUI::KEY_RIGHT); break;
      case 'e': ui.key(WraithUI::KEY_OK); break;    case 'q': ui.key(WraithUI::KEY_BACK); break;
    }
  }
  ui.update(millis());                              // redraws only when something changed
}
