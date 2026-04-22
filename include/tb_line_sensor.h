#ifndef __TB_LINE_SENSOR_H__
#define __TB_LINE_SENSOR_H__

#include "car_config.h"
#include "tb_type.h"

typedef struct
{
    u8 bit[8];
    u8 raw_byte;
} LineSensorData_t;

void tb_line_sensor_init(void);
HAL_StatusTypeDef LineSensor_SetAdjustMode(u8 enable);
HAL_StatusTypeDef LineSensor_Read(LineSensorData_t *data);

#endif
