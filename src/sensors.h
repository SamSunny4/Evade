#pragma once
#include <Arduino.h>
#include "config.h"

// Sector Angles for the 4 Ultrasonic Sensors (90° Spacing)
// Index 0: 0°   (Front)
// Index 1: 90°  (Right)
// Index 2: 180° (Back)
// Index 3: 270° (Left / -90°)

class SensorsManager {
public:
    SensorsManager();
    void init();
    void update(); // Non-blocking trigger and timeout watchdog

    float getDistance(uint8_t index) const;
    float getRawDistance(uint8_t index) const;
    const float* getAllDistances() const;

    // Obstacle Sector Checks
    bool isFrontBlocked(float threshold) const;
    bool isRearBlocked(float threshold) const;
    bool isLeftBlocked(float threshold) const;
    bool isRightBlocked(float threshold) const;
    bool isBothSidesBlocked(float threshold) const;
    bool isTrapped(float threshold) const;

    // Direction Solver: Returns relative angle in degrees (-180 to +180) of maximum clearance
    float getBestClearanceAngle(float threshold) const;

    // Overall Status Evaluation
    ObstacleStatus evaluateStatus(float threshold) const;

    // Direct ISR handlers
    static void IRAM_ATTR handleEchoChange(uint8_t index);

private:
    void triggerPulse();

    static volatile uint32_t echoStartMicros[NUM_ULTRASONIC_SENSORS];
    static volatile uint32_t echoDurationMicros[NUM_ULTRASONIC_SENSORS];
    static volatile bool echoReceived[NUM_ULTRASONIC_SENSORS];

    float rawHistory[NUM_ULTRASONIC_SENSORS][2]; // Double-read validation buffer
    uint8_t sampleCount[NUM_ULTRASONIC_SENSORS];
    float smoothedDistances[NUM_ULTRASONIC_SENSORS];
    uint32_t lastTriggerTime;
    bool triggerPending;
};

extern SensorsManager sensors;
