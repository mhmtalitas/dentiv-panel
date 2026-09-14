/*
  DENTIV - Açı Ölçüm Sistemi - ESP32 Firmware
  ---------------------------------------------------------------------------
  ESP32 kendi WiFi ağını açar (Access Point), index.html'i kendi üzerinden
  sunar ve pitch/roll/batarya verisini WebSocket (/ws) üzerinden 20Hz'de
  yayınlar. Böylece iOS/Android tarayıcılarında Web Bluetooth kısıtlaması
  olmadan, doğrudan http://192.168.4.1 adresinden kullanılabilir.

  Gerekli kütüphaneler (Arduino IDE > Kütüphane Yöneticisi):
    - ESPAsyncWebServer (me-no-dev veya mathieucarbou fork)
    - AsyncTCP
    - Adafruit MPU6050
    - Adafruit Unified Sensor

  Donanım:
    - MPU6050 IMU: SDA -> GPIO21, SCL -> GPIO22 (ESP32 varsayılan I2C pinleri)
    - Sıfırlama butonu: GPIO27 -> GND (INPUT_PULLUP, aktif LOW)
    - Batarya gerilim bölücü: GPIO34 (ADC1, 2x100k direnç ile 1/2 bölücü varsayılmıştır)

  index.html'de değişiklik yaptıysanız, bu dosyayı derlemeden önce şunu çalıştırın:
    python3 tools/generate_firmware_html.py
*/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "webpage.h"

// ================== YAPILANDIRMA ==================
static const char *AP_SSID_PREFIX = "DENTIV";
static const char *AP_PASSWORD    = "dentiv2024"; // En az 8 karakter olmalı. Dağıtımdan önce değiştirin.

static const int ZERO_BUTTON_PIN = 27;
static const int BATTERY_PIN     = 34; // ADC1 pini olmalı (GPIO34-39)

static const float BATTERY_DIVIDER_RATIO = 2.0f;  // Gerilim bölücü oranınıza göre ayarlayın
static const float ADC_REF_VOLTAGE       = 3.3f;
static const float BATTERY_MIN_VOLTAGE   = 3.3f;  // %0 kabul edilen gerilim (LiPo boşalmış)
static const float BATTERY_MAX_VOLTAGE   = 4.2f;  // %100 kabul edilen gerilim (LiPo dolu)

static const unsigned long BROADCAST_INTERVAL_MS = 50; // ~20Hz
static const unsigned long DEBOUNCE_MS = 50;

// ================== GLOBAL NESNELER ==================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Adafruit_MPU6050 mpu;

bool lastButtonReading = HIGH;
bool lastStableButtonState = HIGH;
unsigned long lastDebounceTime = 0;
bool zeroRequested = false;

unsigned long lastBroadcast = 0;

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
            zeroRequested = true; // basma anının yükselen/düşen kenarı
        }
        lastStableButtonState = reading;
    }

    lastButtonReading = reading;
}

void broadcastData() {
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

    ws.textAll(buf);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] İstemci bağlandı: #%u\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] İstemci ayrıldı: #%u\n", client->id());
    }
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

    String apSsid = String(AP_SSID_PREFIX) + "-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFF), HEX);
    WiFi.softAP(apSsid.c_str(), AP_PASSWORD);
    Serial.print("WiFi ağı başlatıldı: ");
    Serial.println(apSsid);
    Serial.print("Panel adresi: http://");
    Serial.println(WiFi.softAPIP());

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Bulunamadı");
    });

    server.begin();
}

void loop() {
    checkZeroButton();

    if (millis() - lastBroadcast >= BROADCAST_INTERVAL_MS) {
        lastBroadcast = millis();
        broadcastData();
    }

    ws.cleanupClients();
}
