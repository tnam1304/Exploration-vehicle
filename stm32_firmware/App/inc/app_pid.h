/**
 * @file    app_pid.h
 * @brief   Bộ điều khiển PID tốc độ cho động cơ (Tầng App)
 */

#ifndef APP_PID_H
#define APP_PID_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Cấu trúc dữ liệu cho một bộ điều khiển PID
 */
typedef struct {
    float Kp;           /**< Hệ số Tỷ lệ */
    float Ki;           /**< Hệ số Tích phân */
    float Kd;           /**< Hệ số Đạo hàm */

    float setpoint;     /**< Tốc độ mục tiêu (cm/s) */
    float integral;     /**< Tổng tích phân sai số */
    float prev_error;   /**< Sai số lần đọc trước */

    float out_min;      /**< Tốc độ PWM nhỏ nhất (-1000) */
    float out_max;      /**< Tốc độ PWM lớn nhất (1000) */
    float dt;           /**< Chu kỳ lấy mẫu (giây, ví dụ 0.02s = 20ms) */
} PID_Controller_t;

extern PID_Controller_t g_pid_left;
extern PID_Controller_t g_pid_right;

/**
 * @brief Khởi tạo tham số cho bộ PID
 */
void App_PID_Init(PID_Controller_t *pid, float Kp, float Ki, float Kd, float min, float max, float dt);

/**
 * @brief Reset trạng thái tích phân và sai số của PID
 */
void App_PID_Reset(PID_Controller_t *pid);

/**
 * @brief Tính toán giá trị đầu ra PID (Trả về giá trị PWM)
 * @param pid      Pointer tới cấu trúc PID
 * @param setpoint Tốc độ mong muốn (cm/s)
 * @param feedback Tốc độ thực tế đo từ Encoder (cm/s)
 * @return float   Giá trị PWM tương ứng (-1000 đến 1000)
 */
float App_PID_Compute(PID_Controller_t *pid, float setpoint, float feedback);

/**
 * @brief Khởi tạo cả 2 bộ PID cho bánh trái và bánh phải
 */
void App_PID_Motors_Init(void);

#endif /* APP_PID_H */
