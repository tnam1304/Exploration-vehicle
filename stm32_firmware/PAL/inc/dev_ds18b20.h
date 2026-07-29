/**
 * @file    dev_ds18b20.h
 * @brief   Thu vien dieu khien cam bien nhiet do DS18B20 qua 1-Wire (Tang PAL)
 */

#ifndef __DEV_DS18B20_H
#define __DEV_DS18B20_H

#include "bsp_pinout.h"

/**
 * @brief Khoi tao va kiem tra su ton tai cua cam bien DS18B20
 * @return 1: Tim thay cam bien, 0: Loi
 */
uint8_t Dev_DS18B20_Init(void);

/**
 * @brief Doc gia tri nhiet do nguyen (do C) tu DS18B20
 * @return Gia tri nhiet do nguyen do C (int16_t)
 */
int16_t Dev_DS18B20_ReadTemp(void);

#endif /* __DEV_DS18B20_H */
