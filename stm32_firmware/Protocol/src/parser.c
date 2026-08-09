#include "parser.h"
#include <stddef.h>

/**
 * @brief Hàm đóng gói dữ liệu thành Frame để gửi đi
 */
void Parser_CreateTelemetryFrame(TelemetryFrame_t *frame, float temp, float dist, uint8_t warn, float x, float y, float theta) {
    if (frame == NULL) return;

    frame->header1 = 0xAA;
    frame->header2 = 0x55;
    frame->temp = temp;
    frame->dist = dist;
    frame->warn = warn;
    frame->x = x;
    frame->y = y;
    frame->theta = theta;

    // Tính Checksum: XOR toàn bộ các byte trong Struct (Trừ byte checksum ở cuối)
    uint8_t *ptr = (uint8_t *)frame;
    uint8_t sum = 0;

    // sizeof(TelemetryFrame_t) = 24 bytes, vòng lặp chạy từ 0 đến 22
    for (uint16_t i = 0; i < sizeof(TelemetryFrame_t) - 1; i++) {
        sum ^= ptr[i];
    }

    frame->checksum = sum;
}

/**
 * @brief Hàm bóc tách gói tin điều khiển (4 bytes) nhận được từ ESP32-CAM
 */
bool Parser_ParseCommandFrame(uint8_t *buffer, uint16_t len, uint8_t *out_cmd) {
    // Nếu kích thước mảng byte không đủ 1 Frame -> Hủy
    if (buffer == NULL || out_cmd == NULL || len < sizeof(CommandFrame_t)) {
        return false;
    }

    // Quét tìm bộ Header 0xCC 0x33 (Phòng trường hợp bị trượt byte rác ở đầu)
    for (uint16_t i = 0; i <= len - sizeof(CommandFrame_t); i++) {

        if (buffer[i] == 0xCC && buffer[i+1] == 0x33) {

            // Ép kiểu mảng byte tại vị trí tìm thấy thành Struct
            CommandFrame_t *cmd_frame = (CommandFrame_t *)&buffer[i];

            // Tính toán lại Checksum để đối chiếu
            uint8_t calc_checksum = cmd_frame->header1 ^ cmd_frame->header2 ^ cmd_frame->cmd;

            // Nếu Checksum khớp, nghĩa là gói tin nguyên vẹn, không bị nhiễu
            if (calc_checksum == cmd_frame->checksum) {
                *out_cmd = cmd_frame->cmd; // Xuất lệnh ra ngoài biến con trỏ
                return true;
            }
        }
    }

    return false; // Không tìm thấy frame nào hợp lệ
}
