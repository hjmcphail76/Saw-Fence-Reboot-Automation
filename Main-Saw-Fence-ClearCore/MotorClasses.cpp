#include "MotorClasses.h"

// --- SDMotor ---
SDMotor::SDMotor(int maxAccel, int maxVel, int motorProgInputRes)
  : maxAccel(maxAccel), maxVel(maxVel), motorProgInputRes(motorProgInputRes) {}

int SDMotor::getMaxAccel() const {
  return maxAccel;
}

int SDMotor::getMaxVel() const {
  return maxVel;
}

int SDMotor::getMotorProgInputRes() const {
  return motorProgInputRes;
}

// --- MCMotor ---
MCMotor::MCMotor(int maxAccel, int maxVel)
  : maxAccel(maxAccel), maxVel(maxVel) {
    
  }

int MCMotor::getMaxAccel() const {
  return maxAccel;
}

int MCMotor::getMaxVel() const {
  return maxVel;
}

int MCMotor::getMotorProgInputRes() const {
  return 0;  // Not relevant for MC motors
}
