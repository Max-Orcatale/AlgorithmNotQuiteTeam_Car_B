/* include/tb_gpio.h */
#ifndef __TB_GPIO_H__
#define __TB_GPIO_H__

#include "main.h"
#include "tb_type.h"

/* C24+v5 前四路舵机接口定义 */
#define SERVO0_GPIO_PORT     GPIOB
#define SERVO0_GPIO_PIN      GPIO_PIN_9
#define SERVO0_TIM_INSTANCE  TIM4
#define SERVO0_TIM_CHANNEL   TIM_CHANNEL_4

#define SERVO1_GPIO_PORT     GPIOB
#define SERVO1_GPIO_PIN      GPIO_PIN_8
#define SERVO1_TIM_INSTANCE  TIM4
#define SERVO1_TIM_CHANNEL   TIM_CHANNEL_3

#define SERVO2_GPIO_PORT     GPIOB
#define SERVO2_GPIO_PIN      GPIO_PIN_5
#define SERVO2_TIM_INSTANCE  TIM3
#define SERVO2_TIM_CHANNEL   TIM_CHANNEL_2

#define SERVO3_GPIO_PORT     GPIOB
#define SERVO3_GPIO_PIN      GPIO_PIN_4
#define SERVO3_TIM_INSTANCE  TIM3
#define SERVO3_TIM_CHANNEL   TIM_CHANNEL_1

/* SERVO2/SERVO3 使用 TIM3 部分重映射 */
#define SERVO23_USE_TIM3_PARTIAL_REMAP 1

/* SERVO3 的 PB4 默认被 JTAG 占用，使用前通常要关闭 JTAG、保留 SWD */
#define SERVO3_USE_PB4_REQUIRES_NOJTAG 1

/* 板级 GPIO/AFIO 基础初始化 */
void tb_gpio_init(void);

/* 舵机相关 GPIO 初始化，保留原函数名 */
void dj_io_init(void);

#endif
