#ifndef __TB_MOTOR_H__
#define __TB_MOTOR_H__

#include "tb_type.h"

void tb_motor_init(void);
void tb_motor_set_speed(u8 motor_id, int16_t speed);
void tb_motor_set_all(int16_t speed1, int16_t speed2, int16_t speed3, int16_t speed4);
void tb_motor_stop_all(void);

#endif
