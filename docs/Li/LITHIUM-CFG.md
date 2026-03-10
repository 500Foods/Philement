# Lithium Configuration

Lithium uses a JSON configuration file (`lithium.json`) loaded at runtime. This document covers the configuration structure and all available options.

**File:** [`config/lithium.json`](elements/003-lithium/config/lithium.json)

---

## Configuration Structure

```json
{
  "server": { ... },
  "auth": { ... },
  "app": { ... },
  "icons": { ... },
  "features": { ... }
}
```

---

## Server Configuration

```json
"server": {
  "url": "https://lithium.philement.com",
  "api_prefix": "/api",
  "websocket_url": "wss://lithium.philement.com/wss"
}
```

| Property | Type | Description |
|----------|------|-------------|
| `url` | string | Hydrogen backend server URL |
| `api_prefix` | string | API route prefix |
| `websocket_url` | string | WebSocket server URL (future use) |

---

## Authentication Configuration

```json
"auth": {
  "api_key": "EveryGoodBoyDeservesFudge",
  "default_database": "Lithium",
  "session_timeout_minutes": 30
}
```

| Property | Type | Description |
|----------|------|-------------|
| `api_key` | string | API key sent with login requests |
| `default_database` | string | Default database to connect to |
| `session_timeout_minutes` | number | Session timeout in minutes |

---

## Application Configuration

```json
"app": {
  "name": "Lithium",
  "tagline": "A Philement Project",
  "version": "1.0.0",
  "logo": "/assets/images/logo-li.svg",
  "default_theme": "dark",
  "default_language": "en-US"
}
```

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Application name |
| `tagline` | string | Application tagline |
| `version` | string | Application version |
| `logo` | string | Logo path |
| `default_theme` | string | Default theme (currently only `dark`) |
| `default_language` | string | Default locale code |

---

## Icon Configuration

This section configures the Font Awesome icon system.

```json
"icons": {
  "set": "duotone",
  "weight": "thin",
  "prefix": "fa-duotone fa-thin",
  "fallback": "solid"
}
```

| Property | Type | Description |
|----------|------|-------------|
| `set` | string | Font Awesome set name (`solid`, `regular`, `light`, `thin`, `duotone`, `sharp-solid`, etc.) |
| `weight` | string | Font weight for compatible sets |
| `prefix` | string | Full CSS prefix (overrides set/weight) |
| `fallback` | string | Fallback set if primary unavailable |

### Icon Sets

| Set | Prefix | Example |
|------|--------|---------|
| `solid` | `fas` | `fas fa-user` |
| `regular` | `far` | `far fa-user` |
| `light` | `fal` | `fal fa-user` |
| `thin` | `fat` | `fat fa-user` |
| `duotone` | `fad` | `fad fa-user` |
| `sharp-solid` | `fass` | `fass fa-user` |
| `brands` | `fab` | `fab fa-github` |

**Note:** The `prefix` field allows full control over the CSS classes generated. The value `fa-duotone fa-thin` generates icons like: `<i class="fa-duotone fa-thin fa-user"></i>`

---

## Features Configuration

```json
"features": {
  "offline_mode": true,
  "install_prompt": true,
  "debug_logging": false
}
```

| Property | Type | Description |
|----------|------|-------------|
| `offline_mode` | boolean | Enable service worker offline support |
| `install_prompt` | boolean | Show PWA install prompt |
| `debug_logging` | boolean | Enable verbose console logging |

---

## Loading Configuration

The configuration is loaded by [`core/config.js`](elements/003-lithium/src/core/config.js):

```javascript
export async function loadConfig() {
  const response = await fetch('/config/lithium.json');
  const config = await response.json();
  // Deep merge with defaults
  cachedConfig = deepMerge(DEFAULT_CONFIG, config);
  return cachedConfig;
}
```

### Default Values

If the config file is missing or incomplete, sensible defaults apply:

```javascript
const DEFAULT_CONFIG = {
  server: {
    url: 'http://localhost:8080',
    api_prefix: '/api',
    websocket_url: 'ws://localhost:8080/ws',
  },
  auth: {
    api_key: '',
    default_database: 'Demo_PG',
    session_timeout_minutes: 30,
  },
  app: {
    name: 'Lithium',
    tagline: 'A Philement Project',
    version: '1.0.0',
    logo: '/assets/images/logo-li.svg',
    default_theme: 'dark',
    default_language: 'en-US',
  },
  features: {
    offline_mode: true,
    install_prompt: true,
    debug_logging: false,
  },
};
```

---

## Accessing Configuration

### Get Full Config

```javascript
import { getConfig } from '../../core/config.js';

const config = getConfig();
console.log(config.server.url);
```

### Get Single Value

```javascript
import { getConfigValue } from '../../core/config.js';

const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
const apiKey = getConfigValue('auth.api_key', '');
```

### Dot Notation

Use dot notation to access nested values:

```javascript
getConfigValue('icons.set')        // 'duotone'
getConfigValue('icons.weight')     // 'thin'
getConfigValue('features.offline_mode') // true
```

---

## Runtime Updates

The config file uses **stale-while-revalidate** caching strategy, meaning:

- Cached config is served immediately for fast load
- Background fetch checks for updates
- Next page load uses updated config

This allows runtime configuration changes without deployment.

---

## Environment-Specific Config

For different environments (dev, staging, prod), create separate config files:

```structure
config/
├── lithium.json        # Default/base
├── lithium.dev.json   # Development overrides
└── lithium.prod.json  # Production overrides
```

Then configure your server to serve the appropriate file based on environment.

---

## Related Documentation

- [LITHIUM-ICN.md](LITHIUM-ICN.md) - Icon system (uses icons config)
- [LITHIUM-PWA.md](LITHIUM-PWA.md) - Service worker (uses features config)
- [LITHIUM-OTH.md](LITHIUM-OTH.md) - Config module (core/config.js)
