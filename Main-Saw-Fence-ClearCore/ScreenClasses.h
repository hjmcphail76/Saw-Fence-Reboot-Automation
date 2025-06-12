#pragma once
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"

//Way for the main ino code to at a high level tell whatever implementation a screen object and vise versa to get values.
//ONLY objects that need to be set/get accessed, not static labels for example.
enum SCREEN_OBJECT {
  NONE,  //Default for when nothing is being pressed
  MAIN_MEASUREMENT_LABEL,
  MEASURE_BUTTON,
  EDIT_TARGET_BUTTON,
  HOME_BUTTON,
  RESET_SERVO_BUTTON,
  SETTINGS_BUTTON,
  EDIT_HOME_TO_BLADE_OFFSET,
  LIVE_PARAMETER_INPUT_LABEL
};

enum SCREEN {
  SPLASH_SCREEN,
  MAIN_CONTROL_SCREEN,
  PARAMETER_EDIT_SCREEN,
  SETTINGS_SCREEN,
  OUTSIDE_RANGE_ERROR_SCREEN,
  HOMING_ALERT_SCREEN,
  PLEASE_HOME_ERROR_SCREEN
};

class Screen {
public:
  virtual void SetStringLabel(SCREEN_OBJECT label, String str);
  virtual void SetScreen(SCREEN screen);

  virtual SCREEN_OBJECT ScreenPeriodic() {}

  typedef void (*ScreenEventCallback)(SCREEN_OBJECT object);
  virtual void RegisterEventCallback(ScreenEventCallback callback);

  bool isConnected = false;
protected:
  ScreenEventCallback eventCallback = nullptr;
};



class Screen4D : public Screen {
private:
  Genie genie;
  double baudRate;

  static Screen4D *instance;  // Static pointer to the current instance

public:
  Screen4D(double baud);
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void SetScreen(SCREEN screen) override;
  SCREEN_OBJECT ScreenPeriodic() override;

  // This handles the Genie event and must be static
  static void StaticGenieEventHandler();

  // Instance method to handle the event
  void GenieEventHandler();

  void RegisterEventCallback(ScreenEventCallback callback) override;
};
