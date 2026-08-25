/**
 * @file    dev_encoder.c
 * @brief   Triển khai đọc xung Encoder động cơ sử dụng Timer4 và Timer5
 */

#include "dev_encoder.h"
#include "dev_motor.h" /* Chứa biến motor_left_dir và motor_right_dir */

/* Biến lưu trữ tổng số xung đã đếm */
static volatile int32_t pulse_count_left = 0;
static volatile int32_t pulse_count_right = 0;

/* Biến lưu trữ giá trị Timer ở lần đọc trước */
static uint32_t last_cnt_left = 0;
static uint16_t last_cnt_right = 0;

void Encoder_Init(void) {
    /* 1. Cấp clock cho GPIOA, GPIOB, TIM4 và TIM5 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM5EN;

    /* 2. Cấu hình PA0 (Encoder Trái) -> Alternate Function (AF2 - TIM5_CH1) */
    GPIOA->MODER &= ~(3U << (ENCODER_LEFT_PIN * 2));
    GPIOA->MODER |=  (2U << (ENCODER_LEFT_PIN * 2)); /* Chế độ AF */
    GPIOA->PUPDR &= ~(3U << (ENCODER_LEFT_PIN * 2));
    GPIOA->PUPDR |=  (1U << (ENCODER_LEFT_PIN * 2)); /* Pull-up để lọc nhiễu */
    GPIOA->AFR[0] &= ~(0xFU << (ENCODER_LEFT_PIN * 4));
    GPIOA->AFR[0] |=  (2U << (ENCODER_LEFT_PIN * 4)); /* Chọn AF2 cho PA0 */

    /* 3. Cấu hình PB6 (Encoder Phải) -> Alternate Function (AF2 - TIM4_CH1) */
    GPIOB->MODER &= ~(3U << (ENCODER_RIGHT_PIN * 2));
    GPIOB->MODER |=  (2U << (ENCODER_RIGHT_PIN * 2)); /* Chế độ AF */
    GPIOB->PUPDR &= ~(3U << (ENCODER_RIGHT_PIN * 2));
    GPIOB->PUPDR |=  (1U << (ENCODER_RIGHT_PIN * 2)); /* Pull-up để lọc nhiễu */
    GPIOB->AFR[0] &= ~(0xFU << (ENCODER_RIGHT_PIN * 4));
    GPIOB->AFR[0] |=  (2U << (ENCODER_RIGHT_PIN * 4)); /* Chọn AF2 cho PB6 */

    /* 4. Cấu hình TIM5 đếm xung PA0 (Left) ở chế độ External Clock Mode 1 */
    TIM5->CR1 = 0;
    TIM5->PSC = 0;                                 /* Không chia tần, đếm mọi xung */
    TIM5->ARR = ENCODER_TIM5_ARR_32BIT;            /* TIM5 là Timer 32-bit */
    /* CC1S = 01, IC1F = 1111: lọc xung nhiễu ngắn trước khi đếm. */
    TIM5->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_IC1F;
    /* TS = 101 (TI1FP1 làm trigger), SMS = 111 (External Clock Mode 1) */
    TIM5->SMCR = ENCODER_TIM_SMCR_EXT_CLK_MODE1;
    TIM5->CR1 |= TIM_CR1_CEN;                      /* Bật Timer 5 */

    /* 5. Cấu hình TIM4 đếm xung PB6 (Right) ở chế độ External Clock Mode 1 */
    TIM4->CR1 = 0;
    TIM4->PSC = 0;
    TIM4->ARR = ENCODER_TIM4_ARR_16BIT;            /* TIM4 là Timer 16-bit */
    /* Dùng cùng mức lọc cho encoder phải để hai bánh có đặc tính giống nhau. */
    TIM4->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_IC1F;
    TIM4->SMCR = ENCODER_TIM_SMCR_EXT_CLK_MODE1;
    TIM4->CR1 |= TIM_CR1_CEN;                      /* Bật Timer 4 */

    /* Khởi tạo giá trị bắt đầu */
    last_cnt_left = TIM5->CNT;
    last_cnt_right = TIM4->CNT;
}

int32_t Encoder_GetCount_Left(void) {
    /* Đọc giá trị Timer hiện tại */
    uint32_t current_cnt = TIM5->CNT;
    /* Tính số xung mới sinh ra kể từ lần đọc trước (Xử lý luôn cả hiện tượng tràn Timer) */
    uint32_t delta = current_cnt - last_cnt_left;
    last_cnt_left = current_cnt;

    /* Cộng hoặc trừ số xung tùy theo chiều quay của động cơ */
    if (motor_left_dir >= 0) {
        pulse_count_left += delta;
    } else {
        pulse_count_left -= delta;
    }

    return pulse_count_left;
}

int32_t Encoder_GetCount_Right(void) {
    /* Đọc giá trị Timer hiện tại (TIM4 16-bit) */
    uint16_t current_cnt = TIM4->CNT;
    uint16_t delta = current_cnt - last_cnt_right;
    last_cnt_right = current_cnt;

    if (motor_right_dir >= 0) {
        pulse_count_right += delta;
    } else {
        pulse_count_right -= delta;
    }

    return pulse_count_right;
}

void Encoder_Reset(void) {
    pulse_count_left = 0;
    pulse_count_right = 0;
    last_cnt_left = TIM5->CNT;
    last_cnt_right = TIM4->CNT;
}
