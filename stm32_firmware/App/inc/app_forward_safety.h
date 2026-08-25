#ifndef APP_FORWARD_SAFETY_H
#define APP_FORWARD_SAFETY_H

#include "app_safety.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    Safety_State_t state;
    uint8_t speed_percent;
    uint8_t stop_requested;
    float aeb_threshold_cm;
    float pre_brake_threshold_cm;
    float fcw_threshold_cm;
} App_ForwardSafetyOutput_t;

void App_ForwardSafety_Init(void);
void App_ForwardSafety_RequestRestart(void);
void App_ForwardSafety_Process(bool moving_forward,
                               bool new_sample,
                               bool distance_valid,
                               float distance_cm,
                               float measured_speed_cms,
                               float commanded_speed_cms,
                               App_ForwardSafetyOutput_t *output);

#endif /* APP_FORWARD_SAFETY_H */
