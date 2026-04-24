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
    UNWIND = 0,
    APP_STAGE_ROUTE1,
    APP_STAGE_ARM1,
    APP_STAGE_ROUTE2,
    APP_STAGE_ARM2,
    APP_STAGE_ROUTE3,
    APP_STAGE_ARM3,
    APP_STAGE_ROUTE4,
    APP_STAGE_ROUTE5,
    APP_STAGE_ROUTE6,
    APP_STAGE_ROUTE7,
    WIND,
    APP_STAGE_DONE
} AppStage_t;

//实际路线

static const RouteStep_t route2_steps[] = {
    {1, TURN_LEFT},
    {2, TURN_RIGHT},
    {2, TURN_LEFT}
};

static const RouteStep_t route5_steps[] = {
    {1, TURN_RIGHT},
    {1, TURN_LEFT},
    {1, TURN_LEFT}
};




static const Route_t route2 = {
    route2_steps,
    (u16)(sizeof(route2_steps) / sizeof(route2_steps[0]))
};

static const Route_t route6 = {
    route5_steps,
    (u16)(sizeof(route5_steps) / sizeof(route5_steps[0]))
};


//测试用路线
static const RouteStep_t test_route1_steps[] = {
    {2, TURN_LEFT},
    {2, TURN_STRAIGHT}
};

static const RouteStep_t test_route2_steps[] = {
    {2, TURN_RIGHT},
    {2, TURN_STRAIGHT}
};

static const RouteStep_t test_route3_steps[] = {
    {2, TURN_BACK},
    {2, TURN_STRAIGHT}
};

static const Route_t test_route1 = {
    test_route1_steps,
    (u16)(sizeof(test_route1_steps) / sizeof(test_route1_steps[0]))
};

static const Route_t test_route2 = {
    test_route2_steps,
    (u16)(sizeof(test_route2_steps) / sizeof(test_route2_steps[0]))
};

static const Route_t test_route3 = {
    test_route3_steps,
    (u16)(sizeof(test_route3_steps) / sizeof(test_route3_steps[0]))
};

static const Route_t *const g_test_routes[] = {
    &test_route1,
    &test_route2,
    &test_route3
};

int main(void)
{
    u8 last_key_state = 1U;
    u8 current_key_state;
    u8 route_index = 0U;
    const Route_t *active_route = 0;

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

    AppStage_t stage = UNWIND;


    while (1)
    {
        /*current_key_state = key_read();


        if ((last_key_state == 1U) && (current_key_state == 0U) && (active_route == 0))
        {
            active_route = g_test_routes[route_index];
            route_index++;
            if (route_index >= (u8)(sizeof(g_test_routes) / sizeof(g_test_routes[0])))
            {
                route_index = 0U;
            }
        }

        last_key_state = current_key_state;

        if (active_route != 0)
        {
            if (run_route(active_route) != 0U)
            {
                active_route = 0;
                route_runner_abort();
            }
        }*/
    
    
        tb_servo_update(); // 主循环持续推进机械臂动作

        switch (stage)
        {
        case UNWIND:
            if (uart_send("unwind\n") != 0U) // 一次性串口发送
            {
                stage = APP_STAGE_ROUTE1;
            }
            break;
        
        case APP_STAGE_ROUTE1:
            if (run_forward_ms(1820, 1300) != 0U) // 定时寻线前进
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
                if (run_route(&route2) != 0U) // 走格子路线状态机
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
                    stage = APP_STAGE_ROUTE3;
                }
            }
            break;

        case APP_STAGE_ROUTE3:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_ms(1500, -1200) != 0U) // 定时寻线前进
                {
                    stage = APP_STAGE_ROUTE4;
                }
            }
            break;

        case APP_STAGE_ROUTE4:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_while_follow_line(2700, 1200) != 0U) // 定时直行/倒退
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
                    stage = APP_STAGE_ROUTE5;
                }
            }
            break;

        case APP_STAGE_ROUTE5:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_ms(2000, -1200) != 0U) // 定时寻线前进
                {
                    stage = APP_STAGE_ROUTE6;
                }
            }
            break;
            
        case APP_STAGE_ROUTE6:
            if (tb_servo_is_busy() == 0U)            {
                if (run_route(&route6) != 0U) // 走格子路线状态机
                {                    
                    stage = APP_STAGE_ROUTE7;
                }
            }
            break;

        case APP_STAGE_ROUTE7:
            if (tb_servo_is_busy() == 0U)            {
                if (run_strafe_right_ms(2000, 1200) != 0U) // 定时右平移
                {                    
                    stage = WIND;
                }
            }
            break;

        case WIND:
            if (tb_servo_is_busy() == 0U)
            {
                if (uart_send("wind\n") != 0U) // 一次性串口发送
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
