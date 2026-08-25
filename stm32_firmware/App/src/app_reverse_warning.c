#include "app_reverse_warning.h"

#include <stddef.h>

#define RW_DANGER_DISTANCE_CM       5.0f
#define RW_WARNING_DISTANCE_CM      8.0f
#define RW_CONFIRM_SAMPLES          2U
#define RW_SIDE_EQUAL_MARGIN_CM     1.0f

static Safety_State_t s_state;
static Safety_State_t s_candidate;
static uint8_t s_candidate_samples;

static float RW_Abs(float value) {
    return (value < 0.0f) ? -value : value;
}

static void RW_Confirm(Safety_State_t desired) {
    if (desired == s_state) {
        s_candidate = desired;
        s_candidate_samples = 0U;
        return;
    }

    if (desired != s_candidate) {
        s_candidate = desired;
        s_candidate_samples = 1U;
    } else if (s_candidate_samples < 255U) {
        s_candidate_samples++;
    }

    if (s_candidate_samples >= RW_CONFIRM_SAMPLES) {
        s_state = desired;
        s_candidate_samples = 0U;
    }
}

void App_ReverseWarning_Init(void) {
    s_state = SAFETY_STATE_SAFE;
    s_candidate = SAFETY_STATE_SAFE;
    s_candidate_samples = 0U;
}

void App_ReverseWarning_Process(bool moving_reverse,
                                bool new_sample,
                                bool left_valid,
                                float left_distance_cm,
                                bool right_valid,
                                float right_distance_cm,
                                App_ReverseWarningOutput_t *output) {
    Safety_State_t desired = SAFETY_STATE_SAFE;

    if (output == NULL) {
        return;
    }

    output->side = SAFETY_SIDE_NONE;
    output->nearest_distance_cm = 0.0f;
    output->distance_valid = 0U;

    if (!moving_reverse) {
        App_ReverseWarning_Init();
        output->state = SAFETY_STATE_SAFE;
        return;
    }

    if (left_valid && right_valid) {
        output->distance_valid = 1U;
        output->nearest_distance_cm =
            (left_distance_cm <= right_distance_cm) ?
            left_distance_cm : right_distance_cm;
        if (RW_Abs(left_distance_cm - right_distance_cm) <=
            RW_SIDE_EQUAL_MARGIN_CM) {
            output->side = SAFETY_SIDE_BOTH;
        } else {
            output->side = (left_distance_cm < right_distance_cm) ?
                           SAFETY_SIDE_LEFT : SAFETY_SIDE_RIGHT;
        }
    } else if (left_valid) {
        output->distance_valid = 1U;
        output->nearest_distance_cm = left_distance_cm;
        output->side = SAFETY_SIDE_LEFT;
    } else if (right_valid) {
        output->distance_valid = 1U;
        output->nearest_distance_cm = right_distance_cm;
        output->side = SAFETY_SIDE_RIGHT;
    }

    if (new_sample) {
        if (output->distance_valid == 0U) {
            desired = SAFETY_STATE_SONAR_ERROR;
        } else if (output->nearest_distance_cm <= RW_DANGER_DISTANCE_CM) {
            desired = SAFETY_STATE_REVERSE_DANGER;
        } else if (output->nearest_distance_cm <= RW_WARNING_DISTANCE_CM) {
            desired = SAFETY_STATE_REVERSE_WARNING;
        }
        RW_Confirm(desired);
    }

    output->state = s_state;
}
