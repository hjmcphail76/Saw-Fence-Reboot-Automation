#include "ClearCore.h"
#include <genieArduinoDEV.h>

Genie genie;
#define motor ConnectorM0

//Serial Monitor Settings: -------------------------
int serialMoniterBaudRate = 115200;
//--------------------------------------------------

//4D UART settings: --------------------------------
int genieBaudRate = 9600;
//--------------------------------------------------

// Declare our user-defined helper functions: -----
bool MoveAbsolutePosition(int32_t position);
void PrintAlerts();
void HandleAlerts();
void ConfigureSDMotor();  //Step-Direction type
void ConfigureMCMotor();  // PWM type
void PerformSensorlessHoming();
void SetMeasurementUIDisplay();
void ParameterInput();
String GetParameterInputValue();
//---------------------------------------------------

//Motor predefined constants:

float ConversionFactor;
int MotorProgInputRes = 200;  // Example: steps per revolution
int MaxAccel = 10000;
int MaxVel = 2000;

// -------------------------------------------------

//Misc:
String inches = " in";
String millimeters = " mm";
String currentUnits = inches;
bool hasHomed = false;

char keyvalue[10];  // Array to hold keyboard character values
int counter = 0;    // Keyboard number of characters
int temp, sumTemp;  // Keyboard Temp values
int newValue;

double currentMainMeasurement = 0.0;
//---------------------------------------------------

void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);
  Serial.println("Serial Monitor init");

  // Initialize Serial1 for the 4D Display
  ConnectorCOM1.RtsMode(SerialBase::LINE_OFF);
  Serial1.begin(9600);
  Serial1.ttl(true);

  ConfigureSDMotor();
  HandleAlerts();

  Serial.println("Attempting to initialize Genie...");
  while (!genie.Begin(Serial1)) {
    Serial.println("waiting for Genie...");
    delay(250);
  }

  if (genie.IsOnline()) {
    Serial.println("Genie initialization successful. Display is online!");
    genie.AttachEventHandler(myGenieEventHandler);
    genie.SetForm(1);
  } else {
    Serial.println("Genie initialization failed.");
  }
}

void loop() {
  genie.DoEvents();  // Keep the Genie library processing events (even if we're not handling any yet)
}

// You can add your myGenieEventHandler function here if needed in the future
void myGenieEventHandler(void) {
  genieFrame Event;
  // Handle events here
  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    switch (Event.reportObject.index) {
      case 0:
        Serial.println("Measure pressed");
        break;
      case 1:
        Serial.println("Edit Measurement pressed");
        genie.SetForm(2);
        //From here on out, the events will get sent to the  keyboard object
        break;
      case 2:
        Serial.println("HOME BUTTON PRESSED");
        PerformSensorlessHoming();
        break;
      case 3:
        Serial.println("Reset Servo pressed");
        HandleAlerts();
        break;
      case 4:
        Serial.println("Settings Button Pressed");
        break;
    }
  }
  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD){
    if (ParameterInput(Event)){//If it returns true, we set the main screen label.
      SetMeasurementUIDisplay();
    }
  }
}

//UI Functions: --------------------------------------------
void SetMeasurementUIDisplay() {
  genie.SetForm(1);
  String combinedString = GetParameterInputValue() + currentUnits;
  genie.WriteInhLabel(0, combinedString.c_str());
}

/*
Returns true when user is done entering info, or cancels. When done, retrive the value at the keyvalue array.
Some of this code is from the clearcore/HMI example project. TODO: figure out how to attribute it.
*/
/*
Returns true when user presses Enter. Updates the global 'newValue'.
Some of this code is from the clearcore/HMI example project. TODO: figure out how to attribute it.
*/
bool ParameterInput(genieFrame Event) {
  temp = genie.GetEventData(&Event);             // Store the value of the key pressed
  if (temp >= 48 && temp <= 57 && counter < 9)   // Convert value of key into Decimal, limit to 9 digits
  {
    keyvalue[counter] = temp;                   // Append the decimal value of the key pressed
    keyvalue[counter + 1] = '\0';               // Null-terminate the string
    sumTemp = atoi(keyvalue);                   // Convert the array into a number
    counter++;
  }
  else if (temp == 110){ // '.' pressed
    keyvalue[counter] = temp;                   // Append the decimal value of the key pressed
    keyvalue[counter + 1] = '\0';               // Null-terminate the string
    sumTemp = atoi(keyvalue);                   // Convert the array into a number
    counter++;
  }
  else if (temp == 8)                           // Check if Backspace/delete Key
  {
    if (counter > 0) {
      counter--;                                // Decrement the counter
      keyvalue[counter] = 0;                    // Overwrite the last position with null
      genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, atoi(keyvalue)); // Update display
    }
  }
  else if (temp == 13)                          // Check if 'Enter' Key
  {
    
    // Reset for the next input
    counter = 0;
    newValue = sumTemp;
    sumTemp = 0;
    Serial.println(String(keyvalue));
    return true; // Indicate that Enter was pressed
  }
  return false; // Indicate that Enter was not pressed
}

/*
We have a function to get the value as a String object, then discard keyvalue's contents for the next time it gets used.
*/
String GetParameterInputValue(){
  String stringKeyValue = String(keyvalue);
  for (int f = 0; f < 10; f++) {
    keyvalue[f] = 0;
  }
  counter = 0;

  //Set the double type of the string to the current set parameter. TODO:
  currentMainMeasurement = 0.0;

  return stringKeyValue;
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
  motor.VelMax(10000);  // Example value, adjust as needed

  // Set the maximum acceleration for each move
  motor.AccelMax(100000);  // Example value, adjust as needed
}

void ConfigureMCMotor() {
}
//-----------------------------------------------------------



//Motor Control Functions: ----------------------------------

bool MoveAbsolutePosition(int32_t position) {
  motor.EnableRequest(true);
  hasHomed = true;


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
  hasHomed = true;
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
