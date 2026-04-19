#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"
#include "tb_type.h"

typedef struct {
    u16 pulse[4];
    u16 time_ms;
} ArmPose;


void Error_Handler(void);

#endif
