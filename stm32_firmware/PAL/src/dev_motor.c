/**
 * @file dev_motor.c
 * @brief Driver dieu khien dong co va huong di chuyen (Tang PAL)
 */

#include "dev_motor.h"
#include "bsp_pinout.h"

/* Toc do mac dinh cua 2 banh (Gia tri PWM tu 0 den 1000) */
uint16_t speed_fwd_left = 550;
uint16_t speed_fwd_right = 550;
uint16_t speed_bwd_left = 550;
uint16_t speed_bwd_right = 550;

/**
 * @brief Khoi tao ngoai vi: Cap clock GPIO/Timer3, cau hinh chan Output va PWM
 */
void Dev_Motor_Init(void) {
}

/**
 * @brief Dat toc do va chieu quay dong co ben phai
 */
void Dev_Motor_Right_SetSpeed(int16_t speed) {
}

/**
 * @brief Dat toc do va chieu quay dong co ben trai
 */
void Dev_Motor_Left_SetSpeed(int16_t speed) {
}

/**
 * @brief Cho xe di tien
 */
void Dev_Car_Forward_Normal(void) {
    Dev_Motor_Left_SetSpeed(speed_fwd_left);
    Dev_Motor_Right_SetSpeed(speed_fwd_right);
}

/**
 * @brief Cho xe di lui
 */
void Dev_Car_Backward(void) {
    Dev_Motor_Left_SetSpeed(-speed_bwd_left);
    Dev_Motor_Right_SetSpeed(-speed_bwd_right);
}

/**
 * @brief Re trai
 */
void Dev_Car_TurnLeft(void) {
    Dev_Motor_Left_SetSpeed(-500);
    Dev_Motor_Right_SetSpeed(500);
}

/**
 * @brief Re phai
 */
void Dev_Car_TurnRight(void) {
    Dev_Motor_Left_SetSpeed(500);
    Dev_Motor_Right_SetSpeed(-500);
}

/**
 * @brief Dung xe
 */
void Dev_Car_Stop(void) {
    Dev_Motor_Left_SetSpeed(0);
    Dev_Motor_Right_SetSpeed(0);
}
