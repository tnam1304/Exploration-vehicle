#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "stdint.h"
#include "math.h"

typedef struct {
    float x;     //hoành độ
    float y;     //tung độ
    float theta; //hướng di chuyển
} Pose2D_t;

typedef struct {
    float wheel_base;				//khoảng cách 2 bánh
    float wheel_circumference;		//chu vi bánh
    int32_t ticks_per_rev;			//tổng số xung/1 vòng
    Pose2D_t current_pose;			//tọa độ
} OdometryTracker;

/**
 * @brief Khởi tạo các thông số hình học và trạng thái ban đầu cho bộ Odometry.
 * @param odom Con trỏ trỏ tới đối tượng quản lý OdometryTracker cần khởi tạo.
 * @param base Khoảng cách thực tế giữa hai bánh xe.
 * @param circ Chu vi của bánh xe.
 * @param tpr Số xung encoder trên một vòng quay trọn vẹn.
 */
void Odometry_Init(OdometryTracker *odom, float base, float circ, int32_t tpr);

/**
 * @brief Cập nhật tọa độ không gian dựa trên số xung encoder thay đổi (Delta).
 * @details Hàm này được gọi định kỳ trong vòng lặp hoặc ngắt, tính toán quãng đường
 *          dịch chuyển của từng bánh để suy ra độ dịch chuyển (x, y, theta) mới.
 * @param odom Con trỏ trỏ tới đối tượng OdometryTracker.
 * @param delta_left Số xung thay đổi của bánh trái từ lần đọc trước.
 * @param delta_right Số xung thay đổi của bánh phải từ lần đọc trước.
 */
void Odometry_Update(OdometryTracker *odom, int32_t delta_left, int32_t delta_right);

/**
 * @brief Truy xuất tọa độ không gian hiện tại của robot.
 * @param odom Con trỏ trỏ tới đối tượng OdometryTracker.
 * @return Pose2D_t Trả về cấu trúc chứa tọa độ (x, y, theta) hiện tại.
 */
Pose2D_t Odometry_GetPose(OdometryTracker *odom);

/**
 * @brief Đặt lại mốc tọa độ không gian của robot về gốc ban đầu (0, 0, 0).
 * @param odom Con trỏ trỏ tới đối tượng OdometryTracker.
 */
void Odometry_Reset(OdometryTracker *odom);

#endif
