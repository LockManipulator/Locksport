/*
Arduino IDE board settings
--------------------------
Board: ESP32S3 Dev Module  
USB CDC on boot: Enabled  
CPU Frequency: 240MHz (WiFi)  
Core Debug Level: None  
USB DFU On Boot: Disabled  
Erase All Flash Before Sketch Upload: Disabled  
Events Run On: Core 1  
Flash Mode: QIO 80MHz  
Flash Size: 4MB (32MB)  
JTAG Adapter: Disabled  
Arduino Runs On: Core 1  
USB Firmware MSC On Boot: Disabled  
Partition Scheme: Default 4MB with SPIFFS (1.2MB APP/1.5MB SPIFFS)
PSRAM: QSPI PSRAM  
Upload Mode: UART0/Hardware CDC  
Upload Speed: 921600  
USB Mode: Hardware CDC and JTAG  


Libraries
---------
Rob Tillaart's AS5600 library
https://github.com/RobTillaart/AS5600/tree/master

Adafruit's GC9A01A library
https://github.com/adafruit/Adafruit_GC9A01A/tree/main

Wifi libraries
https://github.com/mathieucarbou/AsyncTCP
https://github.com/mathieucarbou/ESPAsyncWebServer


Uploading data folder to esp32 storage
--------------------------------------
Make sure your arduino sketch has a folder called 'data' with all the files in it.
Download the latest .vsix file from https://github.com/earlephilhower/arduino-littlefs-upload/releases
Put it in ~/.arduinoIDE/plugins/ on Mac and Linux or C:\Users\<username>\.arduinoIDE\plugins\ on Windows (you may need to make this directory yourself beforehand)
Restart the IDE
[Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32".

On macOS, press [⌘] + [Shift] + [P] to open the Command Palette in the Arduino IDE, then "Upload LittleFS to Pico/ESP8266/ESP32".


To do:
-----
Guided manipulation
*/

#include "AS5600.h"
#include <WiFi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#define TFT_MOSI 3  // SDA but really MOSI
#define TFT_SCLK 2  // SCL but really SCLK
#define TFT_DC 10   // DC Data Command control pin
#define TFT_CS 11   // CS Chip select control pin
#define TFT_RST 12  // RST Reset pin (could connect to Arduino RESET pin)

const char* configFile = "/config.json";

// Screen variables
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);
int tftWidth = tft.width();
int tftHeight = tft.height();
int prevX = tftWidth / 2;               // Used to draw over text instead of updating entire screen.
int prevY = tftHeight / 2;              // Used to draw over text instead of updating entire screen.
int rotation = 1;                       // Default screen rotation is 1 (for if the magnifier sticks out from the dial to the left).
int textSize = 5;                       // Default text size is 5.
int prevTextSize = 5;                   // For erasing old text.
bool updateText = true;                 // Whether the text needs to be updated or not. Default is true.
int background[3] = {0, 0, 0};          // Background. Default is [0, 0, 0] (black).
int foreground[3] = {255, 255, 255};    // Text. Default is [255, 255, 255] (white).
const char* bootImages[] = {"none", "dial"};    // Currently available boot images.
String bootImage = "dial";              // Which boot screen to show. Look in bootDisplay() for more info. Default is "dial".
int bootTime = 1000;                    // Amount of milliseconds to show the boot screen for. Default is 1000.
bool drawBorder = true;                 // Whether or not to draw a border around the screen. Default is true.
float borderThickness = 0.1;            // Thickness of the border in percentage of the radius of the screen. Default is 0.1.
int borderColor[3] = {255, 255, 255};   // Color of the border. Default is [255, 255, 255] (white).
bool drawPointer = true;                // Whether or not to have a fancy animated pointer. Works with or without border and adjusts to inside the border if border is enabled. Default is true.
int pointerSize = 18;                   // Size of the pointer. Default is 18.
int prev_x1 = 0;                        // For erasing previous pointer if enabled.
int prev_x2 = 0;                        // For erasing previous pointer if enabled.
int prev_y1 = 0;                        // For erasing previous pointer if enabled.
int prev_y2 = 0;                        // For erasing previous pointer if enabled.
bool modeChanged = false;               // Bandaid for screen artifacts.

// Encoder variabes
AS5600 as5600;                // Use default Wire.
int dirPin = 5;               // Change this if you didn't use pin 5 to encoder DIR.
int updateInterval = 50;      // How many milliseconds in-between encoder reading.
int baseLine = 0;             // To zero our encoder.
float sensorData = 0;         // The variable the encoder position gets stored in.
float prev_sensorData = 0;    // Used for calculations.

// Access point handling
const char *ssidap = "DDM";           // Can set custom access point name here.
const char *passwordap = "magnifier"; // Can set custom access point password here.
String loginname = "";                // Web page login page username.
String loginpassword = "";            // Web page login page password (both needed if setting just one).
IPAddress local_ip(192, 168, 0, 1);   // Go to 192.168.0.1 in a browser to control device.
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
String header;
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  LittleFS.begin();   // Initialize LittleFS
  load_settings();    // Load settings

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // Inform about our pins for the screen.
  tft.begin();                                // Start the screen.
  tft.setRotation(rotation);         // Rotate the screen so the protrusion is up.
  tft.fillScreen(toColor(background));        // Fills the screen with the background color.
  tft.setTextColor(toColor(foreground));      // Sets the text color to the foreground color.
  tft.setTextSize(textSize);                  // Sets the text size.

  Wire.begin();                               // Start the encoder.
  as5600.begin(dirPin);                       // Set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);     // Default, just be explicit.

  // Send web page with input fields to client and handle requests
  wifi_setup(ssidap, passwordap);

  // Show the main home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Show login prompt if web page is username and password protected
    if (loginname != "" && loginpassword != "") {
      if (!request->authenticate((char *)loginname.c_str(), (char *)loginpassword.c_str())) {
        return request->requestAuthentication();
      }
    }
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Serve the CSS file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  // Serve other files (if necessary, like JS)
  server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.js", "application/javascript");
  });

  // Serve other files (if necessary, like JS)
  server.on("/settings.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/settings.js", "application/javascript");
  });

  // Serve other files (if necessary, like JS)
  server.on("/chart.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/chart.js", "application/javascript");
  });

  // Show help page
  server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/help.html", "text/html");
  });

  // Show graph page
  server.on("/graph.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/graph.js", "application/javascript");
  });

  // Show graph page
  server.on("/graph", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/graph.html", "text/html");
  });

  // Show settings page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/settings.html", "text/html");
  });

  // Reset encoder baseline
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    baseLine = as5600.rawAngle(); // Zero out our encoder
    request->send_P(200, "text/text", "true");
  });

  // Show settings page
  server.on("/savesettings", HTTP_GET, [](AsyncWebServerRequest *request) {
    rotation = request->getParam("screen_rotation")->value().toInt();
    textSize = request->getParam("text_size")->value().toInt();
    bootImage = request->getParam("boot_image")->value();
    bootTime = request->getParam("boot_time")->value().toInt();
    drawBorder = request->getParam("dial_border")->value() == "true";
    borderThickness = request->getParam("dial_border_thickness")->value().toFloat();
    drawPointer = request->getParam("show_pointer")->value() == "true";
    pointerSize = request->getParam("pointer_size")->value().toFloat();
    updateInterval = request->getParam("update_interval")->value().toInt();

    save_settings();
    modeChanged = true;
    request->send_P(200, "text/text", "true");
  });

  // Send current settings to setting page
  server.on("/getoptions", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;

    // Save values to JSON
    doc["screen_rotation"] = rotation;
    doc["text_size"] = textSize;
    doc["boot_image"] = bootImage;
    doc["boot_time"] = bootTime;
    doc["dial_border"] = drawBorder;
    doc["border_thickness"] = borderThickness;
    doc["show_pointer"] = drawPointer;
    doc["pointer_size"] = pointerSize;
    doc["update_interval"] = updateInterval;

    // Send the JSON response to the client
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  // Send available boot images to web page
  server.on("/getBoots", HTTP_GET, [](AsyncWebServerRequest *request) {
      // Calculate the number of elements in the array
      size_t arraySize = sizeof(bootImages) / sizeof(bootImages[0]);

      // Use ArduinoJson to convert the array into a JSON string
      DynamicJsonDocument doc(256); // Adjust size based on expected data
      JsonArray array = doc.to<JsonArray>();

      // Add each element of the C-string array to the JSON array
      for (size_t i = 0; i < arraySize; i++) {
          array.add(bootImages[i]);
      }

      // Serialize JSON to a String
      String response;
      serializeJson(doc, response);

      // Send the JSON string as the response
      request->send(200, "application/json", response);
  });

  server.on("/bgcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    background[0] = request->getParam("r")->value().toInt();
    background[1] = request->getParam("g")->value().toInt();
    background[2] = request->getParam("b")->value().toInt();
    modeChanged = true;
    request->send_P(200, "text/text", "true");
  });

  server.on("/fgcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    foreground[0] = request->getParam("r")->value().toInt();
    foreground[1] = request->getParam("g")->value().toInt();
    foreground[2] = request->getParam("b")->value().toInt();
    modeChanged = true;
    request->send_P(200, "text/text", "true");
  });

  server.on("/bordercolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    borderColor[0] = request->getParam("r")->value().toInt();
    borderColor[1] = request->getParam("g")->value().toInt();
    borderColor[2] = request->getParam("b")->value().toInt();
    modeChanged = true;
    request->send_P(200, "text/text", "true");
  });

  // Sends the current dial position to the web page
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/text", String(sensorData).c_str());
  });

  server.onNotFound(notFound);
  server.begin();

  // The esp turns on pretty fast so the movement of releasing the on button causes the screen to start just slightly off of 0 occasionally.
  // This is both a cool artificial boot screen and a delay before zero'ing the dial to ensure no unwanted movement.
  bootDisplay();
  baseLine = as5600.rawAngle(); // Zero out our encoder
  modeChanged = true;
}

void loop() {
  static uint32_t lastTime = 0;

  if (millis() - lastTime >= updateInterval) {
    lastTime = millis();
    sensorData = getInc();
    if (sensorData != prev_sensorData || modeChanged) {
      updateDisplay();
    }
  }  
}

// Handles updating the display during normal operations
void updateDisplay() { 
  String screenText = String(sensorData);
  tft.setTextSize(textSize);

  if (modeChanged) {
    drawScreenBorder();
    modeChanged = false;
  }
  else {
    tft.fillCircle(tftWidth / 2, tftHeight / 2, int((tftWidth / 2) * (1 - borderThickness)), toColor(background));
  }

  // Calculate text boundaries
  int16_t textX, textY;
  uint16_t textWidth, textHeight;

  // Measure the bounding box of the text
  tft.getTextBounds(screenText, 0, 0, &textX, &textY, &textWidth, &textHeight);

  // Calculate position to center the text
  int16_t x = (tftWidth - textWidth) / 2;
  // Adjust y-coordinate for baseline alignment
  int16_t y = tftHeight / 2 - textHeight / 2;

  // Set text properties and draw
  tft.setTextColor(toColor(foreground));
  tft.setCursor(x, y); // y is now adjusted
  tft.println(screenText);

  // Draw pointer
  if (drawPointer) {
    showPointer();
  }
  prev_sensorData = sensorData;
}

void showPointer() {
  // Erase previous pointer
  float angle = ((100 - prev_sensorData) * 3.6) - 90; // Make sure 0 is at the top of the screen and we're following 0 with the pointer

  // Convert angle to radians
  float angleRad = radians(angle);
  
  int startX = (tftWidth / 2) + (((tftWidth / 2)) * cos(angleRad));
  int startY = (tftHeight / 2) + (((tftHeight / 2)) * sin(angleRad));

  // Calculate the starting point (18 pixels away from the center along the angle)
  if (drawBorder) {
    startX = (tftWidth / 2) + (((tftWidth / 2) * (1 - borderThickness)) * cos(angleRad));
    startY = (tftHeight / 2) + (((tftHeight / 2) * (1 - borderThickness)) * sin(angleRad));
  }

  // Calculate the other two points of the isosceles triangle
  // The acute angle will be split symmetrically on either side
  float halfAngle = radians(45 / 2); // Half the acute angle to split it evenly
  
  // Draw the isosceles triangle
  tft.fillTriangle(startX, startY, prev_x1, prev_y1, prev_x2, prev_y2, toColor(background));

  // Draw moving pointer
  angle = ((100 - sensorData) * 3.6) - 90; // Make sure 0 is at the top of the screen and we're following 0 with the pointer
  
  // Convert angle to radians
  angleRad = radians(angle);
  
  startX = (tftWidth / 2) + (((tftWidth / 2)) * cos(angleRad));
  startY = (tftHeight / 2) + (((tftHeight / 2)) * sin(angleRad));

  // Calculate the starting point (18 pixels away from the center along the angle)
  if (drawBorder) {
    startX = (tftWidth / 2) + (((tftWidth / 2) * (1 - borderThickness)) * cos(angleRad));
    startY = (tftHeight / 2) + (((tftHeight / 2) * (1 - borderThickness)) * sin(angleRad));
  }
  
  // Calculate the other two points of the isosceles triangle
  // The acute angle will be split symmetrically on either side
  halfAngle = radians(45 / 2); // Half the acute angle to split it evenly

  // Calculate the two other points based on the isosceles property
  int x1 = startX - pointerSize * cos(angleRad - halfAngle);
  int y1 = startY - pointerSize * sin(angleRad - halfAngle);
  
  int x2 = startX - pointerSize * cos(angleRad + halfAngle);
  int y2 = startY - pointerSize * sin(angleRad + halfAngle);

  prev_x1 = x1;
  prev_x2 = x2;
  prev_y1 = y1;
  prev_y2 = y2;
  
  // Draw the isosceles triangle
  tft.fillTriangle(startX, startY, x1, y1, x2, y2, toColor(borderColor));
}

uint16_t toColor(int color[]) {
  return tft.color565(color[0], color[1], color[2]);
}

void load_settings() {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    save_settings();
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse settings to load!");
    file.close();
    return;
  }

  // Load values from JSON
  rotation = doc["screen_rotation"];
  textSize = doc["text_size"];
  JsonArray bgColor = doc["background_color"];
  if (!bgColor.isNull() && bgColor.size() == 3) {
    background[0] = bgColor[0];
    background[1] = bgColor[1];
    background[2] = bgColor[2];
  }

  JsonArray fgColor = doc["foreground_color"];
  if (!fgColor.isNull() && fgColor.size() == 3) {
    foreground[0] = fgColor[0];
    foreground[1] = fgColor[1];
    foreground[2] = fgColor[2];
  }

  bootImage = doc["boot_image"].as<String>();
  bootTime = doc["boot_time"];
  drawBorder = doc["dial_border"];
  borderThickness = doc["border_thickness"];
  JsonArray bColor = doc["border_color"];
  if (!bColor.isNull() && bColor.size() == 3) {
    borderColor[0] = bColor[0];
    borderColor[1] = bColor[1];
    borderColor[2] = bColor[2];
  }

  drawPointer = doc["show_pointer"];
  pointerSize = doc["pointer_size"];
  updateInterval = doc["update_interval"];

  file.close();
}

void save_settings() {
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("Failed to open file for saving!");
    return;
  }

  StaticJsonDocument<512> doc;

  // Save values to JSON
  doc["screen_rotation"] = rotation;
  doc["text_size"] = textSize;

  JsonArray bgColor = doc.createNestedArray("background_color");
  for (int i = 0; i < 3; i++) {
    bgColor.add(background[i]);
  }

  JsonArray fgColor = doc.createNestedArray("foreground_color");
  for (int i = 0; i < 3; i++) {
    fgColor.add(foreground[i]);
  }

  doc["boot_image"] = bootImage;
  doc["boot_time"] = bootTime;
  doc["dial_border"] = drawBorder;
  doc["border_thickness"] = borderThickness;

  JsonArray bColor = doc.createNestedArray("border_color");
  for (int i = 0; i < 3; i++) {
    bColor.add(borderColor[i]);
  }

  doc["show_pointer"] = drawPointer;
  doc["pointer_size"] = pointerSize;
  doc["update_interval"] = updateInterval;

  // Write JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write settings!");
  }

  file.close();
}

float getInc() {
  float currentNum = (as5600.rawAngle() - baseLine) / 40.96;
  if (currentNum < 0.000) {
    currentNum += 100.000;
  }
  else if (currentNum == 100.00) {
    currentNum = 0.000;
  }
  return currentNum;
}


// Draw border around screen if enabled
void drawScreenBorder() {
  if (drawBorder) {
    tft.fillScreen(toColor(borderColor));
    tft.fillCircle(tftWidth / 2, tftHeight / 2, int((tftWidth / 2) * (1 - borderThickness)), toColor(background));
  }
  else {
    tft.fillScreen(toColor(background));
  }
}

// Add new boot screen options here
void bootDisplay() {
  if (bootImage == "dial") {
    draw_dial();
  }

  // Wait long enough to make sure we're not touching the encoder
  delay(bootTime);
  tft.fillScreen(toColor(background)); // Clear screen
}

// Draw safe dial
void draw_dial() {
  int radius = 120;
  int increments = 50;

  int x = tftWidth / 2;
  int y = tftHeight / 2;
  int outer_ring = int(radius * 0.95);
  int dial_outer = int(outer_ring * 0.95);
  int dial_inner = int(dial_outer * 0.5);
  int knob = int(dial_inner * 0.85);

  // Draw outer ring
  tft.fillCircle(x, y, outer_ring, toColor(background));

  // Draw dial
  tft.fillCircle(x, y, dial_outer, toColor(foreground));
  tft.fillCircle(x, y, dial_inner, toColor(background));

  // Mark dial
  for (int i = 0; i < increments; i++) {
    float angle = i * (360.0 / increments); // 360 degrees divided by number of increments
    float radian = angle * PI / 180;
    int16_t xStart = x + (dial_outer - (i % 5 == 0 ? 25 : 15)) * cos(radian); // Longer lines for multiples of 5
    int16_t yStart = y + (dial_outer - (i % 5 == 0 ? 25 : 15)) * sin(radian);
    int16_t xEnd = x + dial_outer * cos(radian);
    int16_t yEnd = y + dial_outer * sin(radian);

    // Calculate line thickness using fillRect
    int thickness = 3; // Set line thickness
    float dx = xEnd - xStart; // Difference in x
    float dy = yEnd - yStart; // Difference in y
    float length = sqrt(dx * dx + dy * dy); // Line length

    // Perpendicular offsets for rectangle thickness
    float offsetX = thickness * (dy / length) / 2;
    float offsetY = thickness * (-dx / length) / 2;

    // Draw the rectangle
    tft.fillTriangle(
        xStart - offsetX, yStart - offsetY,
        xStart + offsetX, yStart + offsetY,
        xEnd - offsetX, yEnd - offsetY,
        toColor(background));

    tft.fillTriangle(
        xEnd - offsetX, yEnd - offsetY,
        xEnd + offsetX, yEnd + offsetY,
        xStart + offsetX, yStart + offsetY,
        toColor(background));
  }

  // Draw knob
  tft.fillCircle(x, y, knob, toColor(foreground));

    // Notch the knob
  for (int i = 0; i < 50; i++) {
    float angle = i * (360.0 / 50); // 360 degrees divided by number of increments
    float radian = angle * PI / 180;
    int16_t xStart = x + (knob - 3) * cos(radian);
    int16_t yStart = y + (knob - 3) * sin(radian);
    int16_t xEnd = x + knob * cos(radian);
    int16_t yEnd = y + knob * sin(radian);

    // Calculate line thickness using fillRect
    int thickness = 1; // Set line thickness
    float dx = xEnd - xStart; // Difference in x
    float dy = yEnd - yStart; // Difference in y
    float length = sqrt(dx * dx + dy * dy); // Line length

    // Perpendicular offsets for rectangle thickness
    float offsetX = thickness * (dy / length) / 2;
    float offsetY = thickness * (-dx / length) / 2;

    // Draw the rectangle
    tft.fillTriangle(
        xStart - offsetX, yStart - offsetY,
        xStart + offsetX, yStart + offsetY,
        xEnd - offsetX, yEnd - offsetY,
        toColor(background));

    tft.fillTriangle(
        xEnd - offsetX, yEnd - offsetY,
        xEnd + offsetX, yEnd + offsetY,
        xStart + offsetX, yStart + offsetY,
        toColor(background));
  }
}

int normalizeNum(int num) {
  while (num >= 100) {
    num -= 100;
  }
  while (num < 0) {
    num += 100;
  }
  return num;
}

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
