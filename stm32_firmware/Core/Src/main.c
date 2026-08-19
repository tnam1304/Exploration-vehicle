/**
 * @file    main.c
 * @brief   Chương trình chính điều khiển xe thám hiểm đa chức năng STM32F401RCT6
 */

#include "stm32f4xx.h"
#include "bsp_pinout.h"
#include "dev_oled.h"
#include "dev_ds18b20.h"
#include "dev_encoder.h"
#include "dev_motor.h"
#include "protocol_uart.h"
#include "app_safety.h"
#include "app_control.h"
#include "app_display.h"
#include "app_pid.h"
#include "odometry.h"
#include "app_buzzer.h"
#include "app_ra_control.h"
#include "app_ra_config.h"

#include <stddef.h>

/* ========================================================================== */
/* THÔNG SỐ CHU KỲ NHIỆM VỤ & HẰNG SỐ CẤU HÌNH (TASK & SYSTEM CONSTANTS)      */
/* ========================================================================== */
#define TASK_PID_PERIOD_US          50000       /* Chu kỳ Task PID & Odometry (50ms = 20Hz) */
#define TASK_RA_SNAPSHOT_US         40000       /* Chu kỳ chép khoảng cách front cho code nhóm */
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

/* Ngưỡng cảnh báo nhiệt độ của code nhóm */
#define TEMP_WARN_LIMIT_C           45          /* Ngưỡng cảnh báo nhiệt độ cao (°C) */

/* ========================================================================== */
/* HÀM GHÉP REVERSE ASSIST VỚI CODE ĐIỀU KHIỂN CỦA NHÓM                     */
/* ========================================================================== */

/** @brief Đổi lệnh điều khiển hiện tại sang hướng mà Reverse Assist hiểu. */
static RA_Direction_t Main_GetRADirection(char command)
{
    if (command == 'F')
    {
        return RA_DIR_FORWARD;
    }
    if (command == 'B')
    {
        return RA_DIR_REVERSE;
    }
    if ((command == 'L') || (command == 'R'))
    {
        return RA_DIR_OTHER;
    }
    return RA_DIR_STOPPED;
}

/**
 * @brief Kiểm tra cơ bản dữ liệu tốc độ của Odometry.
 *
 * Project nhóm chưa có cờ lỗi phần cứng encoder riêng. Vì vậy hàm chỉ loại
 * NaN và giá trị vượt giới hạn vật lý; hạn chế này được ghi rõ trong README.
 */
static bool Main_IsEncoderDataValid(void)
{
    const float left_mps = g_odometry.speed_left_cms / 100.0f;
    const float right_mps = g_odometry.speed_right_cms / 100.0f;
    const float left_abs = (left_mps < 0.0f) ? -left_mps : left_mps;
    const float right_abs = (right_mps < 0.0f) ? -right_mps : right_mps;

    return (left_mps == left_mps) &&
           (right_mps == right_mps) &&
           (left_abs <= VEH_ENCODER_MAX_SPEED_MPS) &&
           (right_abs <= VEH_ENCODER_MAX_SPEED_MPS);
}

/** @brief Đưa hướng, tốc độ hai bánh và tốc độ yêu cầu vào Reverse Assist. */
static void Main_UpdateRAInput(void)
{
    const RA_Direction_t direction = Main_GetRADirection(drive_cmd);
    float commanded_speed_mps = 0.0f;

    if ((direction == RA_DIR_FORWARD) || (direction == RA_DIR_REVERSE))
    {
        commanded_speed_mps = TARGET_SPEED_FWD / 100.0f;
    }

    RA_Control_SetMotionInput(
        direction,
        g_odometry.speed_left_cms / 100.0f,
        g_odometry.speed_right_cms / 100.0f,
        Main_IsEncoderDataValid(),
        commanded_speed_mps);
}

/** @brief Dừng ngay hai motor và xóa tích lũy PID trong chu kỳ hiện tại. */
static void Main_StopMotorImmediately(void)
{
    pid_left.target = 0.0f;
    pid_right.target = 0.0f;
    PID_Reset(&pid_left);
    PID_Reset(&pid_right);
    Motor_Left_SetSpeed(0);
    Motor_Right_SetSpeed(0);
}

/**
 * @brief Áp dụng target của Reverse Assist lên target PID khi xe tiến.
 *
 * PRE_BRAKE và BOOST chỉ đổi vận tốc mục tiêu. Đây không phải khẳng định vận
 * tốc thật giảm/tăng tuyến tính; PID và phản hồi encoder mới quyết định đáp ứng.
 */
static void Main_ApplyRATarget(const RA_Output_t *ra_output)
{
    if (ra_output == NULL)
    {
        Main_StopMotorImmediately();
        return;
    }

    if ((ra_output->motor_action == RA_MOTOR_STOP) ||
        (ra_output->motor_action == RA_MOTOR_EMERGENCY_BRAKE) ||
        (ra_output->motor_action == RA_MOTOR_AEB_HOLD))
    {
        Main_StopMotorImmediately();
        return;
    }

    if (drive_cmd == 'F')
    {
        const float target_cms = ra_output->target_speed_mps * 100.0f;
        pid_left.target = target_cms;
        pid_right.target = target_cms;
    }
}

/* ========================================================================== */
/* CHƯƠNG TRÌNH CHÍNH (MAIN FUNCTION)                                         */
/* ========================================================================== */

int main(void) {
    RA_Output_t ra_output;
    SonarData_t front_sonar;

    /* 1. Khởi tạo hạ tầng phần cứng BSP và Driver ngoại vi */
    Timer2_Init();
    Peripherals_Init();
    Encoder_Init();
    Odometry_Init();
    OLED_Init();
    OLED_Clear();

    /* Reverse Assist dùng sonar không blocking và không sửa file ngắt CubeMX. */
    (void)RA_Control_Init();
    RA_Buzzer_Init();

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

        /* Sonar và state machine phải chạy liên tục để AEB phản ứng sớm. */
        Main_UpdateRAInput();
        RA_Control_Process();
        (void)RA_Control_GetSafetyOutput(&ra_output);

        /* AEB/STOP được áp xuống motor ngay, không đợi đến task PID 50 ms. */
        if ((ra_output.motor_action == RA_MOTOR_STOP) ||
            (ra_output.motor_action == RA_MOTOR_EMERGENCY_BRAKE) ||
            (ra_output.motor_action == RA_MOTOR_AEB_HOLD))
        {
            Main_StopMotorImmediately();
        }

        RA_Buzzer_Process(current_time, &ra_output);
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

            /* Dùng mẫu tốc độ mới nhất để cập nhật ngưỡng động FCW/AEB. */
            Main_UpdateRAInput();
            RA_Control_Process();
            (void)RA_Control_GetSafetyOutput(&ra_output);

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

                /* Front Safety có ưu tiên cao hơn target tiến mặc định. */
                Main_ApplyRATarget(&ra_output);

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
        /* TASK 2: CHÉP KHOẢNG CÁCH FRONT CHO TELEMETRY/OLED CŨ (40MS)       */
        /* ------------------------------------------------------------------ */
        if ((current_time - last_fast_task) >= TASK_RA_SNAPSHOT_US) {
            last_fast_task = current_time;

            if (RA_Control_GetSonarData(SONAR_FRONT, &front_sonar) &&
                front_sonar.valid)
            {
                distance_cm = (uint32_t)front_sonar.filtered_distance_cm;
            }
            else
            {
                distance_cm = SONAR_INIT_DIST_CM;
            }
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
            else if (warn_code == SAFETY_WARN_FIRE)
            {
                /* Chỉ xóa cảnh báo cháy; còi Reverse Assist tự điều phối. */
                warn_code = SAFETY_WARN_NONE;
            }

            /* Gửi dữ liệu Telemetry về máy tính/App qua UART */
            UART_Send_Telemetry(raw_temp, distance_cm, warn_code,
                                Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                                g_odometry.total_distance_cm,
                                (int)ra_output.state,
                                (int)ra_output.rear_collision_state);

            /* Cập nhật thông số vận hành lên màn hình OLED */
            App_Display_Render(wifi_status_online, distance_cm, temp_c, warn_code,
                               Encoder_GetCount_Left(), g_odometry.speed_left_cms,
                               g_odometry.total_distance_cm,
                               ra_output.state,
                               ra_output.rear_collision_state);
        }
    }
}
