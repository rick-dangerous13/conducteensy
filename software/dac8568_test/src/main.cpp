/*
 * DAC8568 Test Application for Teensy 4.1
 * 
 * Tests the TI DAC8568 8-channel 16-bit DAC BoosterPack
 * Outputs test voltages on all 8 channels to verify functionality
 * 
 * Hardware Connections (from Polyphonion project):
 * - SCLK  → Pin 13 (SPI Clock, shared bus)
 * - MOSI  → Pin 11 (SPI Data, shared bus)
 * - /SYNC → Pin 16 (Chip Select, active LOW)
 * - /LDAC → GND (tied for immediate updates)
 * - /CLR  → 5V (tied HIGH to disable clear)
 * - VDD   → 5V (CRITICAL: must be 5V for full range)
 * - GND   → GND
 * 
 * Output Channels:
 * - VOUTA (Ch 0) → CV Out 1
 * - VOUTB (Ch 1) → CV Out 2
 * - VOUTC (Ch 2) → CV Out 3
 * - VOUTD (Ch 3) → CV Out 4
 * - VOUTE (Ch 4) → CV Out 5
 * - VOUTF (Ch 5) → CV Out 6
 * - VOUTG (Ch 6) → CV Out 7
 * - VOUTH (Ch 7) → CV Out 8
 * 
 * Date: December 3, 2025
 */

#include <SPI.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>

// Display pins (same as O_C project)
#define TFT_DC  9
#define TFT_CS  10
#define TFT_RST 8

// Touch screen pins
#define TOUCH_CS 4
#define TOUCH_IRQ 2

// Touch calibration values for landscape mode (rotation 3)
#define TS_MINX 3500
#define TS_MINY 3560
#define TS_MAXX 280
#define TS_MAXY 320

// Touch button dimensions
#define BUTTON_HEIGHT 50
#define BUTTON_Y 190
#define BUTTON_LEFT_X 10
#define BUTTON_RIGHT_X 170
#define BUTTON_WIDTH 140

// DAC pins
#define DAC_CS_PIN 16  // /SYNC pin (chip select)
#define DAC_RST_PIN 17 // Hardware reset line (if wired)

// Initialize display and touch
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Test state management
int currentTest = -1;  // -1 = main menu, 0-9 = test numbers
bool testInProgress = false;
const int totalTests = 10;
bool testWaiting = false;

// DAC8568 Command definitions (24-bit frame format)
// Based on datasheet Table 6. Command Definition
#define DAC8568_CMD_WRITE_INPUT       0x00  // Write to input register only (no DAC update)
#define DAC8568_CMD_UPDATE_DAC        0x01  // Update DAC register from input register
#define DAC8568_CMD_WRITE_UPDATE_ALL  0x02  // Write to input register, update all DAC registers
#define DAC8568_CMD_WRITE_UPDATE      0x03  // Write to input register and update DAC register
#define DAC8568_CMD_POWER             0x04  // Power down/up control
#define DAC8568_CMD_CLEAR             0x05  // Clear code register
#define DAC8568_CMD_LDAC              0x06  // LDAC register
#define DAC8568_CMD_RESET             0x07  // Software reset
#define DAC8568_CMD_REFERENCE         0x08  // Set up internal reference (DAC8568 specific)

// DAC channel addresses
#define DAC_CH_A  0x00
#define DAC_CH_B  0x01
#define DAC_CH_C  0x02
#define DAC_CH_D  0x03
#define DAC_CH_E  0x04
#define DAC_CH_F  0x05
#define DAC_CH_G  0x06
#define DAC_CH_H  0x07
#define DAC_CH_ALL 0x0F

// SPI settings for DAC8568
// Max clock: 50 MHz, but we'll use 1 MHz for initial testing (increase if working)
// Mode: SPI_MODE1 (CPOL=0, CPHA=1) - most common for DAC856x series
// DAC8568 datasheet: Data latched on rising edge of SCLK
// O_C uses SPI_MODE2 (CPOL=1, CPHA=0) for DAC8568
SPISettings dacSPISettings(1000000, MSBFIRST, SPI_MODE2);

// Debug flag - set to true to see all SPI commands
bool debugSPI = false;

// Button structure
struct Button {
  int x, y, w, h;
  const char* label;
  uint16_t color;
};

Button btnPrev = {10, 210, 100, 25, "< PREV", ILI9341_BLUE};
Button btnNext = {210, 210, 100, 25, "NEXT >", ILI9341_BLUE};
Button btnRun = {115, 210, 90, 25, "RUN", ILI9341_GREEN};

void drawButton(Button &btn, bool pressed = false) {
  uint16_t color = pressed ? ILI9341_DARKGREY : btn.color;
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 5, color);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 5, ILI9341_WHITE);
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(btn.label, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(btn.x + (btn.w - w) / 2, btn.y + (btn.h - h) / 2);
  tft.print(btn.label);
}

void drawNavigationButtons() {
  drawButton(btnPrev, currentTest == 0);
  drawButton(btnRun);
  drawButton(btnNext, currentTest >= totalTests - 1);
}

bool isTouched(Button &btn, int x, int y) {
  return (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h);
}

TS_Point getTouchPoint() {
  TS_Point p = touch.getPoint();
  // Map touch coordinates to screen coordinates
  // Adjust these values based on your touch screen calibration
  int x = map(p.x, 200, 3700, 0, 320);
  int y = map(p.y, 300, 3800, 0, 240);
  return {x, y, p.z};
}

// Display helper functions
void displayTestHeader(const char* testName, int testNum) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("TEST ");
  tft.print(testNum);
  tft.print("/");
  tft.print(totalTests);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 35);
  tft.setTextSize(2);
  tft.println(testName);
  tft.drawLine(0, 60, 320, 60, ILI9341_CYAN);
  drawNavigationButtons();
}

void displayExpectedVoltage(const char* label, float voltage, int yPos) {
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, yPos);
  tft.print(label);
  tft.print(": ");
  tft.setTextColor(ILI9341_WHITE);
  tft.print(voltage, 3);
  tft.print("V");
}

void displayMessage(const char* msg, uint16_t color, int yPos) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(10, yPos);
  tft.println(msg);
}

void displayAllChannels(const uint16_t codes[8]) {
  int yStart = 70;
  int yStep = 20;
  const char channels[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
  
  for (int i = 0; i < 8; i++) {
    float v = (codes[i] / 65535.0) * 5.0;
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, yStart + i * yStep);
    tft.print("Ch");
    tft.print(channels[i]);
    tft.print(": ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(v, 3);
    tft.print("V");
  }
}

void drawNavigationButtons() {
  // Previous button (left)
  tft.fillRect(BUTTON_LEFT_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, 
               currentTest > 0 ? ILI9341_BLUE : ILI9341_DARKGREY);
  tft.drawRect(BUTTON_LEFT_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(BUTTON_LEFT_X + 30, BUTTON_Y + 15);
  tft.println("< PREV");
  
  // Next button (right)
  tft.fillRect(BUTTON_RIGHT_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, 
               currentTest < 9 ? ILI9341_GREEN : ILI9341_DARKGREY);
  tft.drawRect(BUTTON_RIGHT_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(BUTTON_RIGHT_X + 35, BUTTON_Y + 15);
  tft.println("NEXT >");
}

bool touchPreviousButton() {
  if (!touch.touched()) return false;
  
  TS_Point p = touch.getPoint();
  // Map raw touch coordinates to screen coordinates using calibration
  int x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  int y = map(p.y, TS_MINY, TS_MAXY, 0, 240);
  
  return (x > BUTTON_LEFT_X && x < BUTTON_LEFT_X + BUTTON_WIDTH &&
          y > BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT);
}

bool touchNextButton() {
  if (!touch.touched()) return false;
  
  TS_Point p = touch.getPoint();
  // Map raw touch coordinates to screen coordinates using calibration
  int x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  int y = map(p.y, TS_MINY, TS_MAXY, 0, 240);
  
  return (x > BUTTON_RIGHT_X && x < BUTTON_RIGHT_X + BUTTON_WIDTH &&
          y > BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT);
}

// Function declarations
void dacWriteCommand(uint8_t command, uint8_t address, uint16_t data);
void dacReset();
void dacSetReference(bool internalRef);
void dacPowerUpAll();
void setChannel(uint8_t channel, uint16_t value);
void setAllChannels(uint16_t value);
uint16_t voltageToDAC(float voltage);
float dacToVoltage(uint16_t dacValue);
void testIndividualChannels();
void testAllChannelsSweep();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize display
  tft.begin();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(ILI9341_BLACK);
  
  // Initialize touch screen
  touch.begin();
  touch.setRotation(1);
  
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(3);
  tft.setCursor(20, 80);
  tft.println("DAC8568 Test");
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Initializing...");
  tft.setTextSize(1);
  tft.setCursor(40, 150);
  tft.println("Touch enabled");
  
  Serial.println("\n=== DAC8568 Test Application ===");
  Serial.println("Teensy 4.1 + TI DAC8568 BoosterPack");
  Serial.println("Firmware compiled: " __DATE__ " " __TIME__);
  Serial.println();
  
  Serial.println("CRITICAL WIRING CHECK:");
  Serial.println("  VDD → 5V (NOT 3.3V)");
  Serial.println("  /LDAC → GND (immediate updates)");
  Serial.println("  /CLR → 5V (normal operation)");
  Serial.println("  /SYNC → Pin 16 (CS)");
  Serial.println("  SCLK → Pin 13");
  Serial.println("  MOSI → Pin 11");
  Serial.println();
  
  Serial.println("IMPORTANT: Internal 2.5V reference means:");
  Serial.println("  - Max output = 2.5V (NOT 5V)");
  Serial.println("  - 1.25V setting = actual 1.25V output");
  Serial.println("  - VREFOUT pin should measure ~2.5V");
  Serial.println();
  
  // Initialize SPI1 (MOSI=26, SCK=27 on Teensy 4.1)
  SPI1.begin();
  
  // Initialize control pins
  pinMode(DAC_CS_PIN, OUTPUT);
  digitalWrite(DAC_CS_PIN, HIGH);  // CS is active LOW
  pinMode(DAC_RST_PIN, OUTPUT);
  digitalWrite(DAC_RST_PIN, HIGH); // keep DAC out of reset
  // O_C assumes LDAC is tied to GND and CLR to 5V
  Serial.println("O_C hardware: LDAC should be GND, CLR should be 5V");
  
  Serial.println("Initializing DAC8568...");
  Serial.println();
  
  // Small delay for hardware to stabilize
  delay(100);
  
  // Reset DAC
  Serial.println("Step 1: Reset DAC");
  dacReset();
  
  // Configure internal reference
  Serial.println();
  Serial.println("Step 2: Enable Internal Reference");
  Serial.println("  (May not apply if using DAC8568A with external ref)");
  dacSetReference(true);
  
  // Power up all channels
  Serial.println();
  Serial.println("Step 3: Power Up All Channels");
  dacPowerUpAll();
  
  Serial.println();
  Serial.println("=== DAC8568 Initialization Complete ===");
  Serial.println();
}

// Send O_C-style 32-bit word: (cmd<<24)|(addr<<20)|(data<<4)
static void ocSend(uint8_t cmd, uint8_t addr, uint16_t data) {
  uint32_t word = ((uint32_t)cmd << 24) | ((uint32_t)addr << 20) | ((uint32_t)data << 4);
  SPI1.beginTransaction(dacSPISettings);
  digitalWrite(DAC_CS_PIN, LOW);
  SPI1.transfer((word >> 24) & 0xFF);
  SPI1.transfer((word >> 16) & 0xFF);
  SPI1.transfer((word >> 8) & 0xFF);
  SPI1.transfer(word & 0xFF);
  digitalWrite(DAC_CS_PIN, HIGH);
  SPI1.endTransaction();
}

// Helper to print expected voltage
void printExpected(const char* ch, uint16_t code) {
  float voltage = (code / 65535.0) * 5.0;
  Serial.print("  ");
  Serial.print(ch);
  Serial.print(": ");
  Serial.print(voltage, 3);
  Serial.println("V");
}

void runTest1_AllZero() {
  displayTestHeader("ALL CHANNELS ZERO", 1);
  
  Serial.println("\n========================================");
  Serial.println("TEST 1: ALL CHANNELS TO ZERO");
  Serial.println("========================================");
  Serial.println("Setting all 8 channels to 0x0000 (0V)");
  
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, 0x0000);
  }
  
  displayMessage("All channels set to:", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.setCursor(60, 100);
  tft.println("0.000 V");
  
  displayMessage("Measure all 8 outputs", ILI9341_YELLOW, 160);
  displayMessage("Expected: 0.0V +/- 0.01V", ILI9341_CYAN, 180);
  
  Serial.println("\nEXPECTED READINGS:");
  Serial.println("  ALL channels: 0.000V (within ±0.01V)");
  Serial.println("\nPLEASE MEASURE:");
  Serial.println("  VoutA, VoutB, VoutC, VoutD, VoutE, VoutF, VoutG, VoutH");
  Serial.println("  Report any channel NOT at 0.0V");
}

void runTest2_AllMax() {
  displayTestHeader("ALL CHANNELS MAX", 2);
  
  Serial.println("\n========================================");
  Serial.println("TEST 2: ALL CHANNELS TO MAXIMUM");
  Serial.println("========================================");
  Serial.println("Setting all 8 channels to 0xFFFF (5V)");
  
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, 0xFFFF);
  }
  
  displayMessage("All channels set to:", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.setCursor(60, 100);
  tft.println("5.000 V");
  
  displayMessage("Measure all 8 outputs", ILI9341_YELLOW, 160);
  displayMessage("Expected: 5.0V +/- 0.05V", ILI9341_CYAN, 180);
  
  Serial.println("\nEXPECTED READINGS:");
  Serial.println("  ALL channels: 5.000V (within ±0.05V)");
  Serial.println("\nPLEASE MEASURE:");
  Serial.println("  VoutA, VoutB, VoutC, VoutD, VoutE, VoutF, VoutG, VoutH");
  Serial.println("  Report any channel NOT at 5.0V");
}

void runTest3_AllMid() {
  displayTestHeader("ALL CHANNELS MID", 3);
  
  Serial.println("\n========================================");
  Serial.println("TEST 3: ALL CHANNELS TO MID-SCALE");
  Serial.println("========================================");
  Serial.println("Setting all 8 channels to 0x8000 (2.5V)");
  
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, 0x8000);
  }
  
  displayMessage("All channels set to:", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.setCursor(60, 100);
  tft.println("2.500 V");
  
  displayMessage("Measure all 8 outputs", ILI9341_YELLOW, 160);
  displayMessage("Expected: 2.5V +/- 0.03V", ILI9341_CYAN, 180);
  
  Serial.println("\nEXPECTED READINGS:");
  Serial.println("  ALL channels: 2.500V (within ±0.03V)");
  Serial.println("\nPLEASE MEASURE:");
  Serial.println("  VoutA, VoutB, VoutC, VoutD, VoutE, VoutF, VoutG, VoutH");
  Serial.println("  Report any channel NOT at 2.5V");
}

void runTest4_StaircaseAscending() {
  displayTestHeader("ASCENDING STAIRCASE", 4);
  
  Serial.println("\n========================================");
  Serial.println("TEST 4: ASCENDING STAIRCASE");
  Serial.println("========================================");
  Serial.println("Setting channels to ascending voltages:");
  
  const uint16_t codes[8] = {
    0x0000,  // A: 0V
    0x2492,  // B: ~0.714V (1/7 of 5V)
    0x4924,  // C: ~1.429V (2/7 of 5V)
    0x6DB6,  // D: ~2.143V (3/7 of 5V)
    0x9249,  // E: ~2.857V (4/7 of 5V)
    0xB6DB,  // F: ~3.571V (5/7 of 5V)
    0xDB6D,  // G: ~4.286V (6/7 of 5V)
    0xFFFF   // H: 5.000V
  };
  
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, codes[ch]);
  }
  
  displayAllChannels(codes);
  displayMessage("Each ~0.714V higher", ILI9341_YELLOW, 220);
  
  Serial.println("\nEXPECTED READINGS (ascending staircase):");
  printExpected("VoutA", codes[0]);
  printExpected("VoutB", codes[1]);
  printExpected("VoutC", codes[2]);
  printExpected("VoutD", codes[3]);
  printExpected("VoutE", codes[4]);
  printExpected("VoutF", codes[5]);
  printExpected("VoutG", codes[6]);
  printExpected("VoutH", codes[7]);
  Serial.println("\nPLEASE MEASURE ALL 8 CHANNELS");
  Serial.println("  Each should be ~0.714V higher than the previous");
}

void runTest5_StaircaseDescending() {
  displayTestHeader("DESCENDING STAIRCASE", 5);
  
  Serial.println("\n========================================");
  Serial.println("TEST 5: DESCENDING STAIRCASE");
  Serial.println("========================================");
  Serial.println("Setting channels to descending voltages:");
  
  const uint16_t codes[8] = {
    0xFFFF,  // A: 5.000V
    0xDB6D,  // B: ~4.286V
    0xB6DB,  // C: ~3.571V
    0x9249,  // D: ~2.857V
    0x6DB6,  // E: ~2.143V
    0x4924,  // F: ~1.429V
    0x2492,  // G: ~0.714V
    0x0000   // H: 0.000V
  };
  
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, codes[ch]);
  }
  
  displayAllChannels(codes);
  displayMessage("Each ~0.714V lower", ILI9341_YELLOW, 220);
  
  Serial.println("\nEXPECTED READINGS (descending staircase):");
  printExpected("VoutA", codes[0]);
  printExpected("VoutB", codes[1]);
  printExpected("VoutC", codes[2]);
  printExpected("VoutD", codes[3]);
  printExpected("VoutE", codes[4]);
  printExpected("VoutF", codes[5]);
  printExpected("VoutG", codes[6]);
  printExpected("VoutH", codes[7]);
  Serial.println("\nPLEASE MEASURE ALL 8 CHANNELS");
  Serial.println("  Each should be ~0.714V lower than the previous");
}

void runTest6_AlternatingPattern() {
  displayTestHeader("ALTERNATING PATTERN", 6);
  
  Serial.println("\n========================================");
  Serial.println("TEST 6: ALTERNATING HIGH/LOW PATTERN");
  Serial.println("========================================");
  Serial.println("Setting odd channels HIGH, even channels LOW:");
  
  uint16_t codes[8];
  for (uint8_t ch = 0; ch < 8; ++ch) {
    codes[ch] = (ch % 2 == 0) ? 0x0000 : 0xFFFF;
    ocSend(0x03, ch, codes[ch]);
  }
  
  displayAllChannels(codes);
  displayMessage("Pattern: 0-5-0-5-0-5-0-5", ILI9341_YELLOW, 220);
  
  Serial.println("\nEXPECTED READINGS:");
  Serial.println("  VoutA (ch0): 0.000V");
  Serial.println("  VoutB (ch1): 5.000V");
  Serial.println("  VoutC (ch2): 0.000V");
  Serial.println("  VoutD (ch3): 5.000V");
  Serial.println("  VoutE (ch4): 0.000V");
  Serial.println("  VoutF (ch5): 5.000V");
  Serial.println("  VoutG (ch6): 0.000V");
  Serial.println("  VoutH (ch7): 5.000V");
  Serial.println("\nPLEASE MEASURE ALL 8 CHANNELS");
  Serial.println("  Should alternate: 0V, 5V, 0V, 5V, 0V, 5V, 0V, 5V");
}

void runTest7_FineSteps() {
  displayTestHeader("FINE VOLTAGE SWEEP", 7);
  
  Serial.println("\n========================================");
  Serial.println("TEST 7: FINE VOLTAGE STEPS ON CHANNEL A");
  Serial.println("========================================");
  Serial.println("Testing DAC resolution on VoutA:");
  Serial.println("  Setting channels B-H to 0V");
  Serial.println("  Sweeping VoutA through fine steps");
  
  displayMessage("Sweeping Channel A", ILI9341_GREEN, 70);
  displayMessage("Channels B-H: 0V", ILI9341_CYAN, 90);
  
  // Set all other channels to 0
  for (uint8_t ch = 1; ch < 8; ++ch) {
    ocSend(0x03, ch, 0x0000);
  }
  
  const uint16_t fineSteps[10] = {
    0x0000,  // 0.000V
    0x0CCC,  // 0.500V
    0x1999,  // 1.000V
    0x2666,  // 1.500V
    0x3333,  // 2.000V
    0x4000,  // 2.500V
    0x4CCC,  // 3.000V
    0x5999,  // 3.500V
    0x6666,  // 4.000V
    0x7333   // 4.500V
  };
  
  Serial.println("\nSweeping VoutA in 0.5V steps:");
  for (uint8_t i = 0; i < 10; ++i) {
    ocSend(0x03, 0, fineSteps[i]);
    float v = (fineSteps[i] / 65535.0) * 5.0;
    
    // Update display with current step
    tft.fillRect(0, 110, 320, 80, ILI9341_BLACK);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(10, 120);
    tft.print("Step ");
    tft.print(i + 1);
    tft.print(" of 10\");
    
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.setCursor(40, 150);
    tft.print(\"VoutA: \");
    tft.print(v, 3);
    tft.print(\"V\");
    
    Serial.print("  Step ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(v, 3);
    Serial.print("V (code 0x");
    Serial.print(fineSteps[i], HEX);
    Serial.println(")");
    delay(1500);
  }
  
  displayMessage("Measure VoutA at each step\", ILI9341_YELLOW, 200);
  
  Serial.println("\nPLEASE MEASURE VoutA at each step");
  Serial.println("  Should match the values above (±0.02V)");
  Serial.println("  Channels B-H should remain at 0V");
}

void runTest8_IndividualChannels() {
  Serial.println("\n========================================");
  Serial.println("TEST 8: INDIVIDUAL CHANNEL ISOLATION");
  Serial.println("========================================");
  Serial.println("Testing each channel individually at 3.3V");
  Serial.println("(All others at 0V to check for crosstalk)");
  
  const uint16_t testCode = 0xA8F5;  // ~3.3V
  
  for (uint8_t activeChannel = 0; activeChannel < 8; ++activeChannel) {
    char channelNames[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    
    displayTestHeader("CHANNEL ISOLATION", 8);
    
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(10, 70);
    tft.print("Testing Channel ");
    tft.print(channelNames[activeChannel]);
    
    Serial.println();
    Serial.print("--- Setting ONLY Vout");
    Serial.print(channelNames[activeChannel]);
    Serial.println(" to 3.3V ---");
    
    // Set all to 0, then active channel to 3.3V
    uint16_t codes[8];
    for (uint8_t ch = 0; ch < 8; ++ch) {
      codes[ch] = (ch == activeChannel) ? testCode : 0x0000;
      ocSend(0x03, ch, codes[ch]);
    }
    
    displayAllChannels(codes);
    displayMessage("Press ENTER for next", ILI9341_GREEN, 220);
    
    Serial.println("EXPECTED:");
    for (uint8_t ch = 0; ch < 8; ++ch) {
      Serial.print("  Vout");
      Serial.print(channelNames[ch]);
      Serial.print(": ");
      Serial.println((ch == activeChannel) ? "3.300V" : "0.000V");
    }
    
    Serial.println("\nPLEASE MEASURE ALL 8 CHANNELS");
    Serial.println("  Only the active channel should be 3.3V");
    Serial.println("  All others should be 0V (no crosstalk)");
    Serial.println("  Press ENTER when ready for next channel...");
    
    // Wait for user
    while (!Serial.available()) {
      delay(100);
    }
    while (Serial.available()) Serial.read();  // Clear buffer
  }
}

void runTest9_PowerDown() {
  displayTestHeader("POWER-DOWN TEST", 9);
  
  Serial.println("\n========================================");
  Serial.println("TEST 9: POWER-DOWN MODES");
  Serial.println("========================================");
  Serial.println("Testing power-down functionality:");
  
  // First set all to 2.5V
  displayMessage("Step 1: Set all to 2.5V", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 100);
  tft.println("2.500 V");
  displayMessage("Press ENTER when measured", ILI9341_YELLOW, 180);
  
  Serial.println("\n1. Setting all channels to 2.5V (baseline)");
  for (uint8_t ch = 0; ch < 8; ++ch) {
    ocSend(0x03, ch, 0x8000);
  }
  delay(500);
  Serial.println("   MEASURE: All should be 2.5V");
  Serial.println("   Press ENTER when measured...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
  
  // Power down all channels (1kΩ to GND mode)
  tft.fillRect(0, 70, 320, 140, ILI9341_BLACK);
  displayMessage("Step 2: Power-down mode", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.setCursor(40, 100);
  tft.println("~0.000 V");
  displayMessage("1k ohm to GND", ILI9341_CYAN, 140);
  displayMessage("Press ENTER when measured", ILI9341_YELLOW, 180);
  
  Serial.println("\n2. Powering down all channels (1kΩ to GND)");
  ocSend(0x04, 0x00, 0x00FF);  // Power down bits: 2 bits per channel, 01=1kΩ to GND
  delay(100);
  Serial.println("   EXPECTED: All channels near 0V");
  Serial.println("   (DAC outputs driven to GND through 1kΩ)");
  Serial.println("   Press ENTER when measured...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
  
  // Power back up
  tft.fillRect(0, 70, 320, 140, ILI9341_BLACK);
  displayMessage("Step 3: Power restored", ILI9341_GREEN, 70);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 100);
  tft.println("2.500 V");
  displayMessage("Should restore to 2.5V", ILI9341_CYAN, 140);
  displayMessage("Press ENTER when measured", ILI9341_YELLOW, 180);
  
  Serial.println("\n3. Powering up all channels");
  ocSend(0x04, 0x00, 0x0000);  // Normal operation
  delay(100);
  Serial.println("   EXPECTED: All channels back to 2.5V");
  Serial.println("   (DAC should restore previous values)");
  Serial.println("   Press ENTER when measured...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
}

void runTest10_UpdateModes() {
  displayTestHeader("UPDATE MODES", 10);
  
  Serial.println("\n========================================");
  Serial.println("TEST 10: UPDATE MODES VERIFICATION");
  Serial.println("========================================");
  Serial.println("Testing different update commands:");
  
  // Test WRITE_INPUT (0x00) vs WRITE_UPDATE (0x03)
  displayMessage("Step 1: Write input only", ILI9341_GREEN, 70);
  displayMessage("CMD 0x00: No update", ILI9341_CYAN, 90);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 120);
  tft.println("VoutA should NOT change");
  displayMessage("Press ENTER after measuring", ILI9341_WHITE, 180);
  
  Serial.println("\n1. Write to input register WITHOUT update (cmd 0x00)");
  Serial.println("   Writing 0xFFFF to channel A input register");
  ocSend(0x00, 0x00, 0xFFFF);  // Write to input only
  delay(100);
  Serial.println("   EXPECTED: VoutA should NOT change yet");
  Serial.println("   Press ENTER after measuring VoutA...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
  
  tft.fillRect(0, 70, 320, 140, ILI9341_BLACK);
  displayMessage("Step 2: Update from input", ILI9341_GREEN, 70);
  displayMessage("CMD 0x01: Update", ILI9341_CYAN, 90);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 120);
  tft.println("VoutA: 5.0V");
  displayMessage("Press ENTER after measuring", ILI9341_YELLOW, 180);
  
  Serial.println("\n2. Update DAC from input register (cmd 0x01)");
  ocSend(0x01, 0x00, 0x0000);  // Update DAC from input
  delay(100);
  Serial.println("   EXPECTED: VoutA should NOW be 5.0V");
  Serial.println("   Press ENTER after measuring VoutA...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
  
  tft.fillRect(0, 70, 320, 140, ILI9341_BLACK);
  displayMessage("Step 3: Write & update all", ILI9341_GREEN, 70);
  displayMessage("CMD 0x02: All channels", ILI9341_CYAN, 90);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, 120);
  tft.println("All: 1.25V");
  displayMessage("Press ENTER after measuring", ILI9341_YELLOW, 180);
  
  Serial.println("\n3. Write and update all channels simultaneously (cmd 0x02)");
  ocSend(0x02, 0x0F, 0x4000);  // Write to all input regs and update all DACs
  delay(100);
  Serial.println("   EXPECTED: ALL channels should be 1.25V");
  Serial.println("   Press ENTER after measuring all channels...");
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
}

void runCurrentTest() {
  switch(currentTest) {
    case 0: runTest1_AllZero(); break;
    case 1: runTest2_AllMax(); break;
    case 2: runTest3_AllMid(); break;
    case 3: runTest4_StaircaseAscending(); break;
    case 4: runTest5_StaircaseDescending(); break;
    case 5: runTest6_AlternatingPattern(); break;
    case 6: runTest7_FineSteps(); break;
    case 7: runTest8_IndividualChannels(); break;
    case 8: runTest9_PowerDown(); break;
    case 9: runTest10_UpdateModes(); break;
  }
}

void showTestMenu() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(3);
  tft.setCursor(20, 20);
  tft.println("DAC8568 Tests");
  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Test ");
  tft.print(currentTest + 1);
  tft.print(" of ");
  tft.println(totalTests);
  
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(20, 90);
  tft.println("Use buttons to navigate:");
  tft.setCursor(20, 110);
  tft.println("PREV - Previous test");
  tft.setCursor(20, 125);
  tft.println("RUN  - Execute current test");
  tft.setCursor(20, 140);
  tft.println("NEXT - Next test");
  
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 170);
  const char* testNames[] = {
    "All Zero", "All Max", "All Mid",
    "Ascending", "Descending", "Alternating",
    "Fine Steps", "Isolation", "Power Down", "Update Modes"
  };
  tft.println(testNames[currentTest]);
  
  drawNavigationButtons();
}

void loop() {
  static bool inited = false;
  if (!inited) {
    inited = true;
    touch.begin();
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("DAC8568 COMPREHENSIVE TEST SUITE");
    Serial.println("========================================");
    Serial.println("O_C-compatible SPI_MODE2 configuration");
    Serial.println("External 5V reference assumed");
    Serial.println("Touch screen enabled");
    Serial.println();
    
    // Initialize DAC
    Serial.println("Initializing DAC...");
    ocSend(0x08, 0x00, 0x0001);  // Enable internal ref (may not apply to 8568A)
    delay(10);
    ocSend(0x04, 0x00, 0x0000);  // Power up all
    delay(10);
    
    // Show main menu
    currentTest = 0;
    
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(3);
    tft.setCursor(20, 40);
    tft.println("DAC8568 TEST");
    tft.setTextSize(2);
    tft.setCursor(40, 100);
    tft.setTextColor(ILI9341_WHITE);
    tft.println("10 Comprehensive Tests");
    tft.setCursor(20, 130);
    tft.println("Use NEXT/PREV buttons");
    tft.setCursor(20, 155);
    tft.println("to navigate tests");
    drawNavigationButtons();
    
    Serial.println("========================================");
    Serial.println("READY - Use touch buttons to navigate");
    Serial.println("========================================");
  }
  
  // Handle touch input
  if (touchNextButton()) {
    if (currentTest < 9) {
      currentTest++;
      delay(300);  // Debounce
    }
  }
  
  if (touchPreviousButton()) {
    if (currentTest > 0) {
      currentTest--;
      delay(300);  // Debounce
    }
  }
  
  // Run selected test
  switch(currentTest) {
    case 0: runTest1_AllZero(); break;
    case 1: runTest2_AllMax(); break;
    case 2: runTest3_AllMid(); break;
    case 3: runTest4_StaircaseAscending(); break;
    case 4: runTest5_StaircaseDescending(); break;
    case 5: runTest6_AlternatingPattern(); break;
    case 6: runTest7_FineSteps(); break;
    case 7: runTest8_IndividualChannels(); break;
    case 8: runTest9_PowerDown(); break;
    case 9: runTest10_UpdateModes(); break;
  }
}

void dacReset() {
  // Software reset command - DAC8568 uses command 0x07
  Serial.println("  Sending reset command (CMD=0x07)...");
  ocSend(DAC8568_CMD_RESET, 0x00, 0x0000);
  delay(10);  // Wait for reset to complete
  Serial.println("  DAC reset complete (waited 10ms)");
}

void dacSetReference(bool internalRef) {
  // Set reference mode - DAC8568 specific command 0x08
  Serial.print("  Setting reference (CMD=0x08) to: ");
  Serial.println(internalRef ? "Internal 2.5V (always on)" : "External (flexible mode)");
  
  uint16_t refData = internalRef ? 0x0001 : 0x0000;
  ocSend(DAC8568_CMD_REFERENCE, 0x00, refData);
  delay(20);  // Wait for reference to stabilize
  Serial.println("  Reference command sent (waited 20ms for stabilization)");
}

void dacPowerUpAll() {
  // Power up all channels - Command 0x04
  Serial.println("  Powering up all channels (CMD=0x04, DATA=0x0000)...");
  ocSend(DAC8568_CMD_POWER, 0x00, 0x0000);
  delay(5);  // Small delay for power-up
  Serial.println("  All channels powered up (normal operation)");
}

void setChannel(uint8_t channel, uint16_t value) {
  // Write and update specific channel immediately
  if (channel > 7) return;
  dacWriteCommand(DAC8568_CMD_WRITE_UPDATE, channel, value);
}

void setAllChannels(uint16_t value) {
  // Write and update all channels to same value
  dacWriteCommand(DAC8568_CMD_WRITE_UPDATE_ALL, DAC_CH_ALL, value);
}

uint16_t voltageToDAC(float voltage) {
  // Convert voltage (0-5V) to 16-bit DAC value (0-65535)
  if (voltage < 0) voltage = 0;
  if (voltage > 5.0) voltage = 5.0;
  return (uint16_t)((voltage / 5.0) * 65535.0);
}

float dacToVoltage(uint16_t dacValue) {
  // Convert 16-bit DAC value to voltage
  return (dacValue / 65535.0) * 5.0;
}

// === Test Functions ===

void testIndividualChannels() {
  const char channelNames[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
  
  for (int ch = 0; ch < 8; ch++) {
    Serial.print("  Testing Channel ");
    Serial.print(ch);
    Serial.print(" (VOUT");
    Serial.print(channelNames[ch]);
    Serial.println("):");
    
    // Sweep from 0V to 5V
    Serial.println("    Sweeping 0V → 5V...");
    for (uint16_t val = 0; val <= 65535; val += 6553) {  // ~10 steps
      setChannel(ch, val);
      float voltage = dacToVoltage(val);
      Serial.print("      ");
      Serial.print(voltage, 2);
      Serial.println("V");
      delay(300);
    }
    
    // Set to 0V before next channel
    setChannel(ch, 0);
    Serial.println("    Reset to 0V");
    delay(500);
  }
}

void testAllChannelsSweep() {
  Serial.println("  Sweeping all channels together 0V → 5V → 0V");
  
  // Sweep up
  for (uint16_t val = 0; val <= 65535; val += 2048) {
    setAllChannels(val);
    float voltage = dacToVoltage(val);
    Serial.print("    All channels: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    delay(200);
  }
  
  delay(1000);
  
  // Sweep down
  for (uint16_t val = 65535; val > 0; val -= 2048) {
    setAllChannels(val);
    delay(200);
  }
  setAllChannels(0);
  
  Serial.println("  Sweep complete");
  delay(1000);
}
