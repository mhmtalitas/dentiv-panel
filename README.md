# Dentiv - Açı Ölçüm Sistemi

ESP32 tabanlı hassas açı ölçüm sistemi. **Ana kullanım yolu artık native bir
iOS uygulaması + Bluetooth (BLE)** — bkz. [mobile-app/](mobile-app/).

> Repoda ayrıca WiFi/WebSocket tabanlı bir web paneli de duruyor (aşağıda
> "Alternatif: WiFi/WebSocket web paneli" bölümü). Günlük kullanım için
> **native app + BLE** öneriliyor; WiFi sürümü her kullanımda telefonun
> ayrı bir ağa bağlanmasını gerektirdiği için pratik değil.

---

## Kurulum Rehberi (Adım Adım) — Native App + BLE

### 1️⃣ Donanım Bağlantısı

ESP32'ye şunları bağla:

| Parça | ESP32 Pini |
|---|---|
| MPU6050 SDA | GPIO21 |
| MPU6050 SCL | GPIO22 |
| MPU6050 VCC | 3.3V |
| MPU6050 GND | GND |
| Sıfırlama butonu (1 ucu) | GPIO27 |
| Sıfırlama butonu (diğer ucu) | GND |
| Batarya (+) → 2 direnç ile bölünmüş uç | GPIO34 |

Buton ve batarya devresi yoksa şimdilik atlayabilirsin, sadece MPU6050 bağlı olsun
yeter — kod çalışır, buton/pil kısmı sonra eklenir.

### 2️⃣ Bilgisayarda Arduino IDE Hazırlığı

1. **Arduino IDE**'yi aç (yoksa arduino.cc'den indir).
2. Üstte **Dosya → Tercihler**'e gir, "Ek Kart Yöneticisi URL'leri" kutusuna şunu ekle:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Araçlar → Kart → Kart Yöneticisi**'ne gir, "esp32" ara, kur.
4. **Araçlar → Kütüphane Yöneticisi**'ne gir, şunları tek tek ara ve kur:
   - `Adafruit MPU6050`
   - `Adafruit Unified Sensor`

   (BLE kütüphanesi ekstra kurulum gerektirmez, ESP32 çekirdeğiyle birlikte gelir.)

### 3️⃣ Firmware'i Yükleme (BLE sürümü)

1. Bilgisayarında `dentiv-panel` klasörünü aç (GitHub'dan indirdiğin/klonladığın).
2. `firmware/dentiv_esp32_ble/dentiv_esp32_ble.ino` dosyasına çift tıkla — Arduino IDE açılır.
   (`dentiv_esp32` — WiFi sürümü — ile karıştırma.)
3. ESP32'yi USB ile bilgisayara tak.
4. **Araçlar → Kart** → kullandığın ESP32 modelini seç (genelde "ESP32 Dev Module").
5. **Araçlar → Port** → ESP32'nin bağlı olduğu portu seç.
6. Sağ üstteki **Yükle (→)** butonuna bas, bitmesini bekle.
7. **Araçlar → Seri Port Monitörü**'nü aç, baud hızını **115200** yap. Şunu göreceksin:
   ```
   BLE yayını başladı, cihaz adı: DENTIV
   ```

### 4️⃣ iOS Uygulamasını Kurma ve Telefona Yükleme

Detaylı adımlar için [mobile-app/README.md](mobile-app/README.md)'ye bakın. Özet:

```bash
cd mobile-app
npm install
npx cap sync ios
npx cap open ios
```

Xcode açılınca **Signing & Capabilities**'ten Apple Developer hesabını (Team)
seç, iPhone'u USB ile bağlayıp **▶ Run**'a bas. Uygulama ilk açıldığında
Bluetooth izni isteyecek.

### 5️⃣ Kullanım

1. ESP32'yi çalıştır (USB güç bankı ya da batarya ile).
2. iPhone'da Dentiv app'ini aç.
3. **"Cihaz Tara ve Bağlan"**'a bas, çıkan listeden `DENTIV`'i seç.
4. Bağlantı kurulunca pitch/roll canlı gelmeye başlar.

> Bu akışta telefonun WiFi/internet bağlantısına hiç dokunulmuyor — BLE ayrı
> bir kanal, normal internet kullanımını etkilemiyor.

---

## Alternatif: WiFi/WebSocket web paneli (isteğe bağlı, artık ikincil)

Bu yaklaşımda ESP32 kendi WiFi ağını açar, `index.html`'i kendi üzerinden
sunar, tarayıcı WebSocket ile bağlanır. Web Bluetooth'un iOS'ta hiç
desteklenmemesi sorununu çözer ama her kullanımda telefonun WiFi ağını
değiştirmesi gerektiğinden günlük kullanım için native app kadar pratik değil.
Yine de kod versiyon kontrolü / masaüstünden hızlı önizleme için işe yarayabilir.

**Kullanmak isterseniz:** `firmware/dentiv_esp32/dentiv_esp32.ino` firmware'ini
yükleyin (BLE sürümü yerine), ESP32'nin açtığı `DENTIV-xxxx` ağına bağlanıp
Safari'de `http://192.168.4.1` açın, "Ana Ekrana Ekle" ile kullanın.
Vercel'e deploy etmek isterseniz repo kökünü Vercel'de "Import Project" ile
bağlamanız yeterli (statik site, ekstra ayar gerekmez) — ama o link doğrudan
ESP32'ye bağlanamaz (mixed-content kısıtlaması), sadece kod önizlemesi/yedeği
içindir.

## Klasör yapısı

```
mobile-app/          Native iOS app (Capacitor + BLE) — ana kullanım yolu
  www/index.html      Uygulama arayüzü (BLE mantığı)
  ios/                Xcode projesi (npx cap add ios ile üretildi)
  resources/icon.png  App ikonu kaynağı

firmware/
  dentiv_esp32_ble/   BLE firmware'i (mobile-app ile birlikte kullanılır)
  dentiv_esp32/       WiFi/WebSocket firmware'i (alternatif web paneli için)

index.html, manifest.json, sw.js, icons/   WiFi web paneli (alternatif yol)
tools/generate_firmware_html.py            index.html -> webpage.h üretici script (yalnızca WiFi firmware'i için)
```

## Veri formatı (ESP32 → App/Tarayıcı)

Hem BLE notification hem WebSocket üzerinden aynı format kullanılıyor, virgülle
ayrılmış metin karesi:

```
pitch,roll,battery[,ZERO]
```

- `pitch`, `roll`: derece cinsinden ondalık sayı
- `battery`: 0-100 arası tam sayı
- `ZERO` (opsiyonel 4. alan): cihaz üzerindeki fiziksel butona basıldığını belirtir,
  uygulama bunu görünce aktif ölçüm parametresini otomatik sıfırlar

## Donanım ayarlarını değiştirmek istersen

Her iki firmware'in de (`dentiv_esp32_ble.ino` ve `dentiv_esp32.ino`) en üstündeki
`YAPILANDIRMA` bölümünden şunları özelleştirebilirsin:
- `ZERO_BUTTON_PIN`, `BATTERY_PIN`: pin numaraları
- `BATTERY_DIVIDER_RATIO`, `BATTERY_MIN_VOLTAGE`, `BATTERY_MAX_VOLTAGE`: gerilim bölücünüze göre pil yüzdesi hesaplaması
- WiFi sürümünde ayrıca `AP_PASSWORD`
