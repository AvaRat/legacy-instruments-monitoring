#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

#define CAMERA_MODEL_AI_THINKER
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



void init_camera(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
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
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA; //UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  pinMode(33, OUTPUT);
}

void deinit_camera() {
  long millisstart = millis();
  uint32_t heapstart = ESP.getFreeHeap();

  esp_err_t err2 = esp_camera_deinit();
  if (err2 != ESP_OK) {
    Serial.printf("Camera deinit failed with error 0x % x", err2);
    return;
  }
  Serial.printf("Camera deinit % d ms, saving % d bytes\n", millis() - millisstart, - heapstart + ESP.getFreeHeap() );
  Serial.printf("IP %s, Rssi %d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

camera_fb_t * get_good_jpeg() {

  camera_fb_t * fb;
  int failures = 0;

  do {
    long start = millis();
    fb = esp_camera_fb_get();
    Serial.printf(" --%4d ms-- ", millis() - start);
    if (!fb) {
      Serial.printf("Camera Capture Failed %d ", failures);
      failures++;
      sensor_t * ss = esp_camera_sensor_get();
      int qual = ss->status.quality ;
      ss->set_quality(ss, qual + 2);
      Serial.printf("Lower the quality to %d\n", qual + 2);
      delay(100);
    } else {
      Serial.printf("%6d bytes\n", fb->len);
      break;
    }
  } while (failures < 5);   // normally leave the loop with a break()
  return fb;
}