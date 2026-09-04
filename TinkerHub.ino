/*
 * ==============================================================================
 * TinkerHub CyberBot: Autonomous Evade Bot with Dual BTS7960B & 8 Ultrasonic Sensors
 * ==============================================================================
 * ESP32 DevKit V1 Firmware
 * Compatible with PlatformIO and Arduino IDE 2.x
 *
 * Subsystems:
 * - 8-Directional Ultrasonic Radar (Non-blocking ISR based)
 * - MPU6050 6-DOF IMU (Gyro Yaw Tracking)
 * - Dual BTS7960B 43A H-Bridge Drivers (Tank Steering)
 * - FreeRTOS Dual-Core Processing (Core 0: WiFi/Web/OTA, Core 1: Control/Sensors)
 * - Bi-Directional UART2 Telemetry with Raspberry Pi
 * - Cyberpunk-themed Web Admin Portal with Live Radar Visualizer & Virtual Controls
 * - ArduinoOTA (Over-The-Air Wireless Flashing)
 */

#include "src/config.h"
#include "src/sensors.h"
#include "src/imu.h"
#include "src/motors.h"
#include "src/evasion.h"
#include "src/comm_pi.h"
#include "src/web_admin.h"

TaskHandle_t NetworkTaskHandle = NULL;

void networkTask(void *pvParameters) {
    webAdmin.init();
    for (;;) {
        webAdmin.update();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=======================================================");
    Serial.println("   TINKERHUB CYBERBOT - ESP32 DUAL BTS7960B CONTROLLER ");
    Serial.println("=======================================================");

    motors.init();
    sensors.init();
    imu.init();
    commPi.init();
    evasion.init();

    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        8192,
        NULL,
        1,
        &NetworkTaskHandle,
        0
    );

    Serial.println("[System] FreeRTOS initialized. Bot operational.");
}

// --- Serial Monitor Telemetry Reporting ---
static uint32_t lastSerialMonitorPrint = 0;
static ObstacleStatus lastReportedStatus = (ObstacleStatus)-1;

void printSerialMonitorTelemetry(RobotControlMode mode, ObstacleStatus status) {
    // Alert immediately on obstacle status change
    if (status != lastReportedStatus) {
        lastReportedStatus = status;
        const char* stStr = "CLEAR";
        if (status == STATUS_OBJECT_IN_FRONT)        stStr = "OBJECT_IN_FRONT";
        else if (status == STATUS_OBJECT_BOTH_SIDES) stStr = "OBJECT_BOTH_SIDES";
        else if (status == STATUS_ALL_SIDES_TRAPPED) stStr = "ALL_SIDES_TRAPPED";

        Serial.printf("\n>>> [ALERT: STATUS CHANGED] >>> %s (Threshold: %.1f cm) <<<\n", stStr, evasion.getThreshold());
    }

    // Print periodic telemetry HUD at 4 Hz (every 250ms)
    uint32_t now = millis();
    if (now - lastSerialMonitorPrint < 250) return;
    lastSerialMonitorPrint = now;

    const char* modeStr = "AUTO_EVADE";
    if (motors.isEmergencyStopped()) modeStr = "EMERG_STOP ";
    else if (mode == MODE_PI_OVERRIDE) modeStr = "PI_OVERRIDE ";
    else if (mode == MODE_WEB_OVERRIDE) modeStr = "WEB_OVERRIDE";

    int16_t lSpd, rSpd;
    motors.getSpeeds(lSpd, rSpd);

    Serial.printf("[YAW:%+6.1f°] [%s] [MOT:L=%+4d R=%+4d] | S0(F):%5.1f S1(FR):%5.1f S2(R):%5.1f S3(BR):%5.1f S4(B):%5.1f S5(BL):%5.1f S6(L):%5.1f S7(FL):%5.1f\n",
        imu.getYaw(),
        modeStr,
        lSpd, rSpd,
        sensors.getDistance(0),
        sensors.getDistance(1),
        sensors.getDistance(2),
        sensors.getDistance(3),
        sensors.getDistance(4),
        sensors.getDistance(5),
        sensors.getDistance(6),
        sensors.getDistance(7)
    );
}

void loop() {
    imu.update();
    sensors.update();
    commPi.update();

    RobotControlMode activeMode;
    if (webAdmin.isWebOverrideActive()) {
        activeMode = MODE_WEB_OVERRIDE;
    } else if (commPi.isPiOverrideActive()) {
        activeMode = MODE_PI_OVERRIDE;
    } else {
        activeMode = MODE_AUTO_EVADE;
    }

    if (motors.isEmergencyStopped()) {
        motors.stop();
    } else if (activeMode == MODE_AUTO_EVADE) {
        evasion.update();
    }

    ObstacleStatus status = evasion.getObstacleStatus();

    // 1. Send telemetry to Raspberry Pi over UART2 (115200 baud)
    commPi.sendTelemetry(status, imu.getYaw(), activeMode);

    // 2. Print live dashboard to USB Serial Monitor
    printSerialMonitorTelemetry(activeMode, status);

    delayMicroseconds(500);
}
