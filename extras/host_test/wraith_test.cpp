#include <Arduino.h>
#include <SPI.h>
#include "WraithUI.h"
#include "CasioFont_VI.h"
#include "CasioFont_ZH_CN.h"
int g_a0=0, PIN_A0=10, PIN_CS=14; SPIClass SPI; void csEdge(int){}
uint8_t SPIClass::transfer(uint8_t){return 0;}
int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static uint8_t lastId=255; static int16_t lastVal=-1; static int events=0;
static void h(uint8_t id,int16_t v){ lastId=id; lastVal=v; events++; }
static int count(CasioST75256& l){ int n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(l.getPixel(x,y)) n++; return n; }
static void art(CasioST75256& l){ for(int y=0;y<63;y++){ for(int x=0;x<192;x++) putchar(l.getPixel(x,y)==3?'#':l.getPixel(x,y)?'+':'.'); putchar('\n'); } }
int main(){
  SPIClass sp; CasioST75256 lcd(14,10,11,12,13,&sp); lcd.begin();
  WraithUI ui(lcd); ui.onEvent(h);
  static const char* items[]={"Tieng Viet","Thai","Chinese","English","Four","Five"};
  uint8_t title=ui.label(0,0,0,192,"MENU",WraithUI::CENTER);
  uint8_t lst=ui.list(0,2,10,90,30,items,6);
  uint8_t btn=ui.button(0,100,10,60,16,"Go");
  uint8_t sl=ui.slider(1,5,5,100,0,100,50,10);
  uint8_t tg=ui.toggle(1,5,25,false);
  uint8_t ck=ui.checkbox(1,40,25,80,"Haunted",true);
  uint8_t br=ui.bar(1,5,45,100,8,0);
  uint8_t ga=ui.gauge(2,40,50,25,0,100,0);
  static int16_t buf[20]={0}; uint8_t ch=ui.chart(2,80,5,100,40,buf,20,0,100);
  CHECK(title==0&&lst==1&&btn==2); CHECK(ui.focused()==lst);
  CHECK(ui.update(1000)); CHECK(count(lcd)>200);
  CHECK(!ui.update(1010));                    // nothing changed -> no redraw
  // list navigation
  ui.key(WraithUI::KEY_DOWN); ui.key(WraithUI::KEY_DOWN); ui.key(WraithUI::KEY_OK); CHECK(lastId==lst && lastVal==2);
  for(int i=0;i<10;i++) ui.key(WraithUI::KEY_DOWN);     // list end (5), then focus moves to button
  CHECK(ui.focused()==btn);
  ui.key(WraithUI::KEY_OK); CHECK(lastId==btn && lastVal==1);
  CHECK(ui.update(1020));                      // flash redraw
  ui.key(WraithUI::KEY_UP); CHECK(ui.focused()==lst);
  ui.key(WraithUI::KEY_RIGHT); CHECK(ui.focused()==btn);   // list ignores left/right -> focus moves
  // screen 1: slider/toggle/check/bar
  ui.show(1); CHECK(ui.screen()==1 && ui.focused()==sl);
  ui.key(WraithUI::KEY_RIGHT); CHECK(lastId==sl && lastVal==60); ui.key(WraithUI::KEY_LEFT); ui.key(WraithUI::KEY_LEFT); CHECK(lastVal==40);
  for(int i=0;i<20;i++) ui.key(WraithUI::KEY_LEFT); CHECK(ui.value(sl)==0);
  ui.key(WraithUI::KEY_DOWN); CHECK(ui.focused()==tg); ui.key(WraithUI::KEY_OK); CHECK(lastVal==1 && ui.value(tg)==1);
  ui.key(WraithUI::KEY_DOWN); CHECK(ui.focused()==ck); ui.key(WraithUI::KEY_LEFT); CHECK(lastVal==0);
  ui.key(WraithUI::KEY_DOWN); CHECK(ui.focused()==sl);       // wraps around
  // tween: bar value animates and converges
  ui.setValue(br,80); uint32_t t=2000; int frames=0; while(ui.update(t)&&frames<200){ t+=16; frames++; }
  CHECK(frames>3 && frames<80); CHECK(ui.value(br)==80);
  { int fill=0; for(int x=7;x<105;x++) if(lcd.getPixel(x,49)==3) fill++; CHECK(fill>=76 && fill<=80); }
  // back goes to previous screen
  ui.key(WraithUI::KEY_BACK); CHECK(ui.screen()==0);
  // gauge + chart
  ui.show(2); for(int i=0;i<30;i++) ui.chartPush(ch,(i*7)%100); ui.setValue(ga,70); t=5000; frames=0; while(ui.update(t)&&frames<200){t+=16;frames++;} CHECK(frames<80);
  CHECK(count(lcd)>150);
  // transitions end on a clean frame identical to a normal draw
  ui.show(0, WraithUI::TR_GLITCH, 300); for(t=6000; t<6600; t+=20) ui.update(t); ui.update(6700);
  { static uint8_t snap[16*192]; memcpy(snap,lcd.frameBuffer(),sizeof(snap)); ui.draw(); CHECK(memcmp(snap,lcd.frameBuffer(),sizeof(snap))==0); }
  ui.show(1, WraithUI::TR_MELT, 400); for(t=7000; t<7800; t+=20) ui.update(t); ui.update(8000);
  CHECK(ui.screen()==1);
  { static uint8_t snap[16*192]; memcpy(snap,lcd.frameBuffer(),sizeof(snap)); ui.draw(); CHECK(memcmp(snap,lcd.frameBuffer(),sizeof(snap))==0); }
  ui.show(2, WraithUI::TR_STATIC, 200); for(t=9000; t<9400; t+=20) ui.update(t); CHECK(ui.screen()==2);
  ui.show(0, WraithUI::TR_CRT, 50); CHECK(ui.screen()==0);
  // fonts per widget: VI default + ZH fallback; widget with Chinese text
  ui.setDefaultFont(&UFONT_VI); ui.addFallbackFont(&UFONT_ZH_CN); ui.setText(title,"Xin chào 你好"); ui.update(10000);

  CHECK(lcd.fontHeight()==8);                  // UI restores the built-in font after drawing
  ui.setVisible(btn,false); CHECK(ui.focused()!=btn);
  printf(fails?"%d FAILED\n":"WRAITH OK\n",fails); return fails;
}
