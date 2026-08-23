#include <stdio.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"

#include "nco.pio.h"
#include "quadrature_encoder.pio.h"

#include "pins.h"
#include "ssd1306.h"
#include "font_8x5.h"

namespace {
PIO NCO_PIO = pio0;
constexpr uint NCO_SM = 0;
PIO ENCODER_PIO = pio1;
constexpr uint ENCODER_SM = 0;
constexpr uint32_t MIN_FREQ_HZ = 0;
constexpr uint32_t MAX_FREQ_HZ = 30000000;
constexpr int32_t ENCODER_COUNTS_PER_DETENT = 4;
constexpr int32_t BASE_STEP_HZ = 1000;
constexpr int32_t COARSE_STEP_HZ = 500000;

float compute_clkdiv_for_frequency(uint32_t system_clock_hz, uint32_t target_frequency_hz) {
    if (target_frequency_hz == 0) {
        return 1.0f;
    }

    double ideal_divider = (double)system_clock_hz / (4.0 * (double)target_frequency_hz);
    double rounded_divider = round(ideal_divider * 256.0) / 256.0;
    if (rounded_divider < 1.0) {
        rounded_divider = 1.0;
    }
    return (float)rounded_divider;
}

void init_oled(ssd1306_t &disp) {
    gpio_set_function(PIN_DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_DISPLAY_SDA);
    gpio_pull_up(PIN_DISPLAY_SCL);

    i2c_init(i2c1, 400000);

    disp.external_vcc = false;
    if (!ssd1306_init(&disp, 128, 64, 0x3C, i2c1)) {
        panic("OLED init failed");
    }
    ssd1306_clear(&disp, 0);
    ssd1306_show(&disp);
}

void draw_oled_status(ssd1306_t &disp, uint32_t target_frequency_hz) {
    char line[32];
    ssd1306_clear(&disp, 0);

    // The 5x8 font advances 6 pixels per character. At scale 2 the old
    // "LO 7.250 MHz" title was 144 pixels wide, so it was clipped on the
    // 128-pixel display. Keep the large line to at most 10 characters.
    const uint32_t mhz = target_frequency_hz / 1000000;
    const uint32_t khz = (target_frequency_hz % 1000000) / 1000;
    snprintf(line, sizeof(line), "%lu.%03lu MHz",
             (unsigned long)mhz, (unsigned long)khz);
    int32_t x = (128 - (int32_t)strlen(line) * 12) / 2;
    ssd1306_draw_string(&disp, x, 0, 2, line, 1);

    snprintf(line, sizeof(line), "LO GEN: GPIO 0 & 1");
    x = (128 - (int32_t)strlen(line) * 6) / 2;
    ssd1306_draw_string(&disp, x, 22, 1, line, 1);

    snprintf(line, sizeof(line), "PHASE: 0 & 90 DEG");
    x = (128 - (int32_t)strlen(line) * 6) / 2;
    ssd1306_draw_string(&disp, x, 36, 1, line, 1);

    snprintf(line, sizeof(line), "TUNE STEP: 1 kHz");
    x = (128 - (int32_t)strlen(line) * 6) / 2;
    ssd1306_draw_string(&disp, x, 50, 1, line, 1);

    ssd1306_show(&disp);
}

}  // namespace

int main() {
    stdio_init_all();
    set_sys_clock_khz(125000, true);

    ssd1306_t display = {};
    init_oled(display);

    gpio_set_function(PIN_AB, GPIO_FUNC_PIO1);
    gpio_set_function(PIN_AB + 1, GPIO_FUNC_PIO1);
    gpio_init(PIN_BTN_1);
    gpio_set_dir(PIN_BTN_1, GPIO_IN);
    gpio_pull_up(PIN_BTN_1);
    gpio_init(PIN_BTN_2);
    gpio_set_dir(PIN_BTN_2, GPIO_IN);
    gpio_pull_up(PIN_BTN_2);

    uint encoder_offset = pio_add_program(ENCODER_PIO, &quadrature_encoder_program);
    quadrature_encoder_program_init(ENCODER_PIO, ENCODER_SM, encoder_offset, PIN_AB, 0);

    uint nco_offset = pio_add_program(NCO_PIO, &nco_program);
    nco_program_init(NCO_PIO, NCO_SM, nco_offset);

    uint32_t target_frequency_hz = 7000000;
    int32_t last_encoder_count = quadrature_encoder_get_count(ENCODER_PIO, ENCODER_SM);
    int32_t encoder_remainder = 0;
    draw_oled_status(display, target_frequency_hz);

    while (true) {
        int32_t current_count = quadrature_encoder_get_count(ENCODER_PIO, ENCODER_SM);
        int32_t delta = current_count - last_encoder_count;
        last_encoder_count = current_count;

        if (delta != 0) {
            const bool coarse_mode = (gpio_get(PIN_BTN_1) == 0);
            const int32_t step_hz = coarse_mode ? COARSE_STEP_HZ : BASE_STEP_HZ;

            encoder_remainder += delta;
            int32_t detents = encoder_remainder / ENCODER_COUNTS_PER_DETENT;
            encoder_remainder %= ENCODER_COUNTS_PER_DETENT;

            if (encoder_remainder < 0) {
                encoder_remainder += ENCODER_COUNTS_PER_DETENT;
                --detents;
            }

            if (detents != 0) {
                int32_t new_target = (int32_t)target_frequency_hz + (detents * step_hz);
                if (new_target < (int32_t)MIN_FREQ_HZ) {
                    new_target = MIN_FREQ_HZ;
                }
                if (new_target > (int32_t)MAX_FREQ_HZ) {
                    new_target = MAX_FREQ_HZ;
                }
                target_frequency_hz = (uint32_t)new_target;

                const uint32_t system_clock_hz = 125000000;
                const float clkdiv = compute_clkdiv_for_frequency(system_clock_hz, target_frequency_hz);
                pio_sm_set_clkdiv(NCO_PIO, NCO_SM, clkdiv);

                printf("LO target: %lu Hz  clkdiv: %.6f\n",
                       (unsigned long)target_frequency_hz,
                       (double)clkdiv);

                draw_oled_status(display, target_frequency_hz);
            }
        }

        sleep_ms(20);
    }

    return 0;
}
