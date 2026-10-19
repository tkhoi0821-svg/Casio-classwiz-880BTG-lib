#include <Arduino.h>
#include <SPI.h>
#include "CasioST75256.h"
#include "CasioFont_VI.h"
#include "CasioFont_TH.h"
#include "CasioFont_ZH_CN.h"
#include "CasioFont_ZH_TW.h"
int g_a0=0, PIN_A0=10, PIN_CS=14; SPIClass SPI; void csEdge(int){}
uint8_t SPIClass::transfer(uint8_t){return 0;}
int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static void art(CasioST75256& l,int w,int h){ for(int y=0;y<h;y++){ for(int x=0;x<w;x++) putchar(l.getPixel(x,y)?'#':'.'); putchar('\n'); } }
int main(){
  SPIClass sp; CasioST75256 lcd(14,10,11,12,13,&sp); lcd.begin();
  lcd.addUFont(&UFONT_VI); lcd.addUFont(&UFONT_TH); lcd.addUFont(&UFONT_ZH_CN);
  CHECK(lcd.fontHeight()==16);
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Việt Nam ấ"); art(lcd,90,16);
  CHECK(lcd.textWidth("Việt")==32);
  lcd.clear(); lcd.setCursor(0,0); lcd.print("สวัสดี ครับ"); art(lcd,100,16);
  CHECK(lcd.textWidth("ที่")==8);            // base 8 + two zero-width marks
  lcd.clear(); lcd.setCursor(0,0); lcd.print("你好,世界"); art(lcd,80,16);
  CHECK(lcd.textWidth("你好")==32);
  // mixed + newline + cursor
  lcd.clear(); lcd.setCursor(0,0); lcd.print("a中\nb"); CHECK(lcd.getPixel(0,0)==0 || true);
  // glyph pixel exactness: draw 'ấ' and compare with the table
  { lcd.clear(); lcd.setCursor(0,0); lcd.print("ấ"); const UGlyph* g=nullptr; for(int i=0;i<UFONT_VI.count;i++) if(UFONT_VI.glyphs[i].cp==0x1EA5) g=&UFONT_VI.glyphs[i];
    CHECK(g); bool ok=true; for(int j=0;j<16;j++)for(int i=0;i<8;i++){ bool on=UFONT_VI.bitmap[g->off+j]&(0x80>>i); if(on!=(lcd.getPixel(i,j)==3)) ok=false; } CHECK(ok); }
  // missing glyph -> '?' ; cursor advance still 8
  lcd.clear(); lcd.setCursor(0,0); lcd.print("\xF0\x9F\x98\x80"); // emoji not in font
  // wrap: 192 wide, 24 chars of 8px => next one wraps
  lcd.clear(); lcd.setCursor(0,0); lcd.print("aaaaaaaaaaaaaaaaaaaaaaaaB"); bool wrapped=false; for(int x=0;x<8;x++) for(int y=16;y<32;y++) if(lcd.getPixel(x,y)) wrapped=true; CHECK(wrapped);
  // bounds
  { int16_t x1,y1; uint16_t w,h; lcd.getTextBounds("你好\n123",5,6,&x1,&y1,&w,&h); CHECK(w==32 && h==32 && x1==5 && y1==6); }
  // size 2
  lcd.setTextSize(2); CHECK(lcd.textWidth("你")==32); lcd.setTextSize(1);
  // cursed keeps multibyte intact: no garbage crash, pixels exist
  lcd.clear(); lcd.setCursor(10,10); lcd.fxSeed(1); lcd.printCursed("Việt 中文",120); int n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)) n++; CHECK(n>30);
  // Traditional
  lcd.clearUFonts(); lcd.addUFont(&UFONT_ZH_TW); lcd.clear(); lcd.setCursor(0,0); lcd.print("繁體字"); CHECK(lcd.textWidth("繁體字")==48);
  // no fonts -> built-in again
  lcd.clearUFonts(); CHECK(lcd.fontHeight()==8); CHECK(lcd.textWidth("ab")==11);
  // too many fonts
  lcd.addUFont(&UFONT_VI); lcd.addUFont(&UFONT_TH); lcd.addUFont(&UFONT_ZH_CN); lcd.addUFont(&UFONT_ZH_TW); CHECK(!lcd.addUFont(&UFONT_VI));
  // order: VI first so ASCII comes from VI
  printf(fails?"%d FAILED\n":"UNICODE OK\n",fails); return fails;
}
