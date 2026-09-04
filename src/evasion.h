#pragma once
#include <Arduino.h>
#include "config.h"

enum EvadeState {
    STATE_CLEAR_IDLE = 0,
    STATE_BACKING_UP,
    STATE_ROTATING_TO_BEST_ANGLE,
    STATE_PUSHING_CLEAR,
    STATE_TRAPPED
};

class EvasionManager {
public:
    EvasionManager();
    void init();
    void update(); // Non-blocking state machine tick

    void setThreshold(float cm);
    float getThreshold() const;

    EvadeState getCurrentState() const;
    ObstacleStatus getObstacleStatus() const;
    float getTargetYaw() const;

private:
    void executeEvadeStateMachine();

    float thresholdDistance;
    EvadeState currentState;
    ObstacleStatus currentStatus;
    float targetYaw;
    uint32_t stateStartTime;
};

extern EvasionManager evasion;
