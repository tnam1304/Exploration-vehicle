#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_http_server.h"

// --- THÔNG TIN WIFI VÀ BROKER CLOUD ---
const char* ssid = "502";
const char* password = "88888888@";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* TOPIC_CONTROL = "rover/control";        //lệnh điều khiển
const char* TOPIC_TELEMETRY = "rover/telemetry";    //dữ liệu

WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình chân phần cứng cho ESP32-CAM
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

// Cấu hình định dạng luồng MJPEG Stream
#define PART_BOUNDARY "123456789000000000000987654321"      
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY; //luồng dữ liệu thay thế liên tục
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
httpd_handle_t stream_httpd = NULL;

// Hàm xử lý gửi các khung hình JPEG liên tục qua HTTP Server
static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);  //set kiểu (MJPEG  )
    if(res != ESP_OK) return res;

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            res = ESP_FAIL;
            break;
        } 
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if(!jpeg_converted){
                res = ESP_FAIL;
                break;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
    }
    return res;
}

// Khởi chạy Web Server cho Camera ở cổng 81
void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

// Hàm nhận bản tin MQTT từ Web và chuyển tiếp xuống STM32 qua UART
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String commandStr = "";
    for (unsigned int i = 0; i < length; i++) {
        commandStr += (char)payload[i];
    }

    if (String(topic) == TOPIC_CONTROL) {
        // Gửi lệnh điều khiển ('F', 'B', 'L', 'R', 'S', 'P') xuống STM32 qua Serial2
        Serial2.print(commandStr);
    }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // UART kết nối STM32 (RX=16, TX=17)

  // Cấu hình thông số cảm biến OV2640 của Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.frame_size = FRAMESIZE_QVGA; // Độ phân giải QVGA để stream mượt mà
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Khởi tạo Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_ERR_OK) {
    return;
  }

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Cấu hình MQTT Broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Khởi động HTTP Stream Server
  startCameraServer();
}

void loop() {
  // Duy trì kết nối MQTT Cloud
  if (!client.connected()) {
    while (!client.connected()) {
      if (client.connect("ESP32CAM_Client")) {
        client.subscribe(TOPIC_CONTROL);
      } else {
        delay(5000);
      }
    }
  }
  client.loop();

  // Đọc dữ liệu phản hồi từ STM32 gửi lên để đẩy ngược lên MQTT Cloud (nếu cần)
  if (Serial2.available()) {
    String telemetry = Serial2.readStringUntil('\n');
    client.publish(TOPIC_TELEMETRY, telemetry.c_str());
  }
}