/**
 * @file    app_pid.c
 * @brief   Triển khai bộ điều khiển PID tốc độ (Tầng App)
 * @author  Tran Van Phuong
 */

#include "app_pid.h"

PID_Controller_t g_pid_left;
PID_Controller_t g_pid_right;

void App_PID_Init(PID_Controller_t *pid, float Kp, float Ki, float Kd, float min, float max, float dt) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->out_min = min;
    pid->out_max = max;
    pid->dt = dt;
    App_PID_Reset(pid);
}

void App_PID_Reset(PID_Controller_t *pid) {
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

float App_PID_Compute(PID_Controller_t *pid, float setpoint, float feedback) {
    pid->setpoint = setpoint;
    float error = pid->setpoint - feedback;

    // 1. Thành phần Tỷ lệ (P)
    float p_out = pid->Kp * error;

    // 2. Thành phần Tích phân (I)
    pid->integral += error * pid->dt;
    float i_out = pid->Ki * pid->integral;

    // 3. Thành phần Đạo hàm (D)
    float derivative = (error - pid->prev_error) / pid->dt;
    float d_out = pid->Kd * derivative;

    // Tổng đầu ra lý thuyết
    float output = p_out + i_out + d_out;

    // Giới hạn đầu ra PWM và Anti-Windup cho tích phân
    if (output > pid->out_max) {
        output = pid->out_max;
        pid->integral -= error * pid->dt; // Chống tràn tích phân
    } else if (output < pid->out_min) {
        output = pid->out_min;
        pid->integral -= error * pid->dt;
    }

    pid->prev_error = error;
    return output;
}

void App_PID_Motors_Init(void) {
    // Khởi tạo PID cho Bánh Trái và Bánh Phải với chu kỳ dt = 0.02s (20ms)
    // Các thông số Kp, Ki, Kd ban đầu gợi ý thử nghiệm: Kp = 15.0, Ki = 2.0, Kd = 0.1
    App_PID_Init(&g_pid_left,  15.0f, 2.0f, 0.1f, -1000.0f, 1000.0f, 0.02f);
    App_PID_Init(&g_pid_right, 15.0f, 2.0f, 0.1f, -1000.0f, 1000.0f, 0.02f);
}
