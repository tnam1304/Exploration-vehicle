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
        /* Tự động dừng khẩn cấp nếu vật cản quá gần (<= 10cm) */
        if (distance_cm > 0 && distance_cm <= SAFETY_STOP_DIST_CM) {
            Car_Stop();
        } 
        /* Tự động giảm tốc tuyến tính khi tiệm cận vật cản (10cm < distance <= 50cm) */
        else if (distance_cm > SAFETY_STOP_DIST_CM && distance_cm <= SAFETY_SLOW_DIST_CM) {
            float ti_le = (float)(distance_cm - SAFETY_STOP_DIST_CM) / SAFETY_RANGE_DIST_CM;
            int left_pwm  = MIN_DECEL_PWM + (int)(ti_le * (speed_fwd_left - MIN_DECEL_PWM));
            int right_pwm = MIN_DECEL_PWM + (int)(ti_le * (speed_fwd_right - MIN_DECEL_PWM));
            
            Motor_Left_SetSpeed(left_pwm);
            Motor_Right_SetSpeed(right_pwm);
        } 
        /* Đủ khoảng cách an toàn -> Tiến bình thường */
        else {
            Car_Forward_Normal();
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
