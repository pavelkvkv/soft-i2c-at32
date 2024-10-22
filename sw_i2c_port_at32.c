
#include "sw_i2c.h"

// #include "stm32f1xx_hal.h"
#include "at32f435_437_gpio.h"

#define SW_I2C0_SCL_PORT    GPIOA
#define SW_I2C0_SDA_PORT    GPIOA
#define SW_I2C0_SCL_PIN     GPIO_PINS_15
#define SW_I2C0_SDA_PIN     GPIO_PINS_14

#define SW_I2C1_SCL_PORT    GPIOA
#define SW_I2C1_SDA_PORT    GPIOA
#define SW_I2C1_SCL_PIN     GPIO_PINS_10
#define SW_I2C1_SDA_PIN     GPIO_PINS_9


static void sda_in_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_mode = GPIO_MODE_INPUT,
        .gpio_pull = GPIO_PULL_UP,
    };
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_init(bus->sda_port, &GPIO_InitStruct);
}

static void sda_out_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE,
    };
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_init(bus->sda_port, &GPIO_InitStruct);
}

static void scl_in_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        //.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_INPUT,
        .gpio_pull = GPIO_PULL_UP,
    };
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
}

static void scl_out_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE,
    };
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
}

static int sw_i2c_port_initial(void * arg)
{
    sw_i2c_t * bus = arg;
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_NONE
    };

    // i2c_sw SCL
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
    // i2c_sw SDA
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_init(bus->sda_port, &GPIO_InitStruct);
    return 0;
}

static int sw_i2c_port_deinit(void * arg)
{
    sw_i2c_t * bus = arg;
    gpio_init_type GPIO_InitStruct = {0};

    // i2c_sw SCL
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
    // i2c_sw SDA
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_init(bus->sda_port, &GPIO_InitStruct);
    return 0;
}

static void sw_i2c_port_delay_us(uint32_t us)
{
    // TODO: сделать таймерную задержку
    uint32_t nCount = us/10*25;
    for (; nCount != 0; nCount--);
}

static int sw_i2c_port_io_ctl(uint8_t opt, void * arg)
{
    sw_i2c_t * bus = arg;
    int ret = -1;
    switch (opt)
    {
    case HAL_IO_OPT_SET_SDA_HIGH:
        gpio_bits_set(bus->sda_port, bus->sda_pin);
        break;
    case HAL_IO_OPT_SET_SDA_LOW:
        gpio_bits_reset(bus->sda_port, bus->sda_pin);
        break;
    case HAL_IO_OPT_GET_SDA_LEVEL:
        ret = gpio_input_data_bit_read(bus->sda_port, bus->sda_pin);
        break;
    case HAL_IO_OPT_SET_SDA_INPUT:
        sda_in_mode(bus);
        break;
    case HAL_IO_OPT_SET_SDA_OUTPUT:
        sda_out_mode(bus);
        break;
    case HAL_IO_OPT_SET_SCL_HIGH:
        gpio_bits_set(bus->scl_port, bus->scl_pin);
        break;
    case HAL_IO_OPT_SET_SCL_LOW:
        gpio_bits_reset(bus->scl_port, bus->scl_pin);
        break;
    case HAL_IO_OPT_GET_SCL_LEVEL:
        ret = gpio_input_data_bit_read(bus->scl_port, bus->scl_pin);
        break;
    case HAL_IO_OPT_SET_SCL_INPUT:
        scl_in_mode(bus);
        break;
    case HAL_IO_OPT_SET_SCL_OUTPUT:
        scl_out_mode(bus);
        break;
    default:
        break;
    }
    return ret;
}


sw_i2c_t i2c_bus0 = 
{
    .hal_init = sw_i2c_port_initial,
    .hal_deinit = sw_i2c_port_deinit,
    .hal_io_ctl = sw_i2c_port_io_ctl,
    .hal_delay_us = sw_i2c_port_delay_us,

    .scl_pin = SW_I2C0_SCL_PIN,
    .scl_port = SW_I2C0_SCL_PORT,
    .sda_pin = SW_I2C0_SDA_PIN,
    .sda_port = SW_I2C0_SDA_PORT
},
i2c_bus1 =
{
    .hal_init = sw_i2c_port_initial,
    .hal_deinit = sw_i2c_port_deinit,
    .hal_io_ctl = sw_i2c_port_io_ctl,
    .hal_delay_us = sw_i2c_port_delay_us,

    .scl_pin = SW_I2C1_SCL_PIN,
    .scl_port = SW_I2C1_SCL_PORT,
    .sda_pin = SW_I2C1_SDA_PIN,
    .sda_port = SW_I2C1_SDA_PORT
};


