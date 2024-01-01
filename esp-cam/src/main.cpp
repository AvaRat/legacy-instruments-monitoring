#include <Preferences.h>
#include "sleep.hpp"
#include "networking.hpp"
#include "camera.hpp"
#include "config.h"
#include <String>

Preferences preferences;
// networking - WiFi
// syncronise tim from NTP
// Connect to Azure
// Download schedule
// 

void check_identity(){
    // Open Preferences with my-app namespace. Each application module, library, etc
  // has to use a namespace name to prevent key name collisions. We will open storage in
  // RW-mode (second parameter has to be false).
  // Note: Namespace name is limited to 15 chars.
  int res = preferences.begin("esp-cam", false);
  if(res != true){
    Serial.println("error opening preferences namespace");
  }
  //preferences.clear();
  String device_id_default = "not_set";
  device_id = preferences.getString("device_id", device_id_default);
  if(device_id.equalsIgnoreCase("not_set")){
    Serial.println("setting new device_id......");
    //call REST API for new device_id
    device_id = init_device();
    preferences.putString("device_id", device_id);
  }
  preferences.end();
}

/*
class DeviceConfig(BaseModel):
    upload_times_per_day: int
    upload_mode: str = "test"
*/
void device_config(){
  Serial.println("device config update from server");
  get_device_config();
  //upload_times_per_day = response_json["upload_times_per_day"];
  secconds_2_sleep = response_json["secconds_2_sleep"];
  Serial.print("secconds to sleep: ");
  Serial.println(secconds_2_sleep);
 // upload_mode = config_json["upload_mode"];
}

void ram() {
  Serial.printf(" Ram %7d / %7d  ",  ESP.getFreeHeap(), ESP.getHeapSize());
  Serial.printf(" psRam %7d / %7d\n", ESP.getFreePsram(), ESP.getPsramSize() );
}

void setup(){
  Serial.begin(115200);
  delay(200);
  print_wakeup_reason();
  init_wifi();
  check_identity();
  init_time();
  init_camera();
  delay(200);
  camera_fb_t * fb = NULL;
  fb = get_good_jpeg();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(200);
    ESP.restart();
  }

  Serial.print("img buf len: ");
  Serial.println(fb->len);

  sendPhotoToServer(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  device_config();

  esp_wifi_stop();  
  sleep_setup(secconds_2_sleep);
}



// the control never reach this code
void loop(){
  Serial.println("[ERROR]   THIS SHOULD NEVER SHOW UP!!  ");
  ram();
  camera_fb_t * fb = NULL;
  fb = get_good_jpeg();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(200);
    ESP.restart();
  }

  Serial.print("img buf len: ");
  Serial.println(fb->len);

  sendPhotoToServer(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  ram();
}