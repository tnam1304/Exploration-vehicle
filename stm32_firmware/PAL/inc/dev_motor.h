/**
 * @file    dev_motor.h
 * @brief   Driver điều khiển động cơ TB6612FNG qua PWM TIM3 cho STM32F401RCT6
 */

#ifndef DEV_MOTOR_H
#define DEV_MOTOR_H

#include "stm32f4xx.h"
#include <stdint.h>

#define MOTOR_MAX_SPEED         1000
#define MOTOR_MIN_SPEED        -1000

extern volatile int8_t motor_left_dir;  /* Hướng quay bánh trái: 1 (tiến), -1 (lùi), 0 (dừng) */
extern volatile int8_t motor_right_dir; /* Hướng quay bánh phải: 1 (tiến), -1 (lùi), 0 (dừng) */


/**
 * @brief Thiết lập tốc độ và chiều quay cho động cơ bên trái
 * @param speed Giá trị tốc độ từ -1000 đến 1000
 */
void Motor_Left_SetSpeed(int16_t speed);

/**
 * @brief Thiết lập tốc độ và chiều quay cho động cơ bên phải
 * @param speed Giá trị tốc độ từ -1000 đến 1000
 */
void Motor_Right_SetSpeed(int16_t speed);

#endif /* DEV_MOTOR_H */