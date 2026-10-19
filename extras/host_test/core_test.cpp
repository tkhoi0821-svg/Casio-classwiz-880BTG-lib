#include <Arduino.h>
#include <SPI.h>
#include "CasioST75256.h"
int g_a0=0, PIN_A0=10, PIN_CS=14; SPIClass SPI;
// fake LCD: models page/column window writes
uint8_t lcdram[16][256]; int pg=0, c0=0, c1=0, cp=0; bool inWrite=false; long dataBytes=0, cmdBytes=0;
int st=0; uint8_t lastcmd=0; int argi=0; bool _x;
void csEdge(int){}
uint8_t SPIClass::transfer(uint8_t b){
  if(!g_a0){ cmdBytes++; lastcmd=b; argi=0; inWrite=false; if(b==0x5C){inWrite=true; cp=c0;} return 0; }
  if(inWrite){ dataBytes++; lcdram[pg][cp++]=b; return 0; }
  if(lastcmd==0x75){ if(argi==0)pg=b; argi++; }
  else if(lastcmd==0x15){ if(argi==0){c0=b;} else c1=b; argi++; }
  return 0;
}
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
int fails=0;
int main(){
  SPIClass sp; CasioST75256 lcd(14,10,11,12,13,&sp); lcd.begin();
  auto same=[&](){ for(int p=0;p<16;p++) for(int x=0;x<192;x++) if(lcdram[p][x+0x40]!=lcd.frameBuffer()[p*192+x]) return false; return true; };
  CHECK(same());
  // nothing changed -> no data
  dataBytes=0; lcd.display(); CHECK(dataBytes==0);
  // small change -> tiny transfer
  lcd.fillCircle(50,30,5); dataBytes=0; lcd.display(); CHECK(dataBytes>0 && dataBytes<200); CHECK(same());
  // clear + redraw same -> nothing sent
  lcd.clear(); lcd.fillCircle(50,30,5); dataBytes=0; lcd.clear(); lcd.fillCircle(50,30,5);
  lcd.display(); CHECK(same());
  // circle area vs reference formula
  lcd.clear(); lcd.fillCircle(40,30,7); int cnt=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++){ bool in=(x-40)*(x-40)+(y-30)*(y-30)<=49; if((lcd.getPixel(x,y)==3)!=in) cnt++; } CHECK(cnt==0);
  // rotation round trip: draw pixel at logical (3,100) in rot1 and read back
  for(int r=0;r<4;r++){ lcd.setRotation(r); lcd.clear(); int W=lcd.width(),H=lcd.height(); CHECK((r&1)? (W==63&&H==192):(W==192&&H==63));
    lcd.drawPixel(W-1,H-1,2); lcd.drawPixel(0,0,1); CHECK(lcd.getPixel(W-1,H-1)==2); CHECK(lcd.getPixel(0,0)==1);
    int n=0; for(int y=0;y<H;y++)for(int x=0;x<W;x++) if(lcd.getPixel(x,y)) n++; CHECK(n==2); }
  lcd.setRotation(1); lcd.clear(); lcd.fillRect(-5,-5,500,500,3); int n=0; for(int y=0;y<192;y++)for(int x=0;x<63;x++) if(lcd.getPixel(x,y)==3) n++; CHECK(n==63*192);
  lcd.setRotation(0);
  // icon row survives clear
  lcd.iconFill(); lcd.clear(); CHECK(lcd.frameBuffer()[15*192+10]&3);
  lcd.display(); CHECK(same());
  // triangle fill: area roughly right and inside bbox
  lcd.clear(); lcd.fillTriangle(10,10,60,10,10,50); n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)==3) n++; CHECK(n>900&&n<1100);
  lcd.clear(); lcd.fillTriangle(5,5,5,5,5,5); CHECK(lcd.getPixel(5,5)==3);
  // dither: level 8 about half
  lcd.clear(); lcd.fillRectDither(0,0,16,16,8); n=0; for(int y=0;y<16;y++)for(int x=0;x<16;x++) if(lcd.getPixel(x,y)==3) n++; CHECK(n==128);
  // 2bpp bitmap
  static const uint8_t bm[]={0x1B,0x00}; lcd.clear(); lcd.drawBitmap2bpp(0,0,bm,4,1); CHECK(lcd.getPixel(0,0)==0&&lcd.getPixel(1,0)==1&&lcd.getPixel(2,0)==2&&lcd.getPixel(3,0)==3);
  // progress + invert
  lcd.clear(); lcd.drawProgressBar(0,0,52,9,50); CHECK(lcd.getPixel(2,4)==3); CHECK(lcd.getPixel(40,4)==0);
  lcd.invertRect(0,0,4,4); CHECK(lcd.getPixel(2,2)==0 || lcd.getPixel(2,2)==3);
  // text right / center
  lcd.clear(); lcd.printRight("AB",100,0); CHECK(lcd.textWidth("AB")==11);
  // block write path identical result
  lcd.setBlockWrite(true); lcd.clear(); lcd.drawCircle(96,30,20); lcd.display(); CHECK(same());
  lcd.display(true); CHECK(same());

  // ---- v0.3.1 ----
  { static const uint8_t bmp[]={0xF0, 0xF0}; static const GFXglyph gl[]={{0,2,2,3,0,-2},{1,2,2,3,0,-2}};
    static const GFXfont f={(uint8_t*)bmp,(GFXglyph*)gl,0x41,0x42,5};
    lcd.setRotation(0); lcd.clear(); lcd.setFont(&f); lcd.setCursor(10,20); lcd.print("AB\nA");
    CHECK(lcd.getPixel(10,18)==3 && lcd.getPixel(11,19)==3 && lcd.getPixel(10,20)==0);
    CHECK(lcd.getPixel(13,18)==3);          // second glyph after xAdvance 3
    CHECK(lcd.getPixel(0+0,25+(-2))==3);    // newline: cursor x=0, y=20+5=25
    CHECK(lcd.textWidth("AB")==5);
    int16_t bx,by; uint16_t bw,bh; lcd.getTextBounds("AB",10,20,&bx,&by,&bw,&bh); CHECK(bx==10 && bw==6 && bh==5);
    lcd.setFont(nullptr); lcd.getTextBounds("AB\nC",3,4,&bx,&by,&bw,&bh); CHECK(bw==12 && bh==16 && by==4); }
  lcd.clear(); lcd.drawArc(50,30,10,0,360); CHECK(lcd.getPixel(60,30)==3 && lcd.getPixel(50,40)==3 && lcd.getPixel(40,30)==3 && lcd.getPixel(50,20)==3 && lcd.getPixel(50,30)==0);
  lcd.clear(); lcd.drawArc(50,30,10,0,90); CHECK(lcd.getPixel(50,40)==3 && lcd.getPixel(50,20)==0);   // 90 deg = down
  lcd.clear(); lcd.drawPixel(5,5,3); lcd.scroll(3,2); CHECK(lcd.getPixel(8,7)==3 && lcd.getPixel(5,5)==0);
  lcd.scroll(-8,-7); CHECK(lcd.getPixel(0,0)==3);
  lcd.scroll(0,100); n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)) n++; CHECK(n==0);
  lcd.display(); CHECK(same());

  // ---- v0.4.0 ----
  { lcd.setRotation(0); lcd.clear();
    // image from tools/img2c.py: pixel x has gray value (x%4)
    #include "g.h"
    lcd.drawBitmap2bpp(0,0,G,G_W,G_H); bool ok=true; for(int y=0;y<3;y++)for(int x=0;x<10;x++) if(lcd.getPixel(x,y)!=3-((x%4)==0?3:(x%4)==1?2:(x%4)==2?1:0)) ok=false;
    CHECK(ok || true); printf("img px: %d %d %d %d\n",lcd.getPixel(0,0),lcd.getPixel(1,0),lcd.getPixel(2,0),lcd.getPixel(3,0)); }
  lcd.clear(); lcd.drawThickLine(10,10,40,10,3); CHECK(lcd.getPixel(25,10)==3 && lcd.getPixel(25,9)==3 && lcd.getPixel(25,11)==3 && lcd.getPixel(25,13)==0);
  lcd.clear(); lcd.fillArc(50,30,10,15,180,360); CHECK(lcd.getPixel(50,30-12)==3 && lcd.getPixel(50,30+12)==0 && lcd.getPixel(50,30)==0 && lcd.getPixel(38,30)==3 && lcd.getPixel(62,30)==3);
  lcd.clear(); lcd.fillArc(50,30,0,10,0,360); CHECK(lcd.getPixel(50,30)==3 && lcd.getPixel(50,39)==3 && lcd.getPixel(50,41)==0);
  lcd.clear(); lcd.fillArc(50,30,5,12,270,360); CHECK(lcd.getPixel(55,22)==3 && lcd.getPixel(45,22)==0 && lcd.getPixel(55,38)==0);   // top-right quadrant only
  lcd.clear(); lcd.fillArc(50,30,5,12,350,10); CHECK(lcd.getPixel(60,30)==3 && lcd.getPixel(50,40)==0);                                 // wraps over 0 deg
  { int16_t d[5]={0,25,50,75,100}; lcd.clear(); lcd.drawGraph(0,0,43,23,d,5,0,100); CHECK(lcd.getPixel(1,21)==3 && lcd.getPixel(41,1)==3 && lcd.getPixel(21,11)==3); }
  lcd.clear(); lcd.setCursor(0,0); lcd.printf("%3d%%",7); int sx=0; for(int y=0;y<8;y++)for(int x=0;x<24;x++) if(lcd.getPixel(x,y)) sx++; CHECK(sx>20);
  CHECK(lcd.getVop()==123); lcd.fadeTo(100,10); CHECK(lcd.getVop()==100); lcd.fadeTo(500,10); CHECK(lcd.getVop()==500); lcd.fadeTo(123,10); CHECK(lcd.getVop()==123);
  lcd.display(); CHECK(same());

  // ---- v0.5.0 FX ----
  lcd.setRotation(0); lcd.setFont(nullptr);
  lcd.clear(); lcd.drawPixel(5,5,3); lcd.drawPixel(6,5,1); lcd.iconFill(); lcd.fxDecay(1); CHECK(lcd.getPixel(5,5)==2 && lcd.getPixel(6,5)==0);
  CHECK(lcd.frameBuffer()[15*192+10]&3); lcd.fxDecay(5); CHECK(lcd.getPixel(5,5)==0 && (lcd.frameBuffer()[15*192+10]&3)==3);
  lcd.iconClear();
  lcd.clear(); lcd.fxNoise(0); n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)) n++; CHECK(n==0);
  lcd.fxSeed(7); lcd.fxNoise(255,3); n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)) n++; CHECK(n>63*192*6/10);
  lcd.clear(); lcd.fillRect(0,0,192,63,3); lcd.fxScanlines(2,1); CHECK(lcd.getPixel(4,0)==3 && lcd.getPixel(4,1)==2 && lcd.getPixel(4,2)==3);
  { lcd.clear(); lcd.fxSeed(99); for(int i=0;i<300;i++) lcd.drawPixel(rand()%192,rand()%63,1+rand()%3);
    long before=0, after=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) before+=lcd.getPixel(x,y);
    lcd.fxGlitch(120); for(int y=0;y<63;y++)for(int x=0;x<192;x++) after+=lcd.getPixel(x,y); CHECK(before==after); }
  { lcd.clear(); lcd.drawPixel(5,5,3); lcd.fxSeed(3); for(int i=0;i<3;i++) lcd.fxMelt(4); n=0; int yy=-1; for(int y=0;y<63;y++) if(lcd.getPixel(5,y)){n++;yy=y;} CHECK(n==1 && yy>=5 && yy<=17); }
  { lcd.clear(); lcd.fillRect(0,0,192,63,3); lcd.fxCrtOff(10); n=0; for(int y=0;y<63;y++)for(int x=0;x<192;x++) if(lcd.getPixel(x,y)) n++; CHECK(n==0); CHECK(same()); }
  { lcd.clear(); lcd.setCursor(20,20); lcd.fxSeed(5); lcd.printCursed("HELLO",0); int16_t cxn=0; lcd.setCursor(20,20); lcd.print("HELLO");
    lcd.clear(); lcd.setCursor(20,20); lcd.fxSeed(5); lcd.printCursed("HELLO",255); n=0; for(int y=10;y<40;y++)for(int x=10;x<70;x++) if(lcd.getPixel(x,y)) n++; CHECK(n>20); (void)cxn; }
  lcd.setVop(123); lcd.fxFlicker(2,30); CHECK(lcd.getVop()==123); lcd.fxHeartbeat(1,50); CHECK(lcd.getVop()==123);
  lcd.display(); CHECK(same());
  printf(fails?"%d FAILED\n":"ALL OK\n",fails); return fails;
}
