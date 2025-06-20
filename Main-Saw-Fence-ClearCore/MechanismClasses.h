#ifndef MECHANISM_CLASSES_H
#define MECHANISM_CLASSES_H

#include <Arduino.h> // Required for String and other Arduino types


// Enum for unit types
enum UnitType {
    UNIT_INCHES,
    UNIT_MILLIMETERS
};

struct Value {
    float val;
    UnitType unit;

    Value(float num, UnitType u) : val(num), unit(u) {}
};


// --- Abstract Base Class (Interface) for Mechanism Configuration ---
class Mechanism {
public:
    // Pure virtual functions that derived classes must implement
    virtual int GetMotorProgInputRes() const = 0;
    virtual int GetMaxAccel() const = 0;
    virtual int GetMaxVel() const = 0;
    virtual double CalculateStepsPerUnit() const = 0;

    // Virtual getters for mechanism-specific parameters and their units.
    // Return 0.0 by default for types where the parameter is not applicable.
    virtual double GetPulleyDiameter() const { return 0.0; }
    virtual double GetLeadscrewPitch() const { return 0.0; }
    virtual double GetPinionDiameter() const { return 0.0; }
    virtual UnitType GetParamUnit() const = 0; // Pure virtual getter for unit type

    // Virtual destructor for proper polymorphic deletion
    virtual ~Mechanism() {}
};

// --- Concrete Class for Belt Mechanism ---
class BeltMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double gearboxReduction;
    double pulleyDiameter; // Specific parameter for belt drive
    UnitType pulleyDiameterUnit; // Unit for pulleyDiameter

public:
    // Constructor declaration
    BeltMechanism(int res, int accel, int vel, double diameter, double gearboxReduction, UnitType unit);

    // Implementations of the pure virtual functions
    int GetMotorProgInputRes() const override;
    int GetMaxAccel() const override;
    int GetMaxVel() const override;
    double CalculateStepsPerUnit() const override;

    // Override specific getter for pulley diameter and its unit
    double GetPulleyDiameter() const override;
    UnitType GetParamUnit() const override;
};

// --- Concrete Class for Leadscrew Mechanism ---
class LeadscrewMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double gearboxReduction;
    double leadscrewPitch; // Specific parameter for leadscrew
    UnitType leadscrewPitchUnit; // Unit for leadscrewPitch

public:
    // Constructor declaration
    LeadscrewMechanism(int res, int accel, int vel, double pitch, double gearboxReduction, UnitType unit);

    // Implementations of the pure virtual functions
    int GetMotorProgInputRes() const override;
    int GetMaxAccel() const override;
    int GetMaxVel() const override;
    double CalculateStepsPerUnit() const override;

    // Override specific getter for leadscrew pitch and its unit
    double GetLeadscrewPitch() const override;
    UnitType GetParamUnit() const override;
};

// --- Concrete Class for Rack and Pinion Mechanism ---
class RackAndPinionMechanism : public Mechanism {
private:
    int motorProgInputRes;
    int maxAccel;
    int maxVel;
    double gearboxReduction;
    double pinionDiameter; // Specific parameter for rack and pinion
    UnitType pinionDiameterUnit; // Unit for pinionDiameter

public:
    // Constructor declaration
    RackAndPinionMechanism(int res, int accel, int vel, double diameter, double gearboxReduction, UnitType unit);

    // Implementations of the pure virtual functions
    int GetMotorProgInputRes() const override;
    int GetMaxAccel() const override;
    int GetMaxVel() const override;
    double CalculateStepsPerUnit() const override;

    // Override specific getter for pinion diameter and its unit
    double GetPinionDiameter() const override;
    UnitType GetParamUnit() const override;
};

#endif // MECHANISM_CLASSES_H
