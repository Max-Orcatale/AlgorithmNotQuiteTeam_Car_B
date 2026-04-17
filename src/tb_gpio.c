/* src/tb_gpio.c */
#include "tb_gpio.h"

void tb_gpio_init(void)
{
    /* 使能 AFIO 和 GPIOC 时钟，后面舵机口和重映射都要用到 */
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

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