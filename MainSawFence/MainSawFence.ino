#include "ClearCore.h"
#include <genieArduinoDEV.h>

Genie genie;
//----------------------------------------------------------------------------------------

// Define the ClearCore COM port connected to the HMI
#define SerialPort Serial1          // ClearCore UART Port, connected to 4D Display COM1
#define CcSerialPort ConnectorCOM1  // ClearCore UART Port, connected to 4D Display COM1
//----------------------------------------------------------------------------------------

#define baudRate 115200  // ClearCore Baud Rate, for 4D Display
#define motor ConnectorM0


float ConversionFactor;
int MotorProgInputRes = 200;  // Example: steps per revolution

int CurrentForm = 0;

// Declares user-defined helper functions.
void PrintAlerts();
void HandleAlerts();

// Declares our user-defined helper function, which is used to command moves to
// the motor.
bool MoveAbsolutePosition(int32_t position);

char keyvalue[11] = { 0 };  // Enough room for 10 characters + null terminator
int counter = 0;
char temp;
bool decimalEntered = false;

String inches = "in";
String millimeters = "mm";

String CurrentUnits = inches;


void setup() {
  // Sets the input clocking rate. This normal rate is ideal for ClearPath
  // step and direction applications.
  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

  // Sets all motor connectors into step and direction mode.
  MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                        Connector::CPM_MODE_STEP_AND_DIR);

  // Set the motor's HLFB mode to bipolar PWM
  motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  // Set the HFLB carrier frequency to 482 Hz
  motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

  // Sets the maximum velocity for each move
  motor.VelMax(10000);  // Example value, adjust as needed

  // Set the maximum acceleration for each move
  motor.AccelMax(100000);  // Example value, adjust as needed

  Serial.begin(115200);  // Debug/Console printing over USB

  // Open the HMI serial port
  CcSerialPort.RtsMode(SerialBase::LINE_OFF);
  SerialPort.ttl(true);      // Set the Clearcore UART to be TTL
  SerialPort.begin(115200);  // Set up Serial1

  while (!SerialPort) {
    continue;
  }

  delay(3000);

  while (!genie.Begin(SerialPort))
    ;

  if (genie.IsOnline()) {
    genie.AttachEventHandler(myGenieEventHandler);
  }

  // Enables the motor; homing will begin automatically if enabled
  motor.EnableRequest(true);
  Serial.println("Motor Enabled");

  // Waits for HLFB to assert (waits for homing to complete if applicable)
  Serial.println("Waiting for HLFB...");
  while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED && !motor.StatusReg().bit.AlertsPresent) {
    continue;
  }
  
  HandleAlerts();

  genie.SetForm(1);  // Change to Form 1
  CurrentForm = 1;

  genie.WriteContrast(15);  // Max Brightness
}

// Genie event handler — dequeue and process keypad input immediately
void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);

  // Process keypad input event on keypad index 0 (main control screen keypad)
  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD && Event.reportObject.index == 0) {
    int temp = genie.GetEventData(&Event);

    // Digit keys '0' to '9', it converts to ASCII to compare.
    if (temp >= '0' && temp <= '9' && counter < sizeof(keyvalue) - 1) {
      keyvalue[counter++] = (char)temp;
      keyvalue[counter] = '\0';  // Null-terminate

      double val = atof(keyvalue);
      genie.WriteStr(0, val, 3);  // Update display (String0) with 3 decimals
    }
    // Decimal point ('.' or ASCII 110)
    else if ((temp == '.' || temp == 110) && !decimalEntered && counter < sizeof(keyvalue) - 2) {
      keyvalue[counter++] = '.';
      keyvalue[counter] = '\0';
      decimalEntered = true;

      double val = atof(keyvalue);
      genie.WriteStr(0, val, 3);
    }
    // Backspace (ASCII 8)
    else if (temp == 8 && counter > 0) {
      counter--;
      if (keyvalue[counter] == '.') decimalEntered = false;  // Reset decimal flag if '.' removed
      keyvalue[counter] = '\0';

      double val = atof(keyvalue);
      genie.WriteStr(0, val, 3);
    }
    // Enter key (ASCII 13)
    else if (temp == 13) {
      double finalValue = atof(keyvalue);
      Serial.print("Entered value: ");
      Serial.println(finalValue, 3);

      // Clear input buffer after enter
      memset(keyvalue, 0, sizeof(keyvalue));
      counter = 0;
      decimalEntered = false;
    }
  }

  //Servo Reset Button:

  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    switch (Event.reportObject.index) {
      case 0:
        Serial.println("WinButton 0 pressed");
        break;
      case 1:
        Serial.println("WinButton 1 pressed");
        break;
      case 2:
        Serial.println("HOME BUTTON PRESSED");
        PerformSensorlessHoming();
        break;
      case 3:
        Serial.println("WinButton 3 pressed");
        break;
      case 4:
        Serial.println("RESET SERVO BUTTON PRESSED");
        HandleAlerts();
        break;
    }
  }
}
void loop() {
  genie.DoEvents();  // Calls myGenieEventHandler internally, handling keypad input events
}


/*------------------------------------------------------------------------------
 * MoveAbsolutePosition
 *
 * Command step pulses to move the motor's current position to the absolute
 * position specified by "position"
 * Prints the move status to the USB serial port
 * Returns when HLFB asserts (indicating the motor has reached the commanded
 * position)
 *
 * Parameters:
 * int position    - The absolute position, in step pulses, to move to
 *
 * Returns: True/False depending on whether the move was successfully triggered.
 */
bool MoveAbsolutePosition(int32_t position) {
  // Check if a motor alert is currently preventing motion
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    PrintAlerts();
    if (false) {  // HANDLE_ALERTS is set to 0
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
    if (false) {  // HANDLE_ALERTS is set to 0
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


/*------------------------------------------------------------------------------
 * HandleAlerts
 *
 * Clears alerts, including motor faults.
 * Faults are cleared by cycling enable to the motor.
 * Alerts are cleared by clearing the ClearCore alert register directly.
  * Parameters:
 *    requires "motor" to be defined as a ClearCore motor connector
 *
 * Returns: 
 *    none
 */
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

void EStop() {
  //Not sure the best and safest way to do this...
  motor.EnableRequest(false);
  motor.EStopConnector();
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
}
