/* Tinh v_close, TTC, muc nguy co phia sau va dieu kien BOOST. */

#include "app_rear_collision.h"
#include "app_ra_config.h"

#include <stddef.h>

typedef struct
{
    bool distance_valid;
    bool history_ready;
    bool ttc_valid;
    float distance_cm;
    float v_close_cm_s;
    float ttc_s;
    RC_State_t state;
} RC_SensorResult_t;

typedef struct
{
    RC_SensorResult_t result;
    float history_cm[RC_HISTORY_COUNT];
    uint8_t history_count;
    uint8_t write_index;
    uint8_t candidate_samples;
    uint32_t last_sequence;
    RC_State_t candidate_state;
    bool sequence_initialized;
    bool reset_on_recovery;
} RC_SensorRuntime_t;

typedef struct
{
    RC_SensorRuntime_t left;
    RC_SensorRuntime_t right;
    RC_State_t state;
    RC_Side_t side;
    RA_Direction_t last_direction;
    bool data_valid;
} RC_Runtime_t;

static RC_Runtime_t s_rc;

/* Kiem tra gia tri hop le va khong am. */
static bool RC_IsFiniteNonNegative(float value)
{
    return (value == value) && (value >= 0.0f);
}

/* Gioi han gia tri trong khoang cho phep. */
static float RC_Clamp(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

/* Kiem tra cau hinh Rear Collision va BOOST. */
bool RC_IsConfigValid(void)
{
    return
        (RC_HISTORY_COUNT >= 2U) &&
        (RC_SAMPLE_PERIOD_S > 0.0f) &&
        (RC_MONITOR_DIST_CM >= RC_WARNING_DIST_CM) &&
        (RC_WARNING_DIST_CM > RC_DANGER_DIST_CM) &&
        (RC_DANGER_DIST_CM > 0.0f) &&
        (RC_MIN_VCLOSE_CM_S > 0.0f) &&
        (RC_WARNING_TTC_S > RC_DANGER_TTC_S) &&
        (RC_DANGER_TTC_S > 0.0f) &&
        (RC_APPROACH_CONFIRM > 0U) &&
        (RC_WARNING_CONFIRM > 0U) &&
        (RC_DANGER_CONFIRM > 0U) &&
        (RC_CLEAR_CONFIRM > 0U) &&
        (RC_BOOST_MIN_FRONT_CM > 0.0f) &&
        (RC_BOOST_FRONT_MARGIN_CM >= 0.0f) &&
        (RC_BOOST_RATIO > 1.0f);
}

/* Xoa lich su va state cua mot cam bien sau. */
static void RC_ResetSensor(
    RC_SensorRuntime_t *sensor,
    RC_State_t state,
    uint32_t sequence,
    bool sequence_initialized)
{
    uint8_t index;

    if (sensor == NULL)
    {
        return;
    }

    sensor->result.distance_valid = false;
    sensor->result.history_ready = false;
    sensor->result.ttc_valid = false;
    sensor->result.distance_cm = 0.0f;
    sensor->result.v_close_cm_s = 0.0f;
    sensor->result.ttc_s = 0.0f;
    sensor->result.state = state;

    for (index = 0U; index < (uint8_t)RC_HISTORY_COUNT; index++)
    {
        sensor->history_cm[index] = 0.0f;
    }

    sensor->history_count = 0U;
    sensor->write_index = 0U;
    sensor->candidate_samples = 0U;
    sensor->last_sequence = sequence;
    sensor->candidate_state = state;
    sensor->sequence_initialized = sequence_initialized;
    sensor->reset_on_recovery = false;
}

/* Xoa toan bo runtime Rear Collision. */
static void RC_ResetRuntime(
    RA_Direction_t direction,
    RC_State_t state,
    uint32_t left_sequence,
    uint32_t right_sequence,
    bool sequence_initialized)
{
    RC_ResetSensor(
        &s_rc.left,
        state,
        left_sequence,
        sequence_initialized);
    RC_ResetSensor(
        &s_rc.right,
        state,
        right_sequence,
        sequence_initialized);

    s_rc.state = state;
    s_rc.side = RC_SIDE_NONE;
    s_rc.last_direction = direction;
    s_rc.data_valid = false;
}

/* Khoi tao Rear Collision o trang thai WARMUP. */
void RC_Init(void)
{
    RC_ResetRuntime(
        RA_DIR_STOPPED,
        RC_STATE_WARMUP,
        0UL,
        0UL,
        false);
}

/* Doi state sang muc rui ro de so sanh hai ben. */
static uint8_t RC_GetRisk(RC_State_t state)
{
    switch (state)
    {
        case RC_STATE_APPROACHING:
            return 1U;
        case RC_STATE_WARNING:
            return 2U;
        case RC_STATE_DANGER:
            return 3U;
        default:
            return 0U;
    }
}

/* Lay so mau xac nhan tuong ung voi candidate state. */
static uint8_t RC_GetConfirmCount(
    RC_State_t current_state,
    RC_State_t candidate_state)
{
    const uint8_t current_risk = RC_GetRisk(current_state);
    const uint8_t candidate_risk = RC_GetRisk(candidate_state);

    if ((current_state == RC_STATE_WARMUP) &&
        (candidate_state == RC_STATE_SAFE))
    {
        return 1U;
    }

    if ((candidate_state == RC_STATE_SAFE) ||
        (candidate_risk < current_risk))
    {
        return (uint8_t)RC_CLEAR_CONFIRM;
    }

    if (candidate_state == RC_STATE_APPROACHING)
    {
        return (uint8_t)RC_APPROACH_CONFIRM;
    }
    if (candidate_state == RC_STATE_WARNING)
    {
        return (uint8_t)RC_WARNING_CONFIRM;
    }
    if (candidate_state == RC_STATE_DANGER)
    {
        return (uint8_t)RC_DANGER_CONFIRM;
    }

    return 1U;
}

/* Xac nhan state rieng cua mot cam bien sau. */
static void RC_ConfirmState(
    RC_SensorRuntime_t *sensor,
    RC_State_t candidate_state)
{
    uint8_t required_samples;

    if (candidate_state == sensor->result.state)
    {
        sensor->candidate_state = candidate_state;
        sensor->candidate_samples = 0U;
        return;
    }

    if (candidate_state != sensor->candidate_state)
    {
        sensor->candidate_state = candidate_state;
        sensor->candidate_samples = 1U;
    }
    else if (sensor->candidate_samples < 255U)
    {
        sensor->candidate_samples++;
    }
    else
    {
        /* Bộ đếm đã bão hòa. */
    }

    required_samples = RC_GetConfirmCount(
        sensor->result.state,
        candidate_state);

    if (sensor->candidate_samples >= required_samples)
    {
        sensor->result.state = candidate_state;
        sensor->candidate_samples = 0U;
    }
}

/* Them mot khoang cach moi vao lich su bon mau. */
static void RC_PushHistory(
    RC_SensorRuntime_t *sensor,
    float distance_cm)
{
    sensor->history_cm[sensor->write_index] = distance_cm;
    sensor->write_index++;

    if (sensor->write_index >= (uint8_t)RC_HISTORY_COUNT)
    {
        sensor->write_index = 0U;
    }

    if (sensor->history_count < (uint8_t)RC_HISTORY_COUNT)
    {
        sensor->history_count++;
    }

    sensor->result.history_ready =
        (sensor->history_count >= (uint8_t)RC_HISTORY_COUNT);
}

/* Phan loai SAFE, APPROACHING, WARNING hoac DANGER. */
static RC_State_t RC_Classify(
    const RC_SensorRuntime_t *sensor)
{
    const bool approaching =
        sensor->result.distance_valid &&
        sensor->result.history_ready &&
        (sensor->result.distance_cm <= RC_MONITOR_DIST_CM) &&
        (sensor->result.v_close_cm_s >= RC_MIN_VCLOSE_CM_S);

    if (!approaching)
    {
        return RC_STATE_SAFE;
    }

    if ((sensor->result.ttc_valid &&
         (sensor->result.ttc_s <= RC_DANGER_TTC_S)) ||
        (sensor->result.distance_cm <= RC_DANGER_DIST_CM))
    {
        return RC_STATE_DANGER;
    }

    if ((sensor->result.ttc_valid &&
         (sensor->result.ttc_s <= RC_WARNING_TTC_S)) ||
        (sensor->result.distance_cm <= RC_WARNING_DIST_CM))
    {
        return RC_STATE_WARNING;
    }

    return RC_STATE_APPROACHING;
}

/* Tinh v_close, TTC va state cho mot ben. */
static void RC_ProcessSensor(
    RC_SensorRuntime_t *sensor,
    float distance_cm,
    bool distance_valid,
    uint32_t sequence)
{
    const bool value_valid =
        distance_valid && RC_IsFiniteNonNegative(distance_cm);
    RC_State_t candidate_state;

    if (!sensor->sequence_initialized)
    {
        sensor->sequence_initialized = true;
        sensor->last_sequence = sequence;
        if (sequence == 0UL)
        {
            return;
        }
    }
    else if (sequence == sensor->last_sequence)
    {
        return;
    }
    else
    {
        sensor->last_sequence = sequence;
    }

    if (!value_valid)
    {
        RC_ResetSensor(
            sensor,
            RC_STATE_INVALID,
            sequence,
            true);
        sensor->reset_on_recovery = true;
        return;
    }

    if (sensor->reset_on_recovery)
    {
        RC_ResetSensor(
            sensor,
            RC_STATE_WARMUP,
            sequence,
            true);
    }

    sensor->result.distance_valid = true;
    sensor->result.distance_cm = distance_cm;
    RC_PushHistory(sensor, distance_cm);

    if (!sensor->result.history_ready)
    {
        sensor->result.state = RC_STATE_WARMUP;
        sensor->result.v_close_cm_s = 0.0f;
        sensor->result.ttc_s = 0.0f;
        sensor->result.ttc_valid = false;
        sensor->candidate_state = RC_STATE_WARMUP;
        sensor->candidate_samples = 0U;
        return;
    }

    {
        const uint8_t oldest_index = sensor->write_index;
        const uint8_t newest_index =
            (sensor->write_index == 0U) ?
            ((uint8_t)RC_HISTORY_COUNT - 1U) :
            (sensor->write_index - 1U);
        const float delta_time_s =
            ((float)(RC_HISTORY_COUNT - 1U) * RC_SAMPLE_PERIOD_S);

        if (delta_time_s <= 0.0f)
        {
            RC_ResetSensor(
                sensor,
                RC_STATE_INVALID,
                sequence,
                true);
            sensor->reset_on_recovery = true;
            return;
        }

        sensor->result.v_close_cm_s =
            (sensor->history_cm[oldest_index] -
             sensor->history_cm[newest_index]) /
            delta_time_s;
    }

    if (sensor->result.v_close_cm_s >= RC_MIN_VCLOSE_CM_S)
    {
        sensor->result.ttc_s =
            sensor->result.distance_cm / sensor->result.v_close_cm_s;
        sensor->result.ttc_valid = true;
    }
    else
    {
        sensor->result.ttc_s = 0.0f;
        sensor->result.ttc_valid = false;
    }

    candidate_state = RC_Classify(sensor);
    RC_ConfirmState(sensor, candidate_state);
}

/* Tong hop ben trai/phai thanh state va side chung. */
static void RC_Aggregate(void)
{
    const bool left_usable =
        s_rc.left.result.distance_valid &&
        (s_rc.left.result.state != RC_STATE_INVALID);
    const bool right_usable =
        s_rc.right.result.distance_valid &&
        (s_rc.right.result.state != RC_STATE_INVALID);
    const uint8_t left_risk = RC_GetRisk(s_rc.left.result.state);
    const uint8_t right_risk = RC_GetRisk(s_rc.right.result.state);

    s_rc.side = RC_SIDE_NONE;

    if ((!left_usable) && (!right_usable))
    {
        if ((s_rc.left.result.state == RC_STATE_INVALID) &&
            (s_rc.right.result.state == RC_STATE_INVALID))
        {
            s_rc.state = RC_STATE_INVALID;
        }
        else
        {
            s_rc.state = RC_STATE_WARMUP;
        }
        s_rc.data_valid = false;
        return;
    }

    s_rc.data_valid = true;

    if (left_usable && (!right_usable))
    {
        s_rc.state = s_rc.left.result.state;
        s_rc.side = (left_risk > 0U) ? RC_SIDE_LEFT : RC_SIDE_NONE;
        return;
    }

    if ((!left_usable) && right_usable)
    {
        s_rc.state = s_rc.right.result.state;
        s_rc.side = (right_risk > 0U) ? RC_SIDE_RIGHT : RC_SIDE_NONE;
        return;
    }

    if (left_risk > right_risk)
    {
        s_rc.state = s_rc.left.result.state;
        s_rc.side = RC_SIDE_LEFT;
    }
    else if (right_risk > left_risk)
    {
        s_rc.state = s_rc.right.result.state;
        s_rc.side = RC_SIDE_RIGHT;
    }
    else if (left_risk > 0U)
    {
        s_rc.state = s_rc.left.result.state;
        s_rc.side = RC_SIDE_BOTH;
    }
    else if ((s_rc.left.result.state == RC_STATE_SAFE) ||
             (s_rc.right.result.state == RC_STATE_SAFE))
    {
        s_rc.state = RC_STATE_SAFE;
    }
    else
    {
        s_rc.state = RC_STATE_WARMUP;
    }
}

/* Chep ket qua Rear Collision ra output chung. */
static void RC_CopyOutput(RA_Output_t *output)
{
    output->rear_collision_state = s_rc.state;
    output->rear_collision_side = s_rc.side;
    output->rear_collision_valid = s_rc.data_valid;
    output->rear_left_closing_speed_cm_s = s_rc.left.result.v_close_cm_s;
    output->rear_right_closing_speed_cm_s = s_rc.right.result.v_close_cm_s;
    output->rear_left_ttc_s = s_rc.left.result.ttc_s;
    output->rear_right_ttc_s = s_rc.right.result.ttc_s;
    output->rear_left_ttc_valid = s_rc.left.result.ttc_valid;
    output->rear_right_ttc_valid = s_rc.right.result.ttc_valid;
}

/* Cap nhat Rear Collision song song khi tien hoac dung. */
void RC_Process(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    if ((input == NULL) || (output == NULL))
    {
        return;
    }

    if (input->direction == RA_DIR_REVERSE)
    {
        if (s_rc.last_direction != RA_DIR_REVERSE)
        {
            RC_ResetRuntime(
                RA_DIR_REVERSE,
                RC_STATE_INACTIVE,
                input->rear_left_sample_sequence,
                input->rear_right_sample_sequence,
                true);
        }

        RC_CopyOutput(output);
        return;
    }

    if (input->direction != s_rc.last_direction)
    {
        RC_ResetRuntime(
            input->direction,
            RC_STATE_WARMUP,
            input->rear_left_sample_sequence,
            input->rear_right_sample_sequence,
            true);
    }

    RC_ProcessSensor(
        &s_rc.left,
        input->rear_left_distance_cm,
        input->rear_left_distance_valid,
        input->rear_left_sample_sequence);
    RC_ProcessSensor(
        &s_rc.right,
        input->rear_right_distance_cm,
        input->rear_right_distance_valid,
        input->rear_right_sample_sequence);

    RC_Aggregate();
    RC_CopyOutput(output);
}

/* Tang target toi da 15 phan tram khi du moi dieu kien. */
void RC_ApplyBoost(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    float required_front_cm;
    float target_speed_mps;

    if ((input == NULL) || (output == NULL))
    {
        return;
    }

    required_front_cm =
        output->fcw_threshold_cm + RC_BOOST_FRONT_MARGIN_CM;
    if (required_front_cm < RC_BOOST_MIN_FRONT_CM)
    {
        required_front_cm = RC_BOOST_MIN_FRONT_CM;
    }

    output->rear_boost_required_front_cm = required_front_cm;
    output->rear_boost_active = false;

    if ((input->direction != RA_DIR_FORWARD) ||
        (output->vehicle_speed_mps < VEH_STOP_SPEED_MPS) ||
        (output->state != FS_STATE_SAFE) ||
        (output->motor_action != RA_MOTOR_RUN) ||
        (!input->front_distance_valid) ||
        (!RC_IsFiniteNonNegative(input->front_distance_cm)) ||
        (!output->rear_collision_valid) ||
        (output->rear_collision_state != RC_STATE_DANGER) ||
        (input->front_distance_cm < required_front_cm))
    {
        return;
    }

    target_speed_mps = output->commanded_speed_mps * RC_BOOST_RATIO;
    target_speed_mps = RC_Clamp(
        target_speed_mps,
        0.0f,
        VEH_MAX_SPEED_MPS);

    if (target_speed_mps <= output->commanded_speed_mps)
    {
        return;
    }

    output->target_speed_mps = target_speed_mps;
    output->rear_boost_active = true;
    output->motor_action = RA_MOTOR_ACCEL_FF;
    output->request_motor_stop = false;
    output->request_emergency_brake = false;
}
