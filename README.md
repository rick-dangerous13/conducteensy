# O_C Phazerville with ILI9341 Display - Teensy 4.1

This is a custom fork of the [O_C-Phazerville](https://github.com/djphazer/O_C-Phazerville) firmware modified to use an **ILI9341 TFT display** instead of the standard SH1106 OLED display.

## Hardware Requirements

- **Teensy 4.1** microcontroller
- **ILI9341 TFT Display** (320x240, SPI interface)

## Display Pinout

Connect the ILI9341 display to Teensy 4.1 as follows:

| ILI9341 Pin | Teensy 4.1 Pin | Description |
|-------------|----------------|-------------|
| GND | GND | Ground |
| VCC | 3.3V | Power (3.3V) |
| CS | Pin 10 | Chip Select |
| RST | Pin 8 | Reset |
| DC | Pin 9 | Data/Command |
| MOSI | Pin 11 | SPI MOSI |
| SCK | Pin 13 | SPI Clock |
| MISO | Pin 12 | SPI MISO (optional) |

## Building the Firmware and Getting the HEX File

### Prerequisites

1. Install [PlatformIO](https://platformio.org/install):
   ```bash
   pip install platformio
   ```

2. Clone this repository:
   ```bash
   git clone https://github.com/rick-dangerous13/conducteensy.git
   cd conducteensy
   ```

### Quick Build (Using Build Script)

```bash
cd software
./build.sh
```

### Manual Build Commands

Build the firmware:
```bash
cd software
pio run -e T41_ILI9341
```

### Firmware HEX File Location

After building, the firmware files will be located at:

| File | Location |
|------|----------|
| **HEX file** | `.pio/build/T41_ILI9341/firmware.hex` |
| ELF file | `.pio/build/T41_ILI9341/firmware.elf` |

### Upload to Teensy

**Option 1: Using PlatformIO (recommended)**
```bash
cd software
pio run -e T41_ILI9341 -t upload
```

**Option 2: Using Teensy Loader**
1. Download [Teensy Loader](https://www.pjrc.com/teensy/loader.html)
2. Open the `.pio/build/T41_ILI9341/firmware.hex` file
3. Press the button on Teensy to enter programming mode
4. Click "Program" in Teensy Loader

## Display Scaling

The original O_C interface is designed for a 128x64 monochrome OLED display. This firmware scales the 128x64 content by 2x to fit nicely on the 320x240 ILI9341 display:

- Source: 128x64 pixels
- Scaled: 256x128 pixels (2x scale)
- Centered on the 320x240 display

## Project Structure

```
software/
├── platformio.ini          # PlatformIO configuration
├── build.sh               # Build script (generates .hex file)
├── src/
│   ├── Main.cpp           # Main application entry point
│   ├── src.ino            # Arduino IDE compatibility
│   └── src/
│       └── drivers/
│           ├── ILI9341_Driver.h    # ILI9341 driver header
│           ├── ILI9341_Driver.cpp  # ILI9341 driver implementation
│           ├── SH1106_128x64_driver.h  # Display abstraction
│           ├── display.h           # Display interface
│           ├── display.cpp         # Display implementation
│           ├── framebuffer.h       # Frame buffer
│           ├── page_display_driver.h  # Page driver
│           ├── weegfx.h           # Graphics library header
│           └── weegfx.cpp         # Graphics library implementation
```

## Troubleshooting

### Build fails with toolchain errors
Make sure PlatformIO can download the Teensy toolchain. You may need internet access for the first build.

### Display shows nothing
1. Check wiring according to the pinout table
2. Ensure VCC is connected to 3.3V (not 5V)
3. Verify the display is an ILI9341 (not ST7789 or other)

## Credits

- Original O_C firmware by [mxmxmx](https://github.com/mxmxmx)
- Phazerville Suite by [djphazer](https://github.com/djphazer/O_C-Phazerville)
- Teensy 4.1 port by [Paul Stoffregen](https://github.com/PaulStoffregen/O_C_T41)
- ILI9341 display adaptation for this fork

## License

This project inherits the licensing from O_C-Phazerville. See the original repository for details.
