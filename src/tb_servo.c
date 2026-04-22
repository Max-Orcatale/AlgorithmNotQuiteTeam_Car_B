#include "tb_servo.h"

#include "tb_global.h"
#include "tb_gpio.h"

#define SERVO3_LOOSE 1500
#define SERVO3_TIGHT 1710
#define SERVO_ACTION_COUNT 3

static TIM_HandleTypeDef htim6;

static u8 servo_index = 0;
static u8 servo_phase = 0;
static u8 hand_busy = 0;
static const ArmAction *s_current_action = 0;

static const ArmPose s_pick_poses[] = {
    {{1800, 2400, 1650, SERVO3_LOOSE}, 2000},
    {{1900, 2400, 1650, SERVO3_TIGHT}, 2000},
    {{1500, 1500, 1650, SERVO3_TIGHT}, 2000}
};

static const ArmPose s_direct_poses[] = {
    {{1730, 2100, 1650, SERVO3_TIGHT}, 2000}
};

static const ArmPose s_place_poses[] = {
    {{1700, 2100, 1650, SERVO3_LOOSE}, 2000},
    {{1700, 2100, 1650, SERVO3_TIGHT}, 2000},
    {{1500, 2100, 1650, SERVO3_TIGHT}, 2000},
    {{1750, 1800, 1650, SERVO3_TIGHT}, 2000},
    {{1770, 1800, 1650, SERVO3_TIGHT}, 2000},
    {{1600, 2100, 1650, SERVO3_LOOSE}, 2000}
};

const ArmAction pick = {s_pick_poses, (u8)(sizeof(s_pick_poses) / sizeof(s_pick_poses[0]))};
const ArmAction direct = {s_direct_poses, (u8)(sizeof(s_direct_poses) / sizeof(s_direct_poses[0]))};
const ArmAction place = {s_place_poses, (u8)(sizeof(s_place_poses) / sizeof(s_place_poses[0]))};



static void do_pose(const ArmPose *pose);
static u8 check_dj_state(void);
static void handle_action(const ArmAction *action);

void tb_servo_init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = SERVO_TIMER_PRESCALER;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = SERVO_TIMER_PERIOD;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
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

void pwmServo_angle_set(u8 index, int aim, int time)
{
    if (index >= DJ_NUM)
    {
        return;
    }

    if (aim < 500)
    {
        aim = 500;
    }
    if (aim > 2500)
    {
        aim = 2500;
    }
    if (time > 10000)
    {
        time = 10000;
    }

    if (time < 20)
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

void tb_servo_demo_init(void)
{
    hand_busy = 0;
    s_current_action = 0;
}

/*void tb_servo_demo_update(void)
{
    static u8 last_key_state = 1;
    static u8 act_index = 0;
    static const ArmAction *current_action = 0;
    u8 cur_key_state = key_read();

    if ((last_key_state == 1U) && (cur_key_state == 0U) && (hand_busy == 0U))
    {
        current_action = &s_actions[act_index];
        s_current_action = current_action;
        hand_busy = 1U;

        act_index++;
        if (act_index >= SERVO_ACTION_COUNT)
        {
            act_index = 0;
        }
    }

    last_key_state = cur_key_state;

    if (current_action != 0)
    {
        handle_action(current_action);

        if (hand_busy == 0U)
        {
            current_action = 0;
            s_current_action = 0;
        }
    }
}*/

u8 tb_servo_start_action(const ArmAction *action)
{
    if ((action == 0) || (action->poses == 0) || (action->step_count == 0U))
    {
        return 0U;
    }

    if (hand_busy != 0U)
    {
        return 0U;
    }

    s_current_action = action;
    hand_busy = 1U;
    return 1U;
}

void tb_servo_update(void)
{
    if (s_current_action != 0)
    {
        handle_action(s_current_action);

        if (hand_busy == 0U)
        {
            s_current_action = 0;
        }
    }
}

u8 tb_servo_is_busy(void)
{
    return hand_busy;
}

static u8 check_dj_state(void)
{
    u8 i;

    for (i = 0; i < DJ_NUM; i++)
    {
        if (duoji_doing[i].inc != 0.0f)
        {
            return 1;
        }
    }

    return 0;
}

static void do_pose(const ArmPose *pose)
{
    u8 i;

    if (pose == 0)
    {
        return;
    }

    for (i = 0; i < DJ_NUM; i++)
    {
        pwmServo_angle_set(i, pose->pulse[i], pose->time_ms);
    }
}

static void handle_action(const ArmAction *action)
{
    static u8 g_current_step = 0;
    static u8 g_step_started = 0;

    if ((action == 0) || (action->poses == 0) || (action->step_count == 0))
    {
        hand_busy = 0;
        g_current_step = 0;
        g_step_started = 0;
        return;
    }

    if (g_current_step >= action->step_count)
    {
        g_current_step = 0;
        g_step_started = 0;
        hand_busy = 0;
        return;
    }

    if (g_step_started == 0)
    {
        hand_busy = 1;
        do_pose(&action->poses[g_current_step]);
        g_step_started = 1;
    }
    else if (check_dj_state() == 0)
    {
        g_current_step++;
        g_step_started = 0;
    }
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    u16 pulse_us;

    if (htim->Instance != TIM6)
    {
        return;
    }

    if (servo_phase == 0)
    {
        duoji_inc_handle(servo_index);

        pulse_us = (u16)duoji_doing[servo_index].cur;
        if (pulse_us < 500U)
        {
            pulse_us = 500U;
        }
        if (pulse_us > 2500U)
        {
            pulse_us = 2500U;
        }

        dj_io_set(servo_index, 1);
        __HAL_TIM_SET_AUTORELOAD(&htim6, pulse_us - 1U);
        servo_phase = 1;
    }
    else
    {
        pulse_us = (u16)duoji_doing[servo_index].cur;
        if (pulse_us < 500U)
        {
            pulse_us = 500U;
        }
        if (pulse_us > 2500U)
        {
            pulse_us = 2500U;
        }

        dj_io_set(servo_index, 0);
        __HAL_TIM_SET_AUTORELOAD(&htim6, (SERVO_SLOT_US - pulse_us) - 1U);

        servo_phase = 0;
        servo_index = (servo_index + 1U) % DJ_NUM;
    }

    __HAL_TIM_SET_COUNTER(&htim6, 0);
}
