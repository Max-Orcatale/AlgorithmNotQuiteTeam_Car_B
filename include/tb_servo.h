#ifndef __TB_SERVO_H__
#define __TB_SERVO_H__

#include "main.h"
#include "tb_type.h"

extern const ArmAction pick;
extern const ArmAction direct;
extern const ArmAction place;

void tb_servo_init(void);
void duoji_inc_handle(u8 index);
void pwmServo_angle_set(u8 index, int aim, int time);

void tb_servo_demo_init(void);
void tb_servo_demo_update(void);
u8 tb_servo_start_action(const ArmAction *action);
void tb_servo_update(void);
u8 tb_servo_is_busy(void);

#endif
