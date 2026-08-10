/**
 * @file    dev_ds18b20.c
 * @brief   Triển khai driver giao tiếp chuẩn 1-Wire cho cảm biến DS18B20
 */

#include "dev_ds18b20.h"
#include "bsp_pinout.h"

uint8_t DS18B20_Reset(void) {
    GPIOA->BSRR = (1 << (DS18B20_PIN + 16));    /* Kéo chân DQ xuống LOW */
    Delay_us(480);
    GPIOA->BSRR = (1 << DS18B20_PIN);           /* Thả chân DQ lên HIGH */
    Delay_us(80);
    
    /* Kiểm tra phản hồi Presence pulse từ DS18B20 */
    uint8_t presence = (GPIOA->IDR & (1 << DS18B20_PIN)) ? 0 : 1;
    Delay_us(400);
    
    return presence;
}

void DS18B20_WriteBit(uint8_t bit) {
    if (bit) {
        GPIOA->BSRR = (1 << (DS18B20_PIN + 16)); /* Kéo LOW trong 10us */
        Delay_us(10);
        GPIOA->BSRR = (1 << DS18B20_PIN);        /* Thả HIGH trong 55us */
        Delay_us(55);
    } else {
        GPIOA->BSRR = (1 << (DS18B20_PIN + 16)); /* Kéo LOW trong 65us */
        Delay_us(65);
        GPIOA->BSRR = (1 << DS18B20_PIN);        /* Thả HIGH trong 5us */
        Delay_us(5);
    }
}

void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & (1 << i));
    }
}

uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    
    GPIOA->BSRR = (1 << (DS18B20_PIN + 16));     /* Kéo LOW trong 2us để khởi tạo slot đọc */
    Delay_us(2);
    GPIOA->BSRR = (1 << DS18B20_PIN);            /* Thả HIGH trong 10us */
    Delay_us(10);
    
    if (GPIOA->IDR & (1 << DS18B20_PIN)) {
        bit = 1;
    }
    Delay_us(50);
    
    return bit;
}

uint8_t DS18B20_ReadByte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) { 
        if (DS18B20_ReadBit()) {
            data |= (1 << i); 
        }
    }
    return data;
}

void DS18B20_StartConversion(void) {
    if (DS18B20_Reset()) {
        DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
        DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
    }
}

int16_t DS18B20_ReadRawTemp(void) {
    if (!DS18B20_Reset()) {
        return DS18B20_ERROR_RAW_TEMP;
    }
    
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);
    
    uint8_t lsb = DS18B20_ReadByte();
    uint8_t msb = DS18B20_ReadByte();
    
    return (int16_t)((msb << 8) | lsb);
}