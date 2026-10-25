/**
  ******************************************************************************
  * @file    gfx.c
  * @brief   UTF-8 text renderer for the SSD1306 framebuffer.
  *
  *  - ASCII 0x20..0x7E : 8x16 bitmaps, 8 pixel advance
  *  - Chinese          : 16x16 bitmaps looked up by Unicode code point,
  *                       16 pixel advance
  *  - unknown glyphs   : skipped, the pen still advances, nothing is drawn
  ******************************************************************************
  */
#include "gfx.h"
#include "font.h"
#include "ssd1306.h"

static const uint8_t *cjk_lookup(uint16_t cp)
{
    uint16_t i;

    for (i = 0; i < (uint16_t)FONT_CJK_COUNT; i++) {
        if (font_cjk_cp[i] == cp) {
            return font_cjk16x16[i];
        }
    }
    return 0;
}

/* Decode one UTF-8 character and advance the pointer. */
static uint16_t utf8_next(const char **ps)
{
    const unsigned char *s = (const unsigned char *)(*ps);
    uint16_t cp;

    if (s[0] < 0x80u) {
        cp = s[0];
        *ps += 1;
    } else if ((s[0] & 0xE0u) == 0xC0u) {
        cp = (uint16_t)(((uint16_t)(s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu));
        *ps += 2;
    } else if ((s[0] & 0xF0u) == 0xE0u) {
        cp = (uint16_t)(((uint16_t)(s[0] & 0x0Fu) << 12) |
                        ((uint16_t)(s[1] & 0x3Fu) << 6) |
                         (uint16_t)(s[2] & 0x3Fu));
        *ps += 3;
    } else {
        cp = '?';
        *ps += 1;
    }
    return cp;
}

static void draw_ascii(int16_t x, int16_t y, uint8_t ch)
{
    const uint8_t *g;
    int16_t r;
    int16_t c;
    uint8_t line;

    if (ch < 0x20u || ch > 0x7Eu) {
        ch = '?';
    }
    g = font_ascii8x16[ch - 0x20u];

    for (r = 0; r < 16; r++) {
        line = g[r];
        for (c = 0; c < 8; c++) {
            if (line & (0x80u >> c)) {
                ssd1306_draw_pixel((int16_t)(x + c), (int16_t)(y + r));
            }
        }
    }
}

static void draw_cjk(int16_t x, int16_t y, const uint8_t *g)
{
    int16_t r;
    int16_t c;
    uint16_t line;

    for (r = 0; r < 16; r++) {
        line = (uint16_t)(((uint16_t)g[r * 2] << 8) | g[r * 2 + 1]);
        for (c = 0; c < 16; c++) {
            if (line & (0x8000u >> c)) {
                ssd1306_draw_pixel((int16_t)(x + c), (int16_t)(y + r));
            }
        }
    }
}

int16_t gfx_draw_text(int16_t x, int16_t y, const char *text)
{
    int16_t start = x;
    uint16_t cp;
    const uint8_t *g;

    while (*text) {
        cp = utf8_next(&text);
        if (cp < 0x80u) {
            draw_ascii(x, y, (uint8_t)cp);
            x = (int16_t)(x + 8);
        } else {
            g = cjk_lookup(cp);
            if (g != 0) {
                draw_cjk(x, y, g);
            }
            x = (int16_t)(x + 16);
        }
    }
    return (int16_t)(x - start);
}

int16_t gfx_text_width(const char *text)
{
    int16_t w = 0;
    uint16_t cp;

    while (*text) {
        cp = utf8_next(&text);
        w = (int16_t)(w + ((cp < 0x80u) ? 8 : 16));
    }
    return w;
}
