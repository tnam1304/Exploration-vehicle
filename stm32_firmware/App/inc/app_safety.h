/**
 * @file    app_safety.h
 * @brief   Module quan ly va danh gia an toan he thong (Tang App)
 */

#ifndef __APP_SAFETY_H
#define __APP_SAFETY_H

#include <stdint.h>

/* Nguong canh bao an toan */
#define TEMP_FIRE_THRESHOLD     50      /* Nguong bao chay: >= 50 do C */
#define DIST_STOP_THRESHOLD     15      /* Nguong dung xe: < 15 cm */

/**
 * @brief Kiem tra tat ca cac nguong an toan cua he thong
 * @param temp_c Nhiet do hien tai do tu DS18B20 (do C)
 * @param dist_cm Khoang cach do tu cam bien sieu am (cm)
 * @return 2: ALARM CHAY, 1: STOP VAT CAN, 0: SAFE
 */
uint8_t App_Safety_Check(int16_t temp_c, uint32_t dist_cm);

#endif /* __APP_SAFETY_H */
