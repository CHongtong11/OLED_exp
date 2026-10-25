/**
  ******************************************************************************
  * @file    aht20.c
  * @brief   AHT20 driver.
  *
  * Protocol (datasheet):
  *   power on   : wait >= 40 ms, read status byte, if bit3 (CAL) is 0 send
  *                0xBE 0x08 0x00 to calibrate
  *   measurement: send 0xAC 0x33 0x00, wait >= 75 ms, read 7 bytes:
  *                [0] status, [1..3] humidity 20 bit, [3..5] temperature 20 bit,
  *                [6] CRC8
  *   humidity = raw / 2^20 * 100 %RH
  *   temp     = raw / 2^20 * 200 - 50 degC
  ******************************************************************************
  */
#include "aht20.h"

static uint8_t aht20_send(const uint8_t *cmd, uint16_t len)
{
    return (HAL_I2C_Master_Transmit(&hi2c1, AHT20_I2C_ADDR, (uint8_t *)cmd,
                                    len, 100) == HAL_OK) ? 0u : 1u;
}

uint8_t aht20_init(void)
{
    static const uint8_t calibrate[3] = { 0xBE, 0x08, 0x00 };
    uint8_t status = 0;

    HAL_Delay(50);
    if (HAL_I2C_Master_Receive(&hi2c1, AHT20_I2C_ADDR, &status, 1, 100) != HAL_OK) {
        return 1;
    }
    if ((status & 0x08u) == 0u) {           /* not calibrated yet */
        if (aht20_send(calibrate, 3) != 0u) {
            return 2;
        }
        HAL_Delay(20);
    }
    return 0;
}

uint8_t aht20_read(int16_t *temp_x10, uint16_t *humi_x10)
{
    static const uint8_t trigger[3] = { 0xAC, 0x33, 0x00 };
    uint8_t buf[7];
    uint32_t raw_humi;
    uint32_t raw_temp;
    float humi;
    float temp;

    if (aht20_send(trigger, 3) != 0u) {
        return 1;
    }
    HAL_Delay(80);                          /* measurement takes ~75 ms */

    if (HAL_I2C_Master_Receive(&hi2c1, AHT20_I2C_ADDR, buf, 7, 100) != HAL_OK) {
        return 2;
    }
    if ((buf[0] & 0x80u) != 0u) {           /* still busy */
        return 3;
    }

    raw_humi = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) |
               ((uint32_t)buf[3] >> 4);
    raw_temp = (((uint32_t)buf[3] & 0x0Fu) << 16) |
               ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];

    humi = (float)raw_humi * 100.0f / 1048576.0f;
    temp = (float)raw_temp * 200.0f / 1048576.0f - 50.0f;

    *humi_x10 = (uint16_t)(humi * 10.0f + 0.5f);
    *temp_x10 = (int16_t)(temp * 10.0f + ((temp >= 0.0f) ? 0.5f : -0.5f));
    return 0;
}
