// ILI9341_Driver.cpp - ILI9341 TFT Display Driver Implementation
// 
// This driver provides an interface compatible with the original SH1106 OLED driver
// but outputs to an ILI9341 320x240 TFT display.
//
// Copyright (c) 2024

#include <Arduino.h>
#include "ILI9341_Driver.h"

// Global ILI9341 display instance
static ILI9341_t3 tft(ILI9341_CS_PIN, ILI9341_DC_PIN, ILI9341_RST_PIN);

// Frame buffer for page-based updates
static uint8_t page_buffer[ILI9341_Driver::kNumPages][ILI9341_Driver::kPageSize];
static bool page_dirty[ILI9341_Driver::kNumPages];
static bool display_initialized = false;
static bool flip_mode = false;

/*static*/
void ILI9341_Driver::Init() {
  // Initialize the ILI9341 display
  tft.begin();
  tft.setRotation(flip_mode ? 2 : 0);  // 0 or 2 for portrait, 1 or 3 for landscape
  tft.fillScreen(ILI9341_BG_COLOR);
  
  // Clear page tracking
  memset(page_dirty, false, sizeof(page_dirty));
  memset(page_buffer, 0, sizeof(page_buffer));
  
  display_initialized = true;
  
  // Draw border around the active area for visual reference
  int x = DISPLAY_OFFSET_X - 1;
  int y = DISPLAY_OFFSET_Y - 1;
  int w = kSourceWidth * DISPLAY_SCALE + 2;
  int h = kSourceHeight * DISPLAY_SCALE + 2;
  tft.drawRect(x, y, w, h, ILI9341_DARKGREY);
}

/*static*/
void ILI9341_Driver::Clear() {
  if (!display_initialized) return;
  
  // Clear only the content area (more efficient than full screen clear)
  tft.fillRect(DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
               kSourceWidth * DISPLAY_SCALE, 
               kSourceHeight * DISPLAY_SCALE, 
               ILI9341_BG_COLOR);
               
  memset(page_buffer, 0, sizeof(page_buffer));
  memset(page_dirty, false, sizeof(page_dirty));
}

/*static*/
void ILI9341_Driver::Flush() {
  // For ILI9341, flush is handled by the display library
  // Nothing special needed here
}

/*static*/
void ILI9341_Driver::DrawScaledPixel(int x, int y, bool on) {
  // Apply flip if needed
  if (flip_mode) {
    x = kSourceWidth - 1 - x;
    y = kSourceHeight - 1 - y;
  }
  
  // Scale and offset
  int screen_x = DISPLAY_OFFSET_X + x * DISPLAY_SCALE;
  int screen_y = DISPLAY_OFFSET_Y + y * DISPLAY_SCALE;
  
  uint16_t color = on ? ILI9341_FG_COLOR : ILI9341_BG_COLOR;
  
  // Draw scaled pixel (2x2 block for DISPLAY_SCALE=2)
  tft.fillRect(screen_x, screen_y, DISPLAY_SCALE, DISPLAY_SCALE, color);
}

/*static*/
bool ILI9341_Driver::SendPage(uint_fast8_t index, const uint8_t *data) {
  if (!display_initialized) return false;
  if (index >= kNumPages) return false;
  
  // Store page data and mark as dirty
  memcpy(page_buffer[index], data, kPageSize);
  page_dirty[index] = true;
  
  // Convert page-based format to pixels and draw
  // Each page is 128 bytes wide, 8 pixels tall
  // Bit 0 of each byte is the top pixel
  
  int page_y_start = index * 8;  // Each page is 8 pixels tall
  
  for (int col = 0; col < 128; col++) {
    uint8_t byte = data[col];
    for (int bit = 0; bit < 8; bit++) {
      int y = page_y_start + bit;
      bool pixel_on = (byte >> bit) & 1;
      DrawScaledPixel(col, y, pixel_on);
    }
  }
  
  return true;
}

/*static*/
void ILI9341_Driver::UpdateDisplay(const uint8_t* frame_buffer) {
  if (!display_initialized) return;
  
  // Process entire frame at once (more efficient than page-by-page)
  // Frame buffer is organized as 8 pages, each 128 bytes (128x8 pixels)
  
  for (int page = 0; page < kNumPages; page++) {
    const uint8_t* page_data = frame_buffer + (page * kPageSize);
    int page_y_start = page * 8;
    
    for (int col = 0; col < 128; col++) {
      uint8_t byte = page_data[col];
      for (int bit = 0; bit < 8; bit++) {
        int y = page_y_start + bit;
        bool pixel_on = (byte >> bit) & 1;
        DrawScaledPixel(col, y, pixel_on);
      }
    }
  }
}

/*static*/
void ILI9341_Driver::SPI_send([[maybe_unused]] void *bufr, [[maybe_unused]] size_t n) {
  // This method is provided for API compatibility
  // ILI9341_t3 library handles SPI internally
  // If needed for other purposes, implement direct SPI transfer here
}

/*static*/
void ILI9341_Driver::AdjustOffset([[maybe_unused]] uint8_t offset) {
  // Not applicable for ILI9341 - it doesn't have RAM offset issues like SH1106
}

/*static*/
void ILI9341_Driver::ChangeSpeed([[maybe_unused]] uint32_t speed) {
  // SPI speed is managed by ILI9341_t3 library
}

/*static*/
void ILI9341_Driver::SetFlipMode(bool flip180) {
  flip_mode = flip180;
  if (display_initialized) {
    tft.setRotation(flip180 ? 2 : 0);
  }
}

/*static*/
void ILI9341_Driver::SetContrast([[maybe_unused]] uint8_t contrast) {
  // ILI9341 doesn't have contrast control like OLED
  // Could potentially adjust brightness through backlight if connected
}
