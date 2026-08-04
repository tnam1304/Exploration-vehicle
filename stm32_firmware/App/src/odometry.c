#include <math.h>

// Định nghĩa cấu trúc lưu trạng thái vị trí Robot
typedef struct {
    float x;     // Tọa độ X (mét)
    float y;     // Tọa độ Y (mét)
    float theta; // Góc định hướng (Rad)

    // Biến lưu số xung encoder kỳ trước để tính delta
    int32_t last_enc_left;
    int32_t last_enc_right;
} Odometry_t;

// Các thông số
#define WHEEL_RADIUS       0.033f  // Bán kính bánh xe
#define TRACK_WIDTH        0.18f   // Khoảng cách giữa 2 bánh
#define COUNTS_PER_REVOLUTION 20.0f // Số xung encoder trong 1 vòng quay

Odometry_t robot_odom = {0, 0, 0, 0, 0};

/**
 * @brief Khởi tạo thông số ban đầu cho Odometry
 */

void Odometry_Init(void) {
    robot_odom.x = 0.0f;
    robot_odom.y = 0.0f;
    robot_odom.theta = 0.0f;
    robot_odom.last_enc_left = 0;
    robot_odom.last_enc_right = 0;
}

/**
 * @brief Hàm cập nhật Odometry
 * @param current_enc_left: Tổng số xung hiện tại của encoder bánh trái
 * @param current_enc_right: Tổng số xung hiện tại của encoder bánh phải
 */
void Odometry_Update(int32_t current_enc_left, int32_t current_enc_right) {
    // Tính số xung thay đổi (Delta pulses) từ kỳ trước đến nay
    int32_t delta_enc_L = current_enc_left - robot_odom.last_enc_left;
    int32_t delta_enc_R = current_enc_right - robot_odom.last_enc_right;

    // Cập nhật lại giá trị encoder cũ
    robot_odom.last_enc_left = current_enc_left;
    robot_odom.last_enc_right = current_enc_right;

    // Chuyển đổi từ số xung sang quãng đường tuyến tính của từng bánh
    float ds_l = (2.0f * M_PI * WHEEL_RADIUS * (float)delta_enc_L) / COUNTS_PER_REVOLUTION;
    float ds_r = (2.0f * M_PI * WHEEL_RADIUS * (float)delta_enc_R) / COUNTS_PER_REVOLUTION;

    // Tính quãng đường trung bình và góc xoay của robot
    float ds = (ds_r + ds_l) / 2.0f;
    float dtheta = (ds_r - ds_l) / TRACK_WIDTH;

    // Cập nhật tọa độ (x, y, theta) sử dụng phương pháp Runge-Kutta bậc 2 (Midpoint)
    // Giúp giảm sai số tích lũy khi robot chuyển động cong
    float phi_mid = robot_odom.theta + (dtheta / 2.0f);

    robot_odom.x += ds * cosf(phi_mid);
    robot_odom.y += ds * sinf(phi_mid);
    robot_odom.theta += dtheta;

    // Chuẩn hóa góc theta luôn nằm trong khoảng [-PI, PI]
    if (robot_odom.theta > M_PI)  robot_odom.theta -= (2.0f * M_PI);
    if (robot_odom.theta < -M_PI) robot_odom.theta += (2.0f * M_PI);
}
