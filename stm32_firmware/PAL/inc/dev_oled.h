/**
 * @file    dev_oled.h
 * @brief   Header file driver dieu khien OLED SSD1306 qua I2C1 (Tang Dev)
 */

#ifndef DEV_OLED_H
#define DEV_OLED_H

#include "stm32f4xx.h"

#define OLED_ADDR 0x78

void Dev_OLED_Init(void);
void Dev_OLED_Clear(void);
void Dev_OLED_SetCursor(uint8_t page, uint8_t col);
void Dev_OLED_PutChar(char c);
void Dev_OLED_PrintString(uint8_t page, uint8_t col, const char* str);

#endif /* DEV_OLED_H */
