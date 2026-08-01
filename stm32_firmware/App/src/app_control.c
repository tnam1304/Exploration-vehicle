/**
 * @file    app_control.c
 * @brief   Quản lý vòng lặp điều khiển chính của xe (Tầng App)
 */

#include "app_control.h"
#include "app_pid.h"
#include "dev_encoder.h"
#include "dev_motor.h"

/* Tốc độ mục tiêu cài đặt cho 2 bánh (đơn vị: cm/s) */
static float g_target_speed_left  = 0.0f;
static float g_target_speed_right = 0.0f;

void App_Control_Init(void) {
    Dev_Motor_Init();
    Dev_Encoder_Init();
    App_PID_Motors_Init();
}

/**
 * @brief Đặt tốc độ mục tiêu cho xe theo cm/s
 */
void App_Control_SetTargetSpeed(float speed_left_cms, float speed_right_cms) {
    g_target_speed_left  = speed_left_cms;
    g_target_speed_right = speed_right_cms;
}

/**
 * @brief Vòng lặp điều khiển PID (BẮT BUỘC GỌI ĐỊNH KỲ MỖI 20ms)
 * @note  Có thể gọi trong SysTick Interrupt hoặc Timer Task 20ms
 */
void App_Control_Task_20ms(void) {
    // 1. Đọc và cập nhật Encoder
    Dev_Encoder_Update();

    // 2. Lấy vận tốc thực tế hiện tại của 2 bánh (cm/s)
    float current_speed_left  = Dev_Encoder_Left_GetSpeed();
    float current_speed_right = Dev_Encoder_Right_GetSpeed();

    // 3. Tính toán PID xuất ra giá trị PWM
    float pwm_left  = App_PID_Compute(&g_pid_left,  g_target_speed_left,  current_speed_left);
    float pwm_right = App_PID_Compute(&g_pid_right, g_target_speed_right, current_speed_right);

    // 4. Xuất PWM xuống Driver Motor
    Dev_Motor_Left_SetSpeed((int16_t)pwm_left);
    Dev_Motor_Right_SetSpeed((int16_t)pwm_right);
}

/**
 * @brief Cho xe dừng PID hoàn toàn
 */
void App_Control_Stop(void) {
    App_Control_SetTargetSpeed(0.0f, 0.0f);
    App_PID_Reset(&g_pid_left);
    App_PID_Reset(&g_pid_right);
    Dev_Car_Stop();
}
