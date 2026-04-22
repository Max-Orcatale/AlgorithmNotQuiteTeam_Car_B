/* src/tb_gpio.c */
#include "tb_gpio.h"

void tb_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 AFIO、GPIOA、GPIOB、GPIOC 时钟 */
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* M4 编码器占用 PA15/PB3，关闭 JTAG 只保留 SWD */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    __HAL_AFIO_REMAP_TIM2_ENABLE();

    /* KEY 接在 PB2，按下默认拉低，这里配置成上拉输入 */
    GPIO_InitStruct.Pin = KEY_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);

    /* 软件 I2C 空闲态拉高 */
    GPIO_InitStruct.Pin = LINE_SDA_PIN | LINE_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LINE_SDA_GPIO_PORT, LINE_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE_SCL_GPIO_PORT, LINE_SCL_PIN, GPIO_PIN_SET);
}

void dj_io_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = SERVO0_GPIO_PIN
                        | SERVO1_GPIO_PIN
                        | SERVO2_GPIO_PIN
                        | SERVO3_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SERVO0_GPIO_PORT, &GPIO_InitStruct);
}

void dj_io_set(u8 index, u8 level)
{
    switch (index)
    {
    case 0:
        HAL_GPIO_WritePin(SERVO0_GPIO_PORT, SERVO0_GPIO_PIN, level);
        break;
    case 1:
        HAL_GPIO_WritePin(SERVO1_GPIO_PORT, SERVO1_GPIO_PIN, level);
        break;
    case 2:
        HAL_GPIO_WritePin(SERVO2_GPIO_PORT, SERVO2_GPIO_PIN, level);
        break;
    case 3:
        HAL_GPIO_WritePin(SERVO3_GPIO_PORT, SERVO3_GPIO_PIN, level);
        break;

    default:
        break;
    }
}

u8 key_read(void)
{
    /* 按键按下为低电平，这里统一转换成 0=按下, 1=未按下 */
    return (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1;
}
