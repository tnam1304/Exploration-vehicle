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
#include "odometry.h"

#define TASK_PID_PERIOD_US          50000U
#define TASK_SONAR_PERIOD_US        60000U
#define TASK_DISPLAY_PERIOD_US      100000U
#define TASK_SLOW_PERIOD_US         800000U
#define WATCHDOG_TIMEOUT_US         500000U

#define DT_MAX_LIMIT_SEC            0.065f
#define DT_DEFAULT_SEC              0.050f

/* 20..100% trên giao diện tương ứng với các setpoint ổn định cho encoder 20 PPR. */
#define TARGET_SPEED_FWD_MIN        15.0f
#define TARGET_SPEED_FWD_MAX        35.0f
#define TARGET_SPEED_TURN_MIN       12.0f
#define TARGET_SPEED_TURN_MAX       25.0f

/* PID chỉ sinh phần hiệu chỉnh PWM; chiều quay được quyết định bởi lệnh lái. */
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

#define OBSTACLE_STOP_LIMIT_CM      10U
#define OBSTACLE_SLOW_LIMIT_CM      50U
#define PID_APPROACH_MIN_SPEED      12.0f
#define TEMP_WARN_LIMIT_C           45

static float ClampFloat(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static float AbsoluteFloat(float value) {
    return (value < 0.0f) ? -value : value;
}

static float LimitPwmSlew(float desired, float previous, float dt_s) {
    float max_delta = PID_PWM_SLEW_PER_SEC * dt_s;
    if (desired > previous + max_delta) return previous + max_delta;
    if (desired < previous - max_delta) return previous - max_delta;
    return desired;
}

static float TargetSpeedForPercent(float min_speed, float max_speed) {
    float ratio = ((float)control_speed_percent - (float)CONTROL_SPEED_MIN_PERCENT) /
                  (float)(CONTROL_SPEED_MAX_PERCENT - CONTROL_SPEED_MIN_PERCENT);
    ratio = ClampFloat(ratio, 0.0f, 1.0f);
    return min_speed + (max_speed - min_speed) * ratio;
}

static void GetPidDrive(int8_t *left_direction, int8_t *right_direction, float *target_speed) {
    *left_direction = 0;
    *right_direction = 0;
    *target_speed = 0.0f;

    if (drive_cmd == 'F') {
        if (distance_cm == 0U || distance_cm > OBSTACLE_STOP_LIMIT_CM) {
            *left_direction = 1;
            *right_direction = 1;
            *target_speed = TargetSpeedForPercent(TARGET_SPEED_FWD_MIN, TARGET_SPEED_FWD_MAX);
            if (distance_cm > OBSTACLE_STOP_LIMIT_CM && distance_cm <= OBSTACLE_SLOW_LIMIT_CM) {
                float ratio = (float)(distance_cm - OBSTACLE_STOP_LIMIT_CM) /
                              (float)(OBSTACLE_SLOW_LIMIT_CM - OBSTACLE_STOP_LIMIT_CM);
                *target_speed = PID_APPROACH_MIN_SPEED +
                                (*target_speed - PID_APPROACH_MIN_SPEED) * ClampFloat(ratio, 0.0f, 1.0f);
            }
        }
    } else if (drive_cmd == 'B') {
        *left_direction = -1;
        *right_direction = -1;
        *target_speed = TargetSpeedForPercent(TARGET_SPEED_FWD_MIN, TARGET_SPEED_FWD_MAX);
    } else if (drive_cmd == 'L') {
        *left_direction = -1;
        *right_direction = 1;
        *target_speed = TargetSpeedForPercent(TARGET_SPEED_TURN_MIN, TARGET_SPEED_TURN_MAX);
    } else if (drive_cmd == 'R') {
        *left_direction = 1;
        *right_direction = -1;
        *target_speed = TargetSpeedForPercent(TARGET_SPEED_TURN_MIN, TARGET_SPEED_TURN_MAX);
    }
}

static void UpdateObstacleSafety(uint32_t measured_distance, int16_t temp_c) {
    distance_cm = measured_distance;

    if (distance_cm > 0U && distance_cm <= OBSTACLE_STOP_LIMIT_CM) {
        warn_code = SAFETY_WARN_OBSTACLE;
        pid_left.target = 0.0f;
        pid_right.target = 0.0f;
        if (drive_cmd == 'F') {
            Car_Stop();
        }
    } else if (temp_c <= TEMP_WARN_LIMIT_C) {
        warn_code = SAFETY_WARN_NONE;
    }

    Update_Buzzer_State();
}

int main(void) {
    Timer2_Init();
    Peripherals_Init();
    Encoder_Init();
    Odometry_Init();
    OLED_Init();
    OLED_Clear();

    PID_Init(&pid_left, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT,
             PID_CORRECTION_MIN_PWM, PID_CORRECTION_MAX_PWM);
    PID_Init(&pid_right, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT,
             PID_CORRECTION_MIN_PWM, PID_CORRECTION_MAX_PWM);

    Kalman1D_t kf_temp;
    Kalman1D_Init(&kf_temp, KALMAN_Q_TEMP, KALMAN_R_TEMP, KALMAN_P_TEMP, KALMAN_INIT_TEMP);

    int16_t raw_temp = TEMP_SENSOR_ERROR_RAW;
    int16_t temp_c = 0;
    float applied_pwm_left = 0.0f;
    float applied_pwm_right = 0.0f;
    char previous_pid_command = 'S';
    uint8_t was_pid_enabled = 0;

    uint32_t last_pid_task = TIM2->CNT;
    uint32_t last_sonar_task = TIM2->CNT - TASK_SONAR_PERIOD_US;
    uint32_t last_display_task = TIM2->CNT;
    uint32_t last_slow_task = TIM2->CNT;
    last_cmd_time = TIM2->CNT;

    DS18B20_StartConversion();

    while (1) {
        uint32_t current_time = TIM2->CNT;

        /* PID luôn được xét trước các tác vụ nền để giữ chu kỳ điều khiển ổn định. */
        if ((current_time - last_pid_task) >= TASK_PID_PERIOD_US) {
            float dt = (float)(current_time - last_pid_task) / 1000000.0f;
            last_pid_task = current_time;
            if (dt > DT_MAX_LIMIT_SEC) {
                dt = DT_DEFAULT_SEC;
            }

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

                if (left_direction == 0 || right_direction == 0) {
                    pid_left.target = 0.0f;
                    pid_right.target = 0.0f;
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                    applied_pwm_left = 0.0f;
                    applied_pwm_right = 0.0f;
                    Car_Stop();
                } else {
                    /* PID điều khiển biên độ; không bao giờ dùng PWM âm để đảo cầu H. */
                    pid_left.target = target_speed;
                    pid_right.target = target_speed;

                    float desired_left = PID_FEEDFORWARD_PWM_PER_CMS * target_speed +
                                         PID_Compute(&pid_left, AbsoluteFloat(g_odometry.speed_left_cms), dt);
                    float desired_right = PID_FEEDFORWARD_PWM_PER_CMS * target_speed +
                                          PID_Compute(&pid_right, AbsoluteFloat(g_odometry.speed_right_cms), dt);

                    desired_left = ClampFloat(desired_left, PWM_MIN, PWM_MAX);
                    desired_right = ClampFloat(desired_right, PWM_MIN, PWM_MAX);
                    applied_pwm_left = LimitPwmSlew(desired_left, applied_pwm_left, dt);
                    applied_pwm_right = LimitPwmSlew(desired_right, applied_pwm_right, dt);

                    Motor_Left_SetSpeed((int16_t)(left_direction * applied_pwm_left));
                    Motor_Right_SetSpeed((int16_t)(right_direction * applied_pwm_right));
                }
            } else {
                was_pid_enabled = 0U;
                applied_pwm_left = 0.0f;
                applied_pwm_right = 0.0f;
            }
        }

        if ((current_time - last_cmd_time) > WATCHDOG_TIMEOUT_US) {
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
            wifi_status_online = 0;
        }

        /* HC-SR04 được thăm dò không chặn để không làm trễ PID. */
        if ((current_time - last_sonar_task) >= TASK_SONAR_PERIOD_US) {
            Sonar_StartMeasurement();
            last_sonar_task = current_time;
        }
        {
            uint32_t sonar_reading;
            if (Sonar_Poll(&sonar_reading)) {
                UpdateObstacleSafety(sonar_reading, temp_c);
            }
        }

        /* Nhiệt độ và telemetry có thể chậm hơn, nhưng UART 115200 không còn chặn PID lâu. */
        if ((current_time - last_slow_task) >= TASK_SLOW_PERIOD_US) {
            last_slow_task = current_time;
            raw_temp = DS18B20_ReadRawTemp();
            DS18B20_StartConversion();

            if (raw_temp != TEMP_SENSOR_ERROR_RAW) {
                float current_temp_f = (float)raw_temp / TEMP_SCALE_FACTOR;
                temp_c = (int16_t)Kalman1D_Update(&kf_temp, current_temp_f);
            } else {
                temp_c = 0;
            }

            if (temp_c > TEMP_WARN_LIMIT_C) {
                warn_code = SAFETY_WARN_FIRE;
                Update_Buzzer_State();
            }

            UART_Send_Telemetry(raw_temp, distance_cm, warn_code,
                                Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                                g_odometry.total_distance_cm);
        }

        /* OLED được cập nhật từng hàng, tránh một lần ghi kéo dài gần 100 ms. */
        if ((current_time - last_display_task) >= TASK_DISPLAY_PERIOD_US) {
            last_display_task = current_time;
            App_Display_Render(wifi_status_online, distance_cm, temp_c, warn_code,
                               Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                               g_odometry.total_distance_cm);
        }
    }
}
