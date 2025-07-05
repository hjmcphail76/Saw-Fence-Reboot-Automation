#include "SDHelper.h"

File myFile;
bool sdInit = false;

void initSDCard() {
  Serial.print("Initializing SD card...");

  if (!SD.begin()) {
    Serial.println("SD Card initialization failed!");
    while (true) {
      continue;
    }
  }

  myFile = SD.open("test.txt", FILE_WRITE);

  if (myFile) {
    Serial.println("initialization done.");
  }
  //   Serial.print("Writing to test.txt...");
  //   myFile.println("testing 1, 2, 3.");
  //   // Close the file:
  //   myFile.close();
  //   Serial.println("done.");
  // } else {
  //   // If the file didn't open, print an error:
  //   Serial.println("error opening test.txt");
  //   while (true) {
  //     continue;
  //   }
  // }
}
