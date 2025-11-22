# CO2 Sensor AWS Backend Implementation Plan

## Project Overview
School IoT project: ESP32-S3 device with MH-Z19C (CO2/temp) and DHT-22 (humidity) sensors sending telemetry to AWS backend with web dashboard.

**Goals:**
- Low cost (~$1/month)
- Simple architecture
- Public dashboard via custom domain
- Reliable data collection

---

## Current Data Format

### ESP32 Sends (HTTP POST)
**Endpoint:** `http://192.168.1.68/arsens_site/save.php`
**Method:** POST
**Content-Type:** `application/x-www-form-urlencoded`

**Payload Structure:**
```
&t=<temperature>&h=<humidity>&ppm=<co2>&time=<timestamp>
```

**Example:**
```
&t=23.5&h=65.2&ppm=450&time=1730491200000
```

**Data Types (from sketch):**
```cpp
struct SData {
  float t;      // Temperature (°C) from MH-Z19C
  float h;      // Humidity (%) from DHT-22
  float ppm;    // CO2 (ppm) from MH-Z19C
  uint64_t time; // Unix timestamp in milliseconds
};
```

**Sample Values:**
- Temperature: 15-35°C (typical indoor range)
- Humidity: 20-80%
- CO2: 400-2000 ppm (400=outdoor, 1000+=poor ventilation)
- Timestamp: Unix epoch ms (e.g., 1730491200000 = Nov 1, 2024)

**Frequency:** Every 10 seconds (8,640 readings/day)

**Time Management:**
- **Online (WiFi connected):** Syncs with NTP server (pool.ntp.org) for accurate timestamps
- **Offline (never synced):** Uses millis() for relative timestamps (10000, 20000, 30000...)
- **Offline (previously synced):** Continues from last known time + 10s intervals
- **Accuracy:** ±1 second when online, ±10 seconds per cycle when offline
- **Storage:** Timestamp saved in flash memory (Preferences) between deep sleep cycles

---

## AWS Architecture (Option 3: S3 Static Site)

```
┌─────────┐
│ ESP32   │
└────┬────┘
     │ POST /ingest
     ▼
┌─────────────────┐
│ API Gateway     │ (HTTP API)
│ /ingest         │
└────┬────────────┘
     │
     ▼
┌─────────────────┐
│ Lambda (Ingest) │ (Node.js 20)
└────┬────────────┘
     │
     ▼
┌─────────────────┐
│ DynamoDB        │
│ Table: co2-data │
│ PK: deviceId    │
│ SK: timestamp   │
└─────────────────┘
     ▲
     │
┌────┴────────────┐
│ Lambda (Query)  │ (Node.js 20)
└────┬────────────┘
     │
┌────┴────────────┐
│ API Gateway     │
│ /data?range=24h │
└─────────────────┘
     ▲
     │
┌────┴────────────┐
│ CloudFront      │
└────┬────────────┘
     │
┌────┴────────────┐
│ S3 Static Site  │
│ index.html      │
│ + Chart.js      │
└─────────────────┘
     ▲
     │
┌────┴────────────┐
│ Route53         │
│ sensor.domain   │
└─────────────────┘
```

---

## Implementation Steps

### Phase 1: Backend Setup (SAM)
1. Create SAM project structure
2. Define DynamoDB table
3. Create ingest Lambda function
4. Create query Lambda function
5. Configure API Gateway HTTP API
6. Deploy stack

### Phase 2: ESP32 Integration
1. Update endpoint URL in sketch
2. Test data ingestion
3. Verify DynamoDB records

### Phase 3: Dashboard
1. Create HTML page with Chart.js
2. Implement data fetching from query API
3. Add time range selector (24h/7d/30d)
4. Upload to S3
5. Configure S3 static website hosting

### Phase 4: Domain & CDN
1. Create CloudFront distribution
2. Configure Route53 DNS
3. Add SSL certificate (ACM)

---

## DynamoDB Schema

**Table Name:** `co2-sensor-data`

**Primary Key:**
- Partition Key: `deviceId` (String) - e.g., "esp32-001"
- Sort Key: `timestamp` (Number) - Unix epoch milliseconds

**Attributes:**
```json
{
  "deviceId": "esp32-001",
  "timestamp": 1730491200000,
  "temperature": 23.5,
  "humidity": 65.2,
  "co2": 450,
  "ttl": 1762027200
}
```

**TTL:** 365 days (auto-delete old data to save costs)

**Indexes:** None needed (query by deviceId + timestamp range)

---

## Time Management Logic

### Overview
The ESP32 wakes every 10 seconds, reads sensors, and assigns timestamps. Time management handles three scenarios:

### Scenario 1: Online (WiFi Connected)
```cpp
configTime(5 * 3600, 0, "pool.ntp.org");  // UTC+5 timezone
time_t seconds;
time(&seconds);
now = (uint64_t)seconds * 1000;  // Convert to milliseconds
```
- Syncs with NTP server
- Gets accurate Unix timestamp
- Example: 1730491200000 (Nov 1, 2024, 21:00:00 UTC+5)

### Scenario 2: Offline (Never Synced)
```cpp
if (lastTime == 0) {
  now = millis();  // Milliseconds since boot
}
```
- Uses relative time from device boot
- Timestamps: 500, 10500, 20500... (increments by ~10s)
- Useful for relative time analysis

### Scenario 3: Offline (Previously Synced)
```cpp
if (lastTime > 0) {
  now = lastTime + 10000;  // Add 10 seconds
}
```
- Continues from last known accurate time
- Maintains chronological order
- Accuracy: ±10 seconds per wake cycle

### Timestamp Persistence
```cpp
prefs.begin("time", false);
prefs.putULong64("savedtime", now);  // Save to flash memory
prefs.end();
```
- Stored in ESP32 flash memory (survives deep sleep)
- Retrieved on next wake to calculate new timestamp
- Updated every cycle (online or offline)

### Example Timeline

| Wake | WiFi | lastTime | Calculation | Result | Description |
|------|------|----------|-------------|--------|-------------|
| 1 | ❌ | 0 | millis() | 500 | Never synced, relative time |
| 2 | ❌ | 500 | 500 + 10000 | 10500 | Continue relative |
| 3 | ✅ | 10500 | NTP | 1730491200000 | Synced! |
| 4 | ❌ | 1730491200000 | 1730491200000 + 10000 | 1730491210000 | Continue from accurate |
| 5 | ❌ | 1730491210000 | 1730491210000 + 10000 | 1730491220000 | Still accurate ±10s |

---

## Data Upload Optimization

### Problem
Original code re-sent ALL data every wake cycle, even successfully uploaded records.

### Solution
```cpp
File tempFile = SD_MMC.open("/temp.bin", FILE_WRITE);
bool allSent = true;

while (fileR.read((uint8_t*)&data, sizeof(data)) == sizeof(data)) {
  if (allSent) {
    int httpCode = https.POST(payload);
    if(httpCode != 200){
      allSent = false;
      tempFile.write((uint8_t*)&data, sizeof(data));  // Keep failed record
    }
    // Success: record deleted (not written to temp)
  } else {
    tempFile.write((uint8_t*)&data, sizeof(data));  // Keep remaining
  }
}

SD_MMC.remove("/array.bin");
SD_MMC.rename("/temp.bin", "/array.bin");  // Replace with unsent only
```

### Benefits
- Successfully sent records are deleted
- File shrinks progressively
- Saves battery (no redundant uploads)
- Saves bandwidth
- Stops sending after first failure (efficient)

---

## API Endpoints

### 1. Ingest API (ESP32 → AWS)
**URL:** `https://<api-id>.execute-api.<region>.amazonaws.com/ingest`
**Method:** POST
**Body:** `&t=23.5&h=65.2&ppm=450&time=1730491200000`
**Response:** `200 OK` or `500 Error`

### 2. Query API (Dashboard → AWS)
**URL:** `https://<api-id>.execute-api.<region>.amazonaws.com/data`
**Method:** GET
**Query Params:**
- `range`: `24h` | `7d` | `30d` (default: 24h)
- `deviceId`: `esp32-001` (optional, default: esp32-001)

**Response:**
```json
{
  "data": [
    {
      "timestamp": 1730491200000,
      "temperature": 23.5,
      "humidity": 65.2,
      "co2": 450
    }
  ]
}
```

---

## Cost Estimate

**Monthly (1 device, 10s interval):**
- API Gateway: $0.26 (259,200 requests)
- Lambda: $0 (free tier: 1M requests, 400K GB-seconds)
- DynamoDB: $0 (free tier: 25GB, 25 WCU, 25 RCU)
- S3: $0.02 (storage + transfer)
- CloudFront: $0 (free tier: 1TB transfer)
- Route53: $0.50 (hosted zone)

**Total: ~$0.80/month**

---

## Dashboard Features

**Graphs:**
1. Temperature over time (line chart)
2. Humidity over time (line chart)
3. CO2 over time (line chart, with warning zones)

**Time Ranges:**
- Last 24 hours (default)
- Last 7 days
- Last 30 days

**Display:**
- Current values (latest reading)
- Min/Max/Average for selected range
- Color-coded CO2 levels:
  - Green: <800 ppm (good)
  - Yellow: 800-1200 ppm (moderate)
  - Red: >1200 ppm (poor)

---

## Next Steps

1. ✅ Fix ESP32 sketch (data sending optimization)
2. ✅ Create SAM template
3. ✅ Implement Lambda functions
4. ✅ Create dashboard HTML
5. ✅ Deploy and test
6. ✅ Configure custom domain

## Deployment Complete! 🎉

**API Endpoints:**
- Ingest: https://co2.arsen.ganibek.com/ingest
- Query: https://co2.arsen.ganibek.com/data

**Dashboard:**
- URL: https://co2.arsen.ganibek.com/

**Resources:**
- DynamoDB Table: co2-sensor-data
- S3 Bucket: co2-dashboard-602401188625
- ACM Certificate: arn:aws:acm:eu-central-1:602401188625:certificate/6de7f81e-65c4-4364-8724-8f70ce3573ab
- Custom Domain: co2.arsen.ganibek.com (API Gateway + Lambda serving dashboard)

**ESP32 Sketch:**
- Updated with AWS endpoint
- Ready to upload to device

---

## Notes

- Device ID hardcoded as "esp32-001" (can extend for multiple devices later)
- No authentication on dashboard (add CloudFront signed URLs if needed)
- Data retention: 1 year via DynamoDB TTL
- Timezone: UTC+5 (configured in dashboard display)
