#include "comm_pi.h"
#include "motors.h"
#include "sensors.h"
#include "imu.h"

CommPiManager commPi;

CommPiManager::CommPiManager()
    : rxIndex(0),
      lastTxTime(0),
      lastPiCmdTime(0),
      piOverride(false) {
    memset(rxBuffer, 0, sizeof(rxBuffer));
}

void CommPiManager::init() {
    // HardwareSerial2: GPIO 16 (RX), GPIO 17 (TX)
    Serial2.begin(BAUD_PI_SERIAL, SERIAL_8N1, PIN_PI_RX, PIN_PI_TX);
    Serial.println("[CommPi] Serial2 initialized at 115200 baud for Raspberry Pi.");
}

void CommPiManager::sendTelemetry(ObstacleStatus status, float currentYaw, RobotControlMode mode) {
    uint32_t now = millis();
    // Transmit telemetry at 10 Hz (every 100ms)
    if (now - lastTxTime < 100) return;
    lastTxTime = now;

    const char* statusStr = "CLEAR";
    switch (status) {
        case STATUS_OBJECT_IN_FRONT:     statusStr = "OBJECT_IN_FRONT"; break;
        case STATUS_OBJECT_ON_LEFT:      statusStr = "OBJECT_ON_LEFT"; break;
        case STATUS_OBJECT_ON_RIGHT:     statusStr = "OBJECT_ON_RIGHT"; break;
        case STATUS_OBJECT_IN_REAR:      statusStr = "OBJECT_IN_REAR"; break;
        case STATUS_OBJECT_BOTH_SIDES:   statusStr = "OBJECT_BOTH_SIDES"; break;
        case STATUS_ALL_SIDES_TRAPPED:   statusStr = "ALL_SIDES_TRAPPED"; break;
        default:                         statusStr = "CLEAR"; break;
    }

    const char* modeStr = "AUTO_EVADE";
    if (mode == MODE_PI_OVERRIDE) modeStr = "PI_OVERRIDE";
    else if (mode == MODE_WEB_OVERRIDE) modeStr = "WEB_OVERRIDE";

    int16_t lSpd, rSpd;
    motors.getSpeeds(lSpd, rSpd);

    // Format primary status line for fast parsing on Pi
    Serial2.printf("STATUS:%s\n", statusStr);

    // Format rich JSON telemetry string (4 sensors: F, R, B, L)
    Serial2.printf("{\"status\":\"%s\",\"mode\":\"%s\",\"yaw\":%.1f,\"spd\":[%d,%d],\"d\":[%.1f,%.1f,%.1f,%.1f]}\n",
        statusStr,
        modeStr,
        currentYaw,
        lSpd, rSpd,
        sensors.getDistance(0), sensors.getDistance(1),
        sensors.getDistance(2), sensors.getDistance(3)
    );
}

void CommPiManager::processCommand(const String &cmd) {
    String trimmed = cmd;
    trimmed.trim();
    if (trimmed.length() == 0) return;

    lastPiCmdTime = millis();

    // Mode Toggle Commands
    if (trimmed == "CMD:ENABLE_EVADE" || trimmed == "EVADE_ENABLE") {
        piOverride = false;
        Serial.println("[CommPi] Pi enabled AUTO_EVADE mode.");
        Serial2.println("ACK:EVADE_ENABLED");
        return;
    }
    if (trimmed == "CMD:DISABLE_EVADE" || trimmed == "EVADE_DISABLE") {
        piOverride = true;
        motors.stop();
        Serial.println("[CommPi] Pi disabled AUTO_EVADE. PI_OVERRIDE active.");
        Serial2.println("ACK:EVADE_DISABLED");
        return;
    }

    // When in Pi Override, process direct motor controls
    if (piOverride) {
        if (trimmed == "CMD:MOVE:F" || trimmed == "MOVE:F") {
            motors.forward(motors.getBaseSpeed());
            Serial2.println("ACK:MOVE_FORWARD");
        }
        else if (trimmed == "CMD:MOVE:B" || trimmed == "MOVE:B") {
            motors.backward(motors.getBaseSpeed());
            Serial2.println("ACK:MOVE_BACKWARD");
        }
        else if (trimmed == "CMD:MOVE:L" || trimmed == "MOVE:L") {
            motors.rotateLeft(TURN_SPEED);
            Serial2.println("ACK:ROTATE_LEFT");
        }
        else if (trimmed == "CMD:MOVE:R" || trimmed == "MOVE:R") {
            motors.rotateRight(TURN_SPEED);
            Serial2.println("ACK:ROTATE_RIGHT");
        }
        else if (trimmed == "CMD:MOVE:STOP" || trimmed == "MOVE:STOP" || trimmed == "STOP") {
            motors.stop();
            Serial2.println("ACK:STOPPED");
        }
        else if (trimmed.startsWith("CMD:TANK:") || trimmed.startsWith("TANK:")) {
            int colonIdx = trimmed.indexOf("TANK:") + 5;
            String params = trimmed.substring(colonIdx);
            int commaIdx = params.indexOf(',');
            if (commaIdx > 0) {
                int leftVal = params.substring(0, commaIdx).toInt();
                int rightVal = params.substring(commaIdx + 1).toInt();
                motors.setSpeeds(leftVal, rightVal);
                Serial2.printf("ACK:TANK_%d_%d\n", leftVal, rightVal);
            }
        }
        else if (trimmed.startsWith("CMD:SPEED:") || trimmed.startsWith("SPEED:")) {
            int colonIdx = trimmed.lastIndexOf(':');
            int spd = trimmed.substring(colonIdx + 1).toInt();
            motors.setBaseSpeed(constrain(spd, 0, 255));
            Serial2.printf("ACK:SPEED_%d\n", spd);
        }
    }
}

void CommPiManager::update() {
    // Non-blocking read from Serial2
    while (Serial2.available() > 0) {
        char c = (char)Serial2.read();
        if (c == '\n' || c == '\r') {
            if (rxIndex > 0) {
                rxBuffer[rxIndex] = '\0';
                processCommand(String(rxBuffer));
                rxIndex = 0;
            }
        } else {
            if (rxIndex < sizeof(rxBuffer) - 1) {
                rxBuffer[rxIndex++] = c;
            } else {
                rxIndex = 0; // Overflow safety
            }
        }
    }

    // Failsafe: If Pi has override active but stops sending commands/heartbeat for > 2.0s, stop motors
    if (piOverride && (millis() - lastPiCmdTime > PI_HEARTBEAT_TIMEOUT_MS)) {
        motors.stop();
    }
}

bool CommPiManager::isPiOverrideActive() const {
    return piOverride;
}

void CommPiManager::setPiOverride(bool active) {
    piOverride = active;
}

uint32_t CommPiManager::getLastCommandTime() const {
    return lastPiCmdTime;
}
