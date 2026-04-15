/* src/tb_timer.c */
#include "tb_timer.h"
#include "tb_global.h"
#include "tb_gpio.h"

static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef htim4;

static void pwmServo_write_raw(u8 index, u16 pulse)
{
    if (pulse < 1000)
    {
        pulse = 1000;
    }
    if (pulse > 2000)
    {
        pulse = 2000;
    }

    switch (index)
    {
        case 0:
            /* SERVO0 -> PB9 -> TIM4_CH4 */
            __HAL_TIM_SET_COMPARE(&htim4, SERVO0_TIM_CHANNEL, pulse);
            break;
        case 1:
            /* SERVO1 -> PB8 -> TIM4_CH3 */
            __HAL_TIM_SET_COMPARE(&htim4, SERVO1_TIM_CHANNEL, pulse);
            break;
        case 2:
            /* SERVO2 -> PB5 -> TIM3_CH2 */
            __HAL_TIM_SET_COMPARE(&htim3, SERVO2_TIM_CHANNEL, pulse);
            break;
        case 3:
            /* SERVO3 -> PB4 -> TIM3_CH1 */
            __HAL_TIM_SET_COMPARE(&htim3, SERVO3_TIM_CHANNEL, pulse);
            break;
        default:
            break;
    }
}


/* PWM 舵机初始化函数，也是 TIM4 初始化函数 ，因为舵机控制使用 TIM4 */
void pwmServo_init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* 4 路舵机分布在 TIM3 和 TIM4 上，因此两个定时器都要初始化 */
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* TIM3: 驱动 SERVO2 / SERVO3，20ms 周期，1us 计数精度 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 20000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    /* TIM4: 驱动 SERVO0 / SERVO1，20ms 周期，1us 计数精度 */
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

    /* TIM3_CH1 -> SERVO3，TIM3_CH2 -> SERVO2 */
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, SERVO3_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, SERVO2_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }

    /* TIM4_CH3 -> SERVO1，TIM4_CH4 -> SERVO0 */
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, SERVO1_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, SERVO0_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }

    /* 依次启动 4 路 PWM 输出 */
    if (HAL_TIM_PWM_Start(&htim3, SERVO3_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim3, SERVO2_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim4, SERVO1_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Start(&htim4, SERVO0_TIM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
}

/* TIM2 初始化函数 */
void TIM2_Int_Init(u16 arr, u16 psc)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = psc;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = arr;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
}

void duoji_inc_handle(u8 index)
{
    float aim;
    float cur;
    float inc;

    if (index >= DJ_NUM)
    {
        return;
    }

    aim = duoji_doing[index].aim;
    cur = duoji_doing[index].cur;
    inc = duoji_doing[index].inc;

    if (inc == 0.0f)
    {
        return;
    }

    if ((inc > 0.0f && (aim - cur) <= inc) ||
        (inc < 0.0f && (cur - aim) <= -inc))
    {
        duoji_doing[index].cur = aim;
        duoji_doing[index].inc = 0.0f;
    }
    else
    {
        duoji_doing[index].cur += inc;
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

    pwmServo_write_raw(index, pulse);
    duoji_doing[index].cur = pulse;
    duoji_doing[index].aim = pulse;
    duoji_doing[index].inc = 0;
}

/*统一接口*/
void pwmServo_angle_set(u8 index, int aim, int time){
    if (index >= DJ_NUM)
        return;

    if (aim < 1000) aim = 1000;
    if (aim > 2000) aim = 2000;

    if (time > 10000)
        time = 10000;

    if (time < 20)  //如果时间太短，直接设置到位，不进行过渡
    {
        pwmServo_write_raw(index, (u16)aim);
        duoji_doing[index].cur = (u16)aim;
        duoji_doing[index].aim = (u16)aim;
        duoji_doing[index].time = 20;
        duoji_doing[index].inc = 0;
        return;
    }

    duoji_doing[index].aim = (u16)aim;
    duoji_doing[index].time = (u16)time;
    duoji_doing[index].inc =
        (duoji_doing[index].aim - duoji_doing[index].cur) /
        (duoji_doing[index].time / 20.0f);
}



/* TIM2 中断处理函数 */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);  //里面会调用 HAL_TIM_PeriodElapsedCallback
}

/* TIM周期更新回调函数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        u8 i;

        for (i = 0; i < DJ_NUM; i++)
        {
            duoji_inc_handle(i);
            pwmServo_write_raw(i, (u16)duoji_doing[i].cur);
        }
    }
}
