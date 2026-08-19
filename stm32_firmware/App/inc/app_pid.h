/**
 * @file    app_pid.h
 * @brief   Bộ điều khiển PID kín (Closed-Loop PID) cho hai bánh xe
 */

#ifndef APP_PID_H
#define APP_PID_H

#include "stm32f4xx.h"
#include <stdint.h>


/* CẤU TRÚC DỮ LIỆU PID (PID CONTROLLER STRUCT)                               */
/**
 * @brief Cấu trúc dữ liệu chứa tham số và trạng thái bộ điều khiển PID
 */
typedef struct {
    float kp;               /* Hệ số tỉ lệ (Proportional) */
    float ki;               /* Hệ số tích phân (Integral) */
    float kd;               /* Hệ số vi phân (Derivative) */
    float target;           /* Giá trị đặt mong muốn (SetPoint) */
    float integral;         /* Bội số tích phân tích lũy */
    float prev_error;       /* Sai số của chu kỳ trước */
    float max_output;       /* Giá trị đầu ra tối đa (Bão hòa trên) */
    float min_output;       /* Giá trị đầu ra tối thiểu (Bão hòa dưới) */
} PID_Controller_t;

/* BIẾN TOÀN CỤC CHIA SẺ (SHARED GLOBAL VARIABLES)                           */
extern PID_Controller_t pid_left;
extern PID_Controller_t pid_right;

/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */

/**
 * @brief Khởi tạo các tham số ban đầu cho bộ điều khiển PID
 * @param pid Con trỏ tới đối tượng PID
 * @param kp Hệ số P
 * @param ki Hệ số I
 * @param kd Hệ số D
 * @param min_out Giới hạn đầu ra tối thiểu
 * @param max_out Giới hạn đầu ra tối đa
 */
void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd, float min_out, float max_out);

/**
 * @brief Tính toán tín hiệu điều khiển PID
 * @param pid Con trỏ tới đối tượng PID
 * @param measured_value Giá trị thực tế đo được từ cảm biến/encoder
 * @param dt_s Khoảng thời gian lấy mẫu (tính bằng giây)
 * @retval Tín hiệu điều khiển đầu ra (PWM)
 */
float PID_Compute(PID_Controller_t *pid, float measured_value, float dt_s);

/**
 * @brief Đặt lại (Reset) các giá trị tích lũy của bộ PID
 * @param pid Con trỏ tới đối tượng PID
 */
void PID_Reset(PID_Controller_t *pid);

#endif /* APP_PID_H */
