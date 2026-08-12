/**
 * @file    dev_sonar.c
 * @brief   Driver HC-SR04 theo state machine, không chặn vòng lặp điều khiển.
 */

#include "dev_sonar.h"
#include "bsp_pinout.h"

typedef enum {
    SONAR_IDLE = 0,
    SONAR_WAIT_RISE,
    SONAR_WAIT_FALL
} Sonar_State_t;

static Sonar_State_t sonar_state = SONAR_IDLE;
static uint32_t sonar_started_at = 0;
static uint32_t echo_started_at = 0;

static uint8_t Sonar_EchoIsHigh(void) {
    return (GPIOB->IDR & (1U << SONAR_ECHO_PIN)) != 0U;
}

void Sonar_StartMeasurement(void) {
    if (sonar_state != SONAR_IDLE) {
        return;
    }

    GPIOB->BSRR = (1U << SONAR_TRIG_PIN);
    Delay_us(SONAR_TRIG_PULSE_US);
    GPIOB->BSRR = (1U << (SONAR_TRIG_PIN + 16U));

    sonar_started_at = TIM2->CNT;
    sonar_state = SONAR_WAIT_RISE;
}

bool Sonar_Poll(uint32_t *distance_cm) {
    if (distance_cm == 0 || sonar_state == SONAR_IDLE) {
        return false;
    }

    uint32_t now = TIM2->CNT;

    if (sonar_state == SONAR_WAIT_RISE) {
        if (Sonar_EchoIsHigh()) {
            echo_started_at = now;
            sonar_state = SONAR_WAIT_FALL;
        } else if ((now - sonar_started_at) > SONAR_MAX_TIME_US) {
            *distance_cm = SONAR_ERROR_DIST;
            sonar_state = SONAR_IDLE;
            return true;
        }
    } else if (sonar_state == SONAR_WAIT_FALL) {
        if (!Sonar_EchoIsHigh()) {
            *distance_cm = (now - echo_started_at) / SONAR_US_TO_CM_FACTOR;
            sonar_state = SONAR_IDLE;
            return true;
        }

        if ((now - echo_started_at) > SONAR_MAX_TIME_US) {
            *distance_cm = SONAR_ERROR_DIST;
            sonar_state = SONAR_IDLE;
            return true;
        }
    }

    return false;
}

uint32_t Measure_Distance(void) {
    /* API cũ chỉ giữ lại để tương thích; luồng chính dùng Sonar_Poll(). */
    uint32_t distance = SONAR_ERROR_DIST;
    Sonar_StartMeasurement();
    while (!Sonar_Poll(&distance)) {
    }
    return distance;
}
