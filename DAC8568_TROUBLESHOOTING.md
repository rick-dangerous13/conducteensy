# DAC8568 Implementation Troubleshooting Guide

## Critical Issues Found and Fixed

### 1. **WRONG COMMAND FORMAT** (CRITICAL BUG)
**Problem:** Original code used 32-bit SPI frames, but DAC8568 uses **24-bit frames**.

**Original (WRONG):**
```cpp
uint32_t message = ((uint32_t)command << 28) | 
                   ((uint32_t)address << 24) | 
                   ((uint32_t)data << 8);
// Sends 32 bits (4 bytes)
```

**Corrected (RIGHT):**
```cpp
uint32_t message = ((uint32_t)command << 20) | 
                   ((uint32_t)address << 16) | 
                   ((uint32_t)data);
// Sends 24 bits (3 bytes)
```

**Frame Structure:**
```
Bit 23-20: Command (4 bits)
Bit 19-16: Address (4 bits)  
Bit 15-0:  Data (16 bits)
```

**Impact:** This was preventing ALL commands from working correctly, including reference enable and DAC writes.

---

### 2. **WRONG SPI MODE**
**Problem:** Used SPI_MODE1, but DAC8568 typically works better with MODE2 or MODE0.

**SPI Mode Comparison:**
- **MODE0** (CPOL=0, CPHA=0): Clock idle low, sample on leading edge
- **MODE1** (CPOL=0, CPHA=1): Clock idle low, sample on trailing edge
- **MODE2** (CPOL=1, CPHA=0): Clock idle high, sample on leading edge ← **RECOMMENDED**
- **MODE3** (CPOL=1, CPHA=1): Clock idle high, sample on trailing edge

**Fix:** Changed to MODE2 in the corrected code. If issues persist, try MODE0.

```cpp
// Try this first
SPISettings dacSPISettings(10000000, MSBFIRST, SPI_MODE2);

// If that doesn't work, try MODE0
SPISettings dacSPISettings(10000000, MSBFIRST, SPI_MODE0);
```

---

### 3. **INSUFFICIENT TIMING DELAYS**
**Problem:** DAC needs time to process commands, especially after:
- Reset (10ms minimum)
- Reference enable (10-15ms for stabilization)
- Power-up (5ms)

**Fixed in updated code with proper delays.**

---

### 4. **INTERNAL REFERENCE = 2.5V MAX OUTPUT**
**Critical Understanding:**
- Internal reference: 2.5V → **Maximum output = 2.5V**
- For 0-5V output: **Must use external 5V reference**

**You CANNOT get 5V output with internal 2.5V reference!**

---

## Common DAC8568 Problems and Solutions

### Problem: No Voltage Output (0V on all channels)

**Possible Causes:**
1. ✅ **24-bit command format issue** (FIXED ABOVE)
2. ✅ **Internal reference not enabled** (FIXED ABOVE)
3. ❌ **SPI connections wrong:**
   - Check MOSI → Pin 11
   - Check SCK → Pin 13
   - Check CS (/SYNC) → Pin 16
4. ❌ **/LDAC not tied to GND:**
   - If /LDAC is floating or HIGH, DAC registers won't update
   - **Solution:** Tie /LDAC to GND for write-through mode
5. ❌ **VDD/AVDD not powered:**
   - Check 5V supply to VDD and AVDD pins
   - Should measure 5V (not 3.3V) for full range
6. ❌ **Wrong SPI mode:**
   - Try MODE2, MODE1, or MODE0

---

### Problem: Output Stuck at ~1.25V (Mid-Scale)

**Possible Causes:**
1. ❌ **Power-on default clear code is mid-scale:**
   - DAC defaults to mid-scale (0x8000) at power-up
   - This means 1.25V with 2.5V reference
2. ✅ **Commands not updating DAC registers** (FIXED - was 32-bit vs 24-bit issue)
3. ❌ **/LDAC pin not connected to GND:**
   - Solution: Hardware tie /LDAC to GND

---

### Problem: Maximum Voltage is 2.5V (Can't Reach 5V)

**Cause:** Using internal 2.5V reference.

**Solution:** Use external 5V reference:

1. **Disable internal reference:**
   ```cpp
   dacSetReference(false);  // Disable internal 2.5V reference
   ```

2. **Apply external 5V reference to VREFIN/VREFOUT pin:**
   - Use precision reference IC (REF5050, MAX6350, etc.)
   - Connect 5V reference output to DAC VREFIN/VREFOUT pin
   - Add 10µF capacitor on reference pin to GND

3. **Hardware modification may be required:**
   - BOOST-DAC8568 board may not have external reference input by default
   - Check schematic (SBAR001A.ZIP) for jumper/solder pad options

---

### Problem: Voltage Slightly Wrong (e.g., 1.15V instead of 1.25V)

**Possible Causes:**
1. ❌ **Reference voltage not exactly 2.5V:**
   - Measure VREFOUT pin voltage
   - Should be 2.500V ±0.1%
   - If significantly off, reference may not be enabled
2. ❌ **VDD/AVDD voltage incorrect:**
   - Should be 5.0V for proper operation
   - If 3.3V, output may be clamped/inaccurate
3. ❌ **Gain/offset error:**
   - Normal for ±0.15% gain error
   - ±5mV offset error typical

---

### Problem: Some Channels Work, Others Don't

**Possible Causes:**
1. ❌ **Channel address wrong in command:**
   - Addresses: 0x0 (A), 0x1 (B), ... 0x7 (H)
2. ❌ **Power-down mode per channel:**
   - Check if channels were individually powered down
   - Solution: Send power-up command to all channels
3. ❌ **Hardware issue on specific channels:**
   - Check for shorts/opens on output pins

---

## Diagnostic Checklist

### 1. Verify SPI Communication
```cpp
// Add debug output to dacWriteCommand:
Serial.print("Sending: CMD=0x");
Serial.print(command, HEX);
Serial.print(" ADDR=0x");
Serial.print(address, HEX);
Serial.print(" DATA=0x");
Serial.println(data, HEX);
```

### 2. Check Hardware Connections
- [ ] VDD = 5V (measure with multimeter)
- [ ] AVDD = 5V
- [ ] GND connected
- [ ] MOSI (Pin 11) → DAC DIN
- [ ] SCK (Pin 13) → DAC SCLK
- [ ] CS (Pin 16) → DAC /SYNC
- [ ] /LDAC → GND (tied permanently)
- [ ] /CLR → 5V (via pull-up, or tied to VDD)

### 3. Verify Reference Voltage
- [ ] Measure voltage on VREFIN/VREFOUT pin
- [ ] Should read 2.5V if internal reference enabled
- [ ] Should read external reference voltage if using external ref

### 4. Test Sequence
1. **Reset DAC:** Send clear/reset command
2. **Enable reference:** Wait 15ms
3. **Power up channels:** Send power-up command
4. **Set mid-scale:** Write 0x8000 to channel 0
5. **Measure output:** Should be ~1.25V with 2.5V reference

---

## Working Code Examples from Research

### Successful Arduino Forum Implementation
From the Arduino forum thread about "DAC8568 Question (2.5v output vs 5v)":

**Key findings:**
- User had same problem: max 2.5V output
- **Solution:** Had to use external 5V reference
- Cannot achieve 5V output with internal 2.5V reference
- Board modification may be required to connect external reference

### TI Forum Discussions
Common issues found:
1. **SPI mode confusion:** MODE0 vs MODE1 vs MODE2 all work, but must match
2. **24-bit vs 32-bit frames:** Must use 24-bit
3. **Reference enable timing:** Need 10ms+ delay after enable
4. **/LDAC timing:** If not tied to GND, must pulse properly

---

## Reference Command Variations

Different implementations use different commands for reference control:

**Option 1: Command 0x07**
```cpp
dacWriteCommand(0x07, 0x00, 0x0001);  // Enable internal ref
```

**Option 2: Command 0x08**
```cpp
dacWriteCommand(0x08, 0x00, 0x0001);  // Enable internal ref
```

**Option 3: Control register write**
```cpp
// Some boards use control register approach
dacWriteCommand(0x09, 0x00, 0x0001);  
```

**Recommendation:** Try 0x07 first (most common), then 0x08 if that doesn't work.

---

## SPI Timing Requirements

From datasheet:
- **Clock frequency:** Up to 50 MHz (we use 10 MHz for safety)
- **CS setup time:** 5ns minimum
- **CS hold time:** 5ns minimum  
- **Data setup time:** 10ns minimum
- **Data hold time:** 10ns minimum
- **Output settling:** 5µs typical

Our corrected code includes small delays to meet these requirements.

---

## Voltage Formula

```
Output Voltage = (DAC_CODE / 65536) × VREF + AVSS

Where:
- DAC_CODE: 0 to 65535 (0x0000 to 0xFFFF)
- VREF: Reference voltage (2.5V internal or external)
- AVSS: Analog negative supply (typically 0V)
```

**Examples:**
- Code 0x0000 → 0.000V
- Code 0x8000 (32768) → 1.250V with 2.5V ref, or 2.500V with 5V ref
- Code 0xFFFF (65535) → 2.500V with 2.5V ref, or 5.000V with 5V ref

---

## Testing with Updated Code

1. **Upload the fixed code** to Teensy 4.1
2. **Open Serial Monitor** at 115200 baud
3. **Observe initialization sequence:**
   ```
   Step 1: Reset DAC
   Step 2: Enable Internal Reference
   Step 3: Power Up All Channels
   ```
4. **Measure VREFOUT pin:** Should read ~2.5V
5. **Measure channel A output:** Should read ~1.25V (mid-scale test)
6. **All 8 channels** should output 1.25V

**If still not working:**
- Try changing SPI mode (MODE2 → MODE1 → MODE0)
- Check VDD is 5V (not 3.3V)
- Verify /LDAC is tied to GND
- Use oscilloscope to verify SPI signals

---

## Next Steps for 0-5V Output

To achieve full 0-5V range:

1. **Get a precision 5V reference IC:**
   - REF5050 (Texas Instruments)
   - MAX6350 (Maxim)
   - LM4040-5.0 (budget option)

2. **Hardware modification:**
   - Locate VREFIN/VREFOUT pin on BOOST-DAC8568
   - May need to cut trace or remove jumper
   - Connect external 5V reference output to this pin
   - Add 10µF capacitor from ref pin to GND

3. **Software change:**
   ```cpp
   dacSetReference(false);  // Disable internal reference
   // External 5V reference now controls output range
   ```

4. **Test:**
   - Write 0xFFFF → should output 5.0V
   - Write 0x8000 → should output 2.5V
   - Write 0x0000 → should output 0.0V

---

## Summary of Fixes

| Issue | Status | Impact |
|-------|--------|--------|
| 24-bit vs 32-bit command format | ✅ FIXED | CRITICAL - prevented all commands from working |
| SPI Mode (MODE1 vs MODE2) | ✅ FIXED | Potential issue with some boards |
| Insufficient timing delays | ✅ FIXED | Could cause reference/power-up failures |
| Reference command variations | ⚠️ ADDRESSED | Code tries command 0x07 (most common) |
| 2.5V max output limitation | ⚠️ DOCUMENTED | Hardware limitation, needs external ref for 5V |
| /LDAC pin connection | ⚠️ CHECK HARDWARE | Must be tied to GND for write-through mode |
| VDD voltage (5V vs 3.3V) | ⚠️ CHECK HARDWARE | Must be 5V for proper operation |

---

## Additional Resources

- **Datasheet:** SBAS424 (DAC7568/DAC8168/DAC8568)
- **User Guide:** SBAU275 (BOOST-DAC8568)
- **Schematic:** SBAR001A.ZIP
- **Arduino Forum:** Search "DAC8568" for user experiences
- **TI E2E Forum:** Search for DAC8568 troubleshooting threads
