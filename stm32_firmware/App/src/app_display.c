/**
 * @file    app_display.c
 * @brief   Module quan ly giao dien hien thi OLED SSD1306 (Tang App)
 */

#include "app_display.h"
#include "dev_oled.h"
#include <stdio.h>

void App_Display_Init(void) {
    Dev_OLED_Init();
    Dev_OLED_Clear();
}

void App_Display_Update(uint8_t wifi_online, uint32_t dist_cm, int16_t temp_c, uint8_t warn_code) {
    char oled_buffer[20];

    // 1. Dòng 0: Trạng thái kết nối Wi-Fi
    if (wifi_online != 0) {
        Dev_OLED_PrintString(0, 0, "WIFI: ONLINE   ");
    } else {
        Dev_OLED_PrintString(0, 0, "WIFI: OFFLINE  ");
    }

    // 2. Dòng 2: Khoảng cách siêu âm (Xử lý trường hợp lỗi giá trị 999cm)
    if (dist_cm == 999) {
        sprintf(oled_buffer, "DIST:   0 CM    ");
    } else {
        sprintf(oled_buffer, "DIST: %3lu CM    ", (unsigned long)dist_cm);
    }
    Dev_OLED_PrintString(2, 0, oled_buffer);

    // 3. Dòng 4: Nhiệt độ môi trường từ DS18B20
    sprintf(oled_buffer, "TEMP: %2d C     ", (int)temp_c);
    Dev_OLED_PrintString(4, 0, oled_buffer);

    // 4. Dòng 6: Trạng thái cảnh báo an toàn
    if (warn_code == 2) {
        Dev_OLED_PrintString(6, 0, "!! ALARM CHAY !!");
    } else if (warn_code == 1) {
        Dev_OLED_PrintString(6, 0, "!! STOP VAT CAN ");
    } else {
        Dev_OLED_PrintString(6, 0, "STATUS: SAFE    ");
    }
}
