/**
 * @file    bsp_pinout.h
 * @brief   Cấu hình chân ngoại vi và thông số hệ thống cho STM32F401RCT6
 */

#ifndef BSP_PINOUT_H
#define BSP_PINOUT_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* ĐỊNH NGHĨA CHÂN GPIO (PIN MAPPING)                                         */
/* ========================================================================== */

/* Cảm biến nhiệt độ DS18B20 */
#define DS18B20_PIN             1       /* PA1 */

/* Giao tiếp Bluetooth/Serial USART2 */
#define USART2_TX_PIN           2       /* PA2 */
#define USART2_RX_PIN           3       /* PA3 */

/* Động cơ TB6612FNG - Kênh PWM TIM3 */
#define MOTOR_PWM_A_PIN         6       /* PA6 - TIM3_CH1 */
#define MOTOR_PWM_B_PIN         7       /* PA7 - TIM3_CH2 */

/* Động cơ TB6612FNG - Tín hiệu điều khiển hướng */
#define MOTOR_AIN1_PIN          8       /* PA8 */
#define MOTOR_AIN2_PIN          9       /* PA9 */
#define MOTOR_BIN1_PIN          10      /* PA10 */
#define MOTOR_BIN2_PIN          0       /* PB0 */
#define MOTOR_STBY_PIN          5       /* PB5 */

/* Còi báo Buzzer */
#define BUZZER_PIN              1       /* PB1 */

/* Cảm biến siêu âm HC-SR04 */
#define HC_SR04_ECHO_PIN        2       /* PB2 */
#define HC_SR04_TRIG_PIN        10      /* PB10 */

/* Màn hình OLED I2C1 */
#define OLED_SCL_PIN            8       /* PB8 */
#define OLED_SDA_PIN            9       /* PB9 */

/* Timer 2 - Bộ đếm micro-giây */
#define TIM2_PRESCALER_1US      (16 - 1)        /* HSI 16MHz: 16 xung / 16 = 1MHz (1us/tick) */
#define TIM2_ARR_MAX_32BIT      0xFFFFFFFFUL   /* Đếm tối đa 32-bit */

/* Timer 3 - Cấu hình PWM Động cơ */
#define TIM3_PWM_PRESCALER      (16 - 1)        /* Tần số đếm 1MHz */
#define TIM3_PWM_PERIOD_ARR     (1000 - 1)      /* Tần số PWM = 1kHz (0 đến 999) */

/* USART2 - Cấu hình Tốc độ Baud */
#define USART2_BRR_115200_16MHZ 0x008B          /* Tốc độ 115200 bps tại xung APB1 = 16MHz */

/* I2C1 - Cấu hình Tốc độ Standard Mode (100kHz) */
#define I2C_CR2_16MHZ           16              /* Tần số xung đầu vào APB1 = 16MHz */
#define I2C_CCR_100KHZ          160             /* Giá trị CCR cho tốc độ 100kHz */
#define I2C_TRISE_100KHZ        17              /* Thời gian tăng điện áp tối đa */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Khởi tạo TIM2 làm bộ đếm micro-giây 32-bit
 */
void Timer2_Init(void);

/**
 * @brief Tạo thời gian trễ theo micro-giây
 * @param us Số micro-giây cần trễ
 */
void Delay_us(uint32_t us);

/**
 * @brief Tạo thời gian trễ theo mili-giây
 * @param ms Số mili-giây cần trễ
 */
void Delay_ms(uint32_t ms);

/**
 * @brief Khởi tạo tất cả GPIO và ngoại vi hệ thống (OLED, Cảm biến, Động cơ, UART)
 */
void Peripherals_Init(void);

#endif /* BSP_PINOUT_H */
