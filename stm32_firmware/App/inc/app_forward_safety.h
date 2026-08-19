/**
 * @file app_forward_safety.h
 * @brief Xử lý FCW, PRE_BRAKE, AEB và AEB_HOLD khi xe tiến.
 */

#ifndef APP_FORWARD_SAFETY_H
#define APP_FORWARD_SAFETY_H

#include "app_ra.h"

void FS_Init(void);
void FS_ResetForNewDirection(void);
void FS_RequestRestart(void);
bool FS_IsAebActive(void);
bool FS_IsConfigValid(void);

bool FS_CalculateThresholds(
    float vehicle_speed_mps,
    float *aeb_threshold_m,
    float *pre_brake_threshold_m,
    float *fcw_threshold_m);

/** @brief Chép lại các ngưỡng front gần nhất cho UART debug. */
void FS_CopyLastThresholds(RA_Output_t *output);

void FS_Process(
    const RA_Input_t *input,
    RA_Output_t *output);

#endif /* APP_FORWARD_SAFETY_H */
