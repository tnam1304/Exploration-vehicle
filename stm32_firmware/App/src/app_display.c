/**
 * @file    app_display.c
 * @brief   Cập nhật OLED theo từng hàng để không làm trễ PID.
 */

#include "app_display.h"
#include "dev_oled.h"
#include <stdio.h>
#include <stdlib.h>

static const char *Display_SideText(Safety_Side_t side) {
    if (side == SAFETY_SIDE_LEFT) return "L";
    if (side == SAFETY_SIDE_RIGHT) return "R";
    if (side == SAFETY_SIDE_BOTH) return "LR";
    return "";
}

void App_Display_Render(uint8_t wifi_online, uint32_t dist, int16_t temp,
                        int warn, int32_t enc, float speed, float travel,
                        Safety_State_t safety_state,
                        Safety_Side_t safety_side) {
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
            } else {
                const char *side = Display_SideText(safety_side);
                switch (safety_state) {
                    case SAFETY_STATE_FCW:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "FRONT: FCW      ");
                        break;
                    case SAFETY_STATE_PRE_BRAKE:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "FRONT: BRAKING  ");
                        break;
                    case SAFETY_STATE_AEB:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "FRONT: AEB STOP ");
                        break;
                    case SAFETY_STATE_AEB_HOLD:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "FRONT: AEB HOLD ");
                        break;
                    case SAFETY_STATE_REVERSE_WARNING:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REV WARN", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_REVERSE_DANGER:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REV DANGER", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_REAR_APPROACHING:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REAR NEAR", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_REAR_WARNING:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REAR WARN", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_REAR_DANGER:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REAR DANGER", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_REAR_BOOST:
                        sprintf(oled_buffer, "%-11s %-2s  ", "REAR BOOST", side);
                        OLED_PrintString(OLED_PAGE_STATUS, 0, oled_buffer);
                        break;
                    case SAFETY_STATE_SONAR_ERROR:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "SONAR: ERROR     ");
                        break;
                    case SAFETY_STATE_ENCODER_ERROR:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "ENCODER: ERROR   ");
                        break;
                    default:
                        OLED_PrintString(OLED_PAGE_STATUS, 0, "STATUS: SAFE    ");
                        break;
                }
            }
            break;

        case OLED_PAGE_TEMP:
            sprintf(oled_buffer, "TEMP: %2d C   ", temp);
            OLED_PrintString(OLED_PAGE_TEMP, 0, oled_buffer);
            break;

        case OLED_PAGE_DIST:
            sprintf(oled_buffer, "DIST: %3lu CM    ",
                    (dist == SONAR_ERROR_DIST_VAL) ? 0UL : (unsigned long)dist);
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
