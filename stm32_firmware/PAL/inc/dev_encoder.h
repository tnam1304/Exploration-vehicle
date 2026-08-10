#ifndef __DEV_ENCODER_H
#define __DEV_ENCODER_H

#include "stm32f4xx.h"

void Encoder_Init(void);
int32_t Encoder_GetCount_Left(void);
int32_t Encoder_GetCount_Right(void);
void Encoder_Reset(void);

#endif /* __DEV_ENCODER_H */
/**
 * @file    dev_encoder.h
 * @brief   Driver đọc encoder động cơ qua bộ đếm Timer cho STM32F401RCT6
 */

#ifndef DEV_ENCODER_H
#define DEV_ENCODER_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* ĐỊNH NGHĨA CHÂN VÀ THÔNG SỐ ENCODER (PIN MAPPING & HARDWARE CONFIG)       */
/* ========================================================================== */
#define ENCODER_LEFT_PIN                0               /* PA0 - TIM5_CH1 */
#define ENCODER_RIGHT_PIN               6               /* PB6 - TIM4_CH1 */

#define ENCODER_TIM5_ARR_32BIT          0xFFFFFFFFUL   /* Giá trị nạp lại Timer 5 (32-bit) */
#define ENCODER_TIM4_ARR_16BIT          0xFFFFU        /* Giá trị nạp lại Timer 4 (16-bit) */

/* Cấu hình External Clock Mode 1 (TS = 101: TI1FP1, SMS = 111: External Clock) */
#define ENCODER_TIM_SMCR_EXT_CLK_MODE1  ((5U << 4) | (7U << 0))

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Khởi tạo Timer 4 và Timer 5 ở chế độ đếm xung Encoder ngoại vi
 */
void Encoder_Init(void);

/**
 * @brief Lấy tổng số xung đếm được của Encoder bên trái
 * @retval Tổng số xung đếm được (có tính đến chiều quay)
 */
int32_t Encoder_GetCount_Left(void);

/**
 * @brief Lấy tổng số xung đếm được của Encoder bên phải
 * @retval Tổng số xung đếm được (có tính đến chiều quay)
 */
int32_t Encoder_GetCount_Right(void);

/**
 * @brief Đặt lại giá trị bộ đếm xung encoder về 0
 */
void Encoder_Reset(void);

#endif /* DEV_ENCODER_H */
