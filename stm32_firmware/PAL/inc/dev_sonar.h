/**
 * @file    dev_sonar.h
 * @brief   Driver đo khoảng cách cảm biến siêu âm HC-SR04 cho STM32F401RCT6
 */

#ifndef DEV_SONAR_H
#define DEV_SONAR_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================== */
/* THÔNG SỐ CẤU HÌNH CẢM BIẾN SIÊU ÂM (SONAR HARDWARE CONFIGURATION)          */
/* ========================================================================== */
#define SONAR_TRIG_PIN              10          /* PB10 - Chân Trigger phát xung */
#define SONAR_ECHO_PIN              2           /* PB2  - Chân Echo nhận phản hồi */

#define SONAR_TRIG_PULSE_US         10          /* Độ rộng xung Trigger (10us) */
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

/** Bắt đầu một lần đo; hàm chỉ chặn 10us để phát xung trigger. */
void Sonar_StartMeasurement(void);

/**
 * @brief Thăm dò kết quả đo không chặn CPU.
 * @return true khi một kết quả (kể cả timeout) đã sẵn sàng.
 */
bool Sonar_Poll(uint32_t *distance_cm);

#endif /* DEV_SONAR_H */
