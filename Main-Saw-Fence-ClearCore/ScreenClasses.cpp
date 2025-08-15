#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"
#include "Utils.h"
// GIGA screen class:

//constructer
ScreenGiga::ScreenGiga(float baud)
  : baudRate(baud) {
  //We don not do any setup here since the constructor is not called in setup(). call InitAndConnect before any other screen method calls.
}

void ScreenGiga::InitAndConnect(UnitType defaultBootUnit) {
  enterPressed = false;

  Serial1.begin(baudRate);  // Serial 1 is the giga thin client interface that we send commands over
  Serial1.ttl(true);
  delay(5000);  // wait for Giga to boot up

  Serial.println("ClearCore ready, sending HELLO");

  //Handshake to the giga. 10 sec timeout, and the giga will have no timeout and will wait for the clearcore to connect and start the handshake
  String inputBuffer = "";
  unsigned long startTime = millis();
  bool handshakeDone = false;

  Serial.println("Waiting for Giga handshake response...");
  while (millis() - startTime < 10000) {  // 10 second timeout
    Serial1.println("HELLO");             // send HELLO to Giga
    while (Serial1.available()) {
      char c = Serial1.read();

      //Debugging, show what was received character by character
      // Serial.print("RX char: ");
      // Serial.println(c);

      if (c == '\n' || c == '\r') {
        // end of line
        inputBuffer.trim();  // remove spaces


        //debugging, show full recieved line buffer
        // Serial.print("RX line: ");
        // Serial.println(inputBuffer);

        if (inputBuffer == "ACK") {
          Serial.println("Received ACK from Giga!");
          handshakeDone = true;
          break;
        }
        // clear buffer for next line
        inputBuffer = "";
      } else {
        inputBuffer += c;
      }
    }

    if (handshakeDone) break;
    delay(100);
  }

  if (defaultBootUnit == UNIT_MILLIMETERS) {
    Serial1.println("SETSWITCHTOSTATE:12");
    Serial.println("Switching to millimeters");
  } else if (defaultBootUnit == UNIT_INCHES) {
    Serial1.println("SETSWITCHTOSTATE:11");
    Serial.println("Switching to inches");
  }

  if (!handshakeDone) {
    Serial.println("Handshake timed out, no ACK received.");
  } else {
    Serial.println("Handshake complete.");
  }
}



String ScreenGiga::GetParameterInputValue() {
  return lastEntered;
}

float ScreenGiga::GetParameterEnteredAsFloat() {
  String raw = GetParameterInputValue();
  float val = raw.toFloat();
  if (val == 0.0 && !raw.startsWith("0")) {
    Serial.println("Invalid float input: " + raw);
    return 0.0;
  }
  return val;
}

void ScreenGiga::SetStringLabel(SCREEN_OBJECT label, String str) {
  //Send the object index defined in the enum in h file, The giga code determines if it is a valid label object
  Serial1.print("SETLABEL:");
  Serial1.print((int)label);
  Serial1.print(":");
  Serial1.println(str);
}

void ScreenGiga::SetScreen(SCREEN screen) {
  Serial1.print("SETSCREEN:");
  Serial1.println((int)screen);  // add newline to mark message end!
}

void ScreenGiga::ScreenPeriodic() {
  static String inputBuffer = "";

  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n' || c == '\r') {
      inputBuffer.trim();

      if (inputBuffer.length() > 0) {
        // ----- Process message -----
        if (inputBuffer.startsWith("BUTTON:")) {
          String btnStr = inputBuffer.substring(7);
          btnStr.trim();

          bool validNumber = true;
          for (int i = 0; i < btnStr.length(); i++) {
            if (!isdigit(btnStr.charAt(i))) {
              validNumber = false;
              break;
            }
          }

          if (validNumber && btnStr.length() > 0) {
            int btnIndex = btnStr.toInt();

            SCREEN_OBJECT btnEvent = NONE;
            switch (btnIndex) {
              case 2: btnEvent = MEASURE_BUTTON; break;
              case 3: btnEvent = EDIT_TARGET_BUTTON; break;
              case 4: btnEvent = HOME_BUTTON; break;
              case 5: btnEvent = RESET_SERVO_BUTTON; break;
              case 6: btnEvent = SETTINGS_BUTTON; break;
              case 7: btnEvent = EDIT_MAX_TRAVEL_BUTTON; break;
              case 10: btnEvent = EXIT_SETTINGS_BUTTON; break;
              case 11: btnEvent = INCHES_UNIT_BUTTON; break;
              case 12: btnEvent = MILLIMETERS_UNIT_BUTTON; break;
              default: btnEvent = NONE; break;
            }

            if (btnEvent != NONE && eventCallback) {
              eventCallback(btnEvent);
            }
          }
        } else if (inputBuffer.startsWith("ENTER:")) {
          inputBuffer = inputBuffer.substring(6);
          lastEntered = inputBuffer;
          if (eventCallback) {
            eventCallback(KEYBOARD_VALUE_ENTER);
          }
        }
      }

      // Always clear buffer after newline
      inputBuffer = "";
    } else if (isPrintable(c)) {
      inputBuffer += c;
    }
  }
}



void ScreenGiga::RegisterEventCallback(ScreenEventCallback callback) {
  eventCallback = callback;
}
