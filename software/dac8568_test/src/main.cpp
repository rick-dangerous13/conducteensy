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

// Pin definitions
#define DAC_CS_PIN 16  // /SYNC pin (chip select)

// DAC8568 Command definitions
#define DAC8568_CMD_WRITE_UPDATE      0x03  // Write to input register and update DAC register
#define DAC8568_CMD_WRITE_UPDATE_ALL  0x02  // Write to input register, update all DAC registers
#define DAC8568_CMD_WRITE_INPUT       0x00  // Write to input register only (no update)
#define DAC8568_CMD_UPDATE_DAC        0x01  // Update DAC register from input register
#define DAC8568_CMD_POWER             0x04  // Power down/up control
#define DAC8568_CMD_CLEAR             0x05  // Clear code register
#define DAC8568_CMD_LDAC              0x06  // LDAC register
#define DAC8568_CMD_RESET             0x07  // Software reset
#define DAC8568_CMD_REFERENCE         0x08  // Set up internal reference

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
// Max clock: 50 MHz, but we'll use 10 MHz for stability
// Mode: SPI_MODE1 (CPOL=0, CPHA=1) - data valid on rising edge
SPISettings dacSPISettings(10000000, MSBFIRST, SPI_MODE1);

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
  delay(2000);  // Wait for serial monitor
  
  Serial.println("\n=== DAC8568 Test Application ===");
  Serial.println("Teensy 4.1 + TI DAC8568 BoosterPack\n");
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize CS pin
  pinMode(DAC_CS_PIN, OUTPUT);
  digitalWrite(DAC_CS_PIN, HIGH);  // CS is active LOW
  
  Serial.println("Initializing DAC8568...");
  delay(100);
  
  // Reset DAC
  dacReset();
  delay(10);
  
  // Configure internal reference (if using internal reference)
  // Set reference to always-on mode
  dacSetReference(true);
  delay(10);
  
  // Power up all channels
  dacPowerUpAll();
  delay(10);
  
  Serial.println("DAC8568 initialized successfully!\n");
  Serial.println("Starting test sequence...");
  Serial.println("Monitor CV outputs with multimeter\n");
  
  // Initial test: Set all channels to mid-scale (2.5V)
  Serial.println("Test 1: Setting all channels to 2.5V (mid-scale)");
  setAllChannels(32768);  // 50% of 65535
  delay(3000);
  
  // Test individual channels
  Serial.println("\nTest 2: Individual channel sweep (0V → 5V)");
  testIndividualChannels();
  
  Serial.println("\nTest 3: All channels sweep together");
  testAllChannelsSweep();
  
  Serial.println("\n=== Test Complete ===");
  Serial.println("\nSetting channels to fixed voltages:");
  Serial.println("  Channels 0-3: Maximum voltage (5.0V)");
  Serial.println("  Channels 4-7: Half maximum voltage (2.5V)\n");
}

void loop() {
  // Set channels 0-3 to maximum voltage (5V)
  for (int ch = 0; ch < 4; ch++) {
    setChannel(ch, 65535);  // Maximum value = 5.0V
  }
  
  // Set channels 4-7 to half maximum voltage (2.5V)
  for (int ch = 4; ch < 8; ch++) {
    setChannel(ch, 32768);  // Half of 65535 = 2.5V
  }
  
  // Print status every 5 seconds
  Serial.println("Channel Status:");
  for (int ch = 0; ch < 8; ch++) {
    Serial.print("  Ch");
    Serial.print(ch);
    Serial.print(" (OUT");
    Serial.print((char)('A' + ch));
    Serial.print("): ");
    if (ch < 4) {
      Serial.println("5.000V (MAX)");
    } else {
      Serial.println("2.500V (HALF)");
    }
  }
  Serial.println();
  
  delay(5000);
}

// === DAC Control Functions ===

void dacWriteCommand(uint8_t command, uint8_t address, uint16_t data) {
  // DAC8568 uses 32-bit command structure:
  // Bits 31-28: Command (4 bits)
  // Bits 27-24: Address (4 bits)
  // Bits 23-8:  Data (16 bits)
  // Bits 7-4:   Feature (4 bits, usually 0)
  // Bits 3-0:   Reserved (4 bits, usually 0)
  
  uint32_t message = ((uint32_t)command << 28) | 
                     ((uint32_t)address << 24) | 
                     ((uint32_t)data << 8);
  
  SPI.beginTransaction(dacSPISettings);
  digitalWrite(DAC_CS_PIN, LOW);
  
  // Send 32 bits (4 bytes)
  SPI.transfer((message >> 24) & 0xFF);
  SPI.transfer((message >> 16) & 0xFF);
  SPI.transfer((message >> 8) & 0xFF);
  SPI.transfer(message & 0xFF);
  
  digitalWrite(DAC_CS_PIN, HIGH);
  SPI.endTransaction();
  
  delayMicroseconds(1);  // Small delay for DAC to process
}

void dacReset() {
  // Software reset
  dacWriteCommand(DAC8568_CMD_RESET, 0, 0);
  Serial.println("  DAC reset complete");
}

void dacSetReference(bool internalRef) {
  // Set reference mode
  // Bit 0: 1 = internal reference always on, 0 = internal reference off
  uint16_t refData = internalRef ? 0x0001 : 0x0000;
  dacWriteCommand(DAC8568_CMD_REFERENCE, 0, refData);
  Serial.print("  Reference set to: ");
  Serial.println(internalRef ? "Internal (always on)" : "External");
}

void dacPowerUpAll() {
  // Power up all channels (bits 0-7 = 0 for power up)
  dacWriteCommand(DAC8568_CMD_POWER, 0, 0x0000);
  Serial.println("  All channels powered up");
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
