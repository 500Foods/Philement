# Lithium Blueprint

Architectural blueprint for the Lithium web application. All implementation
work should trace back to this document.

---

## 1. Project Goal

Lightweight, performant, modular SPA using vanilla JavaScript ES modules.
No UI frameworks, no Bootstrap. Dark-mode-first custom CSS with CSS variables,
designed for the Style Manager theming system from day one.

- **Targets:** Desktop browsers + iPad; Android PWA as-is
- **Backend:** Hydrogen (C server, REST API, JWT auth, libwebsockets)
- **Branding:** "Lithium" — "A Philement Project"

---

## 2. Tech Stack

| Category | Choice | Notes |
|----------|--------|-------|
| Language | Vanilla JS (ES modules, async/await, fetch) | No TypeScript |
| Build | Vite (vanilla JS template) | Dev server, hot reload, production build |
| Testing | Vitest + happy-dom | Unit/integration; `@vitest/coverage-v8` |
| Auth | JWT from Hydrogen | localStorage now, HttpOnly cookies later |
| Event system | EventTarget-based event bus | Singleton, cross-module communication |
| Tables | Tabulator (npm) | Dynamic `import('tabulator-tables')` |
| Code editor | CodeMirror 6 (npm) | `@codemirror/*` packages per-mode |
| Sanitization | DOMPurify (npm) | For user-injected content |
| Icons | Font Awesome | CDN Kit-based |
| Fonts | Vanadium Sans + Vanadium Mono | `local()` first, WOFF2 fallback |

**npm-managed:** CodeMirror 6, Tabulator, DOMPurify, Vitest, happy-dom
**CDN:** Font Awesome, Flatpickr (if needed)
**Evaluate per-manager:** Luxon/Intl.*, Highlight.js, SheetJS, Split.js

---

## 3. Terminology

| Term | Definition |
|------|-----------|
| **Module** | An ES module (`.js` file). Building block of the codebase. |
| **Manager** | User-facing feature set with UI. Has init/render/teardown lifecycle. |
| **Menu Manager** | A manager that appears as a named button in the sidebar navigation menu. Permission-gated via Punchcard. |
| **Utility Manager** | A manager bound to a fixed sidebar footer button. Always available regardless of Punchcard; never appears in the menu. |
| **Standalone Manager** | A manager loaded directly via URL parameter, rendered without sidebar/header/footer chrome. |
| **Popup Panel** | A lightweight overlay that does not occupy the workspace. Simpler open/close lifecycle. Not registered as a manager. |
| **Feature** | Capability within a manager, controlled by Punchcard permissions. |
| **Core** | Infrastructure modules managers depend on: event bus, config, JWT, etc. |

Managers require one or more modules. Not all modules are managers. Managers
are independently loadable via dynamic `import()`. The manager *type* determines
how it is surfaced to the user — not how it is coded. All three manager types
share the same `init()` / `render()` / `teardown()` lifecycle contract.
Popup Panels are not managers — they have a simpler `open()` / `close()` / `destroy()`
contract and are not registered in any manager registry.

---

## 3a. Manager Types

### Menu Managers (permission-gated, sidebar navigation)

Menu managers appear as labelled icon buttons in the scrollable sidebar `<nav>`.
The set of visible buttons is built from `getPermittedManagers()`, which reads
the Punchcard JWT claim (or allows all when Punchcard is absent). Clicking a
button lazy-loads the manager into the workspace. State is preserved on switch
(hide/show, not destroy).

Current menu managers registered in `app.js` `managerRegistry`:

| ID | Name | Status |
|----|------|--------|
| 1 | Style Manager | ✅ Implemented |
| 2 | Profile Manager | ⬜ Placeholder |
| 3 | Dashboard | ⬜ Placeholder |
| 4 | Lookups | ⬜ Placeholder |
| 5 | Queries | ⬜ Placeholder |

### Utility Managers (always-present, sidebar footer)

Utility managers are bound to specific icon buttons in the sidebar footer row.
They are **not** Punchcard-gated and **do not** appear in the sidebar menu.
They are loaded on demand (click or keyboard shortcut) and rendered in the
workspace the same way menu managers are. They are registered in a separate
`utilityManagerRegistry` in `app.js` (TBD) to keep Punchcard IDs separate.

Planned utility managers and their sidebar footer bindings:

| Footer Button | Icon | Manager | Status |
|--------------|------|---------|--------|
| Session Log | `fa-scroll` | `session-log/` | ✅ Implemented |
| User Profile | `fa-user-cog` | `user-profile/` | ⬜ Placeholder |

> **Note:** The current sidebar footer contains a **Help** button (`fa-circle-question`).
> This will be **replaced** by the Session Log button. Help content will move
> into a help panel within the Session Log manager or a separate inline popup.

### Standalone Managers (URL-driven, no chrome)

A standalone manager is loaded when a URL parameter requests it directly,
e.g. `lithium.philement.com?USER=x&PASS=y&manager=AccountManager`. In this
mode the app skips the Main Manager entirely and renders only the requested
manager, with no sidebar, header, or footer. The workspace fills the full
viewport. This is useful for embedding Lithium panels in external dashboards
or kiosk displays.

Standalone mode is not yet implemented. When it is:

- `app.js` reads `?manager=<name>` after successful auth
- If present, `loadStandaloneManager(name)` is called instead of `loadMainManager()`
- The manager must be listed in `managerRegistry` or `utilityManagerRegistry`
- No `MainManager` is instantiated; no sidebar/header DOM is created

---

## 3b. Popup Panels

Popup panels are lightweight overlays that appear on top of the current view
without replacing the workspace content. They are **not managers** — they do
not have the `init()`/`render()`/`teardown()` lifecycle, are not registered in
any manager registry, and are not Punchcard-gated (though individual panels
may apply their own access logic).

### Characteristics

| Property | Value |
|----------|-------|
| DOM location | Fixed overlay layer (`document.body` child), not the workspace |
| Lifecycle | `open()` / `close()` / `destroy()` — simpler than manager contract |
| Registration | None — instantiated directly by the triggering component |
| Punchcard | Not gated (access logic is the panel's own concern) |
| Multiple instances | Not supported (popup replaces itself on re-open) |
| Keyboard | Each panel installs/removes its own `keydown` listener |

### Existing Popup: Logout Panel

Already implemented in `src/managers/main/`. Proof-of-concept for the pattern:

- Fixed overlay + panel DOM appended to `document.body` in `render()`
- `showLogoutPanel()` / `hideLogoutPanel()` methods on `MainManager`
- Keyboard: ESC cancels, Enter = Quick logout, `1`-`4` select option
- Removes DOM nodes from `document.body` on `teardown()`

### Planned Popup: Manager Switcher

Shows all currently loaded (cached) manager instances across all manager types.
Allows the user to switch between multiple instances of the same manager type,
e.g. two Accounts Managers open at once.

Key design decisions:

- Triggered by a keyboard shortcut or a dedicated button (TBD)
- Lists instances grouped by manager type (icon + name + instance label)
- Clicking an entry calls `app.showManager(managerId, instanceId)` to make it visible
- The manager loading system must be extended to support multiple instances per
  manager ID (currently `loadedManagers` is a `Map<id, instance>` — needs to
  become `Map<id, instance[]>` or a keyed structure)
- Instance labels could be user-assigned or auto-generated (e.g. "Accounts #1")

Implementation is deferred until a menu manager that benefits from multiple
instances is being built.

### Planned Popup: Annotations

A compact notes/scratch-pad overlay. Not a full manager — no persistent server
storage required initially. Intended as a contextual tool that can be opened
from within any manager.

- Triggered on-demand (keyboard shortcut or in-manager button — TBD)
- Not bound to the sidebar footer
- Simple text area or lightweight editor (not CodeMirror-heavy)
- Annotations scoped to: global session, current manager type, or current record
- May be promoted to a full menu manager later if server-side storage is added

### Adding a New Popup Panel

1. Create `src/panels/popup-name/popup-name.js` (and `.html`, `.css` if needed)
2. Export a class with `open(anchorOrOptions?)`, `close()`, `destroy()`
3. In `open()`: create DOM, append to `document.body`, add keyboard listener
4. In `close()`: hide DOM (opacity/display), remove keyboard listener
5. In `destroy()`: remove DOM from `document.body`, clean up all refs
6. Instantiate from the triggering component (manager, `app.js`, or another panel)
7. Store the instance on the triggering component so `destroy()` can be called
   during that component's `teardown()`

---

## 4. Current File Structure

```structure
elements/003-lithium/
├── index.html                    ✅ Stripped of Bootstrap, clean
├── package.json                  ✅ Vitest 4.x, happy-dom 20.x, ESLint 10
├── version.json                  ✅ Build number, timestamp, version string
├── vite.config.js                ✅
├── vitest.config.js              ✅
├── eslint.config.js              ✅ Flat config
├── config/lithium.json           ✅ Runtime config
├── scripts/
│   └── bump-version.js           ✅ Auto-increment build on deploy
│
├── src/
│   ├── app.js                    ✅ Bootstrap + manager loader
│   ├── core/
│   │   ├── event-bus.js          ✅ 100% coverage
│   │   ├── jwt.js                ✅ 75% coverage
│   │   ├── json-request.js       ✅ 40% coverage
│   │   ├── permissions.js        ✅ 96% coverage
│   │   ├── config.js             ✅ 100% coverage
│   │   └── utils.js              ✅ 88% coverage
│   ├── shared/
│   │   └── lookups.js            ✅ 37% coverage
│   ├── styles/
│   │   ├── base.css              ✅ Variables, reset, font-face, dark defaults
│   │   ├── layout.css            ✅ Login, sidebar, workspace, header
│   │   ├── components.css        ✅ Buttons, inputs, cards, tables, modals
│   │   └── transitions.css       ✅ Fade, crossfade, reduced-motion
│   ├── managers/
│   │   ├── login/                ✅ Full implementation
│   │   ├── main/                 ✅ Full implementation
│   │   ├── style-manager/        ⬜ Placeholder stub
│   │   ├── profile-manager/      ⬜ Placeholder stub
│   │   ├── dashboard/            ⬜ Placeholder stub
│   │   ├── lookups/              ⬜ Placeholder stub
│   │   └── queries/              ⬜ Placeholder stub
│   └── init/                     ✅ Third-party lib wrappers (kept as-is)
│
├── tests/
│   └── unit/                     ✅ 171 tests, 0 failures
│       ├── event-bus.test.js     6 tests
│       ├── jwt.test.js           25 tests
│       ├── permissions.test.js   23 tests
│       ├── config.test.js        13 tests
│       ├── utils.test.js         38 tests
│       ├── json-request.test.js  14 tests
│       ├── lookups.test.js       8 tests
│       ├── icons.test.js         26 tests
│       └── auth.integration.test.js  12 tests
│
└── public/
    ├── manifest.json             ✅ Lithium branding, dark theme
    ├── service-worker.js         ✅ Cache-first statics, build-versioned caches
    ├── version.json              ✅ Copied from root by bump-version script
    ├── assets/
    │   ├── fonts/                ✅ Vanadium WOFF2 files
    │   └── images/               ✅ logo-li.svg, logo-192x192.png, logo-512x512.png
    └── src/managers/            ✅ HTML templates for production deploy
        ├── login/login.html      ✅ Copied from src/ for runtime fetch
        ├── main/main.html        ✅ Copied from src/ for runtime fetch
        └── style-manager/        ✅ style-manager.html copied for runtime fetch
```

---

## 5. Configuration

`config/lithium.json` — loaded at startup before auth. Public file, no secrets.

```json
{
  "server": { "url": "https://hydrogen.example.com", "api_prefix": "/api", "websocket_url": "wss://..." },
  "auth": { "api_key": "public-app-id", "default_database": "Demo_PG", "session_timeout_minutes": 30 },
  "app": { "name": "Lithium", "tagline": "A Philement Project", "version": "1.0.0", "logo": "/assets/images/logo-li.svg", "default_theme": "dark", "default_language": "en-US" },
  "features": { "offline_mode": true, "install_prompt": true, "debug_logging": false }
}
```

`config.js` deep-merges with defaults. Cached after first load. `getConfigValue('server.url')` for dot-notation access.

---

## 6. Bootstrap Flow

```flow
1. index.html → <script type="module" src="/src/app.js">
2. app.js: Load version.json (build number, timestamp)
3. app.js: Load config/lithium.json
4. app.js: Initialize core modules
5. app.js: Reveal page (FOUC prevention — visibility: hidden → visible)
6. app.js: Check JWT in localStorage → valid? → load Main Manager
7. No/expired JWT → load Login Manager
8. Login success → store JWT, fire auth:login, schedule renewal, load Main Manager
9. Main Manager: build sidebar from permitted managers, render header
10. Menu clicks → lazy import() manager → init/render in workspace
11. Manager state preserved on switch (hide/show, not destroy)
```

---

## 7. Authentication and JWT

### Hydrogen Login API

`POST /api/auth/login` with `{ login_id, password, api_key, tz, database }`
Response: `{ token, expires_at, user_id, roles }`

Other endpoints: `/api/auth/logout` (POST), `/api/auth/renew` (POST), `/api/auth/register` (POST)

### JWT Claims (from Hydrogen)

Standard: `iss`, `sub`, `aud`, `exp`, `iat`, `nbf`, `jti`
Custom: `user_id`, `system_id`, `app_id`, `username`, `email`, `roles`, `ip`, `tz`, `tzoffset`, `database`

**Planned additions** (needs Hydrogen changes): `lang` (preferred language), `punchcard` (permissions)

### Client-side JWT Handling

1. **Decode** — Base64url payload, no signature verification (server validates)
2. **Validate expiry** — Check `exp` with 60s clock skew tolerance
3. **Store** — `localStorage` (future: HttpOnly cookies)
4. **Attach** — `Authorization: Bearer <token>` via json-request.js
5. **Renew** — Timer at 80% of token lifetime → POST `/api/auth/renew`
6. **Revoke** — POST `/api/auth/logout` on explicit logout

---

## 8. Permissions (Punchcard)

> Punchcard is planned for a future Hydrogen update. Until then, Lithium falls
> back to granting access to all managers. Code is structured for drop-in support.

### Punchcard JWT Claim (planned)

```json
{ "punchcard": { "managers": [1, 2, 5], "features": { "1": [1,2,3], "2": [1] } } }
```

### Permission Helpers (`permissions.js`)

- `canAccessManager(id, punchcard?)` → boolean (no punchcard = allow all)
- `hasFeature(managerId, featureId, punchcard?)` → boolean
- `getPermittedManagers(punchcard?)` → array of IDs
- `getFeaturesForManager(managerId, punchcard?)` → array of IDs
- `parsePermissions(claims)` → `{ hasPunchcard, managers, features }`

---

## 9. Event Bus

Singleton `EventBus extends EventTarget` with `emit()`, `on()`, `off()`, `once()`.

### Standard Events

| Event | Payload | When |
|-------|---------|------|
| `auth:login` | `{ userId, username, roles }` | Login success |
| `auth:logout` | `{}` | User logs out |
| `auth:expired` | `{}` | JWT expired, renewal failed |
| `auth:renewed` | `{ expiresAt }` | JWT renewed |
| `permissions:updated` | `{ punchcard }` | Permissions changed |
| `manager:loaded` | `{ managerId }` | Manager finished init |
| `manager:switched` | `{ from, to }` | Active manager changed |
| `theme:changed` | `{ themeId, vars }` | Theme applied |
| `locale:changed` | `{ lang, formats }` | Language/format pref changed |
| `network:online` / `network:offline` | `{}` | Connection change |

---

## 10. CSS Architecture

Four files in `src/styles/`, all using CSS variables. Dark theme is default.
Style Manager overrides variables via injected `<style id="dynamic-theme">`.

- `base.css` — `:root` variables, CSS reset, `@font-face`, dark defaults
- `layout.css` — Login, sidebar + workspace split, header, content
- `components.css` — Buttons, inputs, cards, tables, modals, scrollbars
- `transitions.css` — Fade, crossfade, slide; `prefers-reduced-motion` support

Key variable categories: `--bg-*`, `--text-*`, `--border-*`, `--accent-*`,
`--font-*`, `--space-*`, `--shadow-*`, `--transition-*`, `--scrollbar-*`.
See `base.css` for the full set.

---

## 11. Login Manager ✅ DEPLOYED

Implemented in `src/managers/login/`. CCC-style centered card, dark background,
logo + title gradient header, fade transition to main app.

- POSTs to Hydrogen `/api/auth/login` with config-sourced `api_key`, `database`, auto-detected `tz`
- Handles 401 (bad creds), 429 (rate limit with countdown), 5xx, network errors
- FOUC prevention, auto-focus username, loading spinner on submit
- Fade-out (0.8s) on success before Main Manager loads
- **Production deployed** at <https://lithium.philement.com>

### Deployment Lessons

- HTML templates must be copied to `public/src/managers/{name}/` for runtime fetch
- Vite only bundles JS/CSS; runtime-fetched HTML needs manual copy to public/
- Font Awesome Kit (not CDN with SRI) works better for production

---

## 12. Main Menu Manager ✅

Implemented in `src/managers/main/`. Sidebar + header + workspace layout.

- Sidebar menu buttons generated from `getPermittedManagers()` (menu managers only)
- Click → lazy load → render in workspace; state preserved on switch
- Header: manager name, offline indicator, version box, theme stub, profile button, logout
- Sidebar collapsible (desktop) and overlay-based (mobile)
- Logout: four-option panel (Quick / Normal / Public / Global) with keyboard shortcuts

### Sidebar Footer Buttons (current)

| Position | ID | Icon | Action |
|----------|----|------|--------|
| 0 | `sidebar-collapse-btn` | `fa-angles-left` | Collapse/expand sidebar |
| 1 | `sidebar-theme-btn` | `fa-palette` | Open Style Manager (stub) |
| 2 | `sidebar-profile-btn` | `fa-user-cog` | Open Profile Manager (loads manager 2) |
| 3 | `sidebar-help-btn` | `fa-circle-question` | Help (stub — **to be replaced**) |
| 4 | `sidebar-logout-btn` | `fa-sign-out-alt` | Open logout panel |

### Planned Sidebar Footer Changes

The **Help** button (position 3) will be replaced by the **Session Log** utility
manager button. Help will be surfaced as a panel within the Session Log manager
or as a separate lightweight popup.

When utility managers are implemented the footer will become:

| Position | ID | Icon | Action |
|----------|----|------|--------|
| 0 | `sidebar-collapse-btn` | `fa-angles-left` | Collapse/expand sidebar |
| 1 | `sidebar-theme-btn` | `fa-palette` | Open Style Manager (stub) |
| 2 | `sidebar-profile-btn` | `fa-user-cog` | Open User Profile utility manager |
| 3 | `sidebar-logs-btn` | `fa-scroll` | Open Session Log utility manager |
| 4 | `sidebar-logout-btn` | `fa-sign-out-alt` | Open logout panel |

---

## 12a. Manager Slot Button Injection

Each manager slot (header + workspace + footer) is built by `MainManager.createSlot()`.
The header and footer each use a **single unified** `subpanel-header-group` so that
all buttons appear as one connected strip — no visual breaks.

### Slot Header Layout

```layout
[icon + manager name (flex:1)]  [manager-injected extras]  [close ✕]
                                         ↑
                              .slot-header-extras
                              (display:contents)
```

### Slot Footer Layout

```layout
[Reports Placeholder (flex:1)]  [footer-left extras]  [footer-right extras]  [Annotations]  [Tours]  [LMS]
                                        ↑                      ↑
                              .slot-footer-left-extras  .slot-footer-right-extras
                              (display:contents)
```

Both insertion-point `<div>` elements use `display:contents` so their children
appear directly as flex items inside the shared button group.

### Button Injection API

Managers call these `MainManager` methods from their `init()` (after the slot has
been created by `app.js`):

```javascript
// Get the slot id from inside a manager
const mainMgr = this.app._getMainManager();
// Menu manager:
const slotId = mainMgr._slotId(managerId);
// Utility manager:
const slotId = mainMgr._utilitySlotId(managerKey);  // e.g. 'session-log'

// Inject buttons into the header (between title and close)
mainMgr.addHeaderButtons(slotId, [
  {
    id:      'my-refresh-btn',        // optional DOM id
    icon:    'fa-rotate',             // <fa> icon attribute
    title:   'Refresh',              // HTML title (tooltip on hover)
    tooltip: 'Refresh',              // data-tooltip attribute
    onClick: () => this.refresh(),   // click handler
  },
  // Pre-built element form:
  { el: document.createElement('button') },
]);

// Inject buttons into the footer
mainMgr.addFooterButtons(slotId, 'right', [
  { id: 'export-btn', icon: 'fa-file-export', title: 'Export', onClick: fn },
]);
// side = 'left'  → between Reports button and right extras
// side = 'right' → between left extras and fixed action icons (Annotations/Tours/LMS)
```

`addHeaderButtons` / `addFooterButtons` also call `processIcons()` on the container
after insertion so newly injected `<fa>` tags are converted by Font Awesome.

### Session Log — Reference Implementation

`SessionLogManager._injectSlotHeaderButtons()` is the canonical example. It:

1. Gets `mainMgr` via `this.app._getMainManager()`
2. Derives `slotId = mainMgr._utilitySlotId('session-log')`
3. Calls `mainMgr.addHeaderButtons(slotId, [...])` with the Refresh, Coverage,
   and Clear button descriptors
4. Caches references to the injected elements for use by `refreshLog()` / `clearArchivedSessions()`

The workspace template (`session-log.html`) contains **only** the log viewer —
no separate toolbar `<div>`. This is the expected pattern for all managers.

---

## 13. Style Manager ✅ IMPLEMENTED

Theme management: list, apply, edit, share themes. Fully implemented in
`src/managers/style-manager/` with Tabulator, CodeMirror, and DOMPurify.

### Feature IDs (for Punchcard)

| ID | Description |
|----|-------------|
| 1 | View all styles (read-only) |
| 2 | Edit all styles |
| 3 | Use visual editor |
| 4 | Use CSS code editor |
| 5 | Delete themes |

### Layout — three views

1. **List View** — Tabulator table (name, date, owner, actions). New/Apply/Edit/Delete.
2. **CSS View** — CodeMirror with CSS mode. Load/edit raw CSS. Apply + Save buttons.
3. **Visual Editor** (Phase 7) — Preview scene + inspector panel. Deferred.

### Theme Data Model

```json
{
  "id": 42, "name": "Midnight Indigo", "owner_id": 7,
  "shared_with": [3, 12], "public": false,
  "variables": { "--bg-primary": "#0D1117", "--accent-primary": "#58A6FF" },
  "raw_css": "/* optional overrides */",
  "created_at": "2025-01-15T10:30:00Z", "updated_at": "2025-03-01T14:22:00Z"
}
```

### Theme Application Flow

1. Build CSS from `variables` → `:root { --var: value; ... }`
2. Append `raw_css` if present
3. Sanitize with DOMPurify
4. Inject into `<style id="dynamic-theme">` (create or replace)
5. Persist active theme ID in user preferences
6. Fire `theme:changed` event

### Implementation Status ✅

- [x] Create `style-manager/index.js` with init/render/teardown
- [x] Create `style-manager/style-manager.html` and `.css`
- [x] **List View:** Tabulator table, New/Apply/Edit/Delete actions
- [x] **Theme Application:** `applyTheme()`, DOMPurify, dynamic style injection
- [x] **CSS View:** CodeMirror editor, Apply/Save buttons
- [x] Feature-gated UI (check `hasFeature()`)
- [ ] Theme data loaded from Hydrogen API (waiting for backend endpoint)
- [x] `theme:changed` event on apply
- [ ] Unit tests for theme application and CSS sanitization

---

## 14. Profile Manager ⬜

User preferences. Currently a placeholder stub.

### Settings

| Setting | Implementation |
|---------|---------------|
| Language | Save → new JWT with `lang` → refetch lookups → rebuild menu |
| Date format | `Intl.DateTimeFormat` with pref-based options |
| Time format | 12h/24h via `hour12` option |
| Number format | `Intl.NumberFormat` with pref-based options |
| Default database | Stored in prefs |
| Active theme | Delegates to Style Manager's apply function |

### Save Flow

1. User changes preference
2. POST to Hydrogen API
3. Server issues new JWT (if `lang`/perms changed)
4. Client stores new JWT
5. Fire `locale:changed` or `theme:changed`
6. Subscribed modules react

### Profile Checklist

- [ ] Create `profile-manager/index.js` with init/render/teardown
- [ ] Create `profile-manager/profile-manager.html` and `.css`
- [ ] Language, date, time, number format selectors
- [ ] Active theme quick-apply dropdown
- [ ] Save → API call → JWT refresh if needed
- [ ] `locale:changed` event on save

---

## 14a. Session Log Manager ✅ IMPLEMENTED (Utility Manager)

The Session Log manager is a **utility manager** — always available from the
sidebar footer regardless of Punchcard. It replaces the current Help button.

### Purpose

Display the current session's client-side action log in a full-featured
CodeMirror viewer, with the same rendering as the login panel's System Logs
subpanel but with additional capabilities appropriate for an authenticated
session.

### Relationship to Login Panel Logs

The login panel already has a System Logs subpanel (accessible via `fa-scroll`
before login). The Session Log manager is the post-login equivalent. Both read
from the same `src/core/log.js` buffer. Differences:

| Aspect | Login panel subpanel | Session Log manager |
|--------|---------------------|---------------------|
| Trigger | Button in login footer | Sidebar footer button (always) |
| Location | Replaces login form | Loads in workspace |
| Header bar | Coverage + Close buttons | Full subpanel header with more actions |
| Auto-refresh | No (snapshot on open) | Yes — live tail / refresh button |
| Coverage link | Yes (`fa-chart-line`) | Yes |
| Server log browsing | No | Future (Session Logs Manager, separate menu manager) |

### Layout

Single view with a CodeMirror read-only editor (same config as login subpanel:
`oneDark` theme, `Vanadium Mono` font, 4-digit zero-padded line numbers).

Header bar:

| Button | Icon | Action |
|--------|------|--------|
| Title | `fa-scroll` Session Log | Non-interactive label |
| Refresh | `fa-rotate` | Reload log from buffer |
| Coverage | `fa-chart-line` | Open `/coverage/index.html` in new tab |
| Clear archived | `fa-trash-can` | Remove previous-session archives from localStorage |

### Implementation Checklist

- [x] Create `src/managers/session-log/index.js` with init/render/teardown
- [x] Create `src/managers/session-log/session-log.html` and `.css`
- [x] Replace Help button in `main.html` sidebar footer with Session Log button
- [x] Wire `sidebar-logs-btn` click to load Session Log utility manager
- [x] Register in `app.js` utility manager registry (separate from Punchcard IDs)
- [x] CodeMirror read-only viewer (reuse login panel approach)
- [x] Refresh button — re-reads `getRawLog()` and updates editor content
- [x] Coverage link button
- [x] Clear archived sessions button (`window.lithiumLogs.archived`)
- [x] Add `Subsystems.SESSIONLOG` constant to `log.js`
- [x] Log all button interactions via `log(SESSIONLOG, ...)` pattern

---

## 15. Lookups

Server-provided reference data (manager names, feature labels, etc.).
Implemented in `src/shared/lookups.js`.

- `fetchLookups()` → GET `/api/lookups`, cache in memory
- `getLookup(category, key)`, `getManagerName(id)`, `getFeatureName(mgr, feat)`
- Auto-refresh on `locale:changed`, clear on `auth:logout`
- Fallback: returns key if lookup not found

---

## 16. Testing

### Current Status: 171 tests, 0 failures

| File | Stmts | Branch | Tests |
|------|-------|--------|-------|
| event-bus.js | 100% | 100% | 6 |
| config.js | 100% | 90% | 13 |
| permissions.js | 96% | 95% | 23 |
| utils.js | 88% | 86% | 38 |
| jwt.js | 75% | 73% | 25 |
| json-request.js | 40% | 31% | 14 |
| lookups.js | 37% | 18% | 8 |

### Coverage Dashboard

Coverage reports are automatically generated and deployed to production:

```bash
npm run test:coverage   # Generate coverage report locally
npm run coverage:copy   # Copy to public/ for deployment
```

**Live Coverage Dashboard:** <https://lithium.philement.com/coverage/>

The dashboard includes:

- Interactive file tree with coverage percentages
- Line-by-line highlighting (green = covered, red = uncovered)
- Summary statistics (statements, branches, functions, lines)
- Sortable tables and file search
- Drill-down into individual source files

### Still Needed

- [ ] Integration test: login flow with mocked Hydrogen responses
- [ ] Integration test: manager loading into workspace
- [ ] Integration test: menu generation from permissions
- [ ] Improve json-request.js coverage (error handling paths)
- [ ] Improve lookups.js coverage (fetch + cache behavior)

### Testing Notes

- happy-dom over jsdom for better ESM support
- ES module mocking: use `vi.hoisted()` or `vi.mock()` at file top
- `Object.defineProperty(global, 'localStorage', { value: mock })` for storage
- DOM-heavy managers (login, main) need integration tests, not unit tests

---

## 17. PWA

- [x] Update `manifest.json` — Lithium branding, dark theme, SVG + PNG icons
- [x] Update `service-worker.js` — cache-first statics, stale-while-revalidate API
- [x] Versioned cache names with cleanup on `activate`
- [x] Generate 192x192 + 512x512 PNG icons from logo SVG
- [x] Copy `logo-li.svg` to `public/assets/images/`
- [ ] Offline indicator in header bar (wiring exists in main.js, needs visual polish)

---

## 18. Security and Polish

- [ ] DOMPurify for all user HTML/CSS injection (Style Manager)
- [ ] No `innerHTML` with unsanitized user data
- [ ] CSP headers (configure on Hydrogen)
- [ ] Semantic HTML, ARIA attributes, keyboard navigation
- [ ] Focus trap in modals, visible focus indicators
- [ ] Error boundary: `window.onerror` + `unhandledrejection`
- [ ] `@media (prefers-reduced-motion: reduce)` ✅ already in transitions.css

---

## 19. Deployment

### Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `LITHIUM_ROOT` | Project source directory | `/mnt/extra/Projects/Philement/elements/003-lithium` |
| `LITHIUM_DEPLOY` | Web server document root | `/fvl/tnt/t-philement/lithium` |
| `LITHIUM_DOCS_ROOT` | Documentation directory | `/mnt/extra/Projects/Philement/docs/Li` |

All build and deploy scripts use these environment variables so nothing is
hardcoded to a particular machine or directory layout.

### Commands

```bash
npm run build           # Vite build → $LITHIUM_ROOT/dist/
npm run build:prod      # Production build + HTML/JS minification → dist/
npm run preview         # Preview production build locally
npm run deploy          # Test + bump version + build + deploy to $LITHIUM_DEPLOY
npm run version:bump    # Increment build number, update SW cache version
npm run test:coverage   # Run tests with coverage report → coverage/
npm run coverage:copy   # Copy coverage report to public/ for deployment
```

### Deploy Flow

1. `predeploy` — Validates `$LITHIUM_DEPLOY` is set
2. `test:coverage` — Runs full test suite with coverage report
3. `coverage:copy` — Copies coverage report to `public/` for deployment
4. `templates:copy` — Copies manager HTML templates to `public/` for deployment
5. `version:bump` — Increments build number in `version.json`, updates
   service worker `CACHE_VERSION`, copies `version.json` to `public/`
6. `vite build --mode deploy` — Builds directly to `$LITHIUM_DEPLOY`
   (does **not** empty the directory, preserving runtime config)
7. `deploy:config` — Copies `config/lithium.json` only if not already present
8. `deploy:postbuild` — Minifies HTML and service worker in-place

### Config Handling

`config/lithium.json` is **runtime config**, not bundled by Vite. On first
deploy it is seeded from the source copy. On subsequent deploys it is
preserved so any environment-specific edits (server URL, API key, database)
are not overwritten.

### Output Structure

```structure
$LITHIUM_DEPLOY/
├── index.html                    Minified entry point
├── manifest.json                 PWA manifest
├── service-worker.js             Minified SW (CACHE_VERSION = build number)
├── version.json                  Build number + timestamp (updated each deploy)
├── favicon.ico
├── config/
│   └── lithium.json              Runtime config (not overwritten on redeploy)
├── assets/
│   ├── fonts/                    Vanadium WOFF2
│   ├── images/                   Logo SVG + PNG icons
│   ├── index-[hash].css          Combined CSS
│   ├── index-[hash].js           Core + shared modules
│   ├── login-[hash].js           Login manager chunk
│   └── main-[hash].js            Main manager chunk
├── src/managers/                 HTML templates for runtime fetch
│   ├── login/login.html
│   ├── main/main.html
│   └── style-manager/style-manager.html
└── coverage/                     Test coverage report (if generated)
    ├── index.html                Coverage dashboard
    ├── coverage-final.json       Raw coverage data
    ├── core/                     Core module coverage
    ├── managers/                 Manager coverage
    └── shared/                   Shared module coverage
```

### Test Coverage Dashboard

The coverage report generated by `@vitest/coverage-v8` is deployed alongside the application at `/coverage/`:

- **URL:** `https://lithium.philement.com/coverage/`
- **Features:**
  - File tree with coverage percentages
  - Line-by-line highlighting (covered/uncovered)
  - Summary statistics (statements, branches, functions, lines)
  - Sortable tables and search
  - Drill-down into individual files

**To deploy with coverage:**

```bash
npm run test:coverage   # Generate coverage report
npm run deploy          # Coverage automatically copied to public/
```

Serve from Hydrogen or any static server. HTTPS required for PWA.

---

## 20. Implementation Phases

### Phase 0: Blueprint ✅

### Phase 1: Foundation ✅

Core modules, CSS architecture, Vite/Vitest setup, Bootstrap removal.

### Phase 2: Login ✅

CCC-style login against Hydrogen API with error handling and transitions.

### Phase 3: Main Layout ✅

Sidebar + header + workspace, manager lazy loading, responsive.

### Phase 4: Style Manager ✅ COMPLETE

- [x] List View with Tabulator
- [x] Apply theme from list
- [x] CSS View with CodeMirror
- [ ] Theme data model and API integration (waiting for Hydrogen endpoint)
- [x] DOMPurify sanitization

### Phase 4a: Utility Managers & Sidebar Footer ✅ COMPLETE

Introduced the utility manager concept and delivered the first utility manager.

- [x] Add `utilityManagerRegistry` to `app.js` (separate from Punchcard `managerRegistry`)
- [x] Replace Help button with Session Log button in `main.html` sidebar footer
- [x] Implement `session-log/` utility manager (see Section 14a)
- [x] Wire footer button → load utility manager into workspace
- [ ] Document standalone manager URL mode in `app.js` (groundwork only, not full impl)

### Phase 5: Profile Manager ⬜

- [ ] Preference form (language, date, time, number, theme)
- [ ] Save flow with JWT refresh
- [ ] Event-driven reactivity

### Phase 6: Testing and Polish

- [ ] Integration tests for login and manager loading flows
- [ ] Accessibility audit
- [x] PWA manifest and service worker updates

### Phase 7: Visual Editor (Style Manager Phase 2)

- [ ] Preview scene with mockup elements
- [ ] Inspector panel with CSS variable editors
- [ ] Live editing with undo/redo

---

## 21. Documentation Updates Needed

- [ ] Rewrite `INSTRUCTIONS.md` — manager pattern, event bus, CSS variables, JWT, testing
- [ ] Update `README.md` — current tech stack, link to BLUEPRINT.md
- [ ] Update `docs/Li/README.md` and `docs/Li/INSTRUCTIONS.md`

---

## 22. Planned Hydrogen Server Changes

None of these block the current Lithium work.

| Change | Priority | Description |
|--------|----------|-------------|
| `lang` JWT claim | Medium | Preferred language in JWT; used by lookups filtering |
| `punchcard` JWT claim | Medium | Manager/feature permissions in JWT |
| HttpOnly cookie auth | Low | `Set-Cookie` instead of JSON body token |
| Lookups API | Medium | `GET /api/lookups` filtered by `lang` |
| Theme CRUD API | Medium | `GET/POST/PUT/DELETE /api/themes` |
| User preferences API | Medium | `GET/PUT /api/user/preferences` |

---

## 23. Recommended Next Steps

1. **Session Log Utility Manager** — First utility manager. Replaces the Help
   sidebar footer button. Reuses the login panel log viewer pattern but with
   refresh + clear-archived actions. Validates the utility manager architecture
   end-to-end before Phase 5.
2. **Profile Manager** — Demonstrates preferences → JWT refresh → event system flow.
3. **Style Manager** — Remaining items: theme data from Hydrogen API, unit tests.
4. **Manager Switcher Popup** — Deferred until a menu manager that benefits from
   multi-instance support is actively being built. At that point extend
   `loadedManagers` and implement the switcher popup (see § 3b).
5. **Integration tests** — Mock Hydrogen, test login → main menu → manager loading.
6. **Documentation** — Update INSTRUCTIONS.md with utility manager pattern,
   popup panel pattern, and standalone manager URL mode.

---

## 24. Logging System ✅

Lithium maintains a client-side action log in `src/core/log.js`. Entries are stored
in memory (never cleared for the session), persisted to `sessionStorage` for cross-chunk
access, and batched to `POST /api/logs` for optional server archival.

### Architecture

| Concern | Mechanism |
|---------|-----------|
| In-memory store | `_buffer` array, never cleared during a page session |
| Cross-chunk access | `sessionStorage` — reloaded on each module evaluation |
| Previous-session archive | Moved to `localStorage` keyed `lithium_log_archive_<id>_<ts>` on next page load |
| Server upload | Batched to `POST /api/logs` (fire-and-forget, silent failure) |
| Counter | Monotonic integer per session, zero-padded display "000 123" |
| Session ID | Generated on first `log()` call — always unique per page load |

### Raw Entry Format

```json
{
  "sessionId": "mmi50w7i-afsf1mri-hcy1yqd6",
  "counter": 7,
  "timestamp": 1741398052712,
  "subsystem": "Startup",
  "status": "INFO",
  "duration": 218.5,
  "description": "Version: 1.1.1074 (build 1074)"
}
```

`description` may also be a **grouped object** (see below).

### Grouped Log Entries

A single log entry can carry a **title** and an array of **items** as its description.
This stores one entry (one counter, one timestamp) but renders as multiple visual lines
in the log viewer with a `―` continuation prefix.

```js
logGroup(Subsystems.STARTUP, Status.INFO, 'Browser Environment', [
  `Browser: Firefox 148`,
  `Platform: Linux x86_64`,
  `Language: en-US`,
  `Online: true`,
]);
```

Displays as:

```log
HH:MM:SS.ZZZ  Startup  Browser Environment
HH:MM:SS.ZZZ  Startup  ― Browser: Firefox 148
HH:MM:SS.ZZZ  Startup  ― Platform: Linux x86_64
HH:MM:SS.ZZZ  Startup  ― Language: en-US
HH:MM:SS.ZZZ  Startup  ― Online: true
```

The raw `description` field stores: `{ "title": "Browser Environment", "items": ["Browser: Firefox 148", ...] }`

**When to use grouped entries:**

- A single logical event with multiple related detail items (e.g., environment summary, manager list)
- When items all share the same timestamp and semantic context
- Prefer individual `log()` calls when items arrive at distinct times or have independent significance

### Logging API

| Function | Convenience for |
|----------|----------------|
| `log(subsystem, status, description, duration?)` | Generic log entry |
| `logStartup(description, duration?)` | `Startup / INFO` |
| `logAuth(status, description, duration?)` | `Auth / <status>` |
| `logHttp(description, duration?)` | `HTTP / INFO` |
| `logManager(status, description, duration?)` | `Manager / <status>` |
| `logError(subsystem, description, duration?)` | `<subsystem> / ERROR` |
| `logWarn(subsystem, description, duration?)` | `<subsystem> / WARN` |
| `logSuccess(subsystem, description, duration?)` | `<subsystem> / SUCCESS` |
| `logGroup(subsystem, status, title, items, duration?)` | Grouped entry with `―` continuation |

### Subsystem Constants (`Subsystems.*`)

`STARTUP`, `JWT`, `EVENTBUS`, `HTTP`, `MANAGER`, `SESSION`, `AUTH`, `CONFIG`,
`LOOKUPS`, `ICONS`, `PERMS`, `THEME`

### Status Constants (`Status.*`)

`INFO`, `WARN`, `ERROR`, `FAIL`, `DEBUG`, `SUCCESS`

### Logging Coverage Policy

Every module that makes network calls or initializes subsystems **must** log:

1. **Before** the call — describing intent and target
2. **After** success — with duration and bytes/count where available
3. **On error** — with status `WARN` or `ERROR` and the error message

Modules must route through the `log()` API (not bare `console.log`). Use the module's
`Subsystems.*` constant for consistent categorization. `lookups.js` uses a `logger`
helper object that forwards to both `console.*` and `log()`.

### Log Viewer (System Logs Panel)

The login panel's "System Logs" subpanel renders log entries using CodeMirror 6
(read-only, `oneDark` theme, `Vanadium Mono` font, zero-padded 4-digit line numbers).
The log panel header includes two action buttons:

- **Coverage** (`fa-chart-line`) — opens `/coverage/index.html` in a new tab
- **Close** (`fa-xmark`) — returns to the main login panel

Grouped `{ title, items }` descriptions are expanded to multiple display lines
with shared `HH:MM:SS.ZZZ  Subsystem` prefix and `―` continuation marker for items.

---

## Appendix: Known Limitations & Lessons Learned

### Current Limitations

1. **Config path** — Fetches `/config/lithium.json` relative to root; may need adjustment for path-prefixed deployments.
2. **Permissions fallback** — Allow-all fallback is convenient for dev but should be configurable before production.
3. **json-request.js** — No way to suppress `auth:expired` event for specific requests; may need `silent: true` option.
4. **Test coverage gaps** — json-request.js (40%) and lookups.js (37%) are low due to ESM mocking complexity with fetch+auth+event chains.

### Deployment Lessons Learned (March 2025)

1. **HTML Template Loading** — Managers fetch HTML templates at runtime via `fetch()`. Vite bundles only JS/CSS, so HTML templates must be copied to `public/src/managers/{name}/` to be available in production. The `public/` directory is copied as-is during build.

2. **Font Awesome Loading** — CDN links with SRI hashes can fail if the CDN content changes. Font Awesome Kit (`https://kit.fontawesome.com/`) is more reliable for production as it handles its own caching and integrity.

3. **Build Output Verification** — Always verify the deployment directory structure matches runtime expectations. Missing static assets cause 404 errors that don't appear in development (where `src/` is served).

### Build & Import Lessons Learned (March 2026)

1. **Mixed Dynamic/Static Imports** — Vite warns when a module is both statically imported (`import { x } from './mod.js'`) and dynamically imported (`await import('./mod.js')`) in the same file. The dynamic import cannot move the module into a separate chunk because the static import already pulls it into the main bundle. **Fix:** Use static imports for all core modules that are needed at startup. Reserve dynamic `import()` only for lazy-loaded managers and heavy third-party libraries (Tabulator, CodeMirror, DOMPurify).

2. **Dynamic Import with Runtime Variables** — `import(/* @vite-ignore */ someVar)` tells Vite to skip static analysis. The module at `someVar` is never bundled and the path does not exist in production. **Fix:** Use a static `switch` dispatching to literal `import('./path.js')` calls. Vite can see these at build time, create code-split chunks, and rewrite paths in the bundle. See `_importManager()` and `_importUtilityManager()` in `app.js`.

3. **Service Worker Cache Busting** — The service worker's `CACHE_VERSION` must change on each deploy to invalidate stale caches. The `bump-version.js` script automatically updates `CACHE_VERSION` in `service-worker.js` to match the build number, ensuring old caches are cleaned up on `activate`.

4. **Version System** — `version.json` at the project root tracks build number (starting at 1000), timestamp, and version string. The `bump-version.js` script increments it on each deploy and copies it to `public/` for runtime access. The login screen and help panel fetch `/version.json` to display the current build. This is independent of git hashes — it's a simple monotonic counter.
