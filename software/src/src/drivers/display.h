// display.h - Display abstraction layer
//
// Copyright (c) 2016 Patrick Dowling
// Modified for ILI9341 support

#ifndef DRIVERS_DISPLAY_H_
#define DRIVERS_DISPLAY_H_

#include "framebuffer.h"
#include "page_display_driver.h"
#include "SH1106_128x64_driver.h"
#include "weegfx.h"

namespace display {

extern FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;
extern PagedDisplayDriver<SH1106_128x64_Driver> driver;

void Init();
void AdjustOffset(uint8_t offset);
void SetFlipMode(bool flip180);
void SetContrast(uint8_t contrast);

static inline void Flush() __attribute__((always_inline));
static inline void Flush() {
  if (driver.Flush())
    frame_buffer.read();
}

static inline void Update() __attribute__((always_inline));
static inline void Update() {
  if (driver.frame_valid()) {
    driver.Update();
  } else {
    if (frame_buffer.readable())
      driver.Begin(frame_buffer.readable_frame());
  }
}

};

extern weegfx::Graphics graphics;

#define GRAPHICS_BEGIN_FRAME(wait) \
do { \
  uint8_t *frame = NULL; \
  do { \
    if (display::frame_buffer.writeable()) \
      frame = display::frame_buffer.writeable_frame(); \
  } while (!frame && wait); \
  if (frame) { \
    graphics.Begin(frame, weegfx::CLEAR_FRAME_ENABLE); \
    do {} while(0)

#define GRAPHICS_END_FRAME() \
    graphics.End(); \
    display::frame_buffer.written(); \
  } \
} while (0)


#endif // DRIVERS_DISPLAY_H_
