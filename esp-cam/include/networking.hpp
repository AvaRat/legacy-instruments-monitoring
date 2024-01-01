#include <WiFi.h>
#include <esp_wifi.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "config.hpp"

#define MIN_EPOCH (40 * 365 * 24 * 3600)
  

WiFiClient wifi;
HttpClient client = HttpClient(wifi, SERVER_URL, SERVER_PORT);
int status = WL_IDLE_STATUS;


static void init_wifi(){  
    // Wi-Fi connection
    WiFi.begin(SSID, PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    
    Serial.print("Local IP: ");
    Serial.print(WiFi.localIP());
    Serial.println();
  }

static void init_time() {
  time_t epochTime;

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000);

  while (true) {
    epochTime = time(NULL);
    if (epochTime < MIN_EPOCH) {
      Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
      delay(2000);
    } else {
      //Serial.print("Fetched NTP epoch time is: ");
      //Serial.println(epochTime);
      break;
    }
  }
}

DynamicJsonDocument response_json(256);

String init_device(){
  response_json.clear();
  String contentType = "application/json";
  client.beginRequest();
  client.post("/init_device");
  client.sendHeader("Content-Type", contentType);
  client.sendHeader("Accept", contentType);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  deserializeJson(response_json, response);

  return response_json["device_id"];
}

int get_device_config(){
  response_json.clear();
  String contentType = "application/json";
  String path = "/" + device_id + "/config";

  client.beginRequest();
  client.get(path);
  client.sendHeader("Content-Type", contentType);
  client.sendHeader("Accept", contentType);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  if(statusCode != 200){
    return -1;
  }
  Serial.print("status code: ");
  Serial.println(statusCode);
  Serial.print("body: ");
  Serial.println(response);
  deserializeJson(response_json, response);
}


void sendPhotoToServer(uint8_t* fbbuf, int fblen) {
  long millisstart = millis();
  uint32_t heapstart = ESP.getFreeHeap();
  //ram();

  StaticJsonDocument<768> doc;

  Serial.printf("Connecting to %s:%d... ", SERVER_URL, SERVER_PORT);

  if (!client.connect(SERVER_URL, SERVER_PORT)) {
    Serial.println("Failure in connection with the server");
    return;
  }
  //ram();

  String start_request = "";
  String end_request = "";
  const char* boundry = "jz629zj";

  long send_start = millis();

  start_request = start_request + "--" + boundry + "\r\n";
  start_request = start_request + "Content-Disposition: form-data; name=\"file\"; filename=\"CAM.jpg\"\r\n";
  start_request = start_request + "Content-Type: image/jpg\r\n";
  start_request = start_request + "\r\n";

  end_request = end_request + "\r\n";
  end_request = end_request + "--" + boundry + "--" + "\r\n";

  int contentLength = (int)fblen + start_request.length() + end_request.length();

  String headers = "POST http://192.168.1.11:8000/" + device_id + "/image HTTP/1.1\r\n"; //edit for your server

  headers = headers + "Host: " + SERVER_URL + "\r\n";
  headers = headers + "User-Agent: ESP32" + "\r\n";
  headers = headers + "Accept: */*\r\n";
  headers = headers + "Content-Type: multipart/form-data; boundary=" + boundry + "\r\n";
  headers = headers + "Content-Length: " + contentLength + "\r\n";
  headers = headers + "\r\n";
  client.print(headers);
  //Serial.print(headers);
  client.flush();

  //Serial.println(start_request);
  client.print(start_request);
  client.flush();

  int iteration = fblen / 1024;
  for (int i = 0; i < iteration; i++) {
    client.write(fbbuf, 1024);
    fbbuf += 1024;
    client.flush();
  }
  size_t remain = fblen % 1024;
  client.write(fbbuf, remain);
  client.flush();
  client.print(end_request);

  long send_done = millis();
  float kbps = 8.0 * fblen / (send_done - send_start) ;
  Serial.printf("Send %d bytes in %.1f seconds for %.1f kbps\n\n", fblen, (send_done - send_start) / 1000.0, kbps );

  // header response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println(line);
    if (line == "\r") {
      //Serial.println("headers received\n\n");
      break;
    }
  }
  // response body
  String description;
  while (client.available()) {
    char c = client.read();
    description = description + c;
    Serial.write(c);
  }

  client.flush();
/*
  deserializeJson(doc, description);
  const char* description_captions_0_text = doc["description"]["captions"][0]["text"];
  char descriptionWithFullStop[100];   // array to hold the result.
  strcpy(descriptionWithFullStop, description_captions_0_text); // copy string one into the result.
  strcat(descriptionWithFullStop, ".");
  float description_captions_0_confidence = doc["description"]["captions"][0]["confidence"];
  Serial.printf("\n\nAzure Computer Vision says => ");
  Serial.print(descriptionWithFullStop);
  Serial.printf(", confidence=%5.1f\n\n", description_captions_0_confidence * 100);
*/
  //ram();
}