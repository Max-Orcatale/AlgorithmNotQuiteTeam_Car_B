/* src/tb_timer.c */
#include "tb_timer.h"
#include "tb_global.h"
#include "tb_gpio.h"

static TIM_HandleTypeDef htim2;

static u8 servo_index = 0;
static u8 servo_phase = 0;   // 0: 准备拉高, 1: 准备拉低




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


/*统一接口*/
void pwmServo_angle_set(u8 index, int aim, int time){
    if (index >= DJ_NUM)
        return;

    if (aim < 500) aim = 500;
    if (aim > 2500) aim = 2500;

    if (time > 10000)
        time = 10000;

    if (time < 20)  //如果时间太短，直接设置到位，不进行过渡
    {
        duoji_doing[index].cur = (float)aim;
        duoji_doing[index].aim = (u16)aim;
        duoji_doing[index].time = 20;
        duoji_doing[index].inc = 0.0f;
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
/* 具体逻辑：*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    
    if (servo_phase == 0)
    {
        duoji_inc_handle(servo_index);

        u16 pulse_us = (u16)duoji_doing[servo_index].cur;
        if (pulse_us < 500)  pulse_us = 500;
        if (pulse_us > 2500) pulse_us = 2500;

        dj_io_set(servo_index, 1);   // 拉高当前舵机脚
        __HAL_TIM_SET_AUTORELOAD(&htim2, pulse_us - 1);

        servo_phase = 1;
    }
    else
    {
        u16 pulse_us = (u16)duoji_doing[servo_index].cur;
        if (pulse_us < 500)  pulse_us = 500;
        if (pulse_us > 2500) pulse_us = 2500;

        dj_io_set(servo_index, 0);   // 拉低当前舵机脚
        __HAL_TIM_SET_AUTORELOAD(&htim2, (5000 - pulse_us) - 1);

        servo_phase = 0;
        servo_index = (servo_index + 1) % DJ_NUM;   // 切换到下一个舵机
    }

    __HAL_TIM_SET_COUNTER(&htim2, 0);

}
