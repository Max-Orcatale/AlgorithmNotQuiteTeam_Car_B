#include "main.h"
#include "tb_delay.h"
#include "tb_global.h"
#include "tb_gpio.h"
#include "tb_rcc.h"
#include "tb_timer.h"

static void Servo_SetPulseUs(uint16_t us);
static void Servo_SetAngleRelative(int16_t angle);

int main(void)
{
    HAL_Init();         //HAL 库初始化
    tb_rcc_init();      //系统时钟初始化
    tb_global_init();   //全局状态初始化
    tb_gpio_init();     //版极 GPIO 基础初始化
    dj_io_init();       //舵机相关GPIO初始化
    pwmServo_init();    //PWM 初始化

    while (1)
    {
        // 左30°
        Servo_SetAngleRelative(-30);
        tb_delay_ms(1000);

        // 右30°
        Servo_SetAngleRelative(30);
        tb_delay_ms(1000);
    }
}

// 舵机控制函数，设置脉宽（单位：微秒）
static void Servo_SetAngleRelative(int16_t angle)
{
    if (angle < -90) angle = -90;
    if (angle > 90)  angle = 90;

    uint16_t pulse = 1500 + (angle * 500) / 90;
    pwmServo_set(0, pulse);
}

// 错误处理函数
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
