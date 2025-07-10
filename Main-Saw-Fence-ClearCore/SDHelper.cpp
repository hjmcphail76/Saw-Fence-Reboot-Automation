#define ARDUINOJSON_ENABLE_PROGMEM 0
#include "SDHelper.h"
#include <ArduinoJson.h>

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


void writeSettings(systemConfig writeConfig) {
  if (sdInit) {
  }
}

systemConfig readSettings() {
  systemConfig config;

  if (!sdInit) {
    Serial.println("SD card not initialized!");
    return config;
  }

  File myFile = SD.open("/config.txt", FILE_READ);
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

  const char* serialBaud = doc["serialMonitorBaud"] | "115200";
  const char* screenBaud = doc["screenBaud"] | "9600";
  const char* pulses = doc["motorPulsesPerRevolution"] | "1000";
  const char* units = doc["defaultUnits"] | "inches";
  const char* screenType = doc["screenType"] | "giga_shield";
  const char* mech = doc["mechanism"] | "belt";

  JsonObject params = doc["mechanismParameters"];


  Serial.println();
  Serial.println("=== CONFIG LOADED ==="); 
  Serial.println("Serial Baud: " + String(serialBaud));
  Serial.println("Screen Baud: " + String(screenBaud));
  Serial.println("Motor Pulses/Rev: " + String(pulses));
  Serial.println("Units: " + String(units));
  Serial.println("Screen Type: " + String(screenType));
  Serial.println("Mechanism: " + String(mech));
  Serial.println();

  config.serialMonitorBaud = atoi(serialBaud);
  config.screenBaud = atoi(screenBaud);
  config.motorPulsesPerRevolution = atoi(pulses);
  config.defaultUnits = String(units);
  config.screenType = String(screenType);
  config.mechanismType = String(mech);


  return config;
}
