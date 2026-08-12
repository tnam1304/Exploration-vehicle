/**
 * @file    parser.c
 * @brief   Parser UART có khung để lệnh điều khiển không bị lẫn vào nhau.
 */

#include "parser.h"
#include "app_control.h"
#include "app_pid.h"
#include "app_safety.h"
#include "protocol_uart.h"

/* Mỗi lệnh từ ESP32 là một dòng: C:<ký_tự> hoặc V:<20..100>. */
static char rx_buf[PARSER_RX_BUF_SIZE];
static uint8_t rx_idx = 0;

static void Parser_RefreshLink(void) {
    last_cmd_time = TIM2->CNT;
    wifi_status_online = 1;
}

static void Parser_ApplyDriveCommand(char command) {
    char previous_command = drive_cmd;
    drive_cmd = command;

    if (command == 'S') {
        horn_active = 0;
        Update_Buzzer_State();
    }

    /* Không mang tích phân của hướng cũ sang hướng mới. */
    if (command != previous_command) {
        PID_Reset(&pid_left);
        PID_Reset(&pid_right);
    }

    if (pid_enable == 0) {
        Update_Motors_From_Cmd();
    }
}

static void Parser_HandleControl(char command) {
    switch (command) {
        case 'F':
        case 'B':
        case 'L':
        case 'R':
        case 'S':
            Parser_ApplyDriveCommand(command);
            Parser_RefreshLink();
            break;

        case 'P':
            pid_enable = 1;
            PID_Reset(&pid_left);
            PID_Reset(&pid_right);
            Parser_RefreshLink();
            break;

        case 'p':
            pid_enable = 0;
            PID_Reset(&pid_left);
            PID_Reset(&pid_right);
            Update_Motors_From_Cmd();
            Parser_RefreshLink();
            break;

        case 'H':
            horn_active = 1;
            Update_Buzzer_State();
            Parser_RefreshLink();
            break;

        case 'h':
            horn_active = 0;
            Update_Buzzer_State();
            Parser_RefreshLink();
            break;

        case 'O':
            wifi_status_online = 1;
            Parser_RefreshLink();
            break;

        case 'o':
            wifi_status_online = 0;
            last_cmd_time = TIM2->CNT;
            break;

        default:
            break;
    }
}

static void Parser_HandleSpeed(void) {
    uint16_t speed_percent = 0;

    /* V: phải có tối thiểu một chữ số và không chấp nhận ký tự lạ. */
    if (rx_idx < 3) {
        return;
    }

    for (uint8_t i = 2; i < rx_idx; i++) {
        if (rx_buf[i] < '0' || rx_buf[i] > '9') {
            return;
        }
        speed_percent = (uint16_t)(speed_percent * 10U + (uint16_t)(rx_buf[i] - '0'));
        if (speed_percent > CONTROL_SPEED_MAX_PERCENT) {
            return;
        }
    }

    if (speed_percent >= CONTROL_SPEED_MIN_PERCENT) {
        Control_SetSpeedPercent((uint8_t)speed_percent);
        if (pid_enable == 0) {
            Update_Motors_From_Cmd();
        }
        Parser_RefreshLink();
    }
}

static void Parser_HandleFrame(void) {
    rx_buf[rx_idx] = '\0';

    if (rx_idx == 3 && rx_buf[0] == 'C' && rx_buf[1] == ':') {
        Parser_HandleControl(rx_buf[2]);
    } else if (rx_idx >= 3 && rx_buf[0] == 'V' && rx_buf[1] == ':') {
        Parser_HandleSpeed();
    }
}

void Parser_ProcessCmd(char rx_cmd) {
    if (rx_cmd == '\r') {
        return;
    }

    if (rx_cmd == '\n') {
        if (rx_idx > 0) {
            Parser_HandleFrame();
        }
        rx_idx = 0;
        return;
    }

    if (rx_idx < (sizeof(rx_buf) - 1U)) {
        rx_buf[rx_idx++] = rx_cmd;
    } else {
        /* Bỏ trọn frame lỗi để byte rác không tạo lệnh ngẫu nhiên. */
        rx_idx = 0;
    }
}
