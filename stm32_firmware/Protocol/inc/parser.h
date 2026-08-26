/**
 * @file    parser.h
 * @brief   Xử lý và phân tích lệnh truyền thông UART từ ứng dụng điều khiển
 */

#ifndef PARSER_H
#define PARSER_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* HẰNG SỐ CẤU HÌNH PARSER (PARSER CONFIGURATION CONSTANTS)                    */
/* ========================================================================== */
#define PARSER_RX_BUF_SIZE          24          /* Đủ chứa frame PID G:Kp,Ki,Kd */

/* Giới hạn hệ số PID nhận từ Web, biểu diễn theo hệ số nhân 100 */
#define PID_GAIN_SCALE              100.0f
#define PID_KP_MAX_SCALED           2000U       /* Kp tối đa 20.00 */
#define PID_KI_MAX_SCALED           1000U       /* Ki tối đa 10.00 */
#define PID_KD_MAX_SCALED           500U        /* Kd tối đa 5.00 */

/* Giới hạn cài đặt tốc độ động cơ */
#define SPEED_MIN_PERCENT           20          /* Tốc độ tối thiểu tính theo % */
#define SPEED_MAX_PERCENT           100         /* Tốc độ tối đa tính theo % */
#define SPEED_PERCENT_SCALE         10          /* Hệ số quy đổi từ % sang PWM (20-100 -> 200-1000) */

#define SPEED_MIN_PWM               200         /* Tốc độ PWM tối thiểu */
#define SPEED_MAX_PWM               1000        /* Tốc độ PWM tối đa */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Giải mã và xử lý ký tự lệnh nhận được từ UART
 * @param rx_cmd Ký tự lệnh nhận từ UART
 */
void Parser_ProcessCmd(char rx_cmd);

#endif /* PARSER_H */
