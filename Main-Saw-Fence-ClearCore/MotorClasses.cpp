#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MotorClasses.h"
#include "MechanismClasses.h"

// --- SDMotor ---
SDMotor::SDMotor(Mechanism *mech)
  : maxAccel(mech->GetMaxAccel()),
    maxVel(mech->GetMaxVel()),
    motorProgInputRes(mech->GetMotorProgInputRes()) {

  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);
  MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1,
                        Connector::CPM_MODE_STEP_AND_DIR);

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

void SDMotor::StartSensorlessHoming() {
  homingState = HomingState::HOMING_INIT;
}

void SDMotor::HandleAlerts() const {
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

void SDMotor::StateMachinePeriodic(Genie &genie) {
  switch (homingState) {
    case HOMING_INIT:
      hasHomed = false;
      Serial.println("Performing sensorless homing...");
      motor.EnableRequest(false);
      Delay_ms(10);
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
        HandleAlerts();
        homingState = HOMING_ERROR;
      }
      break;

    case HOMING_COMPLETE:
      motor.PositionRefSet(0);
      hasHomed = true;
      genie.SetForm(1);
      homingState = HOMING_IDLE;
      break;

    case HOMING_ERROR:
      genie.SetForm(1);
      homingState = HOMING_IDLE;
      hasHomed = false;
      break;

    case HOMING_IDLE:
      break;
  }
}

// --- MCMotor ---
MCMotor::MCMotor(Mechanism *mech)
  : maxAccel(maxAccel), maxVel(maxVel) {}

int MCMotor::GetMaxAccel() const {
  return maxAccel;
}

int MCMotor::GetMaxVel() const {
  return maxVel;
}
