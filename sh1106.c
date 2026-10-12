#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sh1106.h"
#include "font_8x5.h"

inline static void swap(int32_t *a, int32_t *b) {
    int32_t *t=a;
    *a=*b;
    *b=*t;
}

inline static void fancy_write(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, char *name) {
    switch(i2c_write_blocking(i2c, addr, src, len, false)) {
    case PICO_ERROR_GENERIC:
        printf("[%s] addr not acknowledged!\n", name);
        break;
    case PICO_ERROR_TIMEOUT:
        printf("[%s] timeout!\n", name);
        break;
    default:
        break;
    }
}

inline static void sh1106_write(sh1106_t *p, uint8_t val) {
    uint8_t d[2]= {0x00, val};
    fancy_write(p->i2c_i, p->address, d, 2, "sh1106_write");
}

void sh1106_set_start_line(sh1106_t *p, uint8_t val) {
    sh1106_write(p, 0x40+val);
}

void sh1106_scroll_screen(sh1106_t *p, int16_t x, int16_t y) {
  int32_t start = 0;
  int32_t end = p->width;
  int32_t count = 1;
  if (x>0) {
    start = p->width;
    end = 0;
    count = -1;
  }
  for (int32_t i=start; i!=end; i+=count) {
    uint64_t temp=0;
    for (uint32_t page=0; page < p->pages; page++) {
      temp |= (uint64_t)p->buffer[page*(p->width) + i] << 8*page;
    }
    if (y>0) {
        temp <<= y;
    } else if (y<0) {
        temp >>= -y;
    }
    unsigned int new_x = i + x;
    if ((new_x >= 0) && (new_x <= (p->width - 1))) {
        for (uint32_t page=0; page < p->pages; page++) {
            p->buffer[page*p->width + new_x] = temp & 0xff;
            temp >>= 8;
        }
    }
  }
  if (x != 0) {
    int32_t start = p->width+x;
    int32_t end = p->width - 1;
    if (x>0) {
        start = 1;
        end = x;
    }
    for (int32_t i=start; i<=end; i++) {
        for (uint32_t page=0; page < p->pages; page++) {
            p->buffer[page*p->width + i] = 0;
        }
    }
  }
}

bool sh1106_init(sh1106_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance) {
    if (p == NULL || width != 128 || height != 64) {
        return false;
    }
    p->width=width;
    p->height=height;
    p->pages=height/8;
    p->address=address;
    p->disp_col_offset=2; // Visible columns 2..129 in the 132-column RAM.
    p->i2c_i=i2c_instance;
    p->bufsize = p->pages * p->width;
    p->buffer = NULL;
    p->power_state = true;

    uint8_t init_cmds[] = {
        0xAE, // display off
        0xD5, 0x80, // clock divide ratio / oscillator freq
        0xA8, 0x3F, // multiplex ratio
        0xD3, 0x00, // display offset
        0x40, // start line 0
        SET_DC_DC, p->external_vcc ? 0x8A : 0x8B, // SH1106 DC-DC off/on
        0xA1, // segment remap
        0xC8, // COM scan direction
        0xDA, 0x12, // COM pins hardware config
        0x81, 0xCF, // contrast
        0xD9, 0x22, // discharge / pre-charge periods
        0xDB, 0x40, // VCOMH deselect
        0xA4, // entire display on normal
        0xA6  // normal display; enable after clearing display RAM
    };

    if (p->buffer == NULL) {
        p->buffer = (uint8_t *)calloc(1, p->bufsize);
        if (p->buffer == NULL) {
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(init_cmds); ++i) {
        sh1106_write(p, init_cmds[i]);
    }

    sh1106_clear(p, 0);
    sh1106_show(p);
    sleep_ms(100); // Allow the internal supply to settle before display-on.
    sh1106_poweron(p);
    return true;
}

void sh1106_deinit(sh1106_t *p)
{
    if ( p->buffer )
    {
        free(p->buffer);
    }
}

void sh1106_poweroff(sh1106_t *p) {
    sh1106_write(p, SET_DISP|0x00);
    p->power_state = false;
}

void sh1106_poweron(sh1106_t *p) {
    sh1106_write(p, SET_DISP|0x01);
    p->power_state = true;
}

void sh1106_invert(sh1106_t *p, uint8_t inv) {
    sh1106_write(p, SET_NORM_INV | (inv & 1));
}

void sh1106_clear(sh1106_t *p, uint8_t colour) {
    memset(p->buffer, 0xff*(colour > 0), p->bufsize);
}

void sh1106_draw_pixel(sh1106_t *p, int32_t x, int32_t y, uint8_t colour) {
    if(x>=p->width || y>=p->height) return;
    if(x<0 || y<0) return;
    if(colour == 2) {
        p->buffer[x + (p->width)*(y>>3)] ^= 0x1<<(y&0x07);
    } else if (colour) {
        p->buffer[x + (p->width)*(y>>3)] |= 0x1<<(y&0x07);
    } else {
        p->buffer[x + (p->width)*(y>>3)] &= ~(0x1<<(y&0x07));
    }
}

void sh1106_draw_line(sh1106_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t colour) {
    if(x1>x2) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    if(x1==x2) {
        if(y1>y2)
            swap(&y1, &y2);
        for(int32_t i=y1; i<=y2; ++i)
            sh1106_draw_pixel(p, x1, i, colour);
        return;
    }

    float m=(float) (y2-y1) / (float) (x2-x1);
    for(int32_t i=x1; i<=x2; ++i) {
        float y=m*(float) (i-x1)+(float) y1;
        sh1106_draw_pixel(p, i, (int32_t) y, colour);
    }
}

void sh1106_fill_rectangle(sh1106_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    for(int32_t i=0; i<width; ++i)
        for(int32_t j=0; j<height; ++j)
            sh1106_draw_pixel(p, x+i, y+j, colour);
}

void sh1106_draw_rectangle(sh1106_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    sh1106_draw_line(p, x, y, x+width, y, colour);
    sh1106_draw_line(p, x, y+height, x+width, y+height, colour);
    sh1106_draw_line(p, x, y, x, y+height, colour);
    sh1106_draw_line(p, x+width, y, x+width, y+height, colour);
}

void sh1106_draw_char_with_font(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, char c, uint8_t colour) {
    if(c<font[3]||c>font[4]) return;
    uint32_t parts_per_line=(font[0]>>3)+((font[0]&7)>0);
    for(uint8_t w=0; w<font[1]; ++w) {
        uint32_t pp=(c-font[3])*font[1]*parts_per_line+w*parts_per_line+5;
        for(uint32_t lp=0; lp<parts_per_line; ++lp) {
            uint8_t line=font[pp];
            for(int8_t j=0; j<8; ++j, line>>=1) {
                if(line & 1)
                    sh1106_fill_rectangle(p, x+w*scale, y+((lp<<3)+j)*scale, scale, scale, colour);
            }
            ++pp;
        }
    }
}

void sh1106_draw_string_with_font(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, const char *s, uint8_t colour) {
    for(int32_t x_n=x; *s; x_n+=(font[1]+font[2])*scale) {
        sh1106_draw_char_with_font(p, x_n, y, scale, font, *(s++), colour);
    }
}

void sh1106_draw_char(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, char c, uint8_t colour) {
    sh1106_draw_char_with_font(p, x, y, scale, font_8x5, c, colour);
}

void sh1106_draw_string(sh1106_t *p, int32_t x, int32_t y, uint32_t scale, const char *s, uint8_t colour) {
    sh1106_draw_string_with_font(p, x, y, scale, font_8x5, s, colour);
}

void sh1106_bmp_show_image_with_offset(sh1106_t *p, const uint8_t *data, const long size, uint32_t x_offset, uint32_t y_offset) {
    (void)p; (void)data; (void)size; (void)x_offset; (void)y_offset;
}

void sh1106_bmp_show_image(sh1106_t *p, const uint8_t *data, const long size) {
    sh1106_bmp_show_image_with_offset(p, data, size, 0, 0);
}

void sh1106_show(sh1106_t *p)
{
    if (!p->power_state) return;
    // Keep the I2C control byte separate from the packed pixel framebuffer.
    uint8_t data[1 + 128];
    data[0] = 0x40;
    for (uint8_t page = 0; page < p->pages; page++)
    {
        // SH1106 requires explicit page and column addressing on each row.
        uint8_t payload[] = {SET_PAGE_ADDR | page,
                             SET_LOW_COL_ADDR | (p->disp_col_offset & 0x0F),
                             SET_HIGH_COL_ADDR | (p->disp_col_offset >> 4)};
        for (size_t i = 0; i < sizeof(payload); ++i)
            sh1106_write(p, payload[i]);
        memcpy(data + 1, p->buffer + page * p->width, p->width);
        fancy_write(p->i2c_i, p->address, data, p->width + 1, "sh1106_show");
    }
}
