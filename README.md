# Dentiv - Açı Ölçüm Sistemi

ESP32 tabanlı, WiFi/WebSocket üzerinden çalışan hassas açı ölçüm paneli. iOS dahil
tüm tarayıcılarda çalışır (Web Bluetooth kullanmaz).

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

## ESP32 firmware kurulumu

**Donanım:**
- MPU6050 IMU: SDA → GPIO21, SCL → GPIO22
- Sıfırlama butonu: GPIO27 → GND (dahili pull-up kullanılıyor)
- Batarya gerilim bölücü: GPIO34 (varsayılan 2x100kΩ ile 1/2 bölücü; farklıysa
  `dentiv_esp32.ino` içindeki `BATTERY_DIVIDER_RATIO` değerini güncelleyin)

**Arduino IDE kütüphaneleri (Kütüphane Yöneticisi'nden kurun):**
- ESP Async WebServer
- Async TCP
- Adafruit MPU6050
- Adafruit Unified Sensor

**Adımlar:**
1. `index.html` üzerinde değişiklik yaptıysanız önce şunu çalıştırın:
   ```bash
   python3 tools/generate_firmware_html.py
   ```
   Bu, `firmware/dentiv_esp32/webpage.h` dosyasını günceller.
2. `firmware/dentiv_esp32/dentiv_esp32.ino` dosyasını Arduino IDE'de açın.
3. Board olarak kullandığınız ESP32 kartını seçin, USB ile bağlayıp yükleyin.
4. Seri port monitöründe (115200 baud) WiFi ağı adını ve IP adresini görebilirsiniz
   (varsayılan `http://192.168.4.1`).
5. `AP_PASSWORD` değerini (`dentiv_esp32.ino` içinde) dağıtımdan önce değiştirin.

## iOS'ta ana ekrana ekleme (gerçek kullanım)

1. iPhone'un WiFi ayarlarından `DENTIV-xxxx` ağına bağlanın (şifre: firmware'de
   belirlediğiniz `AP_PASSWORD`).
2. Safari'de `http://192.168.4.1` adresini açın.
3. Paylaş menüsünden **"Ana Ekrana Ekle"**yi seçin.
4. Artık ikon üzerinden tam ekran (adres çubuğu olmadan) açılır.

> Not: ESP32'nin kendi ağına bağlıyken telefonun internet erişimi olmaz; bu
> normaldir, panel zaten internete ihtiyaç duymaz.

## Vercel'e deploy (versiyon kontrolü / masaüstü önizleme için)

```bash
# GitHub'a push ettikten sonra Vercel dashboard'dan "Import Project" ile
# bu repoyu bağlamanız yeterli - ekstra build ayarı gerekmez (statik site).
```

Vercel'deki sürüm internetten erişilebilir ama ESP32'ye bağlanamaz (yukarıdaki
mixed-content kısıtlaması nedeniyle) — sayfa açıldığında bunu belirten bir uyarı
gösterir ve kullanıcıyı `http://192.168.4.1`'e yönlendirir.

## Veri formatı (ESP32 → Tarayıcı)

WebSocket üzerinden metin karesi, virgülle ayrılmış:

```
pitch,roll,battery[,ZERO]
```

- `pitch`, `roll`: derece cinsinden ondalık sayı
- `battery`: 0-100 arası tam sayı
- `ZERO` (opsiyonel 4. alan): cihaz üzerindeki fiziksel butona basıldığını belirtir,
  tarayıcı bunu görünce aktif ölçüm parametresini otomatik sıfırlar
