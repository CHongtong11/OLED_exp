/**
  ******************************************************************************
  * @file    app_u8g2.h
  * @brief   任务3：U8g2 版本的界面（四屏轮播）
  ******************************************************************************
  */
#ifndef APP_U8G2_H
#define APP_U8G2_H

#include <stdint.h>

#define U8G2_PAGE_MS      5000u
#define U8G2_PAGE_COUNT   4

void app_u8g2_init(void);
void app_u8g2_render(uint8_t page, uint32_t page_ms);
void app_u8g2_loop(void);

#endif /* APP_U8G2_H */
