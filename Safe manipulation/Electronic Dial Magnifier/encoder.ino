#include "AS5600.h"
#include <WiFi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "CustomWeb.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"

#define TFT_MOSI 3 //SDA In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 2 //SCL
#define TFT_CS 10 //CS Chip select control pin
#define TFT_DC 11 //DC Data Command control pin
#define TFT_RST 12 //RST Reset pin (could connect to Arduino RESET pin)
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);
int textSize = 5;

AS5600 as5600;            // Use default Wire
int dirPin = 5;            // AS5600 out pin
int updateInterval = 50;  // How many milliseconds in-between encoder reading
int baseLine = 0;
float sensorData = 0;
float prev_sensorData = 0;
int prevX = 0;
int prevY = 0;

//AP handling
const char *ssidap = "Encoder";      // Can set custom access point name here
const char *passwordap = "";         // Can set custom access point password here
String loginname = "root";           // Web page login page username
String loginpassword = "toor";       // Web page login page password
IPAddress local_ip(192, 168, 0, 1);  // Go to 192.168.0.1 in a browser to control device
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);
String esp_response = "";

void setup() {
  Serial.begin(115200);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.begin();
  tft.setRotation(2);

  // Clear the screen
  tft.fillScreen(GC9A01A_BLACK);

  // Set text color and size
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(textSize);

  //update_display();

  Wire.begin();
  as5600.begin(dirPin);  //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
  baseLine = as5600.rawAngle();

  // Send web page with input fields to client and handle requests
  wifi_setup(ssidap, passwordap);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate((char *)loginname.c_str(), (char *)loginpassword.c_str())) {
      return request->requestAuthentication();
    }
    request->send_P(200, "text/html", SendHTML);
  });

  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/text", String(sensorData).c_str());
  });

  server.on("/plot", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message = "";
    String message1 = "";
    String message2 = "";
    String num = request->getParam("testing")->value();
    String rcontactpoint = request->getParam("rcontactpoint")->value();
    String lcontactpoint = request->getParam("lcontactpoint")->value();

    if (rcontactpoint) {
      message1 += "R," + num + "," + lcontactpoint;
    }

    if (lcontactpoint) {
      message2 += "L," + num + "," + lcontactpoint;
    }

    if (rcontactpoint && lcontactpoint) {
      message = message1 + "," + message2;
    }
    else {
      if (rcontactpoint) {
        message = message1;
      }
      else if (lcontactpoint) {
        message = message1;
      }
    }
    request->send_P(200, "text/text", message.c_str());
  });

  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  static uint32_t lastTime = 0;

  //  update every 50 ms
  if (millis() - lastTime >= updateInterval)
  {
    lastTime = millis();
    sensorData = getInc();
    update_display();
  }
}

float getInc() {
  float curNum = (as5600.rawAngle() - baseLine) / 40.96;

  if (curNum < 0.000) {
    curNum += 100.000;
  }
  
  else if (curNum == 100.00) {
    curNum = 0.000;
  }

  return curNum;
}

void update_display() {
  String numberStr = String(sensorData);

  tft.setTextColor(GC9A01A_BLACK);
  tft.setCursor(prevX, prevY);
  tft.println(String(prev_sensorData));

  // Display a number in the center of the screen
  int16_t screenWidth = tft.width();
  int16_t screenHeight = tft.height();

  // Calculate text position for centering
  int textWidth = numberStr.length() * 6 * textSize; // 6 pixels per char, size 3
  int textHeight = 8 * textSize;                    // 8 pixels height per char, size 3
  int16_t x = (screenWidth - textWidth) / 2;
  int16_t y = (screenHeight - textHeight) / 2;

  // Set cursor and display the text
  prevX = x;
  prevY = y;
  prev_sensorData = sensorData;
  tft.setTextColor(GC9A01A_WHITE);
  tft.setCursor(x, y);
  tft.println(numberStr);
}

// If invalid web page is requested
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Handles setting up the AP
void wifi_setup(const char *name, const char *pass) {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  if (pass == "") {
    WiFi.softAP(name, NULL);
  } else {
    WiFi.softAP(name, pass);
  }
}