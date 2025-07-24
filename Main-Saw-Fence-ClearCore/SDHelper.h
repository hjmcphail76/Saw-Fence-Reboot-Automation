#pragma once

#include <SD.h>
#include "Utils.h"

struct mechanismConfig {
  // These fields vary based on the mechanism type
  float pulleyDiameter = 0;    // For belt
  float screwPitch = 0;        // For lead screw
  float pinionDiameter = 0;    // For rack and pinion

  UnitType unit = UnitType::UNIT_UNKNOWN;
  float gearboxReduction = 1; // Example: 1 for 1:1
};
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include "SDHelper.h"
#include <ArduinoJson.h>

// File myFile;
// bool sdInit = false;


struct SystemConfig {
  int serialMonitorBaud = 115200; //default placeholders
  int screenBaud = 9600;
  int motorPulsesPerRevolution = 1000;
  UnitType defaultUnits = UnitType::UNIT_UNKNOWN;
  String screenType = "giga_shield"; // "4d_systems" or "giga_shield"

  int motorShaftVel = 1000;

  int motorShaftAccel = 20000;
  
  String mechanismType = "belt"; // "belt", "lead_screw", or "rack_pinion"
  mechanismConfig mechanismParams = mechanismConfig();
};


extern File myFile;
extern bool sdInit;

void initSDCard();


void writeSettings(SystemConfig writeConfig);
SystemConfig readSettings();