#include <stdlib.h>
#define TAG "SW_I2C"
#include "log.h"
#include "sw_i2c.h"

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

/**
 * @brief Initialize the I2C bus.
 *
 * Checks if the line is busy and initializes the hardware abstraction layer.
 *
 * @param[in] d Pointer to the I2C instance.
 */
void SW_I2C_initial(sw_i2c_t *d)
{
	if (d)
	{
		if (d->hal_io_ctl(HAL_IO_OPT_IS_LINE_BUSY, d) == TRUE)
		{
			logE("line detected busy (port %d, pin %d)", d->scl_port, d->scl_pin);
		}
		d->hal_init(d);
	}
}

/**
 * @brief Deinitialize the I2C bus.
 *
 * Releases hardware resources.
 *
 * @param[in] d Pointer to the I2C instance.
 */
void SW_I2C_deinit(sw_i2c_t *d)
{
	if (d)
	{
		d->hal_deinit(d);
	}
}

static void sda_out(sw_i2c_t *d, uint8_t out)
{
    if(out)
        d->hal_io_ctl(HAL_IO_OPT_SET_SDA_HIGH, d);
    else
        d->hal_io_ctl(HAL_IO_OPT_SET_SDA_LOW, d);
}

static void i2c_clk_data_out(sw_i2c_t *d)
{
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
}

static void i2c_port_initial(sw_i2c_t *d)
{
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_HIGH, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    portEXIT_CRITICAL();
}


static uint8_t SW_I2C_ReadVal_SDA(sw_i2c_t *d)
{
    
    return d->hal_io_ctl(HAL_IO_OPT_GET_SDA_LEVEL, d);
}


static void i2c_start_condition(sw_i2c_t *d)
{
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_HIGH, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    portEXIT_CRITICAL();
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_LOW, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME << 1);
}

static void i2c_stop_condition(sw_i2c_t *d)
{
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_LOW, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    portEXIT_CRITICAL();
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_HIGH, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME);
}

static uint8_t i2c_check_ack(sw_i2c_t *d)
{
    uint8_t ack;
    int i;
    unsigned int temp;
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_INPUT, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    portEXIT_CRITICAL();
    ack = 0;
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    for (i = 10; i > 0; i--)
    {
        temp = !(SW_I2C_ReadVal_SDA(d));
        if (temp)
        {
            ack = 1;
            break;
        }
    }
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_OUTPUT, d);
    portEXIT_CRITICAL();
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    return ack;
}

static void i2c_check_not_ack(sw_i2c_t *d)
{
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_INPUT, d);
    i2c_clk_data_out(d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_OUTPUT, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME);
}

static void i2c_slave_address(sw_i2c_t *d, uint8_t IICID, uint8_t readwrite)
{
    int x;

    if (readwrite)
    {
        IICID |= I2C_READ;
    }
    else
    {
        IICID &= ~I2C_READ;
    }

    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
    for (x = 7; x >= 0; x--)
    {
        sda_out(d, IICID & (1 << x));
        d->hal_delay_us(SW_I2C_WAIT_TIME);
        i2c_clk_data_out(d);

    }
}

static void i2c_register_address(sw_i2c_t *d, uint8_t addr)
{
    int x;

    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);

    for (x = 7; x >= 0; x--)
    {
        sda_out(d, addr & (1 << x));
        d->hal_delay_us(SW_I2C_WAIT_TIME);
        i2c_clk_data_out(d);
    }
}

static void i2c_send_ack(sw_i2c_t *d)
{
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_OUTPUT, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_LOW, d);
    portEXIT_CRITICAL();
    d->hal_delay_us(SW_I2C_WAIT_TIME);
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME << 1);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_LOW, d);
    d->hal_delay_us(SW_I2C_WAIT_TIME << 1);
    portENTER_CRITICAL();
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_OUTPUT, d);
    portEXIT_CRITICAL();
    d->hal_delay_us(SW_I2C_WAIT_TIME);
}

static void SW_I2C_Write_Data(sw_i2c_t *d, uint8_t data)
{
    int x;
    d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
    for (x = 7; x >= 0; x--)
    {
        sda_out(d, data & (1 << x));
        d->hal_delay_us(SW_I2C_WAIT_TIME);
        i2c_clk_data_out(d);
    }
}

static uint8_t SW_I2C_Read_Data(sw_i2c_t *d)
{
    int x;
    uint8_t readdata = 0;
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_INPUT, d);
    for (x = 8; x--;)
    {
        d->hal_io_ctl(HAL_IO_OPT_SET_SCL_HIGH, d);
        readdata <<= 1;
        if (SW_I2C_ReadVal_SDA(d))
            readdata |= 0x01;
        d->hal_delay_us(SW_I2C_WAIT_TIME);
        d->hal_io_ctl(HAL_IO_OPT_SET_SCL_LOW, d);
        d->hal_delay_us(SW_I2C_WAIT_TIME);
    }
    d->hal_io_ctl(HAL_IO_OPT_SET_SDA_OUTPUT, d);
    return readdata;
}

/**
 * @brief Read from I2C device with 8-bit register address.
 *
 * Performs a read operation using an 8-bit register address.
 *
 * @param[in] d Pointer to the I2C instance.
 * @param[in] IICID I2C device address.
 * @param[in] regaddr Register address.
 * @param[out] pdata Pointer to buffer for storing data.
 * @param[in] rcnt Number of bytes to read.
 * @return TRUE if successful, FALSE otherwise.
 */
uint8_t SW_I2C_Read_8addr(sw_i2c_t *d, uint8_t IICID, uint8_t regaddr, uint8_t *pdata, uint8_t rcnt)
{
	uint8_t returnack = TRUE;

	if (d == NULL || pdata == NULL || rcnt == 0)
		return FALSE;

	if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
	{
		i2c_port_initial(d);
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, WRITE_CMD);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		d->hal_delay_us(SW_I2C_WAIT_TIME);
		i2c_register_address(d, regaddr);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		d->hal_delay_us(SW_I2C_WAIT_TIME);
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, READ_CMD);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		for (uint8_t i = 0; i < rcnt - 1; i++)
		{
			pdata[i] = SW_I2C_Read_Data(d);
			i2c_send_ack(d);
		}

		pdata[rcnt - 1] = SW_I2C_Read_Data(d);
		i2c_check_not_ack(d);
		i2c_stop_condition(d);

		xSemaphoreGive(d->i2c_sem);
	}

	return returnack;
}

/**
 * @brief Read from I2C device with 16-bit register address.
 *
 * Performs a read operation using a 16-bit register address.
 *
 * @param[in] d Pointer to the I2C instance.
 * @param[in] IICID I2C device address.
 * @param[in] regaddr 16-bit register address.
 * @param[out] pdata Pointer to buffer for storing data.
 * @param[in] rcnt Number of bytes to read.
 * @return TRUE if successful, FALSE otherwise.
 */
uint8_t SW_I2C_Read_16addr(sw_i2c_t *d, uint8_t IICID, uint16_t regaddr, uint8_t *pdata, uint8_t rcnt)
{
	uint8_t returnack = TRUE;

	if (d == NULL || pdata == NULL || rcnt == 0)
		return FALSE;

	if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
	{
		i2c_port_initial(d);
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, WRITE_CMD);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		d->hal_delay_us(SW_I2C_WAIT_TIME);
		i2c_register_address(d, (uint8_t)(regaddr >> 8));
		if (!i2c_check_ack(d))
			returnack = FALSE;

		d->hal_delay_us(SW_I2C_WAIT_TIME);
		i2c_register_address(d, (uint8_t)regaddr);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		d->hal_delay_us(SW_I2C_WAIT_TIME);
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, READ_CMD);
		if (!i2c_check_ack(d))
			returnack = FALSE;

		for (uint8_t i = 0; i < rcnt - 1; i++)
		{
			pdata[i] = SW_I2C_Read_Data(d);
			i2c_send_ack(d);
		}

		pdata[rcnt - 1] = SW_I2C_Read_Data(d);
		i2c_check_not_ack(d);
		i2c_stop_condition(d);

		xSemaphoreGive(d->i2c_sem);
	}

	return returnack;
}

/**
 * Чтение без указания адреса, например если прошлая команда уже установила адрес
 * Используется для DS2482
 */
uint8_t SW_I2C_Read_Noaddr(sw_i2c_t *d, uint8_t IICID, uint8_t *pdata, uint8_t rcnt)
{
	uint8_t returnack = TRUE;
	if (d == NULL) return FALSE;
	if (!rcnt) return FALSE;

	if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
	{
		i2c_port_initial(d);
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, READ_CMD);
		if (!i2c_check_ack(d))
		{
			returnack = FALSE;
		}
		if (rcnt > 1)
		{
			for (uint8_t index = 0; index < (rcnt - 1); index++)
			{
				d->hal_delay_us(SW_I2C_WAIT_TIME);
				pdata[index] = SW_I2C_Read_Data(d);
				i2c_send_ack(d);
			}
		}
		d->hal_delay_us(SW_I2C_WAIT_TIME);
		pdata[rcnt - 1] = SW_I2C_Read_Data(d);
		i2c_check_not_ack(d);
		i2c_stop_condition(d);

		xSemaphoreGive(d->i2c_sem);
	}

	return returnack;
}

/**
 * @brief Write to I2C device with 8-bit register address.
 *
 * Performs a write operation using an 8-bit register address.
 *
 * @param[in] d Pointer to the I2C instance.
 * @param[in] IICID I2C device address.
 * @param[in] regaddr Register address.
 * @param[in] pdata Pointer to buffer containing data to write.
 * @param[in] rcnt Number of bytes to write.
 * @return TRUE if successful, FALSE otherwise.
 */
uint8_t SW_I2C_Write_8addr(sw_i2c_t *d, uint8_t IICID, uint8_t regaddr, const uint8_t *pdata, uint8_t rcnt)
{
    uint8_t returnack = TRUE;

    if (d == NULL || (pdata == NULL && rcnt != 0)) 
        return FALSE;

    if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
    {
        i2c_port_initial(d);
        i2c_start_condition(d);
        i2c_slave_address(d, IICID, WRITE_CMD);
        if (!i2c_check_ack(d)) 
            returnack = FALSE;

        d->hal_delay_us(SW_I2C_WAIT_TIME);
        i2c_register_address(d, regaddr);
        if (!i2c_check_ack(d)) 
            returnack = FALSE;

        d->hal_delay_us(SW_I2C_WAIT_TIME);
        for (uint8_t i = 0; i < rcnt; i++)
        {
            SW_I2C_Write_Data(d, pdata[i]);
            if (!i2c_check_ack(d)) 
                returnack = FALSE;

            d->hal_delay_us(SW_I2C_WAIT_TIME);
        }

        i2c_stop_condition(d);
        xSemaphoreGive(d->i2c_sem);
    }

    return returnack;
}

uint8_t SW_I2C_Write_16addr(sw_i2c_t *d, uint8_t IICID, uint16_t regaddr, const uint8_t *pdata, uint8_t rcnt)
{
	uint8_t returnack = TRUE;
	uint8_t index;

	if (d == NULL || (pdata == NULL && rcnt != 0))  return FALSE;

	if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
	{
		i2c_port_initial(d);
		i2c_start_condition(d);
		// 写ID
		i2c_slave_address(d, IICID, WRITE_CMD);
		if (!i2c_check_ack(d)) { returnack = FALSE; }
		d->hal_delay_us(SW_I2C_WAIT_TIME);
		// 写高八位地址
		i2c_register_address(d, (uint8_t)(regaddr >> 8));
		if (!i2c_check_ack(d)) { returnack = FALSE; }
		d->hal_delay_us(SW_I2C_WAIT_TIME);
		// 写低八位地址
		i2c_register_address(d, (uint8_t)regaddr);
		if (!i2c_check_ack(d)) { returnack = FALSE; }
		d->hal_delay_us(SW_I2C_WAIT_TIME);
		// 写数据
		for (index = 0; index < rcnt; index++)
		{
			SW_I2C_Write_Data(d, pdata[index]);
			if (!i2c_check_ack(d)) { returnack = FALSE; }
			d->hal_delay_us(SW_I2C_WAIT_TIME);
		}
		i2c_stop_condition(d);
		xSemaphoreGive(d->i2c_sem);
	}
	return returnack;
}

uint8_t SW_I2C_Check_SlaveAddr(sw_i2c_t *d, uint8_t IICID)
{
    uint8_t returnack = TRUE;

	if (xSemaphoreTake(d->i2c_sem, portMAX_DELAY) == pdTRUE)
	{
		i2c_start_condition(d);
		i2c_slave_address(d, IICID, WRITE_CMD);
		if (!i2c_check_ack(d))
		{
			i2c_stop_condition(d);
			returnack = FALSE;
		}
		i2c_stop_condition(d);

		xSemaphoreGive(d->i2c_sem);
	}
	return returnack;
}
