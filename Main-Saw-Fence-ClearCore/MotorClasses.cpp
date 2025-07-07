#include <ClearCore.h>
#include "MotorClasses.h"
#include "MechanismClasses.h"
#include "ScreenClasses.h"

// ====================
// --- Base Motor ---
// ====================

bool Motor::MoveAbsolutePosition(int32_t position) {
  return false;  // default fallback
}

void Motor::StartSensorlessHoming() {
  homingState = HOMING_INIT;
}

void Motor::InitAndConnect() {
  // Default: do nothing
}

// ====================
// --- SDMotor ---
// ====================

SDMotor::SDMotor(Mechanism &mech)
  : maxAccel(mech.GetMaxAccel()),
    maxVel(mech.GetMaxVel()),
    motorProgInputRes(mech.GetMotorProgInputRes()) {}

void SDMotor::InitAndConnect() {
  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
  MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_STEP_AND_DIR);

  motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
  motor.VelMax(maxVel);
  motor.AccelMax(maxAccel);
}

int SDMotor::GetMaxAccel() const {
  return maxAccel;
}

int SDMotor::GetMaxVel() const {
  return maxVel;
}

int SDMotor::GetMotorProgInputRes() const {
  return motorProgInputRes;
}

bool SDMotor::MoveAbsolutePosition(int32_t position) {
  if (!hasHomed) {
    StartSensorlessHoming();
    return false;
  }

  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert detected.");
    HandleAlerts();
    Serial.println("Move canceled.");
    return false;
  }

  // Serial.print("Moving to absolute position: ");
  // Serial.println(position);

  motor.Move(position, MotorDriver::MOVE_TARGET_ABSOLUTE);

  while ((!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) && !motor.StatusReg().bit.AlertsPresent) {
    continue;
  }

  if (motor.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor alert during move.");
    HandleAlerts();
    return false;
  }

  Serial.println("Move Done");
  return true;
}

void SDMotor::StartSensorlessHoming() {
  homingState = HOMING_INIT;
}

void SDMotor::HandleAlerts() const {
  if (motor.AlertReg().bit.MotorFaulted) {
    Serial.println("Faults present. Cycling enable signal.");
    motor.EnableRequest(false);
    Delay_ms(10);
    motor.EnableRequest(true);
  }
  Serial.println("Clearing alerts.");
  motor.ClearAlerts();

  motor.EnableRequest(false);
}

void SDMotor::StateMachinePeriodic(Screen &screen) {
  switch (homingState) {
    case HOMING_INIT:
      hasHomed = false;
      Serial.println("Performing sensorless homing...");
      motor.EnableRequest(false);
      Delay_ms(10);
      motor.EnableRequest(true);
      homingState = HOMING_COMPLETE;

      //Again this is janky and this should be fixed at some point. It works though.
      // if (motor.HlfbState() == MotorDriver::HLFB_ASSERTED) {
      //   Serial.println("Sensorless homing complete!");
      //   hasHomed = true;
      //   homingState = HOMING_COMPLETE;
      // } else if (motor.StatusReg().bit.AlertsPresent) {
      //   Serial.println("Alert occurred during homing.");
      //   HandleAlerts();
      //   homingState = HOMING_ERROR;
      // }
      break;

    case HOMING_COMPLETE:
      motor.PositionRefSet(0);
      hasHomed = true;
      screen.SetScreen(MAIN_CONTROL_SCREEN);
      homingState = HOMING_IDLE;
      break;

    case HOMING_ERROR:
      screen.SetScreen(MAIN_CONTROL_SCREEN);
      homingState = HOMING_IDLE;
      hasHomed = false;
      break;

    case HOMING_IDLE:
      break;
  }
}

// ====================
// --- MCMotor ---
// ====================

MCMotor::MCMotor(Mechanism &mech)
  : maxAccel(mech.GetMaxAccel()), maxVel(mech.GetMaxVel()) {}

void MCMotor::InitAndConnect() {
  // Initialize for ClearCore MC motor
}

int MCMotor::GetMaxAccel() const {
  return maxAccel;
}

int MCMotor::GetMaxVel() const {
  return maxVel;
}

bool MCMotor::MoveAbsolutePosition(int32_t position) {
  if (!hasHomed) {
    StartSensorlessHoming();
    return false;
  }

  //Add MC movement here maybe
}

void MCMotor::StartSensorlessHoming() {
  homingState = HOMING_INIT;
}

void MCMotor::HandleAlerts() const {
  // motor.ClearAlerts();
}

void MCMotor::StateMachinePeriodic(Screen *screen) {
  switch (homingState) {
    case HOMING_INIT:
      break;
    case HOMING_COMPLETE:
      break;
    case HOMING_ERROR:
      break;
    case HOMING_IDLE:
      break;
  }
}
