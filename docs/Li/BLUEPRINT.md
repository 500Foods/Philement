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
| **Feature** | Capability within a manager, controlled by Punchcard permissions. |
| **Core** | Infrastructure modules managers depend on: event bus, config, JWT, etc. |

Managers require one or more modules. Not all modules are managers. Managers
are independently loadable via dynamic `import()`.

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

- Sidebar buttons generated from `getPermittedManagers()`
- Click → lazy load → render in workspace; state preserved on switch
- Header: manager name, profile button (stub), theme button (stub), logout
- Sidebar collapsible (desktop) and overlay-based (mobile)
- Logout: API call → clear JWT → fire `auth:logout` → return to login

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

## 14. Profile Manager ⬜ NEXT

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

1. **Style Manager** — The main unfinished piece. Exercises Tabulator, CodeMirror,
   DOMPurify, theme injection, and permission-gated UI end-to-end.
2. **Profile Manager** — Demonstrates preferences → JWT refresh → event system flow.
3. **Integration tests** — Mock Hydrogen, test login → main menu → manager loading.
4. **Documentation** — Rewrite INSTRUCTIONS.md to match current architecture.

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

2. **Service Worker Cache Busting** — The service worker's `CACHE_VERSION` must change on each deploy to invalidate stale caches. The `bump-version.js` script automatically updates `CACHE_VERSION` in `service-worker.js` to match the build number, ensuring old caches are cleaned up on `activate`.

3. **Version System** — `version.json` at the project root tracks build number (starting at 1000), timestamp, and version string. The `bump-version.js` script increments it on each deploy and copies it to `public/` for runtime access. The login screen and help panel fetch `/version.json` to display the current build. This is independent of git hashes — it's a simple monotonic counter.
