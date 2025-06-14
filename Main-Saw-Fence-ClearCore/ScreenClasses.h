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
  LIVE_PARAMETER_INPUT_LABEL,
  KEYBOARD_VALUE_ENTER //flag to detect when the user has entered a value and to store the buffer that has been saved until the value is safly retrieved ONCE!
};

enum SCREEN {
  SPLASH_SCREEN,// 0
  MAIN_CONTROL_SCREEN,// 1
  PARAMETER_EDIT_SCREEN,// 2
  SETTINGS_SCREEN,// 3
  OUTSIDE_RANGE_ERROR_SCREEN,// 4
  HOMING_ALERT_SCREEN,// 5
  PLEASE_HOME_ERROR_SCREEN// 6
};

class Screen {
public:
  virtual void SetStringLabel(SCREEN_OBJECT label, String str);
  virtual void SetScreen(SCREEN screen);

  virtual void ScreenPeriodic() {}

  typedef void (*ScreenEventCallback)(SCREEN_OBJECT object);
  virtual void RegisterEventCallback(ScreenEventCallback callback);

  // Input handling interface
  virtual String GetParameterInputValue();     // Gets current input and clears buffer
  virtual float GetParameterEnteredAsFloat();  // Converts buffer to float

  bool GetIsConnected(){
    return isConnected;
  }

  bool GetKeyboardEnterPressed(){
    return enterPressed;
  }
protected:
  ScreenEventCallback eventCallback = nullptr;
    char keyvalue[10] = { 0 };
  int counter = 0;
  bool enterPressed = false;
  bool isConnected = false;
};



class Screen4D : public Screen {
private:
  Genie genie;
  double baudRate;
public:
  Screen4D(double baud);
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void SetScreen(SCREEN screen) override;
  void ScreenPeriodic() override;

  void RegisterEventCallback(ScreenEventCallback callback) override;

  // Input handling interface
  String GetParameterInputValue() override;     // Gets current input and clears buffer
  float GetParameterEnteredAsFloat() override;  // Converts buffer to float
};



class ScreenGiga : public Screen {
public:
  ScreenGiga();

  // Screen interface overrides
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void SetScreen(SCREEN screen) override;
  void ScreenPeriodic() override;

  // Input handling interface
  String GetParameterInputValue() override;     // Gets current input and clears buffer
  float GetParameterEnteredAsFloat() override;  // Converts buffer to float

  void RegisterEventCallback(ScreenEventCallback callback) override;
};
