# Standalone quadrature LO test

This folder is a minimal Pico project extracted from the main PicoRX receiver so you can test the quadrature digital LO on GPIO0 and GPIO1 independently.

## What it contains

- `main.cpp` — quadrature LO generation and encoder tuning logic
- `nco.pio` — PIO sequence creating the 4-state quadrature pattern on GPIO0/GPIO1
- `quadrature_encoder.pio` — rotary encoder count input logic
- `ssd1306.c` / `ssd1306.h` — lightweight SSD1306 OLED driver used by this project
- `pins.h` — the pin mapping used for the LO and OLED

## Build

```bash
mkdir build
cd build
cmake -DPICO_BOARD=pico ..
make -j4
```

Then load the UF2 onto the Pico.

## Notes

- GPIO0 and GPIO1 are the quadrature pair.
- The four-state PIO sequence gives a 90-degree phase relationship, not a 4x RF output.
- The encoder changes the tuned frequency in 1 kHz steps in this test harness.
