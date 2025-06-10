#ifndef MECHANISM_CLASSES_H
#define MECHANISM_CLASSES_H

#include <Arduino.h> // Required for String and other Arduino types


// Enum for unit types
enum UnitType {
    UNIT_INCHES,
    UNIT_MILLIMETERS
};

struct Value {
    float myNum;
    UnitType unit;

    Value(float num, UnitType u) : myNum(num), unit(u) {}
};


// --- Abstract Base Class (Interface) for Mechanism Configuration ---
class Mechanism {
public:
    // Pure virtual functions that derived classes must implement
    virtual int getMotorProgInputRes() const = 0;
    virtual int getMaxAccel() const = 0;
    virtual int getMaxVel() const = 0;
    virtual double calculateStepsPerUnit(float gearboxRed) const = 0;

    // Virtual getters for mechanism-specific parameters and their units.
    // Return 0.0 by default for types where the parameter is not applicable.
    virtual double getPulleyDiameter() const { return 0.0; }
    virtual double getLeadscrewPitch() const { return 0.0; }
    virtual double getPinionDiameter() const { return 0.0; }
    virtual UnitType getParamUnit() const = 0; // Pure virtual getter for unit type

    // Virtual destructor for proper polymorphic deletion
    virtual ~Mechanism() {}
};

// --- Concrete Class for Belt Mechanism ---
class BeltMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double pulleyDiameter; // Specific parameter for belt drive
    UnitType pulleyDiameterUnit; // Unit for pulleyDiameter

public:
    // Constructor declaration
    BeltMechanism(int res, int accel, int vel, double diameter, UnitType unit);

    // Implementations of the pure virtual functions
    int getMotorProgInputRes() const override;
    int getMaxAccel() const override;
    int getMaxVel() const override;
    double calculateStepsPerUnit(float gearboxRed) const override;

    // Override specific getter for pulley diameter and its unit
    double getPulleyDiameter() const override;
    UnitType getParamUnit() const override;
};

// --- Concrete Class for Leadscrew Mechanism ---
class LeadscrewMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double leadscrewPitch; // Specific parameter for leadscrew
    UnitType leadscrewPitchUnit; // Unit for leadscrewPitch

public:
    // Constructor declaration
    LeadscrewMechanism(int res, int accel, int vel, double pitch, UnitType unit);

    // Implementations of the pure virtual functions
    int getMotorProgInputRes() const override;
    int getMaxAccel() const override;
    int getMaxVel() const override;
    double calculateStepsPerUnit(float gearboxRed) const override;

    // Override specific getter for leadscrew pitch and its unit
    double getLeadscrewPitch() const override;
    UnitType getParamUnit() const override;
};

// --- Concrete Class for Rack and Pinion Mechanism ---
class RackAndPinionMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double pinionDiameter; // Specific parameter for rack and pinion
    UnitType pinionDiameterUnit; // Unit for pinionDiameter

public:
    // Constructor declaration
    RackAndPinionMechanism(int res, int accel, int vel, double diameter, UnitType unit);

    // Implementations of the pure virtual functions
    int getMotorProgInputRes() const override;
    int getMaxAccel() const override;
    int getMaxVel() const override;
    double calculateStepsPerUnit(float gearboxRed) const override;

    // Override specific getter for pinion diameter and its unit
    double getPinionDiameter() const override;
    UnitType getParamUnit() const override;
};

#endif // MECHANISM_CLASSES_H
