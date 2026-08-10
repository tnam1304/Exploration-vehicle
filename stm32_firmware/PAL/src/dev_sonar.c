/**
 * @file    dev_sonar.c
 * @brief   Triển khai phát xung Trigger và đo độ rộng xung Echo dùng Timer2
 */

#include "dev_sonar.h"
#include "bsp_pinout.h"

uint32_t Measure_Distance(void) {
    /* 1. Phát xung Trigger high trong 10us */
    GPIOB->BSRR = (1 << SONAR_TRIG_PIN);
    Delay_us(SONAR_TRIG_PULSE_US);
    GPIOB->BSRR = (1 << (SONAR_TRIG_PIN + 16));

    /* 2. Chờ chân Echo lên mức HIGH với cơ chế Timeout */
    uint32_t timeout = SONAR_TIMEOUT_COUNT;
    while (!(GPIOB->IDR & (1 << SONAR_ECHO_PIN))) {
        if (--timeout == 0) {
            return SONAR_ERROR_DIST;
        }
    }

    /* 3. Bắt đầu tính thời gian độ rộng xung HIGH trên chân Echo */
    uint32_t start = TIM2->CNT;
    while (GPIOB->IDR & (1 << SONAR_ECHO_PIN)) {
        if ((TIM2->CNT - start) > SONAR_MAX_TIME_US) {
            break;
        }
    }

    /* 4. Quy đổi thời gian phản hồi (us) ra khoảng cách (cm) */
    return (TIM2->CNT - start) / SONAR_US_TO_CM_FACTOR;
}