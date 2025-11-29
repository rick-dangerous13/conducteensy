// SH1106_128x64_driver.h - Display Driver Selection Header
//
// This header provides compatibility by selecting either:
// - Original SH1106 OLED driver
// - ILI9341 TFT driver (when USE_ILI9341_DISPLAY is defined)
//
// Copyright (c) 2016 Patrick Dowling (original SH1106)
// Copyright (c) 2024 (ILI9341 adaptation)

#ifndef SH1106_128X64_DRIVER_H_
#define SH1106_128X64_DRIVER_H_

#include <stdint.h>
#include <string.h>

#ifdef USE_ILI9341_DISPLAY

// Use ILI9341 TFT display driver
#include "ILI9341_Driver.h"

#else

// Original SH1106 OLED driver structure
struct SH1106_128x64_Driver {
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages = 8;
  static constexpr size_t kPageSize = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 2;

  static void Init();
  static void Clear();
  static void Flush();
  static bool SendPage(uint_fast8_t index, const uint8_t *data);
  static void SPI_send(void *bufr, size_t n);

  // SH1106 ram is 132x64, so it needs an offset to center data in display.
  // However at least one display (mine) uses offset 0 so it's minimally
  // configurable
  static void AdjustOffset(uint8_t offset);
  static void ChangeSpeed(uint32_t speed);
  static void SetFlipMode(bool flip180);
  static void SetContrast(uint8_t contrast);
};

#endif // USE_ILI9341_DISPLAY

#endif // SH1106_128X64_DRIVER_H_
