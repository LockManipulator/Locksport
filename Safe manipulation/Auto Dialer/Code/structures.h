#ifndef STRUCTURES
#define STRUCTURES

/*
Hopefully the memory management works.

Sincerely,
- A Python developer
*/

struct Wheel {
  float position;         // Current position
  String rotation;        // Rotation used to get to the current position
  String openRot;         // Rotation used to dial the combination
  float testNum;          // Number being tested while auto dialing (Unused)
  int* exclusions;        // Numbers to not try
  int exclusionCount;     // How many numbers to not try
  
  Wheel() : position(0), rotation(""), openRot(""), testNum(0), exclusions(nullptr), exclusionCount(0) {}
  
  ~Wheel() {
    if (exclusions) {
      delete[] exclusions;
    }
  }
  
  Wheel(const Wheel& other) {
    position = other.position;
    rotation = other.rotation;
    openRot = other.openRot;
    testNum = other.testNum;
    exclusionCount = other.exclusionCount;
    if (exclusionCount > 0) {
      exclusions = new int[exclusionCount];
      for (int i = 0; i < exclusionCount; i++) {
        exclusions[i] = other.exclusions[i];
      }
    } else {
      exclusions = nullptr;
    }
  }
  
  Wheel& operator=(const Wheel& other) {
    if (this != &other) {
      delete[] exclusions;
      position = other.position;
      rotation = other.rotation;
      openRot = other.openRot;
      testNum = other.testNum;
      exclusionCount = other.exclusionCount;
      if (exclusionCount > 0) {
        exclusions = new int[exclusionCount];
        for (int i = 0; i < exclusionCount; i++) {
          exclusions[i] = other.exclusions[i];
        }
      } else {
        exclusions = nullptr;
      }
    }
    return *this;
  }
};

struct SafeLock {
  float position;         // Current number on the dial
  String lastRotation;    // Last rotation used
  String openRot;         // Rotation used for opening
  int wheelCount;         // Number of wheels
  Wheel* wheels;          // Dynamic array for wheels
  float RCP;              // Right contact point
  float LCP;              // Left contact point
  float rotConversion;    // Rotational conversion amount per wheel
  float everyX;           // How many numbers to test wheels by during manipulation
  float openPast;         // How many numbers past a contact point to check for an open
  float avoidRange;       // How far apart each number in the combination should be

  SafeLock() : position(0), lastRotation(""), openRot(""), wheelCount(0), wheels(nullptr), RCP(0), LCP(0), rotConversion(0), everyX(0), openPast(0), avoidRange(0) {}

  ~SafeLock() {
    delete[] wheels;
  }

  // Copy constructor
  SafeLock(const SafeLock& other) {
    position = other.position;
    lastRotation = other.lastRotation;
    openRot = other.openRot;
    wheelCount = other.wheelCount;
    RCP = other.RCP;
    LCP = other.LCP;
    rotConversion = other.rotConversion;
    everyX = other.everyX;
    openPast = other.openPast;
    avoidRange = other.avoidRange;

    if (wheelCount > 0) {
      wheels = new Wheel[wheelCount];
      for (int i = 0; i < wheelCount; i++) {
        wheels[i] = other.wheels[i];
      }
    } else {
      wheels = nullptr;
    }
  }

  // Copy assignment operator
  SafeLock& operator=(const SafeLock& other) {
    if (this != &other) {
      delete[] wheels;

      position = other.position;
      lastRotation = other.lastRotation;
      openRot = other.openRot;
      wheelCount = other.wheelCount;
      RCP = other.RCP;
      LCP = other.LCP;
      rotConversion = other.rotConversion;
      everyX = other.everyX;
      openPast = other.openPast;
      avoidRange = other.avoidRange;

      if (wheelCount > 0) {
        wheels = new Wheel[wheelCount];
        for (int i = 0; i < wheelCount; i++) {
          wheels[i] = other.wheels[i];
        }
      } else {
        wheels = nullptr;
      }
    }
    return *this;
  }

  void addWheel(float position, String curRotation, String openRotation, float testNum, int* exclusions, int exclusionCount) {
    Wheel* newWheels = new Wheel[wheelCount + 1];
    if (!newWheels) {
      Serial.println("Memory allocation failed!");
      return;
    }

    for (int i = 0; i < wheelCount; i++) {
      newWheels[i] = wheels[i];  // Uses Wheel's copy assignment (deep copy)
    }
    delete[] wheels;
    wheels = newWheels;

    int index = wheelCount;
    wheels[index].position = position;
    wheels[index].rotation = curRotation;
    wheels[index].testNum = testNum;
    wheels[index].openRot = openRotation;
    wheels[index].exclusionCount = exclusionCount;

    if (exclusionCount > 0 && exclusions != nullptr) {
      wheels[index].exclusions = new int[exclusionCount];
      for (int i = 0; i < exclusionCount; i++) {
        wheels[index].exclusions[i] = exclusions[i];
      }
    } else {
      wheels[index].exclusions = nullptr;
      wheels[index].exclusionCount = 0;
    }

    wheelCount++;
  }

  void clearWheels() {
    delete[] wheels;
    wheels = nullptr;
    wheelCount = 0;
  }
};

#endif