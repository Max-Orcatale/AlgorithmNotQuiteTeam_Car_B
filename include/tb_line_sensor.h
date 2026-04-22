#ifndef __TB_LINE_SENSOR_H__
#define __TB_LINE_SENSOR_H__

#include "car_config.h"
#include "tb_type.h"

typedef struct
{
    u8 bit[8];
    u8 raw_byte;
} LineSensorData_t;

typedef enum
{
    LINE_SENSOR_ERR_NONE = 0,
    LINE_SENSOR_ERR_NULL_PTR,
    LINE_SENSOR_ERR_ADDR_WRITE_NACK,
    LINE_SENSOR_ERR_REG_NACK,
    LINE_SENSOR_ERR_ADDR_READ_NACK,
    LINE_SENSOR_ERR_ADJUST_ADDR_NACK,
    LINE_SENSOR_ERR_ADJUST_REG_NACK,
    LINE_SENSOR_ERR_ADJUST_DATA_NACK
} LineSensorError_t;

void tb_line_sensor_init(void);
HAL_StatusTypeDef LineSensor_SetAdjustMode(u8 enable);
HAL_StatusTypeDef LineSensor_Read(LineSensorData_t *data);
LineSensorError_t LineSensor_GetLastError(void);
const char *LineSensor_GetLastErrorString(void);
u8 LineSensor_GetSdaLevel(void);
u8 LineSensor_GetSclLevel(void);
u8 LineSensor_ProbeAddress(u8 addr_7bit);

#endif
