/**
 * @file    protocol_uart.h
 * @brief   Giao thức truyền thông UART (USART2) và gửi dữ liệu xa (Telemetry)
 */

#ifndef PROTOCOL_UART_H
#define PROTOCOL_UART_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* HẰNG SỐ CẤU HÌNH TRUYỀN THÔNG (UART CONFIGURATION CONSTANTS)               */
/* ========================================================================== */
#define UART_TX_BUF_SIZE            192         /* Đủ chứa thêm quãng đường hai encoder */

#define TEMP_SENSOR_ERROR_RAW      -9999        /* Mã lỗi nhiệt độ thô */
#define TEMP_SENSOR_ERROR_DISP     -99          /* Hiển thị lỗi nhiệt độ (°C) */
#define TEMP_SCALE_FACTOR           16          /* Hệ số chia quy đổi nhiệt độ thô sang độ C */

#define SONAR_ERROR_RAW_DIST        999         /* Mã lỗi khoảng cách siêu âm thô (cm) */
#define SONAR_ERROR_DISP_DIST       0           /* Hiển thị lỗi khoảng cách (cm) */

#define TELEMETRY_FLOAT_SCALE       10.0f       /* Hệ số nhân float -> int (tránh dùng %f trong sprintf) */
#define TELEMETRY_PID_SCALE         100.0f      /* Hệ số nhân Kp/Ki/Kd -> int */

/* ========================================================================== */
/* BIẾN TOÀN CỤC CHIA SẺ (SHARED GLOBAL VARIABLES)                           */
/* ========================================================================== */
extern volatile uint8_t wifi_status_online;
extern volatile uint32_t last_cmd_time;

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Gửi chuỗi Telemetry chứa các thông số vận hành của xe qua UART
 * @param raw_t Giá trị nhiệt độ thô
 * @param dist_cm Khoảng cách siêu âm (cm)
 * @param warn Cảnh báo vật cản
 * @param enc Số xung Encoder đếm được
 * @param spd Tốc độ hiện tại của xe
 * @param trav Quãng đường di chuyển tổng cộng
 */
void UART_Send_Telemetry(int16_t raw_t, uint32_t dist_cm, int warn,
                         int32_t enc, float spd, float trav,
                         float wheel_left_cm, float wheel_right_cm,
                         int safety_state, int rear_state, int safety_side,
                         int boost_active, int pid_active, int encoder_ok,
                         uint32_t rear_left_cm, uint32_t rear_right_cm,
                         float pid_kp, float pid_ki, float pid_kd);

#endif /* PROTOCOL_UART_H */
