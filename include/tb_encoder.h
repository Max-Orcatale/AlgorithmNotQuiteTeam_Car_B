#ifndef __TB_ENCODER_H__
#define __TB_ENCODER_H__

#include "tb_type.h"

void tb_encoder_init(void);
int32_t tb_encoder_get_count(u8 motor_id);
void tb_encoder_reset(u8 motor_id);

#endif
