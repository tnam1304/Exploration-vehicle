/**
 * @file    app_control.h
 * @brief   Tầng điều khiển chuyển động và trạng thái động cơ cho xe thám hiểm
 */

#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* HẰNG SỐ CẤU HÌNH ĐIỀU KHIỂN (CONTROL CONFIGURATION CONSTANTS)               */
/* ========================================================================== */
#define DEFAULT_SPEED_PWM           400         /* Tốc độ PWM mặc định ban đầu */
#define MIN_DECEL_PWM               200         /* Tốc độ PWM tối thiểu khi giảm tốc tránh vật cản */

#define SAFETY_STOP_DIST_CM         10          /* Khoảng cách dừng xe khẩn cấp (cm) */
#define SAFETY_SLOW_DIST_CM         50          /* Khoảng cách bắt đầu giảm tốc (cm) */
#define SAFETY_RANGE_DIST_CM        40.0f       /* Dải khoảng cách giảm tốc (50cm - 10cm) */

/* ========================================================================== */
/* BIẾN TOÀN CỤC CHIA SẺ (SHARED GLOBAL VARIABLES)                           */
/* ========================================================================== */
extern uint16_t speed_fwd_left;
extern uint16_t speed_fwd_right;
extern uint16_t speed_bwd_left;
extern uint16_t speed_bwd_right;

extern volatile char drive_cmd;
extern uint8_t pid_enable;

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Cho xe tiến thẳng ở tốc độ cài đặt
 */
void Car_Forward_Normal(void);

/**
 * @brief Cho xe lùi thẳng ở tốc độ cài đặt
 */
void Car_Backward(void);

/**
 * @brief Cho xe xoay sang trái
 */
void Car_TurnLeft(void);

/**
 * @brief Cho xe xoay sang phải
 */
void Car_TurnRight(void);

/**
 * @brief Dừng toàn bộ động cơ
 */
void Car_Stop(void);

/**
 * @brief Cập nhật trạng thái động cơ dựa trên lệnh điều khiển và khoảng cách an toàn
 */
void Update_Motors_From_Cmd(void);

#endif /* APP_CONTROL_H */