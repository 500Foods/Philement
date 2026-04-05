// Lithium PWA Service Worker
// Cache strategy: cache-first for statics, stale-while-revalidate for API data

const CACHE_VERSION = 2064;
const STATIC_CACHE = `lithium-static-v${CACHE_VERSION}`;
const API_CACHE = `lithium-api-v${CACHE_VERSION}`;

// Core static assets — cache-first strategy
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

// Install event — pre-cache static assets
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(STATIC_CACHE)
      .then((cache) => {
        console.log('[SW] Pre-caching static assets');
        return cache.addAll(STATIC_ASSETS);
      })
      .then(() => self.skipWaiting())
      .catch((error) => {
        console.error('[SW] Failed to pre-cache:', error);
      })
  );
});

// Allow the page to request immediate activation when an updated worker is waiting
self.addEventListener('message', (event) => {
  if (event.data?.type === 'SKIP_WAITING') {
    self.skipWaiting();
  }
});

// Activate event — clean up old cache versions
self.addEventListener('activate', (event) => {
  const currentCaches = [STATIC_CACHE, API_CACHE];

  event.waitUntil(
    caches.keys()
      .then((cacheNames) => {
        return Promise.all(
          cacheNames
            .filter((name) => !currentCaches.includes(name))
            .map((name) => {
              console.log('[SW] Deleting old cache:', name);
              return caches.delete(name);
            })
        );
      })
      .then(() => {
        console.log('[SW] Activated, controlling all clients');
        return self.clients.claim();
      })
  );
});

// Fetch event — route requests to appropriate strategy
self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);

  // Skip non-GET requests
  if (event.request.method !== 'GET') {
    return;
  }

  // Skip non-cacheable schemes (chrome-extension, moz-extension, etc.)
  if (!url.protocol.startsWith('http')) {
    return;
  }

  // API requests — stale-while-revalidate
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(staleWhileRevalidate(event.request, API_CACHE));
    return;
  }

  // Version file — network-first (must return fresh data for update detection)
  if (url.pathname.endsWith('/version.json')) {
    event.respondWith(networkFirst(event.request, API_CACHE));
    return;
  }

  // Config file — stale-while-revalidate (allows runtime updates)
  if (url.pathname.endsWith('/lithium.json')) {
    event.respondWith(staleWhileRevalidate(event.request, API_CACHE));
    return;
  }

  // Static assets and app code — cache-first
  event.respondWith(cacheFirst(event.request, STATIC_CACHE));
});

/**
 * Cache-first strategy: serve from cache, fall back to network.
 * Caches new network responses for future use.
 */
async function cacheFirst(request, cacheName) {
  const cached = await caches.match(request);
  if (cached) {
    return cached;
  }

  try {
    const response = await fetch(request);

    // Cache successful same-origin responses
    if (response && response.status === 200 && response.type === 'basic') {
      const cache = await caches.open(cacheName);
      cache.put(request, response.clone());
    }

    return response;
  } catch (error) {
    // Offline fallback for navigation requests
    if (request.destination === 'document') {
      return caches.match('/index.html');
    }
    throw error;
  }
}

/**
 * Network-first strategy: try network, fall back to cache.
 * Used for version.json where freshness is critical for update detection.
 */
async function networkFirst(request, cacheName) {
  const cache = await caches.open(cacheName);

  try {
    const response = await fetch(request);

    // Cache successful responses for offline fallback
    if (response && response.status === 200) {
      cache.put(request, response.clone());
    }

    return response;
  } catch (error) {
    // Network failed — return cached version if available
    const cached = await cache.match(request);
    if (cached) {
      return cached;
    }
    throw error;
  }
}

/**
 * Stale-while-revalidate: serve cached version immediately,
 * update cache in the background from network.
 */
async function staleWhileRevalidate(request, cacheName) {
  const cache = await caches.open(cacheName);
  const cached = await cache.match(request);

  // Fetch fresh copy in the background
  const fetchPromise = fetch(request)
    .then((response) => {
      if (response && response.status === 200) {
        cache.put(request, response.clone());
      }
      return response;
    })
    .catch(() => cached);

  // Return cached immediately if available, otherwise wait for network
  return cached || fetchPromise;
}

// Push notification support
self.addEventListener('push', (event) => {
  if (!event.data) return;

  try {
    const data = event.data.json();
    const options = {
      body: data.body,
      icon: '/assets/images/logo-192x192.png',
      badge: '/assets/images/logo-192x192.png',
      data: { url: data.url || '/' }
    };

    event.waitUntil(
      self.registration.showNotification(data.title || 'Lithium', options)
    );
  } catch (error) {
    console.error('[SW] Push notification error:', error);
  }
});

// Notification click handler
self.addEventListener('notificationclick', (event) => {
  event.notification.close();

  const targetUrl = event.notification.data?.url || '/';

  event.waitUntil(
    clients.matchAll({ type: 'window', includeUncontrolled: true })
      .then((clientList) => {
        // Focus existing window if found
        for (const client of clientList) {
          if (client.url === targetUrl && 'focus' in client) {
            return client.focus();
          }
        }
        // Open new window
        if (clients.openWindow) {
          return clients.openWindow(targetUrl);
        }
      })
  );
});
