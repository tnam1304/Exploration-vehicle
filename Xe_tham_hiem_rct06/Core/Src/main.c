/**
 * @file    main.c
 * @brief   Chương trình chính điều khiển xe thám hiểm đa chức năng STM32F401RCT6
 */

#include "stm32f4xx.h"
#include "bsp_pinout.h"
#include "dev_oled.h"
#include "dev_sonar.h"
#include "dev_ds18b20.h"
#include "dev_encoder.h"
#include "dev_motor.h"
#include "protocol_uart.h"
#include "app_safety.h"
#include "app_control.h"
#include "app_display.h"
#include "app_pid.h"
#include "odometry.h"

/* ========================================================================== */
/* THÔNG SỐ CHU KỲ NHIỆM VỤ & HẰNG SỐ CẤU HÌNH (TASK & SYSTEM CONSTANTS)      */
/* ========================================================================== */
#define TASK_PID_PERIOD_US          50000       /* Chu kỳ Task PID & Odometry (50ms = 20Hz) */
#define TASK_SONAR_PERIOD_US        40000       /* Chu kỳ Task Cảm biến siêu âm (40ms = 25Hz) */
#define TASK_SLOW_PERIOD_US         800000      /* Chu kỳ Task Nhiệt độ & Display (800ms) */
#define WATCHDOG_TIMEOUT_US         500000      /* Thời gian chờ mất kết nối WiFi/App (500ms) */

/* Giới hạn dt chống sốc PID */
#define DT_MAX_LIMIT_SEC            0.06f       /* Khống chế dt tối đa do trễ hiển thị OLED */
#define DT_DEFAULT_SEC              0.05f       /* Giá trị dt chuẩn (50ms) */

/* Tốc độ đặt mục tiêu cho PID (cm/s) */
#define TARGET_SPEED_FWD            30.0f       /* Tốc độ tiến/lùi thẳng */
#define TARGET_SPEED_TURN           20.0f       /* Tốc độ quay vòng */

/* Tham số khởi tạo bộ điều khiển PID */
#define PID_KP_DEFAULT              10.0f
#define PID_KI_DEFAULT              2.0f
#define PID_KD_DEFAULT              0.0f
#define PID_MIN_PWM                -1000.0f
#define PID_MAX_PWM                 1000.0f

/* Tham số lọc Kalman nhiệt độ */
#define KALMAN_Q_TEMP               0.01f
#define KALMAN_R_TEMP               2.0f
#define KALMAN_P_TEMP               1.0f
#define KALMAN_INIT_TEMP            25.0f

/* Ngưỡng cảnh báo an toàn */
#define OBSTACLE_STOP_LIMIT_CM      10          /* Ngưỡng dừng khẩn cấp do vật cản (cm) */
#define TEMP_WARN_LIMIT_C           45          /* Ngưỡng cảnh báo nhiệt độ cao (°C) */

/* ========================================================================== */
/* CHƯƠNG TRÌNH CHÍNH (MAIN FUNCTION)                                         */
/* ========================================================================== */

int main(void) {
    /* 1. Khởi tạo hạ tầng phần cứng BSP và Driver ngoại vi */
    Timer2_Init();
    Peripherals_Init();
    Encoder_Init();
    Odometry_Init();
    OLED_Init();
    OLED_Clear();

    /* 2. Khởi tạo tham số cho bộ điều khiển PID hai bánh */
    PID_Init(&pid_left, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT, PID_MIN_PWM, PID_MAX_PWM);
    PID_Init(&pid_right, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT, PID_MIN_PWM, PID_MAX_PWM);

    /* 3. Khởi tạo bộ lọc Kalman 1D cho cảm biến nhiệt độ */
    Kalman1D_t kf_temp;
    Kalman1D_Init(&kf_temp, KALMAN_Q_TEMP, KALMAN_R_TEMP, KALMAN_P_TEMP, KALMAN_INIT_TEMP);

    int16_t raw_temp = TEMP_SENSOR_ERROR_RAW;
    int16_t temp_c = 0;

    /* 4. Khởi tạo mốc thời gian cho bộ lập lịch Task */
    uint32_t last_pid_task  = TIM2->CNT;
    uint32_t last_fast_task = TIM2->CNT;
    uint32_t last_slow_task = TIM2->CNT;
    last_cmd_time           = TIM2->CNT;

    /* Yêu cầu cảm biến nhiệt độ bắt đầu chuyển đổi giá trị đầu tiên */
    DS18B20_StartConversion();

    /* 5. Vòng lặp thực thi chính (Super Loop) */
    while (1) {
        uint32_t current_time = TIM2->CNT;

        /* ------------------------------------------------------------------ */
        /* TASK 1: PID & ODOMETRY CHU KỲ NỀN 50MS (20Hz)                     */
        /* ------------------------------------------------------------------ */
        if ((current_time - last_pid_task) >= TASK_PID_PERIOD_US) {
            float dt = (float)(current_time - last_pid_task) / 1000000.0f;
            last_pid_task = current_time;

            /* Chống sốc PID: Khống chế dt không vượt quá giới hạn cho phép */
            if (dt > DT_MAX_LIMIT_SEC) {
                dt = DT_DEFAULT_SEC;
            }

            /* 1.1 Cập nhật vận tốc và quãng đường tích lũy từ Encoder */
            Odometry_Update(dt);

            /* 1.2 Điều khiển động cơ qua PID (khi cờ pid_enable bật) */
            if (pid_enable == 1) {
                /* Cập nhật tốc độ mục tiêu theo lệnh di chuyển */
                if (drive_cmd == 'F') {
                    pid_left.target  =  TARGET_SPEED_FWD;
                    pid_right.target =  TARGET_SPEED_FWD;
                } else if (drive_cmd == 'B') {
                    pid_left.target  = -TARGET_SPEED_FWD;
                    pid_right.target = -TARGET_SPEED_FWD;
                } else if (drive_cmd == 'L') {
                    pid_left.target  = -TARGET_SPEED_TURN;
                    pid_right.target =  TARGET_SPEED_TURN;
                } else if (drive_cmd == 'R') {
                    pid_left.target  =  TARGET_SPEED_TURN;
                    pid_right.target = -TARGET_SPEED_TURN;
                } else {
                    pid_left.target  = 0.0f;
                    pid_right.target = 0.0f;
                }

                /* Tính toán và xuất tín hiệu điều khiển PWM */
                if (pid_left.target == 0.0f && pid_right.target == 0.0f) {
                    Motor_Left_SetSpeed(0);
                    Motor_Right_SetSpeed(0);
                    PID_Reset(&pid_left);
                    PID_Reset(&pid_right);
                } else {
                    float pwm_left  = PID_Compute(&pid_left, g_odometry.speed_left_cms, dt);
                    float pwm_right = PID_Compute(&pid_right, g_odometry.speed_right_cms, dt);
                    Motor_Left_SetSpeed((int16_t)pwm_left);
                    Motor_Right_SetSpeed((int16_t)pwm_right);
                }
            }
        }

        /* ------------------------------------------------------------------ */
        /* WATCHDOG: TỰ ĐỘNG DỪNG XE KHI MẤT KẾT NỐI ESP32 (500MS)            */
        /* ------------------------------------------------------------------ */
        if ((current_time - last_cmd_time) > WATCHDOG_TIMEOUT_US) {
            if (drive_cmd != 'S') {
                drive_cmd = 'S';
                pid_left.target = 0.0f;
                pid_right.target = 0.0f;
                PID_Reset(&pid_left);
                PID_Reset(&pid_right);
                Car_Stop();
            }
            wifi_status_online = 0;
        }

        /* ------------------------------------------------------------------ */
        /* TASK 2: ĐO KHOẢNG CÁCH SIÊU ÂM & CẢNH BÁO AN TOÀN (40MS)          */
        /* ------------------------------------------------------------------ */
        if ((current_time - last_fast_task) >= TASK_SONAR_PERIOD_US) {
            last_fast_task = current_time;
            distance_cm = Measure_Distance();

            /* Phát hiện vật cản quá gần -> Dừng xe và phát cảnh báo */
            if (distance_cm > 0 && distance_cm <= OBSTACLE_STOP_LIMIT_CM) {
                warn_code = SAFETY_WARN_OBSTACLE;
                pid_left.target = 0.0f;
                pid_right.target = 0.0f;
            } else {
                if (temp_c <= TEMP_WARN_LIMIT_C) {
                    warn_code = SAFETY_WARN_NONE;
                }
            }
            Update_Buzzer_State();
        }

        /* ------------------------------------------------------------------ */
        /* TASK 3: ĐỌC NHIỆT ĐỘ, NỘP TELEMETRY & HIỂN THỊ OLED (800MS)        */
        /* ------------------------------------------------------------------ */
        if ((current_time - last_slow_task) >= TASK_SLOW_PERIOD_US) {
            last_slow_task = current_time;

            /* Đọc kết quả nhiệt độ và bắt đầu chu kỳ đo mới */
            raw_temp = DS18B20_ReadRawTemp();
            DS18B20_StartConversion();

            if (raw_temp != TEMP_SENSOR_ERROR_RAW) {
                float current_temp_f = (float)raw_temp / TEMP_SCALE_FACTOR;
                temp_c = (int16_t)Kalman1D_Update(&kf_temp, current_temp_f);
            } else {
                temp_c = 0;
            }

            /* Cảnh báo sự cố quá nhiệt */
            if (temp_c > TEMP_WARN_LIMIT_C) {
                warn_code = SAFETY_WARN_FIRE;
                Update_Buzzer_State();
            }

            /* Gửi dữ liệu Telemetry về máy tính/App qua UART */
            UART_Send_Telemetry(raw_temp, distance_cm, warn_code, 
                                Encoder_GetCount_Left(), g_odometry.speed_left_cms, g_odometry.total_distance_cm);

            /* Cập nhật thông số vận hành lên màn hình OLED */
            App_Display_Render(wifi_status_online, distance_cm, temp_c, warn_code,
                               Encoder_GetCount_Left(), g_odometry.speed_left_cms, g_odometry.total_distance_cm);
        }
    }
}