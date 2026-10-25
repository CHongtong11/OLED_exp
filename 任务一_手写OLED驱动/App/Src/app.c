/**
  ******************************************************************************
  * @file    app.c
  * @brief   Screens on the 128x64 OLED:
  *            0 - student name and ID
  *            1 - AHT20 temperature / humidity   (only when APP_USE_AHT20 = 1)
  *            2 - long sentence, scrolling
  *            3 - avatar with a bouncing / blinking animation
  *
  *  Every screen is redrawn completely into the framebuffer and then flushed,
  *  so the animation never depends on what was on the panel before.
  ******************************************************************************
  */
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "app.h"
#include "ssd1306.h"
#include "gfx.h"
#include "avatar.h"
#include "strings.h"

#if APP_USE_AHT20
#include "aht20.h"
#endif

/* 1 = use the SSD1306 hardware scroll command on the scrolling screen instead
 * of the software marquee (the whole panel scrolls in that mode). */
#ifndef USE_HW_SCROLL
#define USE_HW_SCROLL   0
#endif

#define SENSOR_PERIOD_MS  2000u
#define FRAME_PERIOD_MS   20u

#if APP_USE_AHT20
static const uint8_t s_screens[] = {
    SCR_CARD, SCR_SENSOR, SCR_SCROLL, SCR_AVATAR
};
#else
static const uint8_t s_screens[] = {
    SCR_CARD, SCR_SCROLL, SCR_AVATAR
};
#endif

#define SCREEN_COUNT  ((uint8_t)(sizeof(s_screens) / sizeof(s_screens[0])))

static uint8_t  s_index;

#if APP_USE_AHT20
static int16_t  s_temp_x10;
static uint16_t s_humi_x10;
static uint8_t  s_sensor_ok;
static uint32_t s_sensor_tick;
#endif

static uint32_t s_page_tick;
#if USE_HW_SCROLL
static uint8_t  s_hw_scroll_on;
#endif

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */
#if APP_USE_AHT20
static void fmt_x10(char *dst, int16_t value)
{
    if (value < 0) {
        *dst++ = '-';
        value = (int16_t)(-value);
    }
    sprintf(dst, "%d.%d", (int)(value / 10), (int)(value % 10));
}

/* Draw "label:value unit" and return the x position after it. */
static int16_t draw_value(int16_t x, int16_t y, const char *label,
                          const char *value, const char *unit)
{
    x = (int16_t)(x + gfx_draw_text(x, y, label));
    x = (int16_t)(x + gfx_draw_text(x, y, ":"));
    x = (int16_t)(x + gfx_draw_text(x, y, value));
    x = (int16_t)(x + gfx_draw_text(x, y, unit));
    return x;
}
#endif

/* ------------------------------------------------------------------ */
/* screens                                                            */
/* ------------------------------------------------------------------ */
static void render_card(void)
{
    gfx_draw_text(0, 0, TXT_NAME);
    gfx_draw_text(0, 16, TXT_ID);
    ssd1306_draw_hline(0, 34, 128);
    gfx_draw_text(0, 40, "STM32F103C8T6");
}

#if APP_USE_AHT20
static void render_sensor(void)
{
    char temp[10];
    char humi[10];
    int16_t pct;

    if (s_sensor_ok) {
        fmt_x10(temp, s_temp_x10);
        fmt_x10(humi, (int16_t)s_humi_x10);
    } else {
        strcpy(temp, "--.-");
        strcpy(humi, "--.-");
    }

    draw_value(0, 0,  TXT_TEMP, temp, "C");
    draw_value(0, 16, TXT_HUMI, humi, "%");

    /* humidity bar, 0..100 % */
    pct = (int16_t)(s_humi_x10 / 10u);
    if (pct < 0) {
        pct = 0;
    }
    if (pct > 100) {
        pct = 100;
    }
    ssd1306_draw_rect(0, 36, 122, 10);
    ssd1306_fill_rect(2, 38, (int16_t)((pct * 118) / 100), 6);

    gfx_draw_text(88, 48, "AHT20");
}
#endif

static void render_scroll(uint32_t page_ms)
{
    static const char ascii_line[] = "STM32F103 + SSD1306 128x64";
    int16_t w;
    int16_t x;
    uint32_t step = (page_ms / 30u) * 2u;   /* 2 px every 30 ms */

    w = gfx_text_width(TXT_LONG);
    x = (int16_t)(128 - (int16_t)(step % (uint32_t)(w + 128)));
    gfx_draw_text(x, 4, TXT_LONG);

    w = gfx_text_width(ascii_line);
    x = (int16_t)(-w + (int16_t)(step % (uint32_t)(w + 128)));
    gfx_draw_text(x, 34, ascii_line);
}

static void render_avatar(uint32_t page_ms)
{
    const uint8_t *face;
    uint32_t phase = page_ms % 2000u;
    uint32_t bob = (page_ms / 250u) % 8u;
    int16_t dy;

    /* 0..4..0 triangle wave */
    dy = (bob < 4u) ? (int16_t)bob : (int16_t)(8u - bob);

    /* blink for 150 ms every 2 s */
    face = ((phase > 1700u) && (phase < 1850u)) ? avatar_blink : avatar_open;

    ssd1306_draw_rect(42, 0, 44, 46);
    ssd1306_draw_bitmap(44, dy, face, AVATAR_W, AVATAR_H);

    gfx_draw_text(34, 48, "IoT 205");
}

/* ------------------------------------------------------------------ */
/* public API                                                         */
/* ------------------------------------------------------------------ */
void app_render(uint8_t page, uint32_t page_ms)
{
    ssd1306_clear();

    switch (page) {
    case SCR_CARD:
        render_card();
        break;
#if APP_USE_AHT20
    case SCR_SENSOR:
        render_sensor();
        break;
#endif
    case SCR_SCROLL:
        render_scroll(page_ms);
        break;
    case SCR_AVATAR:
        render_avatar(page_ms);
        break;
    default:
        break;
    }

    ssd1306_refresh();
}

void app_init(void)
{
    ssd1306_init();

#if APP_USE_AHT20
    s_sensor_ok = (aht20_init() == 0u) ? 1u : 0u;
    if (s_sensor_ok != 0u) {
        s_sensor_ok = (aht20_read(&s_temp_x10, &s_humi_x10) == 0u) ? 1u : 0u;
    }
    s_sensor_tick = HAL_GetTick();
#endif

    s_index = 0;
    s_page_tick = HAL_GetTick();
#if USE_HW_SCROLL
    s_hw_scroll_on = 0;
#endif
}

void app_loop(void)
{
    for (;;) {
        uint32_t now = HAL_GetTick();
        uint32_t page_ms = now - s_page_tick;
        uint8_t  screen;

        if (page_ms >= APP_PAGE_MS) {
            s_index = (uint8_t)((s_index + 1u) % SCREEN_COUNT);
            s_page_tick = now;
            page_ms = 0;
        }
        screen = s_screens[s_index];

#if APP_USE_AHT20
        if ((now - s_sensor_tick) >= SENSOR_PERIOD_MS) {
            int16_t t;
            uint16_t h;

            if (aht20_read(&t, &h) == 0u) {
                s_temp_x10 = t;
                s_humi_x10 = h;
                s_sensor_ok = 1;
            } else {
                s_sensor_ok = 0;
            }
            s_sensor_tick = now;
        }
#endif

#if USE_HW_SCROLL
        if (screen == SCR_SCROLL) {
            if (s_hw_scroll_on == 0u) {
                app_render(SCR_SCROLL, 0);
                ssd1306_hw_scroll_start(1, 2);
                s_hw_scroll_on = 1u;
            }
        } else if (s_hw_scroll_on != 0u) {
            ssd1306_hw_scroll_stop();
            s_hw_scroll_on = 0u;
            app_render(screen, 0);
        }
#else
        app_render(screen, page_ms);
#endif

        HAL_Delay(FRAME_PERIOD_MS);
    }
}
