/* include/tb_timer.h */
#ifndef __TB_TIMER_H__
#define __TB_TIMER_H__

#include "tb_type.h"
#include "main.h"


/* 初始化 TIM2，用作周期更新中断 */
void TIM2_Int_Init(u16 arr, u16 psc);

/* 按目标值推进某一路舵机 */
void duoji_inc_handle(u8 index);

/*pwm舵机设置统一接口*/
void pwmServo_angle_set(u8 index, int aim, int time);


#endif
