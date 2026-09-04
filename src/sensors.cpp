#include "sensors.h"
#include <math.h>

static const uint8_t ECHO_PINS[NUM_ULTRASONIC_SENSORS] = {
    PIN_US_ECHO_0,
    PIN_US_ECHO_1,
    PIN_US_ECHO_2,
    PIN_US_ECHO_3
};

// Sector angles in degrees (Front: 0°, Right: 90°, Back: 180°, Left: -90° / 270°)
static const float SENSOR_ANGLES_DEG[NUM_ULTRASONIC_SENSORS] = {
    0.0f, 90.0f, 180.0f, -90.0f
};

volatile uint32_t SensorsManager::echoStartMicros[NUM_ULTRASONIC_SENSORS] = {0};
volatile uint32_t SensorsManager::echoDurationMicros[NUM_ULTRASONIC_SENSORS] = {0};
volatile bool SensorsManager::echoReceived[NUM_ULTRASONIC_SENSORS] = {false};

// Forward declare individual ISRs
void IRAM_ATTR isr0() { SensorsManager::handleEchoChange(0); }
void IRAM_ATTR isr1() { SensorsManager::handleEchoChange(1); }
void IRAM_ATTR isr2() { SensorsManager::handleEchoChange(2); }
void IRAM_ATTR isr3() { SensorsManager::handleEchoChange(3); }

SensorsManager sensors;

SensorsManager::SensorsManager() : lastTriggerTime(0), triggerPending(false) {
    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        smoothedDistances[i] = MAX_SENSOR_DISTANCE_CM;
        rawHistory[i][0] = MAX_SENSOR_DISTANCE_CM;
        rawHistory[i][1] = MAX_SENSOR_DISTANCE_CM;
        sampleCount[i] = 0;
    }
}

void IRAM_ATTR SensorsManager::handleEchoChange(uint8_t index) {
    uint32_t now = micros();
    uint8_t pin = ECHO_PINS[index];
    if (digitalRead(pin) == HIGH) {
        echoStartMicros[index] = now;
    } else {
        if (now >= echoStartMicros[index]) {
            echoDurationMicros[index] = now - echoStartMicros[index];
            echoReceived[index] = true;
        }
    }
}

void SensorsManager::init() {
    pinMode(PIN_US_TRIG_1, OUTPUT);
    digitalWrite(PIN_US_TRIG_1, LOW);
    pinMode(PIN_US_TRIG_2, OUTPUT);
    digitalWrite(PIN_US_TRIG_2, LOW);

    // Attach interrupt to each echo pin
    pinMode(ECHO_PINS[0], INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHO_PINS[0]), isr0, CHANGE);

    pinMode(ECHO_PINS[1], INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHO_PINS[1]), isr1, CHANGE);

    pinMode(ECHO_PINS[2], INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHO_PINS[2]), isr2, CHANGE);

    pinMode(ECHO_PINS[3], INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHO_PINS[3]), isr3, CHANGE);
}

void SensorsManager::triggerPulse() {
    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        echoReceived[i] = false;
    }
    // 10 microsecond pulse on both trigger lines simultaneously
    digitalWrite(PIN_US_TRIG_1, HIGH);
    digitalWrite(PIN_US_TRIG_2, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG_1, LOW);
    digitalWrite(PIN_US_TRIG_2, LOW);

    lastTriggerTime = millis();
    triggerPending = true;
}

void SensorsManager::update() {
    uint32_t now = millis();

    // Fire pulse every 50ms (20 Hz sampling)
    if (!triggerPending && (now - lastTriggerTime >= 50)) {
        triggerPulse();
        return;
    }

    // Check if measurement window (25ms timeout = ~4.2m) has elapsed
    if (triggerPending && (now - lastTriggerTime >= 25)) {
        for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
            float rawDist = MAX_SENSOR_DISTANCE_CM;
            if (echoReceived[i]) {
                // Distance in cm = time_us / 58.2
                rawDist = (float)echoDurationMicros[i] / 58.2f;
                if (rawDist < 2.0f || rawDist > MAX_SENSOR_DISTANCE_CM) {
                    rawDist = MAX_SENSOR_DISTANCE_CM;
                }
            }

            // --- Double Read Verification ---
            // Shift history buffer: [0] = current raw read, [1] = previous raw read
            rawHistory[i][1] = rawHistory[i][0];
            rawHistory[i][0] = rawDist;
            if (sampleCount[i] < 2) sampleCount[i]++;

            float validatedDist = rawDist;
            if (sampleCount[i] >= 2) {
                float diff = fabs(rawHistory[i][0] - rawHistory[i][1]);
                // If two consecutive reads are consistent (within 15cm or 20%), average them
                if (diff < 15.0f || diff < (rawHistory[i][1] * 0.20f)) {
                    validatedDist = (rawHistory[i][0] + rawHistory[i][1]) * 0.5f;
                } else if (rawHistory[i][0] < rawHistory[i][1]) {
                    // Abrupt drop: damp the first spike until confirmed by the next read
                    validatedDist = (rawHistory[i][0] * 0.4f) + (rawHistory[i][1] * 0.6f);
                } else {
                    // Abrupt jump: smooth transition
                    validatedDist = (rawHistory[i][0] * 0.5f) + (rawHistory[i][1] * 0.5f);
                }
            }

            // --- Exponential Moving Average (EMA) Smoothening ---
            smoothedDistances[i] = (smoothedDistances[i] * 0.65f) + (validatedDist * 0.35f);
        }
        triggerPending = false;
    }
}

float SensorsManager::getDistance(uint8_t index) const {
    if (index >= NUM_ULTRASONIC_SENSORS) return MAX_SENSOR_DISTANCE_CM;
    return smoothedDistances[index];
}

float SensorsManager::getRawDistance(uint8_t index) const {
    if (index >= NUM_ULTRASONIC_SENSORS) return MAX_SENSOR_DISTANCE_CM;
    return rawHistory[index][0];
}

const float* SensorsManager::getAllDistances() const {
    return smoothedDistances;
}

// Check sectors (4 sensors @ 90°: 0:Front, 1:Right, 2:Back, 3:Left)
bool SensorsManager::isFrontBlocked(float threshold) const {
    return (smoothedDistances[0] < threshold);
}

bool SensorsManager::isRightBlocked(float threshold) const {
    return (smoothedDistances[1] < threshold);
}

bool SensorsManager::isRearBlocked(float threshold) const {
    return (smoothedDistances[2] < threshold);
}

bool SensorsManager::isLeftBlocked(float threshold) const {
    return (smoothedDistances[3] < threshold);
}

bool SensorsManager::isBothSidesBlocked(float threshold) const {
    return isLeftBlocked(threshold) && isRightBlocked(threshold);
}

bool SensorsManager::isTrapped(float threshold) const {
    // Trapped if Front, Rear, and Both Sides are blocked or >= 3 sensors are within threshold
    uint8_t blockedCount = 0;
    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        if (smoothedDistances[i] < threshold) {
            blockedCount++;
        }
    }
    if (blockedCount >= 3) return true;
    return (isFrontBlocked(threshold) && isRearBlocked(threshold) && isBothSidesBlocked(threshold));
}

float SensorsManager::getBestClearanceAngle(float threshold) const {
    // 1. Vector field approach: Repulsion from obstacles
    float repulseX = 0.0f;
    float repulseY = 0.0f;

    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        float d = smoothedDistances[i];
        if (d < threshold * 1.5f) { // Consider anything within 1.5x threshold
            float rad = SENSOR_ANGLES_DEG[i] * (PI / 180.0f);
            float weight = (threshold * 1.5f - d); // Closer obstacles exert stronger repulsion
            repulseX += weight * sin(rad); // X is lateral (Right is +X, Left is -X)
            repulseY += weight * cos(rad); // Y is longitudinal (Front is +Y, Back is -Y)
        }
    }

    // If no strong repulsion, return 0 (continue straight)
    if (fabs(repulseX) < 0.1f && fabs(repulseY) < 0.1f) {
        // Find single sensor with maximum distance
        int maxIdx = 0;
        float maxD = -1.0f;
        for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
            if (smoothedDistances[i] > maxD) {
                maxD = smoothedDistances[i];
                maxIdx = i;
            }
        }
        return SENSOR_ANGLES_DEG[maxIdx];
    }

    // Escape vector is opposite to repulsion vector
    float escapeX = -repulseX;
    float escapeY = -repulseY;

    // Angle in degrees from Front (0 deg)
    float bestAngle = atan2(escapeX, escapeY) * (180.0f / PI);
    return bestAngle;
}

ObstacleStatus SensorsManager::evaluateStatus(float threshold) const {
    if (isTrapped(threshold)) {
        return STATUS_ALL_SIDES_TRAPPED;
    }
    if (isBothSidesBlocked(threshold)) {
        return STATUS_OBJECT_BOTH_SIDES;
    }
    if (isFrontBlocked(threshold)) {
        return STATUS_OBJECT_IN_FRONT;
    }
    if (isLeftBlocked(threshold)) {
        return STATUS_OBJECT_ON_LEFT;
    }
    if (isRightBlocked(threshold)) {
        return STATUS_OBJECT_ON_RIGHT;
    }
    if (isRearBlocked(threshold)) {
        return STATUS_OBJECT_IN_REAR;
    }
    return STATUS_CLEAR;
}
