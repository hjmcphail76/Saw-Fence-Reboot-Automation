#include "MechanismClasses.h" // Include its own header file
#include "ClearCore.h" // Include ClearCore.h here as its functions (like Serial.println) are used in implementations

// Define conversion factor from millimeters to inches
const double MM_TO_INCH_FACTOR = 1.0 / 25.4;

// Helper function to convert a value to inches if needed
double convertToInches(double value, UnitType unit) {
    if (unit == UNIT_MILLIMETERS) {
        return value * MM_TO_INCH_FACTOR;
    }
    return value; // Already in inches or unknown unit, return as is
}

// --- Concrete Class for Belt Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
BeltMechanism::BeltMechanism(int res, int accel, int vel, double diameter, UnitType unit)
    : motorProgInputRes(res), maxAccel(accel), maxVel(vel), pulleyDiameter(diameter), pulleyDiameterUnit(unit) {}

int BeltMechanism::getMotorProgInputRes() const { return motorProgInputRes; }
int BeltMechanism::getMaxAccel() const { return maxAccel; }
int BeltMechanism::getMaxVel() const { return maxVel; } // Corrected method name: changed getMaxMaxVel() to getMaxVel()
double BeltMechanism::calculateStepsPerUnit(float gearboxRed) const {
    double diameterInInches = convertToInches(pulleyDiameter, pulleyDiameterUnit);
    // Steps per unit = (Motor Steps per Revolution / (Pulley Diameter * PI)) * Gearbox Reduction
    if (diameterInInches > 0) {
        return (static_cast<double>(motorProgInputRes) / (diameterInInches * PI)) * gearboxRed;
    } else {
        Serial.println("Error: Pulley diameter cannot be zero for belt mechanism.");
        return 0.0;
    }
}

double BeltMechanism::getPulleyDiameter() const { return pulleyDiameter; }
UnitType BeltMechanism::getParamUnit() const { return pulleyDiameterUnit; }


// --- Concrete Class for Leadscrew Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
LeadscrewMechanism::LeadscrewMechanism(int res, int accel, int vel, double pitch, UnitType unit)
    : motorProgInputRes(res), maxAccel(accel), maxVel(vel), leadscrewPitch(pitch), leadscrewPitchUnit(unit) {}

int LeadscrewMechanism::getMotorProgInputRes() const { return motorProgInputRes; }
int LeadscrewMechanism::getMaxAccel() const { return maxAccel; }
int LeadscrewMechanism::getMaxVel() const { return maxVel; }

double LeadscrewMechanism::calculateStepsPerUnit(float gearboxRed) const {
    double pitchInInches = convertToInches(leadscrewPitch, leadscrewPitchUnit);
    // Steps per unit = (Motor Steps per Revolution / Leadscrew Pitch) * Gearbox Reduction
    if (pitchInInches > 0) {
        return (static_cast<double>(motorProgInputRes) / pitchInInches) * gearboxRed;
    } else {
        Serial.println("Error: Leadscrew pitch cannot be zero for leadscrew mechanism.");
        return 0.0;
    }
}

double LeadscrewMechanism::getLeadscrewPitch() const { return leadscrewPitch; }
UnitType LeadscrewMechanism::getParamUnit() const { return leadscrewPitchUnit; }


// --- Concrete Class for Rack and Pinion Mechanism ---
// Corrected constructor definition to match declaration in MechanismClasses.h
RackAndPinionMechanism::RackAndPinionMechanism(int res, int accel, int vel, double diameter, UnitType unit)
    : motorProgInputRes(res), maxAccel(accel), maxVel(vel), pinionDiameter(diameter), pinionDiameterUnit(unit) {}

int RackAndPinionMechanism::getMotorProgInputRes() const { return motorProgInputRes; }
int RackAndPinionMechanism::getMaxAccel() const { return maxAccel; }
int RackAndPinionMechanism::getMaxVel() const { return maxVel; }

double RackAndPinionMechanism::calculateStepsPerUnit(float gearboxRed) const {
    double diameterInInches = convertToInches(pinionDiameter, pinionDiameterUnit);
    // Steps per unit = (Motor Steps per Revolution / (Pinion Diameter * PI)) * Gearbox Reduction
    if (diameterInInches > 0) {
        return (static_cast<double>(motorProgInputRes) / (diameterInInches * PI)) * gearboxRed;
    } else {
        Serial.println("Error: Pinion diameter cannot be zero for rack and pinion mechanism.");
        return 0.0;
    }
}

double RackAndPinionMechanism::getPinionDiameter() const { return pinionDiameter; }
UnitType RackAndPinionMechanism::getParamUnit() const { return pinionDiameterUnit; }
