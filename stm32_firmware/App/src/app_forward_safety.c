/* Xu ly FCW, PRE_BRAKE, AEB, AEB_HOLD va tien cham sau AEB. */

#include "app_forward_safety.h"
#include "app_ra_config.h"

#include <stddef.h>

typedef struct
{
    RA_State_t state;
    RA_State_t candidate_state;
    uint8_t candidate_samples;
    uint8_t invalid_sonar_samples;

    bool aeb_hold_active;
    bool restart_requested;
    bool creep_active;
    bool front_has_valid_sample;

    uint32_t last_front_sequence;

    float last_aeb_m;
    float last_pre_m;
    float last_fcw_m;
    float last_target_mps;
} FS_Runtime_t;

static FS_Runtime_t s_fs;

/* Lay gia tri tuyet doi cua mot so thuc. */
static float FS_Abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

/* Gioi han gia tri trong khoang cho phep. */
static float FS_Clamp(float value, float minimum, float maximum)
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

/* Kiem tra gia tri hop le va khong am. */
static bool FS_IsFiniteNonNegative(float value)
{
    return (value == value) && (value >= 0.0f);
}

/* Kiem tra toan bo cau hinh Forward Safety. */
bool FS_IsConfigValid(void)
{
    return
        (FS_SYSTEM_DELAY_S >= 0.0f) &&
        (FS_FCW_TIME_S > 0.0f) &&
        (FS_COMFORT_DECEL_MPS2 > 0.0f) &&
        (FS_EMERGENCY_DECEL_MPS2 > FS_COMFORT_DECEL_MPS2) &&
        (FS_DISTANCE_MARGIN_M >= 0.0f) &&
        (FS_MIN_PREBRAKE_MPS >= 0.0f) &&
        (FS_CREEP_SPEED_MPS > 0.0f) &&
        (FS_CREEP_SPEED_MPS <= VEH_MAX_SPEED_MPS) &&
        (FS_CREEP_STOP_DISTANCE_M > 0.0f) &&
        (FS_HYSTERESIS_M >= 0.0f) &&
        (FS_CONFIRM_SAMPLES > 0U);
}

/* Dong bo hanh dong motor voi cac co dung/phanh. */
static void FS_SetMotorAction(
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

/* Chep cac nguong dong gan nhat ra output. */
void FS_CopyLastThresholds(RA_Output_t *output)
{
    if (output == NULL)
    {
        return;
    }

    output->aeb_threshold_cm = s_fs.last_aeb_m * CM_PER_M;
    output->pre_brake_threshold_cm = s_fs.last_pre_m * CM_PER_M;
    output->fcw_threshold_cm = s_fs.last_fcw_m * CM_PER_M;
}

/* Tinh D_AEB, D_PRE va D_FCW tu toc do dau vao. */
bool FS_CalculateThresholds(
    float vehicle_speed_mps,
    float *aeb_threshold_m,
    float *pre_brake_threshold_m,
    float *fcw_threshold_m)
{
    const float speed = FS_Abs(vehicle_speed_mps);
    float aeb;
    float pre;
    float fcw;

    if ((!FS_IsConfigValid()) ||
        (!FS_IsFiniteNonNegative(speed)) ||
        (aeb_threshold_m == NULL) ||
        (pre_brake_threshold_m == NULL) ||
        (fcw_threshold_m == NULL))
    {
        return false;
    }

    aeb =
        (speed * FS_SYSTEM_DELAY_S) +
        ((speed * speed) / (2.0f * FS_EMERGENCY_DECEL_MPS2)) +
        FS_DISTANCE_MARGIN_M;

    pre =
        (speed * FS_SYSTEM_DELAY_S) +
        ((speed * speed) / (2.0f * FS_COMFORT_DECEL_MPS2)) +
        FS_DISTANCE_MARGIN_M;

    fcw = pre + (speed * FS_FCW_TIME_S);

    if ((!FS_IsFiniteNonNegative(aeb)) ||
        (!FS_IsFiniteNonNegative(pre)) ||
        (!FS_IsFiniteNonNegative(fcw)) ||
        (!(fcw > pre)) ||
        (!(pre > aeb)))
    {
        return false;
    }

    *aeb_threshold_m = aeb;
    *pre_brake_threshold_m = pre;
    *fcw_threshold_m = fcw;
    return true;
}

/* Phan loai trang thai tu khoang cach va ba nguong dong. */
static RA_State_t FS_Classify(
    float distance_m,
    float aeb_m,
    float pre_m,
    float fcw_m)
{
    if (distance_m <= aeb_m)
    {
        return FS_STATE_AEB;
    }

    if ((s_fs.state == FS_STATE_PRE_BRAKE) &&
        (distance_m <= (pre_m + FS_HYSTERESIS_M)))
    {
        return FS_STATE_PRE_BRAKE;
    }

    if ((s_fs.state == FS_STATE_FCW) &&
        (distance_m > pre_m) &&
        (distance_m <= (fcw_m + FS_HYSTERESIS_M)))
    {
        return FS_STATE_FCW;
    }

    if (distance_m <= pre_m)
    {
        return FS_STATE_PRE_BRAKE;
    }
    if (distance_m <= fcw_m)
    {
        return FS_STATE_FCW;
    }
    return FS_STATE_SAFE;
}

/* Xac nhan chuyen trang thai theo so mau va hysteresis. */
static RA_State_t FS_Confirm(
    RA_State_t desired_state,
    bool front_sample_is_new)
{
    if (desired_state == s_fs.state)
    {
        s_fs.candidate_state = desired_state;
        s_fs.candidate_samples = 0U;
        return s_fs.state;
    }

    if (!front_sample_is_new)
    {
        return s_fs.state;
    }

    if (desired_state != s_fs.candidate_state)
    {
        s_fs.candidate_state = desired_state;
        s_fs.candidate_samples = 1U;
    }
    else if (s_fs.candidate_samples < 255U)
    {
        s_fs.candidate_samples++;
    }
    else
    {
        /* Bộ đếm đã bão hòa. */
    }

    if (s_fs.candidate_samples >= FS_CONFIRM_SAMPLES)
    {
        s_fs.state = desired_state;
        s_fs.candidate_samples = 0U;
    }

    return s_fs.state;
}

/* Noi suy toc do muc tieu trong vung PRE_BRAKE. */
static float FS_CalculatePreBrakeTarget(
    float distance_m,
    float commanded_speed_mps)
{
    const float denominator = s_fs.last_pre_m - s_fs.last_aeb_m;
    float minimum_speed = FS_MIN_PREBRAKE_MPS;
    float ratio;

    if (commanded_speed_mps < minimum_speed)
    {
        minimum_speed = commanded_speed_mps;
    }

    if (denominator <= 0.0f)
    {
        return 0.0f;
    }

    ratio = (distance_m - s_fs.last_aeb_m) / denominator;
    ratio = FS_Clamp(ratio, 0.0f, 1.0f);

    return minimum_speed +
           (ratio * (commanded_speed_mps - minimum_speed));
}

/* Chuyen trang thai Forward thanh canh bao va lenh motor. */
static void FS_ApplyState(
    RA_State_t state,
    float distance_m,
    RA_Output_t *output)
{
    output->state = state;

    if (state == FS_STATE_PRE_BRAKE)
    {
        output->warning_level = RA_WARN_ACTIVE;
        output->target_speed_mps = FS_CalculatePreBrakeTarget(
            distance_m,
            output->commanded_speed_mps);
        FS_SetMotorAction(output, RA_MOTOR_DECEL_FF);
    }
    else if (state == FS_STATE_FCW)
    {
        output->warning_level = RA_WARN_ACTIVE;
        output->target_speed_mps = output->commanded_speed_mps;
        FS_SetMotorAction(output, RA_MOTOR_RUN);
    }
    else if (state == FS_STATE_SAFE)
    {
        output->warning_level = RA_WARN_NONE;
        output->target_speed_mps = output->commanded_speed_mps;
        FS_SetMotorAction(output, RA_MOTOR_RUN);
    }
    else if (state == RA_STATE_SENSOR_INVALID)
    {
        output->warning_level = RA_WARN_ACTIVE;
        output->target_speed_mps = 0.0f;
        FS_SetMotorAction(output, RA_MOTOR_STOP);
    }
    else
    {
        /* AEB được xử lý ở nhánh riêng. */
    }

    s_fs.last_target_mps = output->target_speed_mps;
}

/* Giu output cu khi chua co du mau sonar moi. */
static void FS_ApplyHeldState(RA_Output_t *output)
{
    output->state = s_fs.state;
    output->target_speed_mps = s_fs.last_target_mps;

    if (s_fs.state == FS_STATE_PRE_BRAKE)
    {
        output->warning_level = RA_WARN_ACTIVE;
        FS_SetMotorAction(output, RA_MOTOR_DECEL_FF);
    }
    else if (s_fs.state == FS_STATE_FCW)
    {
        output->warning_level = RA_WARN_ACTIVE;
        FS_SetMotorAction(output, RA_MOTOR_RUN);
    }
    else if (s_fs.state == FS_STATE_SAFE)
    {
        output->warning_level = RA_WARN_NONE;
        FS_SetMotorAction(output, RA_MOTOR_RUN);
    }
    else
    {
        output->warning_level = RA_WARN_ACTIVE;
        output->target_speed_mps = 0.0f;
        FS_SetMotorAction(output, RA_MOTOR_STOP);
    }
}

/* Giu xe dung va xu ly yeu cau STOPPED -> FORWARD moi. */
static void FS_ProcessAebHold(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    const float front_distance_m = input->front_distance_cm * M_PER_CM;
    const bool front_sample_is_new =
        (input->front_sample_sequence != s_fs.last_front_sequence);
    float command_aeb_m;
    float command_pre_m;
    float command_fcw_m;

    if (front_sample_is_new)
    {
        s_fs.last_front_sequence = input->front_sample_sequence;
    }

    output->state = FS_STATE_AEB_HOLD;
    output->warning_level = RA_WARN_DANGER;
    output->target_speed_mps = 0.0f;
    output->sonar_valid = input->front_distance_valid;
    FS_SetMotorAction(output, RA_MOTOR_AEB_HOLD);
    FS_CopyLastThresholds(output);

    /* Chi xu ly dung mot mau moi sau lenh STOP -> FORWARD. */
    if ((!front_sample_is_new) || (!s_fs.restart_requested))
    {
        return;
    }

    /* Lenh da duoc xu ly; neu khong du dieu kien phai bam W lai. */
    s_fs.restart_requested = false;

    if ((!output->encoder_valid) ||
        (!input->front_distance_valid) ||
        (!FS_IsFiniteNonNegative(input->front_distance_cm)) ||
        (output->vehicle_speed_mps >= VEH_STOP_SPEED_MPS) ||
        (output->commanded_speed_mps <= 0.0f) ||
        (front_distance_m <= FS_CREEP_STOP_DISTANCE_M))
    {
        return;
    }

    if (!FS_CalculateThresholds(
            output->commanded_speed_mps,
            &command_aeb_m,
            &command_pre_m,
            &command_fcw_m))
    {
        return;
    }

    s_fs.aeb_hold_active = false;
    s_fs.creep_active = (front_distance_m <= command_pre_m);
    s_fs.state = FS_STATE_SAFE;
    s_fs.candidate_state = FS_STATE_SAFE;
    s_fs.candidate_samples = 0U;

    output->state = FS_STATE_SAFE;
    output->warning_level = RA_WARN_NONE;
    output->target_speed_mps = s_fs.creep_active ?
        FS_CREEP_SPEED_MPS : output->commanded_speed_mps;
    FS_SetMotorAction(
        output,
        s_fs.creep_active ? RA_MOTOR_DECEL_FF : RA_MOTOR_RUN);
}

/* Khoi tao bien runtime cua Forward Safety. */
void FS_Init(void)
{
    s_fs.state = FS_STATE_SAFE;
    s_fs.candidate_state = FS_STATE_SAFE;
    s_fs.candidate_samples = 0U;
    s_fs.invalid_sonar_samples = 0U;
    s_fs.aeb_hold_active = false;
    s_fs.restart_requested = false;
    s_fs.creep_active = false;
    s_fs.front_has_valid_sample = false;
    s_fs.last_front_sequence = 0UL;
    s_fs.last_aeb_m = 0.0f;
    s_fs.last_pre_m = 0.0f;
    s_fs.last_fcw_m = 0.0f;
    s_fs.last_target_mps = 0.0f;
}

/* Xoa trang thai tam khi xe doi huong. */
void FS_ResetForNewDirection(void)
{
    if (FS_IsAebActive())
    {
        return;
    }

    s_fs.front_has_valid_sample = false;
    s_fs.invalid_sonar_samples = 0U;
    s_fs.candidate_samples = 0U;
    s_fs.creep_active = false;
    s_fs.restart_requested = false;
    s_fs.state = FS_STATE_SAFE;
    s_fs.candidate_state = FS_STATE_SAFE;
}

/* Ghi nhan yeu cau thu tien lai sau AEB_HOLD. */
void FS_RequestRestart(void)
{
    s_fs.restart_requested = true;
}

/* Kiem tra AEB hoac AEB_HOLD dang duoc latch. */
bool FS_IsAebActive(void)
{
    return s_fs.aeb_hold_active || (s_fs.state == FS_STATE_AEB);
}

/* Chay mot chu ky FCW, PRE_BRAKE, AEB va creep. */
void FS_Process(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    RA_State_t desired_state;
    RA_State_t confirmed_state;
    float front_distance_m;
    float aeb_m;
    float pre_m;
    float fcw_m;
    bool front_sample_is_new;

    if ((input == NULL) || (output == NULL))
    {
        return;
    }

    if (s_fs.state == FS_STATE_AEB)
    {
        s_fs.aeb_hold_active = true;
        s_fs.state = FS_STATE_AEB_HOLD;
    }

    if (s_fs.aeb_hold_active)
    {
        FS_ProcessAebHold(input, output);
        return;
    }

    if (!output->encoder_valid)
    {
        if (s_fs.creep_active)
        {
            s_fs.creep_active = false;
            s_fs.aeb_hold_active = true;
            s_fs.state = FS_STATE_AEB_HOLD;
            output->state = FS_STATE_AEB_HOLD;
            output->warning_level = RA_WARN_DANGER;
            output->target_speed_mps = 0.0f;
            FS_SetMotorAction(output, RA_MOTOR_AEB_HOLD);
            return;
        }

        output->state = RA_STATE_ENCODER_INVALID;
        output->warning_level = RA_WARN_ACTIVE;
        output->target_speed_mps = 0.0f;
        FS_SetMotorAction(output, RA_MOTOR_STOP);
        return;
    }

    front_sample_is_new =
        (input->front_sample_sequence != s_fs.last_front_sequence);

    if ((!input->front_distance_valid) ||
        (!FS_IsFiniteNonNegative(input->front_distance_cm)))
    {
        output->sonar_valid = false;

        if (s_fs.creep_active)
        {
            s_fs.creep_active = false;
            s_fs.aeb_hold_active = true;
            s_fs.state = FS_STATE_AEB_HOLD;
            output->state = FS_STATE_AEB_HOLD;
            output->warning_level = RA_WARN_DANGER;
            output->target_speed_mps = 0.0f;
            FS_SetMotorAction(output, RA_MOTOR_AEB_HOLD);
            return;
        }

        if (front_sample_is_new)
        {
            s_fs.last_front_sequence = input->front_sample_sequence;
            if (s_fs.invalid_sonar_samples < 255U)
            {
                s_fs.invalid_sonar_samples++;
            }
        }

        if (!s_fs.front_has_valid_sample)
        {
            s_fs.state = RA_STATE_SENSOR_INVALID;
            FS_ApplyState(
                RA_STATE_SENSOR_INVALID,
                0.0f,
                output);
            return;
        }

        if (s_fs.invalid_sonar_samples < FS_CONFIRM_SAMPLES)
        {
            FS_ApplyHeldState(output);
            return;
        }

        s_fs.state = RA_STATE_SENSOR_INVALID;
        FS_ApplyState(
            RA_STATE_SENSOR_INVALID,
            0.0f,
            output);
        return;
    }

    output->sonar_valid = true;
    s_fs.front_has_valid_sample = true;
    s_fs.invalid_sonar_samples = 0U;

    if (front_sample_is_new)
    {
        s_fs.last_front_sequence = input->front_sample_sequence;
    }

    front_distance_m = input->front_distance_cm * M_PER_CM;

    /* Creep chi ton tai khi vat can chua du xa de chay toc do dat. */
    if (s_fs.creep_active)
    {
        float command_aeb_m;
        float command_pre_m;
        float command_fcw_m;

        if ((!FS_CalculateThresholds(
                output->commanded_speed_mps,
                &command_aeb_m,
                &command_pre_m,
                &command_fcw_m)) ||
            (front_distance_m <= FS_CREEP_STOP_DISTANCE_M))
        {
            s_fs.creep_active = false;
            s_fs.state = FS_STATE_AEB;
            s_fs.last_target_mps = 0.0f;
            output->state = FS_STATE_AEB;
            output->warning_level = RA_WARN_DANGER;
            output->target_speed_mps = 0.0f;
            FS_SetMotorAction(output, RA_MOTOR_EMERGENCY_BRAKE);
            return;
        }

        if (front_distance_m > command_pre_m)
        {
            s_fs.creep_active = false;
        }
    }

    if (output->vehicle_speed_mps < VEH_STOP_SPEED_MPS)
    {
        s_fs.state = FS_STATE_SAFE;
        s_fs.candidate_state = FS_STATE_SAFE;
        s_fs.candidate_samples = 0U;
        s_fs.last_aeb_m = 0.0f;
        s_fs.last_pre_m = 0.0f;
        s_fs.last_fcw_m = 0.0f;
        FS_CopyLastThresholds(output);

        output->state = FS_STATE_SAFE;
        output->target_speed_mps = s_fs.creep_active ?
            FS_CREEP_SPEED_MPS : output->commanded_speed_mps;
        FS_SetMotorAction(
            output,
            s_fs.creep_active ? RA_MOTOR_DECEL_FF : RA_MOTOR_RUN);
        s_fs.last_target_mps = output->target_speed_mps;
        return;
    }

    if (!FS_CalculateThresholds(
            output->vehicle_speed_mps,
            &aeb_m,
            &pre_m,
            &fcw_m))
    {
        output->config_valid = false;
        output->state = RA_STATE_CONFIG_INVALID;
        output->warning_level = RA_WARN_DANGER;
        output->target_speed_mps = 0.0f;
        FS_SetMotorAction(output, RA_MOTOR_STOP);
        return;
    }

    s_fs.last_aeb_m = aeb_m;
    s_fs.last_pre_m = pre_m;
    s_fs.last_fcw_m = fcw_m;
    FS_CopyLastThresholds(output);

    desired_state = FS_Classify(
        front_distance_m,
        aeb_m,
        pre_m,
        fcw_m);

    if (desired_state == FS_STATE_AEB)
    {
        s_fs.state = FS_STATE_AEB;
        s_fs.candidate_samples = 0U;
        s_fs.restart_requested = false;
        s_fs.creep_active = false;
        s_fs.last_target_mps = 0.0f;

        output->state = FS_STATE_AEB;
        output->warning_level = RA_WARN_DANGER;
        output->target_speed_mps = 0.0f;
        FS_SetMotorAction(
            output,
            RA_MOTOR_EMERGENCY_BRAKE);
        return;
    }

    confirmed_state = FS_Confirm(
        desired_state,
        front_sample_is_new);
    FS_ApplyState(
        confirmed_state,
        front_distance_m,
        output);

    /* Creep la tran toc do, khong duoc ghi de PRE_BRAKE hoac AEB. */
    if (s_fs.creep_active &&
        (output->target_speed_mps > FS_CREEP_SPEED_MPS))
    {
        output->target_speed_mps = FS_CREEP_SPEED_MPS;
        s_fs.last_target_mps = output->target_speed_mps;
        if (output->motor_action == RA_MOTOR_RUN)
        {
            FS_SetMotorAction(output, RA_MOTOR_DECEL_FF);
        }
    }
}
