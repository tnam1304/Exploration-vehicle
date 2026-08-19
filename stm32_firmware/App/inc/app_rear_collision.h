/**
 * @file app_rear_collision.h
 * @brief Phát hiện vật phía sau tiến gần, tính v_close và TTC.
 */

#ifndef APP_REAR_COLLISION_H
#define APP_REAR_COLLISION_H

#include "app_ra.h"

void RC_Init(void);
bool RC_IsConfigValid(void);

void RC_Process(
    const RA_Input_t *input,
    RA_Output_t *output);

void RC_ApplyBoost(
    const RA_Input_t *input,
    RA_Output_t *output);

#endif /* APP_REAR_COLLISION_H */
