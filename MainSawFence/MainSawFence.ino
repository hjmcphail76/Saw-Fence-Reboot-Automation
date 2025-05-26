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
int SecondsInMinute = 60;
int AxisMoveVel = 1000;   // Example: mm/second
int AxisMoveAccel = 100;  // Example: mm/second^2
long AxisDwell = 1000;    // Example: milliseconds


long AxisMoveDist = 1000;           // Example: steps
int AxisFormFaultLEDGenieNum = 10;  // Example: Genie object number for fault LED
int AxisFormDistGenieNum = 11;      // Example: Genie object number for distance LED digit
int AxisFormVelGenieNum = 12;       // Example: Genie object number for velocity LED digit
int AxisFormAccelGenieNum = 13;     // Example: Genie object number for acceleration LED digit
int AxisFormDwellGenieNum = 14;     // Example: Genie object number for dwell LED digit


int DigitsToEdit = 5;  // Example: Number of digits to edit on the keypad
int LEDDigitToEdit;
int PreviousForm = 0;
bool AxisStopRun = false;
bool AxisFault = false;
bool AxisStartedDwell = false;
unsigned long AxisDwellTimeout = 0;
long AxisMoveTarget = 0;
bool AxisContinuous = false;
int AxisTorque = 0;
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


String f = "D";

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
  // Check if motor alert occurred during enabling
  // Clear alert if configured to do so
  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    PrintAlerts();
    if (false) {  // HANDLE_ALERTS is set to 0
      HandleAlerts();
    } else {
      Serial.println("Enable automatic alert handling by setting HANDLE_ALERTS to 1.");
    }
    Serial.println("Enabling may not have completed as expected. Proceed with caution.");
    Serial.println();
  } else {
    Serial.println("Motor Ready");
  }

  genie.SetForm(1);  // Change to Form 1
  CurrentForm = 1;

  genie.WriteContrast(15);  // Max Brightness

  // Initialize user-defined parameters with example values
  MotorProgInputRes = 200;
  SecondsInMinute = 60;
  AxisMoveVel = 1000;
  AxisMoveAccel = 100;
  AxisDwell = 1000;
  AxisMoveDist = 1000;
  AxisFormFaultLEDGenieNum = 10;
  AxisFormDistGenieNum = 11;
  AxisFormVelGenieNum = 12;
  AxisFormAccelGenieNum = 13;
  AxisFormDwellGenieNum = 14;
  DigitsToEdit = 5;
  PreviousForm = 0;
  AxisStopRun = false;
  AxisFault = false;
  AxisStartedDwell = false;
  AxisDwellTimeout = 0;
  AxisMoveTarget = 0;
  AxisContinuous = false;
  AxisTorque = 0;
  CurrentForm = 0;

  // Converts Revs/minute (RPM) into Steps/second
  ConversionFactor = (float)MotorProgInputRes / (float)SecondsInMinute;
}

void loop() {
  static unsigned long waitPeriod = millis();

  genie.DoEvents();

  if (millis() >= waitPeriod) {
    CurrentForm = genie.GetForm();

    switch (CurrentForm) {
      case 1:  // Main Screen
        //Update elements on control screen
        break;

      case 2:  // Settings screen
        //Update elements on settings screen
        break;
    }


    // // Motor Control:
    // if (AxisStopRun && !AxisFault) {
    //   if (motor.StepsComplete() && motor.HlfbState() == MotorDriver::HLFB_ASSERTED) {
    //     if (!AxisStartedDwell) {
    //       AxisStartedDwell = 1;
    //       AxisDwellTimeout = millis() + AxisDwell;
    //       Serial.print("M-0");
    //       Serial.print(" At Position ");
    //       Serial.println(AxisMoveTarget);
    //     }
    //   }

    //     if (AxisStartedDwell && millis() >= AxisDwellTimeout) {
    //       AxisStartedDwell = 0;
    //       AxisMoveTarget = (AxisMoveTarget == 0) ? AxisMoveDist : 0;
    //       Serial.print("M-0");
    //       Serial.print(" Starting move ");
    //       Serial.println(AxisMoveTarget);
    //     }
    //     MoveAbsolutePosition(AxisMoveTarget);

    // }

    // if (!AxisStopRun) {
    //   Serial.print("M-0");
    //   Serial.println(" Stopped");
    //   motor.MoveStopDecel(AxisMoveAccel * ConversionFactor); // Ensure units match
    // }

    // if (motor.HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
    //   AxisTorque = (int(abs(motor.HlfbPercent())));
    // }

    // if (motor.StatusReg().bit.AlertsPresent && !AxisFault) {
    //   AxisFault = true;
    //   Serial.print("M-0");
    //   Serial.println(" status: 'In Alert'");
    //   genie.WriteObject(GENIE_OBJ_USER_LED, AxisFormFaultLEDGenieNum, 1);
    // } else if (!motor.StatusReg().bit.AlertsPresent && AxisFault) {
    //   AxisFault = false;
    //   genie.WriteObject(GENIE_OBJ_USER_LED, AxisFormFaultLEDGenieNum, 0);
    // }

    waitPeriod = millis() + 50;
  }
}

void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);

  if (Event.reportObject.cmd == GENIE_REPORT_EVENT) {  //If there is a new report event from the screen

    if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {  //If it is a WINBUTTON:
      if (Event.reportObject.index == 0) {
      }
    }


    if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {
      if (Event.reportObject.index == 0) {
        temp = genie.GetEventData(&Event);

        if (temp >= '0' && temp <= '9' && counter < 10) {
          keyvalue[counter++] = temp;
          keyvalue[counter] = '\0';
          double val = atof(keyvalue);
          genie.WriteObject(GENIE_OBJ_LED_DIGITS, 0, (int)(val * 1000));  // Scaled for 3 decimals
        }

        else if (temp == 110 && !decimalEntered && counter < 9) {
          keyvalue[counter++] = '.';
          keyvalue[counter] = '\0';
          decimalEntered = true;
        }

        else if (temp == 8 && counter > 0) {
          counter--;
          if (keyvalue[counter] == '.') decimalEntered = false;
          keyvalue[counter] = '\0';
          double val = atof(keyvalue);
          genie.WriteObject(GENIE_OBJ_LED_DIGITS, 0, (int)(val * 1000));
        }

        else if (temp == 13) {
          double finalValue = atof(keyvalue);
          Serial.print("Entered value: ");
          Serial.println(finalValue, 3);

          memset(keyvalue, 0, sizeof(keyvalue));
          counter = 0;
          decimalEntered = false;
        }
      }
    }
  }
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
//------------------------------------------------------------------------------


/*------------------------------------------------------------------------------
 * PrintAlerts
 *
 * Prints active alerts.
 *
 * Parameters:
 * requires "motor" to be defined as a ClearCore motor connector
 *
 * Returns:
 * none
 */
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
//------------------------------------------------------------------------------


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
//------------------------------------------------------------------------------