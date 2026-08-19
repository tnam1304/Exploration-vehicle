/* Canh bao vat can khi xe lui, khong can thiep motor. */

#include "app_reverse_warning.h"
#include "app_ra_config.h"

#include <stddef.h>

/* Kiem tra mot khoang cach sonar sau. */
static bool RW_IsDistanceValid(float distance_cm, bool valid)
{
    return valid && (distance_cm == distance_cm) && (distance_cm >= 0.0f);
}

/* Kiem tra thu tu nguong WARNING va DANGER. */
bool RW_IsConfigValid(void)
{
    return (RW_WARNING_DIST_M > RW_DANGER_DIST_M) &&
           (RW_DANGER_DIST_M > 0.0f);
}

/* Canh bao lui bang khoang cach hop le gan nhat. */
void RW_Process(
    const RA_Input_t *input,
    RA_Output_t *output)
{
    bool left_valid;
    bool right_valid;

    if ((input == NULL) || (output == NULL))
    {
        return;
    }

    left_valid = RW_IsDistanceValid(
        input->rear_left_distance_cm,
        input->rear_left_distance_valid);
    right_valid = RW_IsDistanceValid(
        input->rear_right_distance_cm,
        input->rear_right_distance_valid);

    output->target_speed_mps = output->commanded_speed_mps;
    output->motor_action = RA_MOTOR_RUN;
    output->request_motor_stop = false;
    output->request_emergency_brake = false;

    if (left_valid && right_valid)
    {
        output->nearest_rear_distance_cm =
            (input->rear_left_distance_cm <= input->rear_right_distance_cm) ?
            input->rear_left_distance_cm :
            input->rear_right_distance_cm;
        output->nearest_rear_distance_valid = true;
    }
    else if (left_valid)
    {
        output->nearest_rear_distance_cm = input->rear_left_distance_cm;
        output->nearest_rear_distance_valid = true;
    }
    else if (right_valid)
    {
        output->nearest_rear_distance_cm = input->rear_right_distance_cm;
        output->nearest_rear_distance_valid = true;
    }
    else
    {
        output->state = RA_STATE_SENSOR_INVALID;
        output->warning_level = RA_WARN_ACTIVE;
        output->sonar_valid = false;
        return;
    }

    output->sonar_valid = true;

    if (output->nearest_rear_distance_cm <=
        (RW_DANGER_DIST_M * CM_PER_M))
    {
        output->state = RW_STATE_DANGER;
        output->warning_level = RA_WARN_DANGER;
    }
    else if (output->nearest_rear_distance_cm <=
             (RW_WARNING_DIST_M * CM_PER_M))
    {
        output->state = RW_STATE_WARNING;
        output->warning_level = RA_WARN_ACTIVE;
    }
    else
    {
        output->state = RW_STATE_SAFE;
        output->warning_level = RA_WARN_NONE;
    }
}
