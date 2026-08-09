#include "odometry.h"
#include <stddef.h> // Hỗ trợ NULL

void Odometry_Init(OdometryTracker *odom, float base, float circ, int32_t tpr) {
    if (odom == NULL) return;

    odom->wheel_base = base;
    odom->wheel_circumference = circ;
    odom->ticks_per_rev = tpr;

    odom->current_pose.x = 0.0f;
    odom->current_pose.y = 0.0f;
    odom->current_pose.theta = 0.0f;
}

void Odometry_Update(OdometryTracker *odom, int32_t delta_left, int32_t delta_right) {
    if (odom == NULL) return;

    // 1. Chuyển đổi từ số xung sang quãng đường tuyến tính của từng bánh (Mét)
    float ds_l = (odom->wheel_circumference * (float)delta_left) / (float)odom->ticks_per_rev;
    float ds_r = (odom->wheel_circumference * (float)delta_right) / (float)odom->ticks_per_rev;

    // 2. Tính quãng đường trung bình và góc dTheta
    float ds = (ds_r + ds_l) / 2.0f;
    float dtheta = (ds_r - ds_l) / odom->wheel_base;

    // 3. Dùng góc trung bình (Midpoint) để nội suy quỹ đạo cong, giảm sai số tích lũy
    float phi_mid = odom->current_pose.theta + (dtheta / 2.0f);

    odom->current_pose.x += ds * cosf(phi_mid);
    odom->current_pose.y += ds * sinf(phi_mid);
    odom->current_pose.theta += dtheta;

    // 4. Ép góc Theta luôn luôn nằm trong khoảng giới hạn [-PI đến PI]
    if (odom->current_pose.theta > M_PI)
        odom->current_pose.theta -= (2.0f * M_PI);
    if (odom->current_pose.theta < -M_PI)
        odom->current_pose.theta += (2.0f * M_PI);
}

Pose2D_t Odometry_GetPose(OdometryTracker *odom) {
    if (odom == NULL) {
        Pose2D_t empty = {0, 0, 0};
        return empty;
    }
    return odom->current_pose;
}

void Odometry_Reset(OdometryTracker *odom) {
    if (odom == NULL) return;
    odom->current_pose.x = 0.0f;
    odom->current_pose.y = 0.0f;
    odom->current_pose.theta = 0.0f;
}
