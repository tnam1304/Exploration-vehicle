/**
 * @file    odometry.h
 * @brief   Tầng tính toán định vị động học (Odometry), quãng đường và vận tốc bánh xe
 */

#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "stm32f4xx.h"
#include <stdint.h>

/* HẰNG SỐ CẤU HÌNH BÁNH XE & ENCODER (ODOMETRY HARDWARE CONFIGURATION)     */
#define WHEEL_DIAMETER_CM           6.5f        /* Đường kính bánh xe TT Motor (cm) */
#define ENCODER_PPR                 20.0f       /* Số xung trên 1 vòng quay của đĩa Encoder 20 khe */
#define ODOMETRY_PI                 3.14159265f /* Hằng số PI */

/* Hệ số bộ lọc trung bình động lũy thừa (EMA Filter) */
#define EMA_ALPHA_OLD               0.7f        /* Trọng số giữ lại của giá trị cũ */
#define EMA_ALPHA_NEW               0.3f        /* Trọng số cập nhật của giá trị mới */

/* CẤU TRÚC DỮ LIỆU ODOMETRY (ODOMETRY DATA STRUCT)                          */
/**
 * @brief Cấu trúc dữ liệu chứa vận tốc và quãng đường di chuyển của xe
 */
typedef struct {
    float speed_left_cms;       /* Vận tốc bánh trái (cm/s) */
    float speed_right_cms;      /* Vận tốc bánh phải (cm/s) */
    float distance_left_cm;     /* Quãng đường tích lũy bánh trái (cm) */
    float distance_right_cm;    /* Quãng đường tích lũy bánh phải (cm) */
    float total_distance_cm;    /* Quãng đường trung bình xe đã đi (cm) */
} Odometry_t;

/* BIẾN TOÀN CỤC CHIA SẺ (SHARED GLOBAL VARIABLES)                           */
extern Odometry_t g_odometry;

/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/**
 * @brief Khởi tạo và đặt lại toàn bộ thông số định vị Odometry
 */
void Odometry_Init(void);

/**
 * @brief Cập nhật quãng đường và vận tốc bánh xe định kỳ theo chu kỳ dt_s
 * @param dt_s Khoảng thời gian giữa 2 lần lấy mẫu (tính bằng giây)
 */
void Odometry_Update(float dt_s);

#endif /* ODOMETRY_H */
