#include "motors.h"

MotorsManager motors;

MotorsManager::MotorsManager()
    : baseSpeed(DEFAULT_SPEED),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      currentCmd(CMD_STOP),
      eStopActive(true) {}

static void writePwm(uint8_t pin, uint8_t channel, uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(pin, duty);
#else
    ledcWrite(channel, duty);
#endif
}

void MotorsManager::init() {
    // Setup Enable pins: Initialize in E-STOP state (LOW = disabled)
    pinMode(PIN_L_EN, OUTPUT);
    digitalWrite(PIN_L_EN, LOW);

    pinMode(PIN_R_EN, OUTPUT);
    digitalWrite(PIN_R_EN, LOW);

    eStopActive = true;

    // Setup PWM pins
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(PIN_L_RPWM, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_L_LPWM, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_R_RPWM, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(PIN_R_LPWM, PWM_FREQ, PWM_RESOLUTION);
#else
    ledcSetup(PWM_CH_L_FWD, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_L_RPWM, PWM_CH_L_FWD);

    ledcSetup(PWM_CH_L_REV, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_L_LPWM, PWM_CH_L_REV);

    ledcSetup(PWM_CH_R_FWD, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_R_RPWM, PWM_CH_R_FWD);

    ledcSetup(PWM_CH_R_REV, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_R_LPWM, PWM_CH_R_REV);
#endif

    stop();
    Serial.println("[Motors] Dual BTS7960B initialized in SAFETY E-STOP state.");
}

void MotorsManager::applyLeftMotor(int16_t speed) {
    if (eStopActive) {
        writePwm(PIN_L_RPWM, PWM_CH_L_FWD, 0);
        writePwm(PIN_L_LPWM, PWM_CH_L_REV, 0);
        currentLeftSpeed = 0;
        return;
    }
    speed = constrain(speed, -255, 255);
    currentLeftSpeed = speed;

    if (speed > 0) {
        // Forward: RPWM = speed, LPWM = 0
        writePwm(PIN_L_RPWM, PWM_CH_L_FWD, (uint8_t)speed);
        writePwm(PIN_L_LPWM, PWM_CH_L_REV, 0);
    } else if (speed < 0) {
        // Reverse: RPWM = 0, LPWM = abs(speed)
        writePwm(PIN_L_RPWM, PWM_CH_L_FWD, 0);
        writePwm(PIN_L_LPWM, PWM_CH_L_REV, (uint8_t)(-speed));
    } else {
        // Brake / Coast: RPWM = 0, LPWM = 0
        writePwm(PIN_L_RPWM, PWM_CH_L_FWD, 0);
        writePwm(PIN_L_LPWM, PWM_CH_L_REV, 0);
    }
}

void MotorsManager::applyRightMotor(int16_t speed) {
    if (eStopActive) {
        writePwm(PIN_R_RPWM, PWM_CH_R_FWD, 0);
        writePwm(PIN_R_LPWM, PWM_CH_R_REV, 0);
        currentRightSpeed = 0;
        return;
    }
    speed = constrain(speed, -255, 255);
    currentRightSpeed = speed;

    if (speed > 0) {
        // Forward: RPWM = speed, LPWM = 0
        writePwm(PIN_R_RPWM, PWM_CH_R_FWD, (uint8_t)speed);
        writePwm(PIN_R_LPWM, PWM_CH_R_REV, 0);
    } else if (speed < 0) {
        // Reverse: RPWM = 0, LPWM = abs(speed)
        writePwm(PIN_R_RPWM, PWM_CH_R_FWD, 0);
        writePwm(PIN_R_LPWM, PWM_CH_R_REV, (uint8_t)(-speed));
    } else {
        // Brake / Coast: RPWM = 0, LPWM = 0
        writePwm(PIN_R_RPWM, PWM_CH_R_FWD, 0);
        writePwm(PIN_R_LPWM, PWM_CH_R_REV, 0);
    }
}

void MotorsManager::setSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    applyLeftMotor(leftSpeed);
    applyRightMotor(rightSpeed);

    if (leftSpeed == 0 && rightSpeed == 0) currentCmd = CMD_STOP;
    else if (leftSpeed > 0 && rightSpeed > 0) currentCmd = CMD_FORWARD;
    else if (leftSpeed < 0 && rightSpeed < 0) currentCmd = CMD_BACKWARD;
    else if (leftSpeed < 0 && rightSpeed > 0) currentCmd = CMD_ROTATE_LEFT;
    else if (leftSpeed > 0 && rightSpeed < 0) currentCmd = CMD_ROTATE_RIGHT;
    else currentCmd = CMD_FORWARD;
}

void MotorsManager::forward(uint8_t speed) {
    currentCmd = CMD_FORWARD;
    setSpeeds(speed, speed);
}

void MotorsManager::backward(uint8_t speed) {
    currentCmd = CMD_BACKWARD;
    setSpeeds(-speed, -speed);
}

void MotorsManager::rotateLeft(uint8_t speed) {
    currentCmd = CMD_ROTATE_LEFT;
    setSpeeds(-speed, speed);
}

void MotorsManager::rotateRight(uint8_t speed) {
    currentCmd = CMD_ROTATE_RIGHT;
    setSpeeds(speed, -speed);
}

void MotorsManager::pivotLeft(uint8_t speed) {
    currentCmd = CMD_PIVOT_LEFT;
    setSpeeds(0, speed);
}

void MotorsManager::pivotRight(uint8_t speed) {
    currentCmd = CMD_PIVOT_RIGHT;
    setSpeeds(speed, 0);
}

void MotorsManager::stop() {
    currentCmd = CMD_STOP;
    setSpeeds(0, 0);
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

void MotorsManager::emergencyStop() {
    eStopActive = true;
    stop();
    digitalWrite(PIN_L_EN, LOW);
    digitalWrite(PIN_R_EN, LOW);
    Serial.println("[Motors] >>> EMERGENCY STOP ACTIVATED! ALL MOTORS CUT OFF <<<");
}

void MotorsManager::resetEmergencyStop() {
    digitalWrite(PIN_L_EN, HIGH);
    digitalWrite(PIN_R_EN, HIGH);
    eStopActive = false;
    stop();
    Serial.println("[Motors] Emergency stop reset. Motor controls restored.");
}

bool MotorsManager::isEmergencyStopped() const {
    return eStopActive;
}
