/**
 * @file dev_encoder.c
 * @brief Driver dem xung va huong di chuyen cho Encoder Quang (Tang PAL)
 * @author Tran Minh Phuc
 */

#include "dev_encoder.h"
#include "bsp_pinout.h"

Encoder_Handle_t g_encoder_left = {
    .timer = TIM5,
    .last_counter = 0,
    .total_pulses = 0,
    .direction = 1
};

Encoder_Handle_t g_encoder_right = {
    .timer = TIM4,
    .last_counter = 0,
    .total_pulses = 0,
    .direction = 1
};

/**
 * @brief Khoi tao TIM5_CH1 (PA0) va TIM4_CH1 (PB6) o che do External Clock Mode 1
 */
void Dev_Encoder_Init(void) {
    /* 1. Cap Clock cho GPIOA, GPIOB, TIM4 va TIM5 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM5EN;

    /* 2. Cau hinh PA0 (TIM5_CH1) - Banh Trai */
    GPIOA->MODER &= ~(3 << (0 * 2));
    GPIOA->MODER |=  (2 << (0 * 2)); // Alternate Function
    GPIOA->PUPDR |=  (1 << (0 * 2)); // Pull-up noi
    GPIOA->AFR[0] |= (2 << (0 * 4)); // AF2 (TIM5)

    /* 3. Cau hinh PB6 (TIM4_CH1) - Banh Phai */
    GPIOB->MODER &= ~(3 << (6 * 2));
    GPIOB->MODER |=  (2 << (6 * 2)); // Alternate Function
    GPIOB->PUPDR |=  (1 << (6 * 2)); // Pull-up noi
    GPIOB->AFR[0] |= (2 << (6 * 4)); // AF2 (TIM4)

    /* 4. Cau hinh TIM5 dem xung ngoai tu CH1 (PA0) */
    TIM5->PSC = 0;
    TIM5->ARR = 0xFFFFFFFF; // TIM5 la Timer 32 bit
    TIM5->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM5->CCMR1 |= (1 << TIM_CCMR1_CC1S_Pos);
    TIM5->SMCR &= ~TIM_SMCR_SMS;
    TIM5->SMCR |= (7 << TIM_SMCR_SMS_Pos);
    TIM5->CNT = 0;
    TIM5->CR1 |= TIM_CR1_CEN;

    /* 5. Cau hinh TIM4 dem xung ngoai tu CH1 (PB6) */
    TIM4->PSC = 0;
    TIM4->ARR = 0xFFFF;     // TIM4 la Timer 16 bit
    TIM4->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM4->CCMR1 |= (1 << TIM_CCMR1_CC1S_Pos);
    TIM4->SMCR &= ~TIM_SMCR_SMS;
    TIM4->SMCR |= (7 << TIM_SMCR_SMS_Pos);
    TIM4->CNT = 0;
    TIM4->CR1 |= TIM_CR1_CEN;

    Dev_Encoder_Reset();
}

/**
 * @brief Cap nhat chieu quay tu module motor
 */
void Dev_Encoder_SetDirection(int8_t dir_left, int8_t dir_right) {
    g_encoder_left.direction = dir_left;
    g_encoder_right.direction = dir_right;
}

/**
 * @brief Cap nhat so xung dem duoc
 */
void Dev_Encoder_Update(void) {
    /* --- Doc va cap nhat Banh Trai (TIM5) --- */
    uint32_t cnt_left = g_encoder_left.timer->CNT;
    uint32_t diff_left = cnt_left - g_encoder_left.last_counter;
    g_encoder_left.last_counter = cnt_left;

    // Cong hoac tru xung tuy theo huong xe đang chay
    g_encoder_left.total_pulses += (int32_t)diff_left * g_encoder_left.direction;

    /* --- Doc va cap nhat Banh Phai (TIM4) --- */
    uint32_t cnt_right = g_encoder_right.timer->CNT;
    // Xu ly cho TIM4 16-bit
    uint16_t diff_right = (uint16_t)(cnt_right - g_encoder_right.last_counter);
    g_encoder_right.last_counter = cnt_right;

    // Cong hoac tru xung tuy theo huong xe đang chay
    g_encoder_right.total_pulses += (int32_t)diff_right * g_encoder_right.direction;
}

/**
 * @brief Reset xung ve 0
 */
void Dev_Encoder_Reset(void) {
    TIM5->CNT = 0;
    TIM4->CNT = 0;

    g_encoder_left.last_counter = 0;
    g_encoder_left.total_pulses = 0;

    g_encoder_right.last_counter = 0;
    g_encoder_right.total_pulses = 0;
}

/* API lay tong so xung */
int32_t Dev_Encoder_Left_GetPulses(void) {
    return g_encoder_left.total_pulses;
}

int32_t Dev_Encoder_Right_GetPulses(void) {
    return g_encoder_right.total_pulses;
}
