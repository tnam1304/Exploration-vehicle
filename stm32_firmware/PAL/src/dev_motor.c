/**
 * @file    dev_motor.c
 * @brief   Triển khai điều khiển chiều quay và tốc độ động cơ DC qua TB6612FNG
 */

#include "dev_motor.h"
#include "bsp_pinout.h"

/* Biến lưu hướng quay thực tế của 2 động cơ */
volatile int8_t motor_left_dir = 0;
volatile int8_t motor_right_dir = 0;

void Motor_Right_SetSpeed(int16_t speed) {
    /* Giới hạn dải tốc độ [-1000, 1000] */
    if (speed > MOTOR_MAX_SPEED) {
        speed = MOTOR_MAX_SPEED;
    }
    if (speed < MOTOR_MIN_SPEED) {
        speed = MOTOR_MIN_SPEED;
    }

    if (speed > 0) {
        /* Quay tiến: PA8 High (AIN1), PA9 Low (AIN2) */
        GPIOA->BSRR = (1 << MOTOR_AIN1_PIN) | (1 << (MOTOR_AIN2_PIN + 16));
        TIM3->CCR1 = speed;
        motor_right_dir = 1;
    } else if (speed < 0) {
        /* Quay lùi: PA8 Low (AIN1), PA9 High (AIN2) */
        GPIOA->BSRR = (1 << (MOTOR_AIN1_PIN + 16)) | (1 << MOTOR_AIN2_PIN);
        TIM3->CCR1 = -speed;
        motor_right_dir = -1;
    } else {
        /* Dừng động cơ: PA8 Low, PA9 Low */
        GPIOA->BSRR = (1 << (MOTOR_AIN1_PIN + 16)) | (1 << (MOTOR_AIN2_PIN + 16));
        TIM3->CCR1 = 0;
        motor_right_dir = 0;
    }
}

void Motor_Left_SetSpeed(int16_t speed) {
    /* Giới hạn dải tốc độ [-1000, 1000] */
    if (speed > MOTOR_MAX_SPEED) {
        speed = MOTOR_MAX_SPEED;
    }
    if (speed < MOTOR_MIN_SPEED) {
        speed = MOTOR_MIN_SPEED;
    }

    if (speed > 0) {
        /* Quay tiến: PB0 High (BIN2), PA10 Low (BIN1) */
        GPIOB->BSRR = (1 << MOTOR_BIN2_PIN);
        GPIOA->BSRR = (1 << (MOTOR_BIN1_PIN + 16));
        TIM3->CCR2 = speed;
        motor_left_dir = 1;
    } else if (speed < 0) {
        /* Quay lùi: PB0 Low (BIN2), PA10 High (BIN1) */
        GPIOB->BSRR = (1 << (MOTOR_BIN2_PIN + 16));
        GPIOA->BSRR = (1 << MOTOR_BIN1_PIN);
        TIM3->CCR2 = -speed;
        motor_left_dir = -1;
    } else {
        /* Dừng động cơ: PB0 Low, PA10 Low */
        GPIOB->BSRR = (1 << (MOTOR_BIN2_PIN + 16));
        GPIOA->BSRR = (1 << (MOTOR_BIN1_PIN + 16));
        TIM3->CCR2 = 0;
        motor_left_dir = 0;
    }
}