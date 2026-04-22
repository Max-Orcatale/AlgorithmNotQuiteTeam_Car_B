/* include/tb_gpio.h */
#ifndef __TB_GPIO_H__
#define __TB_GPIO_H__

#include "main.h"
#include "tb_type.h"

/* OpenRF1 v2.2 LED接口定义 */
#define LED_GPIO_PORT    GPIOC
#define LED_GPIO_PIN     GPIO_PIN_4

/* 按键接口定义：当前按 PB2，默认按下为低电平 */
#define KEY_GPIO_PORT    GPIOB
#define KEY_GPIO_PIN     GPIO_PIN_2

/* OpenRF1 v2.2 前四路舵机接口定义 */
#define SERVO0_GPIO_PORT     GPIOC
#define SERVO0_GPIO_PIN      GPIO_PIN_13

#define SERVO1_GPIO_PORT     GPIOC
#define SERVO1_GPIO_PIN      GPIO_PIN_14

#define SERVO2_GPIO_PORT     GPIOC
#define SERVO2_GPIO_PIN      GPIO_PIN_15

#define SERVO3_GPIO_PORT     GPIOC
#define SERVO3_GPIO_PIN      GPIO_PIN_0


/* 板级 GPIO/AFIO 基础初始化 */
void tb_gpio_init(void);

/* 舵机相关 GPIO 初始化，保留原函数名 */
void dj_io_init(void);

/* 设置舵机 GPIO 输出电平，index 0-3 对应 SERVO0-SERVO3 */
void dj_io_set(u8 index, u8 level);

/* 读取按键状态，返回 0 表示按下，1 表示未按下 */
u8 key_read(void);


#endif
