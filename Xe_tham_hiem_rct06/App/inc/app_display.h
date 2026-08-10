/**
 * @file    app_display.h
 * @brief   Tầng ứng dụng xử lý và định dạng dữ liệu hiển thị lên màn hình OLED
 */

#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================== */
/* HẰNG SỐ CẤU HÌNH HIỂN THỊ (DISPLAY CONFIGURATION CONSTANTS)               */
/* ========================================================================== */
#define DISPLAY_BUF_SIZE            20          /* Kích thước bộ đệm chuỗi in 1 dòng OLED */

/* Các mức cảnh báo hệ thống */
#define WARN_LEVEL_NONE             0           /* Mức an toàn */
#define WARN_LEVEL_OBSTACLE         1           /* Mức cảnh báo vật cản */
#define WARN_LEVEL_FIRE             2           /* Mức cảnh báo hỏa hoạn */

#define OBSTACLE_STOP_DIST_CM       10          /* Ngưỡng khoảng cách dừng xe khẩn cấp (cm) */
#define SONAR_ERROR_DIST_VAL        999         /* Giá trị lỗi từ cảm biến siêu âm */

/* Định nghĩa các trang hiển thị (Page 0 đến Page 7) trên OLED SSD1306 */
#define OLED_PAGE_WIFI              0           /* Trang 0: Trạng thái kết nối WiFi */
#define OLED_PAGE_STATUS            1           /* Trang 1: Cảnh báo an toàn / Vật cản */
#define OLED_PAGE_TEMP              2           /* Trang 2: Nhiệt độ môi trường */
#define OLED_PAGE_DIST              3           /* Trang 3: Khoảng cách vật cản phía trước */
#define OLED_PAGE_SPEED             4           /* Trang 4: Vận tốc di chuyển của xe */
#define OLED_PAGE_TRAVEL            5           /* Trang 5: Quãng đường di chuyển tích lũy */
#define OLED_PAGE_ENCODER           6           /* Trang 6: Số xung Encoder tích lũy */
#define OLED_PAGE_FOOTER            7           /* Trang 7: Đường kẻ phân cách chân trang */

/* ========================================================================== */
/* NGUYÊN MẪU HÀM (FUNCTION PROTOTYPES)                                       */
/* ========================================================================== */

/**
 * @brief Cập nhật và render toàn bộ thông tin vận hành lên 8 trang của OLED
 * @param wifi_online Trạng thái kết nối WiFi (1: Online, 0: Offline)
 * @param dist Khoảng cách đo từ cảm biến siêu âm (cm)
 * @param temp Nhiệt độ đo được (°C)
 * @param warn Cấp độ cảnh báo (0: Safe, 1: Obstacle, 2: Fire)
 * @param enc Số xung Encoder đếm được
 * @param speed Tốc độ hiện tại của xe (cm/s)
 * @param travel Quãng đường đã di chuyển tích lũy (cm)
 */
void App_Display_Render(uint8_t wifi_online, uint32_t dist, int16_t temp, 
                       int warn, int32_t enc, float speed, float travel);

#endif /* APP_DISPLAY_H */