#ifndef DEV_SONAR_H
#define DEV_SONAR_H


#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx.h"

typedef uint8_t Dev_SonarId_t;

#define DEV_SONAR_ID_INVALID \
    ((Dev_SonarId_t)0xFFU)

typedef struct
{
    uint16_t distance_mm;     /* Khoang cach theo mm. */
    uint32_t pulse_width_us;  /* Do rong xung Echo. */
    bool valid;               /* Ket qua Echo hop le. */
} Dev_SonarMeasurement_t;

/* MẶT NẠ CHỌN CẢM BIẾN CẦN QUÉT                                             */

#define DEV_SONAR_MASK(sensor_id) \
    ((uint8_t)(1UL << (uint32_t)(sensor_id)))


/* API KHỞI TẠO, ĐĂNG KÝ VÀ XỬ LÝ                                            */


/* Khoi tao timer va xoa bang dang ky. */
void Dev_Sonar_Init(void);

/* Dang ky mot cap Trigger/Echo va tra ve ID. */
Dev_SonarId_t Dev_Sonar_Add(
    GPIO_TypeDef *trigger_port,
    uint8_t trigger_pin,
    GPIO_TypeDef *echo_port,
    uint8_t echo_pin);

/* Chay scheduler va xu ly timeout khong blocking. */
void Dev_Sonar_Process(void);

/* Chon cac ID duoc phep quet. */
void Dev_Sonar_SetScanMask(uint8_t scan_mask);

/* Doc scan mask hien tai. */
uint8_t Dev_Sonar_GetScanMask(void);

/* Lay mask cua tat ca cam bien da dang ky. */
uint8_t Dev_Sonar_GetRegisteredMask(void);

/* Lay so cam bien da dang ky. */
uint8_t Dev_Sonar_GetSensorCount(void);


/* API ĐỌC KẾT QUẢ */


/* Doc ket qua gan nhat, khong xoa co new_data. */
bool Dev_Sonar_GetLatest(
    Dev_SonarId_t sensor_id,
    Dev_SonarMeasurement_t *measurement);

/* Doc ket qua moi va xoa co new_data. */
bool Dev_Sonar_GetNewData(
    Dev_SonarId_t sensor_id,
    Dev_SonarMeasurement_t *measurement);

#endif /* DEV_SONAR_H */

