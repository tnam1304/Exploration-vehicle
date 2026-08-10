/**
 * @file    app_pid.c
 * @brief   Triển khai thuật toán PID kín điều khiển tốc độ động cơ
 */

#include "app_pid.h"

/* Khai báo đối tượng PID cho động cơ trái và động cơ phải */
PID_Controller_t pid_left;
PID_Controller_t pid_right;

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, float min_out, float max_out) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->target = 0.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->min_output = min_out;
    pid->max_output = max_out;
}

float PID_Compute(PID_Controller_t *pid, float measured_value, float dt_s) {
    /* Tránh lỗi chia cho 0 hoặc khoảng thời gian lấy mẫu không hợp lệ */
    if (dt_s <= 0.0f) {
        return 0.0f;
    }

    /* 1. Tính sai số (Error) */
    float error = pid->target - measured_value;

    /* 2. Tính thành phần Tích phân (Integral) */
    pid->integral += error * dt_s;

    /* Anti-windup cho thành phần tích phân để tránh tràn/quá đà */
    if (pid->integral > pid->max_output) {
        pid->integral = pid->max_output;
    }
    if (pid->integral < pid->min_output) {
        pid->integral = pid->min_output;
    }

    /* 3. Tính thành phần Vi phân (Derivative) */
    float derivative = (error - pid->prev_error) / dt_s;
    pid->prev_error = error;

    /* 4. Tổng hợp tín hiệu điều khiển PID đầu ra */
    float output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    /* 5. Giới hạn bão hòa tín hiệu đầu ra (Output Saturation) */
    if (output > pid->max_output) {
        output = pid->max_output;
    }
    if (output < pid->min_output) {
        output = pid->min_output;
    }

    return output;
}

void PID_Reset(PID_Controller_t *pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}