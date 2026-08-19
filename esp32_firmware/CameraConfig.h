/**
 * @file CameraConfig.h
 * @brief Định nghĩa ngoại vi và cấu hình phần cứng cho module ESP32-CAM (AI-Thinker)
 * 
 * Module này chịu trách nhiệm khởi tạo cảm biến OV2640, thiết lập độ phân giải,
 * quản lý bộ nhớ PSRAM và cấp phát các chân GPIO tương ứng.
 * 
 */

#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "esp_camera.h"

/* =========================================================================
 * ĐỊNH NGHĨA CHÂN NGOẠI VI (PINOUT) CHO BẢN MẠCH ESP32-CAM AI-THINKER
 */
#define FLASH_GPIO_NUM    4    // Chân điều khiển đèn Flash LED tích hợp
#define PWDN_GPIO_NUM     32   // Chân Power Down của cảm biến
#define RESET_GPIO_NUM    -1   // Chân Reset (kết nối trực tiếp vào nguồn)
#define XCLK_GPIO_NUM      0   // Chân xung nhịp ngoại
#define SIOD_GPIO_NUM     26   // Chân I2C SDA
#define SIOC_GPIO_NUM     27   // Chân I2C SCL

#define Y9_GPIO_NUM       35   // Dữ liệu ảnh D7
#define Y8_GPIO_NUM       34   // Dữ liệu ảnh D6
#define Y7_GPIO_NUM       39   // Dữ liệu ảnh D5
#define Y6_GPIO_NUM       36   // Dữ liệu ảnh D4
#define Y5_GPIO_NUM       21   // Dữ liệu ảnh D3
#define Y4_GPIO_NUM       19   // Dữ liệu ảnh D2
#define Y3_GPIO_NUM       18   // Dữ liệu ảnh D1
#define Y2_GPIO_NUM        5   // Dữ liệu ảnh D0
#define VSYNC_GPIO_NUM    25   // Xung đồng bộ mành (Vertical Sync)
#define HREF_GPIO_NUM     23   // Xung đồng bộ dòng (Horizontal Reference)
#define PCLK_GPIO_NUM     22   // Xung nhịp điểm ảnh (Pixel Clock)

/**
 * @brief Khởi tạo phần cứng Camera và cấu hình thông số nén ảnh.
 * @return true nếu khởi tạo thành công, false nếu thất bại.
 */
bool initCameraHardware() {
  camera_config_t config;
  
  // Cấu hình LED PWM cho xung nhịp hệ thống camera
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  
  // Map chân GPIO
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Định dạng nén tối ưu băng thông

  // Phân bổ bộ nhớ tùy thuộc vào việc PSRAM có sẵn hay không
  if (psramFound()) {
    config.frame_size   = FRAMESIZE_QVGA; // 320x240 để tối ưu tốc độ truyền
    config.jpeg_quality = 12;             // Chất lượng (0-63, nhỏ là nét)
    config.fb_count     = 2;              // Double Buffering
  } else {
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count     = 1;
  }

  // Thực thi hàm khởi tạo
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    return false;
  }

  // Tinh chỉnh định dạng hình ảnh (Xoay/Lật)
  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);    // Xoay dọc
    s->set_hmirror(s, 1);  // Xoay ngang
  }
  
  return true;
}

#endif // CAMERA_CONFIG_H