/**
 * @file    app_safety.c
 * @brief   Triển khai lọc Kalman 1D cho cảm biến và logic cảnh báo an toàn còi
 */

#include "app_safety.h"

/* Cờ kích hoạt còi từ app, mã cảnh báo hệ thống và khoảng cách cảm biến siêu âm */
volatile uint8_t horn_active = 0;
volatile int warn_code = SAFETY_WARN_NONE;
volatile uint32_t distance_cm = SONAR_INIT_DIST_CM;

/* ========================================================================== */
/* THUẬT TOÁN BỘ LỌC KALMAN 1D (1D KALMAN FILTER IMPLEMENTATION)              */
/* ========================================================================== */

void Kalman1D_Init(Kalman1D_t *kf, float q, float r, float p, float initial_value) {
    kf->q = q;
    kf->r = r;
    kf->p = p;
    kf->x = initial_value;
    kf->k = 0.0f;
}

float Kalman1D_Update(Kalman1D_t *kf, float measurement) {
    /* 1. Dự báo hiệp phương sai sai số (Predict) */
    kf->p = kf->p + kf->q;

    /* 2. Tính toán hệ số Kalman Gain */
    kf->k = kf->p / (kf->p + kf->r);

    /* 3. Cập nhật ước lượng trạng thái (Update State) */
    kf->x = kf->x + kf->k * (measurement - kf->x);

    /* 4. Cập nhật hiệp phương sai sai số (Update Error Covariance) */
    kf->p = (1.0f - kf->k) * kf->p;

    return kf->x;
}

/* ========================================================================== */
/* CẬP NHẬT TRẠNG THÁI CẢNH BÁO (SAFETY ALARM CONTROL)                        */
/* ========================================================================== */

void Update_Buzzer_State(void) {
    /* Bật còi khi có cảnh báo nguy hiểm (Vật cản / Hỏa hoạn) hoặc cờ còi được bật từ App */
    if (warn_code == SAFETY_WARN_OBSTACLE || warn_code == SAFETY_WARN_FIRE || horn_active) {
        GPIOB->BSRR = (1 << BUZZER_PIN);               /* Bật Buzzer (PB1) */
    } else {
        GPIOB->BSRR = (1 << (BUZZER_PIN + 16));        /* Tắt Buzzer (PB1) */
    }
}