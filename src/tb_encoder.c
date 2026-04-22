#include "tb_encoder.h"

#include "main.h"
#include "stm32f1xx_hal_gpio_ex.h"

static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef htim4;
static TIM_HandleTypeDef htim5;

static void encoder_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    __HAL_AFIO_REMAP_TIM2_ENABLE();

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void encoder_timer_init(TIM_HandleTypeDef *htim, TIM_TypeDef *instance, uint32_t period)
{
    TIM_Encoder_InitTypeDef config = {0};
    TIM_MasterConfigTypeDef master = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = 0;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = period;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    config.EncoderMode = TIM_ENCODERMODE_TI12;
    config.IC1Polarity = TIM_ICPOLARITY_RISING;
    config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    config.IC1Prescaler = TIM_ICPSC_DIV1;
    config.IC1Filter = 0;
    config.IC2Polarity = TIM_ICPOLARITY_RISING;
    config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    config.IC2Prescaler = TIM_ICPSC_DIV1;
    config.IC2Filter = 0;

    if (HAL_TIM_Encoder_Init(htim, &config) != HAL_OK)
    {
        Error_Handler();
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(htim, &master) != HAL_OK)
    {
        Error_Handler();
    }
}

void tb_encoder_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();

    encoder_gpio_init();
    encoder_timer_init(&htim2, TIM2, 0xFFFFU);
    encoder_timer_init(&htim3, TIM3, 0xFFFFU);
    encoder_timer_init(&htim4, TIM4, 0xFFFFU);
    encoder_timer_init(&htim5, TIM5, 0xFFFFFFFFU);

    if (HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK ||
        HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK ||
        HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK ||
        HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL) != HAL_OK)
    {
        Error_Handler();
    }
}

int32_t tb_encoder_get_count(u8 motor_id)
{
    switch (motor_id)
    {
    case 0:
        return (int32_t)__HAL_TIM_GET_COUNTER(&htim5);
    case 1:
        return (int32_t)__HAL_TIM_GET_COUNTER(&htim4);
    case 2:
        return (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
    case 3:
        return (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
    default:
        return 0;
    }
}

void tb_encoder_reset(u8 motor_id)
{
    switch (motor_id)
    {
    case 0:
        __HAL_TIM_SET_COUNTER(&htim5, 0);
        break;
    case 1:
        __HAL_TIM_SET_COUNTER(&htim4, 0);
        break;
    case 2:
        __HAL_TIM_SET_COUNTER(&htim3, 0);
        break;
    case 3:
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        break;
    default:
        break;
    }
}
