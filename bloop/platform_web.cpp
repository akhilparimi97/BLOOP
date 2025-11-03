#ifndef ARDUINO

#include "platform.h"
#include "font5x7.h"
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>
#include <emscripten.h>

extern "C" {
  void   js_draw_pixel(int x, int y, int color);
  void   js_display();
  void   js_clear_display();
  int    js_button_pressed(int pin);
  double js_now();
}

namespace Platform {

  void Init() { /* web: nothing to init */ }

  bool ButtonPressed(Button b) { return js_button_pressed(static_cast<int>(b)) != 0; }
  unsigned long Millis()       { return static_cast<unsigned long>(js_now()); }
  
  // Non-blocking delay for web - just sleep the thread briefly
  void Delay(unsigned ms) { 
    if (ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms)); 
    }
  }

  // Reduce speed scaling for smoother web gameplay
  float SpeedScale() { return 1.0f; }  // Reduced from 3.0f

  int RandomInt(int min_inclusive, int max_exclusive) {
    static bool seeded = false;
    if (!seeded) { std::srand(static_cast<unsigned>(Millis())); seeded = true; }
    if (max_exclusive <= min_inclusive) return min_inclusive;
    return min_inclusive + (std::rand() % (max_exclusive - min_inclusive));
  }

  void ClearDisplay() { js_clear_display(); }
  void Present()      { js_display(); }

  void DrawPixel(int x,int y,bool on){
    if(x<0||y<0||x>=SCREEN_WIDTH||y>=SCREEN_HEIGHT) return;
    js_draw_pixel(x,y,on?1:0);
  }

  void DrawRect(int x,int y,int w,int h,bool on){
    for (int i=0;i<w;++i){ 
      DrawPixel(x+i,y,on); 
      if (h > 1) DrawPixel(x+i,y+h-1,on); 
    }
    for (int j=0;j<h;++j){ 
      DrawPixel(x,y+j,on); 
      if (w > 1) DrawPixel(x+w-1,y+j,on); 
    }
  }

  void FillRect(int x,int y,int w,int h,bool on){
    for (int j=0;j<h;++j)
      for (int i=0;i<w;++i)
        DrawPixel(x+i,y+j,on);
  }

  void DrawLine(int x0,int y0,int x1,int y1,bool on){
    int dx=std::abs(x1-x0), sx=x0<x1?1:-1;
    int dy=-std::abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy, e2;
    while(true){
      DrawPixel(x0,y0,on);
      if(x0==x1&&y0==y1)break;
      e2=2*err;
      if(e2>=dy){ err+=dy; x0+=sx; }
      if(e2<=dx){ err+=dx; y0+=sy; }
    }
  }

  // 5x7 text rasterizer
  static void DrawChar(int x,int y,char c,int scale,bool on){
    if(c<32||c>127) c='?';
    const uint8_t* g = ::FONT5x7[c-32];
    for(int col=0; col<5; ++col){
      uint8_t bits=g[col];
      for(int row=0; row<7; ++row){
        if(bits&(1<<row)){
          for(int dx=0; dx<scale; ++dx)
            for(int dy=0; dy<scale; ++dy)
              DrawPixel(x+col*scale+dx, y+row*scale+dy, on);
        }
      }
    }
  }

  void DrawText(int x,int y,const char* t,int scale,bool on){
    int cx=x;
    for(const char* p=t; *p; ++p){
      if(*p=='\n'){ y+=8*scale; cx=x; continue; }
      DrawChar(cx,y,*p,scale,on);
      cx+=6*scale;
    }
  }

  // Storage via localStorage with better error handling
  bool StorageGet(const char* key, int& outVal) {
    int ok = EM_ASM_INT({
      var k = UTF8ToString($0);
      try {
        if (typeof localStorage === 'undefined') return 0;
        var s = localStorage.getItem(k);
        if (s === null || s === undefined) return 0;
        var val = parseInt(s, 10);
        if (isNaN(val)) return 0;
        setValue($1, val, 'i32');
        return 1;
      } catch(e) {
        console.warn('Storage error:', e);
        return 0;
      }
    }, key, &outVal);
    return ok != 0;
  }

  void StorageSet(const char* key, int value) {
    EM_ASM({
      var k = UTF8ToString($0);
      var v = $1|0;
      try {
        if (typeof localStorage !== 'undefined' && v >= 0) {
          localStorage.setItem(k, v.toString());
        }
      } catch(e) {
        console.warn('Storage save error:', e);
      }
    }, key, value);
  }

} // namespace Platform

#endif // ARDUINO