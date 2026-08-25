/**
 * @file    app_control.c
 * @brief   Triển khai điều hướng di chuyển và tự động giảm tốc/dừng theo cảm biến
 */

#include "app_control.h"
#include "dev_motor.h"
#include "app_safety.h"

/* Cờ cho phép sử dụng bộ điều khiển PID */
volatile uint8_t pid_enable = 1;
volatile uint8_t control_speed_percent = CONTROL_SPEED_DEFAULT_PERCENT;

/* Giới hạn do Forward Safety/Rear Boost áp lên lệnh tiến ở chế độ PWM. */
static volatile uint8_t safety_stop_requested = 0U;
static volatile uint8_t safety_speed_percent = 100U;

/* Biến lưu giá trị tốc độ PWM cho từng hướng di chuyển */
uint16_t speed_fwd_left  = DEFAULT_SPEED_PWM;
uint16_t speed_fwd_right = DEFAULT_SPEED_PWM;
uint16_t speed_bwd_left  = DEFAULT_SPEED_PWM;
uint16_t speed_bwd_right = DEFAULT_SPEED_PWM;

/* Lệnh di chuyển hiện tại ('F', 'B', 'L', 'R', 'S') */
volatile char drive_cmd = 'S';

/* CÁC HÀM ĐIỀU HƯỚNG CƠ BẢN (BASIC MOVEMENT FUNCTIONS)                       */

void Car_Forward_Normal(void) {
    Motor_Left_SetSpeed(speed_fwd_left);
    Motor_Right_SetSpeed(speed_fwd_right);
}

void Car_Backward(void) {
    Motor_Left_SetSpeed(-speed_bwd_left);
    Motor_Right_SetSpeed(-speed_bwd_right);
}

void Car_TurnLeft(void) {
    Motor_Left_SetSpeed(-speed_fwd_left);
    Motor_Right_SetSpeed(speed_fwd_right);
}

void Car_TurnRight(void) {
    Motor_Left_SetSpeed(speed_fwd_left);
    Motor_Right_SetSpeed(-speed_fwd_right);
}

void Car_Stop(void) {
    Motor_Left_SetSpeed(0);
    Motor_Right_SetSpeed(0);
}

/* HÀM CẬP NHẬT TRẠNG THÁI TỔNG HỢP (COMPOSITE UPDATE FUNCTION)               */

void Update_Motors_From_Cmd(void) {
    if (drive_cmd == 'F') {
        if (safety_stop_requested != 0U) {
            Car_Stop();
        } else {
            uint32_t left_pwm =
                ((uint32_t)speed_fwd_left * safety_speed_percent) / 100U;
            uint32_t right_pwm =
                ((uint32_t)speed_fwd_right * safety_speed_percent) / 100U;
            if (left_pwm > 1000U) left_pwm = 1000U;
            if (right_pwm > 1000U) right_pwm = 1000U;
            Motor_Left_SetSpeed((int16_t)left_pwm);
            Motor_Right_SetSpeed((int16_t)right_pwm);
        }
    }
    else if (drive_cmd == 'B') {
        Car_Backward();
    }
    else if (drive_cmd == 'L') {
        Car_TurnLeft();
    }
    else if (drive_cmd == 'R') {
        Car_TurnRight();
    }
    else if (drive_cmd == 'S') {
        Car_Stop();
    }
}

void Control_SetSpeedPercent(uint8_t percent) {
    if (percent < CONTROL_SPEED_MIN_PERCENT || percent > CONTROL_SPEED_MAX_PERCENT) {
        return;
    }

    control_speed_percent = percent;
    uint16_t pwm = (uint16_t)percent * 10U;
    speed_fwd_left  = pwm;
    speed_fwd_right = pwm;
    speed_bwd_left  = pwm;
    speed_bwd_right = pwm;
}

void Control_SetForwardSafetyOverride(uint8_t stop_requested,
                                      uint8_t speed_percent) {
    safety_stop_requested = (stop_requested != 0U) ? 1U : 0U;
    safety_speed_percent = speed_percent;
    if (safety_speed_percent > 115U) {
        safety_speed_percent = 115U;
    }
}
