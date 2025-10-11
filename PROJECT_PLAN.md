# Air Sensor IoT Project Plan

## Compressed Timeline: 3 Weeks

### Week 1: Hardware & Core Firmware
**Days 1-2: Hardware Setup**
- Connect MH-Z19C sensors (test cable distance)
- Integrate DHT22 sensors
- Basic power supply setup

**Days 3-5: Core Firmware**
- Sensor data collection
- WiFi connection management
- Basic data transmission to AWS

**Days 6-7: Power Management**
- Implement deep sleep
- Battery integration and testing

### Week 2: Backend & Data Flow
**Days 1-3: AWS Backend**
- API Gateway + Lambda + DynamoDB setup
- Test data ingestion
- Basic authentication

**Days 4-5: Data Storage**
- SD card caching implementation
- Offline data sync logic

**Days 6-7: BLE Provisioning**
- Simplify WiFi setup process
- LED status indicators

### Week 3: Frontend & Integration
**Days 1-3: Web Dashboard**
- S3 static site with basic charts
- Real-time data display

**Days 4-5: System Integration**
- End-to-end testing
- Bug fixes and optimization

**Days 6-7: Documentation & Presentation**
- Technical documentation
- Project demonstration prep

## Success Criteria (Simplified)
1. **Working sensors** - CO2 and temperature/humidity readings
2. **Data transmission** - Sensors → AWS → Dashboard
3. **Power efficiency** - Deep sleep working, battery life > 24h
4. **Simple setup** - BLE provisioning functional

## Key Deliverables
- [ ] Working ESP32 prototype
- [ ] AWS backend receiving data
- [ ] Web dashboard showing sensor data
- [ ] Basic project documentation
- [ ] Working demonstration

## Risk Mitigation
- **Week 1 delays**: Skip second CO2 sensor if cable extension fails
- **Week 2 delays**: Use HTTP instead of MQTT, skip advanced auth
- **Week 3 delays**: Basic HTML dashboard instead of fancy UI

## Resource Requirements
- ESP32-S3 development board
- 2x MH-Z19C CO2 sensors
- 2x DHT22 temperature/humidity sensors
- SD card module + 1GB card
- 18650 battery + holder
- Basic enclosure materials
- AWS free tier account
