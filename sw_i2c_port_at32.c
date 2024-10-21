
#include "sw_i2c.h"

// #include "stm32f1xx_hal.h"
#include "at32f435_437_gpio.h"

#define SW_I2C1_SCL_PORT    GPIOA
#define SW_I2C1_SDA_PORT    GPIOA
#define SW_I2C1_SCL_PIN     GPIO_PIN_15
#define SW_I2C1_SDA_PIN     GPIO_PIN_14


static void GPIO_SetBits(gpio_type *GPIOx, uint16_t GPIO_Pin)
{
    gpio_bits_set(GPIOx, GPIO_Pin);
}

static void GPIO_ResetBits(gpio_type *GPIOx, uint16_t GPIO_Pin)
{
	gpio_bits_reset(GPIOx, GPIO_Pin);
}

static uint8_t GPIO_ReadInputDataBit(gpio_type *GPIOx, uint16_t GPIO_Pin)
{
	return (uint8_t)gpio_input_data_bit_read(GPIOx, GPIO_Pin);
}

static void sda_in_mode(void)
{
    static gpio_init_type GPIO_InitStruct = 
    {
        //.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        //.gpio_out_type = GPIO_OUTPUT_PUSH_PULL,
        .gpio_mode = GPIO_MODE_INPUT,
        .gpio_pull = GPIO_PULL_UP,
        .gpio_pins = SW_I2C1_SDA_PIN
    };
    gpio_init(SW_I2C1_SDA_PORT, &GPIO_InitStruct);
}

static void sda_out_mode(void)
{
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE,
        .gpio_pins = SW_I2C1_SDA_PIN
    };
    gpio_init(SW_I2C1_SDA_PORT, &GPIO_InitStruct);
}

static void scl_in_mode(void)
{
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        //.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_INPUT,
        .gpio_pull = GPIO_PULL_UP,
        .gpio_pins = SW_I2C1_SCL_PIN
    };
    gpio_init(SW_I2C1_SCL_PORT, &GPIO_InitStruct);
}

static void scl_out_mode(void)
{
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE,
        .gpio_pins = SW_I2C1_SCL_PIN
    };
    gpio_init(SW_I2C1_SCL_PORT, &GPIO_InitStruct);
}

static int sw_i2c_port_initial(void)
{
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE
    };

    // i2c_sw SCL
    GPIO_InitStruct.Pin = SW_I2C1_SCL_PIN;
    gpio_init(SW_I2C1_SCL_PORT, &GPIO_InitStruct);
    // i2c_sw SDA
    GPIO_InitStruct.Pin = SW_I2C1_SDA_PIN;
    gpio_init(SW_I2C1_SDA_PORT, &GPIO_InitStruct);
    return 0;
}

static void sw_i2c_port_delay_us(uint32_t us)
{
    // TODO: сделать таймерную задержку
    uint32_t nCount = us/10*25;
    for (; nCount != 0; nCount--);
}

static int sw_i2c_port_io_ctl(uint8_t opt, void *param)
{
    int ret = -1;
    switch (opt)
    {
    case HAL_IO_OPT_SET_SDA_HIGH:
        gpio_bits_set(SW_I2C1_SDA_PORT, SW_I2C1_SDA_PIN);
        break;
    case HAL_IO_OPT_SET_SDA_LOW:
        gpio_bits_reset(SW_I2C1_SDA_PORT, SW_I2C1_SDA_PIN);
        break;
    case HAL_IO_OPT_GET_SDA_LEVEL:
        ret = gpio_input_data_bit_read(SW_I2C1_SDA_PORT, SW_I2C1_SDA_PIN);
        break;
    case HAL_IO_OPT_SET_SDA_INPUT:
        sda_in_mode();
        break;
    case HAL_IO_OPT_SET_SDA_OUTPUT:
        sda_out_mode();
        break;
    case HAL_IO_OPT_SET_SCL_HIGH:
        gpio_bits_set(SW_I2C1_SCL_PORT, SW_I2C1_SCL_PIN);
        break;
    case HAL_IO_OPT_SET_SCL_LOW:
        gpio_bits_reset(SW_I2C1_SCL_PORT, SW_I2C1_SCL_PIN);
        break;
    case HAL_IO_OPT_GET_SCL_LEVEL:
        ret = gpio_input_data_bit_read(SW_I2C1_SCL_PORT, SW_I2C1_SCL_PIN);
        break;
    case HAL_IO_OPT_SET_SCL_INPUT:
        scl_in_mode();
        break;
    case HAL_IO_OPT_SET_SCL_OUTPUT:
        scl_out_mode();
        break;
    default:
        break;
    }
    return ret;
}


sw_i2c_t sw_i2c_at32_generic = {
    .hal_init = sw_i2c_port_initial,
    .hal_io_ctl = sw_i2c_port_io_ctl,
    .hal_delay_us = sw_i2c_port_delay_us,
    };
