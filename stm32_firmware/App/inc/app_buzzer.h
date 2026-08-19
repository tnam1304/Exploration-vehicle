/* Dieu phoi coi giua canh bao cua nhom va Reverse Assist. */

#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include "app_ra.h"

#include <stdint.h>

/* Tat coi va xoa moc nhip khi khoi dong. */
void RA_Buzzer_Init(void);

/* Cap nhat coi khong blocking theo trang thai an toan hien tai. */
void RA_Buzzer_Process(uint32_t now_us, const RA_Output_t *output);

#endif /* APP_BUZZER_H */
