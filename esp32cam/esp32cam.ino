#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_http_server.h"

// --- THÔNG TIN WIFI VÀ BROKER ---
const char* ssid = "502";
const char* password = "88888888@";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* TOPIC_CONTROL = "rover/control";        
const char* TOPIC_TELEMETRY = "rover/telemetry";    

WiFiClient espClient;
PubSubClient client(espClient);

// --- CẤU HÌNH CHÂN PHẦN CỨNG AI-THINKER ESP32-CAM ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- CẤU TRÚC FRAME NHỊ PHÂN (24 Bytes) ---
#pragma pack(push, 1)
typedef struct {
    uint8_t  header1;   // 0xAA
    uint8_t  header2;   // 0x55
    float    temp;
    float    dist;
    uint8_t  warn;
    float    x;
    float    y;
    float    theta;
    uint8_t  checksum;
} TelemetryFrame_t;
#pragma pack(pop)

// --- MJPEG STREAM SERVER ---
#define PART_BOUNDARY "123456789000000000000987654321"      
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY; 
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);  
    if(res != ESP_OK) return res;

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) { res = ESP_FAIL; break; } 
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if(!jpeg_converted){ res = ESP_FAIL; break; }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){ res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); }
        if(res == ESP_OK){ res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)); }

        if(fb){ esp_camera_fb_return(fb); fb = NULL; _jpg_buf = NULL; } 
        else if(_jpg_buf){ free(_jpg_buf); _jpg_buf = NULL; }
        if(res != ESP_OK) break;
    }
    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    httpd_uri_t stream_uri = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

// Hàm nhận lệnh từ Web qua MQTT và đẩy xuống STM32 qua UART
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (String(topic) == TOPIC_CONTROL) {
        Serial2.write(payload, length);
    }
}

// --- TASK RIÊNG BIỆT: CHUYÊN ĐỌC UART VÀ DUY TRÌ MQTT (Chạy độc lập trên Core 0) ---
void CommunicationTask(void * pvParameters) {
    while(1) {
        // Duy trì kết nối MQTT
        if (!client.connected()) {
            if (client.connect("ESP32CAM_Client_12345")) {
                client.subscribe(TOPIC_CONTROL);
            } else {
                vTaskDelay(3000 / portTICK_PERIOD_MS);
                continue;
            }
        }
        client.loop();

        // Đọc dữ liệu nhị phân từ UART2 và đẩy lên MQTT
        static uint8_t rx_buffer[sizeof(TelemetryFrame_t)];
        static int rx_index = 0;

        while (Serial2.available()) {
            uint8_t inByte = Serial2.read();
            if (rx_index == 0 && inByte != 0xAA) continue;                      
            if (rx_index == 1 && inByte != 0x55) { rx_index = 0; continue; }    

            rx_buffer[rx_index++] = inByte;

            if (rx_index == sizeof(TelemetryFrame_t)) {
                client.publish(TOPIC_TELEMETRY, rx_buffer, sizeof(TelemetryFrame_t));
                rx_index = 0;
            }
        }
        
        vTaskDelay(2 / portTICK_PERIOD_MS); // Nhường CPU tránh chiếm dụng 100%
    }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // UART kết nối STM32

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) return;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.setBufferSize(256); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  startCameraServer(); // HTTP Server chạy tự động ngầm

  // Khởi tạo FreeRTOS Task chạy riêng trên Core 0 chuyên xử lý UART/MQTT
  xTaskCreatePinnedToCore(
      CommunicationTask,   
      "CommTask",          
      4096,                
      NULL,                
      1,                   
      NULL,                
      0                    // Core 0 (Core 1 dành cho Wifi/Camera Server)
  );
}

void loop() {
  // Để trống vì mọi tác vụ đã chạy song song đa nhân qua FreeRTOS
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}