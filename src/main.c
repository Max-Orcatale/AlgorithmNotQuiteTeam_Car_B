#include "main.h"
#include "tb_delay.h"
#include "tb_global.h"
#include "tb_gpio.h"
#include "tb_rcc.h"
#include "tb_timer.h"
#include "tb_uart.h"


#define SERVO3_LOOSE 1500
#define SERVO3_TIGHT 1710

static u8 hand_busy = 0;
  


/*机械臂层控制函数*/
static void do_pose(const ArmPose *pose);
static u8 check_dj_state(void);
static void handle_action(const ArmAction *action);


int main(void)
{
    HAL_Init();         //HAL 库初始化
    tb_rcc_init();      //系统时钟初始化
    tb_global_init();   //全局状态初始化
    tb_gpio_init();     //版极 GPIO 基础初始化
    dj_io_init();       //舵机相关GPIO初始化
    TIM2_Int_Init(20000 - 1, 72 - 1); // TIM2 每 20ms 进一次中断
    usart3_init();      // USART3 初始化

    ArmAction pick = {
        .poses = (ArmPose[]){
            {{1800, 2400, 1650, SERVO3_LOOSE}, 2000},
            {{1900, 2400, 1650, SERVO3_TIGHT}, 2000},
            {{1500, 1500, 1650, SERVO3_TIGHT}, 2000}
        },
        .step_count = 3
    };

    ArmAction direct = {
        .poses = (ArmPose[]){
            {{1730, 2100, 1650, SERVO3_TIGHT}, 2000}
        },
        .step_count = 1
    };

    ArmAction place = {
        .poses = (ArmPose[]){
            {{1700, 2100, 1650, SERVO3_LOOSE}, 2000},
            {{1700, 2100, 1650, SERVO3_TIGHT}, 2000},
            {{1500, 2100, 1650, SERVO3_TIGHT}, 2000}, //抬起
            {{1750, 1800, 1650, SERVO3_TIGHT}, 2000}, //套
            {{1770, 1800, 1650, SERVO3_TIGHT}, 2000}, //缓慢靠近
            {{1600, 2100, 1650, SERVO3_LOOSE}, 2000}  //松开
        },
        .step_count = 6
    };

    ArmAction acts[3] = {pick, direct, place};

    static u8 last_key_state = 0;
    static u8 cur_key_state = 0;
    u8 act_index = 0;
    const ArmAction *current_action = 0;
    
    char rx_buf[64];

    while (1)
    {
        /*cur_key_state = key_read(); // 读取按键状态
        if(last_key_state == 0 && cur_key_state == 1){ // 按键按下
            servo_pulse +=20;
            if(servo_pulse > 2500){
                servo_pulse = 500;
            }
            pwmServo_angle_set(servo_id, servo_pulse, 1000);
        }
        last_key_state = cur_key_state;*/
        
        /*if(usart3_read_line(rx_buf, sizeof(rx_buf))){
            if(usart3_parse_pulses(rx_buf, &temp_pose)){
                do_pose(&temp_pose); // 执行串口解析出的姿态
                usart3_send_string("OK!\n");
            }
        }*/

        cur_key_state = key_read();

        if (last_key_state == 0 && cur_key_state == 1)
        {
            if (hand_busy == 0)
            {
                current_action = &acts[act_index];
                hand_busy = 1;

                act_index++;
                if (act_index >= 3) {
                    act_index = 0;
                }
            }
        }

        last_key_state = cur_key_state;

        if (current_action != 0)
        {
            handle_action(current_action);

            if (hand_busy == 0)
            {
                current_action = 0;
            }
        }
    }
}




/* 检查舵机是否全部到位，1表示未到位，0表示全部到位 */
static u8 check_dj_state(void)
{
    u8 i;

    for (i = 0; i < DJ_NUM; i++)
    {
        if (duoji_doing[i].inc != 0)
        {
            return 1;   // 还有舵机在动
        }
    }

    return 0;           // 全部到位
}

static void do_pose(const ArmPose *pose)
{
    u8 i;
    for(i=0; i<DJ_NUM; i++){
        pwmServo_angle_set(i, pose->pulse[i], pose->time_ms);
    }
}

static void handle_action(const ArmAction *action)
{
    /*状态机控制变量*/
    static u8 g_current_step = 0; // 当前机械臂动作步骤
    static u8 g_step_started = 0; // 当前步骤是否已开始执行
    
    if(action == 0){
        return;
    }

    if(g_current_step >= action->step_count){
        g_current_step = 0; // 重置步骤计数器
        g_step_started = 0; // 重置步骤开始标志
        hand_busy = 0; // 标记机械臂空闲
        return;
    }
    if(!g_step_started){
        hand_busy = 1;
        do_pose(&(action->poses[g_current_step]));
        g_step_started = 1;
    }else if(check_dj_state() == 0){
        g_current_step++;
        g_step_started = 0;
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
