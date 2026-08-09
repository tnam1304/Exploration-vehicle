#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * CẤU TRÚC FRAME TELEMETRY (STM32 -> ESP32-CAM -> WEB)
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  header1;
    uint8_t  header2;
    float    temp;
    float    dist;
    uint8_t  warn;
    float    x;
    float    y;
    float    theta;
    uint8_t  checksum;
} TelemetryFrame_t;
#pragma pack(pop)

/**
* 2. CẤU TRÚC FRAME COMMAND (WEB -> ESP32-CAM -> STM32)
*/

#pragma pack(push, 1)
typedef struct {
    uint8_t  header1;
    uint8_t  header2;
    uint8_t  cmd;       // Mã lệnh
    uint8_t  checksum;
} CommandFrame_t;
#pragma pack(pop)

/**
 * CÁC HÀM API ĐÓNG GÓI / GIẢI MÃ
 */

/**
 * @brief Đóng gói dữ liệu cảm biến và tọa độ vào Frame
 */
void Parser_CreateTelemetryFrame(TelemetryFrame_t *frame, float temp, float dist, uint8_t warn, float x, float y, float theta);

/**
 * @brief Đọc mảng byte nhận từ UART, kiểm tra Header, Checksum và lấy ra lệnh điều khiển
 * @return true nếu Frame chuẩn, false nếu có lỗi nhiễu hoặc sai checksum
 */
bool Parser_ParseCommandFrame(uint8_t *buffer, uint16_t len, uint8_t *out_cmd);

#endif
