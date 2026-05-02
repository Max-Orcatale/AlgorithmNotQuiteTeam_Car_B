#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f1xx_hal.h"
#include "car_config.h"
#include "tb_type.h"

#define EXIT_INITAIL_ZONE_MS    1910
#define EXIT_INITIAL_ZONE_SPEED 1300
#define APPROACH_RING_MS        1220
#define APPROACH_RING_SPEED     1300
#define APPROACH_POLE_MS        1400
#define APPROACH_POLE_SPEED     1200
#define BACKWARD_MS             2400
#define BACKWARD_SPEED          -1200

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
