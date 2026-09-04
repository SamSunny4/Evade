# TinkerHub CyberBot — Complete Hardware Wiring & Circuit Guide

This document provides complete, pin-by-pin hardware schematics, power distribution layouts, and terminal connection guides for the **TinkerHub ESP32 Autonomous Evade Robot**.

---

## 1. System Block Diagram

```
                             +---------------------------------------+
                             |       Main Battery (7.4V - 12V)       |
                             +-------------------+-------------------+
                                                 |
                         +-----------------------+-----------------------+
                         |                                               |
                         v                                               v
           +---------------------------+                   +---------------------------+
           |   DC-DC Buck Converter    |                   |   Relay Power Bus (+V)    |
           |   Output: 5.0V Regulated  |                   | (Through optional Master) |
           +-------------+-------------+                   +-------------+-------------+
                         |                                               |
         +---------------+---------------+                               |
         |               |               |                               |
         v               v               v                               v
  +--------------+ +-----------+ +---------------+                +--------------+
  | ESP32 DevKit | | 4x HC-SR04| | 2/3-Ch Relays |                | COM1 & COM2  |
  |  (VIN / 5V)  | |  (VCC 5V) | |  (VCC / JD)   |                | Relay Inputs |
  +-------+------+ +-----+-----+ +-------+-------+                +-------+------+
          |              |               |                                |
          | I2C (3.3V)   | Shared Trig   | IN1, IN2, (IN3)                |
          v              | 4x Echos      v                                v
  +--------------+       |       +---------------+                +--------------+
  |   MPU6050    |<------+       | ESP32 GPIOs   |                | NO1 -> Left  |
  | (SDA21/SCL22)|               | 18, 19, (4)   |                | NO2 -> Right |
  +--------------+               +---------------+                +-------+------+
                                                                          |
                                                                          v
                                                              +----------------------+
                                                              | 4x DC Geared Motors  |
                                                              +----------------------+
```

---

## 2. Power Distribution & Grounding Architecture

Robots with DC motors and mechanical relays generate electromagnetic interference (EMI) and inductive voltage spikes. Follow this wiring design to prevent ESP32 resets:

### Common Ground (Star Ground)
- **ALL grounds must tie together**: Connect ESP32 `GND`, Buck Converter `GND`, Relay Module `GND`, Sensor `GND`, Raspberry Pi `GND`, and Battery `(-)`.
- Always connect logic grounds to a central bus bar rather than daisy-chaining through the motor power wires.

### Power Rails Summary
| Power Rail | Voltage | Source | Supplies |
| :--- | :--- | :--- | :--- |
| **Motor High-Current Rail** | 7.4V – 12V | Battery (+) | Relay COM terminals -> DC Motors |
| **Logic 5V Rail** | 5.0V Regulated | DC-DC Step-Down (3A+) | ESP32 `VIN`, Relay Board `VCC`, Ultrasonic `VCC`, Raspberry Pi |
| **Sensor 3.3V Rail** | 3.3V Clean | ESP32 `3V3` Pin | MPU6050 IMU |

---

## 3. Relay Module Wiring (Motor Drive & Steering)

### 2-Channel Relay Board Pinout
| Relay Board Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **5V (VIN)** | Powers the optocoupler logic |
| **GND** | **GND** | Logic ground |
| **IN1** | **GPIO 18** | Triggers Relay 1 (Left Motors) |
| **IN2** | **GPIO 19** | Triggers Relay 2 (Right Motors) |

### Optocoupler Isolation Jumper (`VCC` / `JD-VCC`)
* Most multi-channel relay boards feature a 3-pin header with a blue jumper linking `VCC` and `JD-VCC`.
* **Standard Setup (Single 5V supply)**: Keep the jumper on `VCC-JDVCC` and supply 5V to the `VCC` pin.
* **Full Optical Isolation (Best for zero noise)**: Remove the jumper. Connect ESP32 `5V` to `VCC`. Connect an independent 5V supply to `JD-VCC` and `GND` to power only the relay coils.

### Relay Output Terminal Wiring (DC Motors)
Each relay has three screw terminals: `COM` (Common), `NO` (Normally Open), and `NC` (Normally Closed).

```
   [Battery (+) / +V Motor Power]
                 |
        +--------+--------+
        |                 |
        v                 v
   +----------+      +----------+
   | Relay 1  |      | Relay 2  |
   |   COM    |      |   COM    |
   |    NO    |      |    NO    |
   +----+-----+      +----+-----+
        |                 |
        v                 v
   [Left Motor +]   [Right Motor +]
        |                 |
        v                 v
   [Left Motor -]   [Right Motor -]
        |                 |
        +--------+--------+
                 |
                 v
   [Battery (-) Ground Return]
```

#### Motor Flyback Protection (Recommended)
Place a **1N4007** or **1N5819 Schottky diode** directly across each DC motor's terminals (Cathode/band to Motor `+`, Anode to Motor `-`). This absorbs inductive back-EMF spikes when relays switch off, preventing contact pitting and ESP32 resets.

---

## 4. Optional 3rd Relay (Single-Channel) Integration

You can use a 3rd single-channel relay for one of the following setups:

### Option A: Master Safety Kill-Switch / Hardware E-Stop (Recommended)
Wiring the 3rd relay in series before the motor relays gives you a physical power cutoff:
```
[Battery +] ---> [Relay 3 (Master) COM]
                 [Relay 3 (Master) NO ] ---> [Relay 1 COM] & [Relay 2 COM]
```
* **Control Pin**: Connect Relay 3 `IN` to **`GPIO 4`** or **`GPIO 23`**.
* **Safety Benefit**: Cutting Relay 3 kills all motor power immediately during emergencies or if the software watchdog triggers.

### Option B: High-Power Searchlight / Siren / Actuator
```
[12V / 5V +] ---> [Relay 3 COM]
                  [Relay 3 NO ] ---> [Light / Siren / Solenoid (+)]
[Light / Siren / Solenoid (-)] ---> [Battery / GND]
```
* **Control Pin**: Connect Relay 3 `IN` to **`GPIO 4`** or **`GPIO 23`**.

---

## 5. Ultrasonic Sensor Array (4 Directions @ 90° Spacing)

The robot uses **4 orthogonal ultrasonic sensors** (S0 through S3) positioned at 90-degree intervals around the chassis (Front, Right, Back, Left).

### Dual Trigger Lines (GPIO 27 & GPIO 14 / D14)
The sensors are triggered via two dedicated trigger lines pulsed simultaneously:
* **Trigger 1 (ESP32 GPIO 27)**: Connect to `TRIG` on Front (S0) and Right (S1) sensors.
* **Trigger 2 (ESP32 GPIO 14 / D14)**: Connect to `TRIG` on Back (S2) and Left (S3) sensors.
*(Both lines are driven simultaneously by the firmware, reducing current load per GPIO and allowing convenient modular wiring).*

### Dedicated Echo Lines
Each sensor returns its echo pulse to a dedicated ESP32 input pin:

| Sensor Index | Orientation | Direction Angle | ESP32 GPIO | Logic Level Handling |
| :--- | :--- | :--- | :--- | :--- |
| **S0** | Front | 0° | **GPIO 34** | Input Only (Use divider if 5V) |
| **S1** | Right | 90° | **GPIO 35** | Input Only (Use divider if 5V) |
| **S2** | Back | 180° | **GPIO 32** | Digital Input |
| **S3** | Left | 270° | **GPIO 25** | Digital Input |

*(Note: Pins GPIO 36, 39, 33, and 26 are freed up for future expansion).*

### Voltage Divider on 5V HC-SR04 Echo Pins
Standard HC-SR04 sensors output a **5V Echo pulse**, but the ESP32 is **3.3V tolerant only**. Use this simple resistor network on each Echo line:

```
Sensor ECHO Pin (5V)
       |
     [ 1kΩ Resistor ]
       |
       +-------------------> To ESP32 GPIO (e.g. GPIO 34)
       |
     [ 2kΩ Resistor ]
       |
      GND
```
*(Note: If using **HC-SR04P** or **RCWL-9610**, these support 3.3V natively and do not require resistors).*

---

## 6. MPU6050 6-DOF IMU (I2C)

The MPU6050 provides real-time gyro yaw integration for closed-loop turns and rotation tracking.

| MPU6050 Pin | ESP32 DevKit V1 Pin | Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Low-noise 3.3V power from ESP32 regulator |
| **GND** | **GND** | Ground |
| **SDA** | **GPIO 21** | I2C Data line (Hardware I2C) |
| **SCL** | **GPIO 22** | I2C Clock line (Hardware I2C) |
| **AD0** | **GND** | Selects default I2C address `0x68` (Leave tied to GND or unconnected) |
| **INT** | *Not Connected* | Interrupt line (polling/FIFO used) |

---

## 7. Raspberry Pi UART Interconnection

For autonomous telemetry, visualizer streaming, and mission override commands:

| Raspberry Pi 4/5 Pin | ESP32 Pin | Signal / Notes |
| :--- | :--- | :--- |
| **Pin 8 (GPIO 14 - TXD)** | **GPIO 16 (RX2)** | Pi commands transmitted to ESP32 |
| **Pin 10 (GPIO 15 - RXD)**| **GPIO 17 (TX2)** | ESP32 telemetry sent to Pi |
| **Pin 6 or Pin 9 (GND)** | **GND** | **Critical**: Shared Ground reference |

> [!WARNING]
> Do NOT connect the Raspberry Pi's 5V/3.3V power pins to the ESP32's 3.3V pin. Only share the **GND**, **TX**, and **RX** lines.

---

## 8. Complete Master Pin Mapping Table

| ESP32 GPIO | Direction | Connected Peripheral | Voltage | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 2** | Output | Built-in Status LED | 3.3V | Blinks when WiFi connected, OFF on disconnect/E-Stop |
| **GPIO 18** | Output | Relay 1 (Left Motors) | 5V Opto | Active LOW |
| **GPIO 19** | Output | Relay 2 (Right Motors) | 5V Opto | Active LOW |
| **GPIO 4** | Output | *(Optional)* Relay 3 (Master / Aux) | 5V Opto | Safe general purpose I/O |
| **GPIO 27** | Output | Ultrasonic Trigger 1 | 3.3V/5V | Primary trigger (S0, S1) |
| **GPIO 14** | Output | Ultrasonic Trigger 2 (D14) | 3.3V/5V | Secondary trigger (S2, S3) |
| **GPIO 34** | Input | Ultrasonic S0 Echo (Front - 0°) | 3.3V Max | Input-only pin (use divider if 5V) |
| **GPIO 35** | Input | Ultrasonic S1 Echo (Right - 90°) | 3.3V Max | Input-only pin (use divider if 5V) |
| **GPIO 32** | Input | Ultrasonic S2 Echo (Back - 180°)| 3.3V Max | Pull-down supported |
| **GPIO 25** | Input | Ultrasonic S3 Echo (Left - 270°)| 3.3V Max | Pull-down supported |
| **GPIO 21** | I/O | MPU6050 SDA | 3.3V | I2C Data (400 kHz) |
| **GPIO 22** | Output | MPU6050 SCL | 3.3V | I2C Clock |
| **GPIO 16** | Input | Raspberry Pi TX (UART2 RX) | 3.3V | 115200 baud |
| **GPIO 17** | Output | Raspberry Pi RX (UART2 TX) | 3.3V | 115200 baud |

---

## 9. Pre-Power Checklist

Before connecting your main battery:
1. [ ] **Verify Common Ground**: Continuity beep test between ESP32 GND, Relay GND, and Battery (-).
2. [ ] **Check Buck Converter Voltage**: Measure the step-down converter output with a multimeter to ensure it reads **5.0V - 5.2V** before plugging into the ESP32 `VIN`.
3. [ ] **Verify No Shorts across Motors**: Ensure motor leads are isolated and cannot touch the chassis.
4. [ ] **Ensure 3.3V on GPIO 34-39**: If using 5V HC-SR04 sensors, verify voltage dividers are installed on Echo lines.
5. [ ] **Relay Test**: With motors disconnected, power on the ESP32 and confirm the relay LEDs click and toggle appropriately during boot and commands.
