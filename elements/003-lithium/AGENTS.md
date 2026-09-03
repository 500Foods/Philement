# Lithium — Agent Guide

Read this file before changing Lithium. It is the session-start map so you do
not re-derive architecture, IDs, APIs, or known defects. Coding rules live in
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) and **win** on style
conflicts. Active sprint work lives in
[`/docs/Li/plans/LITHIUM_SPRINT.md`](/docs/Li/plans/LITHIUM_SPRINT.md).

**Last reviewed:** 2026-08-22 (architecture review + sprint kickoff).

---

## What Lithium is

Lithium is element **003** — a vanilla JavaScript ES-module **SPA / PWA** that
is the operator and product UI for **Hydrogen** (element 001, C HTTP server).
It does not own business data. It talks to Hydrogen over JWT + Conduit
(SQL QueryRefs) and, for named Lua, `POST /api/conduit/script`. Schema and
seeded Lua live in **Helium** (element 002, Lua migrations). SSO is
**Keycloak → Hydrogen OIDC RP → Hydrogen JWT**. The browser never holds a
Keycloak token.

| Piece | Path | Role |
|-------|------|------|
| Lithium SPA | `elements/003-lithium/` | This project |
| Lithium docs | `docs/Li/` | TOC, INS, manager, table, API, OIDC |
| Hydrogen | `elements/001-hydrogen/hydrogen/` | Auth, Conduit, scripting workers, WS |
| Hydrogen docs | `docs/H/` | APIs, tests, plans (`LUA_CLIENT`, AUTH_FINALE) |
| Helium | `elements/002-helium/` | Acuranzo (and other) Lua migrations |
| Helium docs | `docs/He/` | Migration authoring |
| Reception | `/mnt/extra/Projects/500-Courses-Reception/` | Public 500 Courses SPA (sibling repo) |
| Course Builder plan | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` | Pipeline CB-0..35 (Lua + Helium + human gates) |

Live: https://lithium.philement.com — Coverage: https://lithium.philement.com/coverage/

---

## Stack

- **Language:** Vanilla JS ES modules. No React/Vue/Svelte.
- **Build:** Vite 8 (`vite.config.js`). Dev port **3000**. Production minify Oxc.
- **Test:** Vitest 4 + happy-dom. Config: `vitest.config.js`.
- **Lint:** ESLint 10 (`eslint.config.js`), Stylelint (`src/**/*.css`).
- **Tables:** Tabulator 6 wrapped as **LithiumTable** (`src/tables/`).
- **Editors:** CodeMirror 6 (`src/core/codemirror-setup.js`).
- **HTML in editors:** SunEditor. **Markdown:** marked. **Sanitize:** DOMPurify
  is in `package.json` but **was unused as of 2026-08-22** — use it.
- **Tours:** Shepherd.js. **Dates:** Luxon + flatpickr. **Icons:** Font Awesome kit.
- **Auth:** Hydrogen JWT in `localStorage` via `src/core/jwt.js`.
- **Node:** >= 18, npm >= 9.

---

## Directory map

```text
elements/003-lithium/
├── AGENTS.md                 # This file
├── index.html                # First-stage boot (SW, version, splash)
├── package.json
├── vite.config.js / vitest.config.js / eslint.config.js
├── config/lithium.json       # Runtime config (copied to deploy if missing)
├── src/
│   ├── app.js                # Thin entry: CSS + LithiumApp
│   ├── app/                  # LithiumApp, AuthManager, ManagerLoader
│   ├── core/                 # jwt, log, event-bus, config, popup, toolbar…
│   ├── shared/               # conduit.js, lookups, toast, websocket
│   ├── tables/               # LithiumTable mixins + column manager
│   ├── managers/             # One folder per manager
│   ├── init/                 # Vendor init (CM, SunEditor, highlight, prism)
│   └── styles/               # base / layout / components / vendor-fixes
├── public/                   # Static + runtime-fetched HTML/CSS copies
│   └── src/managers/         # templates:copy destination (HTML fetch paths)
├── tests/unit/  tests/integration/
└── scripts/                  # postbuild, brotli, deploy, registries
```

Docs are **not** under `elements/003-lithium/docs/`. They are `/docs/Li/`.
`LITHIUM-INS.md` still links a missing `elements/003-lithium/docs/REFACTORING_PLAN.md`.

---

## Mandatory coding rules (INS summary)

Full text: [`LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md).

1. **No fallback UIs** for primary features (no textarea-if-JsonTree-fails, no
   JS-cloned login form as the “real” path). Fail visibly (toast + session log).
2. **≤ 1000 lines** per JS / CSS / HTML source file. Split before crossing.
3. **ES modules**, one class per file generally, inject dependencies for tests.
4. **CSS-first.** No `element.style.*` except drag geometry (`top`/`left`/`width`/`height`).
   No `alert` / `confirm` / `prompt`. Use existing modal / toast patterns.
5. Extract pure functions so Vitest can hit them.
6. Reuse existing CSS variables (`--bg-primary`, `--text-primary`, `--space-N`).
7. **`log(Subsystems.*, Status.*, msg)`** — not `console.log` (except
   `window.lithiumLogs.print`).
8. User prefs go through **`window.lithiumSettings`**, not raw `localStorage`.
   JWT and a few documented cache blobs are the exception.
9. **One popup/popout at a time** via `document` `close-all-popups`.

Hydrogen hygiene also applies when you touch C/Lua: no `static` in Hydrogen
`src/` for conduit work; no SEGV tolerance; incomplete work finished,
intentional, or listed in a plan — no “for now / stub implementation” success
paths.

---

## Boot sequence

1. `index.html` applies branding from `/config/lithium.json`, registers
   `/service-worker.js`, gates on `/version.json` (`cache: 'no-store'`).
2. `<script type="module" src="/src/app.js">` — console-capture first, then CSS
   cascade, then `new LithiumApp()` → `window.lithiumApp`.
3. `LithiumApp.init()` (`src/app/lithium-app.js`): version, config, `createRequest`,
   icons, tooltips, **event listeners before auth**, activity → JWT renewal,
   `auth.checkAuthAndLoad()`.
4. Valid JWT → Main manager. No JWT → dynamic import Login.
5. Login success emits `Events.AUTH_LOGIN`; app loads Main. Do not call Main
   directly from Login.
6. Background startup: health, lookups, icon preload, `Events.STARTUP_COMPLETE`.

OIDC return: `?oidc=1&handoff=` exchanged via `POST /api/auth/oidc/handoff`
(`src/core/oidc-client.js`). Store Hydrogen JWT only. Wipe the query string.

---

## Managers

### Lifecycle (code vs docs — fix in sprint Phase 1)

Docs say `constructor(app, container)` → `init()` → `render()` → `teardown()`.

`ManagerLoader.closeManager` (`src/app/manager-loader.js`) currently calls
**only `instance.destroy()`**. Real table managers implement **`cleanup()`**
(Queries, Lookups, Style, Version, Scripting). Login/Main/placeholders implement
**`teardown()`**. Terminal implements **`destroy()`**. Closing Queries today
drops the slot and leaks Tabulator / CodeMirror / splitters.

**Target contract (lock in Phase 0, implement in Phase 1):**

```text
closeManager → destroy() if present
            → else cleanup()
            → else teardown()
showManager  → onActivate() if present
hide         → onDeactivate() if present
```

### Registration

No plugin discovery. Vite needs a **static `switch`** so each manager is its
own chunk. Do **not** add a `manualChunks` rule that matches `'manager'` —
that previously collapsed ~27 managers into one 2.6 MB JS + 359 KB CSS chunk
and broke CSS order (`vite.config.js` comments).

| Kind | Where | IDs |
|------|-------|-----|
| Menu | `ManagerLoader.managerRegistry` | **7–33** (CourseBuilder reserved **34**) |
| Utility | `LithiumApp.utilityManagerRegistry` | `user-profile`=3, `session-log`=4, `terminal`=5 |
| Special | Auth / Main / Tour / Crimson | Login + Main loaded outside registry. Tour is a side-effect import from Main. Crimson is a popout. |

Sidebar labels/icons come from **server menu data** (`getMenu`), not the
registry. Punchcard `getPermittedManagers()` is called **without a punchcard**
today and returns `[7..33]`. Do not assume UI authz until Phase 9.

### Canonical ID map (2026-08-22)

Docs, `lithium.json` `managers`, fallback icons, and `_importManager` cases
1–6 **disagree**. Until Phase 0 writes this table into `LITHIUM-MGR.md`, treat
**`manager-loader.js` + `lithium.json`** as runtime truth for menu IDs:

| ID | Name (config / registry) | Module |
|----|--------------------------|--------|
| 1 | Login | `managers/login/` (not in menu registry) |
| 2 | Menu / Main | `managers/main/` |
| 3 | User Profile (utility) | `profile-manager/` — **not** server-profiles |
| 4 | Session Log (utility) | `session-log/` |
| 5 | Crimson (popout) | `crimson/` — **collides with utility Terminal=5** |
| 6 | Tour | `tour/` (not a slot manager) |
| 7 | Dashboard | `dashboard/` placeholder |
| 8 | Mail | `mail-manager/` placeholder |
| 9 | Server Profiles | `server-profiles.js` class `UserProfilesManager` |
| 10 | Server Sessions | `server-sessions.js` class `SessionLogsManager` |
| 11 | Version | `version-history/` |
| 12–21 | Calendar…Ticketing | mostly 49-line placeholders |
| 22 | Style | `style-manager/` |
| 23 | Lookups | `lookups/` |
| 24–28 | Report, Role, Security, AI Auditor, Job | placeholders |
| 29 | Queries | `queries/` — canonical LithiumTable consumer |
| 30–31 | Sync, Camera | placeholders |
| 32 | Terminal (menu) | `terminal/` (also utility key `terminal`) |
| 33 | Scripting | `scripting/` |
| 34 | Course Builder | **not created yet** — reserve here |

Tour matching uses **numeric ID only** (`"003.Profile"` matches anything with
3). Utility Terminal=5 can steal a Crimson tour. Do not add another ID 5.

`_importManager` cases **1–6** are dead and mapped to the wrong modules.
Theme keyboard `loadManager(1)` is dead (1 is not in the menu registry).

### Implemented vs placeholder

**Real UI:** Login, Main, Queries, Lookups, Style (UI exists; apply/copy are
no-ops), Version History, Scripting (list + Lua editor + save), Profile
(partial; many page `save()` stubs return success), Session Log, Terminal,
Crimson, Tour.

**~22 placeholders** share the same 49-line “under development” shell
(Dashboard, Mail, Calendar, Contacts, Files, Document, Media, Diagram, Chats,
Notifications, Annotation, Ticketing, Reports, Role, Security, AI Auditor,
Job, Sync, Camera, …). `LITHIUM-MGR.md` marks several as Implemented — it is
wrong. Collapse to one `PlaceholderManager` rather than copy-paste.

### New manager checklist (corrected)

1. Reserve the next menu ID (34+). Add to `managerRegistry`, `_importManager`
   `switch`, and `config/lithium.json` `managers`.
2. Create `src/managers/<name>/<name>.{js,html,css}`.
3. Export `default class` with `init()` and `cleanup()` (or the locked trio).
4. Fetch HTML from `/src/managers/<name>/<name>.html` and run
   `npm run templates:copy` so `public/` has the file.
5. Do **not** register a static sidebar row in `main.js`. Menu is server data
   (Helium lookups / menu QueryRefs).
6. Import LithiumTable from `src/tables/lithium-table-main.js` — **not**
   `src/core/lithium-table-main.js` (stale NEW-manager guide).
7. Stay under 1000 lines. Follow Queries/Scripting as the pattern.

---

## Hydrogen integration

Hydrogen is the only backend. Default local URL in `src/core/config.js` is
`http://localhost:8080`. Checked-in `config/lithium.json` points at
`https://lithium.philement.com`. Deployed config is **not** overwritten
(`emptyOutDir: false` on deploy; `deploy:config` copies only if missing).

Client wrapper: `src/core/json-request.js` (JWT header, renew, errors).
Do not invent product C routes (`/api/coursebuilder/*`, `/api/enroll/*`).

### Auth

| Method | Path | Notes |
|--------|------|-------|
| POST | `/api/auth/login` | Password → Hydrogen JWT |
| POST | `/api/auth/renew` | Same JWT shape; scheduled at 80% life (`jwt.js` `getRenewalTime`) |
| POST | `/api/auth/logout` | Invalidate server-side |
| GET | `/api/auth/oidc/start` | 302 → Keycloak (PKCE). Query `database`, `return_to`, `provider` |
| GET | `/api/auth/oidc/callback` | Hydrogen-only; SPA never implements this |
| POST | `/api/auth/oidc/handoff` | `{ handoff }` → login-shaped `{ token, … }` |
| POST | `/api/auth/oidc/end-session` | Optional IdP logout URL |

Docs: [`LITHIUM-OIDC.md`](/docs/Li/LITHIUM-OIDC.md),
[`LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md),
[`docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md),
[`docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md).

**Hard rule:** Keycloak `id_token` / `access_token` never enter the SPA.

### Conduit (SQL)

`src/shared/conduit.js`:

| Helper | Endpoint |
|--------|----------|
| `query` / batch | `POST /api/conduit/query` / `queries` (public) |
| `authQuery` / batch | `POST /api/conduit/auth_query` / `auth_queries` (JWT) |

Payload: `{ query_ref: N, params: { STRING: {…}, INTEGER: {…} } }`.

`LITHIUM-API.md` still documents `GET /api/lookups` and `/api/styles` CRUD.
**Runtime is Conduit QueryRefs** (lookups 001/030/053/054/060, etc.). Treat
the API doc as stale until sprint Phase 12.

### Conduit script (Lua invoke)

Hydrogen: [`docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md).
LUA_CLIENT Phases 0–10 are complete.

```http
POST /api/conduit/script
Authorization: Bearer <hydrogen-jwt>
{ "script": "Group.Name", "params": { }, "wait": true, "timeout_seconds": 15 }

GET /api/conduit/script/{job_id}
```

- Script must exist in Helium `scripts` with **`invokable = 1`**.
- Name is `Group.Name` (dot only).
- Do not send `params._hydrogen` (reserved; server injects claims).
- C does not interpret business logic.

**Lithium has no `invokeScript` helper yet** (`conduit.js` is query-only).
Scripting Manager does **not** invoke. Course Builder operator buttons must
use this surface, not new C.

### Other Hydrogen surfaces

- `GET /api/system/health` — startup.
- WebSocket — `src/shared/` + `LITHIUM-WSS.md`. Config `server.websocket_url`.
- Chat proxy, mail relay, OIDC IdP are Hydrogen subsystems; Lithium managers
  for some of those are still placeholders.

Hydrogen build aliases (C, not Lithium): `zsh -ic 'mkt'` trial, `mka` all,
`mku <base>` Unity, `mkp` pretty, `mks` shell. After Helium migration edits
you must rebuild the Hydrogen payload (`mkt`/`mka`) before DB tests.

---

## Helium integration

Helium is **Lua migrations**, not a runtime. Hydrogen AutoMigrations apply
them. Designs live under `elements/002-helium/<design>/migrations/`
(Acuranzo is the product design).

Lithium cares about:

| Concern | Where |
|---------|--------|
| Table defs for LithiumTable | DB + `tablePath` (e.g. `scripts/script-manager`) |
| QueryRefs | Numbered SQL in Helium; Lithium passes the integer |
| Menu / lookups | Seeded lookup rows; Lithium caches in `shared/lookups.js` |
| Lua scripts | `scripts` table: `group_name`, `script_name`, `code`, `invokable`, schedule/status |
| Course Builder tables | **Not created** — COURSEBUILDER CB-2 (`course_build_runs`, artifacts, events) |

Script seeds already in Acuranzo (examples): `Api.Echo`, `Enroll.FreeCourse`,
`Account.GetSettings` / `UpdatePrefs` / `MyCourses` / `Orders`, Stripe/Catalog
family. Many more will land. Scripting Manager lists them via QueryRef **89**
(search **90**, detail **87**, insert **129**, update **130**). Confirm numbers
against Helium if a query 404s — do not invent refs.

New tables / QueryRefs / `Build.*` scripts are Helium work. After changing
migrations: `mkt` or `mka`, then Hydrogen migration tests as needed (31–38).
Do not hand-edit production DBs as the source of truth.

Helium docs start: [`docs/He/README.md`](/docs/He/README.md),
[`docs/He/GUIDE.md`](/docs/He/GUIDE.md).

---

## Keycloak / 500 Passwords

Production IdP is the **500 Passwords** Keycloak realm (Festival). Lithium
config:

```json
"auth": {
  "oidc_providers": [{
    "id": "500passwords",
    "label": "Sign in with 500 Passwords",
    "start_url": "/api/auth/oidc/start"
  }]
}
```

`id` **must** match `OIDC_RP.Providers[].Name` on Hydrogen. Redirect URI is
exact string match. AUTH_FINALE Phase 11 (real-IdP E2E) has been blocked on
MFA/OTP for the test user — that is ops, not a reason to re-implement RP.

Password login stays independent of IdP availability.

---

## Scripting Manager (ID 33) — current state

Path: `src/managers/scripting/` (`scripting.js` ~1004 lines — already over the
cap, `scripting-editors.js`, html, css).

**Works:** LithiumTable of `scripts`, row select → detail QueryRef, CodeMirror
Lua tab, preview tab, font popup, undo/redo/fold, edit helper, save/duplicate
via QueryRefs 129/130, audit footer, splitter.

**Missing (sprint Phases 14–19):**

- `POST /api/conduit/script` invoke UI (params JSON, wait/timeout, result pane)
- `invokable` column visibility / edit
- Job poll `GET /api/conduit/script/{job_id}` when `wait: false`
- Delete, validate/luacheck, run-as-role
- `cleanup()` actually called (lifecycle Phase 1)
- Unit tests
- `LITHIUM-MGR-SCRIPT.md`
- Shared `invokeScript()` in `conduit.js`

`this.app.user` is unset on `LithiumApp`; save sends `USERID: 0`. Real user is
`this.app.auth.user` — fix when touching save.

---

## Course Builder

Authoritative pipeline:  
`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` (CB-0..35).

Architecture lock (2026-08-12):

- Background ticks: Orchestrator + `invokable=0` Lua.
- Human buttons: JWT `POST /api/conduit/script` with `Build.Accept` /
  `Build.Approve` / `Build.Publish` / … (`invokable=1`).
- LMS I/O only in Lua via `H.http`. No browser Canvas token. No
  `/api/coursebuilder/*`.
- Human gates between expensive LLM stages (Submission, Approve, Review,
  Inspect, Publish). Do not skip Approve into Research.

Lithium’s job in **this** sprint is the **operator manager (ID 34)** plus the
script-invoke client those buttons need. Helium tables and `Build.*` seeds
are entry gates, not an excuse to rewrite COURSEBUILDER.md inside Lithium.

Reception (public catalog / Suggest a Course / Cap forms) is a **different
SPA**. Do not merge Reception UI into Lithium.

---

## Build, test, deploy

Workdir: `elements/003-lithium`.

| Command | What |
|---------|------|
| `npm install` | Once per tree |
| `npm run dev` | Vite :3000 HMR. **Needs Hydrogen** for login/data. TOC’s “cannot run dev” is overstated. Auto-login: `?USER=&PASS=` |
| `npm test` | `vitest run` (unit + integration) |
| `npm run test:watch` | Vitest watch |
| `npm run test:coverage` | v8 → `./coverage` |
| `npm run coverage:copy` | into `public/coverage` |
| `npm run lint` / `lint:css` / `lint:fix` | ESLint + Stylelint |
| `npm run validate:tabulator` | table config |
| `npm run build` | validate + lint:css + vite + style/icon registries |
| `npm run build:prod` | production mode + `postbuild.sh` |
| `npm run templates:copy` | HTML/CSS → `public/src/managers/` (**required** after template edits; not a prebuild hook) |
| `npm run deploy` | needs `$LITHIUM_DEPLOY` (and usually `$LITHIUM_ROOT`) |
| `npm run deploy:500courses` | sibling 500 Courses deploy script |

Env: `LITHIUM_ROOT`, `LITHIUM_DEPLOY`, `LITHIUM_DEPLOY_KEEP`.

**Honest test policy:**

- Unit tests for core (`jwt`, `utils`, `conduit` query helpers, `permissions`,
  LithiumTable resolution) are real. Prefer adding tests there.
- Many manager tests are mocks/smoke. Do not add tests for placeholder shells.
- `tests/integration/auth.integration.test.js` hits
  `https://lithium.philement.com` (override with `HYDROGEN_SERVER_URL`).
  Missing credentials or a down server **skip** the cases. The live cert
  expired 2026-08-02; tests retry once without TLS verify and warn.
- `LITHIUM-TST.md` still says “672 tests” / 14 unit files — stale.
- Coverage excludes `src/init/**`.

You can and should run `npm test` and `npm run lint` without Hydrogen.
Runtime manager work needs a JWT against a migrated DB.

---

## Config and PWA

- Runtime: `/config/lithium.json` (also `config/` in the repo).
- `public/version.json` + root `version.json` — SW update gate.
- Service worker: `public/service-worker.js`. Version mismatch never reveals UI.
- `index.html` splash / FOUC handling — do not casually rewrite.

Do not log JWTs, handoff codes, or Keycloak tokens.

---

## Known defects (do not rediscover)

From the 2026-08-22 review. Tracked in `LITHIUM_SPRINT.md`.

| Sev | Issue |
|-----|--------|
| High | `closeManager` only calls `destroy()` — leaks on Queries/Lookups/Scripting |
| High | `lookups.js` “HTML escape” is a no-op (`&`→`&`); preview `innerHTML` XSS |
| High | `marked.parse` → `innerHTML` in Crimson / Version History; DOMPurify unused |
| High | `highlight-init.js` interpolates code into `innerHTML` |
| High | Style Manager `applyState` empty; apply/copy toast success |
| High | Profile page `save()` stubs return `{ success: true }` |
| High | `suneditor-init.js`: `'Bearer ' + localStorage.getItem(...) \|\| ''` → `"Bearer null"` |
| Med | ~14 files over 1000 lines (`tour.js` 2834, `lookups.js` 1888, …) |
| Med | Three ID schemes; utility Terminal=5 vs Crimson=5 |
| Med | Native `alert`/`confirm`/`prompt` (~9 sites) |
| Med | ~491 `element.style` assignments; ~72 live `console.*` |
| Med | ~138 direct `localStorage` pref writes |
| Med | `public/src` CSS duplicates Vite CSS; orphans `user-profiles/`, `session-logs/`, `style-manager-v2.html` |
| Med | `app.user` never set; `getState()` lies |
| Low | Missing `REFACTORING_PLAN.md`; API/TST/MGR docs stale |

---

## Documentation map

Start: [`docs/Li/LITHIUM-TOC.md`](/docs/Li/LITHIUM-TOC.md).

| Need | Doc |
|------|-----|
| Coding rules | `LITHIUM-INS.md` |
| Dev / npm | `LITHIUM-DEV.md` |
| Tests | `LITHIUM-TST.md` |
| Managers | `LITHIUM-MGR.md`, `LITHIUM-MGR-NEW.md` |
| Tables | `LITHIUM-TAB.md`, `LITHIUM-TAB-TYPES.md` |
| API (stale in parts) | `LITHIUM-API.md` |
| JWT / OIDC | `LITHIUM-JWT.md`, `LITHIUM-OIDC.md`, `LITHIUM-KEYCLOAK.md` |
| Deploy | `LITHIUM-WEB.md` |
| Sprint | `docs/Li/plans/LITHIUM_SPRINT.md` |
| Hydrogen script API | `docs/H/api/conduit/script.md` |
| Course Builder pipeline | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` |

---

## Session checklist

1. Read **CURRENT PAUSE POINT** in `docs/Li/plans/LITHIUM_SPRINT.md`.
2. Re-read this file’s Known defects + ID map if touching managers or HTML.
3. One phase at a time. Do not mark a gate green without the named command
   or check actually passing.
4. After JS/CSS: `npm run lint` and `npm test` from `elements/003-lithium`.
5. After HTML templates: `npm run templates:copy`.
6. After Helium Lua: rebuild Hydrogen payload (`mkt`/`mka`) — that is a
   different tree.
7. Update the phase Status + Working Log. Do not start the next phase in the
   same turn unless the user asked to continue.
8. Never commit secrets, JWTs, or `node_modules`. Do not commit unless asked.
