const CACHE_NAME = "srr-shell-v0.4.0-beta.1";
const SHELL_PATHS = [
  "./", "./index.html", "./manifest.webmanifest", "./icon.svg",
  "./srr-core.js", "./srr-core.wasm",
];

self.addEventListener("install", (event) => {
  event.waitUntil(caches.open(CACHE_NAME).then((cache) => cache.addAll(
    SHELL_PATHS.map((path) => new URL(path, self.registration.scope).href),
  )).then(() => self.skipWaiting()));
});

self.addEventListener("activate", (event) => {
  event.waitUntil(caches.keys().then((keys) => Promise.all(
    keys.filter((key) => key.startsWith("srr-shell-") && key !== CACHE_NAME)
      .map((key) => caches.delete(key)),
  )).then(() => self.clients.claim()));
});

self.addEventListener("fetch", (event) => {
  const request = event.request;
  if (request.method !== "GET" || new URL(request.url).origin !== self.location.origin) return;
  if (request.mode === "navigate") {
    event.respondWith(fetch(request).catch(() => caches.match(new URL("./index.html", self.registration.scope).href)));
    return;
  }
  event.respondWith(caches.match(request).then((cached) => cached || fetch(request)));
});
