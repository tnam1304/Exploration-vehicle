/*
 * Tao nhip coi cho Forward Safety, canh bao lui va Rear Collision.
 * Chan PB1 van do Update_Buzzer_State() cua nhom dieu khien.
 */

#include "app_buzzer.h"

#include "app_ra_config.h"
#include "app_safety.h"

#include <stddef.h>

typedef enum
{
    RA_BEEP_OFF = 0,
    RA_BEEP_SLOW,
    RA_BEEP_FAST,
    RA_BEEP_CONTINUOUS
} RA_BeepMode_t;

static uint32_t s_beep_start_us;

/*
 * Chuyen yeu cau coi Reverse Assist sang API co san cua nhom.
 * Bao chay va coi thu cong luon co uu tien cao hon.
 */
static void RA_Buzzer_Apply(bool enabled)
{
    if ((warn_code == SAFETY_WARN_FIRE) || (horn_active != 0U))
    {
        Update_Buzzer_State();
        return;
    }

    if (enabled)
    {
        warn_code = SAFETY_WARN_OBSTACLE;
    }
    else if (warn_code == SAFETY_WARN_OBSTACLE)
    {
        warn_code = SAFETY_WARN_NONE;
    }
    else
    {
        /* Giu nguyen ma canh bao khac cua nhom. */
    }

    Update_Buzzer_State();
}

/* Chon kieu keu theo muc nguy hiem cao nhat. */
static RA_BeepMode_t RA_Buzzer_SelectMode(const RA_Output_t *output)
{
    if ((warn_code == SAFETY_WARN_FIRE) || (horn_active != 0U))
    {
        return RA_BEEP_CONTINUOUS;
    }

    if (output == NULL)
    {
        return RA_BEEP_OFF;
    }

    if ((output->state == FS_STATE_AEB) ||
        (output->state == FS_STATE_AEB_HOLD))
    {
        return RA_BEEP_CONTINUOUS;
    }

    if ((output->state == FS_STATE_PRE_BRAKE) ||
        (output->state == RW_STATE_DANGER) ||
        (output->rear_collision_state == RC_STATE_DANGER))
    {
        return RA_BEEP_FAST;
    }

    if ((output->state == FS_STATE_FCW) ||
        (output->state == RW_STATE_WARNING) ||
        (output->state == RA_STATE_SENSOR_INVALID) ||
        (output->state == RA_STATE_ENCODER_INVALID) ||
        (output->rear_collision_state == RC_STATE_WARNING))
    {
        return RA_BEEP_SLOW;
    }

    return RA_BEEP_OFF;
}

/* Khoi tao moc tao nhip va dong bo trang thai coi nhom. */
void RA_Buzzer_Init(void)
{
    s_beep_start_us = TIM2->CNT;
    RA_Buzzer_Apply(false);
}

/* Cap nhat coi khong blocking theo moc thoi gian TIM2. */
void RA_Buzzer_Process(uint32_t now_us, const RA_Output_t *output)
{
    const RA_BeepMode_t mode = RA_Buzzer_SelectMode(output);
    uint32_t elapsed_us = now_us - s_beep_start_us;
    uint32_t period_us;
    uint32_t on_time_us;

    if (mode == RA_BEEP_CONTINUOUS)
    {
        RA_Buzzer_Apply(true);
        s_beep_start_us = now_us;
        return;
    }

    if (mode == RA_BEEP_OFF)
    {
        RA_Buzzer_Apply(false);
        s_beep_start_us = now_us;
        return;
    }

    if (mode == RA_BEEP_FAST)
    {
        period_us = RA_BUZZER_FAST_PERIOD_US;
        on_time_us = RA_BUZZER_FAST_ON_US;
    }
    else
    {
        period_us = RA_BUZZER_SLOW_PERIOD_US;
        on_time_us = RA_BUZZER_SLOW_ON_US;
    }

    if (elapsed_us >= period_us)
    {
        s_beep_start_us = now_us;
        elapsed_us = 0UL;
    }

    RA_Buzzer_Apply(elapsed_us < on_time_us);
}
