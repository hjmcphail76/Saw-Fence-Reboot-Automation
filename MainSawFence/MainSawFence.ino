#include "ClearCore.h"
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"  // Include the new header file for mechanism classes

Genie genie;
#define motor ConnectorM0

/*
------------------------------------------------------
Neo7CNC Automated Chop Saw fence

Hardware used:
  -Clearcore controller
  -4D Systems 'Gen4 uLCD 43D CLB' Screen with the 'Clearcore to 4D' adapter.
  -Clearpath MC or SD servo
    -Both are supported
    -Plug MC type servo into ports 2 or 3 on the Clearcore
    -Plug SD type servo into ports 0 or 1 on the Clearcore

--------------------------------------------------------------------------
*/
//User Configuration:

// IMPORTANT: Uncomment ONLY ONE of the 'Mechanism* currentMechanism = new ...' lines below
// to select the mechanism you are currently configuring. Fill in the parameters (motorProgInputRes, maxAccel, maxVel, mechanism_specific_parameter, UnitType)

// Example: Belt Mechanism with 1.0 inch pulley diameter
Mechanism* currentMechanism = new BeltMechanism(200, 5000, 500, 0.236, UNIT_INCHES);

// Example: Leadscrew Mechanism with 0.2 inches per revolution pitch
// Mechanism* currentMechanism = new LeadscrewMechanism(2000, 5000, 500, 0.2, UNIT_INCHES);

// Example: Rack and Pinion Mechanism with 0.5 inch pinion diameter
// Mechanism* currentMechanism = new RackAndPinionMechanism(100, 15000, 3000, 0.5, UNIT_INCHES);

Value maxTravel = Value(47.0, UnitType::UNIT_INCHES);  //This 'Value' structure is defined in mechanismClasses.h and holds our value + whatever units ;)

float gearBoxReduction = 1;  // If none is used, leave the default of 1 (1:1)

float sawBladeThickness = 0.125;

//Serial Monitor Settings:
const int serialMoniterBaudRate = 115200;

//4D UART settings:
const int genieBaudRate = 9600;

// Global variables to hold the currently selected configuration values
// These will be set in setup() based on the selected mechanism object
int MotorProgInputRes;
int MaxAccel;
int MaxVel;
double currentStepsPerUnit = 0;  // This will store the calculated steps per inch/mm. If no mechanism class is created it stays at zero resulting in no movement.

// -------------------------------------------------------------------------

// Declare our user-defined helper functions: ------------------------------
bool MoveAbsolutePosition(int32_t position);
void PrintAlerts();
void HandleAlerts();
void ConfigureSDMotor();  //Step-Direction type
void ConfigureMCMotor();  // PWM type
void PerformSensorlessHoming();
void SetMeasurementUIDisplay();
bool ParameterInput(genieFrame Event);
String GetParameterInputValue();
void myGenieEventHandler(void);
String getUnitString(UnitType unit);
double convertFromInches(float valueInInches, UnitType targetUnit);
double convertToInches(double value, UnitType unit);
double convertUnits(double value, UnitType from, UnitType to);
void OpenParameterOutsideRangeError();
float GetParameterEnteredAsFloat();

//---------------------------------------------------

//Misc:
UnitType currentUnits = UnitType::UNIT_INCHES;  // Default units
bool hasHomed = false;
char keyvalue[10];  // Array to hold keyboard character values
int counter = 0;    // Keyboard number of characters
int temp, sumTemp;  // Keyboard Temp values (sumTemp is not actively used for current value display)
int newValue;       // Not actively used for current value display

float currentMainMeasurement = 0.0;  // The target measurement value entered by the user

//---------------------------------------------------

//Keep track of the state of what parameter the user is editing and what should the keyboard edit.
enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_BLADE_THICKNESS,
  // Add more as needed
};
InputMode currentInputMode = INPUT_MEASUREMENT;

void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);  // Give time for serial monitor to connect
  Serial.println("Serial Monitor init");

  // Initialize Serial1 for the 4D Display
  ConnectorCOM1.RtsMode(SerialBase::LINE_OFF);
  Serial1.begin(genieBaudRate);  // Use genieBaudRate for consistency
  Serial1.ttl(true);

  // Retrieve configuration values from the selected mechanism object
  // currentMechanism is now initialized at global scope.
  if (currentMechanism) {
    MotorProgInputRes = currentMechanism->getMotorProgInputRes();
    MaxAccel = currentMechanism->getMaxAccel();
    MaxVel = currentMechanism->getMaxVel();
    currentStepsPerUnit = currentMechanism->calculateStepsPerUnit(gearBoxReduction);
  }


  ConfigureSDMotor();  // Configure the motor with the selected values
  HandleAlerts();      // Handle any initial motor alerts

  Serial.println("Attempting to initialize Genie...");
  // Loop until Genie display is online
  while (!genie.Begin(Serial1)) {
    Serial.println("waiting for Genie...");
    delay(250);
  }

  if (genie.IsOnline()) {
    Serial.println("Genie initialization successful. Display is online!");
    genie.AttachEventHandler(myGenieEventHandler);  // Attach the event handler
    genie.SetForm(1);                               // Set the initial form to 1
  } else {
    Serial.println("Genie initialization failed.");
  }
}

void loop() {
  genie.DoEvents();  // Keep the Genie library processing
}

// Genie event handler function
void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);  // Remove the next queued event from the buffer, and process it below
  // The Event object is passed as a parameter by genie.DoEvents(),
  // so no need to call genie.ReadEvent(&Event) here.

  // Handle events based on object type and index
  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    switch (Event.reportObject.index) {
      case 0:  // Measure button
        Serial.println("Measure pressed");
        // Convert the currentMainMeasurement (inches or mm) to motor steps
        // using the calculated stepsPerUnit IF
        //if (currentMainMeasurement < )
        MoveAbsolutePosition(static_cast<int32_t>(currentMainMeasurement * currentStepsPerUnit));
        break;
      case 1:  // Edit Measurement button
        Serial.println("Edit Measurement pressed");
        currentInputMode = InputMode::INPUT_MEASUREMENT;
        genie.SetForm(2);  // Switch to the keyboard input form
        break;
      case 2:  // HOME BUTTON
        Serial.println("HOME BUTTON PRESSED");
        PerformSensorlessHoming();
        break;
      case 3:  // Reset Servo button
        Serial.println("Reset Servo pressed");
        hasHomed = false;
        HandleAlerts();  // Clear any motor alerts
        break;
      case 4:  // Settings Button
        Serial.println("Settings Button Pressed");
        genie.SetForm(3);  // Switch to the settings form
        break;
      case 5:  //Edit blade thickness
        Serial.println("Edit blade thickness Button Pressed");
        genie.SetForm(2);  // Switch to the keyboard input form
        currentInputMode = InputMode::INPUT_BLADE_THICKNESS;
        break;
    }
  }
  // Handle keyboard input events. Based on the current parameter being entered, that value gets changed/updated
  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {

    if (ParameterInput(Event)) {
      //updates to happen when user is done are here:

      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();  // Update the main screen label with the new value
          break;
        case INPUT_BLADE_THICKNESS:
          sawBladeThickness = GetParameterEnteredAsFloat();
          break;
      }
    }

    //Updates to happen each key entered here. This is the label on the keyboard form, and is universal
    genie.WriteInhLabel(1, String(keyvalue));
  }
}

//UI Functions: --------------------------------------------


void SetMeasurementUIDisplay() {
  genie.SetForm(1);  // Go back to the main form

  String paramValue = GetParameterInputValue();  // Only call this ONCE
  String combinedString = paramValue + getUnitString(currentUnits);
  genie.WriteInhLabel(0, combinedString.c_str());

  float val = paramValue.toFloat();  // Use the already obtained value here instead

  double maxTravelInCurrentUnits = convertUnits(maxTravel.myNum, maxTravel.unit, currentUnits);

  if (val < maxTravelInCurrentUnits) {
    if (currentUnits == UnitType::UNIT_MILLIMETERS) {
      val = convertFromInches(val, UnitType::UNIT_MILLIMETERS);
    }
    currentMainMeasurement = val;
    Serial.println(currentMainMeasurement);
  } else {
    OpenParameterOutsideRangeError();
  }
}

/*Returns 0 if an error occurs with the operation.*/
float GetParameterEnteredAsFloat() {
  String rawString = GetParameterInputValue();
  float val = rawString.toFloat();

  // Basic check for invalid conversion
  if (val == 0.0 && !rawString.startsWith("0")) {
    Serial.println("Error converting input to float: ");
    Serial.println(rawString);
    return 0.0;
  } else {
    return val;
  }
}


/*
Returns true when user presses Enter. Updates the global 'currentMainMeasurement'.
Some of this code is adapted from the ClearCore/HMI example project.
*/
bool ParameterInput(genieFrame Event) {
  // Get the value of the key pressed
  temp = genie.GetEventData(&Event);

  // Check if the key is a digit (0-9) and if we have space in the array
  if (temp >= 48 && temp <= 57 && counter < 9) {
    keyvalue[counter] = temp;      // Append the decimal value of the key pressed
    keyvalue[counter + 1] = '\0';  // Null-terminate the string
    counter++;
  } else if (temp == 110 && counter < 9) {  // '.' pressed (ASCII for '.')
    // Allow only one decimal point
    bool hasDecimal = false;
    for (int i = 0; i < counter; i++) {
      if (keyvalue[i] == '.') {
        hasDecimal = true;
        break;
      }
    }
    if (!hasDecimal && counter < 9) {
      keyvalue[counter] = '.';       // Append the decimal character
      keyvalue[counter + 1] = '\0';  // Null-terminate the string
      counter++;
    }
  } else if (temp == 8) {  // Check if Backspace/delete Key (ASCII for Backspace)
    if (counter > 0) {
      counter--;              // Decrement the counter
      keyvalue[counter] = 0;  // Overwrite the last position with null
                              // TODO: Update the LED_DIGITS object on the display if you have one
                              // genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, atoi(keyvalue)); // Update display
    }
  } else if (temp == 13) {  // Check if 'Enter' Key (ASCII for Enter)
    return true;            // Indicate that Enter was pressed
  }
  return false;  // Indicate that Enter was not pressed
}

/*
We have a function to get the value as a String object, then discard keyvalue's contents for the next time it gets used.
*/
String GetParameterInputValue() {
  String stringKeyValue = String(keyvalue);
  for (int f = 0; f < 10; f++) {
    keyvalue[f] = 0;
  }
  counter = 0;
  return stringKeyValue;
}

void OpenParameterOutsideRangeError() {
  genie.SetForm(4);
  delay(1500);
  genie.SetForm(2);
}

//Configure Motors Functions: ------------------------------

void ConfigureSDMotor() {
  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
  MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1,
                        Connector::CPM_MODE_STEP_AND_DIR);

  // Set the motor's HLFB mode to bipolar PWM
  motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  // Set the HFLB carrier frequency to 482 Hz
  motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

  // Sets the maximum velocity for each move
  motor.VelMax(MaxVel);  // Example value, adjust as needed

  // Set the maximum acceleration for each move
  motor.AccelMax(MaxAccel);  // Example value, adjust as needed
}

void ConfigureMCMotor() {
}
//-----------------------------------------------------------



//Motor Control Functions: ----------------------------------

bool MoveAbsolutePosition(int32_t position) {
  if (!hasHomed) {
    motor.EnableRequest(true);

    hasHomed = true;
  }


  // Check if a motor alert is currently preventing motion
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    PrintAlerts();
    if (true) {
      HandleAlerts();
    } else {
      Serial.println("Enable automatic alert handling by setting HANDLE_ALERTS to 1.");
    }
    Serial.println("Move canceled.");
    Serial.println();
    return false;
  }

  Serial.print("Moving to absolute position: ");
  Serial.println(position);

  // Command the move of absolute distance
  motor.Move(position, MotorDriver::MOVE_TARGET_ABSOLUTE);

  // Waits for HLFB to assert (signaling the move has successfully completed)
  Serial.println("Moving.. Waiting for HLFB");
  while ((!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) && !motor.StatusReg().bit.AlertsPresent) {
    continue;
  }
  // Check if motor alert occurred during move
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    PrintAlerts();
    if (true) {  // HANDLE_ALERTS is set to 0
      HandleAlerts();
    } else {
      Serial.println("Enable automatic fault handling by setting HANDLE_ALERTS to 1.");
    }
    Serial.println("Motion may not have completed as expected. Proceed with caution.");
    Serial.println();
    return false;
  } else {
    Serial.println("Move Done");
    return true;
  }
}

void PerformSensorlessHoming() {
  Serial.println("Performing sensorless homing...");

  // Ensure motor is disabled first
  motor.EnableRequest(false);
  Delay_ms(10);

  // Enable motor, triggering homing (if configured in MSP)
  motor.EnableRequest(true);

  // Wait for HLFB to assert, indicating homing complete
  Serial.println("Waiting for homing to complete (HLFB asserted)...");
  while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED && !motor.StatusReg().bit.AlertsPresent) {
    continue;
  }

  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Alert occurred during homing.");
    PrintAlerts();
    return;
  }

  Serial.println("Sensorless homing complete!");
  hasHomed = true;
}

void PrintAlerts() {
  // report status of alerts
  Serial.println("Alerts present: ");
  if (motor.AlertReg().bit.MotionCanceledInAlert) {
    Serial.println("     MotionCanceledInAlert ");
  }
  if (motor.AlertReg().bit.MotionCanceledPositiveLimit) {
    Serial.println("     MotionCanceledPositiveLimit ");
  }
  if (motor.AlertReg().bit.MotionCanceledNegativeLimit) {
    Serial.println("     MotionCanceledNegativeLimit ");
  }
  if (motor.AlertReg().bit.MotionCanceledSensorEStop) {
    Serial.println("     MotionCanceledSensorEStop ");
  }
  if (motor.AlertReg().bit.MotionCanceledMotorDisabled) {
    Serial.println("     MotionCanceledMotorDisabled ");
  }
  if (motor.AlertReg().bit.MotorFaulted) {
    Serial.println("     MotorFaulted ");
  }
}

void HandleAlerts() {
  if (motor.AlertReg().bit.MotorFaulted) {
    // if a motor fault is present, clear it by cycling enable
    Serial.println("Faults present. Cycling enable signal to motor to clear faults.");
    motor.EnableRequest(false);
    Delay_ms(10);
    motor.EnableRequest(true);
  }
  // clear alerts
  Serial.println("Clearing alerts.");
  motor.ClearAlerts();
}