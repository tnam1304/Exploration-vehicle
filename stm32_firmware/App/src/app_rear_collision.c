#include "app_rear_collision.h"

#include <stddef.h>

#define RC_HISTORY_COUNT             4U
#define RC_SAMPLE_PERIOD_S           0.120f
#define RC_MONITOR_DISTANCE_CM       150.0f
#define RC_MIN_CLOSING_SPEED_CMS     15.0f
#define RC_WARNING_DISTANCE_CM       80.0f
#define RC_DANGER_DISTANCE_CM        40.0f
#define RC_WARNING_TTC_S             2.0f
#define RC_DANGER_TTC_S              1.0f
#define RC_APPROACH_CONFIRM          2U
#define RC_WARNING_CONFIRM           2U
#define RC_DANGER_CONFIRM            2U
#define RC_CLEAR_CONFIRM             3U
#define RC_BOOST_MIN_FRONT_CM        120.0f
#define RC_BOOST_FRONT_MARGIN_CM     20.0f
#define RC_BOOST_MIN_SPEED_CMS       5.0f

/* Xe đứng yên: chỉ cảnh báo vật đã được theo dõi từ xa tiến dần vào vùng gần. */
#define RC_STOP_ARM_MIN_CM            50.0f
#define RC_STOP_ARM_MAX_CM            80.0f
#define RC_STOP_WARNING_CM            20.0f
#define RC_STOP_DANGER_CM             15.0f
#define RC_STOP_DANGER_RELEASE_CM     18.0f
#define RC_STOP_RELEASE_CM            25.0f
#define RC_STOP_TRACK_SAMPLES         6U
#define RC_STOP_DECREASE_SAMPLES      4U
#define RC_STOP_MIN_TOTAL_DROP_CM     15.0f
#define RC_STOP_RISE_TOLERANCE_CM      4.0f
#define RC_STOP_DECREASE_STEP_CM       1.0f
#define RC_STOP_CLEAR_SAMPLES          3U
#define RC_STOP_INVALID_TOLERANCE      2U
#define RC_STOP_RISE_TOLERANCE_SAMPLES 2U
#define RC_STOP_IDLE_CLEAR_SAMPLES     84U

typedef struct {
    float history[RC_HISTORY_COUNT];
    uint8_t history_count;
    uint8_t write_index;
    App_RearCollisionState_t state;
    App_RearCollisionState_t candidate;
    uint8_t candidate_samples;
    uint8_t distance_valid;
    float distance_cm;
    float closing_speed_cms;
    float ttc_s;
    uint8_t ttc_valid;
} App_RearSensorRuntime_t;

typedef struct {
    uint8_t sample_seen;
    uint8_t valid;
    uint8_t armed;
    uint8_t alert_active;
    uint8_t sample_count;
    uint8_t decrease_samples;
    uint8_t clear_samples;
    uint8_t invalid_samples;
    uint8_t rise_samples;
    uint8_t idle_samples;
    float start_distance_cm;
    float last_distance_cm;
    float idle_reference_distance_cm;
    App_RearCollisionState_t state;
} App_StoppedApproachRuntime_t;

static App_RearSensorRuntime_t s_left;
static App_RearSensorRuntime_t s_right;
static App_StoppedApproachRuntime_t s_stopped_left;
static App_StoppedApproachRuntime_t s_stopped_right;
static uint8_t s_was_reversing;
static uint8_t s_was_stopped;

static float RC_Abs(float value) {
    return (value < 0.0f) ? -value : value;
}

static uint8_t RC_Risk(App_RearCollisionState_t state) {
    if (state == REAR_COLLISION_APPROACHING) return 1U;
    if (state == REAR_COLLISION_WARNING) return 2U;
    if (state == REAR_COLLISION_DANGER) return 3U;
    return 0U;
}

static void RC_ResetSensor(App_RearSensorRuntime_t *sensor,
                           App_RearCollisionState_t state) {
    uint8_t index;
    for (index = 0U; index < RC_HISTORY_COUNT; index++) {
        sensor->history[index] = 0.0f;
    }
    sensor->history_count = 0U;
    sensor->write_index = 0U;
    sensor->state = state;
    sensor->candidate = state;
    sensor->candidate_samples = 0U;
    sensor->distance_valid = 0U;
    sensor->distance_cm = 0.0f;
    sensor->closing_speed_cms = 0.0f;
    sensor->ttc_s = 0.0f;
    sensor->ttc_valid = 0U;
}

static void RC_ResetStoppedTracker(App_StoppedApproachRuntime_t *tracker) {
    tracker->sample_seen = 0U;
    tracker->valid = 0U;
    tracker->armed = 0U;
    tracker->alert_active = 0U;
    tracker->sample_count = 0U;
    tracker->decrease_samples = 0U;
    tracker->clear_samples = 0U;
    tracker->invalid_samples = 0U;
    tracker->rise_samples = 0U;
    tracker->idle_samples = 0U;
    tracker->start_distance_cm = 0.0f;
    tracker->last_distance_cm = 0.0f;
    tracker->idle_reference_distance_cm = 0.0f;
    tracker->state = REAR_COLLISION_SAFE;
}

static void RC_StartStoppedTracking(App_StoppedApproachRuntime_t *tracker,
                                    float distance_cm) {
    tracker->armed = 1U;
    tracker->alert_active = 0U;
    tracker->sample_count = 1U;
    tracker->decrease_samples = 0U;
    tracker->clear_samples = 0U;
    tracker->invalid_samples = 0U;
    tracker->rise_samples = 0U;
    tracker->idle_samples = 0U;
    tracker->start_distance_cm = distance_cm;
    tracker->last_distance_cm = distance_cm;
    tracker->idle_reference_distance_cm = distance_cm;
    tracker->state = REAR_COLLISION_SAFE;
}

static void RC_ProcessStoppedTracker(App_StoppedApproachRuntime_t *tracker,
                                     bool new_sample,
                                     bool valid,
                                     float distance_cm) {
    float previous_distance;
    float total_drop;

    if (!new_sample) return;

    if (!valid || distance_cm < 0.0f || distance_cm != distance_cm) {
        if (tracker->invalid_samples < 255U) tracker->invalid_samples++;
        if (tracker->invalid_samples > RC_STOP_INVALID_TOLERANCE) {
            RC_ResetStoppedTracker(tracker);
            tracker->sample_seen = 1U;
        }
        return;
    }

    tracker->sample_seen = 1U;
    tracker->valid = 1U;
    tracker->invalid_samples = 0U;

    if (tracker->armed == 0U) {
        if (distance_cm >= RC_STOP_ARM_MIN_CM &&
            distance_cm <= RC_STOP_ARM_MAX_CM) {
            RC_StartStoppedTracking(tracker, distance_cm);
        }
        return;
    }

    previous_distance = tracker->last_distance_cm;

    if (tracker->alert_active != 0U) {
        if (distance_cm > RC_STOP_RELEASE_CM) {
            if (tracker->clear_samples < 255U)
                tracker->clear_samples++;
        } else {
            tracker->clear_samples = 0U;
        }

        if (distance_cm <= RC_STOP_DANGER_CM ||
            (tracker->state == REAR_COLLISION_DANGER &&
             distance_cm <= RC_STOP_DANGER_RELEASE_CM)) {
            tracker->state = REAR_COLLISION_DANGER;
        } else {
            tracker->state = REAR_COLLISION_WARNING;
        }

        tracker->last_distance_cm = distance_cm;
        if (tracker->clear_samples >= RC_STOP_CLEAR_SAMPLES) {
            RC_ResetStoppedTracker(tracker);
            tracker->sample_seen = 1U;
            tracker->valid = 1U;
            return;
        }

        if (distance_cm < tracker->idle_reference_distance_cm -
                          RC_STOP_DECREASE_STEP_CM) {
            tracker->idle_samples = 0U;
            tracker->idle_reference_distance_cm = distance_cm;
        } else if (tracker->idle_samples < 255U) {
            tracker->idle_samples++;
        }

        if (tracker->idle_samples >= RC_STOP_IDLE_CLEAR_SAMPLES) {
            RC_ResetStoppedTracker(tracker);
            tracker->sample_seen = 1U;
            tracker->valid = 1U;
        }
        return;
    }

    if (distance_cm > previous_distance + RC_STOP_RISE_TOLERANCE_CM) {
        if (tracker->rise_samples < 255U) tracker->rise_samples++;
        if (tracker->rise_samples >= RC_STOP_RISE_TOLERANCE_SAMPLES) {
            RC_ResetStoppedTracker(tracker);
            tracker->sample_seen = 1U;
            tracker->valid = 1U;
        }
        return;
    }
    tracker->rise_samples = 0U;

    if (distance_cm < previous_distance - RC_STOP_DECREASE_STEP_CM &&
        tracker->decrease_samples < 255U) {
        tracker->decrease_samples++;
    }
    if (tracker->sample_count < 255U) tracker->sample_count++;
    tracker->last_distance_cm = distance_cm;
    total_drop = tracker->start_distance_cm - distance_cm;

    if (distance_cm <= RC_STOP_WARNING_CM &&
        tracker->sample_count >= RC_STOP_TRACK_SAMPLES &&
        tracker->decrease_samples >= RC_STOP_DECREASE_SAMPLES &&
        total_drop >= RC_STOP_MIN_TOTAL_DROP_CM) {
        tracker->alert_active = 1U;
        tracker->clear_samples = 0U;
        tracker->idle_samples = 0U;
        tracker->idle_reference_distance_cm = distance_cm;
        tracker->state = (distance_cm <= RC_STOP_DANGER_CM) ?
                         REAR_COLLISION_DANGER :
                         REAR_COLLISION_WARNING;
    }
}

static void RC_ProcessStopped(bool left_new_sample,
                              bool left_valid,
                              float left_distance_cm,
                              bool right_new_sample,
                              bool right_valid,
                              float right_distance_cm,
                              App_RearCollisionOutput_t *output) {
    uint8_t left_risk;
    uint8_t right_risk;

    RC_ProcessStoppedTracker(&s_stopped_left, left_new_sample,
                             left_valid, left_distance_cm);
    RC_ProcessStoppedTracker(&s_stopped_right, right_new_sample,
                             right_valid, right_distance_cm);

    left_risk = RC_Risk(s_stopped_left.state);
    right_risk = RC_Risk(s_stopped_right.state);
    output->state = REAR_COLLISION_SAFE;
    output->side = SAFETY_SIDE_NONE;
    output->data_valid =
        (s_stopped_left.valid != 0U || s_stopped_right.valid != 0U) ? 1U : 0U;
    output->boost_active = 0U;

    if (output->data_valid == 0U) {
        output->state =
            (s_stopped_left.sample_seen != 0U &&
             s_stopped_right.sample_seen != 0U) ?
            REAR_COLLISION_INVALID : REAR_COLLISION_WARMUP;
    }

    if (left_risk > right_risk) {
        output->state = s_stopped_left.state;
        output->side = SAFETY_SIDE_LEFT;
    } else if (right_risk > left_risk) {
        output->state = s_stopped_right.state;
        output->side = SAFETY_SIDE_RIGHT;
    } else if (left_risk > 0U) {
        output->state = s_stopped_left.state;
        output->side = SAFETY_SIDE_BOTH;
    }

    output->left_closing_speed_cms = 0.0f;
    output->right_closing_speed_cms = 0.0f;
    output->left_ttc_s = 0.0f;
    output->right_ttc_s = 0.0f;
    output->left_ttc_valid = 0U;
    output->right_ttc_valid = 0U;
}

static uint8_t RC_RequiredSamples(App_RearCollisionState_t current,
                                  App_RearCollisionState_t candidate) {
    const uint8_t current_risk = RC_Risk(current);
    const uint8_t candidate_risk = RC_Risk(candidate);
    if (current == REAR_COLLISION_WARMUP &&
        candidate == REAR_COLLISION_SAFE) return 1U;
    if (candidate == REAR_COLLISION_SAFE || candidate_risk < current_risk)
        return RC_CLEAR_CONFIRM;
    if (candidate == REAR_COLLISION_APPROACHING) return RC_APPROACH_CONFIRM;
    if (candidate == REAR_COLLISION_WARNING) return RC_WARNING_CONFIRM;
    if (candidate == REAR_COLLISION_DANGER) return RC_DANGER_CONFIRM;
    return 1U;
}

static void RC_Confirm(App_RearSensorRuntime_t *sensor,
                       App_RearCollisionState_t desired) {
    if (desired == sensor->state) {
        sensor->candidate = desired;
        sensor->candidate_samples = 0U;
        return;
    }
    if (desired != sensor->candidate) {
        sensor->candidate = desired;
        sensor->candidate_samples = 1U;
    } else if (sensor->candidate_samples < 255U) {
        sensor->candidate_samples++;
    }
    if (sensor->candidate_samples >=
        RC_RequiredSamples(sensor->state, desired)) {
        sensor->state = desired;
        sensor->candidate_samples = 0U;
    }
}

static App_RearCollisionState_t RC_Classify(
    const App_RearSensorRuntime_t *sensor) {
    const uint8_t approaching =
        sensor->distance_valid != 0U &&
        sensor->history_count >= RC_HISTORY_COUNT &&
        sensor->distance_cm <= RC_MONITOR_DISTANCE_CM &&
        sensor->closing_speed_cms >= RC_MIN_CLOSING_SPEED_CMS;

    if (approaching == 0U) return REAR_COLLISION_SAFE;
    if ((sensor->ttc_valid != 0U && sensor->ttc_s <= RC_DANGER_TTC_S) ||
        sensor->distance_cm <= RC_DANGER_DISTANCE_CM)
        return REAR_COLLISION_DANGER;
    if ((sensor->ttc_valid != 0U && sensor->ttc_s <= RC_WARNING_TTC_S) ||
        sensor->distance_cm <= RC_WARNING_DISTANCE_CM)
        return REAR_COLLISION_WARNING;
    return REAR_COLLISION_APPROACHING;
}

static void RC_ProcessSensor(App_RearSensorRuntime_t *sensor,
                             bool new_sample,
                             bool valid,
                             float distance_cm) {
    uint8_t oldest_index;
    uint8_t newest_index;

    if (!new_sample) return;

    if (!valid || distance_cm < 0.0f || distance_cm != distance_cm) {
        RC_ResetSensor(sensor, REAR_COLLISION_INVALID);
        return;
    }

    if (sensor->state == REAR_COLLISION_INVALID) {
        RC_ResetSensor(sensor, REAR_COLLISION_WARMUP);
    }

    sensor->distance_valid = 1U;
    sensor->distance_cm = distance_cm;
    sensor->history[sensor->write_index] = distance_cm;
    sensor->write_index++;
    if (sensor->write_index >= RC_HISTORY_COUNT) sensor->write_index = 0U;
    if (sensor->history_count < RC_HISTORY_COUNT) sensor->history_count++;

    if (sensor->history_count < RC_HISTORY_COUNT) {
        sensor->state = REAR_COLLISION_WARMUP;
        sensor->closing_speed_cms = 0.0f;
        sensor->ttc_s = 0.0f;
        sensor->ttc_valid = 0U;
        return;
    }

    oldest_index = sensor->write_index;
    newest_index = (sensor->write_index == 0U) ?
                   (RC_HISTORY_COUNT - 1U) : (sensor->write_index - 1U);
    sensor->closing_speed_cms =
        (sensor->history[oldest_index] - sensor->history[newest_index]) /
        ((RC_HISTORY_COUNT - 1U) * RC_SAMPLE_PERIOD_S);

    if (sensor->closing_speed_cms >= RC_MIN_CLOSING_SPEED_CMS) {
        sensor->ttc_s = sensor->distance_cm / sensor->closing_speed_cms;
        sensor->ttc_valid = 1U;
    } else {
        sensor->ttc_s = 0.0f;
        sensor->ttc_valid = 0U;
    }
    RC_Confirm(sensor, RC_Classify(sensor));
}

void App_RearCollision_Init(void) {
    RC_ResetSensor(&s_left, REAR_COLLISION_WARMUP);
    RC_ResetSensor(&s_right, REAR_COLLISION_WARMUP);
    RC_ResetStoppedTracker(&s_stopped_left);
    RC_ResetStoppedTracker(&s_stopped_right);
    s_was_reversing = 0U;
    s_was_stopped = 1U;
}

void App_RearCollision_Process(bool moving_reverse,
                               bool moving_forward,
                               bool left_new_sample,
                               bool left_valid,
                               float left_distance_cm,
                               bool right_new_sample,
                               bool right_valid,
                               float right_distance_cm,
                               bool front_valid,
                               float front_distance_cm,
                               float front_fcw_threshold_cm,
                               float vehicle_speed_cms,
                               bool encoder_feedback_ok,
                               App_RearCollisionOutput_t *output) {
    uint8_t left_risk;
    uint8_t right_risk;
    float required_front_cm;

    if (output == NULL) return;

    if (!moving_reverse && !moving_forward) {
        if (s_was_stopped == 0U) {
            RC_ResetSensor(&s_left, REAR_COLLISION_WARMUP);
            RC_ResetSensor(&s_right, REAR_COLLISION_WARMUP);
            RC_ResetStoppedTracker(&s_stopped_left);
            RC_ResetStoppedTracker(&s_stopped_right);
        }
        s_was_stopped = 1U;
        s_was_reversing = 0U;
        RC_ProcessStopped(left_new_sample, left_valid, left_distance_cm,
                          right_new_sample, right_valid, right_distance_cm,
                          output);
        return;
    }

    if (s_was_stopped != 0U) {
        RC_ResetSensor(&s_left, REAR_COLLISION_WARMUP);
        RC_ResetSensor(&s_right, REAR_COLLISION_WARMUP);
        s_was_stopped = 0U;
    }

    if (moving_reverse) {
        if (s_was_reversing == 0U) {
            RC_ResetSensor(&s_left, REAR_COLLISION_INACTIVE);
            RC_ResetSensor(&s_right, REAR_COLLISION_INACTIVE);
        }
        s_was_reversing = 1U;
    } else {
        if (s_was_reversing != 0U) {
            RC_ResetSensor(&s_left, REAR_COLLISION_WARMUP);
            RC_ResetSensor(&s_right, REAR_COLLISION_WARMUP);
        }
        s_was_reversing = 0U;
        RC_ProcessSensor(&s_left, left_new_sample, left_valid,
                         left_distance_cm);
        RC_ProcessSensor(&s_right, right_new_sample, right_valid,
                         right_distance_cm);
    }

    output->state = REAR_COLLISION_WARMUP;
    output->side = SAFETY_SIDE_NONE;
    output->data_valid = 0U;
    output->boost_active = 0U;

    if (moving_reverse) {
        output->state = REAR_COLLISION_INACTIVE;
    } else {
        left_risk = RC_Risk(s_left.state);
        right_risk = RC_Risk(s_right.state);
        if (s_left.distance_valid == 0U && s_right.distance_valid == 0U) {
            output->state =
                (s_left.state == REAR_COLLISION_INVALID &&
                 s_right.state == REAR_COLLISION_INVALID) ?
                REAR_COLLISION_INVALID : REAR_COLLISION_WARMUP;
        } else if (s_left.distance_valid != 0U &&
                   s_right.distance_valid == 0U) {
            output->state = s_left.state;
            output->side = (left_risk > 0U) ? SAFETY_SIDE_LEFT : SAFETY_SIDE_NONE;
            output->data_valid = 1U;
        } else if (s_left.distance_valid == 0U &&
                   s_right.distance_valid != 0U) {
            output->state = s_right.state;
            output->side = (right_risk > 0U) ? SAFETY_SIDE_RIGHT : SAFETY_SIDE_NONE;
            output->data_valid = 1U;
        } else {
            output->data_valid = 1U;
            if (left_risk > right_risk) {
                output->state = s_left.state;
                output->side = SAFETY_SIDE_LEFT;
            } else if (right_risk > left_risk) {
                output->state = s_right.state;
                output->side = SAFETY_SIDE_RIGHT;
            } else if (left_risk > 0U) {
                output->state = s_left.state;
                output->side = SAFETY_SIDE_BOTH;
            } else if (s_left.state == REAR_COLLISION_SAFE ||
                       s_right.state == REAR_COLLISION_SAFE) {
                output->state = REAR_COLLISION_SAFE;
            }
        }
    }

    required_front_cm = front_fcw_threshold_cm + RC_BOOST_FRONT_MARGIN_CM;
    if (required_front_cm < RC_BOOST_MIN_FRONT_CM)
        required_front_cm = RC_BOOST_MIN_FRONT_CM;

    if (moving_forward && encoder_feedback_ok &&
        RC_Abs(vehicle_speed_cms) >= RC_BOOST_MIN_SPEED_CMS &&
        output->state == REAR_COLLISION_DANGER &&
        front_valid && front_distance_cm >= required_front_cm) {
        output->boost_active = 1U;
    }

    output->left_closing_speed_cms = s_left.closing_speed_cms;
    output->right_closing_speed_cms = s_right.closing_speed_cms;
    output->left_ttc_s = s_left.ttc_s;
    output->right_ttc_s = s_right.ttc_s;
    output->left_ttc_valid = s_left.ttc_valid;
    output->right_ttc_valid = s_right.ttc_valid;
}
