/* Dieu phoi Forward Safety, Reverse Warning va Rear Collision. */

#include "app_ra.h"
#include "app_forward_safety.h"
#include "app_rear_collision.h"
#include "app_reverse_warning.h"
#include "app_ra_config.h"

#include <stddef.h>

static RA_Direction_t s_last_direction = RA_DIR_STOPPED;

/* Lay gia tri tuyet doi cua mot so thuc. */
static float RA_Abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

/* Kiem tra gia tri hop le va khong am. */
static bool RA_IsFiniteNonNegative(float value)
{
    return (value == value) && (value >= 0.0f);
}

/* Kiem tra cac macro cau hinh chung. */
static bool RA_IsCommonConfigValid(void)
{
    return
        (VEH_WHEEL_DIAMETER_M > 0.0f) &&
        (VEH_ENCODER_PPR > 0.0f) &&
        (VEH_MAX_SPEED_MPS > 0.0f) &&
        (VEH_ENCODER_MAX_SPEED_MPS > VEH_MAX_SPEED_MPS) &&
        (VEH_STOP_SPEED_MPS >= 0.0f) &&
        (RA_MEDIAN_SIZE > 0U) &&
        FS_IsConfigValid() &&
        RW_IsConfigValid() &&
        RC_IsConfigValid();
}

/* Kiem tra co hop le va mien gia tri cua hai encoder. */
static bool RA_IsEncoderValid(const RA_Input_t *input)
{
    float left_speed;
    float right_speed;

    if ((input == NULL) || (!input->encoder_valid))
    {
        return false;
    }

    left_speed = RA_Abs(input->left_speed_mps);
    right_speed = RA_Abs(input->right_speed_mps);

    if ((!RA_IsFiniteNonNegative(left_speed)) ||
        (!RA_IsFiniteNonNegative(right_speed)))
    {
        return false;
    }

    return (left_speed <= VEH_ENCODER_MAX_SPEED_MPS) &&
           (right_speed <= VEH_ENCODER_MAX_SPEED_MPS);
}

/* Dong bo motor_action voi hai co dung va phanh. */
static void RA_SetMotorAction(
    RA_Output_t *output,
    RA_MotorAction_t action)
{
    output->motor_action = action;
    output->request_motor_stop =
        (action == RA_MOTOR_STOP) ||
        (action == RA_MOTOR_EMERGENCY_BRAKE) ||
        (action == RA_MOTOR_AEB_HOLD);
    output->request_emergency_brake =
        (action == RA_MOTOR_EMERGENCY_BRAKE) ||
        (action == RA_MOTOR_AEB_HOLD);
}

/* Dua output ve gia tri mac dinh truoc moi chu ky. */
static void RA_ResetOutput(RA_Output_t *output)
{
    output->state = RA_STATE_STOPPED;
    output->warning_level = RA_WARN_NONE;
    RA_SetMotorAction(output, RA_MOTOR_STOP);

    output->vehicle_speed_mps = 0.0f;
    output->commanded_speed_mps = 0.0f;
    output->target_speed_mps = 0.0f;

    output->aeb_threshold_cm = 0.0f;
    output->pre_brake_threshold_cm = 0.0f;
    output->fcw_threshold_cm = 0.0f;

    output->nearest_rear_distance_cm = 0.0f;
    output->nearest_rear_distance_valid = false;

    output->rear_collision_state = RC_STATE_WARMUP;
    output->rear_collision_side = RC_SIDE_NONE;
    output->rear_left_closing_speed_cm_s = 0.0f;
    output->rear_right_closing_speed_cm_s = 0.0f;
    output->rear_left_ttc_s = 0.0f;
    output->rear_right_ttc_s = 0.0f;
    output->rear_left_ttc_valid = false;
    output->rear_right_ttc_valid = false;
    output->rear_collision_valid = false;
    output->rear_boost_active = false;
    output->rear_boost_required_front_cm = 0.0f;

    output->encoder_valid = false;
    output->sonar_valid = false;
    output->config_valid = false;
}

/* Chon toc do lon hon cua hai banh, khong lay trung binh. */
float RA_GetVehicleSpeedMps(
    float left_speed_mps,
    float right_speed_mps)
{
    const float left = RA_Abs(left_speed_mps);
    const float right = RA_Abs(right_speed_mps);

    /* Không lấy trung bình theo yêu cầu mentor; chọn giá trị bảo thủ hơn. */
    return (left >= right) ? left : right;
}

/* Khoi tao toan bo state machine Reverse Assist. */
void RA_Init(RA_Output_t *output)
{
    s_last_direction = RA_DIR_STOPPED;
    FS_Init();
    RC_Init();

    if (output != NULL)
    {
        RA_ResetOutput(output);
    }
}

/* Ghi nhan canh STOPPED -> FORWARD de xin nha AEB_HOLD. */
void RA_RequestForwardRestart(void)
{
    FS_RequestRestart();
}

/* Chay mot chu ky dieu phoi Forward, Reverse va Rear Collision. */
void RA_Process(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    if (output == NULL)
    {
        return;
    }

    RA_ResetOutput(output);
    FS_CopyLastThresholds(output);

    if (input == NULL)
    {
        output->state = RA_STATE_CONFIG_INVALID;
        output->warning_level = RA_WARN_DANGER;
        return;
    }

    output->config_valid = RA_IsCommonConfigValid();
    output->commanded_speed_mps =
        RA_IsFiniteNonNegative(input->commanded_speed_mps) ?
        input->commanded_speed_mps : 0.0f;
    output->encoder_valid = RA_IsEncoderValid(input);

    if (output->encoder_valid)
    {
        output->vehicle_speed_mps = RA_GetVehicleSpeedMps(
            input->left_speed_mps,
            input->right_speed_mps);
    }

    if (!output->config_valid)
    {
        output->state = RA_STATE_CONFIG_INVALID;
        output->warning_level = RA_WARN_DANGER;
        RA_SetMotorAction(output, RA_MOTOR_STOP);
        return;
    }

    /* Va chạm sau vẫn được cập nhật song song khi tiến hoặc đứng im. */
    RC_Process(input, output);

    if ((!FS_IsAebActive()) &&
        (input->direction != s_last_direction))
    {
        FS_ResetForNewDirection();
        s_last_direction = input->direction;
    }

    /* AEB_HOLD chặn cả lệnh đứng im hoặc lùi cho tới khi nhả đúng điều kiện. */
    if (FS_IsAebActive())
    {
        FS_Process(input, output);
        return;
    }

    if (input->direction == RA_DIR_STOPPED)
    {
        output->state = RA_STATE_STOPPED;
        output->target_speed_mps = 0.0f;
        RA_SetMotorAction(output, RA_MOTOR_STOP);
        return;
    }

    if (input->direction == RA_DIR_FORWARD)
    {
        FS_Process(input, output);
        RC_ApplyBoost(input, output);
        return;
    }

    if (input->direction == RA_DIR_REVERSE)
    {
        RW_Process(input, output);
        return;
    }

    if (input->direction == RA_DIR_OTHER)
    {
        /* Quay trái/phải thuộc điều khiển nhóm, Reverse Assist không ghi đè. */
        output->state = RA_STATE_STOPPED;
        output->warning_level = RA_WARN_NONE;
        output->motor_action = RA_MOTOR_BYPASS;
        output->request_motor_stop = false;
        output->request_emergency_brake = false;
        return;
    }

    output->state = RA_STATE_CONFIG_INVALID;
    output->warning_level = RA_WARN_DANGER;
    RA_SetMotorAction(output, RA_MOTOR_STOP);
}
