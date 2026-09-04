#include "evasion.h"
#include "sensors.h"
#include "imu.h"
#include "motors.h"
#include <math.h>

EvasionManager evasion;

EvasionManager::EvasionManager()
    : thresholdDistance(DEFAULT_THRESHOLD_CM),
      currentState(STATE_CLEAR_IDLE),
      currentStatus(STATUS_CLEAR),
      targetYaw(0.0f),
      stateStartTime(0) {}

void EvasionManager::init() {
    thresholdDistance = DEFAULT_THRESHOLD_CM;
    currentState = STATE_CLEAR_IDLE;
    currentStatus = STATUS_CLEAR;
    targetYaw = 0.0f;
    stateStartTime = millis();
}

void EvasionManager::setThreshold(float cm) {
    thresholdDistance = constrain(cm, MIN_EVADE_THRESHOLD_CM, 100.0f);
}

float EvasionManager::getThreshold() const {
    return thresholdDistance;
}

EvadeState EvasionManager::getCurrentState() const {
    return currentState;
}

ObstacleStatus EvasionManager::getObstacleStatus() const {
    return currentStatus;
}

float EvasionManager::getTargetYaw() const {
    return targetYaw;
}

void EvasionManager::update() {
    executeEvadeStateMachine();
}

void EvasionManager::executeEvadeStateMachine() {
    currentStatus = sensors.evaluateStatus(thresholdDistance);

    switch (currentState) {
        case STATE_CLEAR_IDLE: {
            if (currentStatus == STATUS_ALL_SIDES_TRAPPED) {
                currentState = STATE_TRAPPED;
                stateStartTime = millis();
                motors.stop();
            } else if (currentStatus == STATUS_OBJECT_BOTH_SIDES) {
                // Two sides blocked -> calculate best escape corridor using vector clearance
                float bestOffset = sensors.getBestClearanceAngle(thresholdDistance);
                targetYaw = imu.getYaw() + bestOffset;
                while (targetYaw > 180.0f) targetYaw -= 360.0f;
                while (targetYaw < -180.0f) targetYaw += 360.0f;

                currentState = STATE_ROTATING_TO_BEST_ANGLE;
                stateStartTime = millis();
            } else if (currentStatus == STATUS_OBJECT_IN_FRONT) {
                // Front blocked: if rear has clearance, move backward till threshold is restored
                if (!sensors.isRearBlocked(thresholdDistance * 0.9f)) {
                    currentState = STATE_BACKING_UP;
                    stateStartTime = millis();
                } else {
                    // Rear is also tight -> rotate to best open degree
                    float bestOffset = sensors.getBestClearanceAngle(thresholdDistance);
                    targetYaw = imu.getYaw() + bestOffset;
                    while (targetYaw > 180.0f) targetYaw -= 360.0f;
                    while (targetYaw < -180.0f) targetYaw += 360.0f;

                    currentState = STATE_ROTATING_TO_BEST_ANGLE;
                    stateStartTime = millis();
                }
            } else {
                // No obstacles inside threshold: hold position / stop
                motors.stop();
            }
            break;
        }

        case STATE_BACKING_UP: {
            // Check if front distance is now safely beyond threshold
            if (!sensors.isFrontBlocked(thresholdDistance)) {
                motors.stop();
                currentState = STATE_CLEAR_IDLE;
            } else if (sensors.isRearBlocked(CRITICAL_STOP_CM) || (millis() - stateStartTime > 3500)) {
                // Rear obstacle reached or timeout -> switch to rotation
                float bestOffset = sensors.getBestClearanceAngle(thresholdDistance);
                targetYaw = imu.getYaw() + bestOffset;
                while (targetYaw > 180.0f) targetYaw -= 360.0f;
                while (targetYaw < -180.0f) targetYaw += 360.0f;

                currentState = STATE_ROTATING_TO_BEST_ANGLE;
                stateStartTime = millis();
            } else {
                motors.backward(EVADE_SPEED);
            }
            break;
        }

        case STATE_ROTATING_TO_BEST_ANGLE: {
            float err = imu.getHeadingError(targetYaw);

            // Reached target angle within +/- 6 degrees, or rotation timeout (3s)
            if (fabs(err) < 6.0f || (millis() - stateStartTime > 3000)) {
                motors.stop();
                currentState = STATE_PUSHING_CLEAR;
                stateStartTime = millis();
            } else {
                // Closed-loop gyro rotation: speed scales with angular error
                uint8_t spd = (uint8_t)constrain((int)(fabs(err) * 2.5f + 140.0f), 140, (int)TURN_SPEED);
                if (err > 0) {
                    // Turn right (clockwise)
                    motors.rotateRight(spd);
                } else {
                    // Turn left (counter-clockwise)
                    motors.rotateLeft(spd);
                }
            }
            break;
        }

        case STATE_PUSHING_CLEAR: {
            // Push forward along the newly oriented escape path
            if (sensors.isFrontBlocked(CRITICAL_STOP_CM)) {
                // Critical obstacle encountered; stop immediately
                motors.stop();
                currentState = STATE_CLEAR_IDLE;
            } else if (!sensors.isFrontBlocked(thresholdDistance) || (millis() - stateStartTime > 1200)) {
                // Cleared threshold or push window finished
                motors.stop();
                currentState = STATE_CLEAR_IDLE;
            } else {
                motors.forward(EVADE_SPEED);
            }
            break;
        }

        case STATE_TRAPPED: {
            // All sides within threshold field
            motors.stop();

            // Recheck if space has opened
            if (!sensors.isTrapped(thresholdDistance)) {
                currentState = STATE_CLEAR_IDLE;
            } else {
                // Attempt to maximize distance from all sensors:
                // If there's an angular gradient with better clearance, face towards it
                float bestOffset = sensors.getBestClearanceAngle(thresholdDistance);
                if (fabs(bestOffset) > 25.0f && (millis() - stateStartTime > 1500)) {
                    targetYaw = imu.getYaw() + bestOffset;
                    while (targetYaw > 180.0f) targetYaw -= 360.0f;
                    while (targetYaw < -180.0f) targetYaw += 360.0f;
                    currentState = STATE_ROTATING_TO_BEST_ANGLE;
                    stateStartTime = millis();
                }
            }
            break;
        }
    }
}
