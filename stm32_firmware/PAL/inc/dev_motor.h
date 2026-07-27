/**
 * @file dev_motor.h
 * @brief Module điều khiển động cơ cho xe (Tầng PAL)
 */

#ifndef DEV_MOTOR_H
#define DEV_MOTOR_H

#include "stm32f4xx.h"
#include <stdint.h>
#include "dev_encoder.h"

/* Tốc độ mặc định của 2 bánh xe (đánh giá PWM 0 - 1000) */
extern uint16_t speed_fwd_left;
extern uint16_t speed_fwd_right;
extern uint16_t speed_bwd_left;
extern uint16_t speed_bwd_right;

/**
 * @brief Khởi tạo các chân GPIO và Timer PWM cho động cơ
 */
void Dev_Motor_Init(void);

/**
 * @brief Đặt tốc độ và chiều quay cho bánh phải (Tự động cập nhật chiều đếm Encoder)
 * @param speed Giá trị PWM từ -1000 đến 1000 (Số âm là quay lùi, 0 là dừng)
 */
void Dev_Motor_Right_SetSpeed(int16_t speed);

/**
 * @brief Đặt tốc độ và chiều quay cho bánh trái (Tự động cập nhật chiều đếm Encoder)
 * @param speed Giá trị PWM từ -1000 đến 1000 (Số âm là quay lùi, 0 là dừng)
 */
void Dev_Motor_Left_SetSpeed(int16_t speed);

/**
 * @brief Cho xe chạy tiến
 */
void Dev_Car_Forward_Normal(void);

/**
 * @brief Cho xe chạy lùi
 */
void Dev_Car_Backward(void);

/**
 * @brief Rẽ trái tại chỗ
 */
void Dev_Car_TurnLeft(void);

/**
 * @brief Rẽ phải tại chỗ
 */
void Dev_Car_TurnRight(void);

/**
 * @brief Dừng xe hoàn toàn
 */
void Dev_Car_Stop(void);

#endif /* DEV_MOTOR_H */
