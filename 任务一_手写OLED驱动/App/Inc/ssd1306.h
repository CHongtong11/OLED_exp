/**
  ******************************************************************************
  * @file    ssd1306.h
  * @brief   0.96" 128x64 OLED (SSD1306) driver on the hardware I2C bus.
  ******************************************************************************
  */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "main.h"

/* 7 bit address 0x3C shifted left for the HAL API */
#define SSD1306_I2C_ADDR   (0x3C << 1)
#define SSD1306_WIDTH      128
#define SSD1306_HEIGHT     64

/* The CubeMX generated main.c owns this handle. */
extern I2C_HandleTypeDef hi2c1;

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_refresh(void);

void ssd1306_draw_pixel(int16_t x, int16_t y);
void ssd1306_draw_hline(int16_t x, int16_t y, int16_t w);
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h);
void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h);
void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bmp,
                         int16_t w, int16_t h);

/* SSD1306 hardware scroll (0x26 / 0x27 + 0x2F).  left: 1 = content moves
 * to the left, 0 = to the right.  speed: 0 (slowest) .. 7 (fastest). */
void ssd1306_hw_scroll_start(uint8_t left, uint8_t speed);
void ssd1306_hw_scroll_stop(void);

#endif /* SSD1306_H */
