/* 软件 I2C 巡线模块驱动 */
#include "tb_line_sensor.h"

static void line_i2c_delay(void)
{
    volatile u8 i;

    for (i = 0; i < 8U; i++)
    {
        __NOP();
    }
}

static void line_sda_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(LINE_SDA_GPIO_PORT, LINE_SDA_PIN, state);
}

static void line_scl_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(LINE_SCL_GPIO_PORT, LINE_SCL_PIN, state);
}

static u8 line_sda_read(void)
{
    return (u8)HAL_GPIO_ReadPin(LINE_SDA_GPIO_PORT, LINE_SDA_PIN);
}

static void line_i2c_start(void)
{
    line_sda_write(GPIO_PIN_SET);
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();
    line_sda_write(GPIO_PIN_RESET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_RESET);
    line_i2c_delay();
}

static void line_i2c_stop(void)
{
    line_sda_write(GPIO_PIN_RESET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();
    line_sda_write(GPIO_PIN_SET);
    line_i2c_delay();
}

static u8 line_i2c_write_byte(u8 value)
{
    u8 i;

    for (i = 0; i < 8U; i++)
    {
        if ((value & 0x80U) != 0U)
        {
            line_sda_write(GPIO_PIN_SET);
        }
        else
        {
            line_sda_write(GPIO_PIN_RESET);
        }

        line_i2c_delay();
        line_scl_write(GPIO_PIN_SET);
        line_i2c_delay();
        line_scl_write(GPIO_PIN_RESET);
        value <<= 1;
    }

    line_sda_write(GPIO_PIN_SET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();
    i = (line_sda_read() == GPIO_PIN_RESET) ? 1U : 0U;
    line_scl_write(GPIO_PIN_RESET);

    return i;
}

static u8 line_i2c_read_byte(u8 ack)
{
    u8 i;
    u8 value = 0U;

    line_sda_write(GPIO_PIN_SET);

    for (i = 0; i < 8U; i++)
    {
        value <<= 1;
        line_scl_write(GPIO_PIN_SET);
        line_i2c_delay();

        if (line_sda_read() == GPIO_PIN_SET)
        {
            value |= 0x01U;
        }

        line_scl_write(GPIO_PIN_RESET);
        line_i2c_delay();
    }

    line_sda_write((ack != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_RESET);
    line_sda_write(GPIO_PIN_SET);

    return value;
}

void tb_line_sensor_init(void)
{
    line_sda_write(GPIO_PIN_SET);
    line_scl_write(GPIO_PIN_SET);
}

HAL_StatusTypeDef LineSensor_SetAdjustMode(u8 enable)
{
    line_i2c_start();

    if (line_i2c_write_byte((u8)(LINE_SENSOR_I2C_ADDR_7BIT << 1)) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte(LINE_SENSOR_REG_ADJUST_MODE) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte((enable != 0U) ? 1U : 0U) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    line_i2c_stop();
    return HAL_OK;
}

HAL_StatusTypeDef LineSensor_Read(LineSensorData_t *data)
{
    u8 raw;
    u8 i;

    if (data == 0)
    {
        return HAL_ERROR;
    }

    line_i2c_start();
    if (line_i2c_write_byte((u8)(LINE_SENSOR_I2C_ADDR_7BIT << 1)) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte(LINE_SENSOR_REG_DATA) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    line_i2c_start();
    if (line_i2c_write_byte((u8)((LINE_SENSOR_I2C_ADDR_7BIT << 1) | 0x01U)) == 0U)
    {
        line_i2c_stop();
        return HAL_ERROR;
    }

    raw = line_i2c_read_byte(0U);
    line_i2c_stop();

    data->raw_byte = raw;
    for (i = 0; i < 8U; i++)
    {
        data->bit[i] = (u8)((raw >> (7U - i)) & 0x01U);
    }

    return HAL_OK;
}
