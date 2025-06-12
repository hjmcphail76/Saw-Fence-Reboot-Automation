#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"


Screen4D *Screen4D::instance = nullptr;  // Initialize static pointer

Screen4D::Screen4D(double baud)
  : baudRate(baud) {
  instance = this;  // Set the instance pointer

  // Init serial, for example:
  Serial1.begin(baudRate);

  // Loop until Genie display is online
  while (!genie.Begin(Serial1)) {
    Serial.println("waiting for Genie...");
    delay(250);
  }

  if (genie.IsOnline()) {
    Serial.println("Genie initialization successful. Display is online!");
    genie.AttachEventHandler(StaticGenieEventHandler);  // Attach the event handler
    genie.SetForm(1);                               // Set the initial form to 1
    Serial.println(genie.GetForm());
  } else {
    Serial.println("Genie initialization failed.");
  }

  // genie.AttachEventHandler(StaticGenieEventHandler);  // Attach static handler
  // delay(1000);
  // SetScreen(MAIN_CONTROL_SCREEN);
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

SCREEN_OBJECT Screen4D::ScreenPeriodic() {
  genie.DoEvents();
}

void Screen4D::StaticGenieEventHandler() {
  if (instance) {
    instance->GenieEventHandler();
  }
}

void Screen4D::GenieEventHandler() {
  genieFrame Event;
  genie.DequeueEvent(&Event);

  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    SCREEN_OBJECT btn;

    switch (Event.reportObject.index) {
      case 0: btn = MEASURE_BUTTON; break;
      case 1: btn = EDIT_TARGET_BUTTON; break;
      case 2: btn = HOME_BUTTON; break;
      case 3: btn = RESET_SERVO_BUTTON; break;
      case 4: btn = SETTINGS_BUTTON; break;
      case 5: btn = EDIT_HOME_TO_BLADE_OFFSET; break;
      default: return;  // unhandled button
    }

    if (eventCallback) {
      eventCallback(btn);
    }
  }
}

void Screen4D::RegisterEventCallback(ScreenEventCallback callback) {
  this->eventCallback = callback;
}
