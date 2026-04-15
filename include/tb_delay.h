/* include/tb_delay.h */
#ifndef __TB_DELAY_H__
#define __TB_DELAY_H__

#include "tb_type.h"

/* 毫秒延时，内部直接调用 HAL_Delay */
void tb_delay_ms(u32 ms);

#endif
