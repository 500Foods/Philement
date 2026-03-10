# Lithium Other Utilities

This document covers utility modules that don't warrant their own dedicated documentation yet but are important for understanding the codebase.

**Location:** `src/core/`

---

## Transitions

**File:** [`transitions.js`](elements/003-lithium/src/core/transitions.js)

Provides utilities for managing CSS transitions and animations.

### Functions

| Function | Purpose |
|----------|---------|
| `getTransitionDuration()` | Get transition duration from CSS variable |
| `waitForTransition(duration)` | Wait for transition to complete (returns Promise) |
| `fadeIn(element)` | Fade in an element |
| `fadeOut(element)` | Fade out an element |

### Usage

```javascript
import { getTransitionDuration, waitForTransition } from '../../core/transitions.js';

// Get configured transition duration
const duration = getTransitionDuration(); // Returns 500 (ms)

// Wait for transition
await waitForTransition(duration);
```

---

## Utils

**File:** [`utils.js`](elements/003-lithium/src/core/utils.js)

General utility functions used throughout the application.

### Utils Functions

| Function | Purpose |
|----------|---------|
| `debounce(fn, delay)` | Debounce function calls |
| `throttle(fn, limit)` | Throttle function calls |
| `formatDate(date, locale)` | Format dates |
| `formatNumber(num, locale)` | Format numbers |
| `deepClone(obj)` | Deep clone an object |
| `isEmpty(value)` | Check if value is empty |

### Utils Usage

```javascript
import { debounce, throttle, formatDate } from '../../core/utils.js';

// Debounce search input
const handleSearch = debounce((query) => {
  // Search logic
}, 300);

// Format date
const formatted = formatDate(new Date(), 'en-US');
```

---

## JSON Request

**File:** [`json-request.js`](elements/003-lithium/src/core/json-request.js)

Wrapper around `fetch` for JSON API calls with consistent error handling.

### JSON Features

- Automatic JSON parsing
- JWT token injection
- Error handling
- Request/response logging

### JSON Usage

```javascript
import { jsonRequest } from '../../core/json-request.js';

// GET request
const data = await jsonRequest('/api/data');

// POST request
const result = await jsonRequest('/api/login', {
  method: 'POST',
  body: { username: 'test', password: 'test' }
});
```

### API

```javascript
jsonRequest(url, options)
```

| Option | Type | Description |
|--------|------|-------------|
| `method` | string | HTTP method (GET, POST, etc.) |
| `body` | object | Request body (will be JSON stringified) |
| `headers` | object | Additional headers |
| `auth` | boolean | Whether to include JWT (default: true) |

---

## Log

**File:** [`log.js`](elements/003-lithium/src/core/log.js)

Client-side logging system with structured logging and subsystem tracking.

### Features

- Subsystem-based logging
- Status levels (INFO, WARN, ERROR, SUCCESS)
- Performance timing
- Log archiving

### Subsystems

```javascript
const SUBSYSTEMS = {
  CORE: 'CORE',
  LOGIN: 'LOGIN',
  MAIN: 'MAIN',
  STYLE: 'STYLE',
  // ...
};
```

### Status Levels

```javascript
const Status = {
  INFO: 'INFO',
  WARN: 'WARN',
  ERROR: 'ERROR',
  SUCCESS: 'SUCCESS',
  DEBUG: 'DEBUG'
};
```

### Status Usage

```javascript
import { log, logGroup, Subsystems, Status } from '../../core/log.js';

const MY_MODULE = Subsystems.MY_MODULE;

// Simple log
log(MY_MODULE, Status.INFO, 'Message');

// With timing
const start = performance.now();
// ... operation
log(MY_MODULE, Status.SUCCESS, 'Done', performance.now() - start);

// Log groups
logGroup(MY_MODULE, Status.INFO, 'Operation', () => {
  // grouped logs
});
```

---

## JWT

**File:** [`jwt.js`](elements/003-lithium/src/core/jwt.js)

JWT token management.

### JWT Functions

| Function | Purpose |
|----------|---------|
| `storeJWT(token)` | Store JWT in localStorage |
| `getJWT()` | Retrieve JWT from localStorage |
| `removeJWT()` | Remove JWT from localStorage |
| `getClaims()` | Parse JWT and return claims |
| `isTokenExpired()` | Check if token is expired |

### JWT Usage

```javascript
import { storeJWT, getJWT, getClaims } from '../../core/jwt.js';

// Store token after login
storeJWT(data.token);

// Get token for API calls
const token = getJWT();

// Get decoded claims
const claims = getClaims();
// { user_id: 123, username: 'test', roles: ['admin'], exp: 1234567890 }
```

---

## Permissions

**File:** [`permissions.js`](elements/003-lithium/src/core/permissions.js)

Punchcard-based permission system for manager access control.

### Permissions Functions

| Function | Purpose |
|----------|---------|
| `getPermittedManagers()` | Get list of permitted manager IDs |
| `canAccessManager(id)` | Check if user can access a manager |
| `hasPermission(permission)` | Check specific permission |

### Permissions Usage

```javascript
import { getPermittedManagers, canAccessManager } from '../../core/permissions.js';

// Get all permitted managers
const managers = getPermittedManagers(); // [1, 2, 3]

// Check access
if (canAccessManager(1)) {
  // Show Style Manager
}
```

---

## Event Bus

**File:** [`event-bus.js`](elements/003-lithium/src/core/event-bus.js)

Pub/sub event system for component communication.

### Standard Events

| Event | Data | When |
|-------|------|------|
| `auth:login` | `{ userId, username, roles }` | Login success |
| `auth:logout` | - | User logs out |
| `theme:changed` | `{ theme }` | Theme applied |
| `locale:changed` | `{ lang, previousLang }` | Language changed |
| `manager:loaded` | `{ managerId }` | Manager finished init |
| `network:online` | - | Network connected |
| `network:offline` | - | Network disconnected |

### Events Usage

```javascript
import { eventBus, Events } from '../../core/event-bus.js';

// Listen for events
eventBus.on(Events.AUTH_LOGIN, (e) => {
  console.log('Logged in:', e.detail);
});

// Emit events
eventBus.emit(Events.AUTH_LOGIN, { userId: 123 });

// Listen once
eventBus.once(Events.STARTUP_COMPLETE, () => {
  console.log('Startup complete');
});

// Unsubscribe
const handler = (e) => { ... };
eventBus.on(Events.AUTH_LOGIN, handler);
eventBus.off(Events.AUTH_LOGIN, handler);
```

---

## Shared Modules

**Location:** `src/shared/`

### lookups.js

Manages lookup data from the server.

```javascript
import { getLookup, hasLookup, getLookupCategory } from '../../shared/lookups.js';

// Check if lookup exists
if (hasLookup('themes')) { ... }

// Get specific lookup
const themes = getLookup('themes');

// Get all lookups in category
const icons = getLookupCategory('icons');
```

### languages.js

Internationalization and locale management.

```javascript
import { 
  getLanguageData, 
  getBestGuessLocale, 
  supportedLocales 
} from '../../shared/languages.js';

// Get all supported languages
const languages = getLanguageData();

// Get best guess locale
const locale = await getBestGuessLocale({ ipinfoToken });
```

### log-formatter.js

Formats log entries for display.

```javascript
import { formatLogText, getFlagSvg } from '../../shared/log-formatter.js';

// Format raw log entries
const text = formatLogText(entries);

// Get country flag SVG
const flag = getFlagSvg('US', { width: 22, height: 16 });
```

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager system
- [LITHIUM-ICN.md](LITHIUM-ICN.md) - Icon system
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
