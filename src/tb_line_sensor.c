/* 软件 I2C 巡线模块驱动 */
#include "tb_line_sensor.h"
#include "tb_gpio.h"

static LineSensorError_t s_line_sensor_last_error = LINE_SENSOR_ERR_NONE;

static void line_sda_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LINE_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LINE_SDA_GPIO_PORT, &GPIO_InitStruct);
}

static void line_sda_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LINE_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LINE_SDA_GPIO_PORT, &GPIO_InitStruct);
}

static void line_i2c_delay(void)
{
    volatile u16 i;

    for (i = 0; i < LINE_I2C_DELAY_CYCLES; i++)
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
    line_sda_output();
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
    line_sda_output();
    line_sda_write(GPIO_PIN_RESET);
    line_i2c_delay();
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();
    line_sda_write(GPIO_PIN_SET);
    line_i2c_delay();
}

static u8 line_i2c_wait_ack(void)
{
    u16 timeout = 0U;

    line_sda_input();
    line_i2c_delay();
    line_scl_write(GPIO_PIN_SET);
    line_i2c_delay();

    while (line_sda_read() == GPIO_PIN_SET)
    {
        timeout++;
        line_i2c_delay();
        if (timeout > LINE_I2C_ACK_TIMEOUT)
        {
            line_i2c_stop();
            return 0U;
        }
    }

    line_scl_write(GPIO_PIN_RESET);
    line_i2c_delay();
    line_sda_output();
    return 1U;
}

static u8 line_i2c_write_byte(u8 value)
{
    u8 i;

    line_sda_output();
    line_scl_write(GPIO_PIN_RESET);
    line_i2c_delay();

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

    i = line_i2c_wait_ack();
    return i;
}

static u8 line_i2c_read_byte(u8 ack)
{
    u8 i;
    u8 value = 0U;

    line_sda_input();
    line_scl_write(GPIO_PIN_RESET);
    line_i2c_delay();

    for (i = 0; i < 8U; i++)
    {
        value <<= 1;
        line_i2c_delay();
        line_scl_write(GPIO_PIN_SET);
        line_i2c_delay();

        if (line_sda_read() == GPIO_PIN_SET)
        {
            value |= 0x01U;
        }

        line_scl_write(GPIO_PIN_RESET);
        line_i2c_delay();
    }

    line_sda_output();
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
    s_line_sensor_last_error = LINE_SENSOR_ERR_NONE;
    line_sda_output();
    line_sda_write(GPIO_PIN_SET);
    line_scl_write(GPIO_PIN_SET);
}

HAL_StatusTypeDef LineSensor_SetAdjustMode(u8 enable)
{
    s_line_sensor_last_error = LINE_SENSOR_ERR_NONE;
    line_i2c_start();

    if (line_i2c_write_byte((u8)(LINE_SENSOR_I2C_ADDR_7BIT << 1)) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADJUST_ADDR_NACK;
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte(LINE_SENSOR_REG_ADJUST_MODE) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADJUST_REG_NACK;
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte((enable != 0U) ? 1U : 0U) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADJUST_DATA_NACK;
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

    s_line_sensor_last_error = LINE_SENSOR_ERR_NONE;

    if (data == 0)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_NULL_PTR;
        return HAL_ERROR;
    }

    line_i2c_start();
    if (line_i2c_write_byte((u8)(LINE_SENSOR_I2C_ADDR_7BIT << 1)) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADDR_WRITE_NACK;
        line_i2c_stop();
        return HAL_ERROR;
    }

    if (line_i2c_write_byte(LINE_SENSOR_REG_DATA) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_REG_NACK;
        line_i2c_stop();
        return HAL_ERROR;
    }

    line_i2c_start();
    if (line_i2c_write_byte((u8)((LINE_SENSOR_I2C_ADDR_7BIT << 1) | 0x01U)) == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADDR_READ_NACK;
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

LineSensorError_t LineSensor_GetLastError(void)
{
    return s_line_sensor_last_error;
}

const char *LineSensor_GetLastErrorString(void)
{
    switch (s_line_sensor_last_error)
    {
    case LINE_SENSOR_ERR_NONE:
        return "none";
    case LINE_SENSOR_ERR_NULL_PTR:
        return "null_ptr";
    case LINE_SENSOR_ERR_ADDR_WRITE_NACK:
        return "addr_write_nack";
    case LINE_SENSOR_ERR_REG_NACK:
        return "reg_nack";
    case LINE_SENSOR_ERR_ADDR_READ_NACK:
        return "addr_read_nack";
    case LINE_SENSOR_ERR_ADJUST_ADDR_NACK:
        return "adjust_addr_nack";
    case LINE_SENSOR_ERR_ADJUST_REG_NACK:
        return "adjust_reg_nack";
    case LINE_SENSOR_ERR_ADJUST_DATA_NACK:
        return "adjust_data_nack";
    default:
        return "unknown";
    }
}

u8 LineSensor_GetSdaLevel(void)
{
    return line_sda_read();
}

u8 LineSensor_GetSclLevel(void)
{
    return (u8)HAL_GPIO_ReadPin(LINE_SCL_GPIO_PORT, LINE_SCL_PIN);
}

u8 LineSensor_ProbeAddress(u8 addr_7bit)
{
    u8 ack;

    s_line_sensor_last_error = LINE_SENSOR_ERR_NONE;
    line_i2c_start();
    ack = line_i2c_write_byte((u8)(addr_7bit << 1));
    line_i2c_stop();

    if (ack == 0U)
    {
        s_line_sensor_last_error = LINE_SENSOR_ERR_ADDR_WRITE_NACK;
    }

    return ack;
}
