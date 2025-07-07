#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"

Screen4D::Screen4D(float baud)
  : baudRate(baud) {
  //We don't do any setup here since the constructor is not called in setup(). Use InitAndConnect before any other screen method calls
}

void Screen4D::InitAndConnect() {
  enterPressed = false;

  Serial1.begin(baudRate);

  // Loop until Genie display is online or times out
  unsigned long startTime = millis();
  bool genieReady = false;

  while (!genie.Begin(Serial1)) {
    Serial.println("waiting for Genie...");
    delay(250);

    if (millis() - startTime > 5000) {  // Timeout after 5 seconds
      Serial.println("Genie init timeout.");
      break;
    }
  }

  if (genie.IsOnline()) {
    Serial.println("Genie initialization successful. Display is online!");
    genie.SetForm(1);
    Serial.println(genie.GetForm());
  } else {
    Serial.println("Genie initialization failed.");
  }
}

void Screen4D::SetStringLabel(SCREEN_OBJECT label, String str) {
  // Translate SCREEN_OBJECT to Genie object index
  switch (label) {
    case MAIN_MEASUREMENT_LABEL:
      genie.WriteInhLabel(0, str);
      break;
    case LIVE_PARAMETER_INPUT_LABEL:
      genie.WriteInhLabel(8, str);
      break;
  }
}

void Screen4D::SetScreen(SCREEN screen) {
  switch (screen) {
    case SPLASH_SCREEN:
      genie.SetForm(0);
      break;
    case MAIN_CONTROL_SCREEN:
      genie.SetForm(1);
      break;
    case PARAMETER_EDIT_SCREEN:
      genie.SetForm(2);
      break;
    case SETTINGS_SCREEN:
      genie.SetForm(3);
      break;
    case OUTSIDE_RANGE_ERROR_SCREEN:
      genie.SetForm(4);
      break;
    case HOMING_ALERT_SCREEN:
      genie.SetForm(5);
      break;
    case PLEASE_HOME_ERROR_SCREEN:
      genie.SetForm(6);
      break;
  }
}

void Screen4D::ScreenPeriodic() {
  genie.DoEvents();

  genieFrame Event;
  genie.DequeueEvent(&Event);

  if (Event.reportObject.object == 0) {
    return;  //since we call the dequeue event (this whole method) periodicly, we get a false object (0) that usualy would not appear since it is meant to be hooked up to an event handler.
    //Return and skip rest of processing if this is the case
  }

  Serial.print("Object: ");
  Serial.println(Event.reportObject.object);
  Serial.print("Index: ");
  Serial.println(Event.reportObject.index);

  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {  //handle key presses
    int temp = genie.GetEventData(&Event);

    if (temp >= '0' && temp <= '9' && counter < 9) {
      keyvalue[counter++] = temp;
      keyvalue[counter] = '\0';
    } else if (temp == 110 && counter < 9) {  // '.' key
      bool hasDecimal = false;
      for (int i = 0; i < counter; i++) {
        if (keyvalue[i] == '.') {
          hasDecimal = true;
          break;
        }
      }
      if (!hasDecimal) {
        keyvalue[counter++] = '.';
        keyvalue[counter] = '\0';
      }
    } else if (temp == 8 && counter > 0) {  // Backspace
      keyvalue[--counter] = '\0';
    } else if (temp == 13) {  // Enter key
      enterPressed = true;
      eventCallback(KEYBOARD_VALUE_ENTER);
    }
    genie.WriteInhLabel(1, String(keyvalue));

  } else {
    SCREEN_OBJECT btn = NONE;
    if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {  //handle button presses
      Serial.println("hmm");
      SCREEN_OBJECT btn;
      switch (Event.reportObject.index) {  //example, WinButton6 in workshop4
        case 0: btn = MEASURE_BUTTON; break;
        case 1: btn = EDIT_TARGET_BUTTON; break;
        case 2: btn = HOME_BUTTON; break;
        case 3: btn = RESET_SERVO_BUTTON; break;
        case 4: btn = SETTINGS_BUTTON; break;
        case 5: btn = EDIT_HOME_TO_BLADE_OFFSET; break;
        case 6: btn = EXIT_SETTINGS_BUTTON; break;
        default: btn = NONE; break;  // skip undefined button
      }
      eventCallback(btn);
    }
    if (Event.reportObject.object == GENIE_OBJ_ISWITCHB) {
      Serial.println("Switch pressed!");
      switch (Event.reportObject.index) {
        case 0:
          btn = MEASURE_BUTTON;
          if (genie.GetEventData(&Event)) {  //on
            btn = MILLIMETERS_UNIT_BUTTON;
          } else {
            btn = INCHES_UNIT_BUTTON;
          }
          break;
        default:
          btn = NONE;
          break;
      }
      eventCallback(btn);
    }
  }
}

void Screen4D::RegisterEventCallback(ScreenEventCallback callback) {
  this->eventCallback = callback;
}

/*Returns 0 if an error occurs with the operation.*/
float Screen4D::GetParameterEnteredAsFloat() {
  String rawString = GetParameterInputValue();
  float val = rawString.toFloat();

  // Basic check for invalid conversion
  if (val == 0.0 && !rawString.startsWith("0")) {
    Serial.println("Error converting input to float: ");
    Serial.println(rawString);
    return 0.0;
  } else {
    return val;
  }
}

String Screen4D::GetParameterInputValue() {
  String stringKeyValue = String(keyvalue);
  for (int f = 0; f < 10; f++) {
    keyvalue[f] = 0;
  }
  counter = 0;
  enterPressed = false;  //Resets for next time
  return stringKeyValue;
}


// GIGA screen class:

//constructer
ScreenGiga::ScreenGiga(float baud)
  : baudRate(baud) {
  //We dont do any setup here since the constructor is not called in setup(). Use InitAndConnect before any other screen method calls.
}

void ScreenGiga::InitAndConnect() {
  memset(keyvalue, 0, sizeof(keyvalue));
  counter = 0;
  enterPressed = false;

  Serial1.begin(baudRate);  // Serial 1 is the giga thin client interface that we send commands over
  Serial1.ttl(true);
  delay(5000);  // wait for Giga to boot up

  Serial.println("ClearCore ready, sending HELLO");

  //Handshake to the giga. 10 sec timeout, and the giga will have no timeout and will wait for the clearcore to connect and start the handshake
  String inputBuffer = "";
  unsigned long startTime = millis();
  bool handshakeDone = false;

  while (millis() - startTime < 10000) {  // 10 second timeout
    Serial1.println("HELLO");             // send HELLO to Giga
    while (Serial1.available()) {
      char c = Serial1.read();

      // Debugging, show what was received character by character
      // Serial.print("RX char: ");
      // Serial.println(c);

      if (c == '\n' || c == '\r') {
        // end of line
        inputBuffer.trim();  // remove spaces

        // Serial.print("RX line: ");
        // Serial.println(inputBuffer); //debugging, show full recieved line buffer

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

    Serial.println("Waiting for Giga handshake response...");
    delay(100);
  }

  if (!handshakeDone) {
    Serial.println("Handshake timed out, no ACK received.");
  } else {
    Serial.println("Handshake complete.");
  }
}



String ScreenGiga::GetParameterInputValue() {
  String result = String(keyvalue);
  // Serial.print("parameter input:");
  // Serial.println(result);
  memset(keyvalue, 0, sizeof(keyvalue));
  counter = 0;
  enterPressed = false;  //Reset for the next time.f
  return result;
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
        if (inputBuffer.startsWith("KEY:")) {
          String cKey = inputBuffer.substring(4);
          cKey.trim();

          if (isdigit(cKey.charAt(0)) && counter < 9) {
            keyvalue[counter++] = cKey.charAt(0);
            keyvalue[counter] = '\0';
          } else if (cKey == "." && counter < 9 && strchr(keyvalue, '.') == nullptr) {
            keyvalue[counter++] = '.';
            keyvalue[counter] = '\0';
          } else if (cKey == "BACKSPACE" && counter > 0) {
            keyvalue[--counter] = '\0';
          } else if (cKey == "ENTER") {
            if (eventCallback) eventCallback(KEYBOARD_VALUE_ENTER);
            keyvalue[0] = '\0';
            counter = 0;
          } else if (cKey == "CONNECTED") {
            isConnected = true;
          }
        } else if (inputBuffer.startsWith("BUTTON:")) {
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
              case 7: btnEvent = EDIT_HOME_TO_BLADE_OFFSET; break;
              case 10: btnEvent = EXIT_SETTINGS_BUTTON; break;
              case 11: btnEvent = INCHES_UNIT_BUTTON; break;
              case 12: btnEvent = MILLIMETERS_UNIT_BUTTON; break;
              default: btnEvent = NONE; break;
            }

            if (btnEvent != NONE && eventCallback) {
              eventCallback(btnEvent);
            }
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
