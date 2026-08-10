/**
 * @file Xe_tham_hiem_v2.ino
 * @brief Hệ thống điều khiển Xe Tự Hành IoT qua mạng Wi-Fi
 * 
 * @details 
 * Cung cấp giải pháp truyền tải Video (MJPEG Stream) thời gian thực, 
 * Server điều khiển HTTP API và giao tiếp đồng bộ UART với vi điều khiển STM32.
 * Tích hợp cơ chế bảo vệ Failsafe và cấp phát tĩnh (Static Memory Allocation) 
 * để tối ưu hóa bộ nhớ cho vi điều khiển ESP32.
 */

#include <WiFi.h>
#include "esp_http_server.h"
#include "soc/soc.h"           // Thư viện can thiệp Brownout
#include "soc/rtc_cntl_reg.h"  // Thư viện disable Brownout detector

 * IMPORT MODULES LIÊN KẾT
#include "WebUI.h"
#include "CameraConfig.h"

/* 
 * CẤU HÌNH THÔNG TẤN MẠNG (ACCESS POINT MODE)
 */
const char* ssid     = "Xe_Tham_Hiem";
const char* password = "88888888";

/*
 * GIAO THỨC TRUYỀN PHÁT VIDEO (MJPEG STREAMING PROTOCOL)
 */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/*
 * BIẾN TOÀN CỤC (GLOBAL TELEMETRY DATA)
*/
volatile int   temp_val    = 0;
volatile int   dist_val    = 0;
volatile int   warn_val    = 0; 
volatile long  enc_val     = 0;
volatile float speed_val   = 0.0;
volatile float travel_dist = 0.0;

volatile char current_cmd   = 'S';
volatile int  current_speed = 80;

// Bộ đếm thời gian cho cơ chế Failsafe Watchdog
volatile unsigned long last_heartbeat_time = 0; 

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

/* 
 * API HANDLERS (ĐIỀU HƯỚNG HTTP REQUEST)
 */

/**
 * @brief Tải giao diện WebUI (Frontend)
 */
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, MAIN_page, strlen(MAIN_page));
}

/**
 * @brief Trả về dữ liệu Telemetry chuẩn JSON
 */
static esp_err_t data_handler(httpd_req_t *req) {
  last_heartbeat_time = millis(); // Refresh Heartbeat
  char json[200];
  snprintf(json, sizeof(json), "{\"temp\":%d, \"dist\":%d, \"warn\":%d, \"enc\":%ld, \"spd\":%.1f, \"trav\":%.1f}", 
           temp_val, dist_val, warn_val, enc_val, speed_val, travel_dist);
           
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, json, strlen(json));
}

/**
 * @brief Xử lý lệnh điều hướng. Chuyển tiếp ngay lập tức qua UART (Độ trễ <1ms)
 */
static esp_err_t cmd_handler(httpd_req_t *req) {
  last_heartbeat_time = millis();
  char buf[32];
  
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char val[8];
    if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
      char new_cmd = val[0];
      
      // Gửi UART trực tiếp khi có sự thay đổi trạng thái hoặc phanh khẩn cấp
      if (current_cmd != new_cmd || new_cmd == 'S') {
        current_cmd = new_cmd;
        Serial.print(current_cmd); 
      }
    }
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, "OK", 2);
}

/**
 * @brief Ghi nhận mức công suất/tốc độ mong muốn
 */
static esp_err_t speed_handler(httpd_req_t *req) {
  last_heartbeat_time = millis();
  char buf[32];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char val[8];
    if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
      current_speed = atoi(val);
      Serial.printf("V:%d\n", current_speed); 
    }
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, "OK", 2);
}

/**
 * @brief Bật/Tắt module Flash LED định hướng
 */
static esp_err_t flash_handler(httpd_req_t *req) {
  last_heartbeat_time = millis();
  char buf[32];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char val[8];
    if (httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
      int state = atoi(val);
      digitalWrite(FLASH_GPIO_NUM, state ? HIGH : LOW);
    }
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_send(req, "OK", 2);
}

/**
 * @brief Khởi tạo chuỗi Frame cho Video Streaming MJPEG
 */
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) { 
      res = ESP_FAIL; 
    } else {
      size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
      if (res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
      esp_camera_fb_return(fb);
    }
    
    if (res != ESP_OK) break;
    vTaskDelay(10 / portTICK_PERIOD_MS); // Giải phóng CPU để xử lý Thread khác
  }
  return res;
}

/**
 * @brief Đăng ký các Route API và khởi chạy Web Server
 */
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  
  // Server cấu hình dữ liệu (Port 80)
  config.server_port = 80;
  httpd_uri_t index_uri = { .uri = "/",      .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL };
  httpd_uri_t data_uri  = { .uri = "/data",  .method = HTTP_GET, .handler = data_handler,  .user_ctx = NULL };
  httpd_uri_t cmd_uri   = { .uri = "/cmd",   .method = HTTP_GET, .handler = cmd_handler,   .user_ctx = NULL };
  httpd_uri_t speed_uri = { .uri = "/speed", .method = HTTP_GET, .handler = speed_handler, .user_ctx = NULL };
  httpd_uri_t flash_uri = { .uri = "/flash", .method = HTTP_GET, .handler = flash_handler, .user_ctx = NULL };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &data_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &speed_uri);
    httpd_register_uri_handler(camera_httpd, &flash_uri);
  }

  // Server chuyên dụng cho Video Streaming (Port 81)
  config.server_port = 81;
  config.ctrl_port = 81;
  httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
  
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

/*
 * MODULE GIAO TIẾP NGOẠI VI (UART COMMUNICATION)
 */

/**
 * @brief Quét chuỗi Telemetry từ STM32 gửi lên
 * Sử dụng kỹ thuật Static Memory Allocation (C-string) để tránh 
 * hiện tượng phân mảnh Heap (Heap Fragmentation) khi hoạt động liên tục.
 */
void processSerialUART() {
  static char rx_buf[64];
  static int rx_idx = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n') {
      rx_buf[rx_idx] = '\0'; // Chốt kết thúc chuỗi
      
      if (rx_idx > 0) {
        int p_t = 0, p_d = 0, p_w = 0, p_s = 0, p_tr = 0;
        long p_e = 0;
        
        int parsed = sscanf(rx_buf, "T%d;D%d;W%d;E%ld;S%d;M%d", 
                            &p_t, &p_d, &p_w, &p_e, &p_s, &p_tr);
                            
        if (parsed >= 3) {
          temp_val = p_t;
          dist_val = p_d;
          warn_val = p_w;
          if (parsed >= 6) {
            enc_val     = p_e;
            speed_val   = p_s / 10.0;
            travel_dist = p_tr / 10.0;
          }
        }
      }
      rx_idx = 0; // Đặt lại con trỏ cho chu kỳ tiếp theo
    } 
    else if (c != '\r') {
      // Giới hạn kích thước mảng để phòng ngừa lỗi tràn bộ nhớ (Buffer Overflow)
      if (rx_idx < sizeof(rx_buf) - 1) {
        rx_buf[rx_idx++] = c;
      }
    }
  }
}

/*
 * KHỞI TẠO VÀ VÒNG LẶP CHÍNH (SETUP & LOOP)
 */

void setup() {
  // Tắt kiểm tra sụt áp phần cứng để tránh reset ngẫu nhiên do Motor
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  Serial.begin(9600);
  Serial.setTimeout(10);

  // Khởi tạo và báo lỗi nếu Camera gặp sự cố
  if (!initCameraHardware()) {
    Serial.println("LỖI: Không thể khởi tạo cảm biến Camera!");
    return;
  }

  // Khởi phát mạng WiFi nội bộ
  WiFi.softAP(ssid, password);
  startCameraServer();
}

void loop() {
  processSerialUART();

  /**
   * CƠ CHẾ AN TOÀN (FAILSAFE WATCHDOG)
   * Kích hoạt ép phanh phần cứng ngay lập tức nếu mất tín hiệu Web > 1.2s
   */
  if (current_cmd != 'S' && (millis() - last_heartbeat_time > 1200)) {
    current_cmd = 'S'; 
    Serial.print('S'); 
  }

  /**
   * DUY TRÌ KẾT NỐI (UART HEARTBEAT)
   * Đẩy dữ liệu chu kỳ 50ms xuống STM32 để bù trừ rớt tín hiệu (EMI Interference)
   */
  static unsigned long last_cmd_time = 0;
  if (millis() - last_cmd_time > 50) {
    Serial.print(current_cmd); 
    last_cmd_time = millis();
  }
  
  delay(10);
}