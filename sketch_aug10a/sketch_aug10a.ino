#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <time.h>
#include <MHZ19.h>
#include <SoftwareSerial.h>
#include <SD_MMC.h>

#define esp_id       "abcd1234-abcd-1234-abcd-1234567890ab"
#define wifi_id      "abcd1234-abcd-1234-abcd-1234567890ac"
#define pass_id      "abcd1234-abcd-1234-abcd-1234567890ad"
#define pini 19
#define pino 21
#define pino2 20
#define DHTPIN 14
#define DHTTYPE DHT22
#define RX_PIN 10
#define TX_PIN 11
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial mySerial(RX_PIN, TX_PIN);
MHZ19 myMHZ19;

bool connected = false;
String ssid;
String password;
bool ssid_ = false;
bool password_ = false;
Preferences prefs;
BLEAdvertising *pAdvertising;

class SSIDCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    ssid = pCharacteristic->getValue().c_str();
    ssid_ = true;
    Serial.println("ssid: " + ssid);
  }
};

class PASSCallback: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    password = pCharacteristic->getValue().c_str();
    password_ = true;
    Serial.println("password: " + password);
  }
};
void v1(){
  while (1) {
    if (digitalRead(pini) == HIGH) {
      delay(100);
      while (digitalRead(pini) == HIGH);
      delay(100);
      pAdvertising->stop();
      digitalWrite(pino, LOW);
      Serial.println("advertising stopped");
      sleep();
    }
    if (ssid_ && password_) {
      prefs.begin("wifi", false);
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.end();
      ssid_ = password_ = false;
      v2();
      break;
    }
  }
}
void sleep(){
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  Serial.println("Going to sleep... press button to wake.");
  esp_sleep_enable_ext0_wakeup((gpio_num_t)pini, 1);
  esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
  esp_deep_sleep_start();
}
void connectToWiFi() {
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  prefs.end();
  Serial.println("connecting...");
  Serial.println(ssid);
  Serial.println(password);
  WiFi.begin(ssid, password);
  int attempts {0};
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected :)");
    connected=true;
  } else {
    connected=false;
    Serial.println("didnt connect :(");
    Serial.println("check wifi, password or ssid");
  }
}
struct SData {
  float t;
  float h;
  float ppm;
  uint64_t time;
};
void v2(){
  connectToWiFi();

  SData data;
  uint64_t now;
  data.ppm = myMHZ19.getCO2();
  data.t = myMHZ19.getTemperature();
  data.h = dht.readHumidity();
  unsigned long startMillis = millis();

  if (connected) {  
    configTime(5 * 3600, 0, "pool.ntp.org");
    time_t seconds;
    time(&seconds);
    now = (uint64_t)seconds * 1000;
  }
  else{
    prefs.begin("time", true);
    uint64_t lastTime = prefs.getULong64("savedtime", 0);
    prefs.end();
    
    // If never synced, use millis() as relative time
    if (lastTime == 0) {
      now = millis();
    } else {
      // Add 10 seconds (sleep interval) to last saved time
      now = lastTime + 10000;
    }
  }
  data.time=now;
  
  Serial.println("\n=== CURRENT READING ===");
  Serial.print("Temp: "); Serial.print(data.t);
  Serial.print("°C  Hum: "); Serial.print(data.h);
  Serial.print("%  CO2: "); Serial.print(data.ppm);
  Serial.println(" ppm");
  Serial.print("Timestamp: "); Serial.println(now);

  File fileW = SD_MMC.open("/array.bin", FILE_APPEND);
  fileW.write((uint8_t*)&data, sizeof(data));
  fileW.close();

  if (connected) {
    Serial.println("\n=== UPLOADING SAVED DATA ===");
    File fileR = SD_MMC.open("/array.bin", FILE_READ);
    File tempFile = SD_MMC.open("/temp.bin", FILE_WRITE);
    bool allSent = true;
    int recordCount = 0;
    
    while (fileR.read((uint8_t*)&data, sizeof(data)) == sizeof(data)) {
      recordCount++;
      
      // Skip records with invalid timestamps (less than year 2020)
      if (data.time < 1577836800000) {  // Jan 1, 2020 in milliseconds
        Serial.print("Skipping record #"); Serial.print(recordCount);
        Serial.println(" (invalid timestamp)");
        continue;  // Don't send, don't save to temp
      }
      
      if (allSent) {
        Serial.print("Record #"); Serial.print(recordCount); Serial.print(": ");
        HTTPClient https;
        https.begin("https://co2.arsen.ganibek.com/ingest");
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String payload = "&t=" + String(data.t) + "&h=" + String(data.h) + "&ppm=" + String(data.ppm) + "&time=" + String(data.time);
        int httpCode = https.POST(payload);
        Serial.print("Temp: "); Serial.print(data.t);
        Serial.print("°C  Hum: "); Serial.print(data.h);
        Serial.print("% CO2: "); Serial.print(data.ppm);Serial.println(" ppm");
        Serial.println(data.time);
        Serial.println(httpCode);
        https.end();
        
        if(httpCode != 200){
          allSent = false;
          tempFile.write((uint8_t*)&data, sizeof(data));
        }
      } else {
        tempFile.write((uint8_t*)&data, sizeof(data));
      }
    }
    fileR.close();
    tempFile.close();
    SD_MMC.remove("/array.bin");
    SD_MMC.rename("/temp.bin", "/array.bin");
  }
  
  // Save current timestamp for next wake
  prefs.begin("time", false);
  prefs.putULong64("savedtime", now);
  prefs.end();
  delay(500);
  sleep();
}
void blesetup(){
  BLEDevice::init("ESP32 MyWiFiSetup");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(esp_id);
  BLECharacteristic *pSSIDChar = pService->createCharacteristic(
    wifi_id, BLECharacteristic::PROPERTY_WRITE
  );
  pSSIDChar->setCallbacks(new SSIDCallback());
  BLECharacteristic *pPASSChar = pService->createCharacteristic(
    pass_id, BLECharacteristic::PROPERTY_WRITE
  );
  pPASSChar->setCallbacks(new PASSCallback());
  pAdvertising = BLEDevice::getAdvertising();
  pService->start();
  pAdvertising->addServiceUUID(esp_id);
  pAdvertising->start();
  Serial.println("advertising started");
}
void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  myMHZ19.begin(mySerial);
  myMHZ19.autoCalibration(true);
  pinMode(pini, INPUT_PULLDOWN);
  pinMode(pino, OUTPUT);
  pinMode(pino2, OUTPUT);
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5);
  digitalWrite(pino, LOW);
  digitalWrite(pino2, LOW);
  dht.begin();
  delay(1000);
  
  // Clear SD card data if button held during boot
  if (digitalRead(pini) == HIGH) {
    Serial.println("Button pressed - clearing SD card data...");
    SD_MMC.remove("/array.bin");
    SD_MMC.remove("/temp.bin");
    Serial.println("SD card cleared!");
    delay(2000);
  }
  
  digitalWrite(pino, HIGH);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    blesetup();
    v1();
  }
  else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    v2();
  }
  else sleep();
}
void loop() {}