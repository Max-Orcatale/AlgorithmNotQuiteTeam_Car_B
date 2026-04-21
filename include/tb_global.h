/* include/tb_global.h */
#ifndef __TB_GLOBAL_H__
#define __TB_GLOBAL_H__

#include "tb_type.h"

#include "main.h"

/* 单个舵机的运动状态 */
typedef struct {
    uint8_t  valid; /* 预留字段，可先不使用 */
    uint16_t aim;   /* 目标脉宽 */
    uint16_t time;  /* 运动时间，单位 ms */
    float    cur;   /* 当前脉宽 */
    float    inc;   /* 每次更新增量 */
} duoji_t;

/* 全局状态初始化 */
void tb_global_init(void);

/* 老工程里的字符串工具函数，保留 */
uint16_t str_contain_str(unsigned char *str, unsigned char *str2);

/* 4 路舵机状态数组 */
extern duoji_t duoji_doing[DJ_NUM];

/* 系统毫秒计数，可选 */
extern u32 systick_ms;

#endif
