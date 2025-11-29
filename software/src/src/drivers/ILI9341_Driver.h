// ILI9341_Driver.h - ILI9341 TFT Display Driver for O_C Phazerville
// 
// This driver provides an interface compatible with the original SH1106 OLED driver
// but outputs to an ILI9341 320x240 TFT display. The original 128x64 pixel content
// is scaled up and centered on the display.
//
// ILI9341 Display Pinout (for Teensy 4.1):
// - GND → GND
// - VCC → 3.3V
// - CS → Pin 10
// - RST → Pin 8  
// - DC → Pin 9
// - MOSI → Pin 11 (SPI MOSI)
// - SCK → Pin 13 (SPI CLK)
// - MISO → Pin 12 (optional, for read operations)
//
// Copyright (c) 2024

#ifndef ILI9341_DRIVER_H_
#define ILI9341_DRIVER_H_

#include <stdint.h>
#include <string.h>
#include <SPI.h>
#include <ILI9341_t3.h>

// Pin definitions - can be overridden in platformio.ini
#ifndef ILI9341_CS_PIN
#define ILI9341_CS_PIN 10
#endif

#ifndef ILI9341_DC_PIN
#define ILI9341_DC_PIN 9
#endif

#ifndef ILI9341_RST_PIN
#define ILI9341_RST_PIN 8
#endif

#ifndef ILI9341_MOSI_PIN
#define ILI9341_MOSI_PIN 11
#endif

#ifndef ILI9341_SCK_PIN
#define ILI9341_SCK_PIN 13
#endif

#ifndef ILI9341_MISO_PIN
#define ILI9341_MISO_PIN 12
#endif

// Color definitions for monochrome emulation
#define ILI9341_BG_COLOR ILI9341_BLACK
#define ILI9341_FG_COLOR ILI9341_WHITE

// Scaling factor for displaying 128x64 content on 320x240
// Using 2x scale centers nicely: 128*2=256 (centered in 320), 64*2=128 (centered in 240)
#define DISPLAY_SCALE 2
#define DISPLAY_OFFSET_X ((320 - 128 * DISPLAY_SCALE) / 2)
#define DISPLAY_OFFSET_Y ((240 - 64 * DISPLAY_SCALE) / 2)

// This struct provides API compatibility with SH1106_128x64_Driver
struct ILI9341_Driver {
  // Frame buffer dimensions compatible with SH1106
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages = 8;
  static constexpr size_t kPageSize = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 0;
  
  // ILI9341 native dimensions
  static constexpr size_t kNativeWidth = 320;
  static constexpr size_t kNativeHeight = 240;
  
  // Source dimensions (from SH1106)
  static constexpr size_t kSourceWidth = 128;
  static constexpr size_t kSourceHeight = 64;

  static void Init();
  static void Clear();
  static void Flush();
  static bool SendPage(uint_fast8_t index, const uint8_t *data);
  static void SPI_send(void *bufr, size_t n);

  // Compatibility methods
  static void AdjustOffset(uint8_t offset);
  static void ChangeSpeed(uint32_t speed);
  static void SetFlipMode(bool flip180);
  static void SetContrast(uint8_t contrast);
  
  // Full frame update method for better performance
  static void UpdateDisplay(const uint8_t* frame_buffer);
  
private:
  static void DrawScaledPixel(int x, int y, bool on);
};

// Alias for compatibility with existing code that references SH1106
#ifdef USE_ILI9341_DISPLAY
using SH1106_128x64_Driver = ILI9341_Driver;
#endif

#endif // ILI9341_DRIVER_H_
