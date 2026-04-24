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
#define LINE_I2C_DELAY_CYCLES       64U
#define LINE_I2C_ACK_TIMEOUT        200U

/* 电机 PWM 参数 */
#define MOTOR_PWM_MAX        2000
#define MOTOR_PWM_PRESCALER  (1U)
#define MOTOR_PWM_PERIOD     (1999U)

/* 电机方向修正：正值表示保持当前方向，负值表示软件反相 */
#define MOTOR1_DIR_SIGN      (-1)
#define MOTOR2_DIR_SIGN      (1)
#define MOTOR3_DIR_SIGN      (-1)
#define MOTOR4_DIR_SIGN      (1)

/* 巡线控制参数 */
#define FOLLOW_BASE_SPEED             1300
#define FOLLOW_KP                     130
#define FOLLOW_KD                     220
#define FOLLOW_LOST_LINE_BRAKE_SPEED  1300

/* 路线控制参数 */
#define ROUTE_BLACK_DEBOUNCE_MS 700U
#define ROUTE_PRETURN_FORWARD_MS 330U
#define ROUTE_LEFT_TURN_MS      1590U
#define ROUTE_RIGHT_TURN_MS     1590U
#define ROUTE_BACK_TURN_MS      3200U
#define ROUTE_TURN_SPEED        1300

/* 位置微调参数 */
#define ADJUST_POSITION_SPEED   1180

/* 舵机调度定时器：用 TIM6 */
#define SERVO_TIMER_PRESCALER (72U - 1U)
#define SERVO_TIMER_PERIOD    (20000U - 1U)
#define SERVO_SLOT_US         5000U

#endif
