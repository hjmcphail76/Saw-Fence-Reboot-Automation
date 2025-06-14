#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"

Screen4D::Screen4D(float baud)
  : baudRate(baud) {
  //We dont do any setup here since the constructor is not called in setup(). Use InitAndConnect before any other screen method calls.
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
      genie.WriteInhLabel(1, str);
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
  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {
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
  } else if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    SCREEN_OBJECT btn;

    switch (Event.reportObject.index) {
      case 0: btn = MEASURE_BUTTON; break;
      case 1: btn = EDIT_TARGET_BUTTON; break;
      case 2: btn = HOME_BUTTON; break;
      case 3: btn = RESET_SERVO_BUTTON; break;
      case 4: btn = SETTINGS_BUTTON; break;
      case 5: btn = EDIT_HOME_TO_BLADE_OFFSET; break;
      default: btn = NONE; break;  // skip unhandled
    }

    if (eventCallback) {
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
ScreenGiga::ScreenGiga() {
  //We dont do any setup here since the constructor is not called in setup(). Use InitAndConnect before any other screen method calls.
}

void ScreenGiga::InitAndConnect() {
  Serial1.begin(9600);  // COM1 UART at 9600 baud

  memset(keyvalue, 0, sizeof(keyvalue));
  counter = 0;
  enterPressed = false;
}

String ScreenGiga::GetParameterInputValue() {
  String result = String(keyvalue);
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
  Serial1.print("SETLABEL:");
  Serial1.print((int)label);
  Serial1.print(":");
  Serial1.println(str);
}

void ScreenGiga::SetScreen(SCREEN screen) {
  Serial1.print("SETSCREEN:");
  Serial1.println((int)screen);
}

void ScreenGiga::ScreenPeriodic() {
  //This code segment especially was developed using AI assistance for parsing of digits
  while (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();

    if (msg.startsWith("KEY:")) {
      char c = msg.charAt(4);
      if (isdigit(c) && counter < 9) {
        keyvalue[counter++] = c;
        keyvalue[counter] = '\0';
      } else if (c == '.' && counter < 9 && strchr(keyvalue, '.') == nullptr) {
        keyvalue[counter++] = '.';
        keyvalue[counter] = '\0';
      }
    } else if (msg == "BACKSPACE" && counter > 0) {
      keyvalue[--counter] = '\0';
    } else if (msg == "ENTER") {
      eventCallback(KEYBOARD_VALUE_ENTER);
      enterPressed = true;
    } else if (msg == "CONNECTED") {
      isConnected = true;
    } else if (msg.startsWith("BUTTON:")) {

      // Extract everything after "BUTTON:"
      String btnStr = msg.substring(7);  // from index 7 to end

      btnStr.trim();  // remove any extra whitespace

      // Check if btnStr is not empty and consists only of digits
      bool validNumber = true;
      for (int i = 0; i < btnStr.length(); i++) {
        if (!isdigit(btnStr.charAt(i))) {
          validNumber = false;
          break;
        }
      }

      if (validNumber && btnStr.length() > 0) {
        int btnIndex = btnStr.toInt();  // convert string to int safely

        SCREEN_OBJECT btnEvent = NONE;
        switch (btnIndex) {
          case 0: btnEvent = MEASURE_BUTTON; break;
          case 1: btnEvent = EDIT_TARGET_BUTTON; break;
          case 2: btnEvent = HOME_BUTTON; break;
          case 3: btnEvent = RESET_SERVO_BUTTON; break;
          case 4: btnEvent = SETTINGS_BUTTON; break;
          case 5: btnEvent = EDIT_HOME_TO_BLADE_OFFSET; break;
          // You can add more cases for buttons 6, 7, ... 14, etc.
          default: btnEvent = NONE; break;
        }

        if (btnEvent != NONE && eventCallback) {
          eventCallback(btnEvent);
        }
      }
    }
  }
}


void ScreenGiga::RegisterEventCallback(ScreenEventCallback callback) {
  eventCallback = callback;
}
