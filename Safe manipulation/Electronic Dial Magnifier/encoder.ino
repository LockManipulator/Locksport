/*
Uploading web page to esp32 storage
-----------------------------------
Make sure your arduino sketch has a folder called 'data' with the index.html file in it.
Download the latest .vsix file from https://github.com/earlephilhower/arduino-littlefs-upload/releases
Put it in ~/.arduinoIDE/plugins/ on Mac and Linux or C:\Users\<username>\.arduinoIDE\plugins\ on Windows (you may need to make this directory yourself beforehand)
Restart the IDE
[Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32".

On macOS, press [⌘] + [Shift] + [P] to open the Command Palette in the Arduino IDE, then "Upload LittleFS to Pico/ESP8266/ESP32".


To do:
-----
1. Use config file to save preferences.
2. Multiple boot screens.
3. Dial borders.
4. Face styles.
5. Change and save colors from web page.
*/

#include "AS5600.h"
#include <WiFi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <LittleFS.h>

#define TFT_MOSI 3  // SDA but really MOSI
#define TFT_SCLK 2  // SCL but really SCLK
#define TFT_CS 10   // CS Chip select control pin
#define TFT_DC 11   // DC Data Command control pin
#define TFT_RST 12  // RST Reset pin (could connect to Arduino RESET pin)

// Screen variables
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);
int tftWidth = tft.width();
int tftHeight = tft.height();
int prevX = tftWidth / 2;
int prevY = tftHeight / 2;
int rotation = 2; // Default screen rotation is 2. Makes the protrusion from the screen point up.
int textSize = 5; // Default text size is 5.
uint16_t background = GC9A01A_BLACK;  // Background. Default is GC9A01A_BLACK.
uint16_t foreground = GC9A01A_WHITE;  // Text. Default is GC9A01A_WHITE.
bool bootOn = true;                   // Controls whether or not to show a boot screen (if false, may lead to screen not showing exactly 0 on startup). Default is true.
int bootTime = 1000;                  // Amount of milliseconds to show the boot screen for. Default is 1000.
bool drawBorder = true;               // Whether or not to draw a border around the screen.
float borderThickness = 0.1;          // Thickness of the border in percentage of the radius of the screen.
uint16_t borderColor = GC9A01A_WHITE; // Color of the border.
bool drawPointer = true;              // Whether or not to have a fancy animated pointer. Works with or without border, adjusts to inside the border if border is enabled.
int pointerSize = tftWidth * 0.075;   // Size of the pointer.
int prev_x1 = 0;
int prev_x2 = 0;
int prev_y1 = 0;
int prev_y2 = 0;

// Encoder variabes
AS5600 as5600;              // Use default Wire
int dirPin = 5;             // Change this if you didn't use pin 5 to encoder DIR
int updateInterval = 50;    // How many milliseconds in-between encoder reading
int baseLine = 0;           // To zero our encoder
float sensorData = 0;       // The variable the encoder position gets stored in
float prev_sensorData = 0;  // Used to check if there's a change so we don't needlessly redraw the screen

//Access point handling
const char *ssidap = "Encoder";      // Can set custom access point name here
const char *passwordap = "";         // Can set custom access point password here
String loginname = "root";           // Web page login page username
String loginpassword = "toor";       // Web page login page password
IPAddress local_ip(192, 168, 0, 1);  // Go to 192.168.0.1 in a browser to control device
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
String header;
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  // Start the screen
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(rotation);      // Rotate the screen so the protrusion is up.
  tft.fillScreen(background);     // Fills the screen with the background color.
  tft.setTextColor(foreground);   // Sets the text color to the foreground color.
  tft.setTextSize(textSize);      // Sets the text size.

  // Start the encoder
  Wire.begin();
  as5600.begin(dirPin);                    //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.

  // Send web page with input fields to client and handle requests
  wifi_setup(ssidap, passwordap);

  // Show the main home page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate((char *)loginname.c_str(), (char *)loginpassword.c_str())) {
      return request->requestAuthentication();
    }

    request->send(LittleFS, "/index.html", "text/html");
  });

  // Sends the current dial position to the web page
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/text", String(sensorData).c_str());
  });

  server.on("/bgcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    background = tft.color565(request->getParam("r")->value().toInt(), request->getParam("g")->value().toInt(), request->getParam("b")->value().toInt());

    // Draw border around screen if enabled
    if (drawBorder) {
      tft.fillScreen(background);
      tft.fillCircle(tftWidth / 2, tftHeight / 2, tftWidth / 2, borderColor);
      tft.fillCircle(tftWidth / 2, tftHeight / 2, int((tftWidth / 2) * (1 - borderThickness)), background);
    }
    update_display();

    request->send_P(200, "text/text", "true");
  });

  server.on("/fgcolor", HTTP_GET, [](AsyncWebServerRequest *request) {
    foreground = tft.color565(request->getParam("r")->value().toInt(), request->getParam("g")->value().toInt(), request->getParam("b")->value().toInt());
    borderColor = foreground;

    // Draw border around screen if enabled
    if (drawBorder) {
      tft.fillScreen(background);
      tft.fillCircle(tftWidth / 2, tftHeight / 2, tftWidth / 2, borderColor);
      tft.fillCircle(tftWidth / 2, tftHeight / 2, int((tftWidth / 2) * (1 - borderThickness)), background);
    }
    update_display();

    request->send_P(200, "text/text", "true");
  });

  // Graphs the data user inputs on the website
  server.on("/plot", HTTP_GET, [](AsyncWebServerRequest *request) {

    /*
      Checks if both left and right contact points were entered or only one and graph accordingly.

      Requests should look like "L, 34, 2.25" or "R, 34, 7.5, L, 34, 2.25".
      Format: "{which contact point}, {number being tested}, {contact point}".
      If there are both left and right contact points, put the right first.
    */
    String message = "";
    String message1 = "";
    String message2 = "";
    String num = request->hasParam("testing") ? request->getParam("testing")->value() : "";
    String rcontactpoint = request->hasParam("rcontactpoint") ? request->getParam("rcontactpoint")->value() : "";
    String lcontactpoint = request->hasParam("lcontactpoint") ? request->getParam("lcontactpoint")->value() : "";

    if (rcontactpoint != "") {
      message1 += "R," + num + "," + rcontactpoint;
    }

    if (lcontactpoint != "") {
      message2 += "L," + num + "," + lcontactpoint;
    }

    if (rcontactpoint != "" && lcontactpoint != "") {
      message = message1 + "," + message2;
    }
    else {
      if (rcontactpoint != "") {
        message = message1;
      }
      else if (lcontactpoint != "") {
        message = message1;
      }
    }
    request->send_P(200, "text/text", message.c_str());
  });

  server.onNotFound(notFound);
  server.begin();

  update_display(); // This is needed to ensure no artifacting

  // The esp turns on pretty fast so the movement of releasing the on button causes the screen to start just slightly off of 0 occasionally.
  // This is both a cool artificial boot screen and a delay before zero'ing the dial to ensure no unwanted movement.
  if (bootOn) {
    boot_display();
  }

  // Draw border around screen if enabled
  if (drawBorder) {
    tft.fillScreen(background);
    tft.fillCircle(tftWidth / 2, tftHeight / 2, tftWidth / 2, borderColor);
    tft.fillCircle(tftWidth / 2, tftHeight / 2, int((tftWidth / 2) * (1 - borderThickness)), background);
  }

  baseLine = as5600.rawAngle(); // Zero out our encoder
  update_display();
}

void loop() {
  static uint32_t lastTime = 0;

  if (millis() - lastTime >= updateInterval) {
    lastTime = millis();
    sensorData = getInc();
    if (sensorData != prev_sensorData) {
      update_display();
    }
  }
}

// Figured I'd have one function that you can just uncomment/comment out different boot screens
void boot_display() {
  draw_dial(); // Draws a safe dial
  //draw_example2();
  //draw_example3();
  tft.fillScreen(background); // Clear screen
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
  tft.fillCircle(x, y, outer_ring, background);

  // Draw dial
  tft.fillCircle(x, y, dial_outer, foreground);
  tft.fillCircle(x, y, dial_inner, background);

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
        background);

    tft.fillTriangle(
        xEnd - offsetX, yEnd - offsetY,
        xEnd + offsetX, yEnd + offsetY,
        xStart + offsetX, yStart + offsetY,
        background);
  }

  // Draw knob
  tft.fillCircle(x, y, knob, foreground);

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
        background);

    tft.fillTriangle(
        xEnd - offsetX, yEnd - offsetY,
        xEnd + offsetX, yEnd + offsetY,
        xStart + offsetX, yStart + offsetY,
        background);
  }

  // Wait long enough to make sure we're not touching the encoder
  delay(2000);
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

  // Variables for centering on screen
  int16_t screenWidth = tftWidth;
  int16_t screenHeight = tftHeight;
  int textWidth = numberStr.length() * 6 * textSize; // 6 pixels per char
  int textHeight = 8 * textSize;                    // 8 pixels height per char
  int16_t x = (screenWidth - textWidth) / 2;
  int16_t y = (screenHeight - textHeight) / 2;

  // Erase previous text
  tft.setTextColor(background);
  tft.setCursor(prevX, prevY);
  tft.println(String(prev_sensorData));

  // Set cursor and display the text
  tft.setTextColor(foreground);
  tft.setCursor(x, y);
  tft.println(numberStr);

  // Draw pointer
  if (drawPointer) {
    // Erase previous pointer
    float angle = ((100 - prev_sensorData) * 3.6) - 90; // Make sure 0 is at the top of the screen and we're following 0 with the pointer
    prev_sensorData = sensorData;

    // Convert angle to radians
    float angleRad = radians(angle);
    
    int startX = tftWidth / 2;
    int startY = tftHeight / 2;

    // Calculate the starting point (18 pixels away from the center along the angle)
    if (drawBorder) {
      startX = (tftWidth / 2) + (((tftWidth / 2) * (1 - borderThickness)) * cos(angleRad));
      startY = (tftHeight / 2) + (((tftHeight / 2) * (1 - borderThickness)) * sin(angleRad));
    }

    // Calculate the other two points of the isosceles triangle
    // The acute angle will be split symmetrically on either side
    float halfAngle = radians(45 / 2); // Half the acute angle to split it evenly
    
    // Draw the isosceles triangle
    tft.fillTriangle(startX, startY, prev_x1, prev_y1, prev_x2, prev_y2, background);



    // Draw moving pointer
    angle = ((100 - sensorData) * 3.6) - 90; // Make sure 0 is at the top of the screen and we're following 0 with the pointer
    
    // Convert angle to radians
    angleRad = radians(angle);
    
    startX = tftWidth / 2;
    startY = tftHeight / 2;

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
    tft.fillTriangle(startX, startY, x1, y1, x2, y2, borderColor);
  }

  prevX = x;
  prevY = y;
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