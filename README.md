# TinkerHub CyberBot - ESP32 Autonomous Evade Robot

A high-performance autonomous robot controller firmware for the **ESP32 DevKit V1** with **4-directional orthogonal ultrasonic obstacle detection (90° spacing)**, an **MPU6050 gyro orientation system**, a **2-channel relay module (digital tank steering)**, **WiFi Disconnect Emergency Stop Safety**, **bi-directional Raspberry Pi UART telemetry**, and an embedded **Cyberpunk Web Admin Portal** with wireless **ArduinoOTA** programming.

---

## Hardware Architecture

```
                                  +-----------------------+
                                  |   Raspberry Pi 4 / 5  |
                                  | (UART TX/RX Telemetry)|
                                  +-----------+-----------+
                                              | UART2 (115200)
                                              | GPIO 16 (RX) / 17 (TX)
                                              v
+------------------------+        +-----------+-----------+        +------------------------+
| 4x Ultrasonic Sensors  |        |      ESP32 DevKit     |        |   MPU6050 Gyro / IMU   |
| (Trig1: 27, Trig2: 14) +------->|        V1 (30P)       |<-------+  (I2C: GPIO 21 / 22)   |
| 4x Echos: 34, 35, 32, 25|       +-----------+-----------+        +------------------------+
+------------------------+                    |
                                              | Digital Relay Control
                                              v
                               +--------------+--------------+
                               |    2-Channel Relay Module   |
                               | (Relay 1: L / Relay 2: R)   |
                               +--------------+--------------+
                                              |
                                              v
                               [ 4x High-Torque DC Motors ]
```

> [!TIP]
> 📖 **Looking for full schematics, terminal connections, and power distribution?**
> Check out the complete [Hardware Wiring & Circuit Guide](WIRING_DIAGRAM.md).

---

## Complete ESP32 DevKit V1 Pin Mapping

### 1. 2-Channel Relay Module (Left & Right Motor Steering)
| Relay Module Pin | Function | ESP32 GPIO | Description |
| :--- | :--- | :--- | :--- |
| **IN1 (Relay 1)** | Left Motors | **GPIO 18** | Digital Output (Active LOW by default) |
| **IN2 (Relay 2)** | Right Motors | **GPIO 19** | Digital Output (Active LOW by default) |
| **VCC** | Relay Coil Power | **5V** (VIN) | 5V relay logic supply |
| **GND** | Ground | **GND** | Common ground with ESP32 & battery |

#### Relay Switching Truth Table
| Maneuver | Relay 1 (CH 1 / Left Motors) | Relay 2 (CH 2 / Right Motors) | Physical Motion |
| :--- | :--- | :--- | :--- |
| **FORWARD** | **ON** | **ON** | Both Left & Right motors ON |
| **TURN RIGHT** | **ON** | **OFF** | Left motors ON -> Robot pivots/turns Right |
| **TURN LEFT** | **OFF** | **ON** | Right motors ON -> Robot pivots/turns Left |
| **STOP / E-STOP** | **OFF** | **OFF** | Both motors cut off |

### 2. Ultrasonic Sensor Array (4 Directions @ 90° Spacing)
The 4 sensors are pulsed via two synchronized trigger pins (**GPIO 27** and **GPIO 14 / D14**). Each sensor's echo is measured independently and non-blockingly via hardware interrupts:

| Sensor Index | Direction | Angle | ESP32 GPIO | Logic Level Note |
| :--- | :--- | :--- | :--- | :--- |
| **TRIG 1** | Front & Right | — | **GPIO 27** | Primary 3.3V trigger output |
| **TRIG 2 (D14)**| Back & Left | — | **GPIO 14** | Secondary 3.3V trigger output |
| **S0** | Front | 0° | **GPIO 34** | Input Only (1kΩ / 2kΩ divider if 5V) |
| **S1** | Right | 90° | **GPIO 35** | Input Only (Divider if 5V) |
| **S2** | Back | 180° | **GPIO 32** | Digital Input |
| **S3** | Left | 270° | **GPIO 25** | Digital Input |

> [!CAUTION]
> **Voltage Divider on 5V HC-SR04 Echo Lines:**
> The ESP32 inputs are **3.3V logic only**. If you are using standard 5V HC-SR04 sensors, step down each Echo line using a simple two-resistor voltage divider (1kΩ in series from Echo to GPIO, and 2kΩ from GPIO to GND). Alternatively, use 3.3V-native **HC-SR04P** or **RCWL-9610** modules.

### 3. MPU6050 Gyroscope / IMU
| MPU6050 Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Power supply |
| **GND** | **GND** | Common ground |
| **SDA** | **GPIO 21** | I2C Data (400 kHz Fast Mode) |
| **SCL** | **GPIO 22** | I2C Clock |

### 4. Raspberry Pi UART2 Link
| Raspberry Pi Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **GPIO 14 (TXD, Pin 8)** | **GPIO 16 (RX2)** | Receives commands from Pi |
| **GPIO 15 (RXD, Pin 10)**| **GPIO 17 (TX2)** | Sends telemetry/alerts to Pi |
| **GND (Pin 6 or 9)** | **GND** | Common ground (Critical!) |

---

## Software Features

### 1. Dual-Core FreeRTOS Architecture
- **Core 0 Task (`networkTask`)**:
  - Web Admin Server (REST API + Canvas UI).
  - ArduinoOTA listener (allows wireless firmware updates without plugging in USB).
- **Core 1 Task (`loop()`)**:
  - 100% non-blocking interrupt-based ultrasonic scanning.
  - High-rate MPU6050 yaw drift-compensated integration.
  - Evasion state machine & closed-loop gyro turns.
  - 2-Channel Relay digital motor switching.
  - Raspberry Pi UART packet engine.

### 2. Autonomous Evasion Logic
- **Adjustable Threshold Distance**: Default 25 cm (changeable live from Web or Pi).
- **Front Blocked**: Reverses until front distance clears the threshold. If rear is also blocked, computes best angle to spin.
- **Two Sides Blocked**: Calculates the clearance vector field around all 8 sensors to locate the optimal escape corridor, executes a precise gyro-stabilized tank turn, and advances forward.
- **Trapped (`ALL_SIDES_TRAPPED`)**: When boxed in from all directions, halts motors to prevent collisions and broadcasts `STATUS:ALL_SIDES_TRAPPED` to the Raspberry Pi and Web UI.

### 3. Raspberry Pi Communication Protocol
Baud rate: **115200**.

#### Outbound Telemetry (ESP32 -> Pi)
- Heartbeat & status string at 10 Hz:
  ```
  STATUS:CLEAR
  STATUS:OBJECT_IN_FRONT
  STATUS:OBJECT_BOTH_SIDES
  STATUS:ALL_SIDES_TRAPPED
  ```
- Detailed JSON telemetry:
  ```json
  {"status":"OBJECT_IN_FRONT","mode":"AUTO_EVADE","yaw":14.2,"spd":[180,180],"d":[18.5,45.0,120.0,300.0,300.0,300.0,80.0,22.1]}
  ```

#### Inbound Commands (Pi -> ESP32)
| Command | Action |
| :--- | :--- |
| `CMD:ENABLE_EVADE` | Enables autonomous obstacle evasion mode |
| `CMD:DISABLE_EVADE`| Disables auto evade; Pi takes over manual motor control |
| `CMD:MOVE:F` | Move forward |
| `CMD:MOVE:B` | Move backward |
| `CMD:MOVE:L` | Spin left |
| `CMD:MOVE:R` | Spin right |
| `CMD:MOVE:STOP` | Stop motors |
| `CMD:TANK:<left>,<right>` | Direct left/right motor PWM (-255 to +255) |
| `CMD:SPEED:<0-255>` | Adjust default driving speed |

---

## Web Admin Portal (Dedicated Access Point)

Connect your phone, tablet, or laptop directly to the robot's standalone WiFi Access Point:
- **SSID**: `ESP32-EvadeBot-AP`
- **Password**: `admin12345`
- **Dashboard URL**: [http://192.168.4.1](http://192.168.4.1) (Standard ESP32 SoftAP Gateway)

- **High-Priority Emergency Stop (E-STOP)**: Prominent physical button to cut motor power and halt all motion immediately; locks controls until explicitly reset.
- **Control Mode Switcher**: Seamlessly toggle between `AUTO EVADE`, `MANUAL DRIVE`, and `PI LINK`.
- **Tactile Tank D-Pad**: Drive with zero lag using touch on phones/tablets, mouse clicks, or keyboard keys (`W, A, S, D, Space`).
- **Dynamic Config Sliders**: Rapidly tune motor drive speed (80-255) and evasion distance threshold (10-60 cm).
- **Pure Controls Layout**: Zero monitoring clutter (no radar/compass overhead), ensuring blazing-fast load times and immediate touch response.

---

## Setup & Upload

### Option A: Using PlatformIO (VS Code)
1. Clone or copy this repository into VS Code.
2. Configure your WiFi credentials in [src/config.h](file:///e:/TinkerHub/src/config.h).
3. Connect ESP32 via USB and click **Upload** or run:
   ```bash
   pio run -t upload
   ```

### Option B: Using Arduino IDE
1. Open [TinkerHub.ino](file:///e:/TinkerHub/TinkerHub.ino).
2. Install library `ArduinoJson` (v6.x) via Library Manager.
3. Select board **ESP32 Dev Module** or **DOIT ESP32 DEVKIT V1**.
4. Set CPU Frequency to **240MHz**.
5. Click **Upload**.

### Option C: Wireless Flashing (ArduinoOTA)
Once initially flashed, the ESP32 registers as `esp32-evade-bot` on your local network. You can select its network port in Arduino IDE or PlatformIO and upload code wirelessly without USB!
Password: `admin`.

---

## PC USB Serial Visualizer UI (Testing & Diagnostics)

To test, monitor, and visualize the robot's real-time actions and ultrasonic radar from your PC over the USB cable (with **zero changes** required to the ESP32 code):

1. Plug the ESP32 into your PC via USB.
2. Run the visualizer application:
   ```bash
   python tools/visualizer_ui.py
   ```
3. Select your COM port (e.g. `COM13`) and click **CONNECT**.
4. The dashboard displays:
   - **8-Direction Sonar Radar**: Real-time visual sonar beams with green/yellow/red proximity detection and distance tags.
   - **Robot Heading & Gyro Compass**: Robot chassis rotates in real time with the MPU6050 yaw.
   - **Real-Time Action Taken Banner**: Instant classification of the bot's maneuvers (e.g. `▲ MOVING FORWARD`, `▼ REVERSING AWAY FROM FRONT OBSTACLE`, `◀ SPINNING LEFT`, `🛑 EMERGENCY STOPPED`, `⚠️ TRAPPED`).
   - **Motor PWM Gauges**: Dual progress bars showing Left and Right track speeds (-255 to +255).
   - **Action & Status Event Log**: Timestamped record of every obstacle trigger and state change.
   - **Raw USB Console**: Real-time stream of raw lines emitted by the ESP32.
   - **Simulation Mode**: Built-in simulator toggle to preview the visualizer offline without hardware.
