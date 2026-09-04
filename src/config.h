#pragma once
#include <Arduino.h>

// ====================================================================
// ESP32 DevKit V1 Pin Configuration
// ====================================================================

// --- Ultrasonic Sensors (8-Directional Array) ---
// Shared Trigger Pin pulses all sensors simultaneously
#define PIN_US_TRIG            27

// 8 Dedicated Echo Input Pins (45° apart)
// S0: 0° (Front)
#define PIN_US_ECHO_0          34
// S1: 45° (Front-Right)
#define PIN_US_ECHO_1          35
// S2: 90° (Right)
#define PIN_US_ECHO_2          36  // VP
// S3: 135° (Back-Right)
#define PIN_US_ECHO_3          39  // VN
// S4: 180° (Back)
#define PIN_US_ECHO_4          32
// S5: 225° (Back-Left)
#define PIN_US_ECHO_5          33
// S6: 270° (Left)
#define PIN_US_ECHO_6          25
// S7: 315° (Front-Left)
#define PIN_US_ECHO_7          26

#define NUM_ULTRASONIC_SENSORS 8

// --- BTS7960B Motor Drivers (2x Modules for Tank Steering) ---
// Left Motor Driver
#define PIN_L_RPWM             18  // Forward PWM
#define PIN_L_LPWM             19  // Reverse PWM
#define PIN_L_EN               23  // Enable (Active HIGH, or tie to 3.3V)

// Right Motor Driver
#define PIN_R_RPWM             4   // Forward PWM
#define PIN_R_LPWM             5   // Reverse PWM
#define PIN_R_EN               13  // Enable (Active HIGH, or tie to 3.3V)

// LEDC PWM Channels
#define PWM_CH_L_FWD           0
#define PWM_CH_L_REV           1
#define PWM_CH_R_FWD           2
#define PWM_CH_R_REV           3

#define PWM_FREQ               20000 // 20kHz: silent running, no audible motor whine
#define PWM_RESOLUTION         8     // 8-bit: 0-255

// --- MPU6050 Gyro / Accelerometer (I2C) ---
#define PIN_I2C_SDA            21
#define PIN_I2C_SCL            22
#define MPU6050_I2C_ADDR       0x68

// --- Raspberry Pi Communication (UART2) ---
#define PIN_PI_RX              16  // Connect to Raspberry Pi TX (Pin 8 / GPIO 14)
#define PIN_PI_TX              17  // Connect to Raspberry Pi RX (Pin 10 / GPIO 15)
#define BAUD_PI_SERIAL         115200
#define PI_HEARTBEAT_TIMEOUT_MS 2000

// --- Default Robot Parameters ---
#define DEFAULT_THRESHOLD_CM   25.0f
#define MIN_EVADE_THRESHOLD_CM 10.0f
#define CRITICAL_STOP_CM       12.0f
#define MAX_SENSOR_DISTANCE_CM 300.0f
#define DEFAULT_SPEED          180
#define TURN_SPEED             190
#define EVADE_SPEED            170

// --- Access Point (AP) & OTA Configuration ---
#define AP_SSID                "ESP32-EvadeBot-AP"
#define AP_PASSWORD            "admin12345"
#define OTA_HOSTNAME           "esp32-evade-bot"
#define OTA_PASSWORD           "admin"
#define WEB_SERVER_PORT        80

// --- Control Modes ---
enum RobotControlMode {
    MODE_AUTO_EVADE = 0,
    MODE_PI_OVERRIDE,
    MODE_WEB_OVERRIDE
};

// --- Detection Status Signals ---
enum ObstacleStatus {
    STATUS_CLEAR = 0,
    STATUS_OBJECT_IN_FRONT,
    STATUS_OBJECT_BOTH_SIDES,
    STATUS_ALL_SIDES_TRAPPED
};
