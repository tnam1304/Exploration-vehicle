#ifndef APP_REVERSE_WARNING_H
#define APP_REVERSE_WARNING_H

#include "app_safety.h"
#include <stdbool.h>

typedef struct {
    Safety_State_t state;
    Safety_Side_t side;
    float nearest_distance_cm;
    uint8_t distance_valid;
} App_ReverseWarningOutput_t;

void App_ReverseWarning_Init(void);
void App_ReverseWarning_Process(bool moving_reverse,
                                bool new_sample,
                                bool left_valid,
                                float left_distance_cm,
                                bool right_valid,
                                float right_distance_cm,
                                App_ReverseWarningOutput_t *output);

#endif /* APP_REVERSE_WARNING_H */
