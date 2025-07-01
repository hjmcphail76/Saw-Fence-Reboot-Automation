#pragma once
#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "ScreenClasses.h"

//Way for the main ino code to at a high level tell whatever implementation a screen object and vise versa to get values.
//ONLY objects that need to be set/get accessed, not static labels for example.
//These indexes are used to indicate with and int what object is being refrenced (event firing, setlabels, ect.) over serial (this is helpful for the giga code but 4D Systems screen indexing takes place elsewhere)
enum SCREEN_OBJECT {
  NONE,  //Default for when nothing is being pressed //0
  MAIN_MEASUREMENT_LABEL, //1
  MEASURE_BUTTON, //2
  EDIT_TARGET_BUTTON, //3
  HOME_BUTTON, //4
  RESET_SERVO_BUTTON, //5
  SETTINGS_BUTTON, //6
  EDIT_HOME_TO_BLADE_OFFSET, //7
  LIVE_PARAMETER_INPUT_LABEL, //8
  KEYBOARD_VALUE_ENTER, //9 //flag to detect when the user has entered a value and to store the buffer that has been saved until the value is safely retrieved ONCE!
  EXIT_SETTINGS_BUTTON, //10
  INCHES_UNIT_BUTTON, //11 We treat 11 and 12 as seperate objects so it is easy to implement in the high level code that a event was fired from this (regardless that it is a toggle switch)
  MILLIMETERS_UNIT_BUTTON //12
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

  virtual void InitAndConnect();

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
  float baudRate;
public:
  Screen4D(float baud);
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void SetScreen(SCREEN screen) override;
  void ScreenPeriodic() override;

  void RegisterEventCallback(ScreenEventCallback callback) override;

  // Input handling interface
  String GetParameterInputValue() override;     // Gets current input and clears buffer
  float GetParameterEnteredAsFloat() override;  // Converts buffer to float

  void InitAndConnect() override;
};



class ScreenGiga : public Screen {
private:
  float baudRate;
public:
  ScreenGiga(float baud);

  // Screen interface overrides
  void SetStringLabel(SCREEN_OBJECT label, String str) override;
  void SetScreen(SCREEN screen) override;
  void ScreenPeriodic() override;

  // Input handling interface
  String GetParameterInputValue() override;     // Gets current input and clears buffer
  float GetParameterEnteredAsFloat() override;  // Converts buffer to float

  void RegisterEventCallback(ScreenEventCallback callback) override;

  void InitAndConnect() override;
};
