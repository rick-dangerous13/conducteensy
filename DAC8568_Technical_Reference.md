# DAC8568 & BOOST-DAC8568 Comprehensive Technical Reference

## 1. DAC8568 Chip Core Specifications

### Resolution and Channels
- **Resolution:** 16-bit (65,536 discrete output levels)
- **Channels:** 8 independent voltage output channels (octal)
- **Architecture:** String DAC architecture
- **INL (Integral Non-Linearity):** ±4 LSB maximum (16-bit version)
- **Settling Time:** 5µs typical
- **DNL (Differential Non-Linearity):** ±1 LSB maximum

### Internal Reference Voltage
- **Voltage:** 2.5V nominal
- **Temperature Coefficient:** 2 ppm/°C (ultra-stable)
- **Initial Accuracy:** 0.004% (40 ppm)
- **Load Capability:** Can source up to 20mA
- **Enable/Disable:** Software controlled via internal reference control register
- **Default State:** Disabled by default (must be enabled via command)
- **Power-up State:** Internal reference is off at power-up

### External Reference Voltage Capability
- **Pin:** VREFIN/VREFOUT (bidirectional reference pin)
  - When internal reference is ENABLED: Functions as VREFOUT, provides 2.5V to external circuits
  - When internal reference is DISABLED: Functions as VREFIN, accepts external reference (0V to VDD)
- **External Reference Range:** 0V to VDD (up to 5.5V)
- **Reference Input Impedance:** High impedance input when configured for external reference
- **Flexibility:** Can use any voltage between 0V and VDD as reference to set full-scale output range

### Power Supply Requirements
- **VDD (Digital Supply):** 2.7V to 5.5V
- **AVDD (Analog Positive Supply):** 2.7V to 5.5V
- **AVSS (Analog Negative Supply):** 0V (ground) or negative voltage down to -5.5V
- **Typical Configuration:** VDD = AVDD = 5V, AVSS = 0V (single supply operation)
- **Power Consumption:** 2.2mW typical (all channels active)
- **Dual Supply Support:** Yes, AVSS can be negative for bipolar output ranges

### Output Voltage Range
- **Determined by:** Reference voltage and AVSS
- **Formula:** Output range = AVSS to (VREF + AVSS)
- **With 2.5V Internal Reference:**
  - Single Supply (AVSS = 0V): 0V to 2.5V
  - To achieve 0-5V output with internal 2.5V reference: **NOT POSSIBLE**
- **With 5V External Reference:**
  - Single Supply (AVSS = 0V): 0V to 5V (achievable)
- **Maximum Output:** Limited by AVDD rail (rail-to-rail operation)
- **Output Capability:** Each channel can sink or source up to 20mA

### Buffer Amplifier Specifications
- **Type:** On-chip output buffer amplifier
- **Operation:** Rail-to-rail output capability
- **Output Impedance:** Low impedance buffered outputs
- **Drive Capability:** 20mA sink/source per channel
- **Settling:** 5µs to 0.003% of full scale
- **Bandwidth:** Sufficient for 100kHz update rate (0.1 MSPS)
- **Offset Error:** ±5mV typical
- **Gain Error:** ±0.15% typical

---

## 2. Critical Pin Functions

### /LDAC Pin (Load DAC - Active Low)
**Purpose:** Controls transfer of data from input registers to DAC registers

**Functionality:**
- **Active Low Signal:** Pulsing LOW triggers transfer from input register → DAC register
- **When Tied LOW Permanently:** Enables "write-through mode"
  - Data written to input register immediately transfers to DAC register
  - Output updates immediately upon SPI write completion
  - No need for separate LDAC pulse
  - Simplifies single-channel updates
- **When Controlled (Toggled):** Enables "buffered mode"
  - Write data to multiple channel input registers
  - Assert /LDAC LOW to simultaneously update all DAC registers
  - Enables glitch-free simultaneous multi-channel updates
  - Critical for synchronized output changes

**Best Practices:**
- Tie LOW if only updating one channel at a time
- Control via GPIO if synchronous multi-channel updates needed
- Can update multiple channels then pulse LDAC once for simultaneous output

### /CLR Pin (Clear - Active Low)
**Purpose:** Asynchronous clear/reset of all DAC outputs to a predetermined voltage

**Functionality:**
- **Active Low Signal:** When pulled LOW, all DACs reset to clear code value
- **Clear Code:** Programmable via software (power-on reset register)
  - Can be set to zero-scale (0x0000), mid-scale (0x8000), or full-scale (0xFFFF)
  - Default varies by configuration
- **Power-up Behavior:** 
  - At power-up, DACs reset to the clear code value stored in internal register
  - Default clear code is typically mid-scale (0x8000 = ~1.25V with 2.5V reference)
- **Asynchronous:** Does not require clock signal; acts immediately
- **Recovery:** When released HIGH, DACs remain at clear code until new values written

**Typical Connection:**
- Often tied HIGH (inactive) if clear function not needed
- Can connect to GPIO for software-controlled reset
- Pull-up resistor recommended to prevent accidental clear during power-up

### /SYNC Pin (Synchronization - SPI Chip Select)
**Purpose:** SPI chip select (active low)

**Functionality:**
- **Standard SPI CS Function:** 
  - Pull LOW to begin SPI transaction
  - Keep LOW during entire 24-bit or 32-bit transfer
  - Pull HIGH to latch data and execute command
- **Frame Synchronization:** Rising edge of /SYNC latches the serial data and executes command
- **Multi-Device:** Allows multiple DAC8568 devices on same SPI bus with different CS lines

### SCLK (Serial Clock)
**Purpose:** SPI clock input

**Specifications:**
- **Maximum Frequency:** 50 MHz
- **Clock Mode:** SPI Mode 1 (CPOL=0, CPHA=1) or Mode 2 (CPOL=1, CPHA=0)
- **Schmitt-Triggered:** Input has Schmitt trigger for noise immunity
- **Edge:** Data latched on rising edge, DAC updates occur after complete frame

### DIN (Data Input)
**Purpose:** SPI data input (MOSI)

**Specifications:**
- **Data Width:** 24-bit or 32-bit frames depending on command
- **MSB First:** Data transmitted most significant bit first
- **Schmitt-Triggered:** Input has Schmitt trigger for noise immunity
- **Setup/Hold Time:** ~10ns setup, ~10ns hold relative to SCLK

### ENABLE Pin
**Purpose:** Power-down control pin

**Functionality:**
- **Active HIGH:** Normal operation (ENABLE = HIGH = powered on)
- **LOW:** Places device in power-down mode
  - Reduces power consumption to microamperes
  - Outputs go to high-impedance state or configured power-down resistance
  - SPI interface remains active
- **Power-Down Modes:** Can configure per-channel or global power-down via software
  - 1kΩ to GND
  - 100kΩ to GND  
  - High-impedance (Hi-Z)

**Typical Connection:** Tied HIGH for always-on operation, or controlled via GPIO

---

## 3. BOOST-DAC8568 Board Implementation

### Reference Voltage Configuration
**Internal Reference Mode (Default Hardware Configuration):**
- VREFIN/VREFOUT pin is **not** connected to external reference by default
- Internal 2.5V reference **must be enabled via software command**
- Once enabled, VREFOUT pin sources 2.5V (can provide up to 20mA)
- Output range: 0V to 2.5V

**External Reference Mode:**
- To use external reference (e.g., 5V for 0-5V output range):
  - Apply external voltage to VREFIN/VREFOUT pin
  - Disable internal reference via software command
  - External reference determines full-scale output
- Board may have jumper or solderable connection for external reference input

### /LDAC and /CLR Pin Connections
Based on typical BoosterPack implementations:
- **/LDAC:** Likely **tied to GND (LOW)** on the breakout board
  - Enables automatic write-through mode
  - Simplifies usage for single-channel updates
  - If GPIO control needed, may have jumper or header pin access
- **/CLR:** Likely **tied to VDD (HIGH)** via pull-up resistor
  - Keeps device active, prevents accidental clearing
  - May have jumper or header pin for manual/GPIO control
- **Check schematic (SBAR001A.ZIP) to confirm actual connections**

### Power Supply Configuration
- **VDD/AVDD:** Connected to LaunchPad 3.3V or 5V supply rail
  - Most likely 3.3V for MSP430F5529 LaunchPad compatibility
  - Check BoosterPack headers for actual voltage
- **AVSS:** Connected to ground (0V) - single supply operation
- **Decoupling:** On-board bypass capacitors (typically 0.1µF + 10µF) near supply pins
- **Digital/Analog Separation:** Separate planes or careful routing for noise reduction

### Board Components
**Key Components on BOOST-DAC8568:**
1. **DAC8568 IC** - Main 16-bit 8-channel DAC (TSSOP-16 package)
2. **Bypass Capacitors** - 0.1µF ceramic + 10µF tantalum for supply filtering
3. **SPI Pull-ups/Pull-downs** - Resistors for /SYNC, /LDAC, /CLR pins
4. **Reference Capacitor** - 10µF on VREFIN/VREFOUT for reference stability
5. **Output Headers** - Pin headers for 8 DAC outputs (VOUTA through VOUTH)
6. **BoosterPack Headers** - 40-pin headers for MSP430 LaunchPad connection
7. **Reference Selection** - Possible jumpers for internal/external reference
8. **Test Points** - For debugging and measurement access

### Pin Header Layout
**BoosterPack Standard 40-Pin Headers:**
- **SPI Connection to MSP430:**
  - SCLK → LaunchPad P3.2 (UCB0CLK)
  - DIN (MOSI) → LaunchPad P3.0 (UCB0SIMO)
  - /SYNC (CS) → LaunchPad GPIO (software controlled, typically P2.7 or similar)
- **Output Channels:** 8 separate header pins or terminal blocks
  - VOUTA, VOUTB, VOUTC, VOUTD, VOUTE, VOUTF, VOUTG, VOUTH
- **Power:** VDD, AVDD, AVSS, GND connections from LaunchPad
- **Optional Control:** ENABLE, /LDAC, /CLR may be broken out to headers if not tied on-board

**Typical Pinout (verify with schematic SBAR001A):**
```
J1 Header (Left):
Pin 1:  3.3V/5V
Pin 2:  GND
Pin 3-10: Various GPIO
...

J2 Header (Right):
Pin 1:  3.3V/5V
Pin 2:  GND
Pin 3-10: Various GPIO
...

Output Headers:
- 8 pins for DAC outputs (VOUTA-VOUTH)
- GND reference pins
```

---

## 4. Reference Voltage Details

### Internal 2.5V Reference
**Enabling via Software:**
1. Send command to internal reference setup register
2. Command format: 24-bit or 32-bit frame
   - Prefix: Control register write command
   - Address: Internal reference control register
   - Data: Enable bit set to 1
3. Typical command sequence (24-bit):
   ```
   Command: 0x380001  (Write to internal reference register, enable)
   - Bits [23:20]: 0011 = Write to control register
   - Bits [19:16]: 1000 = Internal reference register
   - Bits [15:0]:  0x0001 = Enable internal reference
   ```
4. After enable command, wait ~10ms for reference to settle
5. VREFOUT pin will now output 2.5V ±0.004%

**Current Sourcing:**
- Can source up to 20mA from VREFOUT
- Can power external circuitry or multiple DACs
- Requires 10µF bypass capacitor on VREFOUT pin

### External Reference Usage
**Configuration Steps:**
1. **Disable internal reference** via software command:
   ```
   Command: 0x380000  (Write to internal reference register, disable)
   ```
2. **Apply external reference voltage** to VREFIN/VREFOUT pin
   - Voltage range: 0V to VDD (up to 5.5V)
   - Must be clean, low-noise reference (e.g., REF5050, LM4040)
3. **Bypass external reference** with 10µF capacitor close to pin
4. External reference determines new full-scale output range

**Example - Using 5V External Reference:**
- Disable internal 2.5V reference: Send 0x380000
- Apply 5V from precision reference chip to VREFIN/VREFOUT
- DAC output range becomes: 0V to 5V (with AVSS = 0V)
- Full resolution: 5V / 65536 = 76.3µV per LSB

### Output Range vs Reference Voltage
**Mathematical Relationship:**
```
Output Voltage = (DAC_CODE / 65536) × VREF + AVSS

Where:
- DAC_CODE: 0 to 65535 (0x0000 to 0xFFFF)
- VREF: Reference voltage (internal 2.5V or external)
- AVSS: Analog negative supply (typically 0V)
```

**Examples:**
1. **Internal 2.5V reference, AVSS = 0V:**
   - DAC_CODE = 0x0000: Output = 0.000V
   - DAC_CODE = 0x8000: Output = 1.250V (mid-scale)
   - DAC_CODE = 0xFFFF: Output = 2.500V (full-scale)

2. **External 5V reference, AVSS = 0V:**
   - DAC_CODE = 0x0000: Output = 0.000V
   - DAC_CODE = 0x8000: Output = 2.500V (mid-scale)
   - DAC_CODE = 0xFFFF: Output = 5.000V (full-scale)

3. **External 3.3V reference, AVSS = 0V:**
   - DAC_CODE = 0x0000: Output = 0.000V
   - DAC_CODE = 0x8000: Output = 1.650V (mid-scale)
   - DAC_CODE = 0xFFFF: Output = 3.300V (full-scale)

### Can You Get 0-5V Output with 2.5V Internal Reference?
**Answer: NO - Not possible with internal reference alone**

**Explanation:**
- Internal reference is fixed at 2.5V
- Maximum output = VREF = 2.5V (assuming AVSS = 0V)
- Cannot exceed reference voltage in standard configuration
- Output buffer can swing rail-to-rail (up to AVDD), but DAC core limited by VREF

**Solutions to Achieve 0-5V Output:**
1. **Use 5V External Reference** (Recommended)
   - Disable internal 2.5V reference
   - Apply 5V precision reference to VREFIN/VREFOUT pin
   - Achieves true 0-5V output range
   
2. **External Amplification** (Not recommended)
   - Use 2.5V reference with 2× gain amplifier after DAC output
   - Adds complexity, noise, offset errors
   - Requires op-amp, additional power supply, calibration

3. **Dual Supply with Negative AVSS** (Special cases)
   - Set AVSS = -2.5V, use 2.5V reference
   - Output range: -2.5V to 0V or 0V to 2.5V depending on DAC code mapping
   - Not typical for unipolar 0-5V application

**Recommendation:** For true 0-5V output, use external 5V reference (e.g., REF5050, MAX6350, or similar precision reference IC)

---

## 5. DAC Update Mechanisms

### Write-Through Mode vs Buffered Mode
**Write-Through Mode:**
- **Configuration:** /LDAC pin tied permanently LOW (to GND)
- **Behavior:**
  - SPI write to DAC channel immediately updates output
  - Input register and DAC register update simultaneously
  - No separate LDAC pulse required
- **Use Case:** Single channel updates, simple sequential control
- **Advantage:** Simplicity, immediate response
- **Disadvantage:** Cannot synchronize multiple channel updates, potential glitches during multi-channel writes

**Buffered Mode:**
- **Configuration:** /LDAC pin controlled by GPIO (pulsed LOW)
- **Behavior:**
  - SPI write updates input register only
  - DAC register (and output) remains unchanged until /LDAC pulsed
  - Multiple input registers can be loaded
  - Single /LDAC pulse updates all loaded DAC registers simultaneously
- **Use Case:** Synchronized multi-channel updates, glitch-free waveform generation
- **Advantage:** Simultaneous outputs, eliminates inter-channel timing skew
- **Disadvantage:** Requires extra GPIO pin, slightly more complex software

### Role of Input Registers vs DAC Registers
**Two-Tier Register Architecture:**

**Input Registers (8 channels):**
- **Function:** Staging/buffer for incoming SPI data
- **Access:** Written via SPI command (write to input register command)
- **Effect:** No immediate change to DAC output
- **Purpose:** Allows pre-loading values before simultaneous update

**DAC Registers (8 channels):**
- **Function:** Active registers controlling DAC output voltage
- **Access:** Updated from input registers when /LDAC asserted LOW
- **Effect:** Immediate change to analog output voltage
- **Purpose:** Holds the current output code while new values are staged

**Data Flow:**
```
SPI Write → Input Register → [LDAC pulse] → DAC Register → Analog Output
                              ^
                              |
                    Controlled by /LDAC pin
```

### How LDAC Pin Controls Simultaneous Updates
**Single-Channel Update Example:**
1. /SYNC LOW (begin SPI frame)
2. Send command: Write to Input Register + Update DAC Register
   - Command prefix: 0x0 (write and update)
   - Channel address: 0x0-0x7 (A-H)
   - DAC code: 16-bit value
3. /SYNC HIGH (latch data, execute command)
4. If /LDAC = LOW (tied), output updates immediately
5. If /LDAC = HIGH, output waits for /LDAC pulse

**Multi-Channel Simultaneous Update Example:**
1. Keep /LDAC HIGH (inactive)
2. Write channel A input register (SPI frame 1)
3. Write channel B input register (SPI frame 2)
4. Write channel C input register (SPI frame 3)
5. ... (continue for all channels to update)
6. Pulse /LDAC LOW then HIGH
7. **All DAC registers update simultaneously** (within <100ns of each other)
8. All outputs change at the same time - glitch-free synchronized update

**LDAC Timing:**
- Setup time: ~5ns
- Hold time: ~5ns
- Pulse width minimum: ~20ns (can be very short)
- Output settling: ~5µs after LDAC pulse

### Command Structure for Updates
**24-Bit SPI Frame Format:**
```
Bit 23-20: Command/Prefix (4 bits)
Bit 19-16: Address (4 bits) - Channel select or register select
Bit 15-0:  Data (16 bits) - DAC code or control data
```

**Common Commands:**

**1. Write to Input Register Only (No DAC update):**
```
Prefix: 0x0
Address: 0x0-0x7 (channels A-H)
Data: 16-bit DAC code
Example: 0x018000 - Load mid-scale (0x8000) to channel A input register
```

**2. Update DAC Register from Input Register (no new write):**
```
Prefix: 0x1
Address: 0x0-0x7 (channels A-H)
Data: Don't care
Example: 0x100000 - Update channel A DAC register from its input register
```

**3. Write to Input Register AND Update DAC Register (Write-Through):**
```
Prefix: 0x3
Address: 0x0-0x7 (channels A-H)
Data: 16-bit DAC code
Example: 0x318000 - Write 0x8000 to channel A input register AND update DAC (immediate)
```

**4. Update All DAC Registers:**
```
Prefix: 0x2
Address: 0xF (all channels)
Data: Don't care
Example: 0x2F0000 - Update all DAC registers from their input registers (software LDAC)
```

**5. Write to Internal Reference Register:**
```
Prefix: 0x3 or 0x8 (control register write)
Address: 0x8 (internal reference control register)
Data: 0x0001 (enable) or 0x0000 (disable)
Example: 0x380001 - Enable internal 2.5V reference
Example: 0x380000 - Disable internal reference (for external ref)
```

**6. Write to Power-Down Register:**
```
Prefix: 0x4
Address: 0x0-0x7 (channels A-H) or 0xF (all)
Data: Power-down mode (0x0=normal, 0x1=1kΩ, 0x2=100kΩ, 0x3=Hi-Z)
Example: 0x400001 - Power down channel A with 1kΩ to GND
```

**7. Write to Clear Code Register:**
```
Prefix: 0x5
Address: 0x0
Data: Clear code (0x0000=zero-scale, 0x8000=mid-scale, 0xFFFF=full-scale)
Example: 0x508000 - Set clear code to mid-scale (1.25V with 2.5V ref)
```

---

## Additional Technical Notes

### Timing Specifications
- **SPI Clock:** Up to 50 MHz
- **Setup Time:** ~10ns (DIN to SCLK)
- **Hold Time:** ~10ns (DIN from SCLK)
- **Output Settling:** 5µs to 0.003% FS
- **Power-On Reset Time:** ~10ms
- **Reference Settling:** ~10ms after enable

### Noise and Performance
- **Output Noise:** ~10nV/√Hz typical
- **Glitch Energy:** 0.5nV·s typical (ultra-low glitch)
- **Crosstalk:** -120dB between channels
- **THD:** -80dB typical

### Temperature Performance
- **Operating Range:** -40°C to +125°C
- **Reference TC:** 2 ppm/°C (internal reference)
- **Gain Drift:** ±1 ppm/°C
- **Offset Drift:** ±0.05µV/°C

### Application Considerations
1. **Bypass Capacitors:** Always place 0.1µF + 10µF near VDD, AVDD pins
2. **Reference Capacitor:** 10µF on VREFIN/VREFOUT for stability
3. **Ground Plane:** Use solid ground plane, separate digital/analog if possible
4. **Output Loading:** Each output can drive 5kΩ || 1000pF load
5. **Multiple DACs:** Can cascade /LDAC for synchronized update across multiple chips
6. **Daisy Chain:** No daisy-chain mode; each DAC requires separate /SYNC

---

## Quick Reference Tables

### Pin Summary Table
| Pin | Name | Function | Typical Connection |
|-----|------|----------|-------------------|
| 1 | DIN | SPI Data Input | MCU MOSI |
| 2 | SCLK | SPI Clock | MCU SCK |
| 3 | /SYNC | SPI Chip Select | MCU GPIO (CS) |
| 4 | /LDAC | Load DAC Control | GND (write-through) or GPIO |
| 5 | /CLR | Asynchronous Clear | VDD (via pull-up) |
| 6 | ENABLE | Power Enable | VDD (always on) |
| 7 | VREFIN/VREFOUT | Reference I/O | 10µF to GND |
| 8 | AVSS | Analog Ground | GND |
| 9-16 | VOUTA-VOUTH | DAC Outputs | Output loads |
| - | AVDD | Analog Supply | 3.3V or 5V |
| - | VDD | Digital Supply | 3.3V or 5V |
| - | GND | Digital Ground | GND |

### Command Prefix Quick Reference
| Prefix | Function |
|--------|----------|
| 0x0 | Write to input register (no DAC update) |
| 0x1 | Update DAC register from input register |
| 0x2 | Update all DAC registers |
| 0x3 | Write to input register AND update DAC |
| 0x4 | Power-down mode |
| 0x5 | Set clear code |
| 0x8 | Write to control register (e.g., internal ref) |

### Output Range Examples
| Reference | AVSS | Output Range | Resolution |
|-----------|------|--------------|------------|
| 2.5V (internal) | 0V | 0V to 2.5V | 38.1µV/LSB |
| 5.0V (external) | 0V | 0V to 5.0V | 76.3µV/LSB |
| 3.3V (external) | 0V | 0V to 3.3V | 50.4µV/LSB |
| 2.5V (internal) | -2.5V | -2.5V to 0V | 38.1µV/LSB |

---

## Documentation References
- **Datasheet:** SBAS424 (DAC7568/DAC8168/DAC8568 Datasheet)
- **User Guide:** SBAU275 (BOOST-DAC8568 Getting Started Guide)
- **Schematic:** SBAR001A.ZIP (BOOST-DAC8568 PCB files)
- **Firmware:** SBAC151 (BOOST-DAC8568 Example Code)
- **TI Product Page:** https://www.ti.com/product/DAC8568
- **BoosterPack Page:** https://www.ti.com/tool/BOOST-DAC8568

---

## Common Questions Answered

**Q: Can I get 0-5V output with the internal 2.5V reference?**
A: No. Maximum output equals the reference voltage. Use a 5V external reference for 0-5V output.

**Q: Do I need to pulse /LDAC for every update?**
A: No, if /LDAC is tied LOW permanently (write-through mode). Yes, if you want synchronized multi-channel updates (buffered mode).

**Q: What happens at power-up?**
A: All DAC outputs go to the clear code value (default mid-scale = ~1.25V with 2.5V reference). Internal reference is disabled by default.

**Q: Can I use the DAC with 3.3V logic?**
A: Yes, the DAC works with 2.7V to 5.5V supplies, compatible with both 3.3V and 5V logic.

**Q: How do I enable the internal 2.5V reference?**
A: Send SPI command 0x380001 to the internal reference control register. Wait ~10ms for settling.

**Q: Can multiple DACs share the same /LDAC pin?**
A: Yes, this is a common technique for synchronizing outputs across multiple DAC8568 chips.

**Q: What's the difference between input registers and DAC registers?**
A: Input registers stage new values via SPI. DAC registers control the actual output. /LDAC transfers data from input → DAC registers.

---

*Document compiled from TI DAC8568 datasheet, BOOST-DAC8568 documentation, and application notes. For complete specifications, always refer to the official TI documentation.*
