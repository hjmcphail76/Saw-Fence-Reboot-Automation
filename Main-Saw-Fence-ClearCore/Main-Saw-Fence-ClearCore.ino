#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"
#include "MotorClasses.h"
#include "ScreenClasses.h"


/*
Neo7CNC Automated Chop Saw fence

Hardware used:
  -Teknic ClearCore controller

  -Arduino Giga+Giga Shield Screen  OR  4D Systems 'Gen4 uLCD 43D CLB' Screen with the 'Clearcore to 4D' adapter.

  -Clearpath MC or SD servo
    -Both are supported
    -Plug MC type servo into port 0 on the Clearcore.
    -Plug SD type servo into ports 3 on the Clearcore.
    -No code changes are required. Everything other than what is listed in 'User Configuration' is taken care on the backend for you.
*/


//--------------------------------------------------User Configuration start: -------------------------------------------------------------------------------

//Serial Monitor Settings:
const int serialMoniterBaudRate = 115200;

//HMI Screen UART settings, applies to both giga and 4D screen.
const int screenBaudRate = 9600;

//Default units for the system to boot with. UnitType::UNIT_INCHES  or  UnitType::UNIT_MILLIMETERS
UnitType currentUnits = UnitType::UNIT_INCHES;
// UnitType currentUnits = UnitType::UNIT_MILLIMETERS;


// Example: Belt Mechanism with 1.0 inch pulley diameter and a 1:1 motor gearbox reduction (none)
BeltMechanism currentMechanism = BeltMechanism(200, 5000, 500, 0.236, 1.0, UNIT_INCHES);

// Example: Leadscrew Mechanism with 0.2 inches per revolution pitch and a 1:1 motor gearbox reduction (none)
// LeadscrewMechanism currentMechanism = new LeadscrewMechanism(2000, 5000, 500, 0.2, 1, UNIT_INCHES);

// Example: Rack and Pinion Mechanism with 0.5 inch pinion diameter and a 1:1 motor gearbox reduction (none)
// RackAndPinionMechanism currentMechanism = new RackAndPinionMechanism(100, 15000, 3000, 0.5, 1, UNIT_INCHES);


//Uncomment the respective screen type
// Screen4D screen = Screen4D(screenBaudRate);
ScreenGiga screen = ScreenGiga();

//Uncomment the respective motor type
SDMotor motor = SDMotor(currentMechanism);
//MCMotor motor = new MCMotor(currentMechanism);

Value maxTravel = Value(47.0, UnitType::UNIT_INCHES);  //This 'Value' structure is defined in mechanismClasses.h and holds our value + whatever units

Value homeToBladeOffset = Value(35, UnitType::UNIT_INCHES);  //Before changing this value, leave it default and deploy everything. Then when running, home the system and measure from blade to saw stop at home position.



//--------------------------------------------------User Configuration end: --------------------------------------------------------------------------------


// -------------------------------------------------------------------------

// Declare our user-defined helper functions: ------------------------------
void SetMeasurementUIDisplay();
bool ParameterInput(genieFrame Event);
String getUnitString(UnitType unit);
float convertFromInches(float valueInInches, UnitType targetUnit);
float convertToInches(float value, UnitType unit);
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

  screen.InitAndConnect();
  motor.InitAndConnect();

  screen.RegisterEventCallback(ButtonHandler);

  motor.HandleAlerts();  // Handle any initial motor alerts

  screen.SetScreen(MAIN_CONTROL_SCREEN);
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
        //Verification that the entered position is within the travel range is done in SetMeasurementUIDisplay(), and if it is outside then the screen and internal measure value stays the same as last verified position
        // Convert the position to motor steps

        float position = convertToInches(homeToBladeOffset.val, homeToBladeOffset.unit) - convertToInches(currentMainMeasurement, currentUnits);
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
      screen.SetScreen(HOMING_ALERT_SCREEN);  // Show homing in progress screen
      motor.StartSensorlessHoming();
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

    case KEYBOARD_VALUE_ENTER:
      // now that the user has entered a value we can safely acsess and use it
      // Using either the float or String acsess methods will result in the buffer of that value being cleared for next time, so only use ONCE!!
      Serial.println("Enter key pressed!!");
      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();  // Update the main screen label with the new value and goes to it
          break;
        case INPUT_HOME_TO_BLADE_OFFSET:
          homeToBladeOffset.val = screen.GetParameterEnteredAsFloat();
          screen.SetScreen(SETTINGS_SCREEN);  //go back to the settings screen
          break;
      }
  }
}

//UI formating helper Functions: --------------------------------------------

void SetMeasurementUIDisplay() {
  screen.SetScreen(MAIN_CONTROL_SCREEN);                // Go back to the main form
                                                        //GetParameterInputValue
  String paramValue = screen.GetParameterInputValue();  // Only call this ONCE since it resets the input buffer.

  // Serial.print("Current parameter input:");
  // Serial.println(paramValue);
  
  String combinedString = paramValue + getUnitString(currentUnits);

  float val = paramValue.toFloat();  // Parse string to float once

  // Check if input was valid
  if (paramValue.length() == 0 || isnan(val)) {
    return;
  }
  float hypotheticalMovementPosition = convertToInches(homeToBladeOffset.val, homeToBladeOffset.unit) - convertToInches(val, currentUnits);


  if (hypotheticalMovementPosition > 0) {
    // Make sure moving to that position is possible
    currentMainMeasurement = val;
    screen.SetStringLabel(MAIN_MEASUREMENT_LABEL, combinedString);
  } else {
    screen.SetScreen(OUTSIDE_RANGE_ERROR_SCREEN);
    delay(displayMsTime) ;
    screen.SetScreen(MAIN_CONTROL_SCREEN);
  }
}





// #include "ClearCore.h"

// void setup() {
//   Serial.begin(115200);

//   Serial1.begin(9600);
//   Serial1.ttl(true);
//   delay(1000);

//   Serial.println("ClearCore sending...");
// }

// void loop() {
//   Serial1.println("Hello from clearcore!");
//   delay(1000);
// }

