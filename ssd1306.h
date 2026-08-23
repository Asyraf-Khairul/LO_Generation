#ifndef _inc_ssd1306
#define _inc_ssd1306
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
    SET_MEM_ADDR = 0x20,
    SET_COL_ADDR = 0x21,
    SET_PAGE_ADDR = 0x22,
    SET_DISP_START_LINE = 0x40,
    SET_SEG_REMAP = 0xA0,
    SET_MUX_RATIO = 0xA8,
    SET_COM_OUT_DIR = 0xC0,
    SET_DISP_OFFSET = 0xD3,
    SET_COM_PIN_CFG = 0xDA,
    SET_DISP_CLK_DIV = 0xD5,
    SET_PRECHARGE = 0xD9,
    SET_VCOM_DESEL = 0xDB,
    SET_CHARGE_PUMP = 0x8D
} ssd1306_command_t;

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
} ssd1306_t;

void ssd1306_set_start_line(ssd1306_t *p, uint8_t val);
void ssd1306_scroll_screen(ssd1306_t *p, int16_t x, int16_t y);
bool ssd1306_init(ssd1306_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance);
void ssd1306_deinit(ssd1306_t *p);
void ssd1306_poweroff(ssd1306_t *p);
void ssd1306_poweron(ssd1306_t *p);
void ssd1306_invert(ssd1306_t *p, uint8_t inv);
void ssd1306_show(ssd1306_t *p);
void ssd1306_clear(ssd1306_t *p, uint8_t colour);
void ssd1306_draw_pixel(ssd1306_t *p, int32_t x, int32_t y, uint8_t colour);
void ssd1306_draw_line(ssd1306_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t colour);
void ssd1306_fill_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour);
void ssd1306_draw_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour);
void ssd1306_bmp_show_image_with_offset(ssd1306_t *p, const uint8_t *data, const long size, uint32_t x_offset, uint32_t y_offset);
void ssd1306_bmp_show_image(ssd1306_t *p, const uint8_t *data, const long size);
void ssd1306_draw_char_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, char c, uint8_t colour);
void ssd1306_draw_char(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, char c, uint8_t colour);
void ssd1306_draw_string_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, const char *s, uint8_t colour);
void ssd1306_draw_string(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const char *s, uint8_t colour);

#ifdef __cplusplus
}
#endif

#endif
