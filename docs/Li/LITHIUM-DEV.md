# Lithium Development Environment

This document describes the development environment, build tools, and workflow for working with Lithium.

---

## IMPORTANT: Use Session Logging, Not console.log

When debugging issues in Lithium, always use the session logging system instead of `console.log`. This ensures:

- All debug output is preserved for later review
- Debug messages are visible in the browser's Lithium session log panel
- Messages can be filtered by subsystem and status
- Debug info persists across hot reloads during development

### How to Log

```javascript
import { log, Subsystems, Status } from '../core/log.js';

// Basic usage
log(Subsystems.MANAGER, Status.INFO, 'Export button clicked');
log(Subsystems.MANAGER, Status.DEBUG, 'Popup rect:', { width: 140, height: 283 });

// For complex objects, log as a group
log(Subsystems.MANAGER, Status.DEBUG, 'Export options:', { 
  exportOptions, 
  extraOptions 
});
```

### Status Levels

| Status | Purpose |
|--------|---------|
| `Status.DEBUG` | Development debugging info |
| `Status.INFO` | General information |
| `Status.WARN` | Warnings (non-critical issues) |
| `Status.ERROR` | Errors that need attention |
| `Status.SUCCESS` | Successful operations |

### Subsystems

Use the relevant subsystem: `Subsystems.MANAGER`, `Subsystems.TABLE`, `Subsystems.UI`, etc.

---

## Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| Node.js | >= 18.0.0 | LTS recommended |
| npm | >= 9.0.0 | Comes with Node.js |
| Browser | Modern (Chrome, Firefox, Edge, Safari) | For testing |

---

## Installation

```bash
cd elements/003-lithium
npm install
```

This installs all dependencies defined in `package.json`.

---

## Development Server

Start the Vite development server with hot reload:

```bash
npm run dev
```

The app will be available at `http://localhost:3000`.

### With Auto-Login

For faster development, you can auto-login using URL parameters:

```bash
# Example: auto-login with test credentials
http://localhost:3000/?USER=testuser&PASS=usertest
```

This fills and submits the login form automatically.

---

## Available npm Scripts

### Development

| Command | Description |
|---------|-------------|
| `npm run dev` | Start development server with hot reload |
| `npm run build` | Build development version (no minification) |
| `npm run preview` | Preview production build locally |

### Testing

| Command | Description |
|---------|-------------|
| `npm test` | Run all tests (588 tests) |
| `npm run test:watch` | Run tests in watch mode |
| `npm run test:coverage` | Run tests with coverage report |

### Production

| Command | Description |
|---------|-------------|
| `npm run build:prod` | Production build with minification |
| `npm run deploy` | Build, test, and deploy |

### Utilities

| Command | Description |
|---------|-------------|
| `npm run lint` | Run ESLint |
| `npm run format` | Format code with Prettier |
| `npm run clean` | Clean build artifacts |
| `npm run version:bump` | Increment build number |

---

## Build System

### Vite Configuration

Lithium uses [Vite](https://vitejs.dev/) as its build tool. Key configuration files:

| File | Purpose |
|------|---------|
| `vite.config.js` | Main Vite configuration |
| `vitest.config.js` | Vitest testing configuration |
| `eslint.config.js` | ESLint flat config |

### Build Output

Running `npm run build` or `npm run build:prod` produces:

```structure
dist/
├── index.html
├── manifest.json
├── service-worker.js
├── config/lithium.json
├── assets/
│   ├── index-[hash].js
│   ├── index-[hash].css
│   └── ...
└── src/managers/
    └── ... (HTML templates)
```

---

## Environment Variables

Lithium uses these environment variables for deployment:

| Variable | Purpose | Example |
|----------|---------|---------|
| `LITHIUM_ROOT` | Project source directory | `/mnt/extra/Projects/Philement/elements/003-lithium` |
| `LITHIUM_DEPLOY` | Web server document root | `/fvl/tnt/t-philement/lithium` |
| `LITHIUM_DEPLOY_KEEP` | Rollback window for deploy pruning | `3` |

---

## Runtime Configuration

The app reads configuration from `config/lithium.json` at startup:

```json
{
  "server": {
    "url": "https://hydrogen.example.com",
    "api_prefix": "/api"
  },
  "auth": {
    "api_key": "public-app-id",
    "default_database": "Demo_PG"
  },
  "app": {
    "name": "Lithium",
    "version": "1.0.0",
    "default_theme": "dark"
  }
}
```

This file is **not** bundled by Vite — it's fetched at runtime. On first deploy, it's seeded from the source. On subsequent deploys, it's preserved.

---

## Project Structure

```structure
elements/003-lithium/
├── index.html              # Entry point
├── package.json            # Dependencies and scripts
├── vite.config.js          # Vite configuration
├── vitest.config.js        # Vitest configuration
├── eslint.config.js        # ESLint configuration
├── config/
│   └── lithium.json        # Runtime configuration
├── src/
│   ├── app.js              # Bootstrap + manager loader
│   ├── core/               # Core modules
│   ├── managers/           # Feature managers
│   ├── shared/             # Shared utilities
│   └── styles/             # CSS files
├── tests/
│   └── unit/               # Vitest tests
└── public/                 # Static assets
```

---

## Testing Setup

### Framework

Lithium uses:

- **Vitest** — Test runner
- **happy-dom** — DOM implementation (better ESM support than jsdom)

### Running Tests

```bash
# Run all tests
npm test

# Run with coverage
npm run test:coverage

# Watch mode
npm run test:watch
```

### Coverage Dashboard

Generate and view coverage:

```bash
npm run test:coverage
npm run coverage:copy
```

Then open `coverage/index.html` in a browser.

**Live dashboard:** <https://lithium.philement.com/coverage/>

---

## Version System

Lithium uses an auto-incrementing build number independent of git:

| Field | Description |
|-------|-------------|
| `build` | Monotonic integer starting at 1000 |
| `timestamp` | ISO 8601 timestamp |
| `version` | Semantic-style: `0.1.<build>` |

Run `npm run version:bump` to increment the build number.

---

## Code Style

Lithium uses:

- **ESLint** for linting (flat config)
- **Prettier** for formatting (optional)

Run:

```bash
npm run lint
npm run format
```

---

## Key Files Reference

| File | Description |
|------|-------------|
| [`app.js`](elements/003-lithium/src/app.js) | Main bootstrap, manager loader |
| [`event-bus.js`](elements/003-lithium/src/core/event-bus.js) | Event system |
| [`jwt.js`](elements/003-lithium/src/core/jwt.js) | JWT handling |
| [`config.js`](elements/003-lithium/src/core/config.js) | Configuration loader |
| [`permissions.js`](elements/003-lithium/src/core/permissions.js) | Punchcard permissions |
| [`manager-ui.js`](elements/003-lithium/src/core/manager-ui.js) | Shared UI utilities (keyboard shortcuts, footer buttons, font popup) |
| [`manager-edit-helper.js`](elements/003-lithium/src/core/manager-edit-helper.js) | Consolidated edit mode, dirty tracking, and save/cancel button management for all managers |
| [`codemirror-setup.js`](elements/003-lithium/src/core/codemirror-setup.js) | Shared CodeMirror config, `setEditorContentNoHistory()`, `updateUndoRedoButtons()` |

## Shared UI Utilities (`manager-ui.js`)

The `manager-ui.js` module provides common UI functionality used across all managers:

### `createFontPopup(options)`

Creates a reusable font settings popup (size, family, weight). Used by Lookups Manager, Style Manager, and any manager with an editor panel.

```javascript
import { createFontPopup } from '../../core/manager-ui.js';

const { popup, toggle, show, hide, getState, setState } = createFontPopup({
  anchor: this.elements.fontBtn,
  fontSize: 14,
  fontFamily: 'var(--font-mono)',
  fontWeight: 'normal',
  onChange: ({ fontSize, fontFamily, fontWeight }) => {
    // Apply to editor elements
  },
});
```

CSS classes: `.manager-font-popup`, `.manager-font-popup-row`, `.manager-font-popup-input`, `.manager-font-popup-select`, `.manager-font-popup-style-btn` (defined in `manager-ui.css`).

### `setupManagerFooterIcons(group, options)`

Creates Print, Email, Export, and Reports select buttons in the manager footer.

### `setupFooterButtons(slotId, managerId, options)`

Creates the standard footer action icons (Crimson, Notifications, Concierge, Annotations, Tours, Training).

---

## Next Steps

- Learn about JavaScript libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Understand the manager system: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Lookup tables: [LITHIUM-LUT.md](LITHIUM-LUT.md) (includes debugging tips)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
- Troubleshooting: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
