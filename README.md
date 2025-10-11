# Air Quality Monitoring System

A DIY IoT project for classroom environmental monitoring using ESP32-S3 microcontroller with multiple sensors.

## Overview

This system monitors classroom air quality and environmental conditions, transmitting sensor data to AWS cloud services via WiFi. The device features BLE provisioning for easy setup and power-efficient operation.

## Hardware Components

- **ESP32-S3** - Main microcontroller with WiFi and BLE
- **2x MH-Z19C** - Infrared CO2 sensors (400-5000 ppm)
- **2x DHT22** - Temperature and humidity sensors
- **SD Card Module** - Local data storage (1GB)
- **18650 Battery** - Portable power supply
- **Status LED** - Visual feedback for device state
- **BLE Button** - Initiate WiFi provisioning

## Features

### Core Functionality
- Real-time environmental monitoring (CO2, temperature, humidity)
- WiFi connectivity with BLE provisioning
- Local data caching on SD card
- Power-efficient deep sleep mode
- AWS cloud integration

### Success Criteria
1. **Power Management** - Deep sleep mode with optimized transmission intervals
2. **Intuitive Dashboard** - Real-time data visualization
3. **Simple Setup** - BLE-based WiFi provisioning
4. **Compact Design** - Durable prototype form factor

## System Architecture

```
ESP32-S3 → Sensors → Local Storage → WiFi → AWS API Gateway → DynamoDB
                                                    ↓
                                            S3 Web Dashboard
```

## Current Status

### ✅ Completed
- BLE provisioning implementation (Python script)
- DHT22 sensor integration
- WiFi credential storage in memory
- Basic breadboard prototype

### 🔄 In Progress
- Mobile BLE provisioning (replacing Python script)
- MH-Z19C sensor integration and calibration
- Code organization and modularization
- SD card data persistence
- Battery integration and power calculations

### 📋 Planned
- PCB/enclosure design
- AWS backend deployment (API Gateway + DynamoDB + Lambda)
- Web dashboard (S3 static hosting)
- Infrastructure as Code (AWS SAM/CloudFormation)

## Technical Specifications

- **Power**: 18650 Li-ion battery with deep sleep optimization
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth 5.0 LE
- **Data Transmission**: HTTP/HTTPS to AWS API Gateway
- **Storage**: Local SD card caching for offline resilience
- **Sensors**: Direct GPIO connection, no additional protocols required

## Project Structure

```
├── README.md
├── PROJECT_PLAN.md      # Detailed implementation timeline
├── TECH_SPECS.md        # Technical specifications and diagrams
├── src/                 # ESP32 firmware source code
├── aws/                 # AWS infrastructure templates
└── web/                 # Dashboard frontend
```

## Getting Started

1. Review [PROJECT_PLAN.md](PROJECT_PLAN.md) for implementation timeline
2. Check [TECH_SPECS.md](TECH_SPECS.md) for detailed technical information
3. Follow hardware assembly instructions (coming soon)
4. Deploy AWS infrastructure using provided templates

## Educational Goals

This project demonstrates key IoT concepts:
- Sensor integration and data collection
- Wireless communication protocols (WiFi, BLE)
- Cloud services and data persistence
- Power management for battery-operated devices
- Real-time data visualization

---

*This is a school project focused on learning IoT development principles and AWS cloud services.*
