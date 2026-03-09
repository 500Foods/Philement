# Lithium Project Instructions

## Overview

Lithium is a lightweight, performant, modular SPA using vanilla JavaScript ES modules. It features a login system, main menu layout, and multiple manager modules. This document provides comprehensive guidance for working with the Lithium codebase.

## Tech Stack

| Category | Technology |
|----------|------------|
| Language | Vanilla JavaScript (ES modules, async/await, fetch) |
| Build Tool | Vite (dev server, hot reload, production build) |
| Testing | Vitest + happy-dom |
| UI | Custom CSS with CSS variables (dark-mode-first) |
| Tables | Tabulator (dynamic import) |
| Code Editor | CodeMirror 6 |
| Sanitization | DOMPurify |
| Auth | JWT from Hydrogen backend |
| PWA | Service worker + manifest |

## Project Structure

```directory
elements/003-lithium/
├── index.html                    # Entry point (Bootstrap removed)
├── package.json                  # Dependencies and scripts
├── version.json                 # Build number, timestamp, version string
├── vite.config.js               # Vite build configuration
├── vitest.config.js             # Vitest configuration
├── eslint.config.js             # ESLint flat config
├── config/
│   └── lithium.json            # Runtime configuration
├── scripts/
│   ├── bump-version.js         # Auto-increment build on deploy
│   └── copy-templates.js      # Copy manager HTML templates to public/
├── public/                      # Static assets (copied as-is to dist/)
│   ├── manifest.json           # PWA manifest
│   ├── service-worker.js       # PWA service worker (CACHE_VERSION = build)
│   ├── version.json            # Copied from root by bump-version script
│   ├── assets/                # Fonts, images
│   ├── coverage/              # Test coverage report (deployed to /coverage/)
│   └── src/managers/          # HTML templates for runtime fetch
│       ├── login/login.html
│       ├── main/main.html
│       ├── profile-manager/profile-manager.html
│       └── style-manager/style-manager.html
├── src/
│   ├── app.js                 # Bootstrap + manager loader
│   ├── core/                  # Core modules
│   │   ├── event-bus.js       # EventTarget-based event system
│   │   ├── jwt.js            # JWT decode/validate/store
│   │   ├── json-request.js   # HTTP client with auth
│   │   ├── permissions.js    # Punchcard permission system
│   │   ├── config.js         # Runtime config loader
│   │   ├── icons.js          # Font Awesome icon system
│   │   └── utils.js          # Utility functions
│   ├── shared/
│   │   └── lookups.js        # Server-provided reference data
│   ├── styles/               # CSS architecture
│   │   ├── base.css         # Variables, reset, font-face
│   │   ├── layout.css       # Login, sidebar, workspace
│   │   ├── components.css   # Buttons, inputs, tables, modals
│   │   └── transitions.css  # Fade, crossfade, reduced-motion
│   ├── managers/            # Feature modules
│   │   ├── login/          # Login manager
│   │   ├── main/           # Main layout manager
│   │   ├── style-manager/  # Theme management
│   │   ├── profile-manager/# User preferences
│   │   ├── dashboard/      # Dashboard (placeholder)
│   │   ├── lookups/        # Reference data (placeholder)
│   │   └── queries/        # Query builder (placeholder)
│   └── init/               # Third-party library wrappers
└── tests/
    └── unit/               # Vitest unit tests
```

## Development Setup

### Prerequisites

- Node.js >= 18.0.0
- npm >= 9.0.0
- Modern browser (Chrome, Firefox, Edge, Safari)

### Installation

```bash
cd elements/003-lithium
npm install
```

### Start Development Server

```bash
cd elements/003-lithium
npm run dev
```

This starts Vite dev server on <http://localhost:3000>

### Available Scripts

- `npm run dev`: Start development server with hot reloading
- `npm run build`: Build production version to dist/
- `npm run build:prod`: Production build with minification
- `npm run preview`: Preview production build locally
- `npm run serve`: Serve production build on port 3000
- `npm test`: Run test suite (171 tests)
- `npm run test:coverage`: Run tests with coverage report
- `npm run coverage:copy`: Copy coverage report to public/ for deployment
- `npm run templates:copy`: Copy manager HTML templates to public/ for deployment
- `npm run version:bump`: Increment build number, update service worker cache version
- `npm run lint`: Run ESLint
- `npm run format`: Format code with Prettier
- `npm run clean`: Clean build artifacts
- `npm run deploy`: Test + bump version + build + deploy to $LITHIUM_DEPLOY

## Manager System

### What is a Manager?

A **manager** is a user-facing feature set with UI. Each manager:

- Has its own directory under `src/managers/`
- Exports a default class with `init()`, `render()`, and `teardown()` lifecycle
- Is independently loadable via dynamic `import()`
- Receives the app instance and container on construction

### Manager Class Structure

```javascript
// src/managers/example-manager/index.js
export default class ExampleManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.show();
  }

  async render() {
    // Load HTML template
    const response = await fetch('/src/managers/example-manager/example-manager.html');
    const html = await response.text();
    this.container.innerHTML = html;

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#example-page'),
      // ... other elements
    };
  }

  setupEventListeners() {
    // Attach DOM event handlers
  }

  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  teardown() {
    // Clean up event listeners, destroy child components
    this.elements = {};
  }
}
```

### Manager Types

Lithium has two types of managers:

| Type | Description | Registry | Access Control |
|------|-------------|----------|----------------|
| **Menu Manager** | Appears in sidebar navigation menu | `managerRegistry` (numeric IDs) | Punchcard permissions |
| **Utility Manager** | Bound to sidebar footer button | `utilityManagerRegistry` (string keys) | Always available |

### Menu Manager Registry

Menu managers are registered in [`app.js`](elements/003-lithium/src/app.js:32) with numeric IDs:

```javascript
this.managerRegistry = {
  1: { id: 1, name: 'Style Manager', path: '/src/managers/style-manager/index.js' },
  3: { id: 3, name: 'Dashboard', path: '/src/managers/dashboard/index.js' },
  4: { id: 4, name: 'Lookups', path: '/src/managers/lookups/index.js' },
  5: { id: 5, name: 'Queries', path: '/src/managers/queries/index.js' },
};
```

**Note:** Profile Manager was converted to a utility manager and removed from
the menu registry. It is now accessed via the sidebar footer profile button.

### Utility Manager Registry

Utility managers are registered in [`app.js`](elements/003-lithium/src/app.js:40) with string keys:

```javascript
this.utilityManagerRegistry = {
  'session-log': { key: 'session-log', name: 'Session Log' },
  'user-profile': { key: 'user-profile', name: 'User Profile' },
};
```

Utility managers:

- Are always available regardless of Punchcard permissions
- Load into the workspace like menu managers but don't appear in the sidebar menu
- Have active state highlighting on their sidebar footer button
- Mutually exclusive with menu managers (selecting one deselects the other)

### Creating a New Manager

#### Menu Manager (sidebar navigation)

1. Create directory: `src/managers/new-manager/`
2. Create `index.js` with manager class
3. Create `new-manager.html` template
4. Create `new-manager.css` styles
5. Register in `app.js` `managerRegistry` with numeric ID
6. Add icon/name in `main.js` `managerIcons`
7. HTML templates are copied automatically on deploy

#### Utility Manager (sidebar footer)

1. Create directory: `src/managers/new-manager/`
2. Create `index.js` with manager class
3. Create `new-manager.html` template
4. Create `new-manager.css` styles
5. Register in `app.js`:
   - Add to `utilityManagerRegistry` with string key
   - Add import case in `_importUtilityManager()`
6. Add button to `main.html` sidebar footer
7. Wire button click in `main.js` to call `app.showUtilityManager()`
8. HTML templates are copied automatically on deploy

The `templates:copy` script runs automatically during `npm run deploy` to copy
all manager HTML templates from `src/managers/` to `public/src/managers/`.

## Event Bus

Lithium uses an EventTarget-based event bus for cross-module communication.

### Usage

```javascript
import { eventBus, Events } from '../../core/event-bus.js';

// Emit an event
eventBus.emit(Events.AUTH_LOGIN, { userId: 123, username: 'john' });

// Listen for an event
eventBus.on(Events.AUTH_LOGIN, (event) => {
  console.log('User logged in:', event.detail);
});

// Listen once
eventBus.once(Events.THEME_CHANGED, (event) => {
  console.log('Theme changed:', event.detail);
});

// Remove listener
eventBus.off(Events.AUTH_LOGOUT, myHandler);
```

### Standard Events

| Event | Payload | Description |
|-------|---------|-------------|
| `auth:login` | `{ userId, username, roles }` | Login success |
| `auth:logout` | `{}` | User logs out |
| `auth:expired` | `{}` | JWT expired, renewal failed |
| `auth:renewed` | `{ expiresAt }` | JWT renewed |
| `theme:changed` | `{ themeId, themeName, vars }` | Theme applied |
| `locale:changed` | `{ lang, formats }` | Language changed |
| `manager:loaded` | `{ managerId }` | Manager finished init |
| `manager:switched` | `{ from, to }` | Active manager changed |
| `network:online` | `{}` | Connection restored |
| `network:offline` | `{}` | Connection lost |

## CSS Architecture

Lithium uses CSS variables for theming. All styles are in `src/styles/`:

### Variable Categories

- `--bg-*`: Background colors (primary, secondary, tertiary)
- `--text-*`: Text colors (primary, secondary, muted)
- `--border-*`: Border colors and widths
- `--accent-*`: Accent colors (primary, secondary, success, danger, warning)
- `--font-*`: Font families and sizes
- `--space-*`: Spacing units
- `--shadow-*`: Box shadows
- `--transition-*`: Transition timings
- `--scrollbar-*`: Custom scrollbar styling

### Dynamic Theming

The Style Manager injects CSS variables via a dynamic style element:

```javascript
// Find or create dynamic style element
let styleEl = document.getElementById('dynamic-theme');
if (!styleEl) {
  styleEl = document.createElement('style');
  styleEl.id = 'dynamic-theme';
  document.head.appendChild(styleEl);
}

// Inject CSS variables
styleEl.textContent = ':root { --bg-primary: #0D1117; }';
```

### Button Group Design Pattern

Lithium uses a consistent button group pattern for related actions and panel headers. This design features:

- **Small gaps** (2px) between buttons
- **Connected appearance** with shared height and alignment
- **Rounded corners** on first and last buttons
- **Consistent styling** using accent colors

#### Example: Login Button Group

```html
<div class="login-btn-group">
  <button type="button" class="login-btn-icon" id="login-language-btn">
    <fa fa-globe></fa>
  </button>
  <button type="button" class="login-btn-icon" id="login-theme-btn">
    <fa fa-palette></fa>
  </button>
  <button type="submit" class="login-btn-primary" id="login-submit">
    <fa fa-sign-in-alt></fa>
    <span>Login</span>
  </button>
  <button type="button" class="login-btn-icon" id="login-logs-btn">
    <fa fa-scroll></fa>
  </button>
  <button type="button" class="login-btn-icon" id="login-help-btn">
    <fa fa-circle-question></fa>
  </button>
</div>
```

#### Example: Panel Header Button Group

```html
<div class="subpanel-header-group">
  <button type="button" class="subpanel-header-btn subpanel-header-primary">
    <fa fa-palette></fa>
    <span>Select Theme</span>
  </button>
  <button type="button" class="subpanel-header-btn subpanel-header-close" id="theme-close-btn">
    <fa fa-xmark></fa>
  </button>
</div>
```

**CSS Classes:**

| Class | Purpose |
|-------|---------|
| `.login-btn-group` | Container for login page buttons |
| `.login-btn-icon` | Icon-only buttons in the group |
| `.login-btn-primary` | Main action button (flex: 1) |
| `.subpanel-header-group` | Container for panel header buttons |
| `.subpanel-header-primary` | Icon + text title button (flex: 1) |
| `.subpanel-header-close` | Close button (fixed width) |

**When to use this pattern:**

- Grouping related actions (login buttons, panel controls)
- Panel headers with icon + title and close button
- Anywhere you need visually connected buttons with a cohesive look

### Manager Slot Button Injection

Each manager slot (rendered by `MainManager`) has a single unified
`subpanel-header-group` for both its header and its footer.  Managers must
**not** create a separate toolbar element inside their workspace — instead they
inject buttons into the slot's existing group via the `MainManager` API.  This
keeps every button strip seamlessly connected with no visual breaks.

#### Slot Header

```layout
[icon + name (flex:1)]  [manager extras here]  [close ✕]
```

To add buttons between the title and close:

```javascript
// Inside manager init() — called after the slot is created by app.js
_injectSlotHeaderButtons() {
  const mainMgr = this.app._getMainManager();
  if (!mainMgr) return;

  // Menu manager:   mainMgr._slotId(managerId)
  // Utility manager: mainMgr._utilitySlotId(managerKey)
  const slotId = mainMgr._utilitySlotId('my-manager');

  mainMgr.addHeaderButtons(slotId, [
    {
      id:      'my-refresh-btn',
      icon:    'fa-rotate',
      title:   'Refresh',
      tooltip: 'Refresh',
      onClick: () => this.refresh(),
    },
    // More buttons…
  ]);
}
```

#### Slot Footer

```layout
[Reports (flex:1)]  [left extras]  [right extras]  [Annotations][Tours][LMS]
```

To add buttons to the footer:

```javascript
mainMgr.addFooterButtons(slotId, 'right', [
  { id: 'export-btn', icon: 'fa-file-export', title: 'Export', onClick: fn },
]);
// side = 'left'  → between Reports and right extras
// side = 'right' → between left extras and the fixed action icons
```

#### Button Descriptor Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `icon` | string | Yes* | FA icon attribute, e.g. `'fa-rotate'` |
| `id` | string | No | DOM `id` for later `querySelector` access |
| `title` | string | No | HTML `title` attribute (tooltip on hover) |
| `tooltip` | string | No | `data-tooltip` attribute |
| `onClick` | function | No | Click event handler |
| `el` | HTMLElement | Yes* | Pre-built element (alternative to `icon`) |

\* Either `icon` or `el` must be supplied.

**Reference:** See `SessionLogManager._injectSlotHeaderButtons()` for the
canonical example of this pattern in practice.

### Transition Overlay System

When switching managers (menu or utility), a transparent overlay blocks user
interaction during the transition to prevent race conditions from rapid clicking:

```javascript
// app.js creates overlay during manager switches
const overlay = this.getTransitionOverlay();
overlay.style.pointerEvents = 'auto';  // Block clicks
// ... perform manager load ...
overlay.style.pointerEvents = 'none';  // Allow clicks
```

**Key characteristics:**

- **Full viewport coverage:** Uses `position: fixed` with `z-index: 9999` to cover both sidebar and workspace
- **Pointer-events toggle:** Overlay is always present but switches between `none` (transparent to clicks) and `auto` (blocking)
- **Automatic management:** Created on demand in `app.js`, reused across transitions
- **No visual appearance:** Completely transparent, purely for interaction blocking

This prevents users from clicking sidebar buttons while a manager is still loading,
which could cause state corruption or double-loading issues.

### Utility Manager Active State

Utility manager buttons in the sidebar footer show an active state when their
manager is loaded:

| Button | Active Class | Visual |
|--------|-------------|--------|
| Profile | `.active` | Accent-secondary background, inset shadow |
| Session Log | `.active` | Accent-secondary background, inset shadow |

**Implementation in `main.js`:**

```javascript
// Set active state when loading utility manager
setActiveUtilityButton(key) {
  this.clearActiveMenuItem();        // Deselect any menu item
  this.clearActiveUtilityButtons();  // Clear other utility buttons
  this.currentUtilityKey = key;
  this.currentManagerId = null;      // Reset menu manager state
  
  const btn = document.getElementById(`sidebar-${key}-btn`);
  btn?.classList.add('active');
}

// Clear when loading a menu manager
loadManager(managerId) {
  this.clearActiveUtilityButtons();  // Deselect utility buttons
  this.currentUtilityKey = null;
  // ... load menu manager
}
```

**State management principles:**

- **Mutual exclusion:** Only one of {menu manager, utility manager} can be active
- **State tracking:** `currentManagerId` (numeric) vs `currentUtilityKey` (string)
- **Visual consistency:** Both menu items and utility buttons use the same `.active` class styling
- **State reset:** Loading one type automatically clears the other's active state

## Icon System

Lithium uses a custom icon system built on top of Font Awesome. Instead of using `<i class="fas fa-icon">` directly, use the `<fa>` tag and let the system apply your configured icon set.

### Why `<fa>` Tags?

The `<fa>` tag provides:

- **Consistent icon styling** across the application
- **Easy theme switching** between icon sets (solid, duotone, thin, etc.)
- **Server-side icon overrides** via lookups (QueryRef 054)
- **Automatic SVG conversion** via Font Awesome's mutation observer

### Configuration

Icon configuration is stored in `config/lithium.json`:

```json
{
  "icons": {
    "set": "duotone",
    "weight": "thin",
    "prefix": "fad",
    "fallback": "solid"
  }
}
```

| Option | Values | Description |
|--------|--------|-------------|
| `set` | solid, regular, light, thin, duotone, sharp-* | The icon set family |
| `weight` | thin, light, regular | Weight for duotone/sharp sets |
| `prefix` | fas, far, fal, fat, fad, fab | Explicit prefix (optional) |
| `fallback` | solid | Fallback set if requested unavailable |

### Using `<fa>` Tags in HTML

**Direct icon reference:**

```html
<!-- In your HTML templates -->
<fa fa-user></fa>
<fa fa-cog fa-fw></fa>
<fa fa-trash fa-spin></fa>
```

**Lookup-based icons (from QueryRef 054):**

```html
<!-- "Status" is a lookup key that maps to an icon -->
<fa Status></fa>
```

Lookup entries have this format:

```json
{
  "lookup_value": "Status",
  "value_txt": "<i class='fa fa-fw fa-flag'></i>"
}
```

### Preserved Utility Classes

The following classes are preserved when processing `<fa>` tags:

- **Size:** `fa-xs`, `fa-sm`, `fa-lg`, `fa-xl`, `fa-2x` through `fa-10x`
- **Animation:** `fa-spin`, `fa-pulse`, `fa-fade`, `fa-beat`, `fa-bounce`
- **Layout:** `fa-fw`, `fa-border`, `fa-pull-left`, `fa-pull-right`
- **Rotation:** `fa-rotate-90`, `fa-rotate-180`, `fa-rotate-270`

### Programmatic Icon Manipulation

```javascript
import { setIcon, createIcon, processIcons } from './core/icons.js';

// Set an icon on an existing element
const element = document.getElementById('my-icon');
setIcon(element, 'user', { set: 'solid', classes: ['fa-fw'] });
// Result: <i class="fas fa-user fa-fw"></i>

// Create a new icon element
const icon = createIcon('cog', { set: 'duotone', weight: 'thin' });
// Result: <i class="fad fa-thin fa-cog"></i>

// Process dynamically added <fa> elements
processIcons(container);
```

### JavaScript Dynamic Icons

For dynamic icon changes in JavaScript (e.g., toggling a chevron):

```javascript
import { setIcon } from './core/icons.js';

// Instead of direct class manipulation:
// icon.className = 'fas fa-chevron-right';

// Use setIcon for consistency:
setIcon(icon, 'chevron-right');
setIcon(icon, 'chevron-left');
```

### Migration from `<i>` to `<fa>`

To migrate existing code:

```html
<!-- Before -->
<i class="fas fa-user"></i>
<i class="fas fa-cog fa-fw"></i>

<!-- After -->
<fa fa-user></fa>
<fa fa-cog fa-fw></fa>
```

## Authentication (JWT)

### JWT Flow

1. User enters credentials on login screen
2. POST to `/api/auth/login` with `{ login_id, password, api_key, tz, database }`
3. Server returns `{ token, expires_at, user_id, roles }`
4. Client stores JWT in localStorage
5. Attach to requests: `Authorization: Bearer <token>`
6. Auto-renew at 80% of token lifetime

### JWT Methods

```javascript
import { storeJWT, retrieveJWT, validateJWT, clearJWT, getClaims } from './core/jwt.js';

// Store JWT
storeJWT(tokenString);

// Retrieve JWT
const token = retrieveJWT();

// Validate JWT (checks expiry with 60s clock skew)
const result = validateJWT(token);
// result = { valid: true, claims: {...} } or { valid: false, error: '...' }

// Get claims without validation
const claims = getClaims();
// claims = { user_id, username, email, roles, punchcard, ... }

// Clear JWT
clearJWT();
```

## Permissions (Punchcard)

Permissions are controlled via a "punchcard" JWT claim:

```json
{
  "punchcard": {
    "managers": [1, 2, 5],
    "features": {
      "1": [1, 2, 3],
      "2": [1]
    }
  }
}
```

### Permission Helper Functions

```javascript
import { canAccessManager, hasFeature, getPermittedManagers, getFeaturesForManager } from './core/permissions.js';

// Check if user can access a manager
const canAccess = canAccessManager(1); // true/false

// Check if user has a specific feature
const canEdit = hasFeature(1, 2); // Style Manager, Feature 2 (Edit)

// Get all permitted manager IDs
const managers = getPermittedManagers(); // [1, 3, 4, 5]

// Get features for a manager
const features = getFeaturesForManager(1); // [1, 2, 3, 4, 5]
```

### Feature IDs (Style Manager)

| Feature ID | Description |
|------------|-------------|
| 1 | View all styles (read-only) |
| 2 | Edit all styles |
| 3 | Use visual editor |
| 4 | Use CSS code editor |
| 5 | Delete themes |

## Runtime Configuration

Runtime configuration is stored in `config/lithium.json`:

```json
{
  "server": { "url": "https://hydrogen.example.com", "api_prefix": "/api" },
  "auth": { "api_key": "public-app-id", "default_database": "Demo_PG" },
  "app": { "name": "Lithium", "version": "1.0.0", "default_theme": "dark" },
  "features": { "offline_mode": true, "debug_logging": false }
}
```

### Using Config

```javascript
import { loadConfig, getConfig, getConfigValue } from './core/config.js';

// Load config (called automatically by app.js)
await loadConfig();

// Get full config
const config = getConfig();

// Get nested value with default
const url = getConfigValue('server.url', 'http://localhost:8080');
```

## Testing

### Running Tests

```bash
npm test              # Run all tests
npm run test:coverage # Run with coverage report
npm run test:watch   # Watch mode
npm run coverage:copy # Copy coverage to public/ for deployment
```

### Test Coverage Dashboard

The test coverage report is automatically deployed alongside the application:

**Live Dashboard:** <https://lithium.philement.com/coverage/>

The coverage report includes:

- Interactive file tree with coverage percentages
- Line-by-line highlighting (green = covered, red = uncovered)
- Summary statistics (statements, branches, functions, lines)
- Sortable tables and file search
- Drill-down into individual source files

**To deploy with coverage:**

```bash
npm run test:coverage  # Generate coverage report
npm run deploy         # Coverage automatically included
```

### Current Coverage

| Module | Coverage |
|--------|----------|
| event-bus.js | 100% |
| config.js | 100% |
| permissions.js | 96% |
| utils.js | 88% |
| icons.js | 85% |
| jwt.js | 75% |
| json-request.js | 40% |
| lookups.js | 37% |

### Writing Tests

```javascript
import { describe, it, expect, vi, beforeEach } from 'vitest';

describe('ModuleName', () => {
  beforeEach(() => {
    // Setup
  });

  it('should do something', () => {
    expect(true).toBe(true);
  });
});
```

## Third-Party Libraries

### Dynamic Imports

Heavy dependencies are dynamically imported to enable code splitting:

```javascript
// Tabulator
const tabulatorModule = await import('tabulator-tables');
const Tabulator = tabulatorModule.default;

// DOMPurify
const dompurifyModule = await import('dompurify');
const DOMPurify = dompurifyModule.default;

// CodeMirror
const { EditorView } = await import('@codemirror/view');
const { css } = await import('@codemirror/lang-css');
```

### Available Libraries

| Library | Purpose | Import Type |
|---------|---------|-------------|
| tabulator-tables | Data tables | Dynamic |
| dompurify | HTML/CSS sanitization | Dynamic |
| @codemirror/* | Code editor | Dynamic |
| Font Awesome | Icons | CDN |
| Vanadium Sans/Mono | Fonts | Local WOFF2 |

## Version System

Lithium uses an auto-incrementing build number system independent of git hashes.

### version.json

Located at the project root, `version.json` tracks:

```json
{
  "build": 1000,
  "timestamp": "2026-03-06T22:19:00-08:00",
  "version": "0.1.1000"
}
```

| Field | Description |
|-------|-------------|
| `build` | Monotonic integer starting at 1000, incremented on each deploy |
| `timestamp` | ISO 8601 timestamp of when the build was created |
| `version` | Semantic-style version string: `0.1.<build>` |

### How It Works

1. `npm run version:bump` runs `scripts/bump-version.js`
2. The script reads `version.json`, increments `build`, updates `timestamp`
3. Writes updated `version.json` back to project root
4. Copies `version.json` to `public/` (so it's available at runtime via `fetch('/version.json')`)
5. Updates `CACHE_VERSION` in `public/service-worker.js` to match the build number

### Where Version Is Displayed

- **Login screen header** — "Build 1001" below "A Philement Project"
- **Help panel** — Version string and build date
- **`app.getState()`** — Returns `{ version, build, ... }` for debugging

### Cache Busting

The service worker uses `CACHE_VERSION` in its cache names (`lithium-static-v1001`, `lithium-api-v1001`). When the build number changes, the old caches are automatically cleaned up on the `activate` event. Vite-bundled JS/CSS files already have content hashes in their filenames (`index-[hash].js`), so they are cache-busted by default.

### Import Rules

Core modules (`jwt.js`, `config.js`, `event-bus.js`, `permissions.js`, etc.) should always be **statically imported** at the top of files that need them. Dynamic `import()` should only be used for:

- **Managers** — Lazy-loaded on demand (`import('./managers/login/login.js')`)
- **Heavy third-party libraries** — Tabulator, CodeMirror, DOMPurify

Mixing static and dynamic imports of the same module causes Vite warnings and prevents code splitting. If a module is already statically imported, use the static reference everywhere in that file.

## PWA Features

### Service Worker

The service worker (`public/service-worker.js`) implements:

- Cache-first strategy for static assets
- Stale-while-revalidate for API calls
- Versioned cache names with cleanup on activate

### Manifest

`public/manifest.json` configures:

- App name: "Lithium"
- Theme color: dark
- Display: standalone
- Icons: SVG + PNG (192x192, 512x512)

### Offline Support

- Assets cached automatically
- Network status monitored via `navigator.onLine`
- Events: `network:online`, `network:offline`

## Deployment

### Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `LITHIUM_ROOT` | Project source | `/mnt/extra/Projects/Philement/elements/003-lithium` |
| `LITHIUM_DEPLOY` | Web server root | `/fvl/tnt/t-philement/lithium` |

### Deploy Flow

```bash
npm run deploy
```

1. Validates `$LITHIUM_DEPLOY` is set
2. Runs full test suite with coverage report
3. Copies coverage report to `public/` for deployment
4. Copies manager HTML templates to `public/` for deployment
5. Bumps build number in `version.json`, updates service worker `CACHE_VERSION`
6. Builds directly to deployment directory
7. Copies `config/lithium.json` if not present (preserves runtime config)
8. Minifies HTML and service worker

## Common Tasks

### Adding a New Menu Manager

1. Create `src/managers/new-manager/index.js`
2. Create `src/managers/new-manager/new-manager.html`
3. Create `src/managers/new-manager/new-manager.css`
4. Register in `app.js` `managerRegistry` with numeric ID
5. Add icon/name in `main.js` `managerIcons`

### Adding a New Utility Manager

1. Create `src/managers/new-manager/index.js`
2. Create `src/managers/new-manager/new-manager.html`
3. Create `src/managers/new-manager/new-manager.css`
4. Register in `app.js`:
   - Add entry to `utilityManagerRegistry` with string key
   - Add case in `_importUtilityManager()` method
5. Add button to sidebar footer in `main.html`
6. Wire button click in `main.js` to call `app.showUtilityManager('key')`

### Adding a New Core Module

1. Create `src/core/module-name.js`
2. Export functions/classes
3. Import in managers as needed
4. Add tests in `tests/unit/module-name.test.js`

### Using the Event Bus

```javascript
// In a manager
import { eventBus, Events } from '../../core/event-bus.js';

// Emit custom event
eventBus.emit('my:event', { data: 'value' });

// Listen
eventBus.on('my:event', (e) => console.log(e.detail));
```

### Sanitizing User Input

```javascript
import DOMPurify from 'dompurify';

const clean = DOMPurify.sanitize(dirtyHtml, {
  ALLOWED_TAGS: ['b', 'i', 'em', 'strong'],
  ALLOWED_ATTR: ['class'],
});
```

## Logging System

Lithium's client-side action log (`src/core/log.js`) tracks every significant
event from startup through the session. It is designed to be:

- **Non-blocking** — `log()` returns immediately; all I/O is fire-and-forget
- **Persistent** — entries survive dynamic `import()` via `sessionStorage`
- **Archivable** — previous-session entries are moved to `localStorage` for
  future server upload
- **Display-friendly** — entries can be plain strings or grouped objects that
  render as multi-line visual blocks

### Using the Log API

Import the helpers you need from `src/core/log.js`:

```javascript
import {
  log, logGroup,
  logStartup, logAuth, logHttp, logManager,
  logError, logWarn, logSuccess,
  Subsystems, Status,
} from '../../core/log.js';
```

#### Simple log entry

```javascript
logStartup('Application initializing');
logAuth(Status.SUCCESS, 'Login successful for user alice', 142);
logHttp('GET /api/lookups - 200, 8192 bytes', 76);
log(Subsystems.MANAGER, Status.ERROR, 'Failed to load StyleManager: 404');
```

#### Grouped log entry (`logGroup`)

Use `logGroup` when a single logical event has multiple related detail items.
Stored as **one** buffer entry (one counter, one timestamp), displayed as multiple
visual lines:

```javascript
logGroup(Subsystems.STARTUP, Status.INFO, 'Browser Environment', [
  `Browser: Firefox 148`,
  `Platform: Linux x86_64`,
  `Language: en-US`,
  `Online: ${navigator.onLine}`,
]);
```

Displayed as:

```log
12:31:52.936  Startup  Browser Environment
12:31:52.936  Startup  ― Browser: Firefox 148
12:31:52.936  Startup  ― Platform: Linux x86_64
12:31:52.936  Startup  ― Language: en-US
12:31:52.936  Startup  ― Online: true
```

**Use grouped entries when:**

- Multiple items share the same timestamp and are logically one event
- You want log storage efficiency (one counter for many display lines)
- The relationship between items is obvious from the title

**Use individual `log()` calls when:**

- Items arrive at different times (e.g., async operations)
- Each item has independent significance or needs a distinct counter

### Subsystem Constants

```javascript
Subsystems.STARTUP    // App bootstrap
Subsystems.AUTH       // JWT / authentication
Subsystems.HTTP       // Fetch / REST calls
Subsystems.MANAGER    // Manager loading
Subsystems.LOOKUPS    // Server reference data
Subsystems.ICONS      // Icon system
Subsystems.PERMS      // Punchcard permissions
Subsystems.THEME      // Theme application
// JWT, EVENTBUS, CONFIG, SESSION also available
```

### Status Constants

```javascript
Status.INFO     // Informational (default for most helpers)
Status.SUCCESS  // Operation completed successfully
Status.WARN     // Non-fatal warning
Status.ERROR    // Error (with recovery)
Status.FAIL     // Fatal / unrecoverable
Status.DEBUG    // Verbose debug info (filtered in production)
```

### Logging Coverage Policy

Every module that makes network calls or initializes a subsystem **must** log:

1. **Before** the operation — what and where
2. **After** success — duration and size/count where available
3. **On failure** — status `WARN` or `ERROR` + error message

Do not emit bare `console.log` / `console.warn` in application code.
Route through `log()` so events appear in the System Logs panel.
Modules that need a `console.*`-compatible interface (e.g., `lookups.js`)
should use a local `logger` object that forwards to both:

```javascript
// lookups.js pattern
const logger = {
  info:  (msg) => { console.log(`[Lookups] ${msg}`);   log(Subsystems.LOOKUPS, Status.INFO,  msg); },
  warn:  (msg) => { console.warn(`[Lookups] ${msg}`);  log(Subsystems.LOOKUPS, Status.WARN,  msg); },
  error: (msg) => { console.error(`[Lookups] ${msg}`); log(Subsystems.LOOKUPS, Status.ERROR, msg); },
};
```

### System Logs Panel (Login page)

The `fa-scroll` button on the login page opens a full-featured log viewer powered
by CodeMirror 6 (`oneDark` theme, `Vanadium Mono` font, 4-digit zero-padded line
numbers, read-only).

The panel header bar contains:

| Button | Icon | Action |
|--------|------|--------|
| Title | `fa-scroll` System Logs | Non-interactive label |
| Coverage | `fa-chart-line` | Opens `/coverage/index.html` in a new tab |
| Close | `fa-xmark` | Returns to main login panel (or press ESC) |

Grouped `{ title, items }` descriptions are expanded at display time:
the title and each item share the same `HH:MM:SS.ZZZ  Subsystem` prefix,
with '― ' prepended to each item.

### Accessing Logs Programmatically

```javascript
// Browser console during development
window.lithiumLogs.print(50);        // Print last 50 entries
window.lithiumLogs.raw;              // Array of raw JSON entries
window.lithiumLogs.display;          // Array of formatted strings
window.lithiumLogs.recent(20);       // Last 20 formatted strings
window.lithiumLogs.count;            // Current counter value
window.lithiumLogs.sessionId;        // Current session ID
window.lithiumLogs.archived;         // Previous-session archives
window.lithiumLogs.flush();          // Force sync to server
window.lithiumLogs.setConsoleLogging(true); // Enable console passthrough
```

---

## Production Deployment

### Deployment Checklist

Before deploying to production:

1. **Run tests with coverage:**

   ```bash
   npm run test:coverage
   ```

2. **Verify HTML templates are in public/:**
   - `public/src/managers/login/login.html`
   - `public/src/managers/main/main.html`
   - `public/src/managers/style-manager/style-manager.html`

   > **Note:** Managers fetch HTML templates at runtime. Vite only bundles JS/CSS, so templates must be copied to `public/` to be available in production.

3. **Deploy:**

   ```bash
   npm run deploy
   ```

### Coverage Dashboard Deployment

The test coverage dashboard is automatically deployed when you run:

```bash
npm run test:coverage  # Generate coverage report
npm run deploy         # Coverage copied to public/ and deployed
```

Access the dashboard at: `https://lithium.philement.com/coverage/`

## Login Panel Features

### Transition Animations

The login panel supports smooth crossfade transitions:

**Login ↔ Subpanels:** When switching between the main login panel and subpanels (Language, Theme, Logs, Help), a crossfade animation plays with both panels visible simultaneously during the transition.

**Login → Main:** After successful login, the login panel crossfades to the main interface.

**Logout:** Clicking logout fades out the main interface, then reloads the page for a clean state.

### Transition System Implementation (March 2025 Fix)

The transition system was refactored to fix double-fade-in issues and focus race conditions:

**Issues Fixed:**

1. **Double-fade-in on login page** - Removed `autofocus` attribute from username input to prevent browser focus conflicts with CSS transitions
2. **Double-fade-in when logging in** - Simplified `hide()` to fade only the panel, letting `transitionToMainManager()` handle the crossfade cleanly
3. **Focus switching back to username after logout** - Consolidated focus logic into `show()` method with proper timing

**Key Implementation Details:**

- **Focus Management:** The `show()` method accepts a `skipUsernameFocus` parameter. When `true` (username was pre-filled), it focuses the password field instead of username after the transition completes.

- **Username Loading:** `loadRememberedUsername()` now returns a boolean indicating whether a username was loaded, which is passed to `show()` to determine proper focus.

- **Transition Timing:** Focus changes occur after `getTransitionDuration() + 50ms` to ensure CSS transitions complete and elements are fully interactive.

- **No Duplicate Animations:** Login panel fade-out and main manager crossfade are coordinated so they don't overlap. The `hide()` method only fades the current panel, not the entire page.

**Files Modified:**

- `src/managers/login/login.html` - Removed `autofocus` attribute
- `src/managers/login/login.js` - Fixed focus race condition, simplified `hide()`
- `src/app.js` - Cleaned up `transitionToMainManager()` timing

### Username Remembering

The login panel automatically remembers the last used username:

- On successful login, the username is saved to localStorage
- On return visits, the username field is pre-filled
- If a username is present, focus automatically moves to the password field
- Storage key: `lithium_last_username`

### Lookup-Enabled Buttons

The login panel buttons are enabled based on lookup data availability:

| Button | Lookup Required | Event |
|--------|----------------|-------|
| Language | `lookup_names` | `lookups:lookup_names:loaded` |
| Theme | `themes` | `lookups:themes:loaded` |
| Logs | `system_info` | `lookups:system_info:loaded` |
| Help | (always enabled) | - |

Buttons start disabled and enable when their respective lookups are loaded from cache or server.

### Clear Button

The X button next to the username field:

- Clears both username and password fields
- Focuses the username field for immediate typing
- Hides any error messages

### CAPS LOCK Detection

The password field shows a yellow background when CAPS LOCK is active, helping users avoid accidental uppercase input.

## Development Auto-Login

For faster development workflow, you can auto-login by passing credentials as URL query parameters:

```url
http://localhost:3000/?USER=testuser&PASS=usertest
```

This feature automatically fills in the login form and submits it, allowing you to bypass the manual login step during development. The credentials are read from the URL parameters:

- `USER` - The username/login_id
- `PASS` - The password

**Example with environment variables:**

```bash
# If you have HYDROGEN_DEMO_USER_NAME and HYDROGEN_DEMO_USER_PASS set
open "http://localhost:3000/?USER=$HYDROGEN_DEMO_USER_NAME&PASS=$HYDROGEN_DEMO_USER_PASS"
```

**Note:** This feature is intended for development use only. In production, always use the manual login form.

## Troubleshooting

### Module Loading Failures

- Check module path in `app.js` managerRegistry
- Verify module exports default class
- Check browser console for import errors

### HTML Template 404 Errors (Production)

If managers fail to load with 404 errors for `.html` files:

1. Verify templates exist in `public/src/managers/{name}/`
2. Run `npm run templates:copy` to copy templates to public/
3. Check that `public/` is copied to deployment directory

### Font Awesome Icons Not Loading

If icons appear as squares or don't load:

- The Font Awesome Kit URL may have changed
- Update in `index.html`: `<script src="https://kit.fontawesome.com/YOUR_KIT_CODE.js" crossorigin="anonymous"></script>`
- Kit-based loading is more reliable than CDN with SRI hashes

### Authentication Problems

- Check JWT storage (localStorage)
- Verify token hasn't expired
- Test login/logout flow

### Build Errors

- Run `npm install`
- Check Vite configuration
- Verify ES module syntax

### Vite "Dynamically Imported by ... but Also Statically Imported" Warning

This warning means a module is both `import`-ed at the top of a file and `await import()`-ed elsewhere in the same file. The dynamic import cannot create a separate chunk because the static import already includes the module in the main bundle.

**Fix:** Add all needed exports to the static import and remove the dynamic `import()` calls. Only use dynamic `import()` for lazy-loaded managers and heavy third-party libraries.

### PWA Issues

- Check service worker registration
- Verify manifest configuration
- Test offline functionality

## Additional Resources

- [Vite Documentation](https://vitejs.dev/)
- [ES6 Modules Guide](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Modules)
- [PWA Guide](https://web.dev/progressive-web-apps/)
- [JWT Specification](https://jwt.io/)
- [Tabulator Documentation](https://tabulator.info/)
- [CodeMirror Documentation](https://codemirror.net/)
