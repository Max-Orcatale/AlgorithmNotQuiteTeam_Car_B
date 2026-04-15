/* src/tb_timer.c */
#include "tb_timer.h"
#include "tb_global.h"
#include "tb_gpio.h"

static TIM_HandleTypeDef htim4;

void pwmServo_init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 72 - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 20000 - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, SERVO0_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(&htim4, SERVO0_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
}

void pwmServo_set(u8 index, u16 pulse)
{
    if (index != 0)
    {
        return;
    }

    if (pulse < 1000)
    {
        pulse = 1000;
    }
    if (pulse > 2000)
    {
        pulse = 2000;
    }

    __HAL_TIM_SET_COMPARE(&htim4, SERVO0_TIM_CHANNEL, pulse);
    duoji_doing[index].cur = pulse;
    duoji_doing[index].aim = pulse;
    duoji_doing[index].inc = 0;
}
