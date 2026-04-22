#ifndef __CAR_CONFIG_H__
#define __CAR_CONFIG_H__

#include "stm32f1xx_hal.h"

#define DJ_NUM 4

/* 软件 I2C 巡线模块 */
#define LINE_SDA_GPIO_PORT GPIOA
#define LINE_SDA_PIN       GPIO_PIN_4
#define LINE_SCL_GPIO_PORT GPIOA
#define LINE_SCL_PIN       GPIO_PIN_5

#define LINE_SENSOR_I2C_ADDR_7BIT   0x12U
#define LINE_SENSOR_REG_ADJUST_MODE 0x01U
#define LINE_SENSOR_REG_DATA        0x30U

/* 电机 PWM 参数 */
#define MOTOR_PWM_MAX        2000
#define MOTOR_PWM_PRESCALER  (0U)
#define MOTOR_PWM_PERIOD     (3599U)

/* 巡线控制参数 */
#define FOLLOW_BASE_SPEED             1200
#define FOLLOW_KP                     380
#define FOLLOW_KD                     220
#define FOLLOW_LOST_LINE_BRAKE_SPEED  600

/* 路线控制参数 */
#define ROUTE_BLACK_DEBOUNCE_MS 120U
#define ROUTE_LEFT_TURN_MS      360U
#define ROUTE_RIGHT_TURN_MS     360U
#define ROUTE_BACK_TURN_MS      700U
#define ROUTE_TURN_SPEED        950

/* 舵机调度定时器：改用 TIM6，避免占用编码器的 TIM2 */
#define SERVO_TIMER_PRESCALER (72U - 1U)
#define SERVO_TIMER_PERIOD    (20000U - 1U)
#define SERVO_SLOT_US         5000U

#endif
