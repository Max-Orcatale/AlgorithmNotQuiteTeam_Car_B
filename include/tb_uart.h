#include "tb_type.h"
#include "main.h"



void usart3_init(void);//初始化uart3和相关GPIO

u8 usart3_read_line(char *buf, u16 max_len);//从串口读取一行文本，存入buf，最大长度为max_len，返回1表示成功读取到一行，0表示没有数据或出错

void usart3_send_string(const char *str);//向串口发送字符串
u8 uart_send(const char *str);//主循环里反复调用时，同一条字符串只发送一次
void uart_send_reset(void);//复位一次性发送状态，允许再次发送同一条字符串

u8 usart3_parse_pulses(const char *str, ArmPose *pose);//解析4个脉冲值，并固定生成 time_ms=500 的姿态
