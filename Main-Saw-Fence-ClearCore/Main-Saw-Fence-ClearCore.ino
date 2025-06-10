#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"

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

//Default untits for the system to boot with. UnitType::UNIT_INCHES  or  UnitType::UNIT_MILLIMETERS
UnitType currentUnits = UnitType::UNIT_INCHES;

// IMPORTANT: Uncomment ONLY ONE of the 'Mechanism* currentMechanism = new ...' lines below
// to select the mechanism you are currently configuring. Fill in the parameters (motorProgInputRes, maxAccel, maxVel, mechanism_specific_parameter, UnitType)
// The unit type here is just for configuration of your system. Swapping between units for cut measurements on the fly can be done on the system in settings
// and will not effect this.

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

//Arduino Giga UART settings:
const int gigaBaudRate = 9600;

//--------------------------------------------------User Configuration end: --------------------------------------------------------------------------------

Genie genie;
#define motor ConnectorM0

// Global variables to hold the currently selected configuration values
// These will be set in setup() based on the selected mechanism object
int MotorProgInputRes;
int MaxAccel;
int MaxVel;
double currentStepsPerUnit = 0;  // This will store the calculated steps per inch/mm. If no mechanism class is created it stays at zero resulting in no movement.

// -------------------------------------------------------------------------

// Declare our user-defined helper functions: ------------------------------
bool MoveAbsolutePosition(int32_t position);
void HandleAlerts();
void ConfigureSDMotor();  //Step-Direction type servo
void ConfigureMCMotor();  // PWM type servo
void StartSensorlessHoming();
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
bool hasHomed = false;
char keyvalue[10];                   // Array to hold keyboard character values
int counter = 0;                     // Keyboard number of characters
int temp, sumTemp;                   // Keyboard Temp values (sumTemp is not actively used for current value display)
int newValue;                        // Not actively used for current value display
float currentMainMeasurement = 0.0;  // The target measurement value entered by the user
int displayMsTime = 1250;            //Time errors and alert screens show in milliseconds.

//Keep track of the state of what parameter the user is editing and what should the keyboard edit.
enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_BLADE_THICKNESS,
  // Add more as needed
};
InputMode currentInputMode = INPUT_MEASUREMENT;

// State machine for homing
enum HomingState {
  HOMING_IDLE,
  HOMING_INIT,
  HOMING_WAIT_HLFB,
  HOMING_COMPLETE,
  HOMING_ERROR
};
HomingState homingState = HOMING_IDLE;

//----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);  // Give time for serial monitor to connect
  Serial.println("Serial Monitor init");

  // Initialize Serial1 for the 4D Display
  ConnectorCOM1.RtsMode(SerialBase::LINE_OFF);
  Serial1.begin(genieBaudRate);  // Use genieBaudRate for consistency
  Serial1.ttl(true);

  // Initialize Serial0 for the Giga display
  ConnectorCOM0.RtsMode(SerialBase::LINE_OFF);
  Serial0.begin(gigaBaudRate);
  Serial0.ttl(true);

  delay(3000);

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
    Serial.println(genie.GetForm());
  } else {
    Serial.println("Genie initialization failed.");
  }
}

void loop() {
  genie.DoEvents();  // Keep the Genie library processing

  // Continuously check homing state
  switch (homingState) {
    case HOMING_INIT:
      Serial.println("Performing sensorless homing...");
      // Ensure motor is disabled first
      motor.EnableRequest(false);
      Delay_ms(10);  // Small delay for state change

      // Enable motor, triggering homing (if configured in MSP)
      motor.EnableRequest(true);
      Serial.println("Waiting for homing to complete (HLFB asserted)...");
      homingState = HOMING_WAIT_HLFB;
      break;

    case HOMING_WAIT_HLFB:
      if (motor.HlfbState() == MotorDriver::HLFB_ASSERTED) {
        Serial.println("Sensorless homing complete!");
        hasHomed = true;
        homingState = HOMING_COMPLETE;
      } else if (motor.StatusReg().bit.AlertsPresent) {
        Serial.println("Alert occurred during homing.");
        homingState = HOMING_ERROR;
      }
      break;

    case HOMING_COMPLETE:
      genie.SetForm(1);  // Switch to the settings form after homing
      Serial.print("Current form after homing: ");
      Serial.println(genie.GetForm());
      homingState = HOMING_IDLE;  // Reset state
      break;

    case HOMING_ERROR:
      // Handle the error, perhaps display an error message on the screen
      HandleAlerts();    // Clear alerts
      genie.SetForm(1);  // Go back to main form or an error form
      Serial.println("Homing failed due to alert.");
      homingState = HOMING_IDLE;  // Reset state
      break;

    case HOMING_IDLE:
      break;
  }
}

// Genie event handler function
void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);


  // Handle events based on object type and index
  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    Serial.println(Event.reportObject.index);

    switch (Event.reportObject.index) {
      case 0:  // Measure button
        Serial.println("Measure pressed");

        if (hasHomed) {
          // Convert the currentMainMeasurement (inches or mm) to motor steps
          MoveAbsolutePosition(static_cast<int32_t>(currentMainMeasurement * currentStepsPerUnit));
        } else {
          genie.SetForm(6);
          delay(displayMsTime);
          genie.SetForm(1);
        }
        break;
      case 1:  // Edit Measurement button
        Serial.println("Edit Measurement pressed");
        currentInputMode = InputMode::INPUT_MEASUREMENT;
        genie.SetForm(2);  // Switch to the keyboard input form
        break;
      case 2:  // HOME BUTTON
        Serial.println("HOME BUTTON PRESSED");
        genie.SetForm(5);         // Show homing in progress screen
        StartSensorlessHoming();  // Initiate the non-blocking homing process
        break;
      case 3:  // Reset Servo button
        Serial.println("Reset Servo pressed");
        hasHomed = false;  //require re homing
        HandleAlerts();    // Clear any motor alerts
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
          Serial.println("Current blade thickness: " + String(sawBladeThickness));
          genie.SetForm(3);
          break;
      }
    }

    //Updates to happen each key entered here. This is the label on the keyboard form, and is universal for all entered parameters.
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
      // This line seems problematic. If val is already in currentUnits, and currentUnits is MM,
      // converting from inches to MM again will scale it incorrectly.
      // Assuming convertFromInches converts a value *in inches* to the target unit.
      // If val is already in the currentUnits (which could be MM), this conversion is wrong.
      // Re-evaluate your unit conversion logic here.
      // For now, commenting it out as it seems to be a logical error based on the comment.
      // val = convertFromInches(val, UnitType::UNIT_MILLIMETERS);
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
  delay(displayMsTime);
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
    // If not homed, initiate homing and return false to indicate move is not yet performed.
    // The move will be re-attempted after homing completes.
    StartSensorlessHoming();
    return false;
  }

  // Check if a motor alert is currently preventing motion
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    if (true) {  // Assuming you want to always handle alerts
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
  // This loop is also blocking. For a more robust system, this should also be
  // converted to a state machine or checked in the main loop.
  // For now, keeping it blocking as the homing was the primary issue.
  while ((!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) && !motor.StatusReg().bit.AlertsPresent) {
    continue;
  }
  // Check if motor alert occurred during move
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    if (true) {  // Assuming you want to always handle alerts
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

// Initiates the non-blocking homing process
void StartSensorlessHoming() {
  homingState = HOMING_INIT;
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

// #include "ClearCore.h"

// void setup() {
//   Serial0.begin(9600);  // COM0 UART at 9600 baud
//   Serial.begin(115200); // USB Serial for debug

//   Serial.println("ClearCore Serial0 started");
// }

// void loop() {
//   Serial0.println("Hello from ClearCore COM0");
//   Serial.println("Sent message on Serial0");

//   delay(1000);
// }


