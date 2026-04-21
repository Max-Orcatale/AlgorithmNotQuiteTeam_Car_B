#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"
#include "tb_type.h"

#define DJ_NUM 4

typedef struct {
    u16 pulse[DJ_NUM];
    u16 time_ms;
} ArmPose;

typedef struct {
    const ArmPose *poses;  // 指向一组姿态
    u8 step_count;         // 这一组里有多少步
} ArmAction;


void Error_Handler(void);

#endif
