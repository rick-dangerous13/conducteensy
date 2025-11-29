#!/bin/bash
# build.sh - Build script for O_C Phazerville with ILI9341 Display
#
# Prerequisites:
# 1. Install PlatformIO: pip install platformio
# 2. Run this script from the software/ directory
#
# This will generate the firmware .hex file at:
#   .pio/build/T41_ILI9341/firmware.hex

set -e

echo "=================================================="
echo "O_C Phazerville ILI9341 Display - Build Script"
echo "=================================================="
echo ""

# Check for PlatformIO
if ! command -v pio &> /dev/null; then
    echo "Error: PlatformIO not found!"
    echo "Install it with: pip install platformio"
    exit 1
fi

echo "PlatformIO found: $(pio --version)"
echo ""

# Build for Teensy 4.1 with ILI9341
echo "Building firmware for Teensy 4.1 with ILI9341 display..."
echo ""
pio run -e T41_ILI9341

echo ""
echo "=================================================="
echo "Build Complete!"
echo "=================================================="
echo ""
echo "Firmware files generated:"
echo "  HEX: .pio/build/T41_ILI9341/firmware.hex"
echo "  ELF: .pio/build/T41_ILI9341/firmware.elf"
echo ""
echo "To upload to Teensy 4.1:"
echo "  pio run -e T41_ILI9341 -t upload"
echo ""
echo "Or use Teensy Loader with the .hex file"
echo ""
