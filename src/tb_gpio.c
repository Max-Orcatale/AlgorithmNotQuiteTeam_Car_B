/* src/tb_gpio.c */
#include "tb_gpio.h"

void tb_gpio_init(void)
{
    /* 使能 AFIO 和 GPIOB 时钟，后面舵机口和重映射都要用到 */
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* TIM3 的 CH1/CH2 需要部分重映射到 PB4/PB5 */
#if SERVO23_USE_TIM3_PARTIAL_REMAP
    __HAL_AFIO_REMAP_TIM3_PARTIAL();
#endif

    /* PB4 默认被 JTAG 占用，这里关闭 JTAG、保留 SWD，释放 PB4 给舵机使用 */
#if SERVO3_USE_PB4_REQUIRES_NOJTAG
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
#endif
}

void dj_io_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 4 路舵机信号脚都配置为复用推挽输出，由 TIM3/TIM4 直接输出 PWM */
    GPIO_InitStruct.Pin = SERVO0_GPIO_PIN
                        | SERVO1_GPIO_PIN
                        | SERVO2_GPIO_PIN
                        | SERVO3_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SERVO0_GPIO_PORT, &GPIO_InitStruct);
}
