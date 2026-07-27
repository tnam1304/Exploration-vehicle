/**
 * @file dev_encoder.h
 * @brief Driver dem xung va huong di chuyen cho Encoder Quang (Tang PAL)
 * @author Tran Minh Phuc
 */

#ifndef DEV_ENCODER_H
#define DEV_ENCODER_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    TIM_TypeDef *timer;         /**< Pointer toi Timer (TIM5 hoac TIM4) */
    uint32_t last_counter;      /**< Gia tri CNT lan doc truoc */
    int32_t total_pulses;       /**< Tong so xung tich luy (co dau: + Tien, - Lui) */
    int8_t direction;           /**< Huong quay: +1 (Tien), -1 (Lui), 0 (Dung) */
} Encoder_Handle_t;

extern Encoder_Handle_t g_encoder_left;
extern Encoder_Handle_t g_encoder_right;

/**
 * @brief Khoi tao Timer dem xung ngoai (TIM5_CH1 cho Left, TIM4_CH1 cho Right)
 */
void Dev_Encoder_Init(void);

/**
 * @brief Cap nhat chieu quay hien tai tu module Motor sang Encoder
 * @param dir_left  1 (Tien), -1 (Lui), 0 (Dung)
 * @param dir_right 1 (Tien), -1 (Lui), 0 (Dung)
 */
void Dev_Encoder_SetDirection(int8_t dir_left, int8_t dir_right);

/**
 * @brief Doc va cap nhat so xung gia tang cua 2 encoder (Goi dinh ky hoac trong loop)
 */
void Dev_Encoder_Update(void);

/**
 * @brief Reset bien dem xung cua 2 encoder ve 0
 */
void Dev_Encoder_Reset(void);

/**
 * @brief Lay tong so xung hien tai cua banh trai
 */
int32_t Dev_Encoder_Left_GetPulses(void);

/**
 * @brief Lay tong so xung hien tai cua banh phai
 */
int32_t Dev_Encoder_Right_GetPulses(void);

#endif /* DEV_ENCODER_H */
