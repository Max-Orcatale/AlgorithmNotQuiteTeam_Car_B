/* src/tb_delay.c */
#include "tb_delay.h"
#include "main.h"

/* 毫秒延时 */
void tb_delay_ms(u32 ms){
    HAL_Delay(ms);
}
