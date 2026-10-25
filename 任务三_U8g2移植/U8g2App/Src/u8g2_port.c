/**
  ******************************************************************************
  * @file    u8g2_port.c
  * @brief   U8g2 与 STM32F103 的接口层：软件 I2C（PB6 = SCL, PB7 = SDA）。
  *
  *  为什么用软件 I2C：本机 Keil 是 MDK-Lite（32 KB 代码限制），
  *  HAL 的 stm32f1xx_hal_i2c.c 编进来就要 14 KB；U8g2 本身约 14 KB。
  *  两者相加超过 32 KB，所以这里用 GPIO 模拟 I2C，协议完全一样，
  *  只多约 0.5 KB 代码。
  ******************************************************************************
  */
#include "u8g2_port.h"
#include "main.h"

#define I2C_SCL_PIN     6u          /* PB6 */
#define I2C_SDA_PIN     7u          /* PB7 */

#define SCL_HIGH()      (GPIOB->BSRR = (1u << I2C_SCL_PIN))
#define SCL_LOW()       (GPIOB->BRR  = (1u << I2C_SCL_PIN))
#define SDA_HIGH()      (GPIOB->BSRR = (1u << I2C_SDA_PIN))
#define SDA_LOW()       (GPIOB->BRR  = (1u << I2C_SDA_PIN))

#define U8G2_TX_MAX     (128 + 2)

static void i2c_delay(void)
{
    uint32_t i;

    for (i = 0; i < 6u; i++) {
    }
}

static void i2c_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* PB6 / PB7: output push-pull, 50 MHz (CRL bits 24..31) */
    GPIOB->CRL &= ~(0xFFu << 24);
    GPIOB->CRL |=  (0x33u << 24);

    SCL_HIGH();
    SDA_HIGH();
}

static void i2c_start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    i2c_delay();
    SDA_LOW();
    i2c_delay();
    SCL_LOW();
}

static void i2c_stop(void)
{
    SCL_LOW();
    SDA_LOW();
    i2c_delay();
    SCL_HIGH();
    i2c_delay();
    SDA_HIGH();
    i2c_delay();
}

static void i2c_write_byte(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8u; i++) {
        if (data & 0x80u) {
            SDA_HIGH();
        } else {
            SDA_LOW();
        }
        data = (uint8_t)(data << 1);
        i2c_delay();
        SCL_HIGH();
        i2c_delay();
        SCL_LOW();
    }

    /* release SDA and clock the acknowledge bit (ACK is ignored) */
    SDA_HIGH();
    i2c_delay();
    SCL_HIGH();
    i2c_delay();
    SCL_LOW();
}

static void i2c_send(uint8_t addr, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    i2c_start();
    i2c_write_byte(addr);
    for (i = 0; i < len; i++) {
        i2c_write_byte(data[i]);
    }
    i2c_stop();
}

/* ------------------------------------------------------------------ */
/* U8g2 callbacks                                                     */
/* ------------------------------------------------------------------ */
uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                               void *arg_ptr)
{
    static uint8_t buffer[U8G2_TX_MAX];
    static uint16_t index;
    uint8_t *data;

    switch (msg) {
    case U8X8_MSG_BYTE_SEND:
        data = (uint8_t *)arg_ptr;
        while (arg_int > 0) {
            if (index < U8G2_TX_MAX) {
                buffer[index++] = *data;
            }
            data++;
            arg_int--;
        }
        break;

    case U8X8_MSG_BYTE_INIT:
        i2c_gpio_init();
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        index = 0;
        break;

    case U8X8_MSG_BYTE_END_TRANSFER:
        i2c_send(u8x8_GetI2CAddress(u8x8), buffer, index);
        break;

    default:
        return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int,
                                  void *arg_ptr)
{
    (void)u8x8;
    (void)arg_ptr;

    switch (msg) {
    case U8X8_MSG_DELAY_MILLI:
        HAL_Delay(arg_int);
        break;
    case U8X8_MSG_DELAY_10MICRO:
        for (volatile uint32_t i = 0; i < (uint32_t)arg_int * 72u; i++) {
        }
        break;
    case U8X8_MSG_DELAY_100NANO:
        for (volatile uint32_t i = 0; i < (uint32_t)arg_int; i++) {
        }
        break;
    default:
        break;
    }
    return 1;
}
