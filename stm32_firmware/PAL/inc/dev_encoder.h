/**
 * @file dev_encoder.h
 * @brief Driver đếm xung, tính vận tốc và quãng đường cho Encoder (Tầng PAL/Driver)
 * @author Tran Minh Phuc
 */

#ifndef DEV_ENCODER_H
#define DEV_ENCODER_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

/* CẤU HÌNH THÔNG SỐ CƠ KHÍ XE*/

/** @brief Số xung đếm được trên 1 vòng quay của trục bánh xe */
#define ENCODER_PPR                 20.0f

/** @brief Đường kính bánh xe (cm) */
#define WHEEL_DIAMETER_CM           6.5f

/** @brief Chu vi bánh xe (cm) = PI * Diameter */
#define WHEEL_CIRCUMFERENCE_CM      (3.14159265f * WHEEL_DIAMETER_CM)

/** @brief Chu kỳ lấy mẫu tính vận tốc (Giây) - Ví dụ 20ms = 0.02s */
#define ENCODER_SAMPLE_TIME_S       0.02f

/* KIỂU DỮ LIỆU CẤU TRÚC ENCODER*/
typedef struct {
    TIM_TypeDef *timer;         /**< Pointer tới Timer (TIM5 hoặc TIM4) */
    uint32_t last_counter;      /**< Giá trị CNT lần đọc trước */
    int32_t total_pulses;       /**< Tổng số xung tích lũy (có dấu: + Tiến, - Lùi) */
    int8_t direction;           /**< Hướng quay: +1 (Tiến), -1 (Lùi), 0 (Dừng) */

    /* Các trường dữ liệu nâng cấp mới */
    float speed_cms;            /**< Vận tốc hiện tại (cm/s) */
    float distance_cm;          /**< Quãng đường tích lũy đã đi được (cm) */
} Encoder_Handle_t;

extern Encoder_Handle_t g_encoder_left;
extern Encoder_Handle_t g_encoder_right;

/* API KHỞI TẠO VÀ CẬP NHẬT */

void Dev_Encoder_Init(void);
void Dev_Encoder_SetDirection(int8_t dir_left, int8_t dir_right);

/**
 * @brief Đọc xung, tính vận tốc (cm/s) và quãng đường (cm).
 */
void Dev_Encoder_Update(void);
void Dev_Encoder_Reset(void);

/* API LẤY THÔNG SỐ (XUNG, VẬN TỐC, QUÃNG ĐƯỜNG)                              */

int32_t Dev_Encoder_Left_GetPulses(void);
int32_t Dev_Encoder_Right_GetPulses(void);

float Dev_Encoder_Left_GetSpeed(void);      /**< Trả về vận tốc Bánh Trái (cm/s) */
float Dev_Encoder_Right_GetSpeed(void);     /**< Trả về vận tốc Bánh Phải (cm/s) */

float Dev_Encoder_Left_GetDistance(void);   /**< Trả về quãng đường Bánh Trái (cm) */
float Dev_Encoder_Right_GetDistance(void);  /**< Trả về quãng đường Bánh Phải (cm) */

float Dev_Encoder_GetAverageDistance(void); /**< Trả về quãng đường trung bình cả xe (cm) */

#endif /* DEV_ENCODER_H */
