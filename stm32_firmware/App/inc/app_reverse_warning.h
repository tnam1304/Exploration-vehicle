/**
 * @file app_reverse_warning.h
 * @brief Cảnh báo khoảng cách khi xe lùi.
 */

#ifndef APP_REVERSE_WARNING_H
#define APP_REVERSE_WARNING_H

#include "app_ra.h"

bool RW_IsConfigValid(void);
void RW_Process(
    const RA_Input_t *input,
    RA_Output_t *output);

#endif /* APP_REVERSE_WARNING_H */
