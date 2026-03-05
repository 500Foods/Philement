# Lithium Blueprint

Comprehensive architectural blueprint for building the Lithium web application.
This is the primary planning document — all implementation work should trace back
to sections and checklists defined here.

---

## 1. Project Goal

Build a lightweight, performant, modular single-page web application using
vanilla JavaScript ES modules. No UI frameworks (no React, Vue, Svelte, no
Bootstrap). Dark-mode-first, custom CSS with CSS variables, designed for the
Style Manager theming system from day one.

- **Primary targets:** Desktop browsers and iPad (responsive within that range)
- **Secondary:** Android PWA as-is; iOS may get a native Swift companion later
- **Backend:** Hydrogen — a C-based server with REST API, JWT auth, and
  libwebsockets for future real-time communication
- **Branding:** "Lithium" with tagline "A Philement Project"
- **First two managers:** Style Manager (theming) and Profile Manager (user
  preferences including language, date/time, number formats)

---

## 2. Core Technologies and Principles

| Category | Choice | Notes |
|----------|--------|-------|
| Language | Vanilla JavaScript (ES modules, async/await, fetch) | No TypeScript for now |
| Frameworks | None — no React, Vue, Svelte, Bootstrap | Custom CSS only |
| Build tool | Vite (vanilla JS template) | Dev server, hot reload, production build, chunking |
| Testing | Vitest + jsdom or happy-dom | Unit/integration testing |
| PWA | manifest.json + service worker | Cache statics, modules, lookups; offline fallbacks |
| Auth | JWT from Hydrogen server | Punchcard claim for permissions (planned extension) |
| Event system | EventTarget-based event bus | Singleton for cross-module communication |
| Tables | Tabulator (npm) | Dynamic `import('tabulator-tables')` in managers |
| Code editor | CodeMirror 6 (npm) | `@codemirror/*` packages; import per-mode in managers |
| Sanitization | DOMPurify (npm) | For any user-injected content |
| Icons | Font Awesome | CDN Kit-based loading |
| Fonts | Vanadium Sans + Vanadium Mono | Local-first (`local()`), self-hosted WOFF2 fallback |
| Date picker | Flatpickr (CDN for now) | If needed for date-related UI |

### Libraries: npm vs CDN

Core libraries used by managers are installed via npm and imported as ES modules.
This enables Vite tree-shaking, consistent versioning, and clean dynamic
`import()` for lazy loading. CDN is used only for global utilities
(Font Awesome icons) or libraries not yet needed.

**npm-managed (install via `package.json`, import in code):**

- **CodeMirror 6** — ESM-native; install `@codemirror/lang-css`,
  `@codemirror/view`, `@codemirror/state`, etc. Import only the modes needed
  per manager. Vite tree-shakes unused parts.
- **Tabulator** — `tabulator-tables` npm package; import in managers that
  need tables.
- **DOMPurify** — `dompurify` npm package; import in core and Style Manager.

**CDN-loaded (in `index.html`, global availability):**

- **Font Awesome** — Kit-based script tag; used everywhere for icons.
- **Flatpickr** — CDN for now; move to npm if heavily used.

**Keep for now, evaluate per-manager (CDN, lazy-load when needed):**

Luxon (may switch to native `Intl.*`), Prism.js or Highlight.js (pick one),
KaTeX, SunEditor, SheetJS, Interact.js, SimpleBar, UA-Parser, Split.js.
These are loaded conditionally — only when a manager that needs them is
activated. Not loaded globally on startup.

**Remove:** Bootstrap CSS/JS (replaced by custom CSS)

---

## 3. Terminology

Clear definitions to avoid confusion throughout the project:

| Term | Definition |
|------|-----------|
| **Module** | An ES module (`.js` file using `import`/`export`). Could be a utility, a service, an API wrapper, or a full manager. The building blocks of the codebase. |
| **Manager** | A large, user-facing feature set packaged as one or more modules. Managers have UI. Examples: Login Manager, Main Menu Manager, Style Manager, Profile Manager. Not all managers are visible to all users. |
| **Feature** | A function or capability *within* a manager that may differ between users. One user might have a simpler UI; another might have full editing access. Features are controlled by the Punchcard permissions. |
| **Core** | Infrastructure modules that managers depend on: event bus, JSON request wrapper, permissions, utilities, logger, etc. |

### Manager vs Module relationship

- Every manager requires at least one module
- A manager may require many modules (e.g., Style Manager → theme list module,
  visual editor module, CSS editor module)
- Not all modules are managers — modules can be shared utilities, API wrappers,
  or core infrastructure
- Managers are independently loadable — some may be shown standalone (without
  the main menu) or skipped entirely (e.g., login via API keys)

---

## 4. Folder Structure

Starting from the existing `elements/003-lithium/` root. This replaces the
current structure. Files marked ✅ exist already; ⬜ need to be created; 🔄
exist but need rework.

```structure
elements/003-lithium/
├── index.html                    🔄 Strip Bootstrap, simplify CDN list
├── package.json                  🔄 Add CodeMirror 6, Tabulator, DOMPurify, Vitest; remove Mocha deps
├── vite.config.js                🔄 Update for new structure
├── vitest.config.js              ✅ Vitest setup with happy-dom
├── BLUEPRINT.md                  ✅ This document
├── INSTRUCTIONS.md               🔄 Rewrite to match new architecture
├── README.md                     🔄 Update to match new architecture
├── LICENSE.md                    ✅ MIT License
│
├── public/                       ✅ Static assets served as-is
│   ├── favicon.ico               ✅
│   ├── manifest.json             🔄 Update name, icons, theme
│   ├── service-worker.js         🔄 Update cache list
│   └── assets/
│       ├── fonts/
│       │   ├── VanadiumSans-SemiExtended.woff2   ✅
│       │   └── VanadiumMono-SemiExtended.woff2   ✅
│       └── images/
│           ├── logo-li.svg       ✅ Lithium logo (move from project root)
│           └── logo-192x192.png  🔄 Update with Lithium branding
│
├── src/
│   ├── app.js                    🔄 Complete rewrite — bootstrap + manager loader
│   │
│   ├── core/                     Core infrastructure modules
│   │   ├── event-bus.js          ✅ Singleton EventTarget with standard events
│   │   ├── json-request.js       ✅ Centralized fetch wrapper (auth, errors)
│   │   ├── permissions.js        ✅ Punchcard parsing + canAccessManager/hasFeature
│   │   ├── config.js             ✅ Configuration loader with defaults
│   │   ├── utils.js              ⬜ Formatters, prefs helpers, date/number formatting
│   │   ├── logger.js             🔄 Adapt existing — add structured events, batch upload
│   │   ├── network.js            🔄 Adapt existing — integrate with json-request
│   │   ├── storage.js            🔄 Adapt existing — keep IndexedDB/localStorage
│   │   └── router.js             🔄 Simplify — manager-based, not page-based
│   │
│   ├── styles/                   Custom CSS (no Bootstrap)
│   │   ├── base.css              ✅ CSS reset, variables, font-face, dark theme defaults
│   │   ├── layout.css            ✅ Login, menu sidebar, workspace, header bar
│   │   ├── components.css        ✅ Buttons, inputs, cards, tables, modals, scrollbars
│   │   └── transitions.css       ✅ Fade, crossfade, slide animations with reduced-motion
│   │
│   ├── managers/                 All manager implementations
│   │   ├── login/
│   │   │   ├── login.js          🔄 Rewrite — Hydrogen JWT login, dark theme
│   │   │   ├── login.html        🔄 Rewrite — CCC-style, no Bootstrap classes
│   │   │   ├── login.css         ⬜ Login-specific styles
│   │   │   └── README.md         🔄 Update documentation
│   │   │
│   │   ├── main/
│   │   │   ├── main.js           🔄 Rewrite — sidebar + workspace + header bar
│   │   │   ├── main.html         🔄 Rewrite — no Bootstrap, custom layout
│   │   │   ├── main.css          ⬜ Main layout styles
│   │   │   └── README.md         🔄 Update documentation
│   │   │
│   │   ├── style-manager/
│   │   │   ├── index.js          ⬜ Exports init, render, teardown
│   │   │   ├── style-manager.html ⬜ Layout: theme list + editor workspace
│   │   │   ├── style-manager.css ⬜ Style Manager styles
│   │   │   └── README.md         ⬜ Documentation
│   │   │
│   │   ├── profile-manager/
│   │   │   ├── index.js          ⬜ Exports init, render, teardown
│   │   │   ├── profile-manager.html ⬜ Preferences form layout
│   │   │   ├── profile-manager.css  ⬜ Profile Manager styles
│   │   │   └── README.md         ⬜ Documentation
│   │   │
│   │   ├── dashboard/            🔄 Keep as placeholder, rework later
│   │   ├── lookups/              🔄 Keep as placeholder, rework later
│   │   ├── queries/              🔄 Keep as placeholder, rework later
│   │   └── source-editor/        🔄 Keep as placeholder, rework later
│   │
│   ├── init/                     ✅ Third-party library init wrappers (keep as-is for now)
│   │   ├── tabulator-init.js     ✅
│   │   ├── codemirror-init.js    ✅
│   │   ├── flatpickr-init.js     ✅
│   │   ├── suneditor-init.js     ✅
│   │   ├── jsoneditor-init.js    ✅
│   │   ├── prism-init.js         ✅
│   │   ├── highlight-init.js     ✅
│   │   ├── tabulator-simple.css  ✅
│   │   ├── tabulator-full.css    ✅
│   │   └── jsoneditor.css        ✅
│   │
│   └── shared/                   Shared data modules
│       └── lookups.js            ⬜ Promise-based lookup getters
│
├── config/                       ✅ Runtime configuration
│   └── lithium.json              ✅ Server URL, API key, default database, etc.
│
└── tests/                        🔄 Migrate from Mocha to Vitest
    ├── unit/                     ⬜ Unit tests
    │   ├── event-bus.test.js     ⬜
    │   ├── json-request.test.js  ⬜
    │   ├── permissions.test.js   ⬜
    │   ├── utils.test.js         ⬜
    │   └── logger.test.js        ⬜
    ├── integration/              ⬜ Integration tests
    │   ├── login-flow.test.js    ⬜
    │   ├── manager-loading.test.js ⬜
    │   └── menu-render.test.js   ⬜
    └── (existing bash test scripts remain for CI/CD linting)
```

### Files to Remove

These files should be removed or replaced as part of the migration:

- `src/acuranzo.css` — Reference CSS from different project; patterns extracted
  into new `styles/*.css` files
- `src/lithium.css` — Replaced by `styles/base.css`, `styles/layout.css`,
  `styles/components.css`, `styles/transitions.css`
- Bootstrap CDN links from `index.html`
- Any references to Bootstrap classes in HTML templates

---

## 5. Configuration File

A new `config/lithium.json` file tells Lithium where to find its Hydrogen
server. This file is loaded at startup before any authentication attempts.

```json
{
  "server": {
    "url": "https://hydrogen.example.com",
    "api_prefix": "/api",
    "websocket_url": "wss://hydrogen.example.com/ws"
  },
  "auth": {
    "api_key": "your-api-key-here",
    "default_database": "Demo_PG",
    "session_timeout_minutes": 30
  },
  "app": {
    "name": "Lithium",
    "tagline": "A Philement Project",
    "version": "1.0.0",
    "logo": "/assets/images/logo-li.svg",
    "default_theme": "dark",
    "default_language": "en-US"
  },
  "features": {
    "offline_mode": true,
    "install_prompt": true,
    "debug_logging": false
  }
}
```

> **Security note:** This config contains no secrets. The `api_key` is a
> public application identifier validated server-side, not a secret key. The
> config file may also include pointers to logo files or other assets needed
> for branding before login. It is intentionally available as a public static
> file.

### Checklist: Configuration

- [x] Create `config/lithium.json` with structure above
- [x] Create `src/core/config.js` module to load and parse the config
- [x] Config loaded in `app.js` before anything else
- [x] Fallback defaults if config file is missing or partial
- [x] Config values accessible to all managers via `app.config`
- [ ] Document config options in INSTRUCTIONS.md

---

## 6. High-Level Bootstrap Flow

This is the sequence that runs when the app loads:

```sequence
1.  index.html loads → <script type="module" src="/src/app.js">
2.  app.js: Load config/lithium.json
3.  app.js: Initialize core modules (event bus, logger, config)
4.  app.js: Apply FOUC prevention (visibility: hidden → visible after paint)
5.  app.js: Check for existing JWT in localStorage
6.  If no/expired JWT → load Login Manager
7.  On login success → store JWT, decode claims, cache user info
8.  Parse Punchcard from JWT (when available) → determine permitted managers
9.  Fetch lookups from server (filtered by lang from JWT/prefs)
10. Load Main Menu Manager → build sidebar from permitted managers
11. Render header bar (profile button, theme selector, logout, manager switcher)
12. Load default/first permitted manager into workspace
13. Menu clicks → lazy import manager → init/render in workspace div
14. Loaded managers can be switched via header (hide/show, preserved state)
```

### Checklist: Bootstrap Flow

- [x] `app.js` loads config before anything else
- [x] Core modules initialized in correct order (event bus → logger → config → network)
- [x] FOUC prevention: `html { visibility: hidden }` in `<style>`, revealed after init
- [x] JWT check uses proper expiry validation (decode + check `exp` claim)
- [x] Login Manager loaded dynamically when no valid JWT
- [x] On successful login, JWT stored and decoded
- [x] Main Menu Manager loaded after successful auth
- [x] Event bus fires `auth:login` event on successful login
- [x] Event bus fires `auth:logout` event on logout
- [x] Managers loaded lazily via dynamic `import()`
- [x] Manager state preserved when switching between managers (hide/show, not destroy)
- [ ] Document bootstrap flow in INSTRUCTIONS.md

---

## 7. Authentication and JWT

### Hydrogen Login API

The Hydrogen server exposes `POST /api/auth/login` expecting:

```json
{
  "login_id": "username_or_email",
  "password": "user_password",
  "api_key": "application_api_key",
  "tz": "America/Vancouver",
  "database": "Demo_PG"
}
```

Successful response:

```json
{
  "token": "Bearer eyJhbGciOiJIUzI1NiIs...",
  "expires_at": 1709625600,
  "user_id": 42,
  "roles": ["admin", "editor"]
}
```

### Current JWT Claims (from Hydrogen)

These are the claims already present in Hydrogen-issued JWTs:

| Claim | Type | Description |
|-------|------|-------------|
| `iss` | string | Issuer |
| `sub` | string | Subject (user ID) |
| `aud` | string | Audience (app ID) |
| `exp` | timestamp | Expiration time |
| `iat` | timestamp | Issued at |
| `nbf` | timestamp | Not before |
| `jti` | string | JWT ID (unique) |
| `user_id` | int | User ID |
| `system_id` | int | System ID |
| `app_id` | int | Application ID |
| `username` | string | Username |
| `email` | string | Email address |
| `roles` | string | User roles (JSON) |
| `ip` | string | Client IP |
| `tz` | string | Timezone |
| `tzoffset` | int | Timezone offset in minutes |
| `database` | string | Database name |

### Planned JWT Extensions (Hydrogen changes needed later)

These claims will be added to Hydrogen when we're ready:

| Claim | Type | Description |
|-------|------|-------------|
| `lang` | string | Preferred language (e.g., "fr-FR") |
| `punchcard` | object | `{ managers: [id, ...], features: { managerId: [featureId, ...] } }` |

### Hydrogen Auth Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/auth/login` | POST | Authenticate and get JWT |
| `/api/auth/logout` | POST | Revoke JWT |
| `/api/auth/renew` | POST | Renew JWT before expiry |
| `/api/auth/register` | POST | Create new account |

### JWT Handling in Lithium

Client-side JWT operations (no server-side JWT libraries needed):

1. **Decode** — Base64url decode the payload segment to read claims
2. **Validate expiry** — Check `exp` claim against current time
3. **Store** — `localStorage` for now (persistent across tabs/sessions)
4. **Attach** — Add `Authorization: Bearer <token>` to every API request
5. **Renew** — Call `/api/auth/renew` before expiry (e.g., at 80% of lifetime)
6. **Revoke** — Call `/api/auth/logout` on explicit logout

> **Future: HttpOnly cookies.** Current best practice favors server-set
> `HttpOnly; Secure; SameSite=Strict` cookies over `localStorage` for JWT
> storage, as this mitigates XSS token theft. This requires a Hydrogen server
> change (set `Set-Cookie` header on login response instead of returning the
> token in the JSON body). The Lithium client would then stop manually
> attaching `Authorization` headers — the browser sends the cookie
> automatically. This is tracked as a future Hydrogen enhancement alongside
> the Punchcard and `lang` claim additions.

### Checklist: Authentication

- [x] Create `src/core/jwt.js` — decode, validate, isExpired, getClaimshelpers
- [x] Create `src/core/json-request.js` — fetch wrapper that attaches JWT,
      handles 401 (redirect to login), parses JSON responses, logs errors
- [x] Login Manager sends `login_id`, `password`, `api_key` (from config), `tz`
       (from `Intl.DateTimeFormat().resolvedOptions().timeZone`), `database`
       (from config)
- [x] On 200 response: store token, decode claims, fire `auth:login` event
- [x] On 401/error: show error message in login form, clear password field
- [x] On 429: show rate limit message with retry-after countdown
- [x] JWT auto-renewal: set timer for ~80% of token lifetime
- [x] On renewal failure: fire `auth:expired` event → redirect to login
- [x] Logout: call `/api/auth/logout`, clear stored JWT, fire `auth:logout`
- [x] All API requests via `json-request.js` — never raw `fetch()` from managers
- [x] Handle network errors gracefully (offline indicator, queued requests)
- [ ] Document JWT flow in INSTRUCTIONS.md
- [x] Unit tests for jwt.js (decode, validate, expiry check) — 25 tests
- [ ] Unit tests for json-request.js (mock fetch, auth header, error handling)

---

## 8. Permissions and Punchcard

> **Note:** The Punchcard system is planned for a future Hydrogen update. Until
> then, Lithium should use a fallback that grants access to all managers. The
> codebase should be structured so Punchcard support can be dropped in without
> refactoring.

### Punchcard Structure (planned JWT claim)

```json
{
  "punchcard": {
    "managers": [1, 2, 5, 8],
    "features": {
      "1": [1, 2, 3, 4, 5],
      "2": [1, 2],
      "5": [1],
      "8": [1, 2, 3]
    }
  }
}
```

### Permission Helpers

```javascript
// src/core/permissions.js
canAccessManager(managerId)  → boolean
hasFeature(managerId, featureId) → boolean
getPermittedManagers() → array of manager IDs
getFeaturesForManager(managerId) → array of feature IDs
```

### Checklist: Permissions

- [x] Create `src/core/permissions.js` with the helpers above
- [x] Fallback: if no `punchcard` claim in JWT, grant access to all managers
- [x] Menu builder filters managers by `canAccessManager()`
- [x] Individual manager UIs check `hasFeature()` before showing advanced controls
- [ ] Backend enforces permissions server-side (Lithium is client-side guard only)
- [x] Permissions module fires `permissions:updated` event when JWT is refreshed
- [x] Unit tests for all permission helpers (with and without punchcard) — 23 tests
- [ ] Document punchcard structure in INSTRUCTIONS.md

---

## 9. Event Bus

A lightweight singleton event bus for cross-module communication. This replaces
direct coupling between modules.

```javascript
// src/core/event-bus.js
class EventBus extends EventTarget {
  emit(name, detail) { this.dispatchEvent(new CustomEvent(name, { detail })); }
  on(name, handler)  { this.addEventListener(name, handler); }
  off(name, handler) { this.removeEventListener(name, handler); }
}
export const eventBus = new EventBus();
```

### Standard Events

| Event | Payload | Fired When |
|-------|---------|------------|
| `auth:login` | `{ userId, username, roles }` | Successful login |
| `auth:logout` | `{}` | User logs out |
| `auth:expired` | `{}` | JWT expired and renewal failed |
| `auth:renewed` | `{ expiresAt }` | JWT successfully renewed |
| `permissions:updated` | `{ punchcard }` | Permissions changed (new JWT) |
| `manager:loaded` | `{ managerId }` | Manager finished loading |
| `manager:switched` | `{ from, to }` | Active manager changed |
| `theme:changed` | `{ themeId, vars }` | Theme applied |
| `locale:changed` | `{ lang, formats }` | Language/format pref changed |
| `network:online` | `{}` | Connection restored |
| `network:offline` | `{}` | Connection lost |

### Checklist: Event Bus

- [x] Create `src/core/event-bus.js` as singleton
- [x] `emit()`, `on()`, `off()` methods
- [x] All cross-module communication uses event bus (no direct imports between
       managers)
- [x] Standard events defined and documented
- [x] Unit tests for emit/on/off — 6 tests, 100% coverage
- [ ] Document event bus API in INSTRUCTIONS.md

---

## 10. CSS Architecture

No Bootstrap. Custom CSS with CSS variables, designed for the Style Manager
theming system from day one. Dark mode is the default.

### File Organization

| File | Purpose |
|------|---------|
| `styles/base.css` | CSS reset, `:root` variables (colors, fonts, spacing, borders, shadows), `@font-face` declarations, dark theme defaults |
| `styles/layout.css` | Full-page login layout, sidebar + workspace split, header bar, content area |
| `styles/components.css` | Buttons, inputs, selects, cards, tables, modals, tooltips, scrollbar styling, badges |
| `styles/transitions.css` | Fade-in/out, crossfade, slide animations, reduced-motion support |
| `managers/login/login.css` | Login-specific overrides |
| `managers/main/main.css` | Main layout overrides |
| `managers/style-manager/style-manager.css` | Style Manager-specific |
| `managers/profile-manager/profile-manager.css` | Profile Manager-specific |

### CSS Variables (`:root`)

The base dark theme variables. These are what the Style Manager will override.

```css
:root {
  /* Colors - Dark theme defaults */
  --bg-primary: #1A1A1A;
  --bg-secondary: #242424;
  --bg-tertiary: #2E2E2E;
  --bg-input: #333333;
  --text-primary: #E0E0E0;
  --text-secondary: #B0B0B0;
  --text-muted: #707070;
  --border-color: #3A3A3A;
  --accent-primary: #5C6BC0;
  --accent-secondary: #7986CB;
  --accent-hover: #3F51B5;
  --accent-danger: #EF5350;
  --accent-success: #66BB6A;
  --accent-warning: #FFA726;

  /* Typography — local-first font strategy */
  --font-family: 'Vanadium Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI',
    Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
  --font-mono: 'Vanadium Mono', 'Courier New', Courier, monospace;
  --font-size-xs: 11px;
  --font-size-sm: 12px;
  --font-size-base: 14px;
  --font-size-lg: 16px;
  --font-size-xl: 20px;
  --font-size-2xl: 24px;
  --line-height: 1.5;

  /* Spacing */
  --space-1: 4px;
  --space-2: 8px;
  --space-3: 12px;
  --space-4: 16px;
  --space-5: 24px;
  --space-6: 32px;

  /* Borders */
  --border-width: 1px;
  --border-radius-sm: 4px;
  --border-radius-md: 6px;
  --border-radius-lg: 12px;
  --border-standard: var(--border-width) solid var(--border-color);

  /* Shadows */
  --shadow-sm: 0 1px 3px rgba(0,0,0,0.3);
  --shadow-md: 0 4px 12px rgba(0,0,0,0.4);
  --shadow-lg: 0 8px 32px rgba(0,0,0,0.5);

  /* Transitions */
  --transition-fast: 150ms ease;
  --transition-std: 300ms ease;
  --transition-slow: 500ms ease;

  /* Scrollbar */
  --scrollbar-width: 8px;
  --scrollbar-track: var(--bg-secondary);
  --scrollbar-thumb: var(--bg-tertiary);
}
```

### Style Manager Integration Points

The Style Manager will override CSS variables by injecting a `<style
id="dynamic-theme">` block with `:root` overrides. This means:

1. All visual properties must use CSS variables — never hardcoded colors
2. The dynamic theme `<style>` tag is injected *after* the base stylesheets
3. User themes are stored as JSON objects mapping variable names → values
4. Optional raw CSS can be appended for advanced customization
5. All user CSS is sanitized with DOMPurify before injection

### Checklist: CSS Architecture

- [x] Create `src/styles/base.css` with full variable set, CSS reset, and
       `@font-face` declarations using `local()` first, self-hosted WOFF2 fallback
- [x] Create `src/styles/layout.css` with login, sidebar, workspace, header layouts
- [x] Create `src/styles/components.css` with all UI components
- [x] Create `src/styles/transitions.css` with animations and reduced-motion
- [x] All colors, fonts, spacing, borders, shadows use CSS variables
- [x] No hardcoded color values anywhere in the codebase
- [x] Dark theme is the default (no media query required for dark)
- [x] Custom scrollbar styling (WebKit + Firefox)
- [x] `@media (prefers-reduced-motion: reduce)` disables animations
- [x] Remove `src/lithium.css` (replaced by new styles)
- [x] Remove `src/acuranzo.css` (patterns extracted into new styles)
- [x] Remove all Bootstrap CDN links from `index.html`
- [x] Remove all Bootstrap class usage from HTML templates
- [ ] Document CSS variable naming convention in INSTRUCTIONS.md
- [ ] Document how Style Manager injects themes in INSTRUCTIONS.md

---

## 11. Login Manager

The first thing users see. Follows the CCC reference project login pattern:
centered card on dark background, logo + title in gradient header, clean inputs,
fade transition to main app.

### Login UI Structure

```html
<div id="login-page">
  <div class="login-container">
    <div class="login-header">
      <img src="/assets/images/logo-li.svg" alt="" class="login-logo">
      <div class="login-header-text">
        <h1>Lithium</h1>
        <p>A Philement Project</p>
      </div>
    </div>
    <form class="login-form" id="login-form">
      <div class="form-group">
        <input type="text" id="username" placeholder="Username" required>
      </div>
      <div class="form-group">
        <input type="password" id="password" placeholder="Password" required>
      </div>
      <div class="login-error" id="login-error">
        <i class="fa-..."></i> Error message here
      </div>
      <button type="submit" class="login-btn">
        <i class="fa-..."></i> Login
      </button>
    </form>
  </div>
</div>
```

### Login Flow

1. Config loaded → server URL and API key available
2. Login page shown with fade-in (FOUC prevented)
3. Username field auto-focused after render
4. On submit: POST to `{server_url}/api/auth/login` with:
   - `login_id` from username field
   - `password` from password field
   - `api_key` from config
   - `tz` from `Intl.DateTimeFormat().resolvedOptions().timeZone`
   - `database` from config
5. On success (200): store JWT, fade out login, load Main Menu Manager
6. On failure (401): show error message, clear password, focus password field
7. On rate limit (429): show "Too many attempts" with countdown
8. On network error: show "Cannot reach server" message

### Login CSS (reference from CCC)

Key styling decisions from the CCC reference:

- Fixed full-page overlay with `display: flex; align-items: center; justify-content: center`
- Container: `width: 360px`, dark secondary background, `border-radius: 12px`,
  prominent `box-shadow`
- Header: gradient background (`accent-primary` → `accent-secondary`), white text,
  logo + title side by side
- Inputs: dark tertiary background, subtle border, `border-radius: 6px`, no
  visible labels (placeholders only)
- Error: red-tinted background, hidden by default, shown with CSS class toggle
- Submit button: accent primary background, full width, `margin-top: 30px`
- Fade transition: `opacity` with `transition: 0.8s ease`

### Checklist: Login Manager

- [x] Rewrite `src/managers/login/login.html` — CCC-style, no Bootstrap
- [x] Rewrite `src/managers/login/login.js` — Hydrogen API login, proper error
       handling
- [x] Create `src/managers/login/login.css` — dark theme login styles
- [ ] Move `logo-li.svg` from project root to `public/assets/images/logo-li.svg`
- [x] Login form POSTs to Hydrogen `/api/auth/login`
- [x] `api_key`, `database`, and server URL come from config
- [x] `tz` auto-detected from browser
- [x] Error handling: 401 (bad credentials), 429 (rate limit), network error
- [x] Fade transition from login → main app (match CCC 0.8s timing)
- [x] FOUC prevention: page hidden until login rendered
- [x] Auto-focus username field on render
- [x] Enter key submits form
- [x] Loading state on submit button (disable + spinner or text change)
- [ ] Unit tests for login logic (mock fetch responses)
- [ ] Update `src/managers/login/README.md`

---

## 12. Main Menu Manager

After successful login, this is the persistent layout. Sidebar on the left,
workspace on the right, header bar across the top of the workspace.

### Layout Structure

```structure
┌──────────────────────────────────────────────────────┐
│ ┌──────────┐ ┌──────────────────────────────────────┐ │
│ │          │ │ Header Bar                           │ │
│ │  Menu    │ │ [Profile] [Theme] [Manager Switcher] │ │
│ │  Sidebar │ ├──────────────────────────────────────┤ │
│ │          │ │                                      │ │
│ │  [Style] │ │        Active Manager                │ │
│ │  [Prof]  │ │        Workspace                     │ │
│ │  [Dash]  │ │                                      │ │
│ │          │ │                                      │ │
│ │          │ │                                      │ │
│ │ ──────── │ │                                      │ │
│ │ [Logout] │ │                                      │ │
│ └──────────┘ └──────────────────────────────────────┘ │
└──────────────────────────────────────────────────────┘
```

### Sidebar

- Manager buttons generated from permitted managers list
- Each button: icon + label (localized name from lookups when available)
- Active manager highlighted
- Logout button at bottom (separated by divider)
- Sidebar optionally resizable (splitter between sidebar and workspace)
- Sidebar optionally collapsible (icon-only mode)

### Header Bar

- Left: current manager name / breadcrumb
- Center: manager-specific controls (optional, each manager can inject)
- Right: profile button, theme quick-switcher (stub), logout button

### Workspace

- Container div where the active manager renders
- Managers are loaded once and shown/hidden (not destroyed/recreated)
- Supports crossfade transitions between managers

### Checklist: Main Menu Manager

- [x] Rewrite `src/managers/main/main.html` — custom layout, no Bootstrap
- [x] Rewrite `src/managers/main/main.js` — sidebar generation from permissions,
       manager loading, header bar logic
- [x] Create `src/managers/main/main.css` — sidebar, workspace, header styles
- [x] Sidebar buttons generated dynamically from `getPermittedManagers()`
- [x] Click sidebar button → lazy load manager → render in workspace
- [x] Manager state preserved on switch (hide/show, not destroy/recreate)
- [x] Crossfade transition between managers
- [x] Header bar with profile button, theme button (stub), logout
- [x] Logout button: call `/api/auth/logout`, clear JWT, fade to login
- [x] Sidebar resizable via splitter (optional, can use existing Split.js)
- [x] Sidebar collapse to icon-only mode (optional)
- [x] Responsive: sidebar stacks above workspace on smaller screens
- [x] Fire `manager:switched` event on manager change
- [ ] Update `src/managers/main/README.md`

---

## 13. Style Manager

The first real "functional" manager. Purpose: create, edit, apply, and share
user themes (CSS variable overrides + optional raw CSS).

### Feature IDs (for Punchcard)

| Feature ID | Description |
|------------|-------------|
| 1 | View all styles (read-only) |
| 2 | Edit all styles |
| 3 | Use visual editor |
| 4 | Use CSS code editor |
| 5 | Delete themes |

### Layout

Three views, switchable via view buttons in the header or workspace:

1. **List View** — Tabulator table of themes (name, modified date, preview
   thumbnail stub, actions). Gated by feature permissions.
2. **Visual Editor** — Preview scene with clickable mockup elements. Click
   element → inspector panel with grouped CSS property editors.
3. **CSS View** — CodeMirror with CSS mode and linting. Full theme CSS, live
   preview.

### Theme Data Model

Themes are stored on the server (via Hydrogen API → Helium database). Client
representation:

```json
{
  "id": 42,
  "name": "Midnight Indigo",
  "owner_id": 7,
  "shared_with": [3, 12],
  "public": false,
  "variables": {
    "--bg-primary": "#0D1117",
    "--accent-primary": "#58A6FF",
    "...": "..."
  },
  "raw_css": "/* optional advanced overrides */",
  "created_at": "2025-01-15T10:30:00Z",
  "updated_at": "2025-03-01T14:22:00Z"
}
```

### Theme Application

1. Build CSS string from `variables` object → `:root { --var: value; ... }`
2. If `raw_css` exists, append it after the variables block
3. Sanitize the complete CSS string with DOMPurify
4. Inject into `<style id="dynamic-theme">` (create or replace)
5. Persist active theme ID in user preferences (Profile Manager)

### Visual Editor (Phase 2+ — skeleton only in initial milestone)

- **Preview scene:** Clickable mockup with buttons (primary, secondary, danger),
  card, input, table snippet, nav item, text blocks
- **Inspector panel:** Click element → grouped properties (colors, borders,
  spacing, typography, shadows, transitions)
- **Property layers:** Default → Standard primitives → Semantic tokens →
  Theme-specific → Borrow from other themes
- **Live apply:** Debounced CSS variable updates as user edits

### Fonts Management (Phase 2+)

- Default: self-hosted Vanadium WOFF2 from server, cached by SW
- User-added: backend downloads from Google Fonts etc. → hosts locally →
  generates `@font-face` → SW caches
- Offline-safe after first import

### Checklist: Style Manager

- [ ] Create `src/managers/style-manager/index.js` with init/render/teardown
- [ ] Create `src/managers/style-manager/style-manager.html` — list + editor layout
- [ ] Create `src/managers/style-manager/style-manager.css`
- [ ] **List View:**
  - [ ] Tabulator table displaying themes (name, date, owner, actions)
  - [ ] "New Theme" button (creates theme with current defaults as base)
  - [ ] "Apply" action (apply theme globally)
  - [ ] "Edit" action (opens visual editor or CSS editor based on permissions)
  - [ ] "Delete" action (confirmation dialog, gated by feature ID 5)
  - [ ] Theme data loaded from Hydrogen API
- [ ] **Theme Application:**
  - [ ] `applyTheme(themeData)` function builds CSS from variables + raw_css
  - [ ] DOMPurify sanitization before injection
  - [ ] `<style id="dynamic-theme">` element managed in document head
  - [ ] Active theme persisted in user preferences
  - [ ] `theme:changed` event fired on application
- [ ] **CSS View (Phase 1):**
  - [ ] CodeMirror editor with CSS mode
  - [ ] Load theme's raw CSS into editor
  - [ ] "Apply" button for live preview
  - [ ] "Save" button to persist changes
- [ ] **Visual Editor (Phase 2 — defer for now):**
  - [ ] Preview scene with mockup elements
  - [ ] Inspector panel stub
  - [ ] Property editors for CSS variables
- [ ] Feature-gated UI (check permissions before showing controls)
- [ ] Create `src/managers/style-manager/README.md`
- [ ] Unit tests for theme application logic
- [ ] Unit tests for CSS sanitization

---

## 14. Profile Manager

User preferences that affect app behavior. Saved to server → triggers JWT
refresh → client re-applies.

### Key Settings

| Setting | Description | Implementation |
|---------|-------------|---------------|
| Language | Select from supported languages | Save → new JWT with `lang` claim → refetch lookups → rebuild menu |
| Date format | Short/medium/long, timezone-aware | `Intl.DateTimeFormat` with pref-based options |
| Time format | 12h / 24h | `Intl.DateTimeFormat` with `hour12` option |
| Number format | Decimal separator, grouping, currency | `Intl.NumberFormat` with pref-based options |
| Default database | For multi-app future | Stored in prefs |
| Active theme | Quick-apply from theme list | Delegates to Style Manager's apply function |

### UI Layout

Simple form layout — can be a single-page form or tabbed sections. Uses
native HTML form elements styled with CSS components (no Bootstrap form classes).

### Save Flow

1. User changes preference
2. POST to Hydrogen API to update user record
3. Server issues new JWT with updated claims (when `lang` or perms change)
4. Client stores new JWT
5. Fire `locale:changed` or `theme:changed` event
6. Subscribed modules react (menu rebuilds, formatters update, etc.)

### Checklist: Profile Manager

- [ ] Create `src/managers/profile-manager/index.js` with init/render/teardown
- [ ] Create `src/managers/profile-manager/profile-manager.html` — prefs form
- [ ] Create `src/managers/profile-manager/profile-manager.css`
- [ ] Language selector (dropdown of supported languages)
- [ ] Date format selector (short/medium/long examples shown)
- [ ] Time format toggle (12h/24h)
- [ ] Number format preview
- [ ] Active theme selector (quick-apply dropdown)
- [ ] Save triggers API call → JWT refresh if needed
- [ ] `locale:changed` event fired on save
- [ ] Format utility functions in `src/core/utils.js` use prefs from JWT/storage
- [ ] Create `src/managers/profile-manager/README.md`

---

## 15. Localization

### Current Approach (Phase 1)

- UI strings: English hard-coded directly in HTML/JS
- Server lookups: backend filters by `lang` from JWT (when available)
- Fallback: `en-US` or first available
- Date/number formatting: native `Intl.*` APIs based on user preferences

### Phase 2 (Extract to JSON bundles)

- Extract all UI strings to `locales/{lang}.json` files
- Create `t(key)` translation helper
- Lazy-load locale files on language change
- Lookups and UI strings both respect active language

### Checklist: Localization

- [ ] `src/core/utils.js` includes `formatDate()`, `formatTime()`,
      `formatNumber()` using `Intl.*` APIs
- [ ] Format functions read user preferences from JWT claims or localStorage
- [ ] Language change: Profile Manager saves → new JWT → `locale:changed` event
- [ ] Menu labels use localized names from lookups when available
- [ ] Phase 1: English strings inline (acceptable for initial development)
- [ ] Phase 2: Extract strings to JSON bundles (tracked separately)
- [ ] Document localization approach in INSTRUCTIONS.md

---

## 16. Lookups

Server-provided reference data used throughout the app (manager names, feature
labels, status values, etc.).

```javascript
// src/shared/lookups.js
let cache = null;

export async function fetchLookups(jsonRequest) {
  const data = await jsonRequest.get('/api/lookups');
  cache = data;
  return data;
}

export function getLookup(category, key) {
  return cache?.[category]?.[key] ?? key;
}

export function getManagerName(managerId) {
  return getLookup('managers', managerId);
}
```

### Checklist: Lookups

- [ ] Create `src/shared/lookups.js` with fetch, cache, and getter functions
- [ ] Lookups fetched after login (uses JWT for auth and lang filtering)
- [ ] Cache in memory; refresh on `locale:changed` event
- [ ] Manager names come from lookups (localized)
- [ ] Fallback to ID or English default if lookup missing
- [ ] Unit tests for lookup getters

---

## 17. PWA and Service Worker

### Manifest Updates

Update `public/manifest.json` for Lithium branding:

```json
{
  "name": "Lithium",
  "short_name": "Lithium",
  "description": "A Philement Project",
  "start_url": ".",
  "display": "standalone",
  "background_color": "#1A1A1A",
  "theme_color": "#5C6BC0",
  "icons": [
    { "src": "assets/images/logo-192x192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "assets/images/logo-512x512.png", "sizes": "512x512", "type": "image/png" }
  ]
}
```

### Service Worker Strategy

- **Cache statics:** `index.html`, core JS, CSS, fonts, icons — `cache-first`
- **Cache Vite chunks:** Manager code-split chunks — `cache-first` after first load
- **Cache lookups/API:** Lookup data, theme data — `stale-while-revalidate`
  (serve cached, refresh in background)
- **Cache CDN assets:** Font Awesome, Flatpickr — `cache-first` with versioned
  cache names (note: cross-origin CDN requests may produce opaque responses;
  handle gracefully)
- **Offline fallback:** Show cached data + "last synced" notice
- **Install prompt:** Listen for `beforeinstallprompt`, optional custom button
- **Cache cleanup:** On `activate` event, delete old cache versions

### Checklist: PWA

- [ ] Update `public/manifest.json` with Lithium branding and dark theme
- [ ] Update `public/service-worker.js` cache list for new file structure
- [ ] Cache core files (HTML, JS, CSS, fonts) — cache-first strategy
- [ ] Cache Vite code-split chunks on first load
- [ ] Cache CDN assets (handle cross-origin/opaque responses)
- [ ] Cache API responses with stale-while-revalidate
- [ ] Versioned cache names with old version cleanup in `activate`
- [ ] Offline indicator in header bar
- [ ] "Last synced" timestamp in offline mode
- [ ] Install prompt handling (optional custom button)
- [ ] Generate proper icon files (192x192, 512x512) with Lithium branding

---

## 18. Security and Polish

### Content Security Policy

- Strict CSP headers (configure on Hydrogen server)
- Nonce for dynamic `<style>` injection (theme system)
- Allow CDN origins for third-party libraries
- No `unsafe-inline` for scripts (all code in modules)

### Input Sanitization

- DOMPurify for any user-provided HTML/CSS content
- All theme CSS sanitized before injection
- Login inputs validated client-side (non-empty) + server-side
- No `innerHTML` with user data without sanitization

### Accessibility

- Semantic HTML throughout (headings, landmarks, form labels)
- ARIA attributes on menus, tables, modals
- Keyboard navigation: Tab through all interactive elements
- Focus management: trap focus in modals, return focus on close
- Visible focus indicators (customized to match theme)
- Screen reader announcements for dynamic content changes

### Error Handling

- Login failures: clear error messages
- Token expiry: redirect to login with "session expired" message
- Network failures: offline indicator, queued requests
- API errors: structured error display (not raw JSON dumps)
- Unhandled JS errors: catch at app level, log to console + optional server

### Client-Side Logging

- Structured events: `{ action, category, details, timestamp }`
- Batched upload to server on interval or on logout
- Configurable log level (debug, info, warn, error)
- Optional: persist to IndexedDB for offline periods

### Checklist: Security and Polish

- [ ] Add DOMPurify to CDN loads or npm
- [ ] All `innerHTML` assignments sanitized if they contain user data
- [ ] Theme CSS sanitized before injection
- [ ] CSP headers documented for Hydrogen server configuration
- [ ] Semantic HTML in all templates
- [ ] ARIA attributes on interactive elements
- [ ] Keyboard navigation tested for all managers
- [ ] Focus trap in modal dialogs
- [ ] `@media (prefers-reduced-motion: reduce)` support
- [ ] Error boundary at app level (window.onerror + unhandledrejection)
- [ ] Structured client-side logging
- [ ] Document security requirements in INSTRUCTIONS.md

---

## 19. Testing Strategy

### Migrate to Vitest

Replace Mocha/Chai/NYC with Vitest (same test runner as Vite ecosystem).

### Test Categories

| Category | Target | Tools |
|----------|--------|-------|
| Unit | Core modules (event bus, JWT, permissions, utils, json-request) | Vitest |
| Unit | Lookup parsing, theme application logic | Vitest |
| Integration | Login flow (mock fetch), manager loading, menu rendering | Vitest + jsdom |
| DOM | Menu render, workspace swap, manager init | Vitest + jsdom |
| E2E | Full login → manager interaction (Phase 2) | Playwright or Cypress |

### Coverage Targets

- **Core logic:** high (80%+)
- **UI interactions:** medium (60%+) initially
- **E2E:** low initially (functional smoke tests)

### Checklist: Testing

- [x] Add `vitest` and `happy-dom` to devDependencies
- [x] Create `vitest.config.js`
- [x] Add `@vitest/coverage-v8` for coverage reporting
- [x] Update `package.json` test scripts to use `vitest run` directly
- [ ] Migrate or remove old Mocha test infrastructure from `tests/package.json`
- [x] Unit tests for `event-bus.js` — 6 tests, 100% coverage
- [x] Unit tests for `jwt.js` — 25 tests, 94% stmts / 90% branches / 100% funcs
- [ ] Unit tests for `config.js` (fetch mock, deep merge, dot-notation access)
- [ ] Unit tests for `json-request.js` (mock fetch, auth headers, errors)
- [x] Unit tests for `permissions.js` — 23 tests, 96% stmts / 94% branches
- [ ] Unit tests for `utils.js` (date/number formatters)
- [ ] Unit tests for `lookups.js` (cache, getters)
- [ ] Integration test: login flow with mocked Hydrogen responses
- [ ] Integration test: manager loading into workspace
- [ ] Integration test: menu generation from permissions
- [x] Keep existing bash test scripts for CI/CD (linting, shellcheck, etc.)
- [ ] Document test setup in INSTRUCTIONS.md

---

## 20. Deployment

### Build Process

```bash
npm run build         # Vite build → dist/
npm run build:prod    # Vite build + minify HTML + minify SW
npm run preview       # Preview production build locally
```

### Serve Options

- **From Hydrogen:** Configure Hydrogen to serve `dist/` as static files
- **Standalone:** Serve from any static file server (Nginx, Caddy, etc.)
- **HTTPS required:** PWA features require HTTPS

### Checklist: Deployment

- [ ] Vite build produces correct `dist/` output
- [ ] `config/lithium.json` excluded from build (runtime config, not bundled)
- [ ] Service worker path correct in production build
- [ ] Document deployment options in INSTRUCTIONS.md
- [ ] Document Hydrogen server configuration for CORS/proxy if needed

---

## 21. Implementation Phases

Ordered milestones for implementation. Each phase builds on the previous.

### Phase 0: Blueprint and Documentation (Current)

- [x] Write comprehensive BLUEPRINT.md (reviewed and refined)
- [ ] Update INSTRUCTIONS.md to reflect new architecture
- [ ] Update README.md to reflect new architecture
- [ ] Update docs/Li/README.md to reflect new direction
- [ ] Update docs/Li/INSTRUCTIONS.md to match in-tree INSTRUCTIONS.md

### Phase 1: Foundation ✅ COMPLETED

Strip out Bootstrap and existing module system, build the new core.

- [x] Install npm packages: `tabulator-tables`, `@codemirror/*`, `dompurify`, `vitest`, `happy-dom`
- [x] Create `src/styles/base.css` with dark theme CSS variables + local-first
      `@font-face` for Vanadium fonts
- [x] Create `src/styles/layout.css` with login layout
- [x] Create `src/styles/components.css` with basic components
- [x] Create `src/styles/transitions.css`
- [x] Create `src/core/event-bus.js`
- [x] Create `src/core/jwt.js`
- [x] Create `src/core/json-request.js`
- [x] Create `src/core/permissions.js` (with fallback)
- [x] Create `config/lithium.json`
- [x] Create `src/core/config.js`
- [x] Create `vitest.config.js`
- [x] Strip Bootstrap from `index.html`
- [x] Remove CDN `<script>` tags for Tabulator and CodeMirror from `index.html`
- [x] Rewrite `src/app.js` for new bootstrap flow
- [x] Remove `src/lithium.css` and `src/acuranzo.css`

### Phase 2: Login ✅ COMPLETED

Get a working login screen against a real Hydrogen server.

- [ ] Move `logo-li.svg` to `public/assets/images/`
- [x] Rewrite `src/managers/login/login.html` (CCC-style)
- [x] Create `src/managers/login/login.css`
- [x] Rewrite `src/managers/login/login.js` (Hydrogen API login)
- [x] FOUC prevention working
- [x] Fade transitions working
- [x] Error handling for all response codes
- [ ] Provision persistent Hydrogen server for testing (separate task)

### Phase 3: Main Layout ✅ COMPLETED

After login, show the main application layout.

- [x] Rewrite `src/managers/main/main.html`
- [x] Create `src/managers/main/main.css`
- [x] Rewrite `src/managers/main/main.js`
- [x] Sidebar with manager buttons (from permissions/fallback)
- [x] Header bar with profile, theme, logout buttons
- [x] Workspace container for active manager
- [x] Manager lazy loading and state preservation
- [x] Logout flow (API call + fade to login)

### Phase 4: Style Manager (Skeleton) 🔄 IN PROGRESS

Basic theme management — list themes, apply themes, edit raw CSS.

- [x] Create `src/managers/style-manager/` (placeholder created)
- [ ] List View with Tabulator
- [ ] Apply theme from list
- [ ] CSS View with CodeMirror
- [ ] Theme data model and API integration
- [ ] DOMPurify sanitization for theme CSS

### Phase 5: Profile Manager 🔄 IN PROGRESS

User preferences management.

- [x] Create `src/managers/profile-manager/` (placeholder created)
- [ ] Language, date, time, number format settings
- [ ] Save flow with API call
- [ ] `locale:changed` event integration

### Phase 6: Testing and Polish

- [ ] Migrate to Vitest
- [ ] Write unit tests for core modules
- [ ] Write integration tests for key flows
- [ ] Accessibility audit and fixes
- [ ] PWA manifest and service worker updates
- [ ] Performance optimization

### Phase 7: Visual Editor (Style Manager Phase 2)

- [ ] Preview scene with mockup elements
- [ ] Inspector panel with property editors
- [ ] Live CSS variable editing
- [ ] Undo/redo support

---

## 22. Documentation Updates Needed

The following documents need updates to reflect the new architecture. These
should be updated as implementation progresses, not all at once upfront.

### INSTRUCTIONS.md (in-tree: `elements/003-lithium/INSTRUCTIONS.md`)

This is the primary reference for AI assistants working on the code. Needs:

- [ ] Updated project structure matching new folder layout
- [ ] Manager pattern documentation (vs old "module" pattern)
- [ ] Event bus API documentation
- [ ] CSS variable naming conventions
- [ ] Theme injection approach for Style Manager
- [ ] Configuration file documentation
- [ ] JWT handling documentation
- [ ] Login flow against Hydrogen
- [ ] Punchcard permissions system
- [ ] Testing setup (Vitest)
- [ ] Remove references to Bootstrap, Mocha, old module pattern

### README.md (in-tree: `elements/003-lithium/README.md`)

Brief project overview for humans browsing the repository:

- [ ] Updated project structure links
- [ ] Updated tech stack description
- [ ] Link to BLUEPRINT.md for full architecture
- [ ] Link to docs/Li/ for detailed docs

### docs/Li/README.md

The comprehensive external documentation:

- [ ] Update architecture section
- [ ] Update tech stack (remove Bootstrap, Mocha; add Vitest, custom CSS)
- [ ] Update API reference for new core modules
- [ ] Update module documentation → manager documentation
- [ ] Update testing framework section

### docs/Li/INSTRUCTIONS.md

Mirror of the in-tree INSTRUCTIONS.md:

- [ ] Keep in sync with `elements/003-lithium/INSTRUCTIONS.md`

---

## Appendix A: Reference — CCC Login Implementation

The `/fvl/tnt/t-500nodes/web/` project serves as a reference for the login
screen's look and feel. Key patterns to replicate:

1. **FOUC prevention:** `html { background-color: #1A1A1A; color: #1A1A1A; visibility: hidden; }` in `<style>` tag, then `document.documentElement.style.visibility = 'visible'` after init
2. **Login/app toggle:** Both `#login-page` and `#app` exist in HTML with `.hidden` class; login shown first, then faded out while app fades in
3. **Fade timing:** `transition: opacity 0.8s ease` with 0.5s delay on fade-in
4. **Session persistence:** `sessionStorage` for auth state (revisit: we use JWT in `localStorage` instead)
5. **Auto-focus:** Username field focused after 500ms delay (allows transition to complete)
6. **Error display:** `.login-error` hidden by default, `.visible` class added on auth failure
7. **Logout:** Fade out content → clear session → reload page (we'll replace with event-driven flow)

## Appendix B: Reference — Hydrogen JWT Claims Structure

From `elements/001-hydrogen/hydrogen/src/api/auth/auth_service.h`:

```c
typedef struct {
    char* iss;        // Issuer
    char* sub;        // Subject (user ID)
    char* aud;        // Audience (app ID)
    time_t exp;       // Expiration time
    time_t iat;       // Issued at time
    time_t nbf;       // Not before time
    char* jti;        // JWT ID
    int user_id;      // User ID
    int system_id;    // System ID
    int app_id;       // Application ID
    char* username;   // Username
    char* email;      // Email address
    char* roles;      // User roles
    char* ip;         // Client IP address
    char* tz;         // Client timezone
    int tzoffset;     // Timezone offset (minutes from UTC)
    char* database;   // Database name
} jwt_claims_t;
```

Login request body: `{ login_id, password, api_key, tz, database }`

Login response: `{ token, expires_at, user_id, roles }`

Other endpoints: `/api/auth/logout` (POST), `/api/auth/renew` (POST),
`/api/auth/register` (POST)

## Appendix C: Reference — Acuranzo CSS Patterns

Patterns from `src/acuranzo.css` that should inform our CSS architecture
(these are not copied directly, but the concepts are valuable):

- **CSS variable naming:** `--ACZ-color-*`, `--ACZ-font-*`, `--ACZ-border-*`,
  `--ACZ-margin-*`, `--ACZ-transition-*` — we use a similar but simplified
  prefix-free convention
- **Layout sections:** `#divLeft`, `#divRight` with flex — our sidebar +
  workspace
- **Title component:** `.Title`, `.Title1`, `.Title2`, `.Title3` — graduated
  heading sizes with themed backgrounds
- **Button hierarchy:** `.HugeButton`, `.BigButton`, `.RegButton`,
  `.SmallButton` — we'll use `.btn-*` size modifiers
- **Scrollbar styling:** Custom WebKit scrollbar with themed colors
- **Wait cursor:** `body.wait *{ cursor: wait }` — useful for loading states
- **Splitter:** Invisible but functional drag handles between panels
- **Menu system:** `.Menu`, `.MenuItem` with positioned overlays
- **Transition system:** `--ACZ-transition-std`, `--ACZ-transition-quick`,
  `--ACZ-transition-panel` — multiple transition speeds

## Appendix D: Planned Hydrogen Server Changes

These are changes needed in the Hydrogen C server to fully support Lithium.
They are tracked here so we know what to implement when ready; none are
blocking for Phase 1–3.

| Change | Priority | Description |
|--------|----------|-------------|
| `lang` JWT claim | Medium | Add preferred language to JWT claims; used by lookups filtering |
| `punchcard` JWT claim | Medium | Add `{ managers: [...], features: {...} }` to JWT; used by permissions system |
| HttpOnly cookie auth | Low | Set JWT via `Set-Cookie: HttpOnly; Secure; SameSite=Strict` instead of JSON body; browser auto-sends |
| Lookups API | Medium | `GET /api/lookups` returning manager names, feature labels, status values, filtered by `lang` |
| Theme CRUD API | Medium | `GET/POST/PUT/DELETE /api/themes` for Style Manager |
| User preferences API | Medium | `GET/PUT /api/user/preferences` for Profile Manager |

---

## Appendix E: Implementation Notes and Learnings

This section captures insights gained during implementation that may be helpful for future development.

### CSS Architecture Decisions

**Variable Naming Convention**

- Used semantic naming (e.g., `--bg-primary`, `--text-secondary`) rather than color-based names (e.g., `--color-blue`)
- This allows the Style Manager to override variables meaningfully without breaking the design system
- Added expanded variable set beyond the initial blueprint: `--bg-hover`, `--bg-active`, `--text-disabled`, etc.

**Font Loading Strategy**

- Implemented `local()` first with WOFF2 fallback in `@font-face` declarations
- This allows users with Vanadium fonts installed locally to avoid downloads
- Font files are in `public/assets/fonts/` for static serving

**CSS Organization**

- Separated concerns into four files: base (variables/reset), layout (structural), components (UI elements), transitions (animations)
- Each file is independently importable if managers need specific subsets
- All files use CSS variables exclusively — no hardcoded colors anywhere

### Core Module Design Patterns

**Event Bus Implementation**

- Extended native `EventTarget` rather than creating a custom pub/sub
- This ensures compatibility with standard DOM event patterns
- Added `once()` method for one-time listeners (useful for initialization)
- Exported `Events` object provides event name constants to prevent typos

**JWT Utilities**

- Client-side JWT handling is intentionally minimal: decode only, no signature verification
- Server must always validate tokens; client validation is for UX only
- `getRenewalTime()` calculates 80% of token lifetime for proactive renewal
- Clock skew tolerance (60 seconds) prevents edge-case expiry issues

**JSON Request Wrapper**

- Returns parsed JSON directly rather than Response objects
- Automatically attaches JWT from localStorage via `Authorization: Bearer` header
- Emits `auth:expired` event on 401 for global handling
- Error objects include `status`, `data`, and `retryAfter` (for 429 responses)

**Permissions System**

- Implemented with fallback strategy: if no punchcard in JWT, grant all access
- This allows development without waiting for Hydrogen punchcard implementation
- Functions accept optional `punchcard` parameter for testing flexibility

**Config Loader**

- Uses deep merge with defaults to handle partial config files gracefully
- Caches config after first load to avoid repeated fetches
- Provides `getConfigValue()` for dot-notation access (e.g., `getConfigValue('server.url')`)
- Config file is in `config/` directory which should be served as static files

### Testing Strategy Notes

**Vitest + Happy-DOM Choice**

- Selected `happy-dom` over `jsdom` for better ESM support and faster execution
- Vitest aligns with Vite ecosystem for consistent build/test experience
- Coverage provider set to `v8` for accurate native ES module coverage

### Known Limitations / Future Improvements

1. **Config Loading**: Currently fetches `/config/lithium.json` relative to root. May need adjustment for deployments with path prefixes.

2. **Permissions Fallback**: The allow-all fallback is convenient for development but should be removed or made configurable before production.

3. **Error Handling**: json-request.js emits events but doesn't provide a way to suppress them for specific requests. May need `silent: true` option.

4. **CSS Variables**: The variable set is comprehensive but may need expansion for specific manager requirements (e.g., data visualization colors).

### Validation Checklist for Current State

To verify the foundation is working correctly:

```bash
# 1. Install dependencies
npm install

# 2. Run linting on new files
npx eslint src/core/*.js src/styles/*.css

# 3. Verify config file is valid JSON
node -e "console.log(JSON.parse(require('fs').readFileSync('./config/lithium.json')))"

# 4. Check that all imports resolve
npx vite build --mode development 2>&1 | head -50

# 5. Run unit tests (when written)
npx vitest run
```

### Next Steps Priority

Based on implementation progress, the next session should focus on:

1. **Test login flow**: Verify login → main menu flow with real or mocked Hydrogen server
2. **Implement Style Manager**: Build theme list with Tabulator, CSS editor with CodeMirror
3. **Add unit tests**: Write Vitest tests for core modules (JWT, permissions, event bus)
4. **Create utils.js**: Add date/number formatters using Intl.* APIs
5. **Update documentation**: Rewrite INSTRUCTIONS.md and README.md for new architecture

### Progress Summary (2025-03-05)

**Completed:**
- ✅ Phase 1: Foundation (index.html, app.js, core modules, CSS architecture)
- ✅ Phase 2: Login Manager (CCC-style, Hydrogen API, error handling)
- ✅ Phase 3: Main Menu Manager (sidebar, header, workspace, responsive)
- ✅ Phase 4/5: Placeholder managers for all registered managers

**Architecture in place:**
- Event-driven architecture with EventBus
- JWT handling with auto-renewal at 80% lifetime
- Permission system with punchcard fallback
- Manager lazy loading with state preservation
- FOUC prevention and fade transitions
- Responsive layout with mobile support

**Ready for testing with a Hydrogen server.**

---

## Appendix F: Test and Coverage Assessment (2026-03-05)

### Test Execution Status

**54 tests, 0 failures** across 3 test files:

| Test File | Tests | Status |
|-----------|-------|--------|
| `tests/unit/event-bus.test.js` | 6 | ✅ All passing |
| `tests/unit/jwt.test.js` | 25 | ✅ All passing |
| `tests/unit/permissions.test.js` | 23 | ✅ All passing |

### Coverage Report (new architecture files only)

| File | Stmts | Branch | Funcs | Lines | Notes |
|------|-------|--------|-------|-------|-------|
| **event-bus.js** | 100% | 100% | 100% | 100% | Fully covered |
| **jwt.js** | 94% | 90% | 100% | 94% | Uncovered: `atob` error catch, `Buffer` fallback, no-decoder-available throw (defensive paths) |
| **permissions.js** | 96% | 94% | 83% | 96% | Uncovered: internal `getPunchcard()` stub (returns null, unused) |
| **config.js** | 0% | 0% | 0% | 0% | No tests written yet |
| **json-request.js** | 0% | 0% | 0% | 0% | No tests written yet |
| **utils.js** | 0% | 0% | 0% | 0% | No tests written yet |
| **All managers** | 0% | 0% | 0% | 0% | No tests — managers require DOM/integration tests |
| **shared/lookups.js** | 0% | 0% | 0% | 0% | No tests written yet |
| **Overall** | 19% | 80% | 61% | 19% | Across all new architecture files |

### Infrastructure Fixes Applied

1. **Fixed no-op test:** `isExpired` clock skew test had no assertions — replaced
   with proper test that validates clock skew window behavior using `vi.useFakeTimers()`
2. **Cleaned coverage config:** Updated `vitest.config.js` to exclude legacy files
   (`src/modules/`, `src/init/`, `src/core/logger/`, `src/core/network/`,
   `src/core/router/`, `src/core/storage/`) from coverage report so numbers
   reflect the actual new architecture
3. **Fixed test scripts:** Updated `package.json` test scripts from
   `cd tests && npm test` (Mocha) to `vitest run` (Vitest)
4. **Installed coverage provider:** Added `@vitest/coverage-v8@^1.6.1` as devDependency

### Honest Assessment: What's Real vs. What's Claimed

**What's genuinely solid:**
- The 3 tested core modules (event-bus, jwt, permissions) are well-tested with
  meaningful assertions. Tests cover happy paths, edge cases, error handling,
  type coercion (string IDs), and fallback behavior.
- The architecture is clean: EventTarget-based event bus, proper JWT decode-only
  approach, punchcard permissions with graceful fallback.
- CSS architecture is complete with variables, responsive layout, dark theme defaults.

**What exists but is untested:**
- `config.js` — config loading, deep merge, dot-notation access. All pure functions,
  easily unit-testable. The `fetch` call needs mocking.
- `json-request.js` — fetch wrapper with auth headers, error handling, event bus
  integration. Needs `fetch` mocking and event bus spy assertions.
- `utils.js` — date/number/currency formatting with `Intl.*` APIs, debounce,
  throttle, deep clone, HTML escaping. Mostly pure functions.
- `shared/lookups.js` — lookup caching and getters. Pure functions + one fetch call.

**What exists as structure only (placeholder managers):**
- `managers/dashboard/index.js` — placeholder stub
- `managers/lookups/index.js` — placeholder stub
- `managers/queries/index.js` — placeholder stub
- `managers/style-manager/index.js` — placeholder stub
- `managers/profile-manager/index.js` — placeholder stub
- `managers/login/login.js` — full implementation, untested (DOM-heavy)
- `managers/main/main.js` — full implementation, untested (DOM-heavy)

**What still exists from the old architecture and should eventually be removed:**
- `src/modules/` — entire directory is the old module system, now replaced by
  `src/managers/`. Contains old login, main, dashboard, lookups, queries,
  source-editor modules. Should be deleted once new managers are verified working.
- `src/core/logger/`, `src/core/network/`, `src/core/router/`, `src/core/storage/`
  — old core infrastructure marked 🔄 in the folder structure (Section 4). Need
  rework to align with new architecture.
- `tests/package.json` — still references Mocha/Chai/NYC. The shell scripts for
  CI/CD linting are fine but the JS test infrastructure is now handled by vitest
  at the root level.

### Recommended Next Steps (Priority Order)

1. **Write unit tests for `config.js`** — Mock `fetch`, test `loadConfig()` with
   successful response, failed response, partial config, `getConfigValue()` with
   dot-notation, `clearConfig()`. This is the easiest win: pure functions + one
   fetch mock. Target: 90%+ coverage.

2. **Write unit tests for `json-request.js`** — Mock `fetch` and `localStorage`,
   test `get/post/put/del/patch` methods, verify auth header attachment, test
   401 → `auth:expired` event emission, test 429 with `retry-after`, test network
   errors. Target: 80%+ coverage.

3. **Write unit tests for `utils.js`** — Test `formatDate`, `formatTime`,
   `formatNumber`, `formatCurrency`, `formatPercent`, `formatRelativeTime` with
   mocked preferences. Test `debounce`, `throttle` with fake timers. Test
   `deepClone`, `escapeHtml`, `generateId`. Target: 90%+ coverage.

4. **Write unit tests for `shared/lookups.js`** — Test `fetchLookups` with mock
   fetch, test cache behavior, test `getLookup` and `getManagerName` getters.

5. **Delete `src/modules/` directory** — The old module system is fully replaced
   by `src/managers/`. Keeping it around creates confusion and inflates the
   codebase. The README files in those directories have useful documentation
   that should be extracted to the new `managers/*/README.md` files first.

6. **Integration tests for login flow** — Mock Hydrogen API responses, test the
   full login → JWT storage → main menu transition. Requires DOM environment
   (happy-dom) and fetch mocking.

7. **Rework legacy core modules** — `logger.js`, `network.js`, `router.js`,
   `storage.js` are listed as 🔄 in the blueprint. Decide: rework to fit new
   architecture or remove and rebuild when needed.

8. **Style Manager implementation** — Phase 4 is "in progress" but only has a
   placeholder `index.js`. This is the first real functional manager and the
   main unfinished piece of the application.
