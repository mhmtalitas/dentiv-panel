# Dentiv - iOS App (Capacitor + Bluetooth LE)

ESP32'ye doğrudan Bluetooth (BLE) üzerinden bağlanan gerçek bir iOS uygulaması.
Web Bluetooth'a değil, native CoreBluetooth'a erişen bir Capacitor eklentisi
(`@capacitor-community/bluetooth-le`) kullanır — bu yüzden Safari'nin
desteklemediği bir şeyi native app olarak yapabiliyoruz.

Arayüz kodu `www/index.html` içinde düz HTML/CSS/JS olarak yazıldı (build
aracı/bundler yok), `capacitor.js` ve `bluetooth-le.js` script'leri Capacitor'ün
kendi paketlerinden kopyalandı.

## Gerekli olanlar

- Xcode (kuruluysa hazırsınız — `xcode-select -p` ile kontrol edilebilir)
- Node.js / npm
- Apple Developer hesabı (kendi telefonuna kalıcı yükleme ve ileride
  TestFlight/App Store için)

## İlk kurulum

```bash
cd mobile-app
npm install
npx cap sync ios
```

## Xcode'da açma ve telefona yükleme

1. Xcode'u aç:
   ```bash
   npx cap open ios
   ```
2. Sol panelden **App** projesine tıkla → **Signing & Capabilities** sekmesi.
3. **Team** açılır menüsünden Apple Developer hesabını seç.
4. **Bundle Identifier** alanı `com.dentiv.panel` — istersen kendi domain'ine
   göre değiştirebilirsin (örn. `com.senin-adin.dentiv`), tek şart App Store
   Connect'te benzersiz olması.
5. iPhone'u USB ile bağla (veya aynı ağdaysa kablosuz debugging), üstteki cihaz
   seçiciden telefonunu seç.
6. **▶ (Run)** butonuna bas. İlk yüklemede telefonda **Ayarlar → Genel →
   VPN ve Cihaz Yönetimi**'nden geliştirici sertifikana güvenmen istenecek.
7. Uygulama açılınca Bluetooth izni isteyecek — **İzin Ver**.

## index.html'de değişiklik yaptıysan

`www/index.html` üzerinde değişiklik yaptıktan sonra, Xcode'da tekrar
derlemeden önce şunu çalıştır:

```bash
npx cap sync ios
```

Bu, `www/`'yi `ios/App/App/public/`'e kopyalar.

## ESP32 firmware

Bu uygulama, `../firmware/dentiv_esp32_ble/dentiv_esp32_ble.ino` firmware'i ile
eşleşir (BLE tabanlı, WiFi'lı sürümle karıştırmayın). Kurulum adımları için
ana [README.md](../README.md)'ye bakın.

## App ikonu / splash ekranını değiştirmek istersen

`resources/icon.png` (1024x1024) dosyasını değiştirip şunu çalıştır:

```bash
npx @capacitor/assets generate --iconBackgroundColor '#0f1115' --iconBackgroundColorDark '#0f1115' --splashBackgroundColor '#0f1115' --splashBackgroundColorDark '#0f1115' --ios
```

## TestFlight / App Store dağıtımı (ileride)

Ücretli Developer hesabınla Xcode üzerinden **Product → Archive** yapıp
App Store Connect'e yükleyerek TestFlight'a dağıtabilirsin — bu, herkesin
Xcode'a ihtiyaç duymadan uygulamayı yükleyebilmesini sağlar.
