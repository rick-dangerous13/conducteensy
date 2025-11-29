// Main.cpp - O_C Phazerville with ILI9341 Display
//
// This is a simplified main file for Teensy 4.1 with ILI9341 TFT display
// Based on O_C-Phazerville firmware by djphazer
//
// ILI9341 Display Pinout:
// - GND → GND
// - VCC → 3.3V
// - CS → Pin 10
// - RST → Pin 8  
// - DC → Pin 9
// - MOSI → Pin 11 (SPI MOSI)
// - SCK → Pin 13 (SPI CLK)
// - MISO → Pin 12 (optional)

#include <Arduino.h>
#include "src/drivers/display.h"

// Version information
#define OC_VERSION_MAJOR 1
#define OC_VERSION_MINOR 0
#define OC_VERSION_PATCH 0

#ifndef OC_VERSION_EXTRA
#define OC_VERSION_EXTRA ""
#endif

// Forward declarations
void setup();
void loop();

// Timing
static uint32_t last_redraw = 0;
static const uint32_t REDRAW_INTERVAL_MS = 33; // ~30 FPS

// Animation state
static int frame_count = 0;
static int ball_x = 64;
static int ball_y = 32;
static int ball_dx = 2;
static int ball_dy = 1;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(100);
  
  Serial.println("O_C Phazerville - ILI9341 Display");
  Serial.println("Initializing...");
  
  // Initialize display subsystem
  display::Init();
  
  Serial.println("Display initialized");
  Serial.println("Starting main loop...");
}

void loop() {
  uint32_t now = millis();
  
  // Redraw at fixed interval
  if (now - last_redraw >= REDRAW_INTERVAL_MS) {
    last_redraw = now;
    frame_count++;
    
    // Update ball position
    ball_x += ball_dx;
    ball_y += ball_dy;
    
    // Bounce off walls
    if (ball_x <= 4 || ball_x >= 123) ball_dx = -ball_dx;
    if (ball_y <= 4 || ball_y >= 59) ball_dy = -ball_dy;
    
    // Begin frame
    GRAPHICS_BEGIN_FRAME(true);
    
    // Draw border
    graphics.drawFrame(0, 0, 128, 64);
    
    // Draw title
    graphics.drawStr(20, 2, "O_C Phazerville");
    graphics.drawStr(28, 12, "ILI9341 Demo");
    
    // Draw animated ball
    graphics.drawCircle(ball_x, ball_y, 4);
    
    // Draw frame counter
    graphics.setPrintPos(2, 54);
    graphics.printf("Frame: %d", frame_count);
    
    // Draw version info
    graphics.setPrintPos(70, 54);
    graphics.printf("v%d.%d.%d%s", 
      OC_VERSION_MAJOR, OC_VERSION_MINOR, OC_VERSION_PATCH,
      OC_VERSION_EXTRA);
    
    GRAPHICS_END_FRAME();
    
    // Update display
    display::Update();
    display::Flush();
  }
  
  // Handle display updates
  display::Update();
}
