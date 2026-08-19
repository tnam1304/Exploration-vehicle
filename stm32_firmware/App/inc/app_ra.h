/**
 * @file app_ra.h
 * @brief Giao diện điều phối các chức năng an toàn của xe.
 *
 * File này chỉ chứa kiểu dữ liệu dùng chung và API điều phối. Thuật toán tiến,
 * cảnh báo lùi và va chạm sau nằm trong các module riêng.
 */

#ifndef APP_RA_H
#define APP_RA_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Hướng chuyển động hiện tại. */
typedef enum
{
    RA_DIR_STOPPED = 0,
    RA_DIR_FORWARD,
    RA_DIR_REVERSE,
    RA_DIR_OTHER /* Xe đang quay trái/phải: chỉ giám sát, không can thiệp. */
} RA_Direction_t;

/** @brief Mức cảnh báo gửi cho tầng hiển thị. */
typedef enum
{
    RA_WARN_NONE = 0,
    RA_WARN_ACTIVE,
    RA_WARN_DANGER
} RA_Warning_t;

/** @brief Trạng thái chính của xe. */
typedef enum
{
    RA_STATE_STOPPED = 0,

    FS_STATE_SAFE,
    FS_STATE_FCW,
    FS_STATE_PRE_BRAKE,
    FS_STATE_AEB,
    FS_STATE_AEB_HOLD,

    RW_STATE_SAFE,
    RW_STATE_WARNING,
    RW_STATE_DANGER,

    RA_STATE_SENSOR_INVALID,
    RA_STATE_ENCODER_INVALID,
    RA_STATE_CONFIG_INVALID
} RA_State_t;

/** @brief Hành động motor mà tầng tích hợp phải thực hiện. */
typedef enum
{
    RA_MOTOR_BYPASS = 0, /* Không ghi đè lệnh motor của chức năng khác. */
    RA_MOTOR_STOP,
    RA_MOTOR_RUN,
    RA_MOTOR_DECEL_FF,
    RA_MOTOR_ACCEL_FF,
    RA_MOTOR_EMERGENCY_BRAKE,
    RA_MOTOR_AEB_HOLD
} RA_MotorAction_t;

/** @brief Trạng thái phát hiện vật phía sau đang tiến gần. */
typedef enum
{
    RC_STATE_INACTIVE = 0,
    RC_STATE_WARMUP,
    RC_STATE_SAFE,
    RC_STATE_APPROACHING,
    RC_STATE_WARNING,
    RC_STATE_DANGER,
    RC_STATE_INVALID
} RC_State_t;

/** @brief Phía có nguy cơ va chạm cao hơn. */
typedef enum
{
    RC_SIDE_NONE = 0,
    RC_SIDE_LEFT,
    RC_SIDE_RIGHT,
    RC_SIDE_BOTH
} RC_Side_t;

/** @brief Dữ liệu đầu vào sau khi encoder và sonar đã được xử lý. */
typedef struct
{
    RA_Direction_t direction;

    float left_speed_mps;  /* Vận tốc bánh trái từ Odometry. */
    float right_speed_mps; /* Vận tốc bánh phải từ Odometry. */
    bool encoder_valid;

    /* Vận tốc yêu cầu ước lượng từ lệnh PWM, không phải vận tốc đo thật. */
    float commanded_speed_mps;

    float front_distance_cm;
    float rear_left_distance_cm;
    float rear_right_distance_cm;

    bool front_distance_valid;
    bool rear_left_distance_valid;
    bool rear_right_distance_valid;

    /* Bộ đếm giúp xác nhận theo mẫu sonar mới, không theo tốc độ vòng lặp. */
    uint32_t front_sample_sequence;
    uint32_t rear_left_sample_sequence;
    uint32_t rear_right_sample_sequence;
} RA_Input_t;

/** @brief Kết quả chung cho motor và UART debug. */
typedef struct
{
    RA_State_t state;
    RA_Warning_t warning_level;
    RA_MotorAction_t motor_action;

    float vehicle_speed_mps;
    float commanded_speed_mps;
    float target_speed_mps;

    float aeb_threshold_cm;
    float pre_brake_threshold_cm;
    float fcw_threshold_cm;

    float nearest_rear_distance_cm;
    bool nearest_rear_distance_valid;

    RC_State_t rear_collision_state;
    RC_Side_t rear_collision_side;

    float rear_left_closing_speed_cm_s;
    float rear_right_closing_speed_cm_s;
    float rear_left_ttc_s;
    float rear_right_ttc_s;

    bool rear_left_ttc_valid;
    bool rear_right_ttc_valid;
    bool rear_collision_valid;
    bool rear_boost_active;

    float rear_boost_required_front_cm;

    bool encoder_valid;
    bool sonar_valid;
    bool config_valid;
    bool request_motor_stop;
    bool request_emergency_brake;
} RA_Output_t;

/** @brief Xóa các state machine và khởi tạo đầu ra. */
void RA_Init(RA_Output_t *output);

/** @brief Ghi nhận một lệnh tiến mới để xin nhả AEB_HOLD. */
void RA_RequestForwardRestart(void);

/**
 * @brief Chọn vận tốc đại diện của xe từ hai bánh.
 *
 * Không lấy trung bình. Giá trị lớn hơn được dùng để ngưỡng FCW/AEB bảo thủ,
 * tránh đánh giá thấp tốc độ khi một bánh bị trượt hoặc encoder hụt xung.
 */
float RA_GetVehicleSpeedMps(float left_speed_mps, float right_speed_mps);

/** @brief Chạy một chu kỳ điều phối an toàn. */
void RA_Process(
    const RA_Input_t *input,
    RA_Output_t *output);

#endif /* APP_RA_H */
