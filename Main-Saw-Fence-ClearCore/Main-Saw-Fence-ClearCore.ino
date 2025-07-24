#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"
#include "MotorClasses.h"
#include "ScreenClasses.h"
#include "SDHelper.h"
#include "Utils.h"
#include <Arduino.h>

/*
Neo7CNC Automated Chop Saw fence

Wiki page with step-by-step documentation: https://github.com/hjmcphail76/Saw-Fence-Reboot-Automation/wiki
*/

//--------------------------------------------------User Configuration start: -------------------------------------------------------------------------------



//--------------------------------------------------User Configuration end: --------------------------------------------------------------------------------

// Forward declarations
String getUnitString(UnitType unit);
float convertUnits(float value, UnitType from, UnitType to);
float GetParameterEnteredAsFloat();

//---------------------------------------------------

char keyvalue[10];
int counter = 0;
int temp, sumTemp;
int newValue;
float currentMainMeasurement = 0.0f;
int displayMsTime = 1250;

enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_HOME_TO_BLADE_OFFSET
};
InputMode currentInputMode = INPUT_MEASUREMENT;

// Global pointers for objects (must be created in setup)
Mechanism* currentMechanismPtr = nullptr;
ScreenGiga* screenPtr = nullptr;
SDMotor* motorPtr = nullptr;
UnitType currentUnits;
int serialMoniterBaudRate;
int screenBaudRate;

void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);
  Serial.println("Serial Monitor init");

  initSDCard();

  SystemConfig config = readSettings();

  serialMoniterBaudRate = config.serialMonitorBaud;
  screenBaudRate = config.screenBaud;

  currentUnits = config.defaultUnits;

  if (config.mechanismType == "belt") {

    currentMechanismPtr = new BeltMechanism(config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.pulleyDiameter,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit);

  } else if (config.mechanismType == "lead_screw") {
    currentMechanismPtr = new LeadscrewMechanism(
      config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.screwPitch,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit);
  }
  else if (config.mechanismType == "rack_pinion"){
    currentMechanismPtr = new RackAndPinionMechanism(config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.pinionDiameter,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit);
  }

  screenPtr = new ScreenGiga(screenBaudRate);
  motorPtr = new SDMotor(currentMechanismPtr);

  screenPtr->InitAndConnect();
  motorPtr->InitAndConnect();

  screenPtr->RegisterEventCallback(ButtonHandler);

  motorPtr->HandleAlerts();


  screenPtr->SetScreen(MAIN_CONTROL_SCREEN);

  SetMeasurementUIDisplay();
}

void loop() {
  if (screenPtr != nullptr) {
    screenPtr->ScreenPeriodic();
    motorPtr->StateMachinePeriodic(screenPtr);
    delay(10);
  }
}

void ButtonHandler(SCREEN_OBJECT obj) {
  switch (obj) {
    case MEASURE_BUTTON:
      Serial.println("Measure pressed");
      if (motorPtr->hasHomed) {
        float position = convertUnits(currentMainMeasurement, currentUnits, UNIT_INCHES);
        motorPtr->MoveAbsolutePosition(static_cast<int32_t>(position * currentMechanismPtr->CalculateStepsPerUnit()));
      } else {
        screenPtr->SetScreen(PLEASE_HOME_ERROR_SCREEN);
        delay(displayMsTime);
        screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
      }
      break;
    case EDIT_TARGET_BUTTON:
      Serial.println("Edit Measurement pressed");
      currentInputMode = INPUT_MEASUREMENT;
      screenPtr->SetScreen(PARAMETER_EDIT_SCREEN);
      break;
    case HOME_BUTTON:
      Serial.println("HOME BUTTON PRESSED");
      motorPtr->StartSensorlessHoming();
      screenPtr->SetScreen(HOMING_ALERT_SCREEN);
      delay(displayMsTime);
      screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
      break;
    case RESET_SERVO_BUTTON:
      Serial.println("Reset Servo pressed");
      motorPtr->hasHomed = false;
      motorPtr->HandleAlerts();
      break;
    case SETTINGS_BUTTON:
      Serial.println("Settings Button Pressed");
      screenPtr->SetScreen(SETTINGS_SCREEN);
      break;
    case EDIT_HOME_TO_BLADE_OFFSET:
      Serial.println("Edit home to blade offset Button Pressed");
      screenPtr->SetScreen(PARAMETER_EDIT_SCREEN);
      currentInputMode = INPUT_HOME_TO_BLADE_OFFSET;
      break;
    case EXIT_SETTINGS_BUTTON:
      Serial.println("Exit settings button pressed");
      screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
      break;
    case MILLIMETERS_UNIT_BUTTON:
      currentUnits = UNIT_MILLIMETERS;
      currentMainMeasurement = 0.0f;
      screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, String(currentMainMeasurement) + getUnitString(currentUnits));
      break;
    case INCHES_UNIT_BUTTON:
      currentUnits = UNIT_INCHES;
      currentMainMeasurement = 0.0f;
      screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, String(currentMainMeasurement) + getUnitString(currentUnits));
      break;
    case KEYBOARD_VALUE_ENTER:
      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();
          break;
        case INPUT_HOME_TO_BLADE_OFFSET:
          float newVal = screenPtr->GetParameterEnteredAsFloat();
          if (newVal != 0.0f) {
            // handle newVal if needed
          }
          screenPtr->SetScreen(SETTINGS_SCREEN);
          break;
      }
      break;
  }
}

void SetMeasurementUIDisplay() {
  screenPtr->SetScreen(MAIN_CONTROL_SCREEN);

  String paramValue = screenPtr->GetParameterInputValue();

  String combinedString = paramValue + getUnitString(currentUnits);

  float val = paramValue.toFloat();

  if (paramValue.length() == 0 || isnan(val)) {
    return;
  }

  float hypotheticalMovementPosition = convertUnits(val, currentUnits, UNIT_INCHES);

  if (hypotheticalMovementPosition >= 0) {
    currentMainMeasurement = val;
    screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, combinedString);
  } else {
    screenPtr->SetScreen(OUTSIDE_RANGE_ERROR_SCREEN);
    delay(displayMsTime);
    screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
  }
}
