/**
  ******************************************************************************
  * @file    ssd1306.c
  * @brief   SSD1306 128x64 OLED driver: software framebuffer + hardware I2C.
  *
  * The panel RAM is organised in 8 pages of 128 columns, one byte holds 8
  * vertical pixels.  A 1024 byte buffer mirrors it, every drawing routine
  * only touches the buffer, ssd1306_refresh() pushes it to the panel.
  ******************************************************************************
  */
#include <string.h>
#include "ssd1306.h"

static uint8_t s_buf[SSD1306_HEIGHT / 8][SSD1306_WIDTH];

/* ---------------------------------------------------------------------- */
/* low level I2C helpers                                                  */
/* ---------------------------------------------------------------------- */
static void ssd1306_cmd(uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = 0x00;              /* control byte: command stream */
    buf[1] = cmd;
    (void)HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, buf, 2, 100);
}

static void ssd1306_data(const uint8_t *data, uint16_t len)
{
    uint8_t buf[SSD1306_WIDTH + 1];

    if (len > SSD1306_WIDTH) {
        len = SSD1306_WIDTH;
    }
    buf[0] = 0x40;              /* control byte: data stream */
    memcpy(&buf[1], data, len);
    (void)HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, buf,
                                  (uint16_t)(len + 1U), 100);
}

/* ---------------------------------------------------------------------- */
/* public API                                                             */
/* ---------------------------------------------------------------------- */
void ssd1306_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,               /* display off                      */
        0xD5, 0x80,         /* clock divide ratio / oscillator  */
        0xA8, 0x3F,         /* multiplex ratio 1/64             */
        0xD3, 0x00,         /* display offset 0                 */
        0x40,               /* start line 0                     */
        0x8D, 0x14,         /* charge pump enabled              */
        0x20, 0x02,         /* page addressing mode             */
        0xA1,               /* segment remap                    */
        0xC8,               /* COM scan direction remapped      */
        0xDA, 0x12,         /* COM pins hardware configuration  */
        0x81, 0xCF,         /* contrast                         */
        0xD9, 0xF1,         /* pre-charge period                */
        0xDB, 0x40,         /* VCOMH deselect level             */
        0xA4,               /* output follows RAM content       */
        0xA6,               /* normal (not inverted)            */
        0xAF                /* display on                       */
    };
    uint32_t i;

    HAL_Delay(100);         /* let the panel power up            */
    for (i = 0; i < sizeof(init_seq); i++) {
        ssd1306_cmd(init_seq[i]);
    }
    ssd1306_clear();
    ssd1306_refresh();
}

void ssd1306_clear(void)
{
    memset(s_buf, 0x00, sizeof(s_buf));
}

void ssd1306_refresh(void)
{
    uint8_t page;

    for (page = 0; page < (SSD1306_HEIGHT / 8); page++) {
        ssd1306_cmd((uint8_t)(0xB0 | page));    /* page start address */
        ssd1306_cmd(0x00);                      /* column low  nibble */
        ssd1306_cmd(0x10);                      /* column high nibble */
        ssd1306_data(s_buf[page], SSD1306_WIDTH);
    }
}

void ssd1306_draw_pixel(int16_t x, int16_t y)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }
    s_buf[y >> 3][x] |= (uint8_t)(1u << (y & 7));
}

void ssd1306_draw_hline(int16_t x, int16_t y, int16_t w)
{
    int16_t i;

    for (i = 0; i < w; i++) {
        ssd1306_draw_pixel((int16_t)(x + i), y);
    }
}

void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t i;
    int16_t j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            ssd1306_draw_pixel((int16_t)(x + i), (int16_t)(y + j));
        }
    }
}

void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    ssd1306_draw_hline(x, y, w);
    ssd1306_draw_hline(x, (int16_t)(y + h - 1), w);
    ssd1306_fill_rect(x, y, 1, h);
    ssd1306_fill_rect((int16_t)(x + w - 1), y, 1, h);
}

void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bmp,
                         int16_t w, int16_t h)
{
    int16_t stride = (int16_t)((w + 7) / 8);
    int16_t row;
    int16_t col;
    uint8_t byte;

    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            byte = bmp[row * stride + (col >> 3)];
            if (byte & (0x80u >> (col & 7))) {
                ssd1306_draw_pixel((int16_t)(x + col), (int16_t)(y + row));
            }
        }
    }
}

void ssd1306_hw_scroll_start(uint8_t left, uint8_t speed)
{
    ssd1306_cmd(0x2E);                  /* deactivate first             */
    ssd1306_cmd(left ? 0x27 : 0x26);    /* 0x27 = left, 0x26 = right    */
    ssd1306_cmd(0x00);                  /* dummy byte                   */
    ssd1306_cmd(0x00);                  /* start page                   */
    ssd1306_cmd((uint8_t)(speed & 0x07));/* frame interval              */
    ssd1306_cmd(0x07);                  /* end page                     */
    ssd1306_cmd(0x00);                  /* dummy byte                   */
    ssd1306_cmd(0xFF);                  /* dummy byte                   */
    ssd1306_cmd(0x2F);                  /* activate                     */
}

void ssd1306_hw_scroll_stop(void)
{
    ssd1306_cmd(0x2E);
}
