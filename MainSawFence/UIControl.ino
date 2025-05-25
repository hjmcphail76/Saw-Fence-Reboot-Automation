#include "ClearCore.h"
#include <genieArduinoDEV.h>

Genie genie;

//----------------------------------------------------------------------------------------
// Define the ClearCore COM port connected to the HMI
#define SerialPort Serial1          // ClearCore UART Port, connected to 4D Display COM1
#define CcSerialPort ConnectorCOM1   // ClearCore UART Port, connected to 4D Display COM1
//----------------------------------------------------------------------------------------

#define baudRate 115200           // ClearCore Baud Rate, for 4D Display

// Define motor 0 (Axis 1)
#define motorIndex 0
MotorDriver* motor = &ConnectorM0;
char motorConnectorName[] = "M-0";

// Defined to make it clear in the MoveAbsolutePosition function
#define Axis1 0

// Declares our user-defined helper function for absolute position moves.
bool MoveAbsolutePosition(int axisNumber, int position);

// Declares our user-defined helper function for velocity moves.
bool MoveAtVelocity(int axisNumber, int velocity);

int CurrentForm = -1;
int PreviousForm = -1;
int LEDDigitToEdit = -1;
int DigitsToEdit = -1;

int MotorProgInputRes = 800; // Motor Programmed Input Resolution
int SecondsInMinute = 60;
float ConversionFactor;

bool AxisStopRun = 0; // Stop = 0, Run = 1
int AxisCurrentPos = 0;
int AxisMoveDist = 4000; // Default SP
int AxisMoveVel = 800;   // Default SP
int AxisMoveAccel = 4000; // Default SP
int AxisDwell = 800;     // Default SP
unsigned long AxisDwellTimeout = 0;
int AxisTorque = 0;
int AxisCurrentPosLast = 0;
int AxisMoveDistLast = 0;
int AxisMoveVelLast = 0;
int AxisMoveAccelLast = 0;
int AxisDwellLast = 0;
int AxisTorqueLast = 0;
int AxisAnimation = 0; // 0 to 59 Frames

int AxisMoveTarget = 0;
bool AxisFault = 0;
bool AxisContinuous = 0;
bool AxisStartedDwell = 0;

//Genie Axis Form #s (for Axis 1)
int Form1AxisAnimationGenieNum = 4;
int Form1StartGenieNum = 0;
int Form1StopGenieNum = 1;

int AxisFormAnimationGenieNum = 7;
int AxisFormCurrentPositionGenieNum = 0;
int AxisFormDistGenieNum = 1;
int AxisFormVelGenieNum = 2;
int AxisFormAccelGenieNum = 3;
int AxisFormDwellGenieNum = 4;
int AxisFormTorqueGenieNum = 5;

int AxisFormFaultLEDGenieNum = 0;

int AxisFormClrFaultGenieNum = 6;
int AxisFormContModeGenieNum = 9;
int AxisFormBackGenieNum = 3;
int AxisFormDistEditGenieNum = 6;
int AxisFormVelEditGenieNum = 7;
int AxisFormAccelEditGenieNum = 8;
int AxisFormDwellEditGenieNum = 9;

char keyvalue[10];
int counter = 0;
int temp, sumTemp;

void setup() {
  // Sets the input clocking rate.
  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

  // Sets motor 0 into step and direction mode.
  MotorMgr.MotorModeSet(ConnectorM0, Connector::CPM_MODE_STEP_AND_DIR);

  // Converts Revs/minute (RPM) into Steps/second
  ConversionFactor = (float)MotorProgInputRes / (float)SecondsInMinute;

  // Set the motor's HLFB mode to bipolar PWM
  motor->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  // Set the HFLB carrier frequency to 482 Hz
  motor->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
  // Sets the maximum velocity
  motor->VelMax(AxisMoveVel * ConversionFactor);
  // Set the maximum acceleration
  motor->AccelMax(AxisMoveAccel * ConversionFactor);

  Serial.begin(115200); // Debug/Console printing over USB

  // Open the HMI serial port
  CcSerialPort.RtsMode(SerialBase::LINE_OFF);
  SerialPort.ttl(true);   // Set the Clearcore UART to be TTL
  SerialPort.begin(115200); // Set up Serial1

  while (!SerialPort) {
    continue;
  }

  delay(3000);

  while (!genie.Begin(SerialPort));

  if (genie.IsOnline()) {
    genie.AttachEventHandler(myGenieEventHandler);
  }

  // Enables the motor
  motor->EnableRequest(true);
  Serial.println("Motor 0 Enabled");
  delay(10);

  genie.SetForm(1); // Change to Form 1
  CurrentForm = 1;

  genie.WriteContrast(15); // Max Brightness
}

void loop() {
  static unsigned long waitPeriod = millis();

  genie.DoEvents();

  if (millis() >= waitPeriod) {
    CurrentForm = genie.GetForm();

    switch (CurrentForm) {
      case 1: // Main Screen
        if (motor->StatusReg().bit.StepsActive) {
          genie.WriteObject(GENIE_OBJ_VIDEO, Form1AxisAnimationGenieNum, AxisAnimation);
        }
        break;

      case 2: // Axis 1 Form
        AxisCurrentPos = constrain(motor->PositionRefCommanded(), 0, 65535);

        if (motor->StatusReg().bit.StepsActive) {
          genie.WriteObject(GENIE_OBJ_VIDEO, AxisFormAnimationGenieNum, AxisAnimation);
        }
        break;

      case 5: // Edit Parameter
        break;
    }

    // Animation
    if (motor->StatusReg().bit.StepsActive && !motor->StatusReg().bit.MoveDirection) {
      AxisAnimation = (AxisAnimation < 59) ? AxisAnimation + 1 : 0;
    } else if (motor->StatusReg().bit.StepsActive && motor->StatusReg().bit.MoveDirection) {
      AxisAnimation = (AxisAnimation > 0) ? AxisAnimation - 1 : 59;
    }

    // Motor Control
    if (AxisStopRun && !AxisFault) {
      if (motor->StepsComplete() && motor->HlfbState() == MotorDriver::HLFB_ASSERTED) {
        if (!AxisStartedDwell) {
          AxisStartedDwell = 1;
          AxisDwellTimeout = millis() + AxisDwell;
          Serial.print(motorConnectorName); Serial.print(" At Position "); Serial.println(AxisMoveTarget);
        }
      }

      if (AxisContinuous) {
        MoveAtVelocity(Axis1, AxisMoveVel * ConversionFactor);
      } else {
        if (AxisStartedDwell && millis() >= AxisDwellTimeout) {
          AxisStartedDwell = 0;
          AxisMoveTarget = (AxisMoveTarget == 0) ? AxisMoveDist : 0;
          Serial.print(motorConnectorName); Serial.print(" Starting move "); Serial.println(AxisMoveTarget);
        }
        MoveAbsolutePosition(Axis1, AxisMoveTarget);
      }
    }

    if (!AxisStopRun) {
      Serial.print(motorConnectorName); Serial.println(" Stopped");
      motor->MoveStopDecel(AxisMoveAccel);
    }

    if (motor->HlfbState() == MotorDriver::HLFB_HAS_MEASUREMENT) {
      AxisTorque = (int(abs(motor->HlfbPercent())));
    }

    if (motor->StatusReg().bit.AlertsPresent && !AxisFault) {
      AxisFault = true;
      Serial.print(motorConnectorName); Serial.println(" status: 'In Alert'");
      genie.WriteObject(GENIE_OBJ_USER_LED, AxisFormFaultLEDGenieNum, 1);
    } else if (!motor->StatusReg().bit.AlertsPresent && AxisFault) {
      AxisFault = false;
      genie.WriteObject(GENIE_OBJ_USER_LED, AxisFormFaultLEDGenieNum, 0);
    }

    waitPeriod = millis() + 50;
  }
}

void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);

  if (Event.reportObject.cmd == GENIE_REPORT_EVENT) {
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON) {
      if (Event.reportObject.index == Form1StartGenieNum) { // Start Button
        AxisStopRun = 1;
      } else if (Event.reportObject.index == Form1StopGenieNum) { // Stop Button
        AxisStopRun = 0;
      } else if (Event.reportObject.index == AxisFormClrFaultGenieNum) { // Clear Fault Button
        Serial.print(motorConnectorName); Serial.println(" Clearing Fault if present");
        if (motor->StatusReg().bit.AlertsPresent) {
          if (motor->StatusReg().bit.MotorInFault) {
            motor->EnableRequest(false);
            delay(10);
            motor->EnableRequest(true);
          }
          motor->ClearAlerts();
        }
      } else if (Event.reportObject.index == AxisFormContModeGenieNum) { // Continuous Mode Button
        int temp = genie.GetEventData(&Event);
        AxisContinuous = (temp == 1);
      }
    }

    if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
      if (Event.reportObject.index == 0) { // Axis 1 Info Button on Form 1
        genie.SetForm(2); // Change to Form 2 (Axis 1 Info)
      } else if (Event.reportObject.index == AxisFormBackGenieNum) { // Back Button on Axis 1 Form
        genie.SetForm(1); // Change to Form 1 (Main Screen)
      } else if (Event.reportObject.index == AxisFormDistEditGenieNum) { // Move Distance Edit
        PreviousForm = 2;
        LEDDigitToEdit = AxisFormDistGenieNum;
        DigitsToEdit = 5;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, 0);
        genie.SetForm(5); // Change to Form 5 - Edit Parameter
      } else if (Event.reportObject.index == AxisFormVelEditGenieNum) { // Move Velocity Edit
        PreviousForm = 2;
        LEDDigitToEdit = AxisFormVelGenieNum;
        DigitsToEdit = 4;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, 0);
        genie.SetForm(5);
      } else if (Event.reportObject.index == AxisFormAccelEditGenieNum) { // Move Acceleration Edit
        PreviousForm = 2;
        LEDDigitToEdit = AxisFormAccelGenieNum;
        DigitsToEdit = 4;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, 0);
        genie.SetForm(5);
      } else if (Event.reportObject.index == AxisFormDwellEditGenieNum) { // Dwell Edit
        PreviousForm = 2;
        LEDDigitToEdit = AxisFormDwellGenieNum;
        DigitsToEdit = 4;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, 0);
        genie.SetForm(5);
      } else if (Event.reportObject.index == 18) { // Cancel Button on Edit Parameter Form
        for (int f = 0; f < 5; f++) keyvalue[f] = 0;
        counter = 0;
        genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, 0);
        genie.SetForm(PreviousForm);
      }
    }

    if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {
      if (Event.reportObject.index == 0) { // Keyboard 0
        temp = genie.GetEventData(&Event);
        if (temp >= 48 && temp <= 57 && counter < DigitsToEdit) {
          keyvalue[counter] = temp;
          sumTemp = atoi(keyvalue);
          if (DigitsToEdit == 5 && sumTemp > 65535) sumTemp = 65535;
          genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, sumTemp);
          counter++;
        } else if (temp == 8) { // Backspace
          if (counter > 0) {
            counter--;
            keyvalue[counter] = 0;
            genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, atoi(keyvalue));
          }
        } else if (temp == 13) { // Enter
          int newValue = sumTemp;
          sumTemp = 0;
          for (int f = 0; f < 5; f++) keyvalue[f] = 0;
          counter = 0;

          if (LEDDigitToEdit == AxisFormDistGenieNum) AxisMoveDist = newValue;
          else if (LEDDigitToEdit == AxisFormVelGenieNum) {
            AxisMoveVel = constrain(newValue, 0, 6000);
            motor->VelMax(AxisMoveVel * ConversionFactor);
          } else if (LEDDigitToEdit == AxisFormAccelGenieNum) {
            AxisMoveAccel = newValue;
            motor->AccelMax(AxisMoveAccel * ConversionFactor);
          } else if (LEDDigitToEdit == AxisFormDwellGenieNum) AxisDwell = newValue;

          genie.SetForm(PreviousForm);
        }
      }
    }
  }
}

bool MoveAbsolutePosition(int position) {
  if (motor->StatusReg().bit.AlertsPresent) {
    Serial.print(motorConnectorName); Serial.println(" status: 'In Alert'. Move Canceled.");
    return false;
  }
  Serial.print(motorConnectorName); Serial.print(" Moving to absolute position: "); Serial.println(position);
  motor->Move(position, MotorDriver::MOVE_TARGET_ABSOLUTE);
  return true;
}
}