/**
 * @file    dev_ds18b20.c
 * @brief   Thuc thi giao trinh 1-Wire doc nhiet do tu DS18B20
 */

#include "dev_ds18b20.h"

/* Cac lenh tieu chuan cua DS18B20 */
#define DS18B20_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_CONVERT_TEMP    0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

/**
 * @brief Dat chan DS18B20 thanh Output
 */
static void DS18B20_SetPin_Output(void) {
    DS18B20_PORT->MODER &= ~(3 << (DS18B20_PIN * 2));
    DS18B20_PORT->MODER |=  (1 << (DS18B20_PIN * 2));
}

/**
 * @brief Dat chan DS18B20 thanh Input
 */
static void DS18B20_SetPin_Input(void) {
    DS18B20_PORT->MODER &= ~(3 << (DS18B20_PIN * 2));
}

/**
 * @brief Phat xung Reset va kiem tra phan hoi tu DS18B20
 * @return 1: Co phan hoi, 0: Khong co phan hoi
 */
static uint8_t DS18B20_Reset(void) {
    uint8_t response = 0;

    DS18B20_SetPin_Output();
    DS18B20_PORT->BSRR = (1 << (DS18B20_PIN + 16)); /* Keo LOW */
    BSP_Delay_us(480);

    DS18B20_SetPin_Input();                         /* Tha HIGH */
    BSP_Delay_us(80);

    if (!(DS18B20_PORT->IDR & (1 << DS18B20_PIN))) {
        response = 1;                               /* Truyen thanh cong */
    }

    BSP_Delay_us(400);
    return response;
}

/**
 * @brief Ghi 1 bit sang DS18B20
 */
static void DS18B20_WriteBit(uint8_t bit) {
    DS18B20_SetPin_Output();
    DS18B20_PORT->BSRR = (1 << (DS18B20_PIN + 16));

    if (bit) {
        BSP_Delay_us(10);
        DS18B20_SetPin_Input();
        BSP_Delay_us(55);
    } else {
        BSP_Delay_us(65);
        DS18B20_SetPin_Input();
        BSP_Delay_us(5);
    }
}

/**
 * @brief Doc 1 bit tu DS18B20
 */
static uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;

    DS18B20_SetPin_Output();
    DS18B20_PORT->BSRR = (1 << (DS18B20_PIN + 16));
    BSP_Delay_us(2);

    DS18B20_SetPin_Input();
    BSP_Delay_us(10);

    if (DS18B20_PORT->IDR & (1 << DS18B20_PIN)) {
        bit = 1;
    }

    BSP_Delay_us(50);
    return bit;
}

/**
 * @brief Ghi 1 Byte sang DS18B20
 */
static void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & 0x01);
        data >>= 1;
    }
}

/**
 * @brief Doc 1 Byte tu DS18B20
 */
static uint8_t DS18B20_ReadByte(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) {
            value |= (1 << i);
        }
    }
    return value;
}

/**
 * @brief Khoi tao DS18B20
 */
uint8_t Dev_DS18B20_Init(void) {
    return DS18B20_Reset();
}

/**
 * @brief Doc gia tri nhiet do do C tu DS18B20
 */
int16_t Dev_DS18B20_ReadTemp(void) {
    uint8_t temp_lsb, temp_msb;
    int16_t raw_temp;

    /* 1. Yeu cau cam bien do nhiet do */
    if (!DS18B20_Reset()) return -99;
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_TEMP);

    /* 2. Doc du lieu tu Scratchpad */
    if (!DS18B20_Reset()) return -99;
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);

    temp_lsb = DS18B20_ReadByte();
    temp_msb = DS18B20_ReadByte();

    /* 3. Tinh toan ra do C nguyen */
    raw_temp = (temp_msb << 8) | temp_lsb;
    return (int16_t)(raw_temp / 16);
}
