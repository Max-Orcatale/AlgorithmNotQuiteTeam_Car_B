#include <stdio.h>

#include "main.h"
#include "tb_encoder.h"
#include "tb_global.h"
#include "tb_gpio.h"
#include "line_follow_ctrl.h"
#include "tb_line_sensor.h"
#include "tb_motor.h"
#include "tb_rcc.h"
#include "tb_servo.h"
#include "tb_uart.h"

/*状态机*/
typedef enum
{
    APP_STAGE_ROUTE1 = 0,
    APP_STAGE_ARM1,
    APP_STAGE_ROUTE2,
    APP_STAGE_ARM2,
    APP_STAGE_ARM3,
    APP_STAGE_DONE
} AppStage_t;

static const RouteStep_t route1_steps[] = {
    {1, TURN_STRAIGHT}
};

static const RouteStep_t route2_steps[] = {
    {1, TURN_LEFT},
    {2, TURN_STRAIGHT},
    {3, TURN_LEFT},
    {1, TURN_LEFT}
};

static const Route_t route1 = {
    route1_steps,
    (u16)(sizeof(route1_steps) / sizeof(route1_steps[0]))
};

static const Route_t route2 = {
    route2_steps,
    (u16)(sizeof(route2_steps) / sizeof(route2_steps[0]))
};

int main(void)
{

    HAL_Init();         //HAL 库初始化
    tb_rcc_init();      //系统时钟初始化
    tb_global_init();   //全局状态初始化
    tb_gpio_init();     //版极 GPIO 基础初始化
    tb_line_sensor_init(); // 软件I2C巡线模块初始化
    tb_motor_init();    // 电机PWM初始化
    tb_encoder_init();  // 电机编码器初始化
    route_runner_init(); // 走格子/路线状态机初始化
    dj_io_init();       //舵机相关GPIO初始化
    tb_servo_init();    // 舵机调度定时器初始化
    tb_servo_demo_init(); // 机械臂状态初始化
    usart3_init();      // USART3 初始化

    AppStage_t stage = APP_STAGE_ROUTE1;


    while (1)
    {
    
        tb_servo_update();

        switch (stage)
        {
        case APP_STAGE_ROUTE1:
            if (run_forward_ms(2000, 1300) != 0U)
            {
                stage = APP_STAGE_ARM1;
            }
            break;

        case APP_STAGE_ARM1:
            route_runner_abort();
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&pick) != 0U)
                {
                    stage = APP_STAGE_ROUTE2;
                }
            }
            break;

        case APP_STAGE_ROUTE2:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_route(&route2) != 0U)
                {
                    stage = APP_STAGE_ARM2;
                }
            }
            break;

        case APP_STAGE_ARM2:
            route_runner_abort();
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&direct) != 0U)
                {
                    stage = APP_STAGE_ARM3;
                }
            }
            break;

        case APP_STAGE_ARM3:
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&place) != 0U)
                {
                    stage = APP_STAGE_DONE;
                }
            }
            break;

        case APP_STAGE_DONE:
        default:
            route_runner_abort();
            break;
        }
    }
}

// 错误处理函数
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
