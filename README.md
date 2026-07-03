# SensoKit MainBoard

![Arduino](https://img.shields.io/badge/Arduino-MKR%20NB%201500-00979D?logo=arduino&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Arduino-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Status](https://img.shields.io/badge/Status-Active-success)

A portable environmental sensing platform based on the **Arduino MKR NB
1500** for collecting air quality and environmental data together with
GPS coordinates and transmitting the measurements via **NB-IoT/LTE-M**.

Designed for mobile environmental monitoring, research projects, and
smart city applications.

------------------------------------------------------------------------

## Table of Contents

-   [Features](#features)
-   [Hardware](#hardware)
-   [Sensors](#sensors)
-   [Software Requirements](#software-requirements)
-   [Installation](#installation)
-   [Project Structure](#project-structure)
-   [Power Management](#power-management)
-   [Status LED](#status-led)
-   [Data Format](#data-format)
-   [Configuration](#configuration)
-   [Building](#building)
-   [Future Improvements](#future-improvements)
-   [License](#license)
-   [Acknowledgements](#acknowledgements)

------------------------------------------------------------------------

# Features

-   GPS position tracking
-   Automatic sensor detection
-   NB-IoT/LTE-M data transmission
-   Battery monitoring
-   Deep sleep mode
-   NeoPixel status indication
-   Automatic modem shutdown
-   Automatic GPS standby mode
-   GeoJSON data generation
-   Designed for low-power battery operation

------------------------------------------------------------------------

# Hardware

## Controller

-   Arduino MKR NB 1500

## Communication

-   Integrated NB-IoT / LTE-M modem
-   GPS receiver (SparkFun I²C GPS / PA1010D compatible)

## Additional Components

-   Adafruit TCA9548A I²C Multiplexer
-   Adafruit NeoPixel
-   Illuminated push button
-   LiPo Battery

------------------------------------------------------------------------

# Sensors

  Sensor                              Measurement
  ----------------------------------- ----------------------------
  Sensirion SCD41                     CO₂, Temperature, Humidity
  ScioSense ENS160                    AQI, TVOC
  DFRobot SEN0321                     Ozone
  PCB Artists I²C Sound Level Meter   Noise Level

------------------------------------------------------------------------

# Software Requirements

Install the following Arduino libraries:

-   MKRNB
-   TinyGPS++
-   Adafruit GPS
-   SparkFun I²C GPS Arduino Library
-   Wire
-   ArduinoLowPower
-   Adafruit NeoPixel
-   DFRobot_OzoneSensor
-   ScioSense ENS160
-   Sensirion I²C SCD4x

Most libraries are available through the Arduino Library Manager.

------------------------------------------------------------------------

# Installation

``` bash
git clone https://github.com/yourusername/sensokit.git
```

Open `sensokit_code.ino`, install the required libraries, select
**Arduino MKR NB 1500**, and upload the sketch.

------------------------------------------------------------------------

# Project Structure

``` text
.
├── sensokit_code.ino
├── README.md
└── LICENSE
```

------------------------------------------------------------------------

# Power Management

-   Automatic modem shutdown
-   GPS standby mode
-   Deep sleep mode
-   Push-button wake-up
-   Battery voltage monitoring
-   Automatic shutdown at critical battery voltage

------------------------------------------------------------------------

# Status LED

  Color            Status
  ---------------- ---------------
  🟡 Yellow        Startup
  🟢 Green         Running
  🟢 Light Green   Measuring
  🔵 Blue          GPS Error
  🟣 Magenta       Network Error
  🔴 Red           Low Battery
  ⚫ Off           Sleep Mode

------------------------------------------------------------------------

# License

MIT License unless stated otherwise.
