/*
This project is licensed under the PolyForm Noncommercial License 1.0.0. 
You may use, modify, and distribute this software for non-commercial purposes only.
See https://polyformproject.org/licenses/noncommercial/1.0.0 for details.

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

If speed, acceleration, deceleration, motor current, chopper off-time, or microstep is changed, you must test the CalibrateStall() code with a known combination!
StallGuard is very finicky and has to be tuned for many variables. The current calibration process will not work if some of these variables change.
*/

#include <Arduino.h>
#include "structures.h"
#include <WiFi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <AsyncEventSource.h>
#include <LittleFS.h>
#include <TMCStepper.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"

#define stepsPerRevolution 1600   // How many steps the motor moves for 1 full rotation
#define STEP_PIN 10               // Step pin
#define DIR_PIN 9                 // Direction pin
#define MISO_PIN 6                // MISO pin
#define MOSI_PIN 3                // MOSI pin
#define SCK_PIN 4                 // SCK pin
#define CS_PIN 5                  // Chip select pin
#define DIAG0_PIN 8               // Interrupt pin
#define DIAG1_PIN 7               // Position compare pin
#define R_SENSE 0.075f            // TMC5160T Pro is 0.075

// TMC5160 Register addresses, not used but here in case anyone wants them
#define REG_GCONF      0x00
#define REG_IHOLD_IRUN 0x10
#define REG_TPOWERDOWN 0x11
#define REG_TPWMTHRS   0x13
#define REG_RAMPMODE   0x20
#define REG_XACTUAL    0x21
#define REG_VMAX       0x27
#define REG_VSTART     0x23
#define REG_VSTOP      0x24
#define REG_AMAX       0x26
#define REG_DMAX       0x2A
#define REG_A1         0x24  
#define REG_XTARGET    0x2D
#define REG_CHOPCONF   0x6C
#define REG_PWMCONF    0x70

// Program state variables
bool start = false;                      // Variable to control starting dialing sequence
bool motorRunning = false;               // If motor is running
bool center = false;                     // If need to center dial
bool spin = false;                       // If need to spin dial
bool busy = false;                       // If currently doing something
volatile bool emergencyStop = false;     // If needing to suddenly stop (such as lock is open)
volatile bool stallGuardArmed = false;   // Whether StallGuard is active
volatile bool testingOpen = false;       // Whether we're trying to open the lock

// Motor variables (speeds are in microseconds)
int maxDelay = 700;                      // Slowest speed (in microseconds)   Default = 300
int minDelay = 200;                      // Fastest speed (coasting speed)    Default = 100
int totalSteps = 0;                      // Total steps to do
int stepsLeft = 0;                       // Steps left for the motor to move
int stepsDone = 0;                       // Keep track of where we are
unsigned long previousMotorTime = 0;     // Last time the motor moved
int speed = 3;                           // Speed
int stallVal = -4;                       // -64 to +63, automatically calibrated

// Auto dialing variables
float dropInPoint = 0;                   // Middle of the drop in area
bool checkDropIn = false;                // Whether to check the last wheel in the drop in area
float wheelOneStartPosition = 0;         // Start auto dialing with wheel one at this position
float* possibleNums = nullptr;           // Possible numbers in the combination
int possibleNumsCount = 0;               // How many possible numbers there are

//AP handling
const char* ssidap = "AutoDialer";       // Access point name
const char* passwordap = "AutoDialer";   // Access point password
String loginname = "root";               // Login page username
String loginpassword = "toor";           // Login page password
IPAddress local_ip(192, 168, 0, 1);      // Go to this IP address to control device
IPAddress gateway(192, 168, 0, 1);       // Gateway
IPAddress subnet(255, 255, 255, 0);      // Subnet
AsyncWebServer server(80);               // Webserver
AsyncEventSource events("/events");      // SSE endpoint
unsigned long previousSendTime = 0;      // Last time data was sent to the webserver
String header;

TMC5160Stepper driver(CS_PIN, R_SENSE, MOSI_PIN, MISO_PIN, SCK_PIN);  // Create driver
SafeLock* lock = new SafeLock;                                        // Create lock object

// Called when stepper stalls
void OnStall() {
  SendStatusSimple("EMERGENCY STOP");
  SendLog("EMERGENCY STOP");
  emergencyStop = true;
}

// Setup
void setup() {
  LittleFS.begin();
  WifiSetup(ssidap, passwordap);

  events.onConnect([](AsyncEventSourceClient* client) {
    client->send("", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Handles main web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!request->authenticate((char*)loginname.c_str(), (char*)loginpassword.c_str())) {
      return request->requestAuthentication();
    }
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Handles serving the css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/styles.css", "text/css");
  });

  // Handles serving the js file
  server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.js", "application/javascript");
  });

  // Centers the dial on 0
  server.on("/centerdial", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!busy) {
      request->send(200, "text/plain", "true");
      SendLog("Centering dial...");
      String num_now = request->getParam("center")->value();
      float offset = round(num_now.toFloat() * (float)stepsPerRevolution) / (float)stepsPerRevolution;
      lock->position = offset;
      center = true;
    } 
    else {
      request->send(200, "text/plain", "false");
    }
  });

  // Stops everything
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "true");
    emergencyStop = true;
    SendStatusSimple("EMERGENCY STOP");
  });

  // Spins back and forth
  server.on("/spin", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!busy) {
      request->send(200, "text/plain", "true");
      spin = true;
    } 
    else {
      request->send(200, "text/plain", "false");
    }
  });

  // Starts auto dialing
  server.on("/start", HTTP_POST, [](AsyncWebServerRequest* request){}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (busy) {
        request->send(200, "text/plain", "false");
        return;
      }

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, data, len);
      if (error) {
        request->send(400, "text/plain", "Invalid JSON");
        return;
      }

      // Get variables
      lock->clearWheels();
      lock->RCP = doc["rcp"].as<float>();
      lock->LCP = doc["lcp"].as<float>();
      lock->everyX = doc["everyX"].as<float>();
      lock->openRot = doc["openRot"].as<String>();
      lock->rotConversion = doc["rotConversion"].as<float>();
      lock->openPast = doc["openDistance"].as<float>();
      lock->avoidRange = doc["avoidWithin"].as<float>();

      // Add wheels
      JsonArray wheelsArray = doc["wheels"].as<JsonArray>();
      for (JsonObject w : wheelsArray) {
        String openRot = w["openRot"].as<String>();
        JsonArray excArr = w["exclusions"].as<JsonArray>();
        int exclusionCount = excArr.size();
        int* exclusions = new int[exclusionCount];
        for (int i = 0; i < exclusionCount; i++) {
          exclusions[i] = excArr[i].as<int>();
        }
        lock->addWheel(0, "L", openRot, 0, exclusions, exclusionCount);
        delete[] exclusions;
      }

      // Get drop in point
      float dropInWidth = GetDistance(lock->LCP, lock->RCP, "L") / stepsPerRevolution;
      dropInPoint = NormalizeNum(lock->RCP - (dropInWidth / 2.0));
      if (doc["dropCheck"].as<String>() == "Y") {
        checkDropIn = true;
      }

      // Free previous array if exists
      if (possibleNums != nullptr) {
        delete[] possibleNums;
        possibleNums = nullptr;
      }

      // Parse possible known numbers
      JsonArray numsArr = doc["possibleNums"].as<JsonArray>();
      possibleNumsCount = numsArr.size();
      if (possibleNumsCount > 0) {
        possibleNums = new float[possibleNumsCount];
        for (int i = 0; i < possibleNumsCount; i++) {
          possibleNums[i] = numsArr[i].as<float>();
        }
      }

      // Get wheel 1 start position
      wheelOneStartPosition = doc["w1start"].as<float>();

      // Get speed
      speed = doc["speed"].as<int>();

      if (speed == 1) {
        maxDelay = 1100;
        minDelay = 300; 
      }
      else if (speed == 2) {
        maxDelay = 900;
        minDelay = 250; 
      }
      else if (speed == 3) {
        // Any faster and steps will be skipped
        maxDelay = 700;
        minDelay = 200; 
      }

      request->send(200, "text/plain", "true");
      start = true;
    }
  );

  server.onNotFound(NotFound);
  server.begin();

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  driver.begin();
  driver.toff(6);             // Chopper slow decay off-time. Affects StallGuard so change with caution.
  driver.rms_current(2200);   // Motor current. Running a bit low to reduce heat. Affects StallGuard so change with caution.
  driver.microsteps(8);       // 16 microstep (1,600 steps per revolution).
  driver.en_pwm_mode(false);  // SpreadCycle. Required for StallGuard on TMC5160.

  // StallGuard config
  driver.TCOOLTHRS(0xFFFFF);  // Enable StallGuard at all speeds (tune this down later)
  driver.sgt(stallVal);       // (SGT, -64 to +63 — start around 10, not 100)
  driver.sfilt(1);            // Optional: filter SG result for stability

  // Route stall to DIAG0 pin
  driver.diag0_stall(1);        // Assert DIAG0 on stall
  driver.diag0_int_pushpull(1); // Push-pull output for interrupt

  // Pin setup
  pinMode(STEP_PIN, OUTPUT);    // Step pin
  pinMode(DIR_PIN, OUTPUT);     // Direction pin
  pinMode(DIAG0_PIN, INPUT);    // Interrupt pin
  attachInterrupt(digitalPinToInterrupt(DIAG0_PIN), OnStall, RISING);

  // Disable StallGuard temporarily
  detachInterrupt(digitalPinToInterrupt(DIAG0_PIN));
  driver.TCOOLTHRS(0);
}

// Checks for what task to do next
void loop() {
  if (start && !busy) {
    start = false;
    busy = true;
    emergencyStop = false;    

    SendLog("Calibrating stall...");
    CalibrateStall();

    SendLog("Starting auto dialing");
    LogWheels();
    SendLog("Speed: " + String(speed));
    SendLog("Lock open rotation: " + lock->openRot);
    SendLog("Check drop-in: " + String(checkDropIn ? "True" : "False"));
    SendLog("RCP: " + String(lock->RCP));
    SendLog("LCP: " + String(lock->LCP));
    SendLog("Check every 'x': " + String(lock->everyX));
    SendLog("Avoid range: " + String(lock->avoidRange));
    SendLog("Open past by: " + String(lock->openPast));
    SendLog("Rotational conversion: " + String(lock->rotConversion));

    if (possibleNumsCount > 0) {
      TryCombinations(possibleNums, possibleNumsCount);
    }

    if (!emergencyStop) {
      AutoDial();
    }
  
    busy = false;
  }
  if (center && !busy) {
    center = false;
    busy = true;
    emergencyStop = false;

    // Spin the shortest distance to 0
    if (lock->position <= 50) {
      Move("R", lock->position);
    }
    else {
      Move("L", 100.0 - lock->position);
    }

    lock->position = 0;
    busy = false;
  }
  if (spin && !busy) {
    spin = false;
    busy = true;
    emergencyStop = false;

    // Wiggle dial in both directions to make sure there's no slippage
    Move("L", 25);
    Move("R", 50);
    Move("L", 25);

    busy = false;
  }
}

// Handles 404 webpage
void NotFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

// Handles setting up the AP
void WifiSetup(const char* name, const char* pass) {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  if (pass == "") {
    WiFi.softAP(name, NULL);
  } 
  else {
    WiFi.softAP(name, pass);
  }
}

// Send message to webserver
void SendLog(String message) {
  events.send(message.c_str(), "log", millis());
}

// Sends wheels config to web server
void LogWheels() {
  SendLog("Wheels");
  SendLog("--------");

  for (int x = 0; x < lock->wheelCount; x++) {
    SendLog("Wheel " + String(x + 1));
    SendLog("Open rotation: " + lock->wheels[x].openRot);
    
    String wheelMsg = "Exclusions: ";
    for (int y = 0; y < lock->wheels[x].exclusionCount; y++) {
      wheelMsg += String(lock->wheels[x].exclusions[y]);
      if (y < lock->wheels[x].exclusionCount - 1) {
        wheelMsg += ", ";
      }
    }
    SendLog(wheelMsg);
  }
}

// Send status update to web server
void SendStatus(String status, float* wheelNums, int wheelCount, int current, int total) {
  String json = "{\"status\":\"" + status + "\",\"wheels\":[";
  for (int i = 0; i < wheelCount; i++) {
    json += String(wheelNums[i], 1);
    if (i < wheelCount - 1) json += ",";
  }
  json += "],\"current\":" + String(current) + ",\"total\":" + String(total) + "}";
  events.send(json.c_str(), "status", millis());
}

// Send status with no wheel numbers (waiting / emergency stop)
void SendStatusSimple(String status) {
  String json = "{\"status\":\"" + status + "\",\"wheels\":[],\"current\":0,\"total\":0}";
  events.send(json.c_str(), "status", millis());
}

// Normalizes numbers to within the 0-100 dial range
float NormalizeNum(float num) {
  while (num >= 100) {
    num -= 100;
  }
  while (num < 0) {
    num += 100;
  }
  return num;
}

// Get acceleration curve
float CalculateSCurve(float progress) {
  // Quintic smoothstep - very gentle at start/end, aggressive in middle
  return progress * progress * progress * (progress * (progress * 6.0 - 15.0) + 10.0);
}

// Moves the motor and keeps track of dial position
void MoveMotor(String direction, int steps) {
  if (steps == 0) {
    return;
  }

  totalSteps = steps;
  stepsLeft = steps;
  int currentDelay;
  motorRunning = true;
  stallGuardArmed = false;
  
  // Calculate acceleration steps (10 increments)
  int accelSteps = (stepsPerRevolution / 100) * 10;
  // Ensure we don't try to accelerate longer than half the move
  if (accelSteps > totalSteps / 2) {
    accelSteps = totalSteps / 2;
  }
  
  unsigned long startTime = millis();
  while (stepsLeft > 0 && motorRunning) {
    if (emergencyStop) {
      stepsLeft = 0;
      detachInterrupt(digitalPinToInterrupt(DIAG0_PIN));
      driver.TCOOLTHRS(0);
      stallGuardArmed = false;
      motorRunning = false;
      return;
    }
    // Run motor with S-curve acceleration
    int currentStep = totalSteps - stepsLeft;
    
    // Enable StallGuard after x steps and only if we're testing an open
    if (testingOpen && !stallGuardArmed && stepsPerRevolution / 50 >= 0 && currentStep >= stepsPerRevolution / 50) {
        driver.GSTAT(0b111);
        delayMicroseconds(100);
        stallGuardArmed = true;
        attachInterrupt(digitalPinToInterrupt(DIAG0_PIN), OnStall, RISING);
        driver.TCOOLTHRS(0xFFFFF);
    }

    // Calculate current delay based on S-curve acceleration profile
    if (currentStep < accelSteps) {
      // Acceleration phase - S-curve ramp up
      float progress = (float)currentStep / accelSteps;
      progress = CalculateSCurve(progress);  // Apply S-curve
      currentDelay = maxDelay - (maxDelay - minDelay) * progress;
      
    } else if (stepsLeft < accelSteps) {
      // Deceleration phase - S-curve ramp down
      float progress = (float)stepsLeft / accelSteps;
      progress = CalculateSCurve(progress);  // Apply S-curve
      currentDelay = maxDelay - (maxDelay - minDelay) * progress;
      
    } 
    else {
      // Constant speed phase
      currentDelay = minDelay;
    }
    
    // If it's time to move the motor
    if (micros() - previousMotorTime >= currentDelay) {
      if (stepsLeft > 0) {
        stepsLeft -= 1;
        lock->lastRotation = direction;
        
        // Set motor direction. Change R/L if direction is reversed.
        if (direction == "R") {
          digitalWrite(DIR_PIN, HIGH);
        } 
        else {
          digitalWrite(DIR_PIN, LOW);
        }
        
        // Generate step pulse
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(3);  // Minimum pulse width
        digitalWrite(STEP_PIN, LOW);
        
        previousMotorTime = micros();
        stepsDone += 1;
        
        // Reset step count once we hit a full number
        if (stepsDone >= stepsPerRevolution / 100) {
          stepsDone = 0;
        }
        
        // If we pass zero going in either direction then wrap back around
        if (direction == "L") {
          lock->position += 100.0 / (float)stepsPerRevolution;
          if (lock->position >= 100.0) {
            lock->position = NormalizeNum(lock->position);
          }
        } 
        else {
          lock->position -= 100.0 / (float)stepsPerRevolution;
          if (lock->position < 0.0) {
            lock->position = NormalizeNum(lock->position);
          }
        }
      }
    }
  }

  // Disable StallGuard
  detachInterrupt(digitalPinToInterrupt(DIAG0_PIN));
  driver.TCOOLTHRS(0);
  stallGuardArmed = false;
  motorRunning = false;
}

// Returns steps to do from num to num with given direction
float GetDistance(float fromNum, float toNum, String direction) {
  float distance = 0.0;
  
  if (direction == "L") {
    if (fromNum <= toNum) {
      distance = toNum - fromNum;
    }
    else {
      distance = (100.0 - fromNum) + toNum;
    }
  }
  else {
    if (fromNum >= toNum) {
      distance = fromNum - toNum;
    }
    else {
      distance = fromNum + (100.0 - toNum);
    }
  }
  
  return distance * ((float)stepsPerRevolution / 100.0);
}

// If a number is between 2 numbers
bool InRange(float checkNum, float startNum, float endNum, String direction) {
  const float DIAL_SIZE = 100.0;
  checkNum = fmod(fmod(checkNum, DIAL_SIZE) + DIAL_SIZE, DIAL_SIZE);
  startNum = fmod(fmod(startNum, DIAL_SIZE) + DIAL_SIZE, DIAL_SIZE);
  endNum   = fmod(fmod(endNum,   DIAL_SIZE) + DIAL_SIZE, DIAL_SIZE);

  if (direction == "L") {
    if (startNum <= endNum) {
      return checkNum >= startNum && checkNum <= endNum;
    } 
    else {
      return checkNum >= startNum || checkNum <= endNum;
    }
  } 
  else if (direction == "R") {
    if (startNum >= endNum) {
      return checkNum <= startNum && checkNum >= endNum;
    } 
    else {
      return checkNum <= startNum || checkNum >= endNum;
    }
  }

  return false;
}

// Checks if a wheel will be hit
bool CollisionCheck(int wheel, float targetPosition, String direction) {
  // Change wheel number to wheel index
  wheel -= 1;

  // Wheel 1 can't ever hit anything
  if (wheel == 0) {
    return false;
  }

  float distanceToNextWheel = GetDistance(lock->wheels[wheel].position, lock->wheels[wheel - 1].position, direction); // How far to next wheel
  float distanceToTarget = GetDistance(lock->wheels[wheel].position, targetPosition, direction);                      // How far to target position

  // If wheels are at the same place
  if (distanceToNextWheel == 0) {
    // It will collide only if the wheel was set with the same direction we are moving in
    if (lock->wheels[wheel].rotation == direction) {
      return true;
    }
    else {
      return false;
    }
  }
  // If wheels are not at the same place
  else if (distanceToNextWheel < distanceToTarget) {
    // Then it will collide if the next wheel is closer than the target
    if (direction == lock->wheels[wheel - 1].rotation) {
      return true;
    }
    else {
      int wheelsToNextWheel = lock->wheelCount - wheel + 1;                   // How many wheels are picked up when going to the next wheel
      float rotationalConversion = lock->rotConversion * wheelsToNextWheel;   // Get the rotational conversion amount
      float nextWheelPosition;                                                // The actual location of the next wheel

      // Next wheel location will be higher if going left
      if (direction == "L") {
        nextWheelPosition = NormalizeNum(lock->wheels[wheel - 1].position + rotationalConversion);
      }
      // Next wheel location will be lower if going right
      else {
        nextWheelPosition = NormalizeNum(lock->wheels[wheel - 1].position - rotationalConversion);
      }

      // The actual location of the next wheel
      distanceToNextWheel = GetDistance(lock->wheels[wheel].position, nextWheelPosition, direction);

      // It will collide if the next wheel is closer than the target
      if (distanceToNextWheel < distanceToTarget) {
        return true;
      }
      else {
        return false;
      }
    }
  }
  else {
    return false;
  }
}

// Sets a given wheel to a given position with a given direction. Ensures no next wheels are disturbed.
void SetWheel(int wheel, float targetPos, String direction) {
  wheel -= 1;   // Convert wheel number to wheel index

  if (wheel < 0) {
    return;
  }

  // If the wheel is already where it needs to be, do nothing
  if (lock->wheels[wheel].position == targetPos && lock->wheels[wheel].rotation == direction) {
    return;
  }

  // If we would hit the next wheel
  bool collision = CollisionCheck(wheel + 1, targetPos, direction);
  if (collision) {
    String tempDirection = direction == "L" ? "R" : "L";           // We must go the "wrong" way
    int wheelsToNextWheel = lock->wheelCount - wheel + 1;          // Next wheel is the xth wheel to pick up
    float minOffset = lock->rotConversion * (wheelsToNextWheel);   // Minimal amount to go past target
    float tempTargetPos;                                           // Temporary place we set the target wheel to

    // Go past by the smallest amount possible just in case the next wheel is in the same place with opposite rotaiton
    if (tempDirection == "L") {
      tempTargetPos = NormalizeNum(targetPos + minOffset);
    }
    else {
      tempTargetPos = NormalizeNum(targetPos - minOffset);
    }

    SetWheel(wheel + 1, tempTargetPos, tempDirection);
  }

  // Put a wheel where it needs to be, standard dialing because there's no collisions
  float distance = 0;                // Total steps to do
  float tempPos = lock->position;    // For keeping track of where we are, starting with the current number on the dial

  // Pick up all wheels up to and including the target wheel
  for (int x = lock->wheelCount - 1; x >= wheel; x--) {
    // Current wheel is the wheel we're picking up
    float currentWheelPosition = lock->wheels[x].position;                                  // Position of the current wheel *in it's set rotation*
    float distanceToCurrentWheel = GetDistance(tempPos, currentWheelPosition, direction);   // How far to current wheel
    float rotConversionInc = (float)(lock->wheelCount - x) * lock->rotConversion;           // Rotational conversion amount for current wheel in increments
    float rotConversion = rotConversionInc * (stepsPerRevolution / 100.0);                  // Rotational conversion amount for current wheel in steps
    bool rotConvUsed = false;

    // If wheels are in the same place
    if (distanceToCurrentWheel == 0) {
      // If current wheel is the last wheel
      if (x == lock->wheelCount - 1) {
        // Then it's only 1 full rotation away + rotational conversion if dial is switching rotations
        if (direction != lock->lastRotation) {
          distanceToCurrentWheel += stepsPerRevolution + rotConversion;
          rotConvUsed = true;
        }
      }
      // If current wheel is not the last wheel
      else {
        // Then it's only 1 full rotation away + rotational conversion if the wheel we have is switching rotations
        if (direction != lock->wheels[x + 1].rotation) {
          distanceToCurrentWheel += stepsPerRevolution + rotConversion;
          rotConvUsed = true;
        }
      }
    }
    // If the wheels are not in the same place and the next wheel was set in the opposite direction than we're going
    else if (lock->wheels[x].rotation != direction) {
      // Add rotational conversion
      distanceToCurrentWheel += rotConversion;
      rotConvUsed = true;
    }

    // If rotational conversion was used
    if (rotConvUsed) {
      // Get it's position in our direction of rotation to update our position
      if (direction == "L") {
        // If turning left, we have to turn x numbers higher
        currentWheelPosition = NormalizeNum(currentWheelPosition + rotConversionInc);
      }
      else {
        // If turning right, we have to turn x numbers lower
        currentWheelPosition = NormalizeNum(currentWheelPosition - rotConversionInc);
      }
    }
    distance += distanceToCurrentWheel;   // Go to next wheel
    tempPos = currentWheelPosition;       // We are now at the wheel we just picked up
  }

  float distToTarget = GetDistance(tempPos, targetPos, direction);      // How far target wheel is from target position
  distance += distToTarget;         // Go to the target position with the target wheel
  MoveMotor(direction, distance);   // Do the full move

  // Update wheel positions to where the dial currently is
  for (int x = lock->wheelCount - 1; x >= wheel; x--) {
    lock->wheels[x].position = lock->position;
    lock->wheels[x].rotation = lock->lastRotation;
  }

  // If target wheel did not end up where it's supposed to be with the correct rotation
  if (lock->wheels[wheel].position != targetPos || lock->wheels[wheel].rotation != direction) {
    SendLog("Wheel position lost!");
    //emergencyStop = true;
  }
}

// Move by increments (DANGER: Does not update wheel positions!)
void Move(String direction, float incs) {
  MoveMotor(direction, incs * (stepsPerRevolution / 100));
}

// Move by full rotations (DANGER: Does not update wheel positions!)
void Rotate(String direction, int rotations) {
  MoveMotor(direction, rotations * stepsPerRevolution);
}

// Go to a number (DANGER: Does not update wheel positions!)
void GoTo(String direction, float num) {
  while (num < 0) num += 100.0;
  while (num >= 100.0) num -= 100.0;

  float steps_to_do = GetDistance(lock->position, num, direction);
  MoveMotor(direction, (int)steps_to_do);
}

// Calibrate value for StallGuard
void CalibrateStall() {
  int buffer = 4;

  SendStatusSimple("Calibrating");

  // Pick up all wheels to calibrate stall value with the weight of all wheels
  Rotate(lock->openRot, lock->wheelCount);
  
  // Test different stall values
  testingOpen = true;
  for (int x = 50; x >= -64; x -= 2) {
    SendLog("Testing: " + String(x));
    driver.sgt(x);

    // Move the same way to calibrate stall as when detecting an open
    Move(lock->openRot, lock->openPast);

    // Set it to just above where it stalled
    if (emergencyStop) {
      stallVal = x + buffer;
      SendLog("Stall value: " + String(stallVal));
      driver.sgt(stallVal);
      emergencyStop = false;
      break;
    }
  }

  testingOpen = false;
  GoTo("L", 0);
  delay(1000);
}

// Checks if the current combination to test is valid
bool ValidCombo(float* nums, int count) {
  if (count < 2) return false;

  // Rule 1: Consecutive numbers can't be within avoidRange of each other
  for (int i = 0; i < count - 1; i++) {
    float diff = fabs(nums[i] - nums[i + 1]);
    if (diff > 50.0) diff = 100.0 - diff;
    if (diff < lock->avoidRange) return false;
  }

  // Rule 2: If checkDropIn is false, last number can't be between LCP and RCP
  if (!checkDropIn) {
    if (InRange(nums[count - 1], lock->LCP, lock->RCP, "L")) return false;
  }

  // Rule 3: xth number can't be in xth wheel's exclusion list
  for (int i = 0; i < count && i < lock->wheelCount; i++) {
    float num = nums[i];
    int floorNum = (int)floor(num);
    int ceilNum  = (int)ceil(num);
    bool isFloat = (num != (float)floorNum);

    int* excl = lock->wheels[i].exclusions;
    int exclCount = lock->wheels[i].exclusionCount;

    if (isFloat) {
      bool floorExcluded = false;
      bool ceilExcluded  = false;
      for (int j = 0; j < exclCount; j++) {
        if (excl[j] == floorNum) floorExcluded = true;
        if (excl[j] == ceilNum)  ceilExcluded  = true;
      }
      if (floorExcluded && ceilExcluded) return false;
    } else {
      for (int j = 0; j < exclCount; j++) {
        if (excl[j] == floorNum) return false;
      }
    }
  }

  return true;
}

// Checks if the lock is open after each combination
void TestOpen() {
  // Go to drop-in area
  if (lock->wheels[lock->wheelCount - 1].openRot == "L") {
    GoTo("R", dropInPoint);
  }
  else if (lock->wheels[lock->wheelCount - 1].openRot == "R") {
    GoTo("L", dropInPoint);
  }

  // Try to open the lock
  if (lock->openRot == "R") {
    float targetPos = NormalizeNum(lock->LCP - lock->openPast);
    float tempPos = lock->position;

    // Update wheels if any are picked up from trying to open the lock
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      if (InRange(lock->wheels[x].position, tempPos, targetPos, "R")) {
        lock->wheels[x].position = targetPos;
        lock->wheels[x].rotation = "R";
        tempPos = lock->wheels[x].position;
      }
      else {
        break;
      }
    }
    testingOpen = true;
    GoTo("R", targetPos);
    testingOpen = false;
  }
  else if (lock->openRot == "L") {
    float targetPos = NormalizeNum(lock->RCP + lock->openPast);\
    float tempPos = lock->position;

    // Update wheels if any are picked up from trying to open the lock
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      if (InRange(lock->wheels[x].position, tempPos, targetPos, "L")) {
        lock->wheels[x].position = targetPos;
        lock->wheels[x].rotation = "L";
        tempPos = lock->wheels[x].position;
      }
      else {
        break;
      }
    }

    testingOpen = true;
    GoTo("L", targetPos);
    testingOpen = false;
  }
  // If opening direction is unknown, try both
  else {
    float lastWheelPos = lock->wheels[lock->wheelCount - 1].position;   // Save last wheel's position in case we mess it up
    float targetPos = NormalizeNum(lock->LCP - lock->openPast);
    float tempPos = lock->position;

    // Update wheels if any are picked up from trying to open the lock
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      if (InRange(lock->wheels[x].position, tempPos, targetPos, "R")) {
        lock->wheels[x].position = targetPos;
        lock->wheels[x].rotation = "R";
        tempPos = lock->wheels[x].position;
      }
      else {
        break;
      }
    }
    testingOpen = true;
    GoTo("R", targetPos);
    testingOpen = false;

    // Reset last wheel in case we moved it
    SetWheel(lock->wheelCount - 1, lastWheelPos, lock->wheels[lock->wheelCount - 1].openRot);

    // Go to drop-in area
    if (lock->lastRotation == "L") {
      GoTo("R", dropInPoint);
    }
    else if (lock->lastRotation == "R") {
      GoTo("L", dropInPoint);
    }

    targetPos = NormalizeNum(lock->RCP + lock->openPast);
    tempPos = lock->position;

    // Update wheels if any are picked up from trying to open the lock
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      if (InRange(lock->wheels[x].position, tempPos, targetPos, "L")) {
        lock->wheels[x].position = targetPos;
        lock->wheels[x].rotation = "L";
        tempPos = lock->wheels[x].position;
      }
      else {
        break;
      }
    }

    testingOpen = true;
    GoTo("L", targetPos);
    testingOpen = false;
  }
}

long long TotalPermutations(int n, int r) {
  long long result = 1;
  for (int i = 0; i < r; i++) {
    result *= (n - i);
  }
  return result;
}

// Permutates through user given possible numbers
void SinglePermutations(float* nums, int count, float* current, bool* used, int depth, long long& currentTry, long long totalTries) {
  if (emergencyStop) {
    return;
  }

  // Stop at lock->wheelCount length of combination
  if (depth == lock->wheelCount) {
    currentTry++;

    // Build combo string for status
    String comboStr = "";
    for (int x = 0; x < lock->wheelCount; x++) {
      comboStr += String(current[x], 1);
      if (x < lock->wheelCount - 1) comboStr += " - ";
    }
    SendStatus("Trying " + comboStr, current, lock->wheelCount, currentTry, totalTries);

    // Set each wheel to its position with its opening rotation
    for (int x = 0; x < lock->wheelCount; x++) {
      SetWheel(x + 1, current[x], lock->wheels[x + 1].openRot);
    }
    return;
  }

  // Build current combination
  for (int i = 0; i < count; i++) {
    
    // If number has not been used yet
    if (!used[i]) {
      used[i] = true;                                                                     // Mark it used
      current[depth] = nums[i];                                                           // Add it to combo
      SinglePermutations(nums, count, current, used, depth + 1, currentTry, totalTries);  // Test combo
      used[i] = false;                                                                    // Mark as unused for future permutations
    }
  }
}

// Try user given possible combinations before auto dialing
void TryCombinations(float* nums, int count) {
  if (count < lock->wheelCount) {
    return;
  }
  float* current = new float[lock->wheelCount];
  bool* used = new bool[count];

  // Mark each number as unused first
  for (int i = 0; i < count; i++) {
    used[i] = false;
  }

  long long currentTry = 0;
  long long totalTries = TotalPermutations(count, lock->wheelCount);
  SinglePermutations(nums, count, current, used, 0, currentTry, totalTries);
  
  delete[] current;
  delete[] used;
}

// Starts auto dialing
void AutoDial() {
  float startNum = 0;                       // Starting number for incrementing combination
  int maxTries = (int)(100 / lock->everyX); // How many possible numbers to try per wheel
  float curCombo[lock->wheelCount];         // Current combination
  int numsTried[lock->wheelCount];          // How many iterations a wheel has gone through

  // Initialize wheel positions
  for (int x = 0; x < lock->wheelCount; x++) {
    curCombo[x] = startNum;
    numsTried[x] = 0;
  }

  // Count total amount of possibilities
  int count = 0;
  int iterCount = 0;
  bool done = false;
  while (!done) {
    if (ValidCombo(curCombo, lock->wheelCount)) {
      count += 1;
    }

    // Feed the watchdog every 1000 iterations
    iterCount++;
    if (iterCount >= 5000) {
      iterCount = 0;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // Iterate through wheels
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      // Increment left turning wheels up
      if (lock->wheels[x].openRot == "L") {
        curCombo[x] = NormalizeNum(curCombo[x] + lock->everyX);
      } 
      // Increment right turning wheels down
      else {
        curCombo[x] = NormalizeNum(curCombo[x] - lock->everyX);
      }
      numsTried[x] += 1;

      // If we haven't exhausted all possibilities for this wheel, exit
      if (numsTried[x] < maxTries) {
        break;
      } 
      // Otherwise reset it and move onto the next wheel
      else {
        // If this was the first wheel, we've done them all
        if (x == 0) {
          done = true;
          break;
        }
        curCombo[x] = startNum;
        numsTried[x] = 0;
      }
    }
  }
  SendLog("Total combinations: " + String(count));
  SendStatusSimple("Waiting");

  // Reset current combination
  done = false;
  int current = 0;
  for (int x = 0; x < lock->wheelCount; x++) {
    curCombo[x] = startNum;
    numsTried[x] = 0;
  }

  // Make sure all wheels are on 0
  Rotate(lock->wheels[0].openRot, lock->wheelCount);

  curCombo[0] = wheelOneStartPosition;      // Start wheel on value from webpage

  // Dew it
  while (!done) {
    if (emergencyStop) {
      return;
    }

    // If combination is valid
    if (ValidCombo(curCombo, lock->wheelCount)) {
      current++;

      /*
      // Send each combination to web page. Can cause lag.
      String logMsg = "Trying: ";
      for (int x = 0; x < lock->wheelCount; x++) {
        logMsg += String(curCombo[x]);
        if (x < lock->wheelCount - 1) logMsg += " - ";
      }
      SendLog(logMsg);
      */

      // Build combo string for status
      String comboStr = "";
      for (int x = 0; x < lock->wheelCount; x++) {
        comboStr += String(curCombo[x], 1);
        if (x < lock->wheelCount - 1) comboStr += " - ";
      }
      SendStatus("Trying " + comboStr, curCombo, lock->wheelCount, current, count);
      
      // Dial combination
      for (int x = 0; x < lock->wheelCount; x++) {
        SetWheel(x + 1, curCombo[x], lock->wheels[x].openRot);
      }
      TestOpen();   // Test if lock is open
    }

    // Increment combination
    for (int x = lock->wheelCount - 1; x >= 0; x--) {
      // Make sure we're incrementing in the correct direction
      if (lock->wheels[x].openRot == "L") {
        curCombo[x] = NormalizeNum(curCombo[x] + lock->everyX);
      } 
      else {
        curCombo[x] = NormalizeNum(curCombo[x] - lock->everyX);
      }
      numsTried[x] += 1;

      // If we haven't exhausted all possibilities for this wheel, exit
      if (numsTried[x] < maxTries) {
        break;
      } 
      // Otherwise reset it and move onto the next wheel
      else {
        // If this was the first wheel, we've done them all
        if (x == 0) {
          done = true;
          SendStatusSimple("Waiting");
          break;
        }
        curCombo[x] = startNum;
        numsTried[x] = 0;
      }
    }

    // Make sure we start bruteforcing a wheel from the next wheel's position (ignoring wheel 1 for this) for minimal dialing
    for (int x = lock->wheelCount - 1; x > 0; x--) {
      // If a wheel was just reset
      if (numsTried[x] == 0) {
        // Start it from the next wheel's position
        if (lock->wheels[x].openRot == "L") {
          curCombo[x] = NormalizeNum(curCombo[x - 1] + lock->everyX);
        }
        else {
          curCombo[x] = NormalizeNum(curCombo[x - 1] - lock->everyX);
        }
      }
    }
  }

  SendLog("Auto dialing finished");
}