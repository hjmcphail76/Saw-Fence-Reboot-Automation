#define ARDUINOJSON_ENABLE_PROGMEM 0
#include "SDHelper.h"
#include <ArduinoJson.h>
#include "Utils.h"

File myFile;
bool sdInit = false;

void initSDCard() {
  Serial.println("Initializing SD card...");

  if (!SD.begin()) {
    Serial.println("SD Card initialization failed!");
    while (true) {}
  }

  if (SD.exists("/config.txt")) {
    Serial.println("initialization done.");
    sdInit = true;
  } else {
    Serial.println("init FAILED. File missing");
    while (true) {}
  }
}


void writeSettings(SystemConfig writeConfig) {
  if (sdInit) {
  }
}

SystemConfig readSettings() {
  SystemConfig config;

  if (!sdInit) {
    Serial.println("SD card not initialized!");
    return config;
  }

  myFile = SD.open("/config.txt", FILE_READ);
  if (!myFile) {
    Serial.println("Failed to open config.txt");
    return config;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, myFile);
  myFile.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return config;
  }

  JsonObject params = doc["mechanismParameters"].as<JsonObject>();

  // Assign values directly from JSON
  config.serialMonitorBaud = String(doc["serialMonitorBaud"] | "115200").toInt();
  config.screenBaud = String(doc["screenBaud"] | "9600").toInt();
  config.motorPulsesPerRevolution = String(doc["motorPulsesPerRevolution"] | "1000").toInt();
  config.defaultUnit = getUnitFromString(String(doc["defaultUnit"] | "Undefined"));
  config.screenType = String(doc["screenType"] | "giga_shield");
  config.mechanismType = String(doc["mechanism"] | "belt");
  config.motorShaftVel = String(doc["motorShaftVelocity"] | "1000").toInt();
  config.motorShaftAccel = String(doc["motorShaftAcceleration"] | "10000").toInt();

  if (!params.isNull()) {
    config.mechanismParams.unit1 = getUnitFromString(String(params["unit"] | "Undefined")); // Pulley/pitch/diameter unit

    config.mechanismParams.maxTravel = String(params["maxTravel"] | "0.0").toFloat();


    if (config.mechanismType == "belt") {
      config.mechanismParams.pulleyDiameter = String(params["pulleyDiameter"] | "0").toFloat();
      config.mechanismParams.gearboxReduction = String(params["gearboxReduction"] | "1").toFloat();

    } else if (config.mechanismType == "lead_screw") {
      config.mechanismParams.screwPitch = String(params["pitch"] | "0").toFloat();
      config.mechanismParams.gearboxReduction = String(params["gearboxReduction"] | "1").toFloat();

    } else if (config.mechanismType == "rack_pinion") {
      config.mechanismParams.pinionDiameter = String(params["pinionDiameter"] | "0").toFloat();
      config.mechanismParams.gearboxReduction = String(params["gearboxReduction"] | "1").toFloat();
    }
  }

  // Print out what's stored
  Serial.println();
  Serial.println("=== CONFIG LOADED ===");
  Serial.println("Serial Baud: " + String(config.serialMonitorBaud));
  Serial.println("Screen Baud: " + String(config.screenBaud));
  Serial.println("Motor Pulses/Rev: " + String(config.motorPulsesPerRevolution));
  Serial.println("Unit: " + String(getUnitString(config.defaultUnit)));
  Serial.println("Screen Type: " + config.screenType);
  Serial.println("Mechanism: " + config.mechanismType);
  Serial.println("Motor Shaft Velocity: " + String(config.motorShaftVel));
  Serial.println("Motor Shaft Acceleration: " + String(config.motorShaftAccel));

  if (config.mechanismType == "belt") {
    Serial.println("Pulley diameter: " + String(config.mechanismParams.pulleyDiameter));
    Serial.println("Gearbox reduction: " + String(config.mechanismParams.gearboxReduction));

  } else if (config.mechanismType == "lead_screw") {
    Serial.println("Leadscrew pitch: " + String(config.mechanismParams.screwPitch));
    Serial.println("Gearbox reduction: " + String(config.mechanismParams.gearboxReduction));
  } else if (config.mechanismType == "rack_pinion") {
    Serial.println("Pinion diameter: " + String(config.mechanismParams.pinionDiameter));
    Serial.println("Gearbox reduction: " + String(config.mechanismParams.gearboxReduction));
  }

  Serial.println("Max Travel: " + String(config.mechanismParams.maxTravel));

  Serial.println();

  return config;
}
