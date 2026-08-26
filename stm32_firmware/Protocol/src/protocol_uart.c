/**
 * @file    protocol_uart.c
 * @brief   Triển khai trình phục vụ ngắt USART2_IRQHandler và hàm gửi Telemetry
 */

#include "protocol_uart.h"
#include "parser.h"
#include <stdio.h>

/* Cờ trạng thái kết nối WiFi và thời điểm nhận lệnh gần nhất */
volatile uint8_t wifi_status_online = 0;
volatile uint32_t last_cmd_time = 0;

/**
 * @brief Trình phục vụ ngắt USART2 (Nhận dữ liệu điều khiển từ ứng dụng)
 */
void USART2_IRQHandler(void) {
    /* 1. Xóa các cờ lỗi Overrun, Noise, Framing Error bằng cách đọc thanh ghi DR */
    if (USART2->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE)) {
        (void)USART2->DR;
    }

    /* 2. Kiểm tra cờ RXNE (Receive Data Register Not Empty) */
    if (USART2->SR & USART_SR_RXNE) {
        char rx_cmd = (char)(USART2->DR & 0xFF);

        /* Parser chỉ làm mới watchdog sau khi nhận được một frame hợp lệ. */
        Parser_ProcessCmd(rx_cmd);
    }
}

void UART_Send_Telemetry(int16_t raw_t, uint32_t dist_cm, int warn,
                         int32_t enc, float spd, float trav,
                         float wheel_left_cm, float wheel_right_cm,
                         int safety_state, int rear_state, int safety_side,
                         int boost_active, int pid_active, int encoder_ok,
                         uint32_t rear_left_cm, uint32_t rear_right_cm,
                         float pid_kp, float pid_ki, float pid_kd) {
    char tx_buffer[UART_TX_BUF_SIZE];
    
    /* Quy đổi nhiệt độ thô ra độ C */
    int16_t temp_c = (raw_t == TEMP_SENSOR_ERROR_RAW) ? TEMP_SENSOR_ERROR_DISP : (raw_t / TEMP_SCALE_FACTOR);
    
    /* Lọc giá trị khoảng cách lỗi */
    uint32_t d_send = (dist_cm == SONAR_ERROR_RAW_DIST) ? SONAR_ERROR_DISP_DIST : dist_cm;

    /* Nhân hệ số 10 để ép kiểu float thành int (tránh lỗi %f khi dùng nano.specs trong Keil C/GCC) */
    int spd_int = (int)(spd * TELEMETRY_FLOAT_SCALE);
    int trav_int = (int)(trav * TELEMETRY_FLOAT_SCALE);
    long wheel_left_int = (long)(wheel_left_cm * TELEMETRY_FLOAT_SCALE);
    long wheel_right_int = (long)(wheel_right_cm * TELEMETRY_FLOAT_SCALE);
    int kp_int = (int)(pid_kp * TELEMETRY_PID_SCALE + 0.5f);
    int ki_int = (int)(pid_ki * TELEMETRY_PID_SCALE + 0.5f);
    int kd_int = (int)(pid_kd * TELEMETRY_PID_SCALE + 0.5f);

    /* Sáu trường đầu giữ nguyên; các trường sau đồng bộ safety với ESP32. */
    int len = sprintf(tx_buffer,
                      "T%d;D%lu;W%d;E%ld;S%d;M%d;A%d;R%d;X%d;B%d;P%d;I%d;DL%lu;DR%lu;KP%d;KI%d;KD%d;EL%ld;ER%ld\n",
                      temp_c, (unsigned long)d_send, warn, (long)enc,
                      spd_int, trav_int,
                      safety_state, rear_state, safety_side, boost_active,
                      pid_active, encoder_ok,
                      (unsigned long)rear_left_cm,
                      (unsigned long)rear_right_cm,
                      kp_int, ki_int, kd_int,
                      wheel_left_int, wheel_right_int);

    /* Truyền chuỗi ký tự qua UART2 */
    for (int i = 0; i < len; i++) {
        while (!(USART2->SR & USART_SR_TXE));
        USART2->DR = tx_buffer[i];
    }
}
