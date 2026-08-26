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
    pid->integral_limit = (ki != 0.0f) ? ((max_out - min_out) / (2.0f * ((ki > 0.0f) ? ki : -ki))) : 0.0f;
    pid->prev_error = 0.0f;
    pid->min_output = min_out;
    pid->max_output = max_out;
}

void PID_SetTunings(PID_Controller_t *pid, float kp, float ki, float kd) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral_limit = (ki != 0.0f) ?
        ((pid->max_output - pid->min_output) /
         (2.0f * ((ki > 0.0f) ? ki : -ki))) : 0.0f;
    PID_Reset(pid);
}

float PID_Compute(PID_Controller_t *pid, float measured_value, float dt_s) {
    /* Tránh lỗi chia cho 0 hoặc khoảng thời gian lấy mẫu không hợp lệ */
    if (dt_s <= 0.0f) {
        return 0.0f;
    }

    /* 1. Tính sai số (Error) */
    float error = pid->target - measured_value;

    /* 2. Tính thành phần Vi phân (Derivative) */
    float derivative = (error - pid->prev_error) / dt_s;
    pid->prev_error = error;

    /*
     * Chỉ tích phân khi đầu ra chưa bão hòa hoặc sai số đang kéo đầu ra trở
     * lại vùng làm việc. Điều này tránh integral wind-up khi xe bị kẹt.
     */
    float candidate_integral = pid->integral + error * dt_s;
    if (pid->integral_limit > 0.0f) {
        if (candidate_integral > pid->integral_limit) candidate_integral = pid->integral_limit;
        if (candidate_integral < -pid->integral_limit) candidate_integral = -pid->integral_limit;
    }

    float output = (pid->kp * error) + (pid->ki * candidate_integral) + (pid->kd * derivative);
    if (!((output >= pid->max_output && error > 0.0f) ||
          (output <= pid->min_output && error < 0.0f))) {
        pid->integral = candidate_integral;
    }

    /* 3. Tổng hợp tín hiệu điều khiển PID đầu ra */
    output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);

    /* 4. Giới hạn bão hòa tín hiệu đầu ra (Output Saturation) */
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
