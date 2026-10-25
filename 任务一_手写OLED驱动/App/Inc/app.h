/**
  ******************************************************************************
  * @file    app.h
  * @brief   Application layer: screens rotated on the OLED.
  ******************************************************************************
  */
#ifndef APP_H
#define APP_H

#include <stdint.h>

/* 1 = also show the AHT20 temperature / humidity screen (4 screens).
 * 0 = display only, no sensor needed (3 screens).
 * The sensor code stays in the project either way. */
#ifndef APP_USE_AHT20
#define APP_USE_AHT20    0
#endif

/* Rotate every APP_PAGE_MS milliseconds. */
#define APP_PAGE_MS      5000

/* Screen ids handed to app_render(). */
#define SCR_CARD         0
#define SCR_SENSOR       1
#define SCR_SCROLL       2
#define SCR_AVATAR       3

/* Initialise display (and sensor when enabled). */
void app_init(void);

/* Never returns: rotates the screens and animates them. */
void app_loop(void);

/* Draw one screen into the framebuffer (also used by the PC test bench).
 *   page    : SCR_xxx
 *   page_ms : milliseconds since this screen was entered
 */
void app_render(uint8_t page, uint32_t page_ms);

#endif /* APP_H */
