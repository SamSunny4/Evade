#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class ImuManager {
public:
    ImuManager();
    bool init();
    void update(); // Continuous non-blocking integration

    void calibrate(uint16_t samples = 300);
    void resetHeading(float angleDeg = 0.0f);

    float getYaw() const;          // Normalized -180.0 to +180.0 degrees
    float getRawYaw() const;       // Continuous un-wrapped degrees
    float getHeadingError(float targetYaw) const; // Shortest angular distance [-180, +180]
    float getAngularVelocityZ() const; // deg/sec

    bool isConnected() const;

private:
    bool writeRegister(uint8_t reg, uint8_t val);
    bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);

    float currentYaw;
    float gyroBiasZ;
    float rateZ;
    uint32_t lastMicros;
    bool connected;
};

extern ImuManager imu;
