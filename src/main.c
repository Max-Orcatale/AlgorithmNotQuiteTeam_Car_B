#include "main.h"
#include "tb_delay.h"
#include "tb_global.h"
#include "tb_gpio.h"
#include "tb_rcc.h"
#include "tb_timer.h"
#include "tb_uart.h"

#define SERVO3_MAX 1300
#define SERVO3_TIGHT 1650
#define ARM_TASK_STEP_NUM 5   // 机械臂动作步骤数，根据实际任务调整

ArmPose g_arm_task[ARM_TASK_STEP_NUM] = {
    // 这里应该有 ARM_TASK_STEP_NUM 个 ArmPose 结构体，代表机械臂的动作序列
};      

/*状态机控制变量*/
static u8 g_current_step = 0; // 当前机械臂动作步骤
static u8 g_step_started = 0; // 当前步骤是否已开始执行

/*机械臂层控制函数*/
static void do_pose(const ArmPose *pose);
static u8 check_dj_state(void);
static void handle_action(void);


int main(void)
{
    HAL_Init();         //HAL 库初始化
    tb_rcc_init();      //系统时钟初始化
    tb_global_init();   //全局状态初始化
    tb_gpio_init();     //版极 GPIO 基础初始化
    dj_io_init();       //舵机相关GPIO初始化
    TIM2_Int_Init(20000 - 1, 72 - 1); // TIM2 每 20ms 进一次中断
    usart3_init();      // USART3 初始化

    //static u16 servo_pulse = SERVO3_MAX;
    //static u8 servo_id = 3;
    //static u8 last_key_state = 0;
    //static u8 cur_key_state = 0;
    //pwmServo_angle_set(servo_id, servo_pulse, 1000);
    char rx_buf[64];
    ArmPose temp_pose = {{1500, 1000, 1500, 1500}, 500};

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
        
        if(usart3_read_line(rx_buf, sizeof(rx_buf))){
            if(usart3_parse_pulses(rx_buf, &temp_pose)){
                do_pose(&temp_pose); // 执行串口解析出的姿态
                usart3_send_string("OK!\n");
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

static void handle_action(void)
{
    if(g_current_step >= ARM_TASK_STEP_NUM){
        return; // 所有步骤完成
    }
    if(!g_step_started){
        do_pose(&g_arm_task[g_current_step]);
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
