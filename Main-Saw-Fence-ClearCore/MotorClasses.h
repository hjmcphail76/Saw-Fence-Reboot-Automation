#pragma once
#include <genieArduinoDEV.h>
#include <ClearCore.h>

class Motor {
public:
  enum HomingState {
    HOMING_INIT,
    HOMING_WAIT_HLFB,
    HOMING_COMPLETE,
    HOMING_ERROR,
    HOMING_IDLE
  };

  virtual int GetMaxAccel() const = 0;
  virtual int GetMaxVel() const = 0;

  virtual bool MoveAbsolutePosition(int32_t position);
  virtual void StartSensorlessHoming();
  virtual void HandleAlerts() const = 0;

protected:
  HomingState homingState = HomingState::HOMING_IDLE;
  bool hasHomed = false;
};


// SDMotor class
class SDMotor : public Motor {
private:
  int motorProgInputRes;
  int maxAccel;
  int maxVel;
  MotorDriver &motor = ConnectorM0;

public:
  SDMotor(int maxAccel, int maxVel, int motorProgInputRes);

  int GetMaxAccel() const override;
  int GetMaxVel() const override;
  int GetMotorProgInputRes() const;

  bool MoveAbsolutePosition(int32_t position) override;
  void StartSensorlessHoming() override;
  void HandleAlerts() const override;
  void StateMachinePeriodic(Genie &genie);
};


// MCMotor class
class MCMotor : public Motor {
private:
  int maxAccel;
  int maxVel;
  MotorDriver &motor = ConnectorM3;

public:
  MCMotor(int maxAccel, int maxVel);

  int GetMaxAccel() const override;
  int GetMaxVel() const override;

  bool MoveAbsolutePosition(int32_t position) override;
  void StartSensorlessHoming() override {}
  void HandleAlerts() const override {}
};
