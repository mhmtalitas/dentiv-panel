# Dentiv - Açı Ölçüm Sistemi

ESP32 tabanlı, WiFi/WebSocket üzerinden çalışan hassas açı ölçüm paneli. iOS dahil
tüm tarayıcılarda çalışır (Web Bluetooth kullanmaz).

---

## Kurulum Rehberi (Adım Adım)

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
   - `ESP Async WebServer`
   - `Async TCP`
   - `Adafruit MPU6050`
   - `Adafruit Unified Sensor`

### 3️⃣ Firmware'i Yükleme

1. Bilgisayarında `dentiv-panel` klasörünü aç (GitHub'dan indirdiğin/klonladığın).
2. `firmware/dentiv_esp32/dentiv_esp32.ino` dosyasına çift tıkla — Arduino IDE açılır.
3. ESP32'yi USB ile bilgisayara tak.
4. **Araçlar → Kart** → kullandığın ESP32 modelini seç (genelde "ESP32 Dev Module").
5. **Araçlar → Port** → ESP32'nin bağlı olduğu portu seç.
6. Sağ üstteki **Yükle (→)** butonuna bas, bitmesini bekle.
7. **Araçlar → Seri Port Monitörü**'nü aç, baud hızını **115200** yap. Şunu göreceksin:
   ```
   WiFi ağı başlatıldı: DENTIV-xxxx
   Panel adresi: http://192.168.4.1
   ```

### 4️⃣ Telefonda (iOS) Kullanma

1. iPhone **Ayarlar → WiFi**'ye git, `DENTIV-xxxx` ağına bağlan (şifre: `dentiv2024`,
   `dentiv_esp32.ino` içindeki `AP_PASSWORD` ile aynı olmalı).
2. **Safari**'yi aç, adres çubuğuna `192.168.4.1` yaz, git.
3. Panel açılınca alt ortadaki **Paylaş** ikonuna (kare + ok) bas → **"Ana Ekrana Ekle"**'yi seç.
4. Artık telefonun ana ekranında DENTIV ikonu var, ona basınca uygulama gibi tam ekran açılır.

> Not: ESP32'nin ağına bağlıyken internetin gitmesi normal — panel zaten internete
> ihtiyaç duymuyor.

### 5️⃣ Vercel'e Yükleme (isteğe bağlı, kod yedeği/önizleme için)

1. [vercel.com](https://vercel.com)'a git, GitHub hesabınla giriş yap.
2. **Add New → Project**'e bas.
3. `dentiv-panel` reposunu seç, **Import**'a bas.
4. Hiçbir ayar değiştirmeden **Deploy**'a bas.
5. Birkaç saniyede bir link üretir (örn. `dentiv-panel.vercel.app`) — bu link
   sadece kod önizlemesi/yedeği içindir, ESP32'ye bağlanmaz (aşağıdaki "Neden
   WebSocket?" bölümündeki HTTPS kısıtlaması yüzünden). Gerçek kullanım her
   zaman **`192.168.4.1`** üzerinden.

---

## Neden WebSocket, neden Web Bluetooth değil?

Safari (ve iOS'taki tüm tarayıcılar, WebKit motoru zorunlu olduğu için) Web
Bluetooth API'sini desteklemiyor. Bu yüzden panel, ESP32 ile **WiFi + WebSocket**
üzerinden konuşuyor.

HTTPS bir sayfadan (örn. Vercel) yerel ağdaki ESP32'ye şifresiz `ws://` isteği
atmak "mixed content" olarak tarayıcı tarafından engellenir. Bu yüzden **ESP32
kendi arayüzünü kendisi sunar**: cihaz kendi WiFi ağını açar, sayfayı ve
WebSocket'i aynı adresten (`http://192.168.4.1`) verir. Telefon gerçek
kullanımda bu adresi ana ekrana ekler; Vercel ise kod versiyonlama ve
masaüstünden önizleme içindir.

## Klasör yapısı

```
index.html          Ana panel (hem ESP32'ye gömülür hem Vercel'e deploy edilir)
manifest.json        PWA manifesti (Vercel/tarayıcı üzerinden "Install" için)
sw.js                Service worker (yalnızca HTTPS'te devrede, offline shell cache)
icons/               PWA ikonları
firmware/dentiv_esp32/
  dentiv_esp32.ino   ESP32 Arduino firmware'i
  webpage.h          index.html'den otomatik üretilen gömülü HTML (elle düzenlemeyin)
tools/generate_firmware_html.py   index.html -> webpage.h üretici script
```

## index.html'de değişiklik yaptıysan

Firmware'i yeniden yüklemeden önce şunu çalıştır, yoksa ESP32 eski arayüzü sunmaya
devam eder:

```bash
python3 tools/generate_firmware_html.py
```

Bu, `firmware/dentiv_esp32/webpage.h` dosyasını günceller.

## Donanım ayarlarını değiştirmek istersen

`firmware/dentiv_esp32/dentiv_esp32.ino` dosyasının en üstündeki `YAPILANDIRMA`
bölümünden şunları özelleştirebilirsin:
- `AP_PASSWORD`: WiFi ağı şifresi (dağıtımdan önce değiştirin)
- `ZERO_BUTTON_PIN`, `BATTERY_PIN`: pin numaraları
- `BATTERY_DIVIDER_RATIO`, `BATTERY_MIN_VOLTAGE`, `BATTERY_MAX_VOLTAGE`: gerilim bölücünüze göre pil yüzdesi hesaplaması

## Veri formatı (ESP32 → Tarayıcı)

WebSocket üzerinden metin karesi, virgülle ayrılmış:

```
pitch,roll,battery[,ZERO]
```

- `pitch`, `roll`: derece cinsinden ondalık sayı
- `battery`: 0-100 arası tam sayı
- `ZERO` (opsiyonel 4. alan): cihaz üzerindeki fiziksel butona basıldığını belirtir,
  tarayıcı bunu görünce aktif ölçüm parametresini otomatik sıfırlar
