/* include/tb_timer.h */
#ifndef __TB_TIMER_H__
#define __TB_TIMER_H__

#include "tb_type.h"
#include "main.h"

/* 当前已迁移的单舵机 PWM 初始化 */
void pwmServo_init(void);

/* 保留原函数名：初始化 TIM2，用作周期更新中断 */
void TIM2_Int_Init(u16 arr, u16 psc);

/* 保留原函数名：按目标值推进某一路舵机 */
void duoji_inc_handle(u8 index);

/* 直接设置某一路舵机输出脉宽 */
void pwmServo_set(u8 index, u16 pulse);

#endif
