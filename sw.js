// Dentiv - PWA app shell cache
// Yalnızca aynı origin'deki statik dosyaları önbelleğe alır; ESP32'nin WebSocket
// bağlantısına veya farklı origin isteklerine dokunmaz.
const CACHE_NAME = 'dentiv-shell-v1';
const SHELL_FILES = [
    './',
    './index.html',
    './manifest.json',
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then((cache) => cache.addAll(SHELL_FILES))
            .catch(() => { /* ilk kurulumda ağ yoksa sessizce geç */ })
    );
    self.skipWaiting();
});

self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((keys) =>
            Promise.all(keys.filter((k) => k !== CACHE_NAME).map((k) => caches.delete(k)))
        )
    );
    self.clients.claim();
});

self.addEventListener('fetch', (event) => {
    const url = new URL(event.request.url);

    // Farklı origin'e giden istekler (ESP32'nin kendi IP'si vb.) service worker dışında kalsın
    if (url.origin !== self.location.origin) return;
    if (event.request.method !== 'GET') return;

    event.respondWith(
        caches.match(event.request).then((cached) => {
            if (cached) return cached;
            return fetch(event.request).then((response) => {
                const clone = response.clone();
                caches.open(CACHE_NAME).then((cache) => cache.put(event.request, clone));
                return response;
            }).catch(() => cached);
        })
    );
});
