/**
  ******************************************************************************
  * @file    gfx.h
  * @brief   Text drawing on top of ssd1306: 8x16 ASCII + 16x16 Chinese.
  ******************************************************************************
  */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

int16_t gfx_draw_text(int16_t x, int16_t y, const char *text);
int16_t gfx_text_width(const char *text);

#endif /* GFX_H */
