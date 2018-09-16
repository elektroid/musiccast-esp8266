#include <ArduinoJson.h>

/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include "SSD1306Brzo.h"
// Include the UI lib
#include "OLEDDisplayUi.h"

#define USE_SERIAL Serial

#define SPK1 D7
#define SPK2 D6

#define LOOPDELAY 5000

const char deviceid[] = "00A0DEF89164"; // as reported inside http GET request, check monitor logs to find out

const char netssid[] = {"your network name"};
const char netpsk[] = "your private key";
const char networkPrefix[] = {"192.168.1."};

String currentScanning = "";
String host = "";
String deviceStatus = "unknown";
String deviceInput;
String deviceVolume;

ESP8266WiFiMulti WiFiMulti;

const size_t bufferSize = JSON_OBJECT_SIZE(11) + 260;
DynamicJsonDocument jsonBuffer(bufferSize);
HTTPClient http;

// Initialize the OLED display
SSD1306Brzo  display(0x3c, SDA, SCL); // 3 and 4 on wemos
OLEDDisplayUi ui     ( &display );



void drawFrame0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 10 + y, "Connecting");
  display->drawString(0 + x, 38 + y, "WIFI");

}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "Scanning:" + currentScanning);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "Host:" + host);
  display->drawString(0 + x, 22 + y, "Status:" + deviceStatus);
  display->drawString(0 + x, 33 + y, "Input:" + deviceInput);
  display->drawString(0 + x, 44 + y, "Volume:" + deviceVolume);
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame0, drawFrame1, drawFrame2 };

// how many frames are there?
int frameCount = 3;




int getDeviceInfo(String host, String device_id) {

  USE_SERIAL.print("[HTTP] begin getDeviceInfo for " + host + "...\n");
  http.begin("http://" + host + "/YamahaExtendedControl/v1/system/getDeviceInfo"); //HTTP

  USE_SERIAL.print("[HTTP getDeviceInfo] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    USE_SERIAL.printf("[HTTP getDeviceInfo %s] GET... code: %d\n", netssid, httpCode);

    // let's parse response and check device_id
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      USE_SERIAL.println(payload);
      // Parse JSON object
      DeserializationError error = deserializeJson(jsonBuffer, payload);
      if (error) {
        USE_SERIAL.print(F("deserializeJson() failed: "));
        USE_SERIAL.println(error.c_str());
        return false;
      }

      // Extract values
      JsonObject root = jsonBuffer.as<JsonObject>();

      String deviceId = root["device_id"];
      USE_SERIAL.println("Found device : " + deviceId);
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
      //    JsonObject& root = jsonBuffer.parseObject(payload);
      // Parse JSON object
      DeserializationError error = deserializeJson(jsonBuffer, payload);
      if (error) {
        USE_SERIAL.print(F("deserializeJson() failed: "));
        USE_SERIAL.println(error.c_str());
        return false;
      }

      // Extract values
      JsonObject root = jsonBuffer.as<JsonObject>();
      USE_SERIAL.printf("Device at %s is %s\n", host.c_str(), deviceStatus.c_str());

      String ds = root["power"];
      deviceStatus = ds;

      String di = root["input"];
      deviceInput = di;
      String dv = root["volume"];
      deviceVolume = dv;

      digitalWrite(LED_BUILTIN, LOW);
      if (deviceStatus == "on") {
        USE_SERIAL.println(F("device is on"));
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(SPK1, HIGH);
        digitalWrite(SPK2, HIGH);
      }
      if (deviceStatus == "standby") {
        USE_SERIAL.println(F("device in is standby"));
        digitalWrite(SPK1, LOW);
        digitalWrite(SPK2, LOW);
      }
    }
  } else {
    USE_SERIAL.printf("[HTTP getDeviceStatus] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    lostBox();
  }

  http.end();
  return httpCode == HTTP_CODE_OK;
}


String findBox() {
  for (byte i = 28; i < 255; i++) {
    currentScanning = networkPrefix + String(i);
    ui.update();
    USE_SERIAL.println("in findBox try " + currentScanning);
    if (getDeviceInfo(currentScanning, deviceid)) {
      host = networkPrefix + String(i);
      return host;
    }
  }
  USE_SERIAL.println("findBox loop ended, nothing found\n");
  return host;
}

void checkBox() {
  if (host == "") {
    ui.switchToFrame(1);
    findBox();
  }
  USE_SERIAL.println("Current host : '" + host + "'");
  while (getDeviceStatus(host)) {
    ui.switchToFrame(2);
    ui.update();
    delay(LOOPDELAY);
  }
}

void lostBox() {
  USE_SERIAL.println(F("Box was lost"));
  host = "";
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Initialize the speakers pins, off by default
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
  WiFiMulti.addAP(netssid, netpsk);

  http.setReuse(true);
  ui.setTargetFPS(24);
  // Add frames
  ui.setFrames(frames, frameCount);
  // Initialising the UI will init the display too.
  ui.init();
  ui.disableIndicator();
  ui.disableAutoTransition();
  display.flipScreenVertically();

}

int hostIpSuffix = 0;
void loop() {

  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    ui.switchToFrame(1);
    ui.update();
    USE_SERIAL.printf("wifi %s connected\n", netssid);
    checkBox();
  } else {
    ui.switchToFrame(0);
    ui.update();
    USE_SERIAL.println(F("wifi not connected"));
  }

  //  int remainingTimeBudget = ui.update();

  delay(200);
}
