# Lithium — Agent Guide

Read this file before changing Lithium. It is the session-start map so you do
not re-derive architecture, IDs, APIs, or known defects. Coding rules live in
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) and **win** on style
conflicts. Active plan:
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
(supersedes
[`/docs/Li/plans/LITHIUM_SPRINT.md`](/docs/Li/plans/LITHIUM_SPRINT.md)).
Pause point 2026-10-23: Phases 0, 0a, 0b, 0c, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, and 13 are complete.
Phases 5a–5d and 6 stay deferred. Andrew reported Acuranzo 1389–1393 applied.
Scripting save, update, and duplicate send INTEGER `SCRIPT_TYPE`,
`STATUS`, `USERID`, and `INVOKABLE`. A blank schedule or run time is
`''`. Andrew applied `acuranzo_1395.lua` (QueryRef 130) and
`acuranzo_1396.lua` (QueryRef 129) and confirmed pretty-print then
save. Delete uses
QueryRef 131. On the 500 Courses Lithium database, Lookup 059 key 13 is
`scripts/script-manager`. Andrew confirmed `Api.Echo` on
`https://lithium.500courses.com`: Wait on `BBTDC`, Wait off `CCCRP`, both
completed. Phase 10 is complete. He confirmed the tulip button,
Ctrl+P (`@johnnymorganz/stylua` 2.6.0 defaults), pretty-print then
save, the Preview tab (summary, then Lua), and that 9px survives a
reload. He declined the optional meta tab. It does not move
forward. Phase 11 is complete: CodeMirror 6 is the only stack, Lua
folds statement blocks, and `H.` names use the namespace tag.
Phase 12 is complete: Course Manager is a staff shell. The gate calls `getClaims(retrieveJWT())`. A bare
`getClaims()` hides the row. Andrew confirmed the Learning group
shows it and the shell opens. QueryRef 155 failure still denies.
The sidebar omits ID 34 when the gate fails. Andrew reported
Acuranzo 1378, 1379, 1380, and 1381 applied. Phase 13 is complete
(2026-10-23). Andrew reported the latest build deployed, the latest
migrations applied, and the list Retired checkbox removed. Staff
list **4500**, insert **4501**, update **4502**, history **4503**
and **4504**, prices **4505**. Lookup 059 key 14 is on that
database. The navigator owns add, duplicate, and delete. Footer
save and cancel use ManagerEditHelper. Options stores Retired.
Canvas shows the image at 524×292, stretched. **4004** stays the
published catalog. Phase 14 is in progress. Save prices is **4506**
(`courses_4084.lua`). Orders are **4507** (`courses_4085.lua`).
`Stripe.Mode` is `courses_4086.lua` and is invokable. Andrew
reported 4084–4086 applied. Save prices still reloads the previous
cents. `courses_4087.lua` rewrites QueryRef 4506 and is not
applied. The next Courses file is **4088**, the next caller
ref is **4508**, and the next Acuranzo file is **1399**. File
numbers stay below 4500. Do not mark Phase 14 complete until the
save keeps the typed amount. Do not start Phase 15.
He does deploys and login checks. Do not apply migrations. The report writer (manager 24,
[`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md),
Phases 0–71, Phase 0 approved 2026-10-10) stays parked. Its next
phase is Phase 1, and only after Andrew pulls that plan in. PDF is
the default export there. Phased work follows
[`/docs/PLANNING.md`](/docs/PLANNING.md).

**Last reviewed:** 2026-10-23 (CATCHUP Phase 14 in progress. Andrew reported 4084–4086 applied. Save prices still reloads the previous cents. courses_4087.lua rewrites QueryRef 4506 and is not applied. The next Courses file is 4088 and the next caller ref is 4508. Do not start Phase 15. Phases 5a–5d and 6 stay deferred.)

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
- **Test:** Vitest 5 (`vitest` ^5.0.2) + happy-dom. Config: `vitest.config.js`.
- **Lint:** ESLint 10 (`eslint.config.js`), Stylelint (`src/**/*.css`).
- **Tables:** Tabulator 6 wrapped as **LithiumTable** (`src/tables/`).
- **Editors:** CodeMirror 6 (`src/core/codemirror-setup.js`).
- **HTML in editors:** SunEditor. **Markdown:** marked. **Sanitize:** DOMPurify.
  `sanitizeHtml` in `src/core/utils.js` wraps it (CATCHUP Phase 2,
  2026-10-16). The Phase 3 sinks call it.
- **Tours:** Shepherd.js. **Dates:** Luxon + flatpickr. **Icons:** Font Awesome kit.
- **Auth:** Hydrogen JWT in `localStorage` via `src/core/jwt.js`.
- **Node:** >= 18, npm >= 9.

---

## Directory map

```text
elements/003-lithium/
├── AGENTS.md                 # This file
├── CATCHUP.md                # Active gated plan (Phases 0–38)
├── index.html                # First-stage boot (SW, version, splash)
├── package.json
├── vite.config.js / vitest.config.js / eslint.config.js
├── config/lithium.json       # Runtime config (copied to deploy if missing)
├── src/
│   ├── app.js                # Thin entry: CSS + LithiumApp
│   ├── app/                  # LithiumApp, AuthManager, ManagerLoader
│   ├── core/                 # jwt, log, event-bus, config, popup, toolbar…
│   ├── shared/               # conduit.js, lookups, toast, confirm, websocket
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

### Lifecycle (CATCHUP Phase 1, 2026-10-16)

`ManagerLoader.closeManager` (`src/app/manager-loader.js`) calls one
hook: `destroy`, otherwise `cleanup`, otherwise `teardown`. A thrown
hook is logged and the slot is still removed. Close does not call
`onDeactivate`.

`showManager` calls `onDeactivate` on the outgoing instance and
`onActivate` on the incoming one after the crossfade. There is no
separate hide path. A manager that is already visible is not
activated again.

Queries, Lookups, Style, Version History, and Scripting implement
`cleanup()`. Login, Main, and the placeholders implement `teardown()`.
Terminal implements `destroy()`.

`_importManager` cases 1–6 are gone. Theme `loadManager(1)` still
returns before the import, because 1 is not in the menu registry.

### Registration

No plugin discovery. Vite needs a **static `switch`** so each manager is its
own chunk. Do **not** add a `manualChunks` rule that matches `'manager'` —
that previously collapsed ~27 managers into one 2.6 MB JS + 359 KB CSS chunk
and broke CSS order (`vite.config.js` comments).

| Kind | Where | IDs |
|------|-------|-----|
| Menu | `ManagerLoader.managerRegistry` | **7–35** (34 Course Manager is the staff catalog; 35 Course Builder is still a placeholder) |
| Utility | `LithiumApp.utilityManagerRegistry` | `user-profile`=3, `session-log`=4, `terminal`=5 |
| Special | Auth / Main / Tour / Crimson | Login + Main loaded outside registry. Tour is a side-effect import from Main. Crimson is a popout. |

Sidebar labels/icons come from **server menu data** (`getMenu`), not the
registry. Punchcard `getPermittedManagers()` with no punchcard returns
`[7..35]` (Phase 0a). Course Manager v1 uses JWT `roles` under CATCHUP
L3. Punchcard is not that gate.

### Canonical ID map

Runtime truth is `manager-loader.js` plus `config/lithium.json`.
Ids 34 and 35 were refreshed here on 2026-10-16. `_importManager`
cases 1–6 were removed in CATCHUP Phase 1. Older manager docs still
disagree in places.

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
| 34 | Course Manager | `course-manager/` — staff catalog (Phase 14, in progress 2026-10-23). QueryRef **155** is the roles gate. List **4500**. Prices **4505**. Save prices **4506**. Orders **4507**. |
| 35 | Course Builder | `course-builder/` — placeholder (Phase 0a). Band E is the real UI. |

Tour matching uses **numeric ID only** (`"003.Profile"` matches anything with
3). Utility Terminal=5 can steal a Crimson tour. Do not add another ID 5.

`_importManager` cases **1–6** were removed in CATCHUP Phase 1.
Theme keyboard `loadManager(1)` is still dead (1 is not in the menu registry).

### Implemented vs placeholder

**Real UI:** Login, Main, Queries, Lookups, Style (Apply and Copy write the
CSS editor text into `#style-manager-preview`; Undo and Redo stay disabled),
Version History, Scripting (list, Lua editor, save, duplicate, delete, and a Run tab; Andrew confirmed `Api.Echo` 2026-10-22), Profile (language,
date formats, photo, and Lookups settings persist; other pages refuse save),
Session Log, Terminal, Crimson, Tour.

**~22 placeholders** share the same 49-line “under development” shell
(Dashboard, Mail, Calendar, Contacts, Files, Document, Media, Diagram, Chats,
Notifications, Annotation, Ticketing, Reports, Role, Security, AI Auditor,
Job, Sync, Camera, …). `LITHIUM-MGR.md` marks several as Implemented — it is
wrong. Collapse to one `PlaceholderManager` rather than copy-paste.

Course Manager (34) lists courses for staff. It calls QueryRef
**155** first and implements `cleanup()`. A failed name lookup denies, and
the sidebar omits the row. The catalog calls **4500**, **4501**, and
**4502**. History calls **4503** and **4504**. Prices call **4505**.
Save prices calls **4506**. Orders call **4507**.
Sync, retire, Ensure Product, refund, and deactivate stay
disabled until QueryRef **87** reports `invokable` 1. Course Builder (35) is still the Phase 0a
placeholder and implements `teardown()`. Phase 14 is in progress.
Andrew reported 4084–4086 applied. Save prices still reloads the
previous cents. `courses_4087.lua` is not applied. Band E is the Course Builder UI.

### New manager checklist (corrected)

1. Reserve the next menu ID (36+; 34–35 are CATCHUP). Add to
   `managerRegistry`, `_importManager` `switch`, and
   `config/lithium.json` `managers`.
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
the API doc as stale until CATCHUP Phase 37.

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

`invokeScript` and `getScriptJob` live in `src/shared/conduit.js`
(CATCHUP Phase 2, 2026-10-16). The Scripting Run tab calls them
(CATCHUP Phase 9, complete 2026-10-22). A wait-true POST uses
`timeout_seconds` plus 5 seconds. Course Manager calls
`invokeScript` for `Stripe.Mode` on open. It calls
`Catalog.SyncFromCanvas`, `Catalog.Retire`, `Stripe.EnsureProduct`,
`Stripe.Refund`, and `Stripe.DeactivateCustomer` only when QueryRef
87 says `invokable` is 1. Those five stay 0, so the buttons stay
disabled. Course Builder
does not invoke yet. Course Builder operator buttons must use this
surface, not new C. `sanitizeHtml` is in `src/core/utils.js`.
Lookups, Crimson, Version History, and the Queries summary preview
call it (CATCHUP Phase 3, 2026-10-16).

### Other Hydrogen surfaces

- `GET /api/system/health` — startup.
- WebSocket — `src/shared/` + `LITHIUM-WSS.md`. Config `server.websocket_url`.
- Chat WS (Crimson), mail relay, MCP, OIDC IdP are Hydrogen subsystems;
  Lithium managers for Mail/Jobs/Chats/MCP are still placeholders.
  CATCHUP Bands F–G.

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

Partner buttons (Didit, Apple, Google, Microsoft) render from
`login partners` plus `auth.oidc_providers` (CATCHUP Phase 5,
2026-10-16). The checked-in config shows 500 Passwords only.

---

## Scripting Manager (ID 33) — current state

Path: `src/managers/scripting/` (`scripting.js` 649 lines,
`scripting-invoke.js` 546, `scripting-save.js` 495,
`scripting-editors.js` 415, `scripting-footer.js` 333,
`scripting-format.js` 91, `scripting-font.js` 161,
`scripting-preview.js` 48, html, css).

**Works:** LithiumTable of `scripts`, row select → detail QueryRef, CodeMirror
Lua tab, preview tab, Run tab, font popup, undo/redo/fold, edit helper,
audit footer, splitter. List (089), search (090), and detail (087) return
`invokable` after Acuranzo 1389–1391 (Andrew reported those applied
2026-10-16). Save and update (129/130) send INTEGER `SCRIPT_TYPE`,
`STATUS`, `USERID`, and `INVOKABLE`. A blank schedule or run time is
`''`. Andrew applied Acuranzo 1395 and 1396, which store that blank
as SQL NULL, and confirmed pretty-print then save. Duplicate copies the invokable flag, clears the three
run times, and appends ` (Copy)`. Delete
confirms and calls QueryRef 131 with STRING `GROUP_NAME` and
`SCRIPT_NAME`. A dirty row switch asks to save or stay. Rename of a
saved script is refused. New scripts prompt for group and name.
The Run tab posts `conduit/script` for an invokable `Group.Name`,
strips a top-level `params._hydrogen`, and polls
`GET /api/conduit/script/{job_id}` when Wait is off (250, 500, 1000,
then 2000 ms). Payload tests are
`tests/unit/managers/scripting-save.test.js` and
`tests/unit/managers/scripting-invoke.test.js`. Andrew confirmed
`Api.Echo` on `https://lithium.500courses.com` (Wait on `BBTDC`,
Wait off `CCCRP`, both completed). A database whose
queries 87 and 89 omit `invokable` keeps Run disabled. The Lua
highlighter lives in `src/core/lua-mode.js`. It accepts long
brackets (2026-10-17). Phase 11 (2026-10-22) folds `function`,
`if`, `do`, `for`, and `while` through `end`, and `repeat` through
`until`. `H.` and `H.name` use the namespace tag. Confirm the
deployed bundle before retesting scripts that use `[[`.

Ctrl+P and the tulip button (`#scripting-prettify-btn`, right side of
the Lua toolbar) call `EditorManager.handlePrettify`. It formats with
`@johnnymorganz/stylua` 2.6.0 defaults (tabs, width 4, column 120,
Unix LF). The button is disabled on Preview and Run. The wasm loads
on the first format. A parse error toasts and leaves the buffer.
Lua 5.5 `global` does not parse. Andrew confirmed the button and
Ctrl+P on 2026-10-22.
Tests: `tests/unit/managers/scripting-format.test.js`.

The Preview tab renders `currentScript.summary` as sanitized
markdown, then the Lua source as plain text (`scripting-preview.js`).
A blank summary shows "No summary to preview." A blank buffer shows
"No script content to preview." The editor buffer wins over the row. Font size, family,
and weight go through `window.lithiumSettings` at `33.font.<tab>`
(`get`, `set`, and `removeSection`, one dotted path). The font
button is enabled on Lua and Preview, and disabled on Run. An older
`lithium_scripting_font_*` value is copied once and then removed.
Save calls `flush()` so the account-settings update does not wait
two seconds. A reload that still has the previous server copy reads
`lithium_preferences`.
Doc: [`/docs/Li/LITHIUM-MGR-SCRIPT.md`](/docs/Li/LITHIUM-MGR-SCRIPT.md).
Tests: `scripting-preview.test.js` and `scripting-font.test.js`.
Andrew confirmed the Preview tab, the font popup, and that 9px
survives a reload on 2026-10-22.

**Left after Phase 11.** Andrew declined the optional meta tab
(invokable, schedule, last run) on 2026-10-22. On the 500 Courses
database, Lookup 059 key 13 is `scripts/script-manager`.

- `invokable` column visibility / edit. Not started.
- Validate/luacheck and run-as-role. Not started. The editor footer
  still shows zero errors.

`USERID` reads `this.app.auth.user` (CATCHUP Phase 4, 2026-10-16). A missing
id is still `0`. `this.app.user` is still unset on `LithiumApp`.

---

## Course Manager (ID 34) and Course Builder (ID 35)

**First deploy:** Course Manager — PRIORITIZE Part 5 in
`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`. Phase 12
(2026-10-22) is the staff shell. Phase 13 (2026-10-23) is the
catalog. Andrew reported the latest build deployed and the latest
migrations applied. The list is a LithiumTable on the Query Manager shell:
navigator add, duplicate, and delete, plus footer save and cancel.
Course Options holds Retired and Featured. The list has no Retired checkbox. Canvas shows the image at 524×292, stretched.
Commerce lists prices through QueryRef **4505** and saves them
through **4506**. Orders are **4507**. Authz v1 is CATCHUP L3:
integer `role_id`s from `getClaims(retrieveJWT())`, resolved through
QueryRef **155** to `roles.name` `staff` or `admin`. A failed
resolve denies. The sidebar omits ID 34 when the gate fails. A
direct open shows the lock panel. Andrew confirmed the row and the
shell on 2026-10-22. Doc:
[`/docs/Li/LITHIUM-MGR-COURSE.md`](/docs/Li/LITHIUM-MGR-COURSE.md).
Hydrogen 2.23 `account_roles` is a later cutover (Band H). Phase 14
is in progress. `Stripe.Mode` reports test or live. Ensure, rotate,
refund, and deactivate stay disabled until invokable. Andrew
reported 4084–4086 applied. The save still reloads the previous
cents. `courses_4087.lua` is not applied. Do not start Phase 15.

**Course Builder pipeline:**  
`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` (CB-0..35).

Architecture lock (2026-08-12, still true):

- Background ticks: Orchestrator + `invokable=0` Lua.
- Human buttons: JWT `POST /api/conduit/script` with `Build.Accept` /
  `Build.Approve` / `Build.Publish` / … (`invokable=1`).
- LMS I/O only in Lua via `H.http`. No browser Canvas token. No
  `/api/coursebuilder/*`.
- Human gates between expensive LLM stages (Submission, Approve, Review,
  Inspect, Publish). Do not skip Approve into Research.

Lithium’s job in CATCHUP Band D is Course Manager (ID **34**). Band E is
the Course Builder operator manager (ID **35**). Helium tables and
`Build.*` seeds are entry gates, not an excuse to rewrite COURSEBUILDER.md
inside Lithium.

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
  Missing credentials skip the cases. A health check that is not HTTP
  2xx skips them too. On 2026-10-16 health was up and login, renew,
  and logout returned HTTP 503, so those 10 cases failed. The live
  cert expired 2026-08-02; tests retry once without TLS verify and
  warn.
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

From the 2026-08-22 review. Tracked in
[`CATCHUP.md`](/elements/003-lithium/CATCHUP.md).

| Sev | Issue |
|-----|--------|
| High | `closeManager` lifecycle — fixed in CATCHUP Phase 1 (2026-10-16). `destroy`, else `cleanup`, else `teardown` |
| High | Lookups preview XSS — fixed in CATCHUP Phase 3 (2026-10-16). `escapeHtml`, then highlight, then `sanitizeHtml` |
| High | `marked.parse` → `innerHTML` in Crimson, Version History, and Queries `renderMarkdownPreview` — fixed in Phase 3. Assign is `sanitizeHtml` |
| High | `highlightInline` — fixed in Phase 3. `textContent`, and a language class only for a safe token |
| High | Tour captions, grouping titles, and Queries nav labels — fixed in Phase 3. Text nodes, plus one allowlisted `fa-` icon |
| High | Style Apply / Copy — fixed in CATCHUP Phase 4 (2026-10-16). Non-empty CSS editor text is written to `#style-manager-preview` and copied. Empty buttons stay disabled. Undo and Redo stay disabled. `applyState` is still empty |
| High | Profile `save()` stubs — fixed in Phase 4. Stubs return `{ success: false }` and stay dirty. Language, date formats, photo, and manager 23 still return success |
| High | SunEditor bearer — fixed in Phase 4. `uploadAuthHeader` omits Authorization when `retrieveJWT()` is missing |
| Med | ~14 files over 1000 lines (`tour.js` 2834, `lookups.js` 1888, …) |
| Med | Three ID schemes; utility Terminal=5 vs Crimson=5 |
| Med | Native `alert` / `confirm` / `prompt` — fixed in Phase 4. `rg` of `src/` is empty |
| Med | ~491 `element.style` assignments; ~72 live `console.*` |
| Med | ~138 direct `localStorage` pref writes |
| Med | `public/src` CSS duplicates Vite CSS; orphans `user-profiles/`, `session-logs/`, `style-manager-v2.html` |
| Med | `app.user` never set; `getState()` lies. `USERID` reads `app.auth.user` as of Phase 4 |
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
| Catchup (active) | [`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md) |
| Report writer (Phases 0–71, Phase 0 approved) | [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md) |
| Sprint (superseded) | `docs/Li/plans/LITHIUM_SPRINT.md` |
| Hydrogen script API | `docs/H/api/conduit/script.md` |
| Course Builder pipeline | `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md` |

---

## Session checklist

1. Read **CURRENT PAUSE POINT** in
   [`CATCHUP.md`](/elements/003-lithium/CATCHUP.md).
2. Re-read this file’s Known defects + ID map if touching managers or HTML.
3. One phase at a time. Do not mark a gate green without the named command
   or check actually passing.
4. After JS/CSS: `npm run lint` and `npm test` from `elements/003-lithium`.
5. After HTML templates: `npm run templates:copy`.
6. After Helium Lua: rebuild Hydrogen payload (`mkt`/`mka`) — that is a
   different tree.
7. Close the phase the way [`/docs/PLANNING.md`](/docs/PLANNING.md) says:
   Status, Accomplished, Lessons learned, Handoff, and the Working Log,
   then stop. The next phase is a new conversation.
8. Never commit secrets, JWTs, or `node_modules`. Do not commit unless asked.
