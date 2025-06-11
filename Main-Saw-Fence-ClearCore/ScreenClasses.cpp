#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"


Screen4D *Screen4D::instance = nullptr;  // Initialize static pointer

Screen4D::Screen4D(double baud) : baudRate(baud) {
  instance = this;  // Set the instance pointer

  // Init serial, for example:
  Serial1.begin(baudRate);
  genie.Begin(Serial1);
  genie.AttachEventHandler(StaticGenieEventHandler);  // Attach static handler
  delay(1000);
  genie.SetForm(1);
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

void Screen4D::ScreenPeriodic() {
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

  // Example event handling
  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    switch (Event.reportObject.index) {
      case 0:
        Serial.println("Screen4D: Measure button pressed.");
        break;
      default:
        break;
    }
  }
}
