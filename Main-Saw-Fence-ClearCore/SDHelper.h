#pragma once

#include <SD.h>

struct mechanismConfig {
  String type = "belt";  // "belt", "lead_screw", or "rack_pinion"

  // These fields vary based on the mechanism type
  float pulleyDiameter = 0;    // For belt
  float screwPitch = 0;        // For lead screw
  float pinionDiameter = 0;    // For rack and pinion

  String unit = "inches";             // "inches" or "millimeters"
  float gearboxReduction = 1; // Example: 1 for 1:1
};

struct systemConfig {
  int serialMonitorBaud = 115200; //default placeholders
  int screenBaud = 9600;
  int motorPulsesPerRevolution = 1000;
  String defaultUnits = "inches";
  String screenType = "giga_shield"; // "4d_systems" or "giga_shield"
  
  String mechanismType = "belt";
  mechanismConfig mechanismParams = mechanismConfig();
};


extern File myFile;
extern bool sdInit;

void initSDCard();


void writeSettings(systemConfig writeConfig);
systemConfig readSettings();