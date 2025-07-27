#pragma once
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include "MechanismClasses.h"
#include "ScreenClasses.h"

class Motor {
public:
    enum HomingState {
        HOMING_INIT,
        HOMING_COMPLETE,
        HOMING_ERROR,
        HOMING_IDLE
    };

    virtual int GetMaxAccel() const = 0;
    virtual int GetMaxVel() const = 0;

    virtual bool MoveAbsolutePosition(int32_t position);
    virtual void StartSensorlessHoming();
    virtual void HandleAlerts() = 0;

    virtual void StateMachinePeriodic(Screen *screen) { }  // default empty
    virtual void InitAndConnect();  // default empty

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
    SDMotor(Mechanism *mech);

    int GetMaxAccel() const override;
    int GetMaxVel() const override;
    int GetMotorProgInputRes() const;

    bool MoveAbsolutePosition(int32_t position) override;
    void StartSensorlessHoming() override;
    void HandleAlerts() override;
    void StateMachinePeriodic(Screen *screen);

    void InitAndConnect() override;

    bool hasHomed = false;
};

