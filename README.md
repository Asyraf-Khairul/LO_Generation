# Quadrature LO Generator

This repository contains a standalone Raspberry Pi Pico firmware project for testing a quadrature local oscillator outside the main PicoRX receiver codebase.

The firmware uses the Pico's PIO hardware to generate a two-pin quadrature square-wave output on GPIO 0 and GPIO 1. A rotary encoder adjusts the target LO frequency, and a 128x64 SH1106 OLED shows the current frequency and output status.

## Features

- Quadrature LO output on GPIO 0 and GPIO 1
- 90-degree phase relationship using a four-state PIO sequence
- Rotary encoder tuning input
- Fine tuning in 1 kHz steps
- Coarse tuning in 500 kHz steps while button 1 is held
- SH1106 OLED status display over I2C
- USB serial logging of target frequency and PIO clock divider

## Hardware Connections

| Function | Pico GPIO |
| --- | ---: |
| LO output phase A | GPIO 0 |
| LO output phase B | GPIO 1 |
| Encoder A | GPIO 20 |
| Encoder B | GPIO 21 |
| Button 1 / coarse tune | GPIO 17 |
| Button 2 | GPIO 22 |
| OLED SDA | GPIO 18 |
| OLED SCL | GPIO 19 |

The OLED is configured for I2C address `0x3C` on `i2c1`.

Use a 128x64 SH1106 I2C module. The driver uses SH1106 DC-DC control,
page addressing, and a two-column RAM offset for the visible display.
The I2C clock is 400 kHz. If your module uses address `0x3D`, change the
address passed to `sh1106_init` in `main.cpp`.

The controller commands follow the [SH1106 datasheet](https://www.displayfuture.com/Display/datasheet/controller/SH1106.pdf).

## Build with Pico SDK 2.1.0

The target board is the original Raspberry Pi Pico (RP2040). The project is
configured to fetch Pico SDK **2.1.0** when no local SDK path is supplied.
If `PICO_SDK_PATH` is already set, ensure it points to SDK 2.1.0.

Install Git, CMake, Ninja, Python 3, and the Arm GNU `arm-none-eabi` toolchain.
The firmware was built with CMake 3.31.8, Ninja 1.12.1, Arm GNU Toolchain
13.3.Rel1, and the Pico SDK 2.1.0 versions of `pioasm` and `picotool`.
Make the command-line tools available on `PATH`.

From the repository directory, configure and build:

```sh
cmake -S . -B build -G Ninja -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

For a local SDK checkout, initialize its `lib/tinyusb` submodule for USB serial
support and pass `-DPICO_SDK_PATH=<path-to-pico-sdk-2.1.0>` when configuring.
Use `-DPICO_TOOLCHAIN_PATH=<arm-toolchain-root>` if the compiler is not on `PATH`.
For prebuilt Pico tools, pass `-Dpioasm_DIR=<directory-containing-pioasmConfig.cmake>`
and `-Dpicotool_DIR=<directory-containing-picotoolConfig.cmake>` for version 2.1.0.
Otherwise, building the host tools also requires a native C/C++ compiler.

The build produces `build/standalone_quadrature_lo.uf2`, along with `.elf` and
`.bin` files. After the initial configuration, only the build command is needed
to rebuild source changes. Build outputs are ignored by Git.

## Flash the Pico

1. Hold **BOOTSEL** while connecting the Pico to your computer over USB.
2. Release BOOTSEL when the `RPI-RP2` drive appears.
3. Copy `build/standalone_quadrature_lo.uf2` onto that drive.
4. The Pico restarts and runs the firmware. Check that the OLED shows the
   frequency and that turning the encoder updates it.

The SH1106 conversion has passed a Pico SDK 2.1.0 Release build and a host-side
test of initialization commands, page transfers, edge pixels, framebuffer
preservation, and display power state. It has not yet been tested on physical
hardware.

## Frequency Behavior

The PIO program cycles through four output states:

```text
00 -> 01 -> 11 -> 10
```

That sequence produces the quadrature relationship between GPIO 0 and GPIO 1. The requested LO frequency is converted into a PIO clock divider using:

```text
clkdiv = system_clock / (4 * target_frequency)
```

The firmware uses a 125 MHz system clock, initializes the displayed target to
7 MHz, and clamps tuning requests between 0 Hz and 30 MHz. The divider is
currently applied when the encoder changes the target; the initial 7 MHz
display value is not applied to the output at startup. A target of 0 Hz also
does not disable the output.

## Project Layout

| File | Purpose |
| --- | --- |
| `main.cpp` | Firmware entry point, OLED UI, tuning logic, and PIO setup |
| `nco.pio` | PIO program that generates the two-bit quadrature LO pattern |
| `quadrature_encoder.pio` | PIO quadrature decoder for the rotary encoder |
| `pins.h` | Central pin mapping |
| `sh1106.c`, `sh1106.h` | Lightweight SH1106 display driver |
| `font_8x5.h` | Bitmap font used by the OLED display |
| `pico_sdk_import.cmake` | Pico SDK import helper |
| `CMakeLists.txt` | Build configuration |

## Notes

- GPIO 0 and GPIO 1 are digital square-wave outputs, not sine-wave RF outputs.
- The quadrature sequence sets the phase relationship; it does not multiply the output frequency by four.
- Check the output with a scope or logic analyzer before connecting it to downstream RF hardware.
