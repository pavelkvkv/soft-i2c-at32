#ifndef _SW_I2C_H_
#define _SW_I2C_H_

#include <stdint.h>
#include "at32f435_437_gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define SW_I2C_CLOCK_HZ     300000
#define SW_I2C_WAIT_TIME    ((uint64_t)((1.0 / SW_I2C_CLOCK_HZ)*1000000)) // 10us 400kHz



#define I2C_READ            0x01
#define READ_CMD            1
#define WRITE_CMD           0

typedef enum
{
    HAL_IO_OPT_SET_SDA_LOW = 0,
    HAL_IO_OPT_SET_SDA_HIGH,
    HAL_IO_OPT_SET_SCL_LOW,
    HAL_IO_OPT_SET_SCL_HIGH,
    HAL_IO_OPT_SET_SDA_INPUT,
    HAL_IO_OPT_SET_SDA_OUTPUT,
    HAL_IO_OPT_SET_SCL_INPUT,
    HAL_IO_OPT_SET_SCL_OUTPUT,
    HAL_IO_OPT_GET_SDA_LEVEL,
    HAL_IO_OPT_GET_SCL_LEVEL,
    HAL_IO_OPT_IS_LINE_BUSY,
}hal_io_opt_e;

typedef struct sw_i2c_s 
{
    int (*hal_init)(void * slot);
    int (*hal_deinit)(void * slot);
    int (*hal_io_ctl)(hal_io_opt_e opt, void * slot);
    void (*hal_delay_us)(uint32_t us);
    gpio_type * scl_port;
    gpio_type * sda_port;
    uint32_t scl_pin;
    uint32_t sda_pin;
    SemaphoreHandle_t i2c_sem;
} sw_i2c_t;


/* functions */
void SW_I2C_initial(sw_i2c_t *d);
void SW_I2C_deinit(sw_i2c_t *d);
uint8_t SW_I2C_Read_8addr(sw_i2c_t *d, uint8_t IICID, uint8_t regaddr, uint8_t *pdata, uint8_t rcnt);
uint8_t SW_I2C_Read_16addr(sw_i2c_t *d, uint8_t IICID, uint16_t regaddr, uint8_t *pdata, uint8_t rcnt);
uint8_t SW_I2C_Read_Noaddr(sw_i2c_t *d, uint8_t IICID, uint8_t *pdata, uint8_t rcnt);
uint8_t SW_I2C_Write_8addr(sw_i2c_t *d, uint8_t IICID, uint8_t regaddr, const uint8_t *pdata, uint8_t rcnt);
uint8_t SW_I2C_Write_16addr(sw_i2c_t *d, uint8_t IICID, uint16_t regaddr, const uint8_t *pdata, uint8_t rcnt);
uint8_t SW_I2C_Check_SlaveAddr(sw_i2c_t *d, uint8_t IICID);


#endif  /* __I2C_SW_H */
