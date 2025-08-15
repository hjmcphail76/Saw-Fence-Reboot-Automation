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
*/


String getUnitString(UnitType unit);
float convertUnits(float value, UnitType from, UnitType to);
float GetParameterEnteredAsFloat();

//---------------------------------------------------

char keyvalue[10];
int counter = 0;
int temp, sumTemp;
int newValue;
float currentMainMeasurement = 0.0f;
float maxTravelMeasurement = 0.0f;
UnitType maxTravelUnit;
int displayMsTime = 1250;
UnitType currentUnit;
int serialMoniterBaudRate;
int screenBaudRate;
SystemConfig config;

enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_MAX_TRAVEL_BUTTON
};

InputMode currentInputMode = INPUT_MEASUREMENT;

Mechanism* currentMechanismPtr = nullptr;
ScreenGiga* screenPtr = nullptr;
SDMotor* motorPtr = nullptr;


void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);
  Serial.println("Serial Monitor init");

  initSDCard();

  config = readSettings();

  serialMoniterBaudRate = config.serialMonitorBaud;
  screenBaudRate = config.screenBaud;

  currentUnit = config.defaultUnit;

  maxTravelMeasurement = config.mechanismParams.maxTravel;
  maxTravelUnit = config.mechanismParams.maxTravelUnit;

  //per mechanism type config loading
  if (config.mechanismType == "belt") {

    // I hate to name it this, but unit1 repersents the unit of either pulley diameter, screw pitch, or gear pitch diameter

    currentMechanismPtr = new BeltMechanism(config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.pulleyDiameter,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit1);

  } else if (config.mechanismType == "lead_screw") {
    currentMechanismPtr = new LeadscrewMechanism(
      config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.screwPitch,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit1);
  }
  else if (config.mechanismType == "rack_pinion"){
    currentMechanismPtr = new RackAndPinionMechanism(config.motorPulsesPerRevolution,
      config.motorShaftAccel,
      config.motorShaftVel,
      config.mechanismParams.pinionDiameter,
      config.mechanismParams.gearboxReduction,
      config.mechanismParams.unit1);
  }

  screenPtr = new ScreenGiga(screenBaudRate);
  motorPtr = new SDMotor(currentMechanismPtr);

  screenPtr->InitAndConnect(currentUnit);
  motorPtr->InitAndConnect();

  screenPtr->RegisterEventCallback(ButtonHandler);

  motorPtr->HandleAlerts();


  screenPtr->SetScreen(MAIN_CONTROL_SCREEN);

  //SetMeasurementUIDisplay();
  screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, "0.00" + getUnitString(currentUnit));
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
        float position = convertUnits(currentMainMeasurement, currentUnit, UNIT_INCHES);
        // Serial.println("Position in inches: "+ String(position)); debugging
        if (position < convertUnits(maxTravelMeasurement, maxTravelUnit, UNIT_INCHES)){
          Serial.println("Max travel: " + String(maxTravelMeasurement) + " " + getUnitString(currentUnit) + getUnitString(config.mechanismParams.maxTravelUnit));
          motorPtr->MoveAbsolutePosition(static_cast<int32_t>(position * currentMechanismPtr->CalculateStepsPerUnit()));
        }
        else{
          screenPtr->SetScreen(OUTSIDE_RANGE_ERROR_SCREEN);
          delay(displayMsTime);
          screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
          return;
        }
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
      currentMainMeasurement = 0.0f;
      screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, String(currentMainMeasurement) + getUnitString(currentUnit));
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
    case EDIT_MAX_TRAVEL_BUTTON:
      Serial.println("Edit home to blade offset Button Pressed");
      screenPtr->SetScreen(PARAMETER_EDIT_SCREEN);
      currentInputMode = INPUT_MAX_TRAVEL_BUTTON;
      break;
    case EXIT_SETTINGS_BUTTON:
      Serial.println("Exit settings button pressed");
      screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
      break;
    case MILLIMETERS_UNIT_BUTTON:
      currentUnit = UNIT_MILLIMETERS;
      currentMainMeasurement = 0.0f;
      screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, String(currentMainMeasurement) + getUnitString(currentUnit));
      config.defaultUnit = currentUnit;
      writeSettings(config);
      break;
    case INCHES_UNIT_BUTTON:
      currentUnit = UNIT_INCHES;
      currentMainMeasurement = 0.0f;
      screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, String(currentMainMeasurement) + getUnitString(currentUnit));
      config.defaultUnit = currentUnit;
      writeSettings(config);
      break;
    case KEYBOARD_VALUE_ENTER:
      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();
          break;
        case INPUT_MAX_TRAVEL_BUTTON:
          float newVal = screenPtr->GetParameterEnteredAsFloat();
          if (newVal != 0.0f) {
            maxTravelMeasurement = newVal;
            config.mechanismParams.maxTravel = maxTravelMeasurement;
            config.mechanismParams.maxTravelUnit = currentUnit;
            writeSettings(config);
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

  if (paramValue.length() < 1){
    paramValue = "0.00";
  }

  String combinedString = paramValue + getUnitString(currentUnit);

  float val = paramValue.toFloat();

  if (paramValue.length() == 0 || isnan(val)) {
    return;
  }

  float hypotheticalMovementPosition = convertUnits(val, currentUnit, UNIT_INCHES);

  if (hypotheticalMovementPosition >= 0) {
    currentMainMeasurement = val;
    screenPtr->SetStringLabel(MAIN_MEASUREMENT_LABEL, combinedString);
  } else {
    screenPtr->SetScreen(OUTSIDE_RANGE_ERROR_SCREEN);
    delay(displayMsTime);
    screenPtr->SetScreen(MAIN_CONTROL_SCREEN);
  }
}
