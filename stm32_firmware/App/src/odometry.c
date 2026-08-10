/**
 * @file    odometry.c
 * @brief   Triển khai tính toán động học bánh xe và bộ lọc vận tốc EMA
 */

#include "odometry.h"
#include "dev_encoder.h"
#include <math.h>

/* Biến toàn cục lưu trữ dữ liệu định vị Odometry */
Odometry_t g_odometry = {0};

/* Biến tĩnh lưu số xung Encoder thu được ở chu kỳ trước */
static int32_t last_pulse_left = 0;
static int32_t last_pulse_right = 0;

void Odometry_Init(void) {
    Encoder_Reset();
    last_pulse_left = 0;
    last_pulse_right = 0;
    
    g_odometry.speed_left_cms = 0.0f;
    g_odometry.speed_right_cms = 0.0f;
    g_odometry.distance_left_cm = 0.0f;
    g_odometry.distance_right_cm = 0.0f;
    g_odometry.total_distance_cm = 0.0f;
}

void Odometry_Update(float dt_s) {
    if (dt_s <= 0.0f) {
        return;
    }

    /* 1. Lấy số xung hiện tại từ Encoder bánh trái và bánh phải */
    int32_t current_left = Encoder_GetCount_Left();
    int32_t current_right = Encoder_GetCount_Right();

    /* 2. Tính chênh lệch xung so với chu kỳ trước */
    int32_t delta_left = current_left - last_pulse_left;
    int32_t delta_right = current_right - last_pulse_right;

    last_pulse_left = current_left;
    last_pulse_right = current_right;

    /* 3. Tính chu vi bánh xe = PI * D (cm) */
    float wheel_circumference = ODOMETRY_PI * WHEEL_DIAMETER_CM;

    /* 4. Tính quãng đường biến thiên của từng bánh trong chu kỳ dt_s */
    float d_left = ((float)delta_left / ENCODER_PPR) * wheel_circumference;
    float d_right = ((float)delta_right / ENCODER_PPR) * wheel_circumference;

    /* 5. Cập nhật tổng quãng đường tích lũy */
    g_odometry.distance_left_cm += d_left;
    g_odometry.distance_right_cm += d_right;
    g_odometry.total_distance_cm = (g_odometry.distance_left_cm + g_odometry.distance_right_cm) / 2.0f;

    /* 6. Tính vận tốc thô chưa qua lọc (cm/s) */
    float raw_speed_left = d_left / dt_s;
    float raw_speed_right = d_right / dt_s;

    /* 7. Áp dụng bộ lọc EMA (Exponential Moving Average) làm mượt vận tốc */
    g_odometry.speed_left_cms = (EMA_ALPHA_OLD * g_odometry.speed_left_cms) + (EMA_ALPHA_NEW * raw_speed_left);
    g_odometry.speed_right_cms = (EMA_ALPHA_OLD * g_odometry.speed_right_cms) + (EMA_ALPHA_NEW * raw_speed_right);
}