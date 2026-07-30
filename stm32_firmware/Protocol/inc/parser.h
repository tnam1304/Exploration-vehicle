#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include "stdint.h"

#pragma pack(push, 1)

// 1. Struct dữ liệu gửi từ STM32 -> ESP32-CAM -> Web
typedef struct {
    uint16_t header;        // Đồng bộ khung truyền
    uint8_t  packet_id;     // Mã gói tin
    int32_t  enc_left;      // Delta xung encoder bánh trái
    int32_t  enc_right;     // Delta xung encoder bánh phải
    int8_t   temperature;   // Nhiệt độ hệ thống / động cơ (°C)
    uint8_t  battery_pct;   // Phần trăm pin (0 - 100)
    uint8_t  car_state;     // Trạng thái xe (0: Đứng yên, 1: Chạy, 2: Tự lùi, 3: Lỗi)
    uint8_t  sys_error;     // Mã lỗi (0: Bình thường, 1: Kẹt động cơ, 2: Quá nhiệt)
    uint16_t crc16;         // Mã kiểm tra
} OptimalTelemetryFrame_t;

// 2. Struct lệnh điều khiển từ Web -> ESP32-CAM -> STM32
typedef struct {
    uint16_t header;
    uint8_t  command_id;    // 0x01: Joystick, 0x02: Support-Parking, 0x03: Voice Command
    uint8_t  voice_action;
    int16_t  setpoint_left; // Vận tốc mong muốn bánh trái
    int16_t  setpoint_right;// Vận tốc mong muốn bánh phải
    uint16_t crc16;
    uint16_t footer;
} CommandFrame_t;

#pragma pack(pop)

/**
 * @brief Hàm tính toán mã CRC16 kiểm tra lỗi khung truyền
 */
uint16_t calculateCRC16(const uint8_t *data, size_t length);

#endif
