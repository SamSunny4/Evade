#include "motors.h"

MotorsManager motors;

MotorsManager::MotorsManager()
    : baseSpeed(DEFAULT_SPEED),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      currentCmd(CMD_STOP),
      eStopActive(true),
      relay1State(false),
      relay2State(false) {}

void MotorsManager::init() {
    pinMode(PIN_RELAY_1, OUTPUT);
    pinMode(PIN_RELAY_2, OUTPUT);

    // Initial safe boot state: Both relays de-energized
    digitalWrite(PIN_RELAY_1, RELAY_OFF);
    digitalWrite(PIN_RELAY_2, RELAY_OFF);

    relay1State = false;
    relay2State = false;
    eStopActive = true;

    stop();
    Serial.println("[RelayMotors] 2-Channel Relay Module initialized in SAFETY E-STOP state.");
}

void MotorsManager::applyRelays(bool r1, bool r2) {
    if (eStopActive) {
        digitalWrite(PIN_RELAY_1, RELAY_OFF);
        digitalWrite(PIN_RELAY_2, RELAY_OFF);
        relay1State = false;
        relay2State = false;
        currentLeftSpeed = 0;
        currentRightSpeed = 0;
        return;
    }

    relay1State = r1;
    relay2State = r2;

    digitalWrite(PIN_RELAY_1, r1 ? RELAY_ON : RELAY_OFF);
    digitalWrite(PIN_RELAY_2, r2 ? RELAY_ON : RELAY_OFF);

    currentLeftSpeed = r1 ? 255 : 0;
    currentRightSpeed = r2 ? 255 : 0;
}

void MotorsManager::setSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    bool r1 = (leftSpeed > 0);
    bool r2 = (rightSpeed > 0);

    applyRelays(r1, r2);

    if (!r1 && !r2) currentCmd = CMD_STOP;
    else if (r1 && r2) currentCmd = CMD_FORWARD;
    else if (r1 && !r2) currentCmd = CMD_ROTATE_RIGHT; // Left ON -> Turn Right
    else if (!r1 && r2) currentCmd = CMD_ROTATE_LEFT;  // Right ON -> Turn Left
}

void MotorsManager::forward(uint8_t speed) {
    // Both relays ON -> Move Forward
    currentCmd = CMD_FORWARD;
    applyRelays(true, true);
}

void MotorsManager::backward(uint8_t speed) {
    // 2-channel forward relay cannot reverse DC polarity -> safely halt
    currentCmd = CMD_BACKWARD;
    applyRelays(false, false);
}

void MotorsManager::rotateLeft(uint8_t speed) {
    // Relay 2 ON, Relay 1 OFF -> Right motors ON -> Turn Left
    currentCmd = CMD_ROTATE_LEFT;
    applyRelays(false, true);
}

void MotorsManager::rotateRight(uint8_t speed) {
    // Relay 1 ON, Relay 2 OFF -> Left motors ON -> Turn Right
    currentCmd = CMD_ROTATE_RIGHT;
    applyRelays(true, false);
}

void MotorsManager::pivotLeft(uint8_t speed) {
    // Relay 2 ON, Relay 1 OFF -> Turn Left
    currentCmd = CMD_PIVOT_LEFT;
    applyRelays(false, true);
}

void MotorsManager::pivotRight(uint8_t speed) {
    // Relay 1 ON, Relay 2 OFF -> Turn Right
    currentCmd = CMD_PIVOT_RIGHT;
    applyRelays(true, false);
}

void MotorsManager::stop() {
    // Both relays OFF -> Halt Motion
    currentCmd = CMD_STOP;
    applyRelays(false, false);
}

void MotorsManager::setBaseSpeed(uint8_t speed) {
    baseSpeed = speed;
}

uint8_t MotorsManager::getBaseSpeed() const {
    return baseSpeed;
}

MotorCommand MotorsManager::getCurrentCommand() const {
    return currentCmd;
}

void MotorsManager::getSpeeds(int16_t &left, int16_t &right) const {
    left = currentLeftSpeed;
    right = currentRightSpeed;
}

bool MotorsManager::isRelay1On() const {
    return relay1State;
}

bool MotorsManager::isRelay2On() const {
    return relay2State;
}

void MotorsManager::emergencyStop() {
    eStopActive = true;
    applyRelays(false, false);
    currentCmd = CMD_STOP;
    Serial.println("[RelayMotors] >>> EMERGENCY STOP ACTIVATED! ALL RELAYS DE-ENERGIZED <<<");
}

void MotorsManager::resetEmergencyStop() {
    eStopActive = false;
    applyRelays(false, false);
    currentCmd = CMD_STOP;
    Serial.println("[RelayMotors] Emergency stop reset. Relay controls restored.");
}

bool MotorsManager::isEmergencyStopped() const {
    return eStopActive;
}
