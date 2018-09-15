#include <ArduinoJson.h>

/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial
#define SPK1 D3
#define SPK2 D4

ESP8266WiFiMulti WiFiMulti;

const size_t bufferSize = JSON_OBJECT_SIZE(11) + 260;
DynamicJsonBuffer jsonBuffer(bufferSize);
HTTPClient http;

String networkPrefix = "192.168.1.";
int getDeviceInfo(String host, String device_id) {

  USE_SERIAL.print("[HTTP] begin get device info for " + host + "...\n");
  http.begin("http://" + host + "/YamahaExtendedControl/v1/system/getDeviceInfo"); //HTTP

  USE_SERIAL.print("[HTTP getDeviceInfo] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();


  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP getDeviceInfo] GET... code: %d\n", httpCode);

    // let's parse response and check device_id
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
      JsonObject& root = jsonBuffer.parseObject(payload);
      USE_SERIAL.printf("Found device : %s\n", root["device_id"]);
      if (root["device_id"] == device_id) {
        http.end();
        return true;
      }
    }
  } else {
    USE_SERIAL.printf("[HTTP getDeviceInfo] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  return false;

}


int getDeviceStatus(String host) {
  USE_SERIAL.print("[HTTP getDeviceStatus] begin get status for " + host + " ...\n");
  // configure traged server and url
  //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
  http.begin("http://" + host + "/YamahaExtendedControl/v1/main/getStatus"); //HTTP

  USE_SERIAL.print("[HTTP getDeviceStatus] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP getDeviceStatus] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
      JsonObject& root = jsonBuffer.parseObject(payload);
      
      USE_SERIAL.printf("Device at %s is %s\n", host.c_str(), root["power"]);
      digitalWrite(LED_BUILTIN, LOW);
      if (root["power"] == "on") {
        USE_SERIAL.println("device is on");
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(SPK1, HIGH);
        digitalWrite(SPK2, HIGH);
      }
      if (root["power"] == "standby") {
        USE_SERIAL.println("devive in is standby");
        digitalWrite(SPK1, LOW);
        digitalWrite(SPK2, LOW);
      }
    }
  } else {
    USE_SERIAL.printf("[HTTP getDeviceStatus] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return httpCode == HTTP_CODE_OK;
}

int findBox() {
  for (int i = 29; i < 255; i++) {
    USE_SERIAL.printf("in find box try %d \n", i);
    if (getDeviceInfo(networkPrefix + String(i), "00A0DEF89164")) {
      USE_SERIAL.printf("exit find box on %d \n", i);
      return i;
    }
  }
  USE_SERIAL.printf("loop ended, no box, last check  \n");
  return 0;
}


void setup() {

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(SPK1, OUTPUT);
  pinMode(SPK2, OUTPUT);
  digitalWrite(SPK1, LOW);
  digitalWrite(SPK2, LOW);

  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());
  USE_SERIAL.printf("Reset reason: %s\n", ESP.getResetReason().c_str());

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(100);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("MOVISTAR_E696", "68465AADE044B1F7937A");

}

int i = 0;
void loop() {

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    USE_SERIAL.println("wifi is on");
    if (i || (i = findBox())) {
      USE_SERIAL.println(networkPrefix + String(i));
      if (!getDeviceStatus(networkPrefix + String(i))) {
        i = 0;
      }
    }
  }

  delay(10000);
}

