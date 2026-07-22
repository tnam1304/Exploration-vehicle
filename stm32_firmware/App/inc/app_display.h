/**
 * @file    app_display.h
 * @brief   Header file quan ly giao dien hien thi OLED SSD1306 (Tang App)
 */

#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <stdint.h>

void App_Display_Init(void);
void App_Display_Update(uint8_t wifi_online, uint32_t dist_cm, int16_t temp_c, uint8_t warn_code);

#endif /* APP_DISPLAY_H */
