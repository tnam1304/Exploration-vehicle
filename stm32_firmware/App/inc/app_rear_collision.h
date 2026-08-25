#ifndef APP_REAR_COLLISION_H
#define APP_REAR_COLLISION_H

#include "app_safety.h"
#include <stdbool.h>

typedef enum {
    REAR_COLLISION_INACTIVE = 0,
    REAR_COLLISION_WARMUP,
    REAR_COLLISION_SAFE,
    REAR_COLLISION_APPROACHING,
    REAR_COLLISION_WARNING,
    REAR_COLLISION_DANGER,
    REAR_COLLISION_INVALID
} App_RearCollisionState_t;

typedef struct {
    App_RearCollisionState_t state;
    Safety_Side_t side;
    float left_closing_speed_cms;
    float right_closing_speed_cms;
    float left_ttc_s;
    float right_ttc_s;
    uint8_t left_ttc_valid;
    uint8_t right_ttc_valid;
    uint8_t data_valid;
    uint8_t boost_active;
} App_RearCollisionOutput_t;

void App_RearCollision_Init(void);
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
                               App_RearCollisionOutput_t *output);

#endif /* APP_REAR_COLLISION_H */
