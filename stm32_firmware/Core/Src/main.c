/**
 * @file    main.c
 * @brief   Vòng điều khiển thời gian thực của xe thám hiểm STM32F401RCT6.
 */

#include "stm32f4xx.h"
#include "bsp_pinout.h"
#include "dev_oled.h"
#include "dev_sonar.h"
#include "dev_ds18b20.h"
#include "dev_encoder.h"
#include "dev_motor.h"
#include "protocol_uart.h"
#include "app_safety.h"
#include "app_control.h"
#include "app_display.h"
#include "app_pid.h"
#include "app_forward_safety.h"
#include "app_reverse_warning.h"
#include "app_rear_collision.h"
#include "filter_median.h"
#include "odometry.h"

#include <stdbool.h>

#define TASK_PID_PERIOD_US          50000U
#define TASK_DISPLAY_PERIOD_US      100000U
#define TASK_TELEMETRY_PERIOD_US    800000U
#define TASK_SLOW_PERIOD_US         800000U
#define WATCHDOG_TIMEOUT_US         500000U
#define DT_MAX_LIMIT_SEC            0.065f
#define DT_DEFAULT_SEC              0.050f

#define TARGET_SPEED_FWD_MIN        15.0f
#define TARGET_SPEED_FWD_MAX        35.0f
#define TARGET_SPEED_TURN_MIN       12.0f
#define TARGET_SPEED_TURN_MAX       25.0f
#define PID_KP_DEFAULT              8.0f
#define PID_KI_DEFAULT              2.0f
#define PID_KD_DEFAULT              0.0f
#define PID_CORRECTION_MIN_PWM     -300.0f
#define PID_CORRECTION_MAX_PWM      300.0f
#define PID_FEEDFORWARD_PWM_PER_CMS 13.0f
#define PID_PWM_SLEW_PER_SEC        500.0f
#define PWM_MIN                     0.0f
#define PWM_MAX                     1000.0f

#define KALMAN_Q_TEMP               0.01f
#define KALMAN_R_TEMP               2.0f
#define KALMAN_P_TEMP               1.0f
#define KALMAN_INIT_TEMP            25.0f
#define TEMP_WARN_LIMIT_C           45
#define REAR_BOOST_PERCENT          115U

#define ENCODER_STALL_MIN_PWM       250.0f
#define ENCODER_STALL_MAX_SPEED_CMS 1.0f
#define ENCODER_STALL_TIMEOUT_US    1500000U
#define FRONT_SONAR_STALE_US        200000U
#define FRONT_SONAR_FAULT_SAMPLES   3U
#define FRONT_SONAR_RECOVER_SAMPLES 3U

typedef struct {
    Dev_SonarId_t id;
    Filter_Median_t median;
    uint8_t filter_ready;
    uint8_t valid;
    uint32_t distance_cm;
} Main_SonarChannel_t;

static float ClampFloat(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float AbsoluteFloat(float value) {
    return (value < 0.0f) ? -value : value;
}

static float MaximumFloat(float first, float second) {
    return (first > second) ? first : second;
}

static float LimitPwmSlew(float desired, float previous, float dt_s) {
    const float max_delta = PID_PWM_SLEW_PER_SEC * dt_s;
    if (desired > previous + max_delta) return previous + max_delta;
    if (desired < previous - max_delta) return previous - max_delta;
    return desired;
}

static float TargetSpeedForPercent(float min_speed, float max_speed) {
    float ratio = ((float)control_speed_percent - CONTROL_SPEED_MIN_PERCENT) /
                  (CONTROL_SPEED_MAX_PERCENT - CONTROL_SPEED_MIN_PERCENT);
    ratio = ClampFloat(ratio, 0.0f, 1.0f);
    return min_speed + (max_speed - min_speed) * ratio;
}

static float CommandedForwardSpeed(void) {
    return TargetSpeedForPercent(TARGET_SPEED_FWD_MIN, TARGET_SPEED_FWD_MAX);
}

static void GetPidDrive(int8_t *left_direction, int8_t *right_direction,
                        float *target_speed) {
    *left_direction = 0;
    *right_direction = 0;
    *target_speed = 0.0f;
    if (drive_cmd == 'F') {
        *left_direction = 1; *right_direction = 1;
        *target_speed = CommandedForwardSpeed();
    } else if (drive_cmd == 'B') {
        *left_direction = -1; *right_direction = -1;
        *target_speed = CommandedForwardSpeed();
    } else if (drive_cmd == 'L') {
        *left_direction = -1; *right_direction = 1;
        *target_speed = TargetSpeedForPercent(TARGET_SPEED_TURN_MIN,
                                             TARGET_SPEED_TURN_MAX);
    } else if (drive_cmd == 'R') {
        *left_direction = 1; *right_direction = -1;
        *target_speed = TargetSpeedForPercent(TARGET_SPEED_TURN_MIN,
                                             TARGET_SPEED_TURN_MAX);
    }
}

static bool Main_InitSonar(Main_SonarChannel_t *channel,
                           GPIO_TypeDef *trigger_port, uint8_t trigger_pin,
                           GPIO_TypeDef *echo_port, uint8_t echo_pin) {
    channel->id = Dev_Sonar_Add(trigger_port, trigger_pin,
                                echo_port, echo_pin);
    channel->filter_ready = 0U;
    channel->valid = 0U;
    channel->distance_cm = SONAR_ERROR_RAW_DIST;
    return channel->id != DEV_SONAR_ID_INVALID;
}

static bool Main_ConsumeSonar(Main_SonarChannel_t *channel) {
    Dev_SonarMeasurement_t measurement;
    uint16_t measured_cm;
    if (channel->id == DEV_SONAR_ID_INVALID ||
        !Dev_Sonar_GetNewData(channel->id, &measurement)) return false;

    if (!measurement.valid) {
        channel->valid = 0U;
        channel->distance_cm = SONAR_ERROR_RAW_DIST;
        return true;
    }

    measured_cm = (uint16_t)(((uint32_t)measurement.distance_mm + 5U) / 10U);
    if (channel->filter_ready == 0U) {
        Filter_Median_Init(&channel->median, measured_cm);
        channel->filter_ready = 1U;
    }
    channel->distance_cm = Filter_Median_Update(&channel->median, measured_cm);
    channel->valid = 1U;
    return true;
}

static void Main_UpdateSonarSchedule(
    char command,
    bool measurement_completed,
    const Main_SonarChannel_t *front,
    const Main_SonarChannel_t *rear_left,
    const Main_SonarChannel_t *rear_right) {
    static uint8_t forward_slot = 0U;
    static uint8_t normal_slot = 0U;
    Dev_SonarId_t next_id;

    if (!measurement_completed) return;

    if (command == 'F') {
        static const uint8_t schedule[6] = {0U, 0U, 1U, 0U, 0U, 2U};
        const uint8_t selected = schedule[forward_slot];
        forward_slot = (uint8_t)((forward_slot + 1U) % 6U);
        if (selected == 0U) next_id = front->id;
        else if (selected == 1U) next_id = rear_left->id;
        else next_id = rear_right->id;
    } else {
        static const uint8_t schedule[3] = {0U, 1U, 2U};
        const uint8_t selected = schedule[normal_slot];
        normal_slot = (uint8_t)((normal_slot + 1U) % 3U);
        if (selected == 0U) next_id = front->id;
        else if (selected == 1U) next_id = rear_left->id;
        else next_id = rear_right->id;
    }

    if (next_id != DEV_SONAR_ID_INVALID)
        Dev_Sonar_SetScanMask(DEV_SONAR_MASK(next_id));
}

static uint8_t Main_UpdateEncoderFeedback(uint32_t now_us,
                                          uint8_t movement_requested,
                                          float applied_power_pwm) {
    static uint32_t stalled_since_us = 0U;
    static uint8_t encoder_ok = 1U;
    const float speed = MaximumFloat(AbsoluteFloat(g_odometry.speed_left_cms),
                                     AbsoluteFloat(g_odometry.speed_right_cms));
    if (speed > ENCODER_STALL_MAX_SPEED_CMS) {
        encoder_ok = 1U;
        stalled_since_us = 0U;
    } else if (movement_requested != 0U &&
               applied_power_pwm >= ENCODER_STALL_MIN_PWM) {
        if (stalled_since_us == 0U) stalled_since_us = now_us;
        else if ((uint32_t)(now_us - stalled_since_us) >=
                 ENCODER_STALL_TIMEOUT_US) encoder_ok = 0U;
    } else {
        stalled_since_us = 0U;
    }
    return encoder_ok;
}

static Safety_State_t Main_SelectSafetyState(
    const App_ForwardSafetyOutput_t *forward,
    const App_ReverseWarningOutput_t *reverse,
    const App_RearCollisionOutput_t *rear,
    uint8_t encoder_ok, uint8_t sonar_config_ok) {
    (void)sonar_config_ok;
    /* Khi dang lui, canh bao theo hai sonar sau. AEB phia truoc van duoc
       latch noi bo de chan lan tien tiep theo neu vat can con gan. */
    if (drive_cmd == 'B') {
        return (reverse->state == SAFETY_STATE_SONAR_ERROR) ?
               SAFETY_STATE_SAFE : reverse->state;
    }
    if (forward->state == SAFETY_STATE_AEB ||
        forward->state == SAFETY_STATE_AEB_HOLD) return forward->state;
    if (drive_cmd == 'F' && forward->state != SAFETY_STATE_SAFE &&
        forward->state != SAFETY_STATE_SONAR_ERROR)
        return forward->state;
    if (rear->boost_active != 0U) return SAFETY_STATE_REAR_BOOST;
    if (rear->state == REAR_COLLISION_DANGER) return SAFETY_STATE_REAR_DANGER;
    if (rear->state == REAR_COLLISION_WARNING) return SAFETY_STATE_REAR_WARNING;
    if (rear->state == REAR_COLLISION_APPROACHING)
        return SAFETY_STATE_REAR_APPROACHING;
    if (encoder_ok == 0U) return SAFETY_STATE_ENCODER_ERROR;
    return SAFETY_STATE_SAFE;
}

int main(void) {
    Main_SonarChannel_t front_sonar;
    Main_SonarChannel_t rear_left_sonar;
    Main_SonarChannel_t rear_right_sonar;
    App_ForwardSafetyOutput_t forward_output = {0};
    App_ReverseWarningOutput_t reverse_output = {0};
    App_RearCollisionOutput_t rear_output = {0};
    uint8_t sonar_config_ok;
    Kalman1D_t kf_temp;
    int16_t raw_temp = TEMP_SENSOR_ERROR_RAW;
    int16_t temp_c = 0;
    float applied_pwm_left = 0.0f;
    float applied_pwm_right = 0.0f;
    char previous_pid_command = 'S';
    char previous_drive_command = 'S';
    uint8_t was_pid_enabled = 0U;
    uint8_t encoder_ok = 1U;
    uint8_t safety_stop = 0U;
    uint8_t safety_speed_percent = 100U;
    uint8_t last_pwm_safety_stop = 0U;
    uint8_t last_pwm_safety_percent = 100U;
    Safety_State_t safety_state = SAFETY_STATE_SAFE;
    Safety_Side_t safety_side = SAFETY_SIDE_NONE;
    uint8_t front_fault_latched = 0U;
    uint8_t front_invalid_samples = 0U;
    uint8_t front_recovery_samples = 0U;
    uint32_t last_front_sample_time;
    uint32_t last_pid_task;
    uint32_t last_display_task;
    uint32_t last_telemetry_task;
    uint32_t last_slow_task;

    Timer2_Init();
    Peripherals_Init();
    Encoder_Init();
    Odometry_Init();
    OLED_Init();
    OLED_Clear();

    Dev_Sonar_Init();
    {
        const bool front_ok =
            Main_InitSonar(&front_sonar, GPIOB, HC_SR04_TRIG_PIN,
                           GPIOB, HC_SR04_ECHO_PIN);
        const bool rear_left_ok =
            Main_InitSonar(&rear_left_sonar, GPIOA, SONAR_REAR_LEFT_TRIG_PIN,
                           GPIOB, SONAR_REAR_LEFT_ECHO_PIN);
        const bool rear_right_ok =
            Main_InitSonar(&rear_right_sonar, GPIOB, SONAR_REAR_RIGHT_TRIG_PIN,
                           GPIOB, SONAR_REAR_RIGHT_ECHO_PIN);
        sonar_config_ok = front_ok && rear_left_ok && rear_right_ok;
    }

    App_ForwardSafety_Init();
    App_ReverseWarning_Init();
    App_RearCollision_Init();
    PID_Init(&pid_left, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT,
             PID_CORRECTION_MIN_PWM, PID_CORRECTION_MAX_PWM);
    PID_Init(&pid_right, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT,
             PID_CORRECTION_MIN_PWM, PID_CORRECTION_MAX_PWM);
    Kalman1D_Init(&kf_temp, KALMAN_Q_TEMP, KALMAN_R_TEMP,
                  KALMAN_P_TEMP, KALMAN_INIT_TEMP);

    last_pid_task = TIM2->CNT;
    last_display_task = TIM2->CNT;
    last_telemetry_task = TIM2->CNT;
    last_slow_task = TIM2->CNT;
    last_cmd_time = TIM2->CNT;
    last_front_sample_time = TIM2->CNT;
    DS18B20_StartConversion();

    while (1) {
        uint32_t current_time = TIM2->CNT;
        bool front_new;
        bool rear_left_new;
        bool rear_right_new;
        float vehicle_speed_cms;
        float applied_power_pwm;
        uint8_t movement_requested;

        Dev_Sonar_Process();
        front_new = Main_ConsumeSonar(&front_sonar);
        rear_left_new = Main_ConsumeSonar(&rear_left_sonar);
        rear_right_new = Main_ConsumeSonar(&rear_right_sonar);
        Main_UpdateSonarSchedule(
            drive_cmd, front_new || rear_left_new || rear_right_new,
            &front_sonar, &rear_left_sonar, &rear_right_sonar);

        if (front_new) {
            last_front_sample_time = current_time;
            if (front_sonar.valid != 0U) {
                front_invalid_samples = 0U;
                if (front_recovery_samples < 255U)
                    front_recovery_samples++;
            } else {
                front_recovery_samples = 0U;
                if (front_invalid_samples < 255U)
                    front_invalid_samples++;
                if (drive_cmd == 'F' &&
                    front_invalid_samples >= FRONT_SONAR_FAULT_SAMPLES)
                    front_fault_latched = 1U;
            }
        }

        if (drive_cmd == 'F' &&
            (uint32_t)(current_time - last_front_sample_time) >=
            FRONT_SONAR_STALE_US)
            front_fault_latched = 1U;

        if (drive_cmd == 'S' && front_sonar.valid != 0U &&
            front_recovery_samples >= FRONT_SONAR_RECOVER_SAMPLES)
            front_fault_latched = 0U;

        distance_cm = front_sonar.valid ? front_sonar.distance_cm :
                      SONAR_ERROR_RAW_DIST;

        if (drive_cmd == 'F' && previous_drive_command != 'F')
            App_ForwardSafety_RequestRestart();
        previous_drive_command = drive_cmd;

        vehicle_speed_cms = MaximumFloat(
            AbsoluteFloat(g_odometry.speed_left_cms),
            AbsoluteFloat(g_odometry.speed_right_cms));
        movement_requested = (drive_cmd == 'F' || drive_cmd == 'B' ||
                              drive_cmd == 'L' || drive_cmd == 'R') ? 1U : 0U;
        applied_power_pwm = (pid_enable != 0U) ?
            (applied_pwm_left + applied_pwm_right) * 0.5f :
            ((float)control_speed_percent * 10.0f *
             (float)safety_speed_percent / 100.0f);
        if (safety_stop != 0U) applied_power_pwm = 0.0f;
        encoder_ok = Main_UpdateEncoderFeedback(current_time,
                                                movement_requested,
                                                applied_power_pwm);

        App_ForwardSafety_Process(
            drive_cmd == 'F', front_new, front_sonar.valid != 0U,
            (float)front_sonar.distance_cm, vehicle_speed_cms,
            CommandedForwardSpeed(), &forward_output);
        App_ReverseWarning_Process(
            drive_cmd == 'B', rear_left_new || rear_right_new,
            rear_left_sonar.valid != 0U, (float)rear_left_sonar.distance_cm,
            rear_right_sonar.valid != 0U, (float)rear_right_sonar.distance_cm,
            &reverse_output);
        App_RearCollision_Process(
            drive_cmd == 'B', drive_cmd == 'F',
            rear_left_new, rear_left_sonar.valid != 0U,
            (float)rear_left_sonar.distance_cm,
            rear_right_new, rear_right_sonar.valid != 0U,
            (float)rear_right_sonar.distance_cm,
            front_sonar.valid != 0U, (float)front_sonar.distance_cm,
            forward_output.fcw_threshold_cm, vehicle_speed_cms,
            encoder_ok != 0U, &rear_output);

        safety_state = Main_SelectSafetyState(&forward_output, &reverse_output,
                                               &rear_output, encoder_ok,
                                               sonar_config_ok);
        safety_side = (drive_cmd == 'B') ? reverse_output.side : rear_output.side;
        /*
         * Forward Safety chi duoc chan chieu tien. Khi AEB_HOLD, nguoi lai
         * van phai duoc phep lui hoac re de thoat vat can; latch AEB van duoc
         * giu va se chan lai neu co lenh tien khi phia truoc chua an toan.
         */
        safety_stop = (drive_cmd == 'F') ?
            forward_output.stop_requested : 0U;
        if (front_fault_latched != 0U && drive_cmd == 'F') safety_stop = 1U;
        if (sonar_config_ok == 0U && drive_cmd == 'F') safety_stop = 1U;
        safety_speed_percent = forward_output.speed_percent;
        if (rear_output.boost_active != 0U &&
            forward_output.state == SAFETY_STATE_SAFE)
            safety_speed_percent = REAR_BOOST_PERCENT;

        if (safety_stop != last_pwm_safety_stop ||
            safety_speed_percent != last_pwm_safety_percent) {
            Control_SetForwardSafetyOverride(safety_stop,
                                             safety_speed_percent);
            last_pwm_safety_stop = safety_stop;
            last_pwm_safety_percent = safety_speed_percent;
            if (pid_enable == 0U) Update_Motors_From_Cmd();
        }

        if (safety_stop != 0U) {
            pid_left.target = 0.0f;
            pid_right.target = 0.0f;
            PID_Reset(&pid_left);
            PID_Reset(&pid_right);
            applied_pwm_left = 0.0f;
            applied_pwm_right = 0.0f;
            Car_Stop();
        }
        Safety_Buzzer_Process(current_time, safety_state);

        if ((current_time - last_pid_task) >= TASK_PID_PERIOD_US) {
            float dt = (float)(current_time - last_pid_task) / 1000000.0f;
            last_pid_task = current_time;
            if (dt > DT_MAX_LIMIT_SEC) dt = DT_DEFAULT_SEC;
            Odometry_Update(dt);

            if (pid_enable != 0U) {
                int8_t left_direction;
                int8_t right_direction;
                float target_speed;
                GetPidDrive(&left_direction, &right_direction, &target_speed);

                if (!was_pid_enabled || drive_cmd != previous_pid_command) {
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                    applied_pwm_left = 0.0f;
                    applied_pwm_right = 0.0f;
                }
                previous_pid_command = drive_cmd;
                was_pid_enabled = 1U;
                if (drive_cmd == 'F')
                    target_speed *= (float)safety_speed_percent / 100.0f;

                if (left_direction == 0 || right_direction == 0 ||
                    safety_stop != 0U) {
                    pid_left.target = 0.0f;
                    pid_right.target = 0.0f;
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                    applied_pwm_left = 0.0f;
                    applied_pwm_right = 0.0f;
                    Car_Stop();
                } else {
                    float desired_left;
                    float desired_right;
                    pid_left.target = target_speed;
                    pid_right.target = target_speed;
                    desired_left = PID_FEEDFORWARD_PWM_PER_CMS * target_speed +
                        PID_Compute(&pid_left,
                                    AbsoluteFloat(g_odometry.speed_left_cms), dt);
                    desired_right = PID_FEEDFORWARD_PWM_PER_CMS * target_speed +
                        PID_Compute(&pid_right,
                                    AbsoluteFloat(g_odometry.speed_right_cms), dt);
                    desired_left = ClampFloat(desired_left, PWM_MIN, PWM_MAX);
                    desired_right = ClampFloat(desired_right, PWM_MIN, PWM_MAX);
                    applied_pwm_left = LimitPwmSlew(desired_left,
                                                    applied_pwm_left, dt);
                    applied_pwm_right = LimitPwmSlew(desired_right,
                                                     applied_pwm_right, dt);
                    Motor_Left_SetSpeed(
                        (int16_t)(left_direction * applied_pwm_left));
                    Motor_Right_SetSpeed(
                        (int16_t)(right_direction * applied_pwm_right));
                }
            } else {
                was_pid_enabled = 0U;
                applied_pwm_left = 0.0f;
                applied_pwm_right = 0.0f;
            }
        }

        if ((current_time - last_cmd_time) > WATCHDOG_TIMEOUT_US) {
            uint32_t irq_state = __get_PRIMASK();

            /* Xác nhận timeout bằng thời gian mới trong vùng không bị UART ISR
               cập nhật last_cmd_time giữa lúc kiểm tra và dừng motor. */
            __disable_irq();
            if ((TIM2->CNT - last_cmd_time) > WATCHDOG_TIMEOUT_US) {
                if (drive_cmd != 'S') {
                    drive_cmd = 'S';
                    pid_left.target = 0.0f;
                    pid_right.target = 0.0f;
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                    applied_pwm_left = 0.0f;
                    applied_pwm_right = 0.0f;
                    Car_Stop();
                }
                wifi_status_online = 0U;
            }
            if (irq_state == 0U) __enable_irq();
        }

        if ((current_time - last_slow_task) >= TASK_SLOW_PERIOD_US) {
            last_slow_task = current_time;
            raw_temp = DS18B20_ReadRawTemp();
            DS18B20_StartConversion();
            if (raw_temp != TEMP_SENSOR_ERROR_RAW) {
                float current_temp_f = (float)raw_temp / TEMP_SCALE_FACTOR;
                temp_c = (int16_t)Kalman1D_Update(&kf_temp, current_temp_f);
            } else temp_c = 0;

            if (temp_c > TEMP_WARN_LIMIT_C) warn_code = SAFETY_WARN_FIRE;
            else if (warn_code == SAFETY_WARN_FIRE) warn_code = SAFETY_WARN_NONE;
        }

        if ((current_time - last_telemetry_task) >= TASK_TELEMETRY_PERIOD_US) {
            last_telemetry_task = current_time;
            UART_Send_Telemetry(
                raw_temp, distance_cm, warn_code,
                Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                g_odometry.total_distance_cm,
                (int)safety_state, (int)rear_output.state,
                (int)safety_side, (int)rear_output.boost_active,
                (int)pid_enable, (int)encoder_ok,
                rear_left_sonar.distance_cm, rear_right_sonar.distance_cm);
        }

        if ((current_time - last_display_task) >= TASK_DISPLAY_PERIOD_US) {
            last_display_task = current_time;
            App_Display_Render(
                wifi_status_online, distance_cm, temp_c, warn_code,
                Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                g_odometry.total_distance_cm, safety_state, safety_side);
        }
    }
}
