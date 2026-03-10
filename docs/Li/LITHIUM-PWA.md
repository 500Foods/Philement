# Lithium PWA (Progressive Web App)

Lithium is a PWA with service worker caching, offline support, and fast update handling. This document covers the service worker, manifest, and the smart update detection system.

**Files:**

- [`public/service-worker.js`](elements/003-lithium/public/service-worker.js)
- [`public/manifest.json`](elements/003-lithium/public/manifest.json)
- [`index.html`](elements/003-lithium/index.html) (bootstrap gate)

---

## Service Worker

### Cache Strategy

The service worker uses different strategies for different resource types:

| Request Type | Strategy | Rationale |
|--------------|----------|------------|
| Static assets (JS, CSS, fonts) | **Cache-first** | Fast load, rarely change |
| API requests | **Stale-while-revalidate** | Balance freshness/cached |
| `version.json` | **Network-first** | Must have fresh data for update detection |
| `lithium.json` config | **Stale-while-revalidate** | Allows runtime updates |

### Cache Versions

```javascript
const CACHE_VERSION = 1177;
const STATIC_CACHE = `lithium-static-v${CACHE_VERSION}`;
const API_CACHE = `lithium-api-v${CACHE_VERSION}`;
```

On each deploy, the cache version increments to force fresh caching.

### Static Assets Cached

```javascript
const STATIC_ASSETS = [
  '/',
  '/index.html',
  '/manifest.json',
  '/favicon.ico',
  '/assets/images/logo-li.svg',
  '/assets/images/logo-192x192.png',
  '/assets/images/logo-512x512.png',
  '/assets/fonts/VanadiumSans-SemiExtended.woff2',
  '/assets/fonts/VanadiumMono-SemiExtended.woff2'
];
```

---

## Fast Update Handling

The key innovation in Lithium's PWA implementation is the **early bootstrap gate** in `index.html`. This prevents the annoying "old UI, then reload" pattern.

### The Problem

Traditional PWA update flow:

1. User visits site → old version loads
2. SW detects update, installs new version
3. User sees old UI
4. SW activates → page reloads
5. User sees flash while new version loads

### Lithium's Solution

The bootstrap gate runs BEFORE the app initializes:

```javascript
// In index.html - runs before anything else
window.__lithiumVersionPromise = (async function() {
  // 1. Start SW registration
  var swBootGate = startServiceWorkerBootGate();

  // 2. Fetch version.json with no-store (fresh every time)
  var response = await fetch('/version.json', { cache: 'no-store' });
  var data = await response.json();

  // 3. Compare with sessionStorage build number
  var stored = sessionStorage.getItem('lithium_build');
  var current = String(data.build);

  // 4. If mismatch, reload IMMEDIATELY - before showing any UI
  if (stored && stored !== current) {
    sessionStorage.setItem('lithium_build', current);
    window.location.reload();
    await blockAppInitForever(); // NEVER resolves
  }
})();
```

### SW Update Detection

```javascript
async function startServiceWorkerBootGate() {
  var registration = await navigator.serviceWorker.register('/service-worker.js', {
    updateViaCache: 'none' // Always check network for updates
  });

  // If there's a waiting SW, activate immediately
  if (registration.waiting) {
    registration.waiting.postMessage({ type: 'SKIP_WAITING' });
  }

  // Wait briefly (1.5s max) for activation
  await Promise.race([
    new Promise(resolve => {
      candidate.addEventListener('statechange', () => {
        if (candidate.state === 'activated') resolve();
      });
    }),
    new Promise(resolve => setTimeout(resolve, 1500))
  ]);

  // If update happened, block app and reload
  if (reloadPending) {
    await blockAppInitForever(); // Never resolves - app hangs
  }
}
```

### Result

- **No UI flash** - If update available, page hangs until new version ready
- **Instant update** - Network-first for version.json means update detected on every visit
- **Smooth reload** - PostMessage to SW triggers immediate activation

---

## Manifest

**File:** [`public/manifest.json`](elements/003-lithium/public/manifest.json)

```json
{
  "name": "Lithium",
  "short_name": "Lithium",
  "description": "A Philement Project",
  "start_url": ".",
  "display": "standalone",
  "background_color": "#1A1A1A",
  "theme_color": "#5C6BC0",
  "orientation": "any",
  "scope": "./",
  "icons": [
    { "src": "assets/images/logo-192x192.png", "sizes": "192x192", "type": "image/png", "purpose": "any" },
    { "src": "assets/images/logo-512x512.png", "sizes": "512x512", "type": "image/png", "purpose": "any" },
    { "src": "assets/images/logo-li.svg", "sizes": "any", "type": "image/svg+xml", "purpose": "any" }
  ]
}
```

### Key Properties

| Property | Value | Purpose |
|----------|-------|---------|
| `display` | `standalone` | Opens without browser chrome |
| `background_color` | `#1A1A1A` | Matches dark theme |
| `theme_color` | `#5C6BC0` | Accent color in task switcher |

---

## Push Notifications

The service worker handles push notifications:

```javascript
self.addEventListener('push', (event) => {
  const data = event.data.json();
  self.registration.showNotification(data.title, {
    body: data.body,
    icon: '/assets/images/logo-192x192.png',
    badge: '/assets/images/logo-192x192.png',
    data: { url: data.url || '/' }
  });
});
```

### Notification Click

```javascript
self.addEventListener('notificationclick', (event) => {
  event.notification.close();
  // Focus existing window or open new
});
```

---

## Offline Support

### Cache-First (Static Assets)

```javascript
async function cacheFirst(request, cacheName) {
  const cached = await caches.match(request);
  if (cached) return cached; // Serve from cache immediately

  const response = await fetch(request);
  if (response.status === 200) {
    const cache = await caches.open(cacheName);
    cache.put(request, response.clone());
  }
  return response;
}
```

### Offline Fallback

For navigation requests that fail:

```javascript
async function cacheFirst(request, cacheName) {
  // ...
  } catch (error) {
    if (request.destination === 'document') {
      return caches.match('/index.html'); // Offline fallback
    }
    throw error;
  }
}
```

---

## Testing PWA

### Check SW Registration

```javascript
// In browser console
navigator.serviceWorker.getRegistration().then(r => console.log(r));
```

### Force Update Check

```javascript
// Clear cache and re-register
navigator.serviceWorker.getRegistration().then(r => r.update());
```

### View Caches

```javascript
// In browser console
caches.keys().then(console.log);
```

---

## Deployment Notes

1. **Cache version** - Increment `CACHE_VERSION` in service-worker.js on each deploy
2. **version.json** - Ensure `/version.json` is generated with unique build number
3. **Static assets** - Add any new assets to `STATIC_ASSETS` array
4. **HTTPS required** - Service workers require HTTPS (or localhost)

---

## Related Documentation

- [LITHIUM-WEB.md](LITHIUM-WEB.md) - Deployment process
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
