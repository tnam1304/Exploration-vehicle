/**
 * @file    bsp_pinout.c
 * @brief   Triển khai khởi tạo ngoại vi và hàm trễ cho STM32F401RCT6
 */

#include "bsp_pinout.h"

void Timer2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = TIM2_PRESCALER_1US;
    TIM2->ARR = TIM2_ARR_MAX_32BIT;
    TIM2->EGR |= TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void Delay_us(uint32_t us) {
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us);
}

void Delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        Delay_us(1000);
    }
}

void Peripherals_Init(void) {
    /* Bật clock cho GPIOA và GPIOB */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    
    /* Bật clock cho I2C1, TIM3 và USART2 */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_USART2EN;

    /* Xóa cấu hình MODER các chân sử dụng trên GPIOA (2 bit mỗi chân) */
    GPIOA->MODER &= ~((3 << (DS18B20_PIN * 2))     | 
                      (3 << (USART2_TX_PIN * 2))   | 
                      (3 << (USART2_RX_PIN * 2))   | 
                      (3 << (MOTOR_PWM_A_PIN * 2)) | 
                      (3 << (MOTOR_PWM_B_PIN * 2)) | 
                      (3 << (MOTOR_AIN1_PIN * 2))  | 
                      (3 << (MOTOR_AIN2_PIN * 2))  | 
                      (3 << (MOTOR_BIN1_PIN * 2)));
    
    /* Xóa cấu hình MODER các chân sử dụng trên GPIOB (2 bit mỗi chân) */
    GPIOB->MODER &= ~((3 << (MOTOR_BIN2_PIN * 2))   | 
                      (3 << (BUZZER_PIN * 2))       | 
                      (3 << (HC_SR04_ECHO_PIN * 2)) | 
                      (3 << (MOTOR_STBY_PIN * 2))   | 
                      (3 << (OLED_SCL_PIN * 2))     | 
                      (3 << (OLED_SDA_PIN * 2))     | 
                      (3 << (HC_SR04_TRIG_PIN * 2)));

    /* ---------------------------------------------------------------------- */
    /* 1. OLED I2C1 (SCL: PB8, SDA: PB9)                                     */
    /* ---------------------------------------------------------------------- */
    GPIOB->MODER  |= (2 << (OLED_SCL_PIN * 2)) | (2 << (OLED_SDA_PIN * 2));     /* Alternate Function mode */
    GPIOB->OTYPER |= (1 << OLED_SCL_PIN)       | (1 << OLED_SDA_PIN);           /* Open-drain output      */
    GPIOB->PUPDR  |= (2 << (OLED_SCL_PIN * 2)) | (2 << (OLED_SDA_PIN * 2));     /* Pull-up enabled        */
    GPIOB->AFR[1] |= (4 << ((OLED_SCL_PIN - 8) * 4)) | (4 << ((OLED_SDA_PIN - 8) * 4)); /* AF4 cho I2C1 */

    /* Reset bus I2C1 */
    I2C1->CR1 |= I2C_CR1_SWRST;
    for (volatile int i = 0; i < 100; i++);
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* Thiết lập tần số bus I2C */
    I2C1->CR2   = I2C_CR2_16MHZ; 
    I2C1->CCR   = I2C_CCR_100KHZ; 
    I2C1->TRISE = I2C_TRISE_100KHZ; 
    I2C1->CR1  |= I2C_CR1_PE;

    /* ---------------------------------------------------------------------- */
    /* 2. Cảm biến siêu âm HC-SR04 (TRIG: PB10, ECHO: PB2)                   */
    /* ---------------------------------------------------------------------- */
    GPIOB->MODER |= (1 << (HC_SR04_TRIG_PIN * 2));     /* PB10 Output            */
    GPIOB->PUPDR |= (2 << (HC_SR04_ECHO_PIN * 2));     /* PB2 Pull-down          */

    /* ---------------------------------------------------------------------- */
    /* 3. Cảm biến nhiệt độ DS18B20 (DQ: PA1)                                */
    /* ---------------------------------------------------------------------- */
    GPIOA->MODER  |= (1 << (DS18B20_PIN * 2));         /* PA1 Output             */
    GPIOA->OTYPER |= (1 << DS18B20_PIN);               /* Open-drain             */
    GPIOA->PUPDR  |= (1 << (DS18B20_PIN * 2));         /* Pull-up                */

    /* ---------------------------------------------------------------------- */
    /* 4. Còi báo Buzzer (PB1)                                                */
    /* ---------------------------------------------------------------------- */
    GPIOB->MODER |= (1 << (BUZZER_PIN * 2));           /* PB1 Output             */

    /* ---------------------------------------------------------------------- */
    /* 5. Mạch điều khiển động cơ TB6612FNG                                  */
    /* ---------------------------------------------------------------------- */
    GPIOB->MODER |= (1 << (MOTOR_STBY_PIN * 2)) | (1 << (MOTOR_BIN2_PIN * 2));
    GPIOA->MODER |= (1 << (MOTOR_BIN1_PIN * 2)) | (1 << (MOTOR_AIN1_PIN * 2)) | (1 << (MOTOR_AIN2_PIN * 2));
    GPIOB->BSRR   = (1 << MOTOR_STBY_PIN);             /* Kích hoạt chân STBY high */

    /* Cấu hình PWM TIM3 (PA6 - CH1, PA7 - CH2) */
    GPIOA->MODER  |= (2 << (MOTOR_PWM_A_PIN * 2)) | (2 << (MOTOR_PWM_B_PIN * 2)); /* Alternate Function mode */
    GPIOA->AFR[0] |= (2 << (MOTOR_PWM_A_PIN * 4)) | (2 << (MOTOR_PWM_B_PIN * 4)); /* AF2 cho TIM3           */

    TIM3->PSC    = TIM3_PWM_PRESCALER; 
    TIM3->ARR    = TIM3_PWM_PERIOD_ARR;
    TIM3->CCMR1 |= (6 << 4) | (6 << 12);               /* PWM Mode 1 CH1 & CH2   */
    TIM3->CCER  |= TIM_CCER_CC1E | TIM_CCER_CC2E;      /* Enable Output CH1/CH2  */
    TIM3->CR1   |= TIM_CR1_CEN;                        /* Enable Counter         */

    /* ---------------------------------------------------------------------- */
    /* 6. Giao tiếp USART2 (TX: PA2, RX: PA3)                                */
    /* ---------------------------------------------------------------------- */
    GPIOA->MODER  |= (2 << (USART2_TX_PIN * 2)) | (2 << (USART2_RX_PIN * 2)); /* Alternate Function mode */
    GPIOA->AFR[0] |= (7 << (USART2_TX_PIN * 4)) | (7 << (USART2_RX_PIN * 4)); /* AF7 cho USART2         */

    USART2->BRR  = USART2_BRR_115200_16MHZ;
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
}
