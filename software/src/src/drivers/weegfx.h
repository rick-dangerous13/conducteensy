// weegfx.h - Lightweight graphics library for embedded displays
//
// Copyright (c) 2016 Patrick Dowling

#ifndef WEEGFX_H_
#define WEEGFX_H_

#include <stdint.h>
#include <stddef.h>

namespace weegfx {

enum ClearFrame {
  CLEAR_FRAME_DISABLE,
  CLEAR_FRAME_ENABLE
};

class Graphics {
public:
  static constexpr size_t kWidth = 128;
  static constexpr size_t kHeight = 64;
  static constexpr size_t kFrameSize = kWidth * kHeight / 8;

  void Begin(uint8_t *frame, ClearFrame clear_frame);
  void End();

  void setPixel(int x, int y);
  void clearPixel(int x, int y);

  void drawHLine(int x, int y, int w);
  void drawVLine(int x, int y, int h);
  void drawLine(int x0, int y0, int x1, int y1);
  void drawRect(int x, int y, int w, int h);
  void drawFrame(int x, int y, int w, int h);
  void invertRect(int x, int y, int w, int h);
  void drawCircle(int x, int y, int r);
  
  void drawBitmap8(int x, int y, int w, const uint8_t *data);
  
  void setPrintPos(int x, int y);
  void print(char c);
  void print(const char *s);
  void print(int n);
  void printf(const char *fmt, ...);
  
  void drawStr(int x, int y, const char *s);

  int getPrintX() const { return print_x_; }
  int getPrintY() const { return print_y_; }

private:
  uint8_t *frame_;
  int print_x_;
  int print_y_;
  
  void plot(int x, int y);
  void unplot(int x, int y);
  bool valid(int x, int y) const {
    return x >= 0 && x < static_cast<int>(kWidth) && 
           y >= 0 && y < static_cast<int>(kHeight);
  }
};

}; // namespace weegfx

#endif // WEEGFX_H_
