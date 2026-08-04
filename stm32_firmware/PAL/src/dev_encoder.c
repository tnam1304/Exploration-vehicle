/**
 * @file dev_encoder.c
 * @brief Driver đếm xung, tính vận tốc và quãng đường cho Encoder (Tầng PAL/Driver)
 */

#include "dev_encoder.h"
#include "bsp_pinout.h"

Encoder_Handle_t g_encoder_left = {
    .timer = TIM5,
    .last_counter = 0,
    .total_pulses = 0,
    .direction = 1,
    .speed_cms = 0.0f,
    .distance_cm = 0.0f
};

Encoder_Handle_t g_encoder_right = {
    .timer = TIM4,
    .last_counter = 0,
    .total_pulses = 0,
    .direction = 1,
    .speed_cms = 0.0f,
    .distance_cm = 0.0f
};

void Dev_Encoder_Init(void) {
    /* 1. Cấp Clock cho GPIOA, GPIOB, TIM4 và TIM5 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM5EN;

    /* 2. Cấu hình PA0 (TIM5_CH1) - Bánh Trái */
    GPIOA->MODER &= ~(3U << (0 * 2));
    GPIOA->MODER |=  (2U << (0 * 2)); // Alternate Function
    GPIOA->PUPDR |=  (1U << (0 * 2)); // Pull-up nội
    GPIOA->AFR[0] |= (2U << (0 * 4)); // AF2 (TIM5)

    /* 3. Cấu hình PB6 (TIM4_CH1) - Bánh Phải */
    GPIOB->MODER &= ~(3U << (6 * 2));
    GPIOB->MODER |=  (2U << (6 * 2)); // Alternate Function
    GPIOB->PUPDR |=  (1U << (6 * 2)); // Pull-up nội
    GPIOB->AFR[0] |= (2U << (6 * 4)); // AF2 (TIM4)

    /* 4. Cấu hình TIM5 đếm xung ngoài từ CH1 (PA0) */
    TIM5->PSC = 0;
    TIM5->ARR = 0xFFFFFFFF; // TIM5 là Timer 32 bit
    TIM5->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM5->CCMR1 |= (1U << TIM_CCMR1_CC1S_Pos);
    TIM5->SMCR &= ~TIM_SMCR_SMS;
    TIM5->SMCR |= (7U << TIM_SMCR_SMS_Pos);
    TIM5->CNT = 0;
    TIM5->CR1 |= TIM_CR1_CEN;

    /* 5. Cấu hình TIM4 đếm xung ngoài từ CH1 (PB6) */
    TIM4->PSC = 0;
    TIM4->ARR = 0xFFFF;     // TIM4 là Timer 16 bit
    TIM4->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM4->CCMR1 |= (1U << TIM_CCMR1_CC1S_Pos);
    TIM4->SMCR &= ~TIM_SMCR_SMS;
    TIM4->SMCR |= (7U << TIM_SMCR_SMS_Pos);
    TIM4->CNT = 0;
    TIM4->CR1 |= TIM_CR1_CEN;

    Dev_Encoder_Reset();
}

void Dev_Encoder_SetDirection(int8_t dir_left, int8_t dir_right) {
    g_encoder_left.direction = dir_left;
    g_encoder_right.direction = dir_right;
}

void Dev_Encoder_Update(void) {
    /* 1. Xử lý Bánh Trái (TIM5 - 32-bit) */
    uint32_t cnt_left = g_encoder_left.timer->CNT;
    uint32_t diff_left = cnt_left - g_encoder_left.last_counter;
    g_encoder_left.last_counter = cnt_left;

    // Số xung gia tăng thực tế (có tính hướng)
    int32_t pulse_delta_left = (int32_t)diff_left * g_encoder_left.direction;
    g_encoder_left.total_pulses += pulse_delta_left;

    // Tính vận tốc (cm/s) = (Số xung vừa đếm / PPR) * Chu vi bánh xe / Delta_T
    g_encoder_left.speed_cms = ((float)pulse_delta_left / ENCODER_PPR) *
                                WHEEL_CIRCUMFERENCE_CM / ENCODER_SAMPLE_TIME_S;

    // Tính tổng quãng đường (cm) = (Tổng xung tích lũy / PPR) * Chu vi bánh xe
    g_encoder_left.distance_cm = ((float)g_encoder_left.total_pulses / ENCODER_PPR) *
                                  WHEEL_CIRCUMFERENCE_CM;

    /* 2. Xử lý Bánh Phải (TIM4 - 16-bit) */
    uint32_t cnt_right = g_encoder_right.timer->CNT;
    uint16_t diff_right = (uint16_t)(cnt_right - g_encoder_right.last_counter);
    g_encoder_right.last_counter = cnt_right;

    // Số xung gia tăng thực tế (có tính hướng)
    int32_t pulse_delta_right = (int32_t)diff_right * g_encoder_right.direction;
    g_encoder_right.total_pulses += pulse_delta_right;

    // Tính vận tốc (cm/s)
    g_encoder_right.speed_cms = ((float)pulse_delta_right / ENCODER_PPR) *
                                 WHEEL_CIRCUMFERENCE_CM / ENCODER_SAMPLE_TIME_S;

    // Tính tổng quãng đường (cm)
    g_encoder_right.distance_cm = ((float)g_encoder_right.total_pulses / ENCODER_PPR) *
                                   WHEEL_CIRCUMFERENCE_CM;
}

void Dev_Encoder_Reset(void) {
    TIM5->CNT = 0;
    TIM4->CNT = 0;

    g_encoder_left.last_counter = 0;
    g_encoder_left.total_pulses = 0;
    g_encoder_left.speed_cms = 0.0f;
    g_encoder_left.distance_cm = 0.0f;

    g_encoder_right.last_counter = 0;
    g_encoder_right.total_pulses = 0;
    g_encoder_right.speed_cms = 0.0f;
    g_encoder_right.distance_cm = 0.0f;
}

/* Các API lấy giá trị */
int32_t Dev_Encoder_Left_GetPulses(void) {
    return g_encoder_left.total_pulses;
}

int32_t Dev_Encoder_Right_GetPulses(void) {
    return g_encoder_right.total_pulses;
}

float Dev_Encoder_Left_GetSpeed(void) {
    return g_encoder_left.speed_cms;
}

float Dev_Encoder_Right_GetSpeed(void) {
    return g_encoder_right.speed_cms;
}

float Dev_Encoder_Left_GetDistance(void) {
    return g_encoder_left.distance_cm;
}

float Dev_Encoder_Right_GetDistance(void) {
    return g_encoder_right.distance_cm;
}

float Dev_Encoder_GetAverageDistance(void) {
    return (g_encoder_left.distance_cm + g_encoder_right.distance_cm) / 2.0f;
}
