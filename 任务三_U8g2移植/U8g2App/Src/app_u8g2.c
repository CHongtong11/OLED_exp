/**
  ******************************************************************************
  * @file    app_u8g2.c
  * @brief   任务3：把 U8g2 移植到 STM32F103 + 0.96" I2C OLED 之后的界面层。
  *
  *  四屏轮播（每 5 s）：
  *    0 - 姓名 / 学号              （中文用 XBM 字模 + u8g2_DrawXBMP）
  *    1 - 长句左右滚动             （u8g2 缓冲 + 逐像素裁剪）
  *    2 - 头像图案 + 浮动/眨眼动画
  *    3 - U8g2 demo：自带字体的文本 + 矩形/圆/线 + 移动方块
  *
  *  中文为什么不直接用 u8g2 的 wqy 字库：F103C8 只有 64 KB Flash，
  *  u8g2_font_wqy16_t_gb2312 一份就是 318 KB，chinese1 也要 14 KB；
  *  这里采用"点阵字模 + DrawXBMP"的方式，整个中文字库只占 0.7 KB。
  ******************************************************************************
  */
#include "main.h"
#include "u8g2.h"
#include "u8g2_port.h"
#include "app_u8g2.h"
#include "xbm_font.h"
#include "strings.h"

u8g2_t u8g2;

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */
static void put_pixel(u8g2_t *u, int16_t x, int16_t y)
{
    if (x < 0 || x > 127 || y < 0 || y > 63) {
        return;
    }
    u8g2_DrawPixel(u, (u8g2_uint_t)x, (u8g2_uint_t)y);
}

/* 8x16 ASCII glyph, XBM layout: 1 byte per row, LSB = leftmost pixel */
static void draw_ascii(u8g2_t *u, int16_t x, int16_t y, uint8_t ch)
{
    const uint8_t *g;
    int16_t r;
    int16_t c;
    uint8_t line;

    if (ch < 0x20u || ch > 0x7Eu) {
        ch = '?';
    }
    g = xbm_ascii8x16[ch - 0x20u];

    for (r = 0; r < 16; r++) {
        line = g[r];
        for (c = 0; c < 8; c++) {
            if (line & (1u << c)) {
                put_pixel(u, (int16_t)(x + c), (int16_t)(y + r));
            }
        }
    }
}

/* 16x16 Chinese glyph, XBM layout: 2 bytes per row */
static void draw_cjk(u8g2_t *u, int16_t x, int16_t y, const uint8_t *g)
{
    int16_t r;
    int16_t c;
    uint8_t line;

    for (r = 0; r < 16; r++) {
        for (c = 0; c < 16; c++) {
            line = g[r * 2 + (c >> 3)];
            if (line & (1u << (c & 7))) {
                put_pixel(u, (int16_t)(x + c), (int16_t)(y + r));
            }
        }
    }
}

/* UTF-8 decode */
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

static const uint8_t *cjk_lookup(uint16_t cp)
{
    int i;

    for (i = 0; i < XBM_CJK_COUNT; i++) {
        if (xbm_cjk_cp[i] == cp) {
            return xbm_cjk16x16[i];
        }
    }
    return 0;
}

static int16_t text_width(const char *text)
{
    int16_t w = 0;
    uint16_t cp;

    while (*text) {
        cp = utf8_next(&text);
        w = (int16_t)(w + ((cp < 0x80u) ? 8 : 16));
    }
    return w;
}

/* returns the pen position after the last character */
static int16_t draw_text(u8g2_t *u, int16_t x, int16_t y, const char *text)
{
    int16_t start = x;
    uint16_t cp;
    const uint8_t *g;

    while (*text) {
        cp = utf8_next(&text);
        if (cp < 0x80u) {
            if ((x > 128) || ((x + 8) < 0)) {
                x = (int16_t)(x + 8);
                continue;
            }
            draw_ascii(u, x, y, (uint8_t)cp);
            x = (int16_t)(x + 8);
        } else {
            if ((x > 128) || ((x + 16) < 0)) {
                x = (int16_t)(x + 16);
                continue;
            }
            g = cjk_lookup(cp);
            if (g != 0) {
                draw_cjk(u, x, y, g);
            }
            x = (int16_t)(x + 16);
        }
    }
    return (int16_t)(x - start);
}

/* ------------------------------------------------------------------ */
/* screens                                                            */
/* ------------------------------------------------------------------ */
static void page_card(void)
{
    draw_text(&u8g2, 0, 0, TXT_NAME);
    draw_text(&u8g2, 0, 16, TXT_ID);
    u8g2_DrawHLine(&u8g2, 0, 34, 128);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 0, 52, "STM32F103C8T6 + U8g2");
}

static void page_scroll(uint32_t page_ms)
{
    static const char ascii_line[] = "U8g2 on STM32F103 - 128x64 SSD1306";
    int16_t w;
    int16_t x;
    uint32_t step = (page_ms / 30u) * 2u;       /* 2 px every 30 ms */

    w = text_width(TXT_LONG);
    x = (int16_t)(128 - (int32_t)(step % (uint32_t)(w + 128)));
    draw_text(&u8g2, x, 4, TXT_LONG);

    w = text_width(ascii_line);
    x = (int16_t)(-w + (int32_t)(step % (uint32_t)(w + 128)));
    draw_text(&u8g2, x, 34, ascii_line);
}

static void page_avatar(uint32_t page_ms)
{
    const uint8_t *face;
    uint32_t phase = page_ms % 2000u;
    uint32_t bob = (page_ms / 250u) % 8u;
    int16_t dy;

    dy = (bob < 4u) ? (int16_t)bob : (int16_t)(8u - bob);
    face = ((phase > 1700u) && (phase < 1850u)) ? xbm_avatar_blink
                                                : xbm_avatar_open;

    u8g2_DrawFrame(&u8g2, 42, 0, 44, 46);
    u8g2_DrawXBMP(&u8g2, 44, (u8g2_uint_t)dy, 40, 40, face);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 34, 60, "IoT 205");
}

static void page_demo(uint32_t page_ms)
{
    int16_t bx = (int16_t)((page_ms / 20u) % 96u);

    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    u8g2_DrawStr(&u8g2, 0, 10, "U8g2 demo");
    u8g2_DrawStr(&u8g2, 0, 22, "box/circle/line");

    u8g2_DrawFrame(&u8g2, 2, 26, 40, 18);
    u8g2_DrawCircle(&u8g2, 88, 35, 9, U8G2_DRAW_ALL);
    u8g2_DrawLine(&u8g2, 46, 44, 76, 26);

    u8g2_DrawBox(&u8g2, bx, 50, 12, 10);
}

/* ------------------------------------------------------------------ */
/* public API                                                         */
/* ------------------------------------------------------------------ */
void app_u8g2_init(void)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0,
                                           u8x8_byte_stm32_hw_i2c,
                                           u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
}

void app_u8g2_render(uint8_t page, uint32_t page_ms)
{
    u8g2_ClearBuffer(&u8g2);

    switch (page) {
    case 0:
        page_card();
        break;
    case 1:
        page_scroll(page_ms);
        break;
    case 2:
        page_avatar(page_ms);
        break;
    default:
        page_demo(page_ms);
        break;
    }

    u8g2_SendBuffer(&u8g2);
}

void app_u8g2_loop(void)
{
    uint32_t page_tick = HAL_GetTick();
    uint8_t page = 0;

    for (;;) {
        uint32_t now = HAL_GetTick();
        uint32_t page_ms = now - page_tick;

        if (page_ms >= U8G2_PAGE_MS) {
            page = (uint8_t)((page + 1u) % U8G2_PAGE_COUNT);
            page_tick = now;
            page_ms = 0;
        }

        app_u8g2_render(page, page_ms);
        HAL_Delay(20);
    }
}
