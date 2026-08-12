/**
 * @file    app_display.c
 * @brief   Cập nhật OLED theo từng hàng để không làm trễ PID.
 */

#include "app_display.h"
#include "dev_oled.h"
#include <stdio.h>
#include <stdlib.h>

void App_Display_Render(uint8_t wifi_online, uint32_t dist, int16_t temp,
                        int warn, int32_t enc, float speed, float travel) {
    static uint8_t next_page = OLED_PAGE_WIFI;
    char oled_buffer[DISPLAY_BUF_SIZE];

    /* Mỗi lần gọi chỉ gửi một hàng (~12 ms ở I2C 100 kHz). */
    switch (next_page) {
        case OLED_PAGE_WIFI:
            OLED_PrintString(OLED_PAGE_WIFI, 0, wifi_online ? "WIFI: ONLINE    " : "WIFI: OFFLINE   ");
            break;

        case OLED_PAGE_STATUS:
            if (warn == WARN_LEVEL_FIRE) {
                OLED_PrintString(OLED_PAGE_STATUS, 0, "!! ALARM CHAY !!");
            } else if (warn == WARN_LEVEL_OBSTACLE || (dist > 0 && dist <= OBSTACLE_STOP_DIST_CM)) {
                OLED_PrintString(OLED_PAGE_STATUS, 0, "!! STOP VAT CAN ");
            } else {
                OLED_PrintString(OLED_PAGE_STATUS, 0, "STATUS: SAFE    ");
            }
            break;

        case OLED_PAGE_TEMP:
            sprintf(oled_buffer, "TEMP: %2d C   ", temp);
            OLED_PrintString(OLED_PAGE_TEMP, 0, oled_buffer);
            break;

        case OLED_PAGE_DIST:
            sprintf(oled_buffer, "DIST: %3lu CM    ", (dist == SONAR_ERROR_DIST_VAL) ? 0UL : (unsigned long)dist);
            OLED_PrintString(OLED_PAGE_DIST, 0, oled_buffer);
            break;

        case OLED_PAGE_SPEED: {
            int speed_int = (int)speed;
            int speed_dec = abs((int)(speed * 10.0f)) % 10;
            if (speed < 0.0f && speed_int == 0) {
                sprintf(oled_buffer, "SPD :-0.%d CM/S  ", speed_dec);
            } else {
                sprintf(oled_buffer, "SPD : %d.%d CM/S  ", speed_int, speed_dec);
            }
            OLED_PrintString(OLED_PAGE_SPEED, 0, oled_buffer);
            break;
        }

        case OLED_PAGE_TRAVEL: {
            int travel_int = (int)travel;
            int travel_dec = abs((int)(travel * 10.0f)) % 10;
            sprintf(oled_buffer, "TRV : %d.%d CM    ", travel_int, travel_dec);
            OLED_PrintString(OLED_PAGE_TRAVEL, 0, oled_buffer);
            break;
        }

        case OLED_PAGE_ENCODER:
            sprintf(oled_buffer, "ENC : %-10ld", (long)enc);
            OLED_PrintString(OLED_PAGE_ENCODER, 0, oled_buffer);
            break;

        default:
            OLED_PrintString(OLED_PAGE_FOOTER, 0, "----------------");
            break;
    }

    next_page++;
    if (next_page > OLED_PAGE_FOOTER) {
        next_page = OLED_PAGE_WIFI;
    }
}
