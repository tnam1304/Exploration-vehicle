/**
 * @file    dev_ds18b20.h
 * @brief   Driver điều khiển cảm biến nhiệt độ 1-Wire DS18B20 cho STM32F401RCT6
 */

#ifndef DEV_DS18B20_H
#define DEV_DS18B20_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* LỆNH VÀ MÃ LỖI CẢM BIẾN DS18B20 (ONE-WIRE COMMANDS & STATUS)              */
/* ========================================================================== */
#define DS18B20_CMD_SKIP_ROM        0xCC    /* Bỏ qua kiểm tra ID ROM */
#define DS18B20_CMD_CONVERT_T       0x44    /* Yêu cầu cảm biến bắt đầu đo và chuyển đổi nhiệt độ */
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE    /* Đọc bộ nhớ Scratchpad chứa dữ liệu nhiệt độ */
#define DS18B20_ERROR_RAW_TEMP      -9999   /* Mã lỗi trả về khi không phát hiện cảm biến */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Phát xung Reset và kiểm tra tín hiệu phản hồi (Presence) từ cảm biến
 * @retval 1 nếu phát hiện cảm biến, 0 nếu không có cảm biến phản hồi
 */
uint8_t DS18B20_Reset(void);

/**
 * @brief Gửi 1 bit dữ liệu đến cảm biến theo chuẩn 1-Wire
 * @param bit Giá trị bit cần gửi (0 hoặc 1)
 */
void DS18B20_WriteBit(uint8_t bit);

/**
 * @brief Gửi 1 byte dữ liệu đến cảm biến
 * @param data Byte dữ liệu cần gửi
 */
void DS18B20_WriteByte(uint8_t data);

/**
 * @brief Đọc 1 bit dữ liệu từ cảm biến theo chuẩn 1-Wire
 * @retval Giá trị bit đọc được (0 hoặc 1)
 */
uint8_t DS18B20_ReadBit(void);

/**
 * @brief Đọc 1 byte dữ liệu từ cảm biến
 * @retval Byte dữ liệu đọc được
 */
uint8_t DS18B20_ReadByte(void);

/**
 * @brief Phát lệnh bắt đầu quá trình đo và chuyển đổi nhiệt độ
 */
void DS18B20_StartConversion(void);

/**
 * @brief Đọc giá trị nhiệt độ thô (16-bit) từ bộ nhớ Scratchpad của cảm biến
 * @retval Giá trị nhiệt độ thô hoặc DS18B20_ERROR_RAW_TEMP (-9999) nếu lỗi
 */
int16_t DS18B20_ReadRawTemp(void);

#endif /* DEV_DS18B20_H */