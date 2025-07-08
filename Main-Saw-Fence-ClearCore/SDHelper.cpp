#define ARDUINOJSON_ENABLE_PROGMEM 0
#include "SDHelper.h"
#include <ArduinoJson.h>

File myFile;
bool sdInit = false;

void initSDCard() {
  Serial.print("Initializing SD card...");

  if (!SD.begin()) {
    Serial.println("SD Card initialization failed!");
    // while (true) {
    //   continue;
    // }
  }

  myFile = SD.open("settings.txt", FILE_WRITE);  //Creates it if not there

  if (myFile) {
    Serial.println("initialization done.");
    sdInit = true;
  } else {
    Serial.println("init FAILED. File missing");
  }

  
  // myFile.println("testing 1, 2, 3.");
  // // Close the file:
  // myFile.close();
}



void writeToSettings(String str){
  
}