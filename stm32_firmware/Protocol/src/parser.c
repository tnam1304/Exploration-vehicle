/**
 * @file    parser.c
 * @brief   Triển khai bộ phân tích cú pháp lệnh điều khiển cho xe thám hiểm
 */

#include "parser.h"
#include "app_control.h"
#include "app_safety.h"
#include "protocol_uart.h"
#include <stdlib.h>

/* Bộ đệm nội bộ chứa chuỗi lệnh nhận qua UART */
static char rx_buf[PARSER_RX_BUF_SIZE];
static uint8_t rx_idx = 0;

void Parser_ProcessCmd(char rx_cmd) {
    /* Cập nhật thời điểm nhận lệnh cuối cùng để kiểm tra an toàn (Timeout) */
    last_cmd_time = TIM2->CNT;

    /* 1. Nhận các lệnh di chuyển đơn lẻ (1 byte) */
    if (rx_cmd == 'F' || rx_cmd == 'B' || rx_cmd == 'L' || rx_cmd == 'R' || rx_cmd == 'S') {
        drive_cmd = rx_cmd;
        if (rx_cmd == 'S') {
            horn_active = 0;
            Update_Buzzer_State();
        }

        /* Nếu PID đang tắt (chạy dự phòng) -> Cập nhật trực tiếp ra PWM */
        if (pid_enable == 0) {
            Update_Motors_From_Cmd();
        }
        rx_idx = 0;
    }
    /* Bật/Tắt chế độ điều khiển PID */
    else if (rx_cmd == 'P') {
        pid_enable = 1;
        rx_idx = 0;
    }
    else if (rx_cmd == 'p') {
        pid_enable = 0;
        rx_idx = 0;
    }
    /* Bật/Tắt còi báo */
    else if (rx_cmd == 'H') {
        horn_active = 1;
        Update_Buzzer_State();
        rx_idx = 0;
    }
    else if (rx_cmd == 'h') {
        horn_active = 0;
        Update_Buzzer_State();
        rx_idx = 0;
    }
    /* Cập nhật trạng thái kết nối WiFi */
    else if (rx_cmd == 'O') {
        wifi_status_online = 1;
        rx_idx = 0;
    }
    else if (rx_cmd == 'o') {
        wifi_status_online = 0;
        rx_idx = 0;
    }

    /* 2. Nhận chuỗi cấu hình tốc độ động cơ */
    else {
        if (rx_cmd == '\n' || rx_cmd == '\r') {
            rx_buf[rx_idx] = '\0';
            if (rx_buf[0] == 'V') {
                int speed_val = 0;
                if (rx_buf[1] == ':') {
                    speed_val = atoi(&rx_buf[2]);
                } else {
                    speed_val = atoi(&rx_buf[1]);
                }

                /* Quy đổi từ phần trăm (%) sang giá trị PWM */
                if (speed_val >= SPEED_MIN_PERCENT && speed_val <= SPEED_MAX_PERCENT) {
                    speed_val = speed_val * SPEED_PERCENT_SCALE;
                }

                /* Áp dụng tốc độ nếu nằm trong dải PWM hợp lệ */
                if (speed_val >= SPEED_MIN_PWM && speed_val <= SPEED_MAX_PWM) {
                    speed_fwd_left  = (uint16_t)speed_val;
                    speed_fwd_right = (uint16_t)speed_val;
                    speed_bwd_left  = (uint16_t)speed_val;
                    speed_bwd_right = (uint16_t)speed_val;

                    /* Nếu PID đang tắt thì cập nhật ngay lập tức xuống động cơ */
                    if (pid_enable == 0) {
                        Update_Motors_From_Cmd();
                    }
                }
            }
            rx_idx = 0;
        } else {
            /* Lưu ký tự vào bộ đệm nếu chưa vượt quá dung lượng */
            if (rx_idx < (sizeof(rx_buf) - 1)) {
                rx_buf[rx_idx++] = rx_cmd;
            } else {
                rx_idx = 0;
            }
        }
    }
}