#include "line_follow_ctrl.h"

#include "tb_motor.h"

static const int8_t s_weight[8] = {-7, -4, -3, -1, 1, 3, 4, 7};

typedef struct
{
    int16_t kp;
    int16_t kd;
} FollowTune_t;

static const FollowTune_t s_follow_forward_tunes[] = {
    {20, 50}
};

/* 当前巡线模块恢复为：1 表示检测到黑线，0 表示白底。 */
static uint8_t line_sensor_is_black(uint8_t bit_value)
{
    return (bit_value == 1U) ? 1U : 0U;
}

typedef struct
{
    LineFollowCtrl_t follow;
    const Route_t *active_route;
    uint16_t current_step;
    uint16_t cells_in_step;
    uint8_t last_all_black;
    uint32_t last_black_tick;
    RouteRunnerState_t state;
    TurnAction_t pending_turn;
    uint8_t finish_after_turn;
    uint32_t preturn_end_tick;
    uint32_t turn_end_tick;
} RouteRunnerCtrl_t;

typedef struct
{
    uint8_t active;
    uint32_t start_tick;
    uint32_t duration_ms;
    int16_t speed;
} ForwardRunnerCtrl_t;

typedef struct
{
    uint8_t active;
    uint8_t direction;
    uint32_t start_tick;
    uint32_t duration_ms;
    int16_t speed;
} StrafeRunnerCtrl_t;

typedef struct
{
    uint8_t active;
    uint32_t start_tick;
    uint32_t duration_ms;
    int16_t speed;
    LineFollowCtrl_t follow;
} FollowForwardRunnerCtrl_t;

typedef struct
{
    uint8_t active;
    uint32_t start_tick;
} AdjustRunnerCtrl_t;

static RouteRunnerCtrl_t s_runner;
static ForwardRunnerCtrl_t s_forward;
static FollowForwardRunnerCtrl_t s_follow_forward;
static StrafeRunnerCtrl_t s_strafe;
static AdjustRunnerCtrl_t s_adjust;

static void follow_forward_apply_tune(LineFollowCtrl_t *ctrl)
{
    if (ctrl == 0)
    {
        return;
    }

    ctrl->kp = s_follow_forward_tunes[0].kp;
    ctrl->kd = s_follow_forward_tunes[0].kd;
}

static int16_t route_get_turn_speed(void)
{
    return (ROUTE_TURN_SPEED >= FOLLOW_BASE_SPEED) ? ROUTE_TURN_SPEED : FOLLOW_BASE_SPEED;
}

static int16_t line_follow_compute_error(const LineSensorData_t *data, uint8_t *valid)
{
    int16_t sum_w = 0;
    int16_t sum_n = 0;
    uint8_t i;

    for (i = 0; i < 8U; i++)
    {
        if (line_sensor_is_black(data->bit[i]) != 0U)
        {
            sum_w += s_weight[i];
            sum_n++;
        }
    }

    if (sum_n == 0)
    {
        *valid = 0U;
        return 0;
    }

    *valid = 1U;
    return (int16_t)(sum_w / sum_n);
}

static uint8_t route_is_all_black(const LineSensorData_t *data)
{
    if (data == 0)
    {
        return 0U;
    }

    if ((line_sensor_is_black(data->bit[2]) != 0U) &&
        (line_sensor_is_black(data->bit[3]) != 0U) &&
        (line_sensor_is_black(data->bit[5]) != 0U) &&
        (line_sensor_is_black(data->bit[7]) != 0U))
    {
        return 1U;
    }

    return 0U;
}

static void route_start_turn(TurnAction_t turn)
{
    uint32_t now = HAL_GetTick();
    int16_t turn_speed = route_get_turn_speed();

    switch (turn)
    {
    case TURN_LEFT:
        tb_motor_stop_all();
        tb_motor_spin_left(turn_speed);
        s_runner.turn_end_tick = now + ROUTE_LEFT_TURN_MS;
        s_runner.state = ROUTE_RUNNER_TURNING;
        break;

    case TURN_RIGHT:
        tb_motor_stop_all();
        tb_motor_spin_right(turn_speed);
        s_runner.turn_end_tick = now + ROUTE_RIGHT_TURN_MS;
        s_runner.state = ROUTE_RUNNER_TURNING;
        break;

    case TURN_BACK:
        tb_motor_stop_all();
        tb_motor_spin_right(turn_speed);
        s_runner.turn_end_tick = now + ROUTE_BACK_TURN_MS;
        s_runner.state = ROUTE_RUNNER_TURNING;
        break;

    case TURN_STRAIGHT:
    default:
        s_runner.state = ROUTE_RUNNER_FOLLOWING;
        break;
    }
}

static void route_begin(const Route_t *route)
{
    LineFollow_Init(&s_runner.follow);
    s_runner.active_route = route;
    s_runner.current_step = 0U;
    s_runner.cells_in_step = 0U;
    s_runner.last_all_black = 0U;
    s_runner.last_black_tick = 0U;
    s_runner.pending_turn = TURN_STRAIGHT;
    s_runner.finish_after_turn = 0U;
    s_runner.preturn_end_tick = 0U;
    s_runner.turn_end_tick = 0U;
    s_runner.state = ROUTE_RUNNER_FOLLOWING;
}

void LineFollow_Init(LineFollowCtrl_t *ctrl)
{
    if (ctrl == 0)
    {
        return;
    }

    ctrl->base_speed = FOLLOW_BASE_SPEED;
    ctrl->kp = FOLLOW_KP;
    ctrl->kd = FOLLOW_KD;
    ctrl->last_error = 0;
}

void LineFollow_Update(LineFollowCtrl_t *ctrl, const LineSensorData_t *data)
{
    int16_t error;
    int16_t d_error;
    int32_t turn;
    int16_t left_speed;
    int16_t right_speed;
    uint8_t valid = 0U;

    if ((ctrl == 0) || (data == 0))
    {
        return;
    }

    error = line_follow_compute_error(data, &valid);
    if (valid == 0U)
    {
        tb_motor_set_all(-FOLLOW_LOST_LINE_BRAKE_SPEED,
                         -FOLLOW_LOST_LINE_BRAKE_SPEED,
                         -FOLLOW_LOST_LINE_BRAKE_SPEED,
                         -FOLLOW_LOST_LINE_BRAKE_SPEED);
        return;
    }

    d_error = (int16_t)(error - ctrl->last_error);
    ctrl->last_error = error;

    turn = (int32_t)ctrl->kp * error + (int32_t)ctrl->kd * d_error;
    left_speed = (int16_t)(ctrl->base_speed + turn);
    right_speed = (int16_t)(ctrl->base_speed - turn);

    tb_motor_set_all(right_speed, left_speed, right_speed, left_speed);
}

void route_runner_init(void)
{
    LineFollow_Init(&s_runner.follow);
    s_runner.active_route = 0;
    s_runner.current_step = 0U;
    s_runner.cells_in_step = 0U;
    s_runner.last_all_black = 0U;
    s_runner.last_black_tick = 0U;
    s_runner.pending_turn = TURN_STRAIGHT;
    s_runner.finish_after_turn = 0U;
    s_runner.preturn_end_tick = 0U;
    s_runner.state = ROUTE_RUNNER_IDLE;
    s_runner.turn_end_tick = 0U;
}

void forward_runner_init(void)
{
    s_forward.active = 0U;
    s_forward.start_tick = 0U;
    s_forward.duration_ms = 0U;
    s_forward.speed = 0;

    s_follow_forward.active = 0U;
    s_follow_forward.start_tick = 0U;
    s_follow_forward.duration_ms = 0U;
    s_follow_forward.speed = 0;
    LineFollow_Init(&s_follow_forward.follow);
    follow_forward_apply_tune(&s_follow_forward.follow);

    s_strafe.active = 0U;
    s_strafe.direction = 0U;
    s_strafe.start_tick = 0U;
    s_strafe.duration_ms = 0U;
    s_strafe.speed = 0;
}

uint8_t run_route(const Route_t *route)
{
    LineSensorData_t sensor_data;
    uint8_t all_black;
    uint32_t now;
    TurnAction_t turn;

    if ((route == 0) || (route->steps == 0) || (route->step_count == 0U))
    {
        return 1U;
    }

    if ((s_runner.state == ROUTE_RUNNER_IDLE) ||
        (s_runner.state == ROUTE_RUNNER_FINISHED) ||
        (s_runner.active_route != route))
    {
        route_begin(route);
    }

    if (LineSensor_Read(&sensor_data) != HAL_OK)
    {
        s_runner.state = ROUTE_RUNNER_ERROR;
        tb_motor_stop_all();
        return 0U;
    }

    now = HAL_GetTick();

    if (s_runner.state == ROUTE_RUNNER_TURNING)
    {
        if ((int32_t)(now - s_runner.turn_end_tick) >= 0)
        {
            tb_motor_stop_all();
            if (s_runner.finish_after_turn != 0U)
            {
                s_runner.finish_after_turn = 0U;
                s_runner.state = ROUTE_RUNNER_FINISHED;
                return 1U;
            }
            s_runner.state = ROUTE_RUNNER_FOLLOWING;
        }
        return 0U;
    }

    if (s_runner.state == ROUTE_RUNNER_PRETURN)
    {
        LineFollow_Update(&s_runner.follow, &sensor_data);

        if ((int32_t)(now - s_runner.preturn_end_tick) >= 0)
        {
            route_start_turn(s_runner.pending_turn);
        }

        s_runner.last_all_black = route_is_all_black(&sensor_data);
        return 0U;
    }

    if (s_runner.state == ROUTE_RUNNER_ERROR)
    {
        tb_motor_stop_all();
        return 0U;
    }

    if (s_runner.state == ROUTE_RUNNER_FINISHED)
    {
        tb_motor_stop_all();
        return 1U;
    }

    LineFollow_Update(&s_runner.follow, &sensor_data);

    all_black = route_is_all_black(&sensor_data);
    if ((all_black == 1U) &&
        (s_runner.last_all_black == 0U) &&
        ((now - s_runner.last_black_tick) > ROUTE_BLACK_DEBOUNCE_MS))
    {
        s_runner.last_black_tick = now;
        s_runner.cells_in_step++;

        if (s_runner.cells_in_step >= route->steps[s_runner.current_step].cells)
        {
            uint8_t finish_after_step;

            turn = route->steps[s_runner.current_step].turn;
            s_runner.cells_in_step = 0U;
            finish_after_step = ((uint16_t)(s_runner.current_step + 1U) >= route->step_count) ? 1U : 0U;
            s_runner.current_step++;

            if (turn == TURN_STRAIGHT)
            {
                if (finish_after_step != 0U)
                {
                    s_runner.state = ROUTE_RUNNER_FINISHED;
                    tb_motor_stop_all();
                    return 1U;
                }
                s_runner.state = ROUTE_RUNNER_FOLLOWING;
            }
            else
            {
                s_runner.pending_turn = turn;
                s_runner.finish_after_turn = finish_after_step;
                s_runner.preturn_end_tick = now + ROUTE_PRETURN_FORWARD_MS;
                s_runner.state = ROUTE_RUNNER_PRETURN;
            }
        }
    }

    s_runner.last_all_black = all_black;
    return 0U;
}

void route_runner_abort(void)
{
    s_runner.active_route = 0;
    s_runner.current_step = 0U;
    s_runner.cells_in_step = 0U;
    s_runner.last_all_black = 0U;
    s_runner.last_black_tick = 0U;
    s_runner.pending_turn = TURN_STRAIGHT;
    s_runner.preturn_end_tick = 0U;
    s_runner.turn_end_tick = 0U;
    s_runner.state = ROUTE_RUNNER_IDLE;
    tb_motor_stop_all();
}

RouteRunnerState_t route_runner_get_state(void)
{
    return s_runner.state;
}

uint16_t route_runner_get_step_index(void)
{
    return s_runner.current_step;
}

uint16_t route_runner_get_cells_in_step(void)
{
    return s_runner.cells_in_step;
}

uint8_t run_forward_ms(uint32_t duration_ms, int16_t speed)
{
    uint32_t now;

    if (duration_ms == 0U)
    {
        tb_motor_stop_all();
        return 1U;
    }

    now = HAL_GetTick();

    if ((s_forward.active == 0U) ||
        (s_forward.duration_ms != duration_ms) ||
        (s_forward.speed != speed))
    {
        s_forward.active = 1U;
        s_forward.start_tick = now;
        s_forward.duration_ms = duration_ms;
        s_forward.speed = speed;
        tb_motor_set_all(speed, speed, speed, speed);
        return 0U;
    }

    if ((now - s_forward.start_tick) >= s_forward.duration_ms)
    {
        tb_motor_stop_all();
        s_forward.active = 0U;
        return 1U;
    }

    tb_motor_set_all(speed, speed, speed, speed);
    return 0U;
}

uint8_t run_forward_while_follow_line(uint32_t duration_ms, int16_t speed)
{
    uint32_t now;
    LineSensorData_t sensor_data;

    if (duration_ms == 0U)
    {
        tb_motor_stop_all();
        return 1U;
    }

    now = HAL_GetTick();

    if ((s_follow_forward.active == 0U) ||
        (s_follow_forward.duration_ms != duration_ms) ||
        (s_follow_forward.speed != speed))
    {
        s_follow_forward.active = 1U;
        s_follow_forward.start_tick = now;
        s_follow_forward.duration_ms = duration_ms;
        s_follow_forward.speed = speed;
        LineFollow_Init(&s_follow_forward.follow);
        s_follow_forward.follow.base_speed = speed;
        follow_forward_apply_tune(&s_follow_forward.follow);
    }

    if ((now - s_follow_forward.start_tick) >= s_follow_forward.duration_ms)
    {
        tb_motor_stop_all();
        s_follow_forward.active = 0U;
        return 1U;
    }

    if (LineSensor_Read(&sensor_data) == HAL_OK)
    {
        s_follow_forward.follow.base_speed = speed;
        follow_forward_apply_tune(&s_follow_forward.follow);
        LineFollow_Update(&s_follow_forward.follow, &sensor_data);
    }
    else
    {
        tb_motor_set_all(speed, speed, speed, speed);
    }

    return 0U;
}

static uint8_t run_strafe_ms(uint32_t duration_ms, int16_t speed, uint8_t direction)
{
    uint32_t now;

    if (duration_ms == 0U)
    {
        tb_motor_stop_all();
        return 1U;
    }

    now = HAL_GetTick();

    if ((s_strafe.active == 0U) ||
        (s_strafe.duration_ms != duration_ms) ||
        (s_strafe.speed != speed) ||
        (s_strafe.direction != direction))
    {
        s_strafe.active = 1U;
        s_strafe.direction = direction;
        s_strafe.start_tick = now;
        s_strafe.duration_ms = duration_ms;
        s_strafe.speed = speed;
    }

    if ((now - s_strafe.start_tick) >= s_strafe.duration_ms)
    {
        tb_motor_stop_all();
        s_strafe.active = 0U;
        return 1U;
    }

    if (direction == 0U)
    {
        tb_motor_strafe_left(speed);
    }
    else
    {
        tb_motor_strafe_right(speed);
    }

    return 0U;
}

uint8_t run_strafe_left_ms(uint32_t duration_ms, int16_t speed)
{
    return run_strafe_ms(duration_ms, speed, 0U);
}

uint8_t run_strafe_right_ms(uint32_t duration_ms, int16_t speed)
{
    return run_strafe_ms(duration_ms, speed, 1U);
}

uint8_t adjust_position(void)
{
    LineSensorData_t sensor_data;
    uint32_t now;

    if (LineSensor_Read(&sensor_data) != HAL_OK)
    {
        tb_motor_stop_all();
        return 0U;
    }

    now = HAL_GetTick();

    if (s_adjust.active == 0U)
    {
        s_adjust.active = 1U;
        s_adjust.start_tick = now;
    }

    if ((now - s_adjust.start_tick) >= 1000U)
    {
        s_adjust.active = 0U;
        tb_motor_stop_all();
        return 1U;
    }

    if ((line_sensor_is_black(sensor_data.bit[0]) == 0U) &&
        (line_sensor_is_black(sensor_data.bit[1]) == 0U) &&
        (line_sensor_is_black(sensor_data.bit[2]) == 0U) &&
        (line_sensor_is_black(sensor_data.bit[3]) != 0U) &&
        (line_sensor_is_black(sensor_data.bit[4]) != 0U) &&
        (line_sensor_is_black(sensor_data.bit[5]) == 0U) &&
        (line_sensor_is_black(sensor_data.bit[6]) == 0U) &&
        (line_sensor_is_black(sensor_data.bit[7]) == 0U))
    {
        s_adjust.active = 0U;
        tb_motor_stop_all();
        return 1U;
    }

    if ((line_sensor_is_black(sensor_data.bit[0]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[1]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[2]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[3]) != 0U))
    {
        tb_motor_spin_left(ADJUST_POSITION_SPEED);
        return 0U;
    }

    if ((line_sensor_is_black(sensor_data.bit[4]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[5]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[6]) != 0U) ||
        (line_sensor_is_black(sensor_data.bit[7]) != 0U))
    {
        tb_motor_spin_right(ADJUST_POSITION_SPEED);
        return 0U;
    }

    tb_motor_stop_all();
    return 0U;
}

void forward_runner_abort(void)
{
    s_forward.active = 0U;
    s_forward.start_tick = 0U;
    s_forward.duration_ms = 0U;
    s_forward.speed = 0;

    s_follow_forward.active = 0U;
    s_follow_forward.start_tick = 0U;
    s_follow_forward.duration_ms = 0U;
    s_follow_forward.speed = 0;

    s_strafe.active = 0U;
    s_strafe.direction = 0U;
    s_strafe.start_tick = 0U;
    s_strafe.duration_ms = 0U;
    s_strafe.speed = 0;
    tb_motor_stop_all();
}
