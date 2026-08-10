/**
 * @file    dev_oled.h
 * @brief   Driver hiển thị màn hình OLED SSD1306 qua I2C1 cho STM32F401RCT6
 */

#ifndef DEV_OLED_H
#define DEV_OLED_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* THÔNG SỐ CẤU HÌNH OLED SSD1306 (OLED HARDWARE CONFIGURATION)              */
/* ========================================================================== */
#define OLED_I2C_ADDR               0x78    /* Địa chỉ I2C của màn hình OLED */
#define OLED_CTRL_CMD               0x00    /* Byte điều khiển: Gửi lệnh (Command) */
#define OLED_CTRL_DATA              0x40    /* Byte điều khiển: Gửi dữ liệu (Data) */

#define OLED_TOTAL_PIXEL_BYTES      1024    /* Tổng số byte bộ nhớ hiển thị (128x64 / 8) */
#define OLED_FONT_WIDTH             8       /* Độ rộng ký tự Font 8x8 (8 pixel) */

/* Ký tự ASCII hỗ trợ trong Font8x8 */
#define OLED_ASCII_OFFSET           32      /* Ký tự khoảng trắng ' ' (ASCII 32) */
#define OLED_ASCII_MAX              90      /* Ký tự 'Z' (ASCII 90) */

/* ========================================================================== */
/* CÁC CÂU LỆNH ĐIỀU KHIỂN OLED (SSD1306 COMMAND SET)                        */
/* ========================================================================== */
#define OLED_CMD_DISPLAY_OFF        0xAE    /* Tắt màn hình OLED */
#define OLED_CMD_DISPLAY_ON         0xAF    /* Bật màn hình OLED */
#define OLED_CMD_MEM_ADDR_MODE      0x20    /* Đặt chế độ định địa chỉ bộ nhớ */
#define OLED_CMD_ADDR_MODE_HORIZ    0x00    /* Chế độ định địa chỉ hàng ngang (Horizontal) */
#define OLED_CMD_CHARGE_PUMP        0x8D    /* Lệnh cấu hình mạch Charge Pump */
#define OLED_CMD_CHARGE_PUMP_ON     0x14    /* Kích hoạt mạch Charge Pump */
#define OLED_CMD_SET_PAGE_START     0xB0    /* Lệnh đặt trang bắt đầu (Page Start) */
#define OLED_CMD_SET_COLUMN_LOW     0x00    /* Lệnh đặt địa chỉ cột thấp (Lower Column) */
#define OLED_CMD_SET_COLUMN_HIGH    0x10    /* Lệnh đặt địa chỉ cột cao (Higher Column) */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Khởi tạo màn hình OLED SSD1306
 */
void OLED_Init(void);

/**
 * @brief Xóa toàn bộ màn hình OLED (xóa bộ nhớ RAM hiển thị)
 */
void OLED_Clear(void);

/**
 * @brief Đặt vị trí con trỏ hiển thị trên màn hình OLED
 * @param page Trang hiển thị (0 đến 7)
 * @param col Cột hiển thị (0 đến 127)
 */
void OLED_SetCursor(uint8_t page, uint8_t col);

/**
 * @brief In 1 ký tự ASCII lên màn hình OLED
 * @param c Ký tự ASCII cần in
 */
void OLED_PutChar(char c);

/**
 * @brief In một chuỗi ký tự lên màn hình OLED tại vị trí chỉ định
 * @param page Trang hiển thị (0 đến 7)
 * @param col Cột hiển thị (0 đến 127)
 * @param str Chuỗi ký tự cần in
 */
void OLED_PrintString(uint8_t page, uint8_t col, const char* str);

#endif /* DEV_OLED_H */