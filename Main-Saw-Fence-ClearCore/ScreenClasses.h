#pragma once
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"

//Way for the main ino code to at a high level tell whatever implementation a screen object and vise versa to get values.
//ONLY objects that need to be set/get accessed, not static labels for example.
enum SCREEN_OBJECT {
  MAIN_MEASUREMENT_LABEL,
  MEASURE_BUTTON,
  EDIT_TARGET_BUTTON,
  HOME_BUTTON,
  RESET_SERVO_BUTTON,
  SETTINGS_BUTTON,
  LIVE_PARAMETER_INPUT_LABEL
};

class Screen {
public:
  virtual void SetStringLabel(SCREEN_OBJECT label, String str);

  virtual void ScreenPeriodic() {}

  bool isConnected = false;
};



class Screen4D : public Screen {
private:
  Genie genie;
  double baudRate;

  static Screen4D *instance;  // Static pointer to the current instance

public:
  Screen4D(double baud);
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void ScreenPeriodic() override;

  // This handles the Genie event and must be static
  static void StaticGenieEventHandler();

  // Instance method to handle the event
  void GenieEventHandler();
};

