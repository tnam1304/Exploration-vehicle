/**
 * @file dev_motor.h
 * @brief Module điều khiển động cơ cho xe (Tang PAL)
 */

#ifndef DEV_MOTOR_H
#define DEV_MOTOR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Toc do mac dinh cua 2 banh xe (danh gia PWM 0 - 1000) */
extern uint16_t speed_fwd_left;
extern uint16_t speed_fwd_right;
extern uint16_t speed_bwd_left;
extern uint16_t speed_bwd_right;

/**
 * @brief Khoi tao các chan GPIO va Timer PWM cho dong co
 */
void Dev_Motor_Init(void);

/**
 * @brief Dat toc do va chieu quay cho banh phai
 * @param speed Gia tri PWM tu -1000 den 1000 (So am la quay lui, 0 la dung)
 */
void Dev_Motor_Right_SetSpeed(int16_t speed);

/**
 * @brief Dat toc do va chieu quay cho banh trai
 * @param speed Gia tri PWM tu -1000 den 1000 (So am la quay lui, 0 la dung)
 */
void Dev_Motor_Left_SetSpeed(int16_t speed);

/**
 * @brief Cho xe chay tien
 */
void Dev_Car_Forward_Normal(void);

/**
 * @brief Cho xe chay lui
 */
void Dev_Car_Backward(void);

/**
 * @brief Re trai
 */
void Dev_Car_TurnLeft(void);

/**
 * @brief Re phai
 */
void Dev_Car_TurnRight(void);

/**
 * @brief Dung xe hoan toan
 */
void Dev_Car_Stop(void);

#endif /* DEV_MOTOR_H */
