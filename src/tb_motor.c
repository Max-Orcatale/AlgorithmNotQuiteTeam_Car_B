#include "tb_motor.h"

#include "car_config.h"
#include "main.h"
#include "stm32f1xx_hal_gpio_ex.h"

static TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef htim8;

static int16_t motor_clamp_speed(int16_t speed)
{
    if (speed > MOTOR_PWM_MAX)
    {
        return MOTOR_PWM_MAX;
    }
    if (speed < -MOTOR_PWM_MAX)
    {
        return -MOTOR_PWM_MAX;
    }
    return speed;
}

static void motor_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Align TIM1 outputs with the board wiring used by the C26 V2.2 reference. */
    __HAL_AFIO_REMAP_TIM1_PARTIAL();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &gpio);
}

static void motor_pwm_timer_init(TIM_HandleTypeDef *htim, TIM_TypeDef *instance)
{
    TIM_OC_InitTypeDef oc = {0};
    TIM_BreakDeadTimeConfigTypeDef bd = {0};

    htim->Instance = instance;
    htim->Init.Prescaler = MOTOR_PWM_PRESCALER;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = MOTOR_PWM_PERIOD;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
        Error_Handler();
    }

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_2) != HAL_OK ||
        HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_3) != HAL_OK ||
        HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }

    bd.OffStateRunMode = TIM_OSSR_DISABLE;
    bd.OffStateIDLEMode = TIM_OSSI_DISABLE;
    bd.LockLevel = TIM_LOCKLEVEL_OFF;
    bd.DeadTime = 0;
    bd.BreakState = TIM_BREAK_DISABLE;
    bd.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    bd.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

    if (HAL_TIMEx_ConfigBreakDeadTime(htim, &bd) != HAL_OK)
    {
        Error_Handler();
    }
}

static void motor_write_pair(TIM_HandleTypeDef *htim,
                             uint32_t ch_forward,
                             uint32_t ch_reverse,
                             int16_t speed)
{
    uint16_t pwm = (uint16_t)((speed >= 0) ? speed : -speed);

    if (speed >= 0)
    {
        __HAL_TIM_SET_COMPARE(htim, ch_forward, pwm);
        __HAL_TIM_SET_COMPARE(htim, ch_reverse, 0);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(htim, ch_forward, 0);
        __HAL_TIM_SET_COMPARE(htim, ch_reverse, pwm);
    }
}

void tb_motor_init(void)
{
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();

    motor_gpio_init();
    motor_pwm_timer_init(&htim1, TIM1);
    motor_pwm_timer_init(&htim8, TIM8);

    if (HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4) != HAL_OK ||
        HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK ||
        HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    tb_motor_stop_all();
}

void tb_motor_set_speed(u8 motor_id, int16_t speed)
{
    switch (motor_id)
    {
    case 0:
        speed = motor_clamp_speed((int16_t)(speed * MOTOR1_DIR_SIGN));
        motor_write_pair(&htim8, TIM_CHANNEL_1, TIM_CHANNEL_2, speed);
        break;
    case 1:
        speed = motor_clamp_speed((int16_t)(speed * MOTOR2_DIR_SIGN));
        motor_write_pair(&htim8, TIM_CHANNEL_3, TIM_CHANNEL_4, speed);
        break;
    case 2:
        speed = motor_clamp_speed((int16_t)(speed * MOTOR3_DIR_SIGN));
        motor_write_pair(&htim1, TIM_CHANNEL_2, TIM_CHANNEL_3, speed);
        break;
    case 3:
        speed = motor_clamp_speed((int16_t)(speed * MOTOR4_DIR_SIGN));
        motor_write_pair(&htim1, TIM_CHANNEL_1, TIM_CHANNEL_4, speed);
        break;
    default:
        break;
    }
}

void tb_motor_set_all(int16_t speed1, int16_t speed2, int16_t speed3, int16_t speed4)
{
    tb_motor_set_speed(0, speed1);
    tb_motor_set_speed(1, speed2);
    tb_motor_set_speed(2, speed3);
    tb_motor_set_speed(3, speed4);
}

void tb_motor_stop_all(void)
{
    tb_motor_set_all(0, 0, 0, 0);
}
