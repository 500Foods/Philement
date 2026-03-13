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
- Error handling with `serverError` enrichment
- Request/response logging
- **Error response body logging** — When a non-2xx response has a JSON body, `json-request.js` logs each key/value as a grouped RESTAPI (Conduit) entry

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
  CONDUIT: 'Conduit',
  RESTAPI: 'Conduit',   // Unified — HTTP and Conduit share the 'Conduit' label
  // ...
};
```

> **Note:** The `RESTAPI` subsystem was renamed from `'RESTAPI'` to `'Conduit'` so that all conduit-related logging (HTTP requests, responses, error bodies) appears under a single subsystem name in the session log.

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

**Note:** JWT documentation has moved to [LITHIUM-JWT.md](LITHIUM-JWT.md) — covering token lifecycle (generation, storage, validation, renewal), claims, and usage examples.

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

### conduit.js

Reusable helper functions for Hydrogen Conduit API queries. Provides both pure payload-building functions (testable without network) and thin API wrappers.

#### Pure Functions

| Function | Purpose |
|----------|---------|
| `buildQueryPayload(queryRef, params)` | Build a single-query payload with typed params |
| `buildBatchPayload(queries, database?)` | Build a batch payload for multi-query endpoints |
| `extractRows(response)` | Extract rows array from a single-query response |
| `extractBatchRows(response)` | Extract a Map of queryRef → rows from batch response |
| `extractError(response)` | Extract error details from a `success: false` response (returns `null` if no error) |

#### API Wrappers

| Function | Purpose |
|----------|---------|
| `authQuery(api, queryRef, params?)` | Execute a single authenticated query (JWT required) |
| `authQueries(api, queries)` | Execute multiple authenticated queries in one request |
| `query(api, queryRef, params?, database?)` | Execute a single public query |
| `queries(api, queryList, database?)` | Execute multiple public queries in one request |

#### Error Enrichment

`authQuery()` catches HTTP errors (non-2xx) from `json-request.js` and enriches them with a `serverError` property extracted from the response body. This means callers always get `error.serverError` when the server returns structured error JSON, regardless of whether the HTTP status was 2xx-with-`success:false` or a 4xx/5xx error:

```javascript
try {
  const rows = await authQuery(app.api, 27, { INTEGER: { QUERY_REF: 27 } });
} catch (error) {
  // error.serverError is always populated when the server returns structured JSON
  // { message, error, queryRef, database }
  console.log(error.serverError.error);   // "Missing required parameters"
  console.log(error.serverError.message); // "Required: QUERYID. Unused Parameters: QUERY_REF"
}
```

#### Conduit Usage

```javascript
import { authQuery, authQueries } from '../../shared/conduit.js';

// Single authenticated query (returns rows array directly)
const rows = await authQuery(app.api, 25);

// With typed parameters
const searchRows = await authQuery(app.api, 32, {
  STRING: { SEARCH: 'USERS' },
});

// Batch authenticated queries (returns Map<queryRef, rows>)
const results = await authQueries(app.api, [
  { queryRef: 25 },
  { queryRef: 32, params: { STRING: { SEARCH: 'X' } } },
]);
const queryList = results.get(25);
```

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

### toast.js

Toast notification system for displaying error, success, warning, and info messages.

#### Toast Features

- **Unified header button group** — Icon, title, subsystem badge, expand, and close in a connected button bar
- **Forced two-part layout** — Every toast has a title and description (defaults to "No additional information provided")
- **Expand button (▼)** — Shows/hides description, keeps the toast from auto-dismissing, rotates 180° when expanded (▲)
- **Subsystem badge** — Indicates which subsystem generated the notification (e.g., Conduit, Auth)
- **Countdown progress bar** — Thin animated bar showing time remaining before auto-dismiss
- **Close (dismiss) button** — Immediately removes the toast
- **Font Awesome icons** — Customizable per toast, with type-appropriate defaults
- **Collapsible detail section** — For extended information (query ref, database, etc.)
- **Type-colored headers** — Button group uses error/success/warning/info colors

#### Font Awesome Animation Note

When animating Font Awesome icons (like the expand chevron rotation), you **must target both `i` and `svg` elements** because Font Awesome's SVG+JS kit replaces `<i>` with `<svg>`:

```css
/* ✅ CORRECT - targets both i and svg */
.toast-header-expand i,
.toast-header-expand svg {
  transition: transform var(--transition-delay, 0.2s) ease;
}

/* ❌ WRONG - only targets i (gets replaced by FA) */
.toast-header-expand i {
  transition: transform 0.2s ease;
}
```

#### Toast Layout

```text
Collapsed (default):
┌──────────────────────────────────────────────────┐
│ │ [icon] Title text [SUBSYSTEM] [▼] [✕]         │
│ ████████████████████░░░░░░░░░░░░ (countdown bar) │
└──────────────────────────────────────────────────┘

Expanded (after clicking ▼):
┌──────────────────────────────────────────────────┐
│ │ [icon] Title text [SUBSYSTEM] [▲] [✕]         │
│   Description text goes here...                   │
│   ┌ Details ────────────────────┐                 │
│   │ Query Ref: 27              │                 │
│   │ Database: Lithium          │                 │
│   └─────────────────────────────┘                 │
│ (countdown bar hidden - toast is kept)            │
└──────────────────────────────────────────────────┘
```

The header uses a unified button group style matching the sidebar-footer and manager headers, with type-specific coloring (red for errors, green for success, yellow for warnings, blue for info).

> **Note:** All toasts use a forced two-part layout with both a title and description. If no description is provided, it defaults to "No additional information provided". This ensures the expand button is always available, allowing users to keep any toast open.

#### Toast Types

| Type | Default Icon | Color |
|------|--------------|-------|
| `error` | `fa-times-circle` | Red |
| `success` | `fa-check-circle` | Green |
| `warning` | `fa-exclamation-triangle` | Yellow |
| `info` | `fa-info-circle` | Blue |

#### Toast API

```javascript
import { toast, TOAST_TYPES } from '../../shared/toast.js';

// Show toast with default options
toast.show('Message');

// Convenience methods by type
toast.error('Error message');
toast.success('Success message');
toast.warning('Warning message');
toast.info('Info message');

// Expand/keep a toast programmatically
const id = toast.show('Important', { duration: 5000, description: 'Details...' });
toast.keep(id);  // Keeps toast open, hides countdown bar

// Dismiss a specific toast
toast.dismiss(id);

// Dismiss all toasts
toast.dismissAll();
```

#### Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `type` | string | `'info'` | Toast type: error, success, warning, info |
| `duration` | number | `5000` | Duration in ms (0 = persistent, no countdown bar) |
| `dismissible` | boolean | `true` | Whether toast can be dismissed |
| `icon` | string | null | Custom Font Awesome icon HTML |
| `title` | string | null | Optional title (uses message if not set) |
| `description` | string | null | Description text shown when expanded |
| `subsystem` | string | null | Subsystem label for badge (e.g., `'Conduit'`, `'Auth'`) |
| `details` | string | null | Details text/HTML for collapsible section |
| `detailsTitle` | string | `'Details'` | Label for details toggle |
| `showDetails` | boolean | `false` | Show description/details expanded by default |
| `forceDescription` | boolean | `true` | Always show description (defaults to "No additional information provided") |

#### Server Error Example

When handling conduit errors, pass the server error object to automatically populate the toast title, description, and details. Note: `json-request.js` already logs the full error response body to the session log, so `toast.error()` does **not** log again (no duplicate).

```javascript
try {
  const rows = await authQuery(app.api, 27, { INTEGER: { QUERY_REF: 27 } });
} catch (error) {
  // authQuery() enriches HTTP errors with error.serverError (from error.data)
  // Toast will:
  // 1. Show red error-colored header button group
  // 2. Show the subsystem badge ("Conduit")
  // 3. Use serverError.error as the toast **title** (e.g., "Missing required parameters")
  // 4. Use serverError.message as the toast **description** (e.g., "Required: QUERYID...")
  // 5. Auto-build details from serverError.query_ref, serverError.database
  // 6. Display countdown progress bar (8 seconds)
  // NOTE: json-request.js logs the full response body — toast does NOT log separately
  toast.error(error.message, {
    serverError: error.serverError,
    subsystem: 'Conduit',
    duration: 8000,
  });
}
```

For an error response like:

```json
{
  "success": false,
  "error": "Missing required parameters",
  "query_ref": 27,
  "database": "Lithium",
  "message": "Required: QUERYID. Unused Parameters: QUERY_REF"
}
```

The toast will display:

- **Title (header):** `"Missing required parameters"` — from `serverError.error`
- **Description (expanded):** `"Required: QUERYID. Unused Parameters: QUERY_REF"` — from `serverError.message`
- **Details (collapsible):** `Query Ref: 27`, `Database: Lithium`
- **Badge:** CONDUIT
- **Countdown bar:** 8-second animated progress bar (hidden after expanding)

The error response body is automatically logged by `json-request.js` (not by toast) to the session log:

```text
15:30:00.000  Conduit     Response 005: POST conduit/auth_query
15:30:00.000  Conduit     ― Code: 400
15:30:00.000  Conduit     ― Time: 168ms
15:30:00.000  Conduit     Response 005: Body
15:30:00.000  Conduit     ― success: false
15:30:00.000  Conduit     ― error: Missing required parameters
15:30:00.000  Conduit     ― query_ref: 27
15:30:00.000  Conduit     ― database: Lithium
15:30:00.000  Conduit     ― message: Required: QUERYID. Unused Parameters: QUERY_REF
```

> **Logging responsibility:** The error response body is logged once at the HTTP layer (`json-request.js`) as a grouped Conduit entry. The toast system does **not** perform any logging — this avoids duplicate entries in the session log.

#### CSS Architecture

Toast styles are in `src/styles/toast.css`. Key design decisions:

- **Unified header button group:** Matches sidebar-footer style with `outline` and connected buttons
- **Type-specific coloring:** Header buttons use error/success/warning/info colors
- **Font Awesome compatibility:** CSS transitions/animations must target BOTH `i` AND `svg` elements (FA replaces `<i>` with `<svg>`)
- **Consistent rounding:** `8px` for the outer toast, `6px` for header buttons
- **Left border indicator:** 4px colored border per type (colorbar)
- **Dark-mode-first:** All colors use CSS variables with dark fallbacks
- **Progress bar animation:** CSS `@keyframes toast-countdown` with `scaleX` transform (GPU-accelerated)
- **Responsive:** Full-width on mobile (`< 480px`)

---

## Related Documentation

- [LITHIUM-JWT.md](LITHIUM-JWT.md) - JWT authentication and claims
- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager system
- [LITHIUM-ICN.md](LITHIUM-ICN.md) - Icon system
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
