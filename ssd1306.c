#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ssd1306.h"
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

inline static void ssd1306_write(ssd1306_t *p, uint8_t val) {
    uint8_t d[2]= {0x00, val};
    fancy_write(p->i2c_i, p->address, d, 2, "ssd1306_write");
}

void ssd1306_set_start_line(ssd1306_t *p, uint8_t val) {
    ssd1306_write(p, 0x40+val);
}

void ssd1306_scroll_screen(ssd1306_t *p, int16_t x, int16_t y) {
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

bool ssd1306_init(ssd1306_t *p, uint16_t width, uint16_t height, uint8_t address, i2c_inst_t *i2c_instance) {
    p->width=width;
    p->height=height;
    p->pages=height/8;
    p->address=address;
    p->disp_col_offset=0;
    p->i2c_i=i2c_instance;
    p->bufsize = (p->pages) * (1 + p->width);
    p->buffer = NULL;
    p->power_state = true;

    uint8_t init_cmds[] = {
        0xAE, // display off
        0xD5, 0x80, // clock divide ratio / oscillator freq
        0xA8, 0x3F, // multiplex ratio
        0xD3, 0x00, // display offset
        0x40, // start line 0
        0x8D, 0x14, // charge pump
        0x20, 0x00, // memory mode = horizontal
        0xA1, // segment remap
        0xC8, // COM scan direction
        0xDA, 0x12, // COM pins hardware config
        0x81, 0xCF, // contrast
        0xD9, 0xF1, // pre-charge period
        0xDB, 0x40, // VCOMH deselect
        0xA4, // entire display on normal
        0xA6, // normal display
        0xAF  // display on
    };

    if (p->buffer == NULL) {
        p->buffer = (uint8_t *)calloc(1, p->bufsize);
        if (p->buffer == NULL) {
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(init_cmds); ++i) {
        ssd1306_write(p, init_cmds[i]);
    }

    ssd1306_clear(p, 0);
    ssd1306_show(p);
    return true;
}

void ssd1306_deinit(ssd1306_t *p)
{
    if ( p->buffer )
    {
        free(p->buffer);
    }
}

void ssd1306_poweroff(ssd1306_t *p) {
    ssd1306_write(p, SET_DISP|0x00);
    p->power_state = true;
}

void ssd1306_poweron(ssd1306_t *p) {
    ssd1306_write(p, SET_DISP|0x01);
    p->power_state = true;
}

void ssd1306_invert(ssd1306_t *p, uint8_t inv) {
    ssd1306_write(p, SET_NORM_INV | (inv & 1));
}

void ssd1306_clear(ssd1306_t *p, uint8_t colour) {
    memset(p->buffer, 0xff*(colour > 0), p->bufsize);
}

void ssd1306_draw_pixel(ssd1306_t *p, int32_t x, int32_t y, uint8_t colour) {
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

void ssd1306_draw_line(ssd1306_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint8_t colour) {
    if(x1>x2) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    if(x1==x2) {
        if(y1>y2)
            swap(&y1, &y2);
        for(int32_t i=y1; i<=y2; ++i)
            ssd1306_draw_pixel(p, x1, i, colour);
        return;
    }

    float m=(float) (y2-y1) / (float) (x2-x1);
    for(int32_t i=x1; i<=x2; ++i) {
        float y=m*(float) (i-x1)+(float) y1;
        ssd1306_draw_pixel(p, i, (int32_t) y, colour);
    }
}

void ssd1306_fill_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    for(int32_t i=0; i<width; ++i)
        for(int32_t j=0; j<height; ++j)
            ssd1306_draw_pixel(p, x+i, y+j, colour);
}

void ssd1306_draw_rectangle(ssd1306_t *p, int32_t x, int32_t y, uint32_t width, uint32_t height, uint8_t colour) {
    ssd1306_draw_line(p, x, y, x+width, y, colour);
    ssd1306_draw_line(p, x, y+height, x+width, y+height, colour);
    ssd1306_draw_line(p, x, y, x, y+height, colour);
    ssd1306_draw_line(p, x+width, y, x+width, y+height, colour);
}

void ssd1306_draw_char_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, char c, uint8_t colour) {
    if(c<font[3]||c>font[4]) return;
    uint32_t parts_per_line=(font[0]>>3)+((font[0]&7)>0);
    for(uint8_t w=0; w<font[1]; ++w) {
        uint32_t pp=(c-font[3])*font[1]*parts_per_line+w*parts_per_line+5;
        for(uint32_t lp=0; lp<parts_per_line; ++lp) {
            uint8_t line=font[pp];
            for(int8_t j=0; j<8; ++j, line>>=1) {
                if(line & 1)
                    ssd1306_fill_rectangle(p, x+w*scale, y+((lp<<3)+j)*scale, scale, scale, colour);
            }
            ++pp;
        }
    }
}

void ssd1306_draw_string_with_font(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const uint8_t *font, const char *s, uint8_t colour) {
    for(int32_t x_n=x; *s; x_n+=(font[1]+font[2])*scale) {
        ssd1306_draw_char_with_font(p, x_n, y, scale, font, *(s++), colour);
    }
}

void ssd1306_draw_char(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, char c, uint8_t colour) {
    ssd1306_draw_char_with_font(p, x, y, scale, font_8x5, c, colour);
}

void ssd1306_draw_string(ssd1306_t *p, int32_t x, int32_t y, uint32_t scale, const char *s, uint8_t colour) {
    ssd1306_draw_string_with_font(p, x, y, scale, font_8x5, s, colour);
}

void ssd1306_bmp_show_image_with_offset(ssd1306_t *p, const uint8_t *data, const long size, uint32_t x_offset, uint32_t y_offset) {
    (void)p; (void)data; (void)size; (void)x_offset; (void)y_offset;
}

void ssd1306_bmp_show_image(ssd1306_t *p, const uint8_t *data, const long size) {
    ssd1306_bmp_show_image_with_offset(p, data, size, 0, 0);
}

void ssd1306_show(ssd1306_t *p)
{
    if (!p->power_state) return;
    for (uint8_t page = 0; page < p->pages; page++)
    {
        uint8_t payload[] = {(0x00 | (p->disp_col_offset & 0x0F)), (0x10 | (p->disp_col_offset >> 4)), (0xB0 | (page & 0x0F))};
        for (size_t i = 0; i < sizeof(payload); ++i)
            ssd1306_write(p, payload[i]);
        p->buffer[page * (1+p->width)] = 0x40;
        fancy_write(p->i2c_i, p->address, &p->buffer[page * (1+p->width)], p->width + 1, "ssd1306_show");
    }
}
