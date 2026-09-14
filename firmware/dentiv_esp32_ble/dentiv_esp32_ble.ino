/*
  DENTIV - Açı Ölçüm Sistemi - ESP32 Firmware (BLE)
  ---------------------------------------------------------------------------
  ESP32, BLE (Bluetooth Low Energy) üzerinden pitch/roll/batarya verisini
  bir GATT characteristic notification olarak yayınlar. Bu firmware,
  mobile-app/ klasöründeki Capacitor tabanlı iOS uygulamasıyla birlikte
  kullanılmak üzere tasarlanmıştır (@capacitor-community/bluetooth-le eklentisi
  üzerinden CoreBluetooth'a erişir, iOS Safari'nin desteklemediği Web
  Bluetooth'a ihtiyaç duymaz).

  Gerekli kütüphaneler: Yok — ESP32 Arduino çekirdeğiyle birlikte gelen
  BLEDevice kütüphanesi kullanılıyor. Ayrıca:
    - Adafruit MPU6050
    - Adafruit Unified Sensor

  Donanım:
    - MPU6050 IMU: SDA -> GPIO21, SCL -> GPIO22 (ESP32 varsayılan I2C pinleri)
    - Sıfırlama butonu: GPIO27 -> GND (INPUT_PULLUP, aktif LOW)
    - Batarya gerilim bölücü: GPIO34 (ADC1, 2x100k direnç ile 1/2 bölücü varsayılmıştır)

  Not: SERVICE_UUID ve CHAR_UUID, mobile-app/www/index.html içindeki değerlerle
  birebir aynı olmalı.
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ================== YAPILANDIRMA ==================
static const char *DEVICE_NAME  = "DENTIV";
static const char *SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
static const char *CHAR_UUID    = "abcd1234-5678-90ab-cdef-1234567890ab";

static const int ZERO_BUTTON_PIN = 27;
static const int BATTERY_PIN     = 34; // ADC1 pini olmalı (GPIO34-39)

static const float BATTERY_DIVIDER_RATIO = 2.0f;  // Gerilim bölücü oranınıza göre ayarlayın
static const float ADC_REF_VOLTAGE       = 3.3f;
static const float BATTERY_MIN_VOLTAGE   = 3.3f;  // %0 kabul edilen gerilim (LiPo boşalmış)
static const float BATTERY_MAX_VOLTAGE   = 4.2f;  // %100 kabul edilen gerilim (LiPo dolu)

static const unsigned long BROADCAST_INTERVAL_MS = 50; // ~20Hz
static const unsigned long DEBOUNCE_MS = 50;

// ================== GLOBAL NESNELER ==================
Adafruit_MPU6050 mpu;

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

bool lastButtonReading = HIGH;
bool lastStableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
bool zeroRequested = false;

unsigned long lastBroadcast = 0;

// ================== BLE CALLBACK'LERİ ==================
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *server) override {
        deviceConnected = true;
        Serial.println("[BLE] İstemci bağlandı");
    }

    void onDisconnect(BLEServer *server) override {
        deviceConnected = false;
        Serial.println("[BLE] İstemci ayrıldı, yeniden yayına başlanıyor");
        BLEDevice::startAdvertising(); // ESP32'de disconnect sonrası advertising otomatik devam etmez
    }
};

// ================== YARDIMCI FONKSİYONLAR ==================
int readBatteryPercent() {
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0f) * ADC_REF_VOLTAGE * BATTERY_DIVIDER_RATIO;
    float pct = (voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0f;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (int)pct;
}

void checkZeroButton() {
    bool reading = digitalRead(ZERO_BUTTON_PIN);

    if (reading != lastButtonReading) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
        if (reading == LOW && lastStableButtonState == HIGH) {
            zeroRequested = true; // basma anının düşen kenarı
        }
        lastStableButtonState = reading;
    }

    lastButtonReading = reading;
}

void broadcastData() {
    if (!deviceConnected) return;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * RAD_TO_DEG;
    float roll  = atan2(a.acceleration.y, a.acceleration.z) * RAD_TO_DEG;

    int batteryPct = readBatteryPercent();

    char buf[48];
    if (zeroRequested) {
        snprintf(buf, sizeof(buf), "%.2f,%.2f,%d,ZERO", pitch, roll, batteryPct);
        zeroRequested = false;
    } else {
        snprintf(buf, sizeof(buf), "%.2f,%.2f,%d", pitch, roll, batteryPct);
    }

    pCharacteristic->setValue((uint8_t *)buf, strlen(buf));
    pCharacteristic->notify();
}

// ================== SETUP / LOOP ==================
void setup() {
    Serial.begin(115200);

    pinMode(ZERO_BUTTON_PIN, INPUT_PULLUP);
    analogReadResolution(12);

    Wire.begin();
    if (!mpu.begin()) {
        Serial.println("HATA: MPU6050 bulunamadı, kabloları kontrol edin.");
        while (true) delay(1000);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.print("BLE yayını başladı, cihaz adı: ");
    Serial.println(DEVICE_NAME);
}

void loop() {
    checkZeroButton();

    if (millis() - lastBroadcast >= BROADCAST_INTERVAL_MS) {
        lastBroadcast = millis();
        broadcastData();
    }
}
