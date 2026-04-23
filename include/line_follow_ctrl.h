#ifndef __LINE_FOLLOW_CTRL_H__
#define __LINE_FOLLOW_CTRL_H__

#include "tb_line_sensor.h"

typedef struct
{
    int16_t base_speed;
    int16_t kp;
    int16_t kd;
    int16_t last_error;
} LineFollowCtrl_t;

typedef enum
{
    TURN_STRAIGHT = 0,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_BACK
} TurnAction_t;

typedef struct
{
    uint16_t cells;
    TurnAction_t turn;
} RouteStep_t;

typedef struct
{
    const RouteStep_t *steps;
    uint16_t step_count;
} Route_t;

typedef enum
{
    ROUTE_RUNNER_IDLE = 0,
    ROUTE_RUNNER_FOLLOWING,
    ROUTE_RUNNER_PRETURN,
    ROUTE_RUNNER_TURNING,
    ROUTE_RUNNER_FINISHED,
    ROUTE_RUNNER_ERROR
} RouteRunnerState_t;

void LineFollow_Init(LineFollowCtrl_t *ctrl);
void LineFollow_Update(LineFollowCtrl_t *ctrl, const LineSensorData_t *data);
void route_runner_init(void);
uint8_t run_route(const Route_t *route);
void route_runner_abort(void);
RouteRunnerState_t route_runner_get_state(void);
uint16_t route_runner_get_step_index(void);
uint16_t route_runner_get_cells_in_step(void);

void forward_runner_init(void);
uint8_t run_forward_ms(uint32_t duration_ms, int16_t speed);
void forward_runner_abort(void);

#endif
