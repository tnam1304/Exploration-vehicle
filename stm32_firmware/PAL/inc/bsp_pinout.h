#ifndef BSP_PINOUT_H
#define BSP_PINOUT_H

#include "stm32f4xx.h"


// 1. CẤU HÌNH ĐỘNG CƠ & CẦU H TB6612
// PWM PA6 (TIM3_CH1) - Bánh Phải, PA7 (TIM3_CH2) - Bánh Trái
#define MOTOR_PWM_PORT          GPIOA
#define MOTOR_RIGHT_PWM_PIN     6
#define MOTOR_LEFT_PWM_PIN      7

// Chân hướng Bánh Phải: PB10 (IN1), PB8 (IN2)
#define MOTOR_R_IN1_PORT        GPIOB
#define MOTOR_R_IN1_PIN         10
#define MOTOR_R_IN2_PORT        GPIOB
#define MOTOR_R_IN2_PIN         8

// Chân hướng Bánh Trái: PB14 (IN3), PB15 (IN4)
#define MOTOR_L_IN3_PORT        GPIOB
#define MOTOR_L_IN3_PIN         14
#define MOTOR_L_IN4_PORT        GPIOB
#define MOTOR_L_IN4_PIN         15

// Encoder
#define ENCODER_L_PORT          GPIOA
#define ENCODER_L_A_PIN         0       // TIM5_CH1

#define ENCODER_R_PORT          GPIOB
#define ENCODER_R_A_PIN         6       // TIM4_CH1

#endif
