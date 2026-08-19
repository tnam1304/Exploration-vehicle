#ifndef DEV_SONAR_CONFIG_H
#define DEV_SONAR_CONFIG_H

#include "stm32f4xx.h"

#define DEV_SONAR_MAX_SENSOR_COUNT           3U

#define DEV_SONAR_ECHO_EXTI_LINE_MIN          2U

#define DEV_SONAR_ECHO_EXTI_LINE_MAX          4U


/* CẤU HÌNH THỜI GIAN ĐO */


/* TIM2 được chia clock để mỗi lần tăng CNT tương ứng 1 us. */
#define DEV_SONAR_TIMER_TICK_HZ             1000000UL

/* HC-SR04/HC-SR05 yêu cầu xung Trigger mức cao tối thiểu 10 us. */
#define DEV_SONAR_TRIGGER_PULSE_US           10UL

/*
 * Chỉ một cảm biến được phát tại mỗi thời điểm.
 * Cứ 20 ms driver mới cho phép phát cảm biến kế tiếp.
 */
#define DEV_SONAR_TRIGGER_INTERVAL_US        20000UL

/*
 * Sau 20 ms không nhận đủ xung Echo thì kết quả được đánh dấu không hợp lệ.
 * Timeout này tương ứng tầm đo thực tế khoảng 3,4 m.
 */
#define DEV_SONAR_ECHO_TIMEOUT_US            20000UL

/* Loại bỏ xung quá ngắn, thường do nhiễu cạnh hoặc dây tín hiệu không ổn định. */
#define DEV_SONAR_MIN_ECHO_PULSE_US          100UL

/* Khoảng cách lớn hơn giá trị này được coi là ngoài vùng đo tin cậy. */
#define DEV_SONAR_MAX_DISTANCE_MM            3400UL

/* Độ ưu tiên ngắt EXTI dùng chung cho các chân Echo đã đăng ký. */
#define DEV_SONAR_EXTI_PRIORITY              3UL


/* KIỂM TRA CẤU HÌNH NGAY KHI BIÊN DỊCH                                      */


#if (DEV_SONAR_MAX_SENSOR_COUNT == 0U)
#error "DEV_SONAR_MAX_SENSOR_COUNT must be greater than zero"
#endif

#if (DEV_SONAR_MAX_SENSOR_COUNT > 3U)
#error "EXTI2..EXTI4 supports no more than three sonar sensors"
#endif

#if (DEV_SONAR_ECHO_EXTI_LINE_MIN != 2U)
#error "This driver implementation starts at EXTI line 2"
#endif

#if (DEV_SONAR_ECHO_EXTI_LINE_MAX != 4U)
#error "This driver implementation ends at EXTI line 4"
#endif

#if (DEV_SONAR_TIMER_TICK_HZ == 0UL)
#error "DEV_SONAR_TIMER_TICK_HZ must be greater than zero"
#endif

#if (DEV_SONAR_TRIGGER_PULSE_US == 0UL)
#error "DEV_SONAR_TRIGGER_PULSE_US must be greater than zero"
#endif

#if (DEV_SONAR_ECHO_TIMEOUT_US < DEV_SONAR_TRIGGER_PULSE_US)
#error "DEV_SONAR_ECHO_TIMEOUT_US is too small"
#endif

#endif /* DEV_SONAR_CONFIG_H */
