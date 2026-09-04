#pragma once
#include <Arduino.h>
#include "config.h"

class CommPiManager {
public:
    CommPiManager();
    void init();
    void update(); // Non-blocking incoming command parsing

    // Periodic telemetry broadcast to Raspberry Pi
    void sendTelemetry(ObstacleStatus status, float currentYaw, RobotControlMode mode);

    bool isPiOverrideActive() const;
    void setPiOverride(bool active);
    uint32_t getLastCommandTime() const;

private:
    void processCommand(const String &cmd);

    char rxBuffer[128];
    uint8_t rxIndex;
    uint32_t lastTxTime;
    uint32_t lastPiCmdTime;
    bool piOverride;
};

extern CommPiManager commPi;
