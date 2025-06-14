#pragma once
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include "MechanismClasses.h"
#include "ScreenClasses.h"

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

    virtual void StateMachinePeriodic(Screen *screen) { }  // default empty
    virtual void InitAndConnect();  // default empty

    bool hasHomed = false;

protected:
    HomingState homingState = HomingState::HOMING_IDLE;
};

// --- SDMotor class ---
class SDMotor : public Motor {
private:
    int maxAccel;
    int maxVel;
    int motorProgInputRes;
    MotorDriver &motor = ConnectorM0;

public:
    SDMotor(Mechanism &mech);

    int GetMaxAccel() const override;
    int GetMaxVel() const override;
    int GetMotorProgInputRes() const;

    bool MoveAbsolutePosition(int32_t position) override;
    void StartSensorlessHoming() override;
    void HandleAlerts() const override;
    void StateMachinePeriodic(Screen &screen);  // specific overload

    void InitAndConnect() override;  // new
};

// --- MCMotor class ---
class MCMotor : public Motor {
private:
    int maxAccel;
    int maxVel;
    MotorDriver &motor = ConnectorM3;

public:
    MCMotor(Mechanism &mech);

    int GetMaxAccel() const override;
    int GetMaxVel() const override;

    bool MoveAbsolutePosition(int32_t position) override;
    void StartSensorlessHoming() override;  // trivial
    void HandleAlerts() const override;     // trivial

    void StateMachinePeriodic(Screen *screen) override;  // stub
    void InitAndConnect() override;  // stub
};
