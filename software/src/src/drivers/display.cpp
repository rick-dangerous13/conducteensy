// display.cpp - Display abstraction layer implementation
//
// Copyright (c) 2016 Patrick Dowling
// Modified for ILI9341 support

#include "display.h"

namespace display {

FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;
PagedDisplayDriver<SH1106_128x64_Driver> driver;

void Init() {
  frame_buffer.Init();
  driver.Init();
}

void AdjustOffset(uint8_t offset) {
  SH1106_128x64_Driver::AdjustOffset(offset);
}

void SetFlipMode(bool flip180) {
  SH1106_128x64_Driver::SetFlipMode(flip180);
}

void SetContrast(uint8_t contrast) {
  SH1106_128x64_Driver::SetContrast(contrast);
}

}; // namespace display

weegfx::Graphics graphics;
