# SensoKit MainBoard

![Arduino](https://img.shields.io/badge/Arduino-MKR%20NB%201500-00979D?logo=arduino&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Arduino-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Status](https://img.shields.io/badge/Status-Active-success)

A portable environmental sensing platform based on the **Arduino MKR NB 1500** for collecting air quality and environmental data together with GPS coordinates and transmitting the measurements via **NB-IoT/LTE-M**.

Designed for mobile environmental monitoring, research projects, and smart city applications.

---

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [Sensors](#sensors)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Project Structure](#project-structure)
- [System Overview](#system-overview)
- [Power Management](#power-management)
- [Status LED](#status-led)
- [Data Format](#data-format)
- [Configuration](#configuration)
- [Building](#building)
- [License](#license)

---

## Features

- GPS position tracking
- Automatic sensor detection
- NB-IoT/LTE-M data transmission
- Battery monitoring
- Deep sleep mode
- NeoPixel status indication
- Automatic modem shutdown
- Automatic GPS standby mode
- GeoJSON data generation
- Designed for low-power battery operation

---

## Hardware

### Controller

- Arduino MKR NB 1500

### Communication

- Integrated NB-IoT / LTE-M modem
- GPS receiver, SparkFun I²C GPS / PA1010D compatible

### Additional Components

- Adafruit TCA9548A I²C Multiplexer
- Adafruit NeoPixel
- Illuminated push button
- LiPo battery

---

## Sensors

| Sensor | Measurement |
|---|---|
| Sensirion SCD41 | CO₂, Temperature, Humidity |
| ScioSense ENS160 | AQI, TVOC |
| DFRobot SEN0321 | Ozone |
| PCB Artists I²C Sound Level Meter | Noise Level |

---

## Software Requirements

Install the following Arduino libraries:

- MKRNB
- TinyGPS++
- Adafruit GPS
- SparkFun I²C GPS Arduino Library
- Wire
- ArduinoLowPower
- Adafruit NeoPixel
- DFRobot_OzoneSensor
- ScioSense ENS160
- Sensirion I²C SCD4x

Most libraries are available through the Arduino Library Manager.

---

## Installation

Clone this repository:

```bash
git clone https://github.com/yourusername/sensokit.git
```

Open the Arduino sketch in the Arduino IDE:

```text
sensokit_code.ino
```

Then:

1. Install all required libraries.
2. Connect the Arduino MKR NB 1500.
3. Select the correct board and serial port.
4. Compile and upload the sketch.

Recommended Arduino IDE settings:

- **Board:** Arduino MKR NB 1500
- **Port:** Your Arduino USB/COM port

---

## Project Structure

```text
.
├── sensokit_code.ino
├── README.md
└── LICENSE
```

---

## System Overview

```text
                 Arduino MKR NB 1500
                         │
             ┌───────────┴───────────┐
             │                       │
         NB-IoT Modem           NeoPixel LED
             │
       Cellular Network

             │
           I²C Bus
             │
      TCA9548A Multiplexer
             │
 ┌──────┬──────┬──────┬──────┐
 │      │      │      │
GPS   SCD41  ENS160 Ozone
              │
      Sound Level Meter
```

The firmware uses the TCA9548A I²C multiplexer to select sensor channels and detect connected devices. Sensor values are collected periodically, combined with GPS coordinates, formatted as GeoJSON, and sent to the configured DATAhub endpoint.

---

## Power Management

The firmware is designed for battery-powered operation.

Power-saving features include:

- Automatic modem shutdown after transmission
- GPS standby mode
- Deep sleep mode
- Wake-up using the push button
- Battery voltage monitoring
- Low battery indication
- Automatic shutdown at critical battery voltage

---

## Status LED

The NeoPixel LED indicates the current device state.

| Color | Status |
|---|---|
| 🟡 Yellow | Startup |
| 🟢 Green | Running |
| 🟢 Light Green | Measuring |
| 🔵 Blue | GPS Error |
| 🟣 Magenta | Network Error |
| 🔴 Red | Low Battery |
| ⚫ Off | Sleep Mode |

---

## Data Format

Sensor data are transmitted as GeoJSON.

Example:

```json
{
  "type": "Feature",
  "geometry": {
    "type": "Point",
    "coordinates": [
      16.391135,
      48.142748
    ]
  },
  "properties": {
    "Noise": [58.4, "dB"],
    "CO2": [540, "ppm"],
    "Temperature": [22.4, "°C"],
    "Humidity": [45.2, "%"],
    "AQI": [2, "AQI"],
    "TVOC": [132, "ppb"],
    "Ozone": [24, "ppb"]
  }
}
```

The firmware also sends status information, including battery level, battery voltage, GPS fix state, and online status.

---

## Configuration

Before uploading the firmware, configure the following values in the sketch:

- DATAhub server URL
- API endpoints
- Authentication/source token
- APN or cellular network settings, if required
- Sampling interval
- Data upload interval
- Sensor configuration
- Battery thresholds

Important configuration values include:

```cpp
char server[] = "your-server.example.com";
char pathdata[] = "/api/sensordata/";
char pathstatus[] = "/api/sensorstatus/";
int port = 80;
```

Replace these values with the correct endpoint and token information for your deployment.

---

## Building

Compile and upload the sketch using the Arduino IDE.

The firmware has been tested with:

- Arduino IDE 2.x
- Arduino MKR NB 1500

Basic build steps:

1. Open the `.ino` file.
2. Install all dependencies.
3. Select **Arduino MKR NB 1500** as the board.
4. Select the correct serial port.
5. Click **Verify**.
6. Click **Upload**.

---

## License

This project is released under the **MIT License** unless stated otherwise.

Update this section if your repository uses another license.

