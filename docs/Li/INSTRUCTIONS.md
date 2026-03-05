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
├── vite.config.js               # Vite build configuration
├── vitest.config.js             # Vitest configuration
├── eslint.config.js             # ESLint flat config
├── config/
│   └── lithium.json            # Runtime configuration
├── public/                      # Static assets (copied as-is to dist/)
│   ├── manifest.json           # PWA manifest
│   ├── service-worker.js       # PWA service worker
│   ├── assets/                # Fonts, images
│   ├── coverage/              # Test coverage report (deployed to /coverage/)
│   └── src/managers/          # HTML templates for runtime fetch
│       ├── login/login.html
│       ├── main/main.html
│       └── style-manager/style-manager.html
├── src/
│   ├── app.js                 # Bootstrap + manager loader
│   ├── core/                  # Core modules
│   │   ├── event-bus.js       # EventTarget-based event system
│   │   ├── jwt.js            # JWT decode/validate/store
│   │   ├── json-request.js   # HTTP client with auth
│   │   ├── permissions.js    # Punchcard permission system
│   │   ├── config.js         # Runtime config loader
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

This starts Vite dev server on <http://localhost:5173>

### Available Scripts

- `npm run dev`: Start development server with hot reloading
- `npm run build`: Build production version to dist/
- `npm run build:prod`: Production build with minification
- `npm run preview`: Preview production build locally
- `npm run serve`: Serve production build on port 3000
- `npm test`: Run test suite (127 tests)
- `npm run test:coverage`: Run tests with coverage report
- `npm run coverage:copy`: Copy coverage report to public/ for deployment
- `npm run lint`: Run ESLint
- `npm run format`: Format code with Prettier
- `npm run clean`: Clean build artifacts
- `npm run deploy`: Build and deploy to $LITHIUM_DEPLOY (includes coverage if generated)

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

### Manager Registry

Managers are registered in [`app.js`](elements/003-lithium/src/app.js:32):

```javascript
this.managerRegistry = {
  1: { id: 1, name: 'Style Manager', path: '/src/managers/style-manager/index.js' },
  2: { id: 2, name: 'Profile Manager', path: '/src/managers/profile-manager/index.js' },
  3: { id: 3, name: 'Dashboard', path: '/src/managers/dashboard/index.js' },
  4: { id: 4, name: 'Lookups', path: '/src/managers/lookups/index.js' },
  5: { id: 5, name: 'Queries', path: '/src/managers/queries/index.js' },
};
```

### Creating a New Manager

1. Create directory: `src/managers/new-manager/`
2. Create `index.js` with manager class
3. Create `new-manager.html` template
4. Create `new-manager.css` styles
5. Register in `app.js` managerRegistry
6. **Copy HTML template to public folder:**

   ```bash
   mkdir -p public/src/managers/new-manager
   cp src/managers/new-manager/new-manager.html public/src/managers/new-manager/
   ```

> **Important:** HTML templates are fetched at runtime, not bundled by Vite. They must exist in `public/src/managers/{name}/` to be available in production. The `coverage:copy` script handles this automatically for existing managers during deployment.

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
const managers = getPermittedManagers(); // [1, 2, 3, 4, 5]

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

## Configuration

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
2. Builds directly to deployment directory
3. Copies `config/lithium.json` if not present (preserves runtime config)
4. Minifies HTML and service worker

## Common Tasks

### Adding a New Manager

1. Create `src/managers/new-manager/index.js`
2. Create `src/managers/new-manager/new-manager.html`
3. Create `src/managers/new-manager/new-manager.css`
4. Register in `app.js` managerRegistry
5. Add icon/name in `main.js` managerIcons

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

## Troubleshooting

### Module Loading Failures

- Check module path in `app.js` managerRegistry
- Verify module exports default class
- Check browser console for import errors

### HTML Template 404 Errors (Production)

If managers fail to load with 404 errors for `.html` files:

1. Verify templates exist in `public/src/managers/{name}/`
2. Ensure `coverage:copy` ran before deploy (it copies manager templates too)
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
