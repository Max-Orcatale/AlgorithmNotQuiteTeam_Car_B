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
    BOOT_SELECTING = 0,
    BOOT_RUNNING
} BootState_t;

typedef enum
{
    APP_MODE_0 = 0,
    APP_MODE_1,
    APP_MODE_DEBUG
} AppMode_t;

typedef enum
{
    MODE0_STAGE_MARCH1=0,  //出初始区域
    MODE0_STAGE_ARM1,    //捡起满仓环
    MODE0_STAGE_ROUTE1,  //寻线至柱前
    MODE0_STAGE_ARM2,    //准备架势
    MODE0_STAGE_MARCH2,  //靠近柱

    WIND,
    MODE0_STAGE_ARM3,    //将环放入柱中

    MODE0_STAGE_MARCH3,  //后退
    MODE0_STAGE_ROUTE2,  //寻线至环前
    MODE0_STAGE_MARCH4,  //靠近环
    MODE0_STAGE_ARM4,    //捡起环
    MODE0_STAGE_ROUTE3,  //巡线至柱前
    MODE0_STAGE_ARM5,    //准备架势
    MODE0_STAGE_MARCH5,  //靠近柱
    MODE0_STAGE_ARM6,    //将环放入柱中
    MODE0_STAGE_MARCH6,  //前进
    MODE0_STAGE_MARCH7,  //平移

    MODE0_STAGE_DONE
} Mode0Stage_t;

typedef enum
{
    MODE1_STAGE_MARCH1 = 0,
    MODE1_STAGE_ROUTE1,
    MODE1_STAGE_ARM1,//偷环姿势
    MODE1_STAGE_MARCH2,//前进
    MODE1_STAGE_ARM2,//偷环
    MODE1_STAGE_MARCH3,//后退
    MODE1_STAGE_ROUTE2,//寻线至我方两分区
    MODE1_STAGE_MARCH4,//前进一小点
    MODE1_STAGE_ARM3,//放环
    MODE1_STAGE_MARCH5,//后退
    MODE1_STAGE_DONE
} Mode1Stage_t;

//实际路线

static const RouteStep_t m0_route1_steps[] = {
    {1, TURN_LEFT},
    {1, TURN_RIGHT},
    {2, TURN_LEFT},
    {1, TURN_STRAIGHT}
};

static const RouteStep_t m0_route2_steps[] = {
    {1, TURN_RIGHT},
    {1, TURN_STRAIGHT}
};

static const RouteStep_t m0_route3_steps[] = {
    {1, TURN_LEFT}
};

static const RouteStep_t m1_route1_steps[] = {
    {3, TURN_RIGHT}
};

static const RouteStep_t m1_route2_steps[] = {
    {1, TURN_LEFT},
    {2, TURN_RIGHT}
};


static const Route_t mode0_route1 = {
    m0_route1_steps,
    (u16)(sizeof(m0_route1_steps) / sizeof(m0_route1_steps[0]))
};

static const Route_t mode0_route2 = {
    m0_route2_steps,
    (u16)(sizeof(m0_route2_steps) / sizeof(m0_route2_steps[0]))
};
    
static const Route_t mode0_route3 = {
    m0_route3_steps,
    (u16)(sizeof(m0_route3_steps) / sizeof(m0_route3_steps[0]))
};

static const Route_t mode1_route1 = {
    m1_route1_steps,
    (u16)(sizeof(m1_route1_steps) / sizeof(m1_route1_steps[0]))
};

static const Route_t mode1_route2 = {
    m1_route2_steps,
    (u16)(sizeof(m1_route2_steps) / sizeof(m1_route2_steps[0]))
};




int main(void)
{
    BootState_t boot_state = BOOT_SELECTING;

    HAL_Init();         //HAL 库初始化
    tb_rcc_init();      //系统时钟初始化
    tb_global_init();   //全局状态初始化
    tb_gpio_init();     //版极 GPIO 基础初始化
    tb_line_sensor_init(); // 软件I2C巡线模块初始化
    tb_motor_init();    // 电机PWM初始化
    tb_encoder_init();  // 电机编码器初始化
    route_runner_init(); // 走格子/路线状态机初始化
    forward_runner_init(); // 定时运动/等待状态机初始化
    dj_io_init();       //舵机相关GPIO初始化
    tb_servo_init();    // 舵机调度定时器初始化
    tb_servo_demo_init(); // 机械臂状态初始化
    usart3_init();      // USART3 初始化

    uint32_t boot_start_tick = HAL_GetTick();
    u8 boot_key_count = 0U;
    char rx_buf[64];
    ArmPose uart_pose;
    AppMode_t app_mode = APP_MODE_0;
    Mode0Stage_t mode0_stage = MODE0_STAGE_MARCH1;
    Mode1Stage_t mode1_stage = MODE1_STAGE_MARCH1;


    while (1)
    {
        if (boot_state == BOOT_SELECTING)
        {
            tb_motor_stop_all();
            if (key_pressed_event() != 0U)
            {
                if (boot_key_count < 3U)
                {
                    boot_key_count++;
                }
            }

            if ((HAL_GetTick() - boot_start_tick) >= 2000U)
            {
                if (boot_key_count >= 3U)
                {
                    app_mode = APP_MODE_DEBUG;
                }
                else
                {
                    app_mode = (boot_key_count == 0U) ? APP_MODE_0 : APP_MODE_1;
                }
                boot_state = BOOT_RUNNING;
                mode0_stage = MODE0_STAGE_MARCH1;
                mode1_stage = MODE1_STAGE_MARCH1;
                forward_runner_abort();
                route_runner_abort();
                uart_send_reset();
            }
            else
            {
                continue;
            }
        }

    

        if (app_mode == APP_MODE_DEBUG)
        {
            tb_motor_stop_all();
            forward_runner_abort();
            route_runner_abort();

            if (usart3_read_line(rx_buf, (u16)sizeof(rx_buf)) != 0U)
            {
                if (usart3_parse_pulses(rx_buf, &uart_pose) != 0U)
                {
                    servo_apply_pose(&uart_pose);
                    usart3_send_string("ok\r\n");
                }
                else
                {
                    usart3_send_string("format error\r\n");
                }
            }
        }
        else if (app_mode == APP_MODE_1)
        {
            tb_servo_update(); // 主循环持续推进机械臂动作
            switch (mode1_stage)
            {
            case MODE1_STAGE_MARCH1:
                if (run_forward_ms(EXIT_INITAIL_ZONE_MS, EXIT_INITIAL_ZONE_SPEED) != 0U) // 定时前进
                {
                    mode1_stage = MODE1_STAGE_ROUTE1;
                }
                break;

            case MODE1_STAGE_ROUTE1:
                if (run_route(&mode1_route1) != 0U) // 走格子路线状态机
                {
                    mode1_stage = MODE1_STAGE_ARM1;
                }
                break;

            case MODE1_STAGE_ARM1:
                route_runner_abort();
                if (tb_servo_is_busy() == 0U)
                {
                    if (tb_servo_start_action(&stole_direct) != 0U)
                    {
                        mode1_stage = MODE1_STAGE_MARCH2;
                    }
                }
                break;

            case MODE1_STAGE_MARCH2:
                if (tb_servo_is_busy() == 0U)
                {
                    if (run_forward_while_follow_line(APPROACH_POLE_MS+500, APPROACH_POLE_SPEED+200) !=0U) // 定时前进
                    {
                        mode1_stage = MODE1_STAGE_ARM2;
                    }
                }
                break;
            
            case MODE1_STAGE_ARM2:
                if (tb_servo_is_busy() == 0U){
                    if (tb_servo_start_action(&stole) != 0U)
                    {
                        mode1_stage = MODE1_STAGE_MARCH3;
                    }
                }
                break;
            
            case MODE1_STAGE_MARCH3:
                if (tb_servo_is_busy() == 0U)
                {
                    if (run_forward_ms(1000, -1300) != 0U) // 定时后退
                    {
                        mode1_stage = MODE1_STAGE_ROUTE2;
                    }
                }
                break;

            case MODE1_STAGE_ROUTE2:
                if (tb_servo_is_busy() == 0U)
                {
                    if (run_route(&mode1_route2) != 0U) // 走格子路线状态机
                    {
                        mode1_stage = MODE1_STAGE_MARCH4;
                    }
                }
                break;
            
            case MODE1_STAGE_MARCH4:
                if (tb_servo_is_busy() == 0U)
                {
                    if (run_forward_while_follow_line(1800, 1200) != 0U) // 前进
                    {
                        mode1_stage = MODE1_STAGE_ARM3;
                    }
                }
                break;

            case MODE1_STAGE_ARM3:
                if (tb_servo_is_busy() == 0U){
                    if (tb_servo_start_action(&stole_place) != 0U)
                    {
                        mode1_stage = MODE1_STAGE_MARCH5;
                    }
                }
                break;
            
            case MODE1_STAGE_MARCH5:
                if (tb_servo_is_busy() == 0U)
                {
                    if (run_forward_ms(1300, -1300) != 0U) // 定时后退
                    {
                        mode1_stage = MODE1_STAGE_DONE;
                    }
                }
                break;

            case MODE1_STAGE_DONE:
            default:
                forward_runner_abort();
                route_runner_abort();
                break;
            }


        }
        else if (app_mode == APP_MODE_0)
        {

            tb_servo_update(); // 主循环持续推进机械臂动作

            switch (mode0_stage)
            {

        
        
            case MODE0_STAGE_MARCH1:
                if (run_forward_ms(EXIT_INITAIL_ZONE_MS, EXIT_INITIAL_ZONE_SPEED) != 0U) // 定时前进
                {
                    mode0_stage = MODE0_STAGE_ARM1;
                }
                break;

            case MODE0_STAGE_ARM1:
                route_runner_abort();
                if (tb_servo_is_busy() == 0U)
                {
                    if (tb_servo_start_action(&pick) != 0U)
                    {
                        mode0_stage = MODE0_STAGE_ROUTE1;
                    }
                }
            break;

            case MODE0_STAGE_ROUTE1:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_route(&mode0_route1) != 0U) // 走格子路线状态机
                {
                    mode0_stage = MODE0_STAGE_ARM2;
                }
            }
            break;

            case MODE0_STAGE_ARM2:
                route_runner_abort();
                if (tb_servo_is_busy() == 0U)
                {
                    if (tb_servo_start_action(&direct) != 0U)
                    {
                        mode0_stage = MODE0_STAGE_MARCH2;
                    }
                }
                break;


        case MODE0_STAGE_MARCH2:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_while_follow_line(APPROACH_POLE_MS, APPROACH_POLE_SPEED) != 0U) // 定时直行/倒退
                {
                    mode0_stage = WIND;
                }
            }
            break;

        case WIND:
            if (tb_servo_is_busy() == 0U)
            {
                if (uart_send("wind 25\n") != 0U) // 一次性串口发送
                {
                    mode0_stage = MODE0_STAGE_ARM3;
                }
            }
            break;
        

        case MODE0_STAGE_ARM3:
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&place) != 0U)
                {
                    mode0_stage = MODE0_STAGE_MARCH3;
                }
            }
            break;

        

        case MODE0_STAGE_MARCH3:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_while_follow_line(BACKWARD_MS, BACKWARD_SPEED) != 0U) // 定时寻线前进
                {
                    mode0_stage = MODE0_STAGE_ROUTE2;
                }
            }
            break;
            
        case MODE0_STAGE_ROUTE2:
            if (tb_servo_is_busy() == 0U)            
            {
                if (run_route(&mode0_route2) != 0U) 
                {                    
                    mode0_stage = MODE0_STAGE_MARCH4;
                }
            }
            break;

        case MODE0_STAGE_MARCH4:
            if (tb_servo_is_busy() == 0U)           
            {
                if (run_forward_while_follow_line(APPROACH_RING_MS, APPROACH_RING_SPEED) != 0U) // 定时左平移
                {                    
                    mode0_stage = MODE0_STAGE_ARM4;
                }
            }
            break;

        case MODE0_STAGE_ARM4:
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&pick) != 0U)
                {
                    mode0_stage = MODE0_STAGE_ROUTE3;
                }
            }
            break;

        case MODE0_STAGE_ROUTE3:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_route(&mode0_route3) != 0U) // 走格子路线状态机
                {
                    mode0_stage = MODE0_STAGE_ARM5;
                }
            }
            break;

        case MODE0_STAGE_ARM5:
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&direct) != 0U)
                {
                    mode0_stage = MODE0_STAGE_MARCH5;
                }
            }
            break;

        case MODE0_STAGE_MARCH5:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_while_follow_line(APPROACH_POLE_MS, APPROACH_POLE_SPEED) != 0U) // 定时前进
                {
                    mode0_stage = MODE0_STAGE_ARM6;
                }
            }
            break;

        case MODE0_STAGE_ARM6:
            if (tb_servo_is_busy() == 0U)
            {
                if (tb_servo_start_action(&place2) != 0U)
                {
                    mode0_stage = MODE0_STAGE_MARCH6;
                }
            }
            break;


        case MODE0_STAGE_MARCH6:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_strafe_left_ms(1800, 1500) != 0U) 
                {
                    mode0_stage = MODE0_STAGE_MARCH7;
                }
            }
            break;
        
        case MODE0_STAGE_MARCH7:
            if (tb_servo_is_busy() == 0U)
            {
                if (run_forward_ms(1800, 1300) != 0U) 
                {
                    mode0_stage = MODE0_STAGE_DONE;
                }
            }
            break;

        
        
        case MODE0_STAGE_DONE:
        default:
            route_runner_abort();
            break;
        }
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
