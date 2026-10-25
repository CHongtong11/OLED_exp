/**
  ******************************************************************************
  * @file    aht20.h
  * @brief   AHT20 temperature / humidity sensor on the same I2C bus.
  ******************************************************************************
  */
#ifndef AHT20_H
#define AHT20_H

#include <stdint.h>
#include "main.h"

/* Defined in Core/Src/main.c (the CubeMX generated project declares it in
 * i2c.c/main.c; keep one definition only). */
extern I2C_HandleTypeDef hi2c1;

#define AHT20_I2C_ADDR   (0x38 << 1)

/* Returns 0 when the sensor answered. */
uint8_t aht20_init(void);

/* Temperature in 0.1 degC, humidity in 0.1 %RH.  Returns 0 on success. */
uint8_t aht20_read(int16_t *temp_x10, uint16_t *humi_x10);

#endif /* AHT20_H */
