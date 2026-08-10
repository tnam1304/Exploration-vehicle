/**
 * @file    app_safety.h
 * @brief   Tầng quản lý an toàn, bộ lọc Kalman 1D và điều khiển còi báo (Buzzer)
 */

#ifndef APP_SAFETY_H
#define APP_SAFETY_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* HẰNG SỐ CẤU HÌNH AN TOÀN (SAFETY CONFIGURATION CONSTANTS)                 */
/* ========================================================================== */
#define BUZZER_PIN                  1           /* PB1 - Chân điều khiển còi báo */

#define SONAR_INIT_DIST_CM          999         /* Khoảng cách khởi tạo mặc định (cm) */

#define SAFETY_WARN_NONE            0           /* Trạng thái an toàn bình thường */
#define SAFETY_WARN_OBSTACLE        1           /* Cảnh báo vật cản nguy hiểm */
#define SAFETY_WARN_FIRE            2           /* Cảnh báo sự cố hỏa hoạn */

/* ========================================================================== */
/* CẤU TRÚC DỮ LIỆU BỘ LỌC KALMAN 1D (KALMAN FILTER STRUCT)                   */
/* ========================================================================== */

/**
 * @brief Cấu trúc dữ liệu chứa tham số và trạng thái của bộ lọc Kalman 1D
 */
typedef struct {
    float q;                /* Nhiễu quá trình (Process Noise Covariance) */
    float r;                /* Nhiễu đo lường (Measurement Noise Covariance) */
    float x;                /* Giá trị ước lượng trạng thái (State Estimate) */
    float p;                /* Hiệu chuẩn sai số ước lượng (Estimation Error Covariance) */
    float k;                /* Hệ số Kalman (Kalman Gain) */
} Kalman1D_t;

/* ========================================================================== */
/* BIẾN TOÀN CỤC CHIA SẺ (SHARED GLOBAL VARIABLES)                           */
/* ========================================================================== */
extern volatile uint8_t horn_active;
extern volatile int warn_code;
extern volatile uint32_t distance_cm;

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Khởi tạo các tham số ban đầu cho bộ lọc Kalman 1D
 * @param kf Con trỏ tới đối tượng Kalman1D
 * @param q Nhiễu quá trình Q
 * @param r Nhiễu đo lường R
 * @param p Độ lệch chuẩn sai số P
 * @param initial_value Giá trị ước lượng ban đầu
 */
void Kalman1D_Init(Kalman1D_t *kf, float q, float r, float p, float initial_value);

/**
 * @brief Cập nhật và tính toán giá trị lọc Kalman 1D từ dữ liệu đo mới
 * @param kf Con trỏ tới đối tượng Kalman1D
 * @param measurement Giá trị đo thô từ cảm biến
 * @retval Giá trị sau khi qua lọc Kalman
 */
float Kalman1D_Update(Kalman1D_t *kf, float measurement);

/**
 * @brief Cập nhật trạng thái bật/tắt còi báo (Buzzer) dựa trên mã cảnh báo và lệnh còi
 */
void Update_Buzzer_State(void);

#endif /* APP_SAFETY_H */