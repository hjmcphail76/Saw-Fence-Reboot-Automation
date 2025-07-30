#include "MechanismClasses.h"
#include "ClearCore.h"
#include "Utils.h"
#include <Arduino.h>


// --- Concrete Class for Belt Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
BeltMechanism::BeltMechanism(int res, int accel, int vel, float diameter, float gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel * res/60), maxVel(vel * res/60), pulleyDiameter(diameter), gearboxReduction(gearboxReduction), pulleyDiameterUnit(unit) {}

int BeltMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int BeltMechanism::GetMaxAccel() const {
  return maxAccel;
}
int BeltMechanism::GetMaxVel() const {
  return maxVel;
}

float BeltMechanism::CalculateStepsPerUnit() const {
  float diameterInInches = convertToInches(pulleyDiameter, pulleyDiameterUnit);
  // Steps per unit = (Motor Steps per Revolution / (Pulley Diameter * PI)) * Gearbox Reduction
  if (diameterInInches > 0) {
    return (static_cast<float>(motorProgInputRes) / (diameterInInches * PI)) * gearboxReduction;
  } else {
    Serial.println("Error: Pulley diameter cannot be zero for belt mechanism.");
    return 0.0;
  }
}

float BeltMechanism::GetPulleyDiameter() const {
  return pulleyDiameter;
}
UnitType BeltMechanism::GetParamUnit() const {
  return pulleyDiameterUnit;
}


// --- Concrete Class for Leadscrew Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
LeadscrewMechanism::LeadscrewMechanism(int res, int accel, int vel, float pitch, float gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel * res/60), maxVel(vel * res/60), leadscrewPitch(pitch), gearboxReduction(gearboxReduction), leadscrewPitchUnit(unit) {}

int LeadscrewMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int LeadscrewMechanism::GetMaxAccel() const {
  return maxAccel;
}
int LeadscrewMechanism::GetMaxVel() const {
  return maxVel;
}

float LeadscrewMechanism::CalculateStepsPerUnit() const {
  
  float pitchInInches = convertToInches(leadscrewPitch, leadscrewPitchUnit);
  // Steps per unit = (Motor Steps per Revolution / Leadscrew Pitch) * Gearbox Reduction
  if (pitchInInches > 0) {
    Serial.println("steps per inch: " + String((static_cast<float>(motorProgInputRes) / pitchInInches) * gearboxReduction));
    return (static_cast<float>(motorProgInputRes) / pitchInInches) * gearboxReduction;
  } else {
    Serial.println("Error: Leadscrew pitch cannot be zero for leadscrew mechanism.");
    return 0.0;
  }
}

float LeadscrewMechanism::GetLeadscrewPitch() const {
  return leadscrewPitch;
}
UnitType LeadscrewMechanism::GetParamUnit() const {
  return leadscrewPitchUnit;
}


// --- Concrete Class for Rack and Pinion Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
RackAndPinionMechanism::RackAndPinionMechanism(int res, int accel, int vel, float diameter, float gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel * res/60), maxVel(vel * res/60), pinionDiameter(diameter), gearboxReduction(gearboxReduction), pinionDiameterUnit(unit) {}

int RackAndPinionMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int RackAndPinionMechanism::GetMaxAccel() const {
  return maxAccel;
}
int RackAndPinionMechanism::GetMaxVel() const {
  return maxVel;
}

float RackAndPinionMechanism::CalculateStepsPerUnit() const {
  float diameterInInches = convertToInches(pinionDiameter, pinionDiameterUnit);
  // Steps per unit = (Motor Steps per Revolution / (Pinion Diameter * PI)) * Gearbox Reduction
  if (diameterInInches > 0) {
    return (static_cast<float>(motorProgInputRes) / (diameterInInches * PI)) * gearboxReduction;
  } else {
    Serial.println("Error: Pinion diameter cannot be zero for rack and pinion mechanism.");
    return 0.0;
  }
}

float RackAndPinionMechanism::GetPinionDiameter() const {
  return pinionDiameter;
}
UnitType RackAndPinionMechanism::GetParamUnit() const {
  return pinionDiameterUnit;
}
