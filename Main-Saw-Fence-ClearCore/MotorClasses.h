#ifndef MOTOR_CLASSES_H
#define MOTOR_CLASSES_H

#include <Arduino.h>

// --- Abstract Base Class ---
class Motor {
public:
  virtual int getMaxAccel() const = 0;
  virtual int getMaxVel() const = 0;

  virtual bool moveAbsolutePosition(int32_t position) {
    return false;
  }

  virtual void StartSensorlessHoming() {
  }

  virtual double getPulleyDiameter() const {
    return 0.0;
  }

  virtual ~Motor() {}
};

// --- SDMotor Implementation ---
class SDMotor : public Motor {
private:
  int motorProgInputRes;
  int maxAccel;
  int maxVel;

public:
  SDMotor(int maxAccel, int maxVel, int motorProgInputRes);

  int getMaxAccel() const override;
  int getMaxVel() const override;

  int getMotorProgInputRes() const;
};

// --- MCMotor Implementation ---
class MCMotor : public Motor {
private:
  int maxAccel;
  int maxVel;

public:
  MCMotor(int maxAccel, int maxVel);

  int getMaxAccel() const override;
  int getMaxVel() const override;

  int getMotorProgInputRes() const;  // Not virtual, optional utility
};

#endif  // MOTOR_CLASSES_H
