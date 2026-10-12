#ifndef _inc_sh1106
#define _inc_sh1106
#include <pico/stdlib.h>
#include <hardware/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SET_CONTRAST = 0x81,
    SET_ENTIRE_ON = 0xA4,
    SET_NORM_INV = 0xA6,
    SET_DISP = 0xAE,
    SET_LOW_COL_ADDR = 0x00,
    SET_HIGH_COL_ADDR = 0x10,
    SET_PAGE_ADDR = 0xB0,
    SET_DISP_START_LINE = 0x40,
    SET_SEG_REMAP = 0xA0,
    SET_MUX_RATIO = 0xA8,
    SET_COM_OUT_DIR = 0xC0,
    SET_DISP_OFFSET = 0xD3,
    SET_COM_PIN_CFG = 0xDA,
    SET_DISP_CLK_DIV = 0xD5,
    SET_PRECHARGE = 0xD9,
    SET_VCOM_DESEL = 0xDB,
    SET_DC_DC = 0xAD
} sh1106_command_t;

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t pages;
    uint8_t address;
    i2c_inst_t *i2c_i;
    bool external_vcc;
    uint8_t *buffer;
    size_t bufsize;
    uint8_t disp_col_offset;
    bool power_state;
} sh1106_t;

void sh1106_set_start_line(sh1106_t *p, uint8_t val);
void sh1106_scroll_screen(sh1106_t *p, int16_t x, int16_t y);
// Supports 128x64 I2C modules with a two-column offset into SH1106 RAM.
bool sh1106_init(sh1106_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance);
void sh1106_deinit(sh1106_t *p);
void sh1106_poweroff(sh1106_t *p);
void sh1106_poweron(sh1106_t *p);
void sh1106_invert(sh1106_t *p, uint8_t inv);
void sh1106_show(sh1106_t *p);
void sh1106_clear(sh1106_t *p, uint8_t colour);
void sh1106_draw_pixel(sh1106_t *p, int32_t x, int32_t y, uint8_t colour);
void sh1106_draw_line(sh1106_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t colour);
void sh1106_fill_rectangle(sh1106_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour);
void sh1106_draw_rectangle(sh1106_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour);
void sh1106_bmp_show_image_with_offset(sh1106_t *p, const uint8_t *data, const long size, uint32_t x_offset, uint32_t y_offset);
void sh1106_bmp_show_image(sh1106_t *p, const uint8_t *data, const long size);
void sh1106_draw_char_with_font(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, char c, uint8_t colour);
void sh1106_draw_char(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, char c, uint8_t colour);
void sh1106_draw_string_with_font(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, const char *s, uint8_t colour);
void sh1106_draw_string(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const char *s, uint8_t colour);

#ifdef __cplusplus
}
#endif

#endif
