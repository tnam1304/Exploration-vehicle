/**
 * @file app_ra_control.h
 * @brief Tầng điều phối dữ liệu cảm biến, bộ lọc và module an toàn.
 *
 * RA_Control không phải AUTOSAR RTE. Đây là tầng ứng dụng viết tay để:
 * - Khởi tạo và đăng ký ba cảm biến siêu âm.
 * - Lấy mỗi mẫu khoảng cách mới đúng một lần từ driver.
 * - Lọc median riêng cho từng vị trí cảm biến.
 * - Đổi khoảng cách từ milimét sang centimét.
 * - Truyền khoảng cách trước và hai khoảng cách sau đã lọc vào RA_Core.
 *
 * Driver, bộ lọc và thuật toán an toàn vẫn độc lập; file này chỉ ghép dữ liệu.
 */

#ifndef APP_RA_CONTROL_H
#define APP_RA_CONTROL_H

#include "app_ra.h"

#include <stdbool.h>
#include <stdint.h>


/* VỊ TRÍ CẢM BIẾN TRÊN XE */


/**
 * @brief Vị trí vật lý của ba cảm biến đang lắp trên xe.
 */
typedef enum
{
    SONAR_FRONT = 0, /* Cảm biến giữa phía trước. */
    SONAR_REAR_L,    /* Cảm biến phía sau bên trái. */
    SONAR_REAR_R,    /* Cảm biến phía sau bên phải. */
    SONAR_COUNT      /* Tổng số vị trí, không dùng làm ID. */
} SonarPosition_t;


/* DỮ LIỆU KHOẢNG CÁCH SAU ĐIỀU PHỐI                                         */


/**
 * @brief Kết quả gần nhất của một vị trí cảm biến, đơn vị centimét.
 */
typedef struct
{
    float raw_distance_cm;      /* Khoảng cách thô vừa nhận từ driver. */
    float filtered_distance_cm; /* Khoảng cách sau median 5 mẫu. */
    bool valid;                 /* true nếu mẫu Echo gần nhất hợp lệ. */
    bool has_sample;            /* true sau khi cảm biến trả kết quả đầu tiên. */
    uint32_t sample_sequence;   /* Tăng một đơn vị cho mỗi Echo/timeout mới. */
} SonarData_t;


/* API KHỞI TẠO VÀ CHẠY ĐỊNH KỲ                                              */


/**
 * @brief Khởi tạo driver sonar, đăng ký ba cảm biến và xóa các bộ lọc.
 *
 * Cấu hình chân hiện tại:
 * - Trước giữa: Trigger PC0, Echo PC6.
 * - Sau trái: Trigger PC1, Echo PC7.
 * - Sau phải: Trigger PC2, Echo PC8.
 *
 * Phải gọi sau SystemClock_Config() và trước vòng lặp chính.
 *
 * @return true nếu cả ba cảm biến đăng ký thành công.
 */
bool RA_Control_Init(void);

/**
 * @brief Chạy driver, lấy mẫu mới, lọc median và cập nhật Reverse Assist.
 *
 * Phải gọi liên tục trong vòng lặp chính. Hàm không dùng HAL_Delay().
 */
void RA_Control_Process(void);


/* API CẬP NHẬT TRẠNG THÁI CHUYỂN ĐỘNG                                       */


/**
 * @brief Cập nhật hướng xe và vận tốc encoder cho Reverse Assist.
 *
 * Khi tiến hoặc đứng im, lịch quét là FRONT, FRONT, REAR_L, FRONT,
 * FRONT, REAR_R để ưu tiên cảm biến trước. Khi lùi, AppControl quét
 * tuần tự hai cảm biến sau. Cập nhật encoder cùng hướng không reset lịch quét.
 * Khi đổi hướng, dữ liệu của nhóm cảm biến mới được đánh dấu chưa hợp lệ cho
 * đến khi nhận kết quả mới, tránh dùng mẫu cũ.
 *
 * @param direction Hướng chuyển động hiện tại của xe.
 * @param left_speed_mps Vận tốc bánh trái từ Odometry, đơn vị m/s.
 * @param right_speed_mps Vận tốc bánh phải từ Odometry, đơn vị m/s.
 * @param encoder_valid true nếu mẫu tốc độ encoder dùng được.
 * @param commanded_speed_mps Vận tốc yêu cầu ước lượng, đơn vị m/s.
 */
void RA_Control_SetMotionInput(
    RA_Direction_t direction,
    float left_speed_mps,
    float right_speed_mps,
    bool encoder_valid,
    float commanded_speed_mps);


/* API ĐỌC KẾT QUẢ */


/**
 * @brief Đọc dữ liệu thô và dữ liệu đã lọc của một vị trí cảm biến.
 *
 * @param position Vị trí cảm biến cần đọc.
 * @param data Nơi nhận kết quả theo centimét.
 * @return false nếu vị trí hoặc con trỏ không hợp lệ.
 */
bool RA_Control_GetSonarData(
    SonarPosition_t position,
    SonarData_t *data);

/**
 * @brief Đọc kết quả an toàn gần nhất.
 *
 * @param output Nơi nhận trạng thái SAFE/FCW/AEB và các giá trị liên quan.
 * @return false nếu con trỏ đầu ra không hợp lệ.
 */
bool RA_Control_GetSafetyOutput(RA_Output_t *output);

/**
 * @brief Kiểm tra cả ba cảm biến đã đăng ký thành công hay chưa.
 *
 * @return true nếu cấu hình GPIO/EXTI hợp lệ cho cả ba cảm biến.
 */
bool RA_Control_IsReady(void);

#endif /* APP_RA_CONTROL_H */
