#pragma once
#include <genieArduinoDEV.h>
#include <ClearCore.h>
#include "MechanismClasses.h"
#include "ScreenClasses.h"

class SDMotor {
public:
    enum HomingState {
        HOMING_INIT,
        HOMING_COMPLETE,
        HOMING_ERROR,
        HOMING_IDLE
    };

    SDMotor(Mechanism *mech);

    int GetMaxAccel() const;
    int GetMaxVel() const;
    int GetMotorProgInputRes() const;

    bool MoveAbsolutePosition(int32_t position);
    void StartSensorlessHoming();
    void HandleAlerts();
    void StateMachinePeriodic(Screen *screen);
    void InitAndConnect();

    bool hasHomed = false;

private:
    int maxAccel;
    int maxVel;
    int motorProgInputRes;
    MotorDriver &motor = ConnectorM0;

    HomingState homingState = HOMING_IDLE;
};
