# Quadrature LO Generator

This repository contains a standalone Raspberry Pi Pico firmware project for testing a quadrature local oscillator outside the main PicoRX receiver codebase.

The firmware uses the Pico's PIO hardware to generate a two-pin quadrature square-wave output on GPIO 0 and GPIO 1. A rotary encoder adjusts the target LO frequency, and a 128x64 SSD1306 OLED shows the current frequency and output status.

## Features

- Quadrature LO output on GPIO 0 and GPIO 1
- 90-degree phase relationship using a four-state PIO sequence
- Rotary encoder tuning input
- Fine tuning in 1 kHz steps
- Coarse tuning in 500 kHz steps while button 1 is held
- SSD1306 OLED status display over I2C
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

## Frequency Behavior

The PIO program cycles through four output states:

```text
00 -> 01 -> 11 -> 10
```

That sequence produces the quadrature relationship between GPIO 0 and GPIO 1. The requested LO frequency is converted into a PIO clock divider using:

```text
clkdiv = system_clock / (4 * target_frequency)
```

The firmware currently uses a 125 MHz system clock, starts at 7 MHz, and clamps the target frequency between 0 Hz and 30 MHz.

## Project Layout

| File | Purpose |
| --- | --- |
| `main.cpp` | Firmware entry point, OLED UI, tuning logic, and PIO setup |
| `nco.pio` | PIO program that generates the two-bit quadrature LO pattern |
| `quadrature_encoder.pio` | PIO quadrature decoder for the rotary encoder |
| `pins.h` | Central pin mapping |
| `ssd1306.c`, `ssd1306.h` | Lightweight SSD1306 display driver |
| `font_8x5.h` | Bitmap font used by the OLED display |
| `pico_sdk_import.cmake` | Pico SDK import helper |
| `CMakeLists.txt` | Build configuration |

## Notes

- GPIO 0 and GPIO 1 are digital square-wave outputs, not sine-wave RF outputs.
- The quadrature sequence sets the phase relationship; it does not multiply the output frequency by four.
- Check the output with a scope or logic analyzer before connecting it to downstream RF hardware.
