#include "imu.h"
#include <math.h>

#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG       0x1A
#define MPU6050_GYRO_CONFIG  0x1B
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_GYRO_ZOUT_H  0x47

// Sensitivity factor for FS_SEL = 1 (+/- 500 deg/s) -> 65.5 LSB per deg/s
#define GYRO_SCALE_500DPS    65.5f

ImuManager imu;

ImuManager::ImuManager()
    : currentYaw(0.0f),
      gyroBiasZ(0.0f),
      rateZ(0.0f),
      lastMicros(0),
      connected(false) {}

bool ImuManager::writeRegister(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

bool ImuManager::readRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;

    Wire.requestFrom((uint8_t)MPU6050_I2C_ADDR, (uint8_t)length);
    for (size_t i = 0; i < length && Wire.available(); i++) {
        buffer[i] = Wire.read();
    }
    return true;
}

bool ImuManager::init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz Fast I2C

    uint8_t whoAmI = 0;
    if (!readRegisters(MPU6050_WHO_AM_I, &whoAmI, 1) || whoAmI != 0x68) {
        Serial.printf("[IMU] MPU6050 not detected at 0x68 (WHO_AM_I: 0x%02X). Continuing...\n", whoAmI);
        connected = false;
        return false;
    }

    // Wake up MPU6050 (clear sleep bit)
    writeRegister(MPU6050_PWR_MGMT_1, 0x00);
    delay(10);

    // Set sample rate divider = 7 -> 1kHz / (1 + 7) = 125 Hz
    writeRegister(MPU6050_SMPLRT_DIV, 0x07);

    // Digital Low Pass Filter = 44 Hz bandwidth
    writeRegister(MPU6050_CONFIG, 0x03);

    // Gyro Full Scale = +/- 500 deg/s
    writeRegister(MPU6050_GYRO_CONFIG, 0x08);

    connected = true;
    Serial.println("[IMU] MPU6050 initialized successfully.");

    calibrate(250);
    lastMicros = micros();
    return true;
}

void ImuManager::calibrate(uint16_t samples) {
    if (!connected) return;

    Serial.println("[IMU] Calibrating Gyro Z-axis. Keep bot stationary...");
    float sumZ = 0.0f;
    uint8_t buf[2];

    for (uint16_t i = 0; i < samples; i++) {
        if (readRegisters(MPU6050_GYRO_ZOUT_H, buf, 2)) {
            int16_t rawZ = (int16_t)((buf[0] << 8) | buf[1]);
            sumZ += (float)rawZ / GYRO_SCALE_500DPS;
        }
        delay(4);
    }
    gyroBiasZ = sumZ / (float)samples;
    Serial.printf("[IMU] Calibration Complete. Gyro Bias Z: %.3f deg/s\n", gyroBiasZ);
}

void ImuManager::update() {
    if (!connected) return;

    uint32_t now = micros();
    if (lastMicros == 0) {
        lastMicros = now;
        return;
    }

    float dt = (float)(now - lastMicros) / 1000000.0f;
    lastMicros = now;

    if (dt <= 0.0f || dt > 0.5f) {
        // Prevent huge integration leap after long delay
        return;
    }

    uint8_t buf[2];
    if (readRegisters(MPU6050_GYRO_ZOUT_H, buf, 2)) {
        int16_t rawZ = (int16_t)((buf[0] << 8) | buf[1]);
        float rawRateZ = (float)rawZ / GYRO_SCALE_500DPS;

        // Apply calibrated bias
        rateZ = rawRateZ - gyroBiasZ;

        // Deadband filter to eliminate stationary drift
        if (fabs(rateZ) < 0.25f) {
            rateZ = 0.0f;
        }

        // Integrate yaw: Note Z-axis right-hand rule (counter-clockwise is positive)
        currentYaw += rateZ * dt;
    }
}

float ImuManager::getYaw() const {
    // Normalize to [-180.0, +180.0] degrees
    float y = fmod(currentYaw, 360.0f);
    if (y > 180.0f) y -= 360.0f;
    else if (y < -180.0f) y += 360.0f;
    return y;
}

float ImuManager::getRawYaw() const {
    return currentYaw;
}

float ImuManager::getHeadingError(float targetYaw) const {
    float diff = targetYaw - getYaw();
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

float ImuManager::getAngularVelocityZ() const {
    return rateZ;
}

void ImuManager::resetHeading(float angleDeg) {
    currentYaw = angleDeg;
}

bool ImuManager::isConnected() const {
    return connected;
}
