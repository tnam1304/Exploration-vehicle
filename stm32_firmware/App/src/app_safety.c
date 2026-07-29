/**
 * @file    app_safety.c
 * @brief   Thuc thi logic kiem tra nguong an toan va tra ve ma canh bao
 */

#include "app_safety.h"

/**
 * @brief Kiem tra tat ca cac nguong an toan cua he thong
 */
uint8_t App_Safety_Check(int16_t temp_c, uint32_t dist_cm) {
    /* 1. Uu tien cao nhat: Canh bao chay neu nhiet do vuot nguong */
    if (temp_c >= TEMP_FIRE_THRESHOLD) {
        return 2; /* Tra ve 2 -> Display: "!! ALARM CHAY !!" */
    }

    /* 2. Uu tien hai: Canh bao vat can qua gan (tru truong hop loi 999cm) */
    if ((dist_cm < DIST_STOP_THRESHOLD) && (dist_cm != 999)) {
        return 1; /* Tra ve 1 -> Display: "!! STOP VAT CAN " */
    }

    /* 3. Binh thuong: He thong an toan */
    return 0;     /* Tra ve 0 -> Display: "STATUS: SAFE    " */
}
