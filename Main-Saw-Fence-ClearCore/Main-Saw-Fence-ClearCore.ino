#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"
#include "MotorClasses.h"
#include "ScreenClasses.h"
#include "SDHelper.h"
#include <Arduino.h>

/*
Neo7CNC Automated Chop Saw fence


Wiki page with step-by-step documentation: https://github.com/hjmcphail76/Saw-Fence-Reboot-Automation/wiki
*/


//--------------------------------------------------User Configuration start: -------------------------------------------------------------------------------

//Serial Monitor Settings. Leave default at first.
const int serialMoniterBaudRate = 115200;

//HMI Screen UART settings, applies to both giga and 4D screen. Leave default at first.
const int screenBaudRate = 9600;

//Default units for the system to boot with. UnitType::UNIT_INCHES  or  UnitType::UNIT_MILLIMETERS
UnitType currentUnits = UnitType::UNIT_INCHES;
// UnitType currentUnits = UnitType::UNIT_MILLIMETERS;


// Example: Belt Mechanism with a 1.0 inch pulley diameter and a 1:1 motor gearbox reduction (none)
// BeltMechanism currentMechanism = BeltMechanism(1000, 5000, 1500, 0.236, 1.0, UNIT_INCHES);

// Example: Leadscrew Mechanism with 0.2 inches per revolution pitch and a 1:1 motor gearbox reduction (none)
LeadscrewMechanism currentMechanism = LeadscrewMechanism(1000, 200000, 4000, 20, 1, UNIT_MILLIMETERS);

// Example: Rack and Pinion Mechanism with 0.5 inch pinion diameter and a 1:1 motor gearbox reduction (none)
// RackAndPinionMechanism currentMechanism = RackAndPinionMechanism(1000, 15000, 3000, 0.5, 1, UNIT_INCHES);


//Uncomment the respective screen type:
ScreenGiga screen = ScreenGiga(screenBaudRate);
// Screen4D screen = Screen4D(screenBaudRate);


//Uncomment the respective motor type:
SDMotor motor = SDMotor(currentMechanism);
//MCMotor motor = new MCMotor(currentMechanism);


//--------------------------------------------------User Configuration end: --------------------------------------------------------------------------------


String getUnitString(UnitType unit);
// float convertFromInches(float valueInInches, UnitType targetUnit);
// float convertToInches(float value, UnitType unit);
float convertUnits(float value, UnitType from, UnitType to);
float GetParameterEnteredAsFloat();

//---------------------------------------------------

//Misc:
char keyvalue[10];                   // Array to hold keyboard character values
int counter = 0;                     // Keyboard number of characters
int temp, sumTemp;                   // Keyboard Temp values (sumTemp is not actively used for current value display)
int newValue;                        // Not actively used for current value display
float currentMainMeasurement = 0.0;  // The target measurement value entered by the user
int displayMsTime = 1250;            //Time errors and alert screens show in milliseconds.

//Keep track of the state of what parameter the user is editing and what should the keyboard edit.
enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_HOME_TO_BLADE_OFFSET
};
InputMode currentInputMode = INPUT_MEASUREMENT;  //default

//----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(5000);  // Give time for serial monitor to connect
  Serial.println("Serial Monitor init");

  initSDCard();

  readSettings(); //TODO: Create systemConfig object to get settings from.

  screen.InitAndConnect();
  motor.InitAndConnect();

  screen.RegisterEventCallback(ButtonHandler);

  motor.HandleAlerts();  // Handle any initial motor alerts

  screen.SetScreen(MAIN_CONTROL_SCREEN); // needs changing
}

void loop() {
  screen.ScreenPeriodic();
  // Continuously call motor homing state machine updates
  motor.StateMachinePeriodic(screen);
  delay(10);
}

//gets called from the screen class for both 4D screen as well as giga screen
//To index and label button indexes at a high level, the enum SCREEN_OBJECT is used.
//it converts on the backend of the screen object to however it needs to refrence to each screen's needs
void ButtonHandler(SCREEN_OBJECT obj) {
  switch (obj) {
    case MEASURE_BUTTON:
      Serial.println("Measure pressed");
      if (motor.hasHomed) {
        //Verification that the entered position is within the travel range is done in SetMeasurementUIDisplay()
        float position = convertUnits(currentMainMeasurement, currentUnits, UNIT_INCHES);
        motor.MoveAbsolutePosition(static_cast<int32_t>(position * currentMechanism.CalculateStepsPerUnit()));
      } else {
        screen.SetScreen(PLEASE_HOME_ERROR_SCREEN);
        delay(displayMsTime);
        screen.SetScreen(MAIN_CONTROL_SCREEN);
      }
      break;
    case EDIT_TARGET_BUTTON:
      Serial.println("Edit Measurement pressed");
      currentInputMode = InputMode::INPUT_MEASUREMENT;
      screen.SetScreen(PARAMETER_EDIT_SCREEN);  // Switch to the keyboard input form
      break;
    case HOME_BUTTON:
      Serial.println("HOME BUTTON PRESSED");
      motor.StartSensorlessHoming();
      screen.SetScreen(HOMING_ALERT_SCREEN);  // Show homing in progress screen
      delay(displayMsTime);                   //This is a crude way but it should not matter in the end. (I don't exactly love this solution to the bug i was running into.)
      screen.SetScreen(MAIN_CONTROL_SCREEN);
      break;
    case RESET_SERVO_BUTTON:
      Serial.println("Reset Servo pressed");
      motor.hasHomed = false;  //require re homing
      motor.HandleAlerts();
      break;
    case SETTINGS_BUTTON:
      Serial.println("Settings Button Pressed");
      screen.SetScreen(SETTINGS_SCREEN);  // Switch to the settings screen
      break;
    case EDIT_HOME_TO_BLADE_OFFSET:
      Serial.println("Edit home to blade offset Button Pressed");
      screen.SetScreen(PARAMETER_EDIT_SCREEN);  // Switch to the keyboard input screen
      currentInputMode = InputMode::INPUT_HOME_TO_BLADE_OFFSET;
      break;
    case EXIT_SETTINGS_BUTTON:
      Serial.println("Exit settings button pressed");
      screen.SetScreen(MAIN_CONTROL_SCREEN);
      break;
    case MILLIMETERS_UNIT_BUTTON:
      currentUnits = UNIT_MILLIMETERS;
      currentMainMeasurement = 0.0;
      screen.SetStringLabel(MAIN_MEASUREMENT_LABEL, currentMainMeasurement + getUnitString(currentUnits));
      break;
    case INCHES_UNIT_BUTTON:
      currentUnits = UNIT_INCHES;
      currentMainMeasurement = 0.0;
      screen.SetStringLabel(MAIN_MEASUREMENT_LABEL, currentMainMeasurement + getUnitString(currentUnits));
      break;

    case KEYBOARD_VALUE_ENTER:
      // now that the user has entered a value we can safely acsess and use it
      // Using either the float or String acsess methods will result in the buffer of that value being cleared for next time, so only use ONCE!!
      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();  // Update the main screen label with the new value and goes to it
          break;
        case INPUT_HOME_TO_BLADE_OFFSET:
          float newVal = screen.GetParameterEnteredAsFloat();
          if (newVal != 0.0) {
          }
          screen.SetScreen(SETTINGS_SCREEN);  //go back to the settings screen
          break;
      }
  }
}

void SetMeasurementUIDisplay() {
  screen.SetScreen(MAIN_CONTROL_SCREEN);  // Go back to the main screen

  String paramValue = screen.GetParameterInputValue();  // Only call this ONCE since it resets the input buffer.

  String combinedString = paramValue + getUnitString(currentUnits);

  float val = paramValue.toFloat();  // Parse string to float once

  // Check if input was valid
  if (paramValue.length() == 0 || isnan(val)) {
    return;
  }

  float hypotheticalMovementPosition = convertUnits(val, currentUnits, UNIT_INCHES);


  if (hypotheticalMovementPosition >= 0) {
    // Make sure moving to that position is possible and if no then give out of range error.
    currentMainMeasurement = val;
    screen.SetStringLabel(MAIN_MEASUREMENT_LABEL, combinedString);
  } else {
    screen.SetScreen(OUTSIDE_RANGE_ERROR_SCREEN);
    delay(displayMsTime);
    screen.SetScreen(MAIN_CONTROL_SCREEN);
  }
}
