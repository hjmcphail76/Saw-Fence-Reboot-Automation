#include "MechanismClasses.h"  // Include its own header file
#include "ClearCore.h"         // Include ClearCore.h here as its functions (like Serial.println) are used in implementations

// Define conversion factor from millimeters to inches
const double MM_TO_INCH_FACTOR = 1.0 / 25.4;


double convertToInches(double value, UnitType unit) {
  if (unit == UNIT_MILLIMETERS) {
    return value * MM_TO_INCH_FACTOR;
  }
  return value;
}

double convertFromInches(float valueInInches, UnitType targetUnit) {
  if (targetUnit == UNIT_MILLIMETERS) {
    return valueInInches / MM_TO_INCH_FACTOR;
  }
  return valueInInches;
}

double convertUnits(double value, UnitType from, UnitType to) {
  if (from == to) return value;
  if (from == UnitType::UNIT_INCHES && to == UnitType::UNIT_MILLIMETERS)
    return value * 25.4;
  if (from == UnitType::UNIT_MILLIMETERS && to == UnitType::UNIT_INCHES)
    return value / 25.4;
  // Add other unit pairs if needed
  return value;
}

String getUnitString(UnitType unit) {
  switch (unit) {
    case UNIT_INCHES:
      return "in";
    case UNIT_MILLIMETERS:
      return "mm";
    default:
      return "";
  }
}

// --- Concrete Class for Belt Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
BeltMechanism::BeltMechanism(int res, int accel, int vel, double diameter, double gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel), maxVel(vel), pulleyDiameter(diameter), gearboxReduction(gearboxReduction), pulleyDiameterUnit(unit) {}

int BeltMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int BeltMechanism::GetMaxAccel() const {
  return maxAccel;
}
int BeltMechanism::GetMaxVel() const {
  return maxVel;
}

double BeltMechanism::CalculateStepsPerUnit() const {
  double diameterInInches = convertToInches(pulleyDiameter, pulleyDiameterUnit);
  // Steps per unit = (Motor Steps per Revolution / (Pulley Diameter * PI)) * Gearbox Reduction
  if (diameterInInches > 0) {
    return (static_cast<double>(motorProgInputRes) / (diameterInInches * PI)) * gearboxReduction;
  } else {
    Serial.println("Error: Pulley diameter cannot be zero for belt mechanism.");
    return 0.0;
  }
}

double BeltMechanism::GetPulleyDiameter() const {
  return pulleyDiameter;
}
UnitType BeltMechanism::GetParamUnit() const {
  return pulleyDiameterUnit;
}


// --- Concrete Class for Leadscrew Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
LeadscrewMechanism::LeadscrewMechanism(int res, int accel, int vel, double pitch, double gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel), maxVel(vel), leadscrewPitch(pitch), gearboxReduction(gearboxReduction), leadscrewPitchUnit(unit) {}

int LeadscrewMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int LeadscrewMechanism::GetMaxAccel() const {
  return maxAccel;
}
int LeadscrewMechanism::GetMaxVel() const {
  return maxVel;
}

double LeadscrewMechanism::CalculateStepsPerUnit() const {
  double pitchInInches = convertToInches(leadscrewPitch, leadscrewPitchUnit);
  // Steps per unit = (Motor Steps per Revolution / Leadscrew Pitch) * Gearbox Reduction
  if (pitchInInches > 0) {
    return (static_cast<double>(motorProgInputRes) / pitchInInches) * gearboxReduction;
  } else {
    Serial.println("Error: Leadscrew pitch cannot be zero for leadscrew mechanism.");
    return 0.0;
  }
}

double LeadscrewMechanism::GetLeadscrewPitch() const {
  return leadscrewPitch;
}
UnitType LeadscrewMechanism::GetParamUnit() const {
  return leadscrewPitchUnit;
}


// --- Concrete Class for Rack and Pinion Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
RackAndPinionMechanism::RackAndPinionMechanism(int res, int accel, int vel, double diameter, double gearboxReduction, UnitType unit)
  : motorProgInputRes(res), maxAccel(accel), maxVel(vel), pinionDiameter(diameter), gearboxReduction(gearboxReduction), pinionDiameterUnit(unit) {}

int RackAndPinionMechanism::GetMotorProgInputRes() const {
  return motorProgInputRes;
}
int RackAndPinionMechanism::GetMaxAccel() const {
  return maxAccel;
}
int RackAndPinionMechanism::GetMaxVel() const {
  return maxVel;
}

double RackAndPinionMechanism::CalculateStepsPerUnit() const {
  double diameterInInches = convertToInches(pinionDiameter, pinionDiameterUnit);
  // Steps per unit = (Motor Steps per Revolution / (Pinion Diameter * PI)) * Gearbox Reduction
  if (diameterInInches > 0) {
    return (static_cast<double>(motorProgInputRes) / (diameterInInches * PI)) * gearboxReduction;
  } else {
    Serial.println("Error: Pinion diameter cannot be zero for rack and pinion mechanism.");
    return 0.0;
  }
}

double RackAndPinionMechanism::GetPinionDiameter() const {
  return pinionDiameter;
}
UnitType RackAndPinionMechanism::GetParamUnit() const {
  return pinionDiameterUnit;
}
