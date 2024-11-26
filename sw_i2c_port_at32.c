
/***
 * Software I2C port for Artery AT32 MCUs
 */

#include "sw_i2c.h"
#include <stdbool.h>
#include "at32f435_437_gpio.h"

#define TAG "I2Cport"
#include "log.h"

/**
 * Macroses
 */
#define SW_I2C0_SCL_PORT    GPIOA
#define SW_I2C0_SDA_PORT    GPIOA
#define SW_I2C0_SCL_PIN     GPIO_PINS_15
#define SW_I2C0_SDA_PIN     GPIO_PINS_14

#define SW_I2C1_SCL_PORT    GPIOA
#define SW_I2C1_SDA_PORT    GPIOA
#define SW_I2C1_SCL_PIN     GPIO_PINS_10
#define SW_I2C1_SDA_PIN     GPIO_PINS_9

/**
 * Static functions prototypes
 */
static bool is_line_busy(void * arg);
static void sda_in_mode(void * arg);
static void sda_out_mode(void * arg);
static void scl_in_mode(void * arg);
static void scl_out_mode(void * arg);
static int sw_i2c_port_initial(void * arg);
static int sw_i2c_port_deinit(void * arg);
static void sw_i2c_port_delay_us(uint32_t us);
static int sw_i2c_port_io_ctl(uint8_t opt, void * param);


/**
 * Static functions
 */

/**
 * Check if I2C line is busy (pulling SCL and SDA low)
 * @param arg pointer to sw_i2c_t
 * @return true if line is busy
 */
static bool is_line_busy(void * arg)
{
    sw_i2c_t * bus = arg;
    gpio_init_type GPIO_InitStruct = 
    {
        .gpio_mode = GPIO_MODE_INPUT,
        .gpio_pull = GPIO_PULL_UP,
    };
    GPIO_InitStruct.gpio_pins = bus->scl_pin | bus->sda_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
    gpio_pin_mux_config(bus->scl_port, bus->scl_pin, GPIO_MUX_0);
    gpio_pin_mux_config(bus->sda_port, bus->sda_pin, GPIO_MUX_0);
    for (volatile int i = 0; i < 10000; i++); // waiting for charging traces
    return (gpio_input_data_bit_read(bus->scl_port, bus->scl_pin) & gpio_input_data_bit_read(bus->sda_port, bus->sda_pin)) ?
        false : true;
}

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
    //logD("Port %d pin %d", bus->sda_port, bus->sda_pin);
}

static void sda_out_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_UP,
    };
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_bits_set(bus->sda_port, bus->sda_pin); // remove possible low
    gpio_init(bus->sda_port, &GPIO_InitStruct);
    //logD("Port %d pin %d", bus->sda_port, bus->sda_pin);
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
    //logD("Port %d pin %d", bus->sda_port, bus->sda_pin);
}

static void scl_out_mode(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_UP,
    };
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_bits_set(bus->scl_port, bus->scl_pin); // remove possible low
    gpio_init(bus->scl_port, &GPIO_InitStruct);
    //logD("Port %d pin %d", bus->sda_port, bus->sda_pin);
}

static int sw_i2c_port_initial(void * arg)
{
    sw_i2c_t * bus = arg;
    static gpio_init_type GPIO_InitStruct = 
    {
        .gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER,
        .gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN,
        .gpio_mode = GPIO_MODE_OUTPUT,
        .gpio_pull = GPIO_PULL_UP
    };

    // i2c_sw SCL
    GPIO_InitStruct.gpio_pins = bus->scl_pin;
    gpio_init(bus->scl_port, &GPIO_InitStruct);
    // i2c_sw SDA
    GPIO_InitStruct.gpio_pins = bus->sda_pin;
    gpio_init(bus->sda_port, &GPIO_InitStruct);
    gpio_pin_mux_config(bus->scl_port, bus->scl_pin, GPIO_MUX_0);
    gpio_pin_mux_config(bus->sda_port, bus->sda_pin, GPIO_MUX_0);

    if(bus->i2c_sem == NULL)
        bus->i2c_sem = xSemaphoreCreateMutex();
    xSemaphoreGive(bus->i2c_sem);

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

    if(bus->i2c_sem != NULL)
        vSemaphoreDelete(bus->i2c_sem);
    return 0;
}


#define DWT_BASE       (0xE0001000UL)    /**< Базовый адрес DWT */
#define DWT_CYCCNT     (*(volatile uint32_t *)(DWT_BASE + 0x04)) /**< Регистр CYCCNT */
#define DWT_CONTROL    (*(volatile uint32_t *)(DWT_BASE)) /**< Регистр управления DWT */
#define SCB_DEMCR      (*(volatile uint32_t *)0xE000EDFC) /**< Регистр управления и отслеживания (Debug Exception and Monitor Control Register) */

static void sw_i2c_port_delay_us(uint32_t us)
{

    if(us % 1000 == 0)
    {
        u32 ms = us / 1000;
        vTaskDelay(ms);
    }
    else
	{
		uint32_t cpu_freq_mhz = SystemCoreClock / 1000000;

		// Включаем DWT и CYCCNT
		SCB_DEMCR |= 0x01000000; // Разрешаем использование DWT
		DWT_CONTROL |= 1;		 // Включаем CYCCNT

		uint32_t start_ticks = DWT_CYCCNT;
		uint32_t delay_ticks = us * cpu_freq_mhz;

		// Ждем, пока не пройдет нужное количество тактов
		while ((DWT_CYCCNT - start_ticks) < delay_ticks);

		// Отключаем CYCCNT для экономии энергии (по желанию)
		DWT_CONTROL &= ~1;
	}
}

static int sw_i2c_port_io_ctl(uint8_t opt, void * arg)
{
    sw_i2c_t * bus = arg;
    int ret = -1;
    switch (opt)
    {
    case HAL_IO_OPT_SET_SDA_HIGH:
        gpio_bits_set(bus->sda_port, bus->sda_pin);
        //logD("SDA HI, Port %d pin %d", bus->sda_port, bus->sda_pin);
        break;
    case HAL_IO_OPT_SET_SDA_LOW:
        gpio_bits_reset(bus->sda_port, bus->sda_pin);
        //logD("SDA LO, Port %d pin %d", bus->sda_port, bus->sda_pin);
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
        //logD("SCL HI, Port %d pin %d", bus->scl_port, bus->scl_pin);
        break;
    case HAL_IO_OPT_SET_SCL_LOW:
        gpio_bits_reset(bus->scl_port, bus->scl_pin);
        //logD("SCL LO, Port %d pin %d", bus->scl_port, bus->scl_pin);
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
    case HAL_IO_OPT_IS_LINE_BUSY:
        ret = is_line_busy(bus);
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
    .sda_port = SW_I2C0_SDA_PORT,
    .i2c_sem = NULL
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
    .sda_port = SW_I2C1_SDA_PORT,
    .i2c_sem = NULL
};


