#include "app_forward_safety.h"

#include <stddef.h>

#define FS_SYSTEM_DELAY_S          0.120f
#define FS_FCW_TIME_S              0.300f
//#define FS_COMFORT_DECEL_MPS2      2.000f
#define FS_COMFORT_DECEL_MPS2      1.500f
#define FS_EMERGENCY_DECEL_MPS2    6.000f
#define FS_DISTANCE_MARGIN_M       0.050f
#define FS_RISK_CONFIRM_SAMPLES    2U
#define FS_CLEAR_CONFIRM_SAMPLES   3U
#define FS_INVALID_SAMPLES         3U
#define FS_HYSTERESIS_CM           1.0f
//#define FS_PRE_BRAKE_MIN_PERCENT   50U
#define FS_PRE_BRAKE_MIN_PERCENT   40U
#define FS_AEB_RELEASE_MARGIN_CM   5.0f
#define FS_AEB_RELEASE_SAMPLES     3U

typedef struct {
    Safety_State_t state;
    Safety_State_t candidate;
    uint8_t candidate_samples;
    uint8_t invalid_samples;
    uint8_t aeb_hold;
    uint8_t restart_requested;
    uint8_t aeb_release_samples;
    App_ForwardSafetyOutput_t output;
} App_ForwardSafetyRuntime_t;

static App_ForwardSafetyRuntime_t s_forward;

static float FS_Abs(float value) {
    return (value < 0.0f) ? -value : value;
}

static float FS_Max(float first, float second) {
    return (first > second) ? first : second;
}

static uint8_t FS_Risk(Safety_State_t state) {
    if (state == SAFETY_STATE_FCW) return 1U;
    if (state == SAFETY_STATE_PRE_BRAKE) return 2U;
    if (state == SAFETY_STATE_AEB || state == SAFETY_STATE_AEB_HOLD) return 3U;
    return 0U;
}

static void FS_UpdateThresholds(float measured_speed_cms,
                                float commanded_speed_cms) {
    const float speed_mps = FS_Max(FS_Abs(measured_speed_cms),
                                  FS_Abs(commanded_speed_cms)) / 100.0f;
    const float aeb_m = (speed_mps * FS_SYSTEM_DELAY_S) +
                        ((speed_mps * speed_mps) /
                         (2.0f * FS_EMERGENCY_DECEL_MPS2)) +
                        FS_DISTANCE_MARGIN_M;
    const float pre_m = (speed_mps * FS_SYSTEM_DELAY_S) +
                        ((speed_mps * speed_mps) /
                         (2.0f * FS_COMFORT_DECEL_MPS2)) +
                        FS_DISTANCE_MARGIN_M;
    const float fcw_m = pre_m + (speed_mps * FS_FCW_TIME_S);

    s_forward.output.aeb_threshold_cm = aeb_m * 100.0f;
    s_forward.output.pre_brake_threshold_cm = pre_m * 100.0f;
    s_forward.output.fcw_threshold_cm = fcw_m * 100.0f;
}

static Safety_State_t FS_Classify(float distance_cm) {
    const float aeb_cm = s_forward.output.aeb_threshold_cm;
    const float pre_cm = s_forward.output.pre_brake_threshold_cm;
    const float fcw_cm = s_forward.output.fcw_threshold_cm;

    if (distance_cm <= aeb_cm) {
        return SAFETY_STATE_AEB;
    }
    if (s_forward.state == SAFETY_STATE_PRE_BRAKE &&
        distance_cm <= (pre_cm + FS_HYSTERESIS_CM)) {
        return SAFETY_STATE_PRE_BRAKE;
    }
    if (s_forward.state == SAFETY_STATE_FCW &&
        distance_cm > pre_cm &&
        distance_cm <= (fcw_cm + FS_HYSTERESIS_CM)) {
        return SAFETY_STATE_FCW;
    }
    if (distance_cm <= pre_cm) {
        return SAFETY_STATE_PRE_BRAKE;
    }
    if (distance_cm <= fcw_cm) {
        return SAFETY_STATE_FCW;
    }
    return SAFETY_STATE_SAFE;
}

static void FS_Confirm(Safety_State_t desired) {
    const uint8_t required_samples =
        (FS_Risk(desired) > FS_Risk(s_forward.state)) ?
        FS_RISK_CONFIRM_SAMPLES : FS_CLEAR_CONFIRM_SAMPLES;

    if (desired == s_forward.state) {
        s_forward.candidate = desired;
        s_forward.candidate_samples = 0U;
        return;
    }

    if (desired != s_forward.candidate) {
        s_forward.candidate = desired;
        s_forward.candidate_samples = 1U;
    } else if (s_forward.candidate_samples < 255U) {
        s_forward.candidate_samples++;
    }

    if (s_forward.candidate_samples >= required_samples) {
        s_forward.state = desired;
        s_forward.candidate_samples = 0U;
    }
}

static void FS_ApplyOutput(float distance_cm) {
    s_forward.output.state = s_forward.state;
    s_forward.output.speed_percent = 100U;
    s_forward.output.stop_requested = 0U;

    if (s_forward.state == SAFETY_STATE_PRE_BRAKE) {
        const float range = s_forward.output.pre_brake_threshold_cm -
                            s_forward.output.aeb_threshold_cm;
        float ratio = 0.0f;
        if (range > 0.0f) {
            ratio = (distance_cm - s_forward.output.aeb_threshold_cm) / range;
        }
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        s_forward.output.speed_percent =
            (uint8_t)(FS_PRE_BRAKE_MIN_PERCENT +
            ratio * (100U - FS_PRE_BRAKE_MIN_PERCENT));
    } else if (s_forward.state == SAFETY_STATE_AEB) {
        s_forward.output.speed_percent = 0U;
        s_forward.output.stop_requested = 1U;
        s_forward.aeb_hold = 1U;
        s_forward.restart_requested = 0U;
        s_forward.aeb_release_samples = 0U;
    } else if (s_forward.state == SAFETY_STATE_SONAR_ERROR) {
        s_forward.output.speed_percent = 0U;
        s_forward.output.stop_requested = 1U;
    }
}

void App_ForwardSafety_Init(void) {
    s_forward.state = SAFETY_STATE_SAFE;
    s_forward.candidate = SAFETY_STATE_SAFE;
    s_forward.candidate_samples = 0U;
    s_forward.invalid_samples = 0U;
    s_forward.aeb_hold = 0U;
    s_forward.restart_requested = 0U;
    s_forward.aeb_release_samples = 0U;
    s_forward.output.state = SAFETY_STATE_SAFE;
    s_forward.output.speed_percent = 100U;
    s_forward.output.stop_requested = 0U;
    s_forward.output.aeb_threshold_cm = 0.0f;
    s_forward.output.pre_brake_threshold_cm = 0.0f;
    s_forward.output.fcw_threshold_cm = 0.0f;
}

void App_ForwardSafety_RequestRestart(void) {
    if (s_forward.aeb_hold != 0U) {
        s_forward.restart_requested = 1U;
    }
}

void App_ForwardSafety_Process(bool moving_forward,
                               bool new_sample,
                               bool distance_valid,
                               float distance_cm,
                               float measured_speed_cms,
                               float commanded_speed_cms,
                               App_ForwardSafetyOutput_t *output) {
    if (output == NULL) {
        return;
    }

    FS_UpdateThresholds(measured_speed_cms, commanded_speed_cms);

    if (s_forward.aeb_hold != 0U) {
        if (new_sample) {
            if (distance_valid &&
                distance_cm > (s_forward.output.fcw_threshold_cm +
                               FS_AEB_RELEASE_MARGIN_CM)) {
                if (s_forward.aeb_release_samples < 255U)
                    s_forward.aeb_release_samples++;
            } else {
                s_forward.aeb_release_samples = 0U;
            }
        }

        if (s_forward.aeb_release_samples >= FS_AEB_RELEASE_SAMPLES) {
            s_forward.aeb_hold = 0U;
            s_forward.restart_requested = 0U;
            s_forward.aeb_release_samples = 0U;
            s_forward.state = SAFETY_STATE_SAFE;
            s_forward.candidate = SAFETY_STATE_SAFE;
            s_forward.candidate_samples = 0U;
        } else {
            s_forward.output.state = SAFETY_STATE_AEB_HOLD;
            s_forward.output.speed_percent = 0U;
            s_forward.output.stop_requested = 1U;
            *output = s_forward.output;
            return;
        }
    }

    if (!moving_forward) {
        s_forward.state = SAFETY_STATE_SAFE;
        s_forward.candidate = SAFETY_STATE_SAFE;
        s_forward.candidate_samples = 0U;
        s_forward.invalid_samples = 0U;
        s_forward.output.state = SAFETY_STATE_SAFE;
        s_forward.output.speed_percent = 100U;
        s_forward.output.stop_requested = 0U;
        *output = s_forward.output;
        return;
    }

    if (new_sample) {
        if (!distance_valid) {
            if (s_forward.invalid_samples < 255U) {
                s_forward.invalid_samples++;
            }
            if (s_forward.invalid_samples >= FS_INVALID_SAMPLES) {
                s_forward.state = SAFETY_STATE_SONAR_ERROR;
            }
        } else {
            s_forward.invalid_samples = 0U;
            const Safety_State_t desired = FS_Classify(distance_cm);

            /*
             * AEB dung ngay theo mau hop le dau tien. FCW/PRE va qua trinh
             * thoat canh bao van can nhieu mau de loc dao dong sonar.
             */
            if (desired == SAFETY_STATE_AEB) {
                s_forward.state = SAFETY_STATE_AEB;
                s_forward.candidate = SAFETY_STATE_AEB;
                s_forward.candidate_samples = 0U;
            } else {
                FS_Confirm(desired);
            }
        }
    }

    FS_ApplyOutput(distance_cm);
    *output = s_forward.output;
}
