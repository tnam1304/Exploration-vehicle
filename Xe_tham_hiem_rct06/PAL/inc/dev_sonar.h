/**
 * @file    dev_sonar.h
 * @brief   Driver đo khoảng cách cảm biến siêu âm HC-SR04 cho STM32F401RCT6
 */

#ifndef DEV_SONAR_H
#define DEV_SONAR_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* THÔNG SỐ CẤU HÌNH CẢM BIẾN SIÊU ÂM (SONAR HARDWARE CONFIGURATION)          */
/* ========================================================================== */
#define SONAR_TRIG_PIN              10          /* PB10 - Chân Trigger phát xung */
#define SONAR_ECHO_PIN              2           /* PB2  - Chân Echo nhận phản hồi */

#define SONAR_TRIG_PULSE_US         10          /* Độ rộng xung Trigger (10us) */
#define SONAR_TIMEOUT_COUNT         30000       /* Số vòng lặp chờ tín hiệu Echo trước khi timeout */
#define SONAR_MAX_TIME_US           25000       /* Thời gian chờ Echo tối đa (us) ~ 430cm */
#define SONAR_US_TO_CM_FACTOR       58          /* Hệ số quy đổi microsecond sang centimet (us / 58 = cm) */
#define SONAR_ERROR_DIST            999         /* Giá trị trả về khi đo lỗi/timeout (cm) */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Thực hiện đo khoảng cách bằng cảm biến siêu âm HC-SR04
 * @retval Khoảng cách đo được tính bằng centimet (cm), hoặc 999 nếu timeout/lỗi
 */
uint32_t Measure_Distance(void);

#endif /* DEV_SONAR_H */