/* src/tb_global.c */
#include "tb_global.h"

duoji_t duoji_doing[DJ_NUM];   //4 路舵机状态数组
u32 systick_ms;

void tb_global_init(void)
{
    u8 i;

    for (i = 0; i < DJ_NUM; i++)
    {
        duoji_doing[i].valid = 0;
        duoji_doing[i].aim = 1500;
        duoji_doing[i].time = 1000;
        duoji_doing[i].cur = 1500;
        duoji_doing[i].inc = 0;
    }

    systick_ms = 0;
}

uint16_t str_contain_str(unsigned char *str, unsigned char *str2)
{
    unsigned char *str_temp, *str_temp2;

    str_temp = str;
    str_temp2 = str2;

    while (*str_temp)
    {
        if (*str_temp == *str_temp2)
        {
            while (*str_temp2)
            {
                if (*str_temp++ != *str_temp2++)
                {
                    str_temp = str_temp - (str_temp2 - str2) + 1;
                    str_temp2 = str2;
                    break;
                }
            }

            if (!*str_temp2)
            {
                return (uint16_t)(str_temp - str);
            }
        }
        else
        {
            str_temp++;
        }
    }

    return 0;
}
