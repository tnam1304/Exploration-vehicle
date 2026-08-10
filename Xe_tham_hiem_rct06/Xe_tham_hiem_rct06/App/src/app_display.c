/**
 * @file    app_display.c
 * @brief   Triển khai xuất dữ liệu dạng chuỗi ký tự chuẩn hóa lên màn hình OLED
 */

#include "app_display.h"
#include "dev_oled.h"
#include <stdio.h>
#include <stdlib.h>

void App_Display_Render(uint8_t wifi_online, uint32_t dist, int16_t temp, 
                       int warn, int32_t enc, float speed, float travel) {
    char oled_buffer[DISPLAY_BUF_SIZE];

    /* 1. Trạng thái kết nối WiFi (Page 0) */
    if (wifi_online) {
        OLED_PrintString(OLED_PAGE_WIFI, 0, "WIFI: ONLINE    ");
    } else {
        OLED_PrintString(OLED_PAGE_WIFI, 0, "WIFI: OFFLINE   ");
    }

    /* 2. Trạng thái cảnh báo an toàn (Page 1) */
    if (warn == WARN_LEVEL_FIRE) {
        OLED_PrintString(OLED_PAGE_STATUS, 0, "!! ALARM CHAY !!");
    } else if (warn == WARN_LEVEL_OBSTACLE || (dist > 0 && dist <= OBSTACLE_STOP_DIST_CM)) {
        OLED_PrintString(OLED_PAGE_STATUS, 0, "!! STOP VAT CAN ");
    } else {
        OLED_PrintString(OLED_PAGE_STATUS, 0, "STATUS: SAFE    ");
    }

    /* 3. Nhiệt độ môi trường (Page 2) */
    sprintf(oled_buffer, "TEMP: %2d C   ", temp);
    OLED_PrintString(OLED_PAGE_TEMP, 0, oled_buffer);

    /* 4. Khoảng cách vật cản (Page 3) */
    sprintf(oled_buffer, "DIST: %3lu CM    ", (dist == SONAR_ERROR_DIST_VAL) ? 0 : dist);
    OLED_PrintString(OLED_PAGE_DIST, 0, oled_buffer);

    /* 5. Vận tốc di chuyển (Page 4) - Bóc tách phần nguyên và phần thập phân (tránh dùng %f của nano.specs) */
    int spd_int = (int)speed;
    int spd_dec = abs((int)(speed * 10.0f)) % 10;
    if (speed < 0.0f && spd_int == 0) {
        sprintf(oled_buffer, "SPD :-0.%d CM/S  ", spd_dec);
    } else {
        sprintf(oled_buffer, "SPD : %d.%d CM/S  ", spd_int, spd_dec);
    }
    OLED_PrintString(OLED_PAGE_SPEED, 0, oled_buffer);

    /* 6. Quãng đường tích lũy (Page 5) */
    int trv_int = (int)travel;
    int trv_dec = abs((int)(travel * 10.0f)) % 10;
    sprintf(oled_buffer, "TRV : %d.%d CM    ", trv_int, trv_dec);
    OLED_PrintString(OLED_PAGE_TRAVEL, 0, oled_buffer);

    /* 7. Số xung Encoder đếm được (Page 6) */
    sprintf(oled_buffer, "ENC : %-10ld", (long)enc);
    OLED_PrintString(OLED_PAGE_ENCODER, 0, oled_buffer);

    /* 8. Đường kẻ trang trí chân trang (Page 7) */
    OLED_PrintString(OLED_PAGE_FOOTER, 0, "----------------");
}
