<!-- markdownlint-disable MD007 MD024 -->
# Lithium Catchup Plan

## Purpose

Bring Lithium up to the Hydrogen / Helium surface that already exists, then
ship **Course Manager** as the first official 500 Courses operator
deployment. This file is the **only** active Lithium implementation plan.

It supersedes
[`/docs/Li/plans/LITHIUM_SPRINT.md`](/docs/Li/plans/LITHIUM_SPRINT.md)
(authored 2026-08-22, 0% done). Do not implement from the sprint. Absorb
its still-true work items here.

Agent map (read first every session):
[`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

Coding rules:
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md).

---

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase is its **own conversation**.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**,
  **Reference**, **Work items**, **Exit gate / validation**, **Status**,
  **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and the target phase.
- After each phase: fill **Status**, append **Working Log**, **stop**.
  Do not start the next phase in the same turn unless the user asked to
  continue.
- Never mark a phase complete with a failing or skipped gate unless the
  Status block records an explicit variance.

### Implementor workflow (every phase)

1. Confirm the prior phase Status is complete. Re-read its Exit gate.
   Do not trust memory of a prior session.
2. Discuss this phase first. Read Goal + Work + Exit + Reference. Grep
   the code. Ask questions. **No source edits yet.**
3. Get explicit approval to start implementation.
4. Ask questions during implementation rather than guessing.
5. Update Working Log at major milestones, not only at the end.
6. Record lessons learned.
7. Mark `[x]` and Status complete only after the named commands ran.
   Intent to verify is not verification.
8. **Never apply a database migration.** Prepare Helium packets and hand
   them to the user. Do not run `schematool` / `schemahelper` apply.
9. **No product C.** Do not add Hydrogen routes, chat-mint endpoints, or
   Canvas/Stripe/admin C. Lua is `POST /api/conduit/script`. Tables are
   LithiumTable. The SPA holds a Hydrogen JWT only.

### Testing policy

| Layer | When | What |
|-------|------|------|
| **Vitest** | Every Lithium JS phase | Real functions, not tautological mocks |
| **Lint** | Every Lithium JS/CSS phase | `npm run lint` and `npm run lint:css` from `elements/003-lithium` |
| **Build** | Vite entry, templates, new managers | `npm run build` |
| **Templates** | HTML/CSS under `src/managers/` | `npm run templates:copy` |
| **Hydrogen / Helium** | QueryRefs, `scripts` rows, tableDefs | Packet + `zsh -ic 'mkt'` **by the human** after apply |
| **Manual / live** | Auth, invoke, Course Manager | Named checklist against Hydrogen + JWT |

Lithium commands run in `elements/003-lithium`. Hydrogen aliases are C/Lua
only.

---

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-09):** Plan authored. **Next: Phase 0**
(design lock, no source). No implementation yet.

### Resume here next session

1. Confirm latest completed phase via Status blocks; active phase = first
   not complete.
2. Re-read Working Log decisions that affect the next chunk.
3. Re-read [`AGENTS.md`](/elements/003-lithium/AGENTS.md) Known defects +
   ID map.
4. Baseline: `cd elements/003-lithium && npm test && npm run lint`.
5. One phase: discuss → approval → implement → verify exit gate → update
   this doc → stop.

---

## Hard rules (do not relitigate)

1. **No product C.** Hydrogen already has auth, Conduit, `/api/conduit/script`,
   chat WS, MCP, Mail Relay. Lithium is the SPA.
2. **SPA holds a Hydrogen JWT only.** Never Keycloak `id_token` /
   `access_token`, never Canvas tokens, never `sk_` / `whsec_`.
3. **Lua via `POST /api/conduit/script`** (+ `GET /api/conduit/script/{job_id}`).
   Do not send `params._hydrogen`. Name is `Group.Name` (dot only).
   `invokable=0` → Hydrogen 404 `script_not_found` (existence-hiding).
4. **Helium packets, not agent apply.** New QueryRefs / `invokable` flips /
   menu rows / tableDefs are migrations the human applies.
5. **LithiumTable** (`src/tables/lithium-table-main.js`) is the table path.
   Do not invent a second grid.
6. **Reception is a different SPA.** Learner catalog / cart / My Courses
   stay in `/mnt/extra/Projects/500-Courses-Reception/`. Lithium at
   `lithium.500courses.com` is the operator surface.
7. **Course Builder pipeline** stays
   [`COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)
   (CB-0..35). This plan only gates the Lithium operator UI.

---

## First official deployment

**500 Courses Course Manager (menu ID 34)** on `lithium.500courses.com`.

Band D is the deployable product. Bands E–I do not block that deploy.
Authz for v1 is the Hydrogen JWT `roles` claim (staff/admin strings locked
in Phase 0). Do **not** wait for Hydrogen 2.23 / AUTH_FINALE Phase 2
`account_roles` rows. When 2.23 lands, Band H records the cutover; the
manager UI contract does not change.

Reception PRIORITIZE Part 5 is the requirements source:
[`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md)
(§ Part 5). That doc still says “do not ship until 2.23”; **this plan
overrides that gate** with JWT `roles`. Staff Lua stays `invokable=0`
until Band D’s Helium packet (human apply) flips the known button list.

---

## Goals

1. Safe close / activate — every manager tears down editors and tables.
2. No XSS on HTML sinks — DOMPurify or shared `escapeHtml`.
3. Honest UI — no success toast on no-ops.
4. One ID scheme — docs, registry, config, tours agree.
   **34 = Course Manager. 35 = Course Builder.**
5. Shared `invokeScript` / `getScriptJob` used by Scripting, Course
   Manager, and Course Builder.
6. Login: OIDC (500 Passwords) + ordered partner buttons that actually
   work or are absent.
7. Scripting Manager can invoke, poll, and show `invokable`.
8. CodeMirror 6 plugins are split, tested, and the only editor stack.
9. Course Manager v1: courses, commerce, learners/Canvas, inbox, staff
   script console. Operators never see the word “Lua”.
10. Course Builder operator queue after Helium `course_build_*` exists.
11. Crimson / Chats / MCP operator UI catch up to CHAT_FINALE + MCP
    without becoming an MCP daemon.
12. Mail / Jobs / Dashboard / Roles stop being copy-paste shells.
13. AUTH_FINALE and Notifications are **consumed**, not re-implemented.
14. Docs match runtime.

### Non-goals

- Reception public SPA, Cap PoW, student chrome.
- Product Hydrogen C (`/api/coursebuilder/*`, `/api/enroll/*`,
  `/api/admin/*`, `/api/stripe/*`, browser MCP on `:3100`).
- Chat JWT mint in C, unless a **separate** Hydrogen plan is approved.
  CATCHUP maps `JWT not authorized for chat` honestly and waits.
- `alt_chat`, public chat, MCP DCR/stdio, QueryRef **#061** from the SPA
  (engines **with API keys**).
- Filling every placeholder (Calendar, Contacts, Files, …). Collapse;
  implement only what a later band names.
- a11y audit, i18n, CSP meta, Lighthouse budget, Playwright CI.
- Impersonating a learner from Lithium.
- Showing `sk_` / Canvas admin tokens.
- APPLY/LOAD migrations from the SPA.
- Luacheck-for-SPA or run-as-role (no Hydrogen route).
- Editing Keycloak credentials from Lithium.

---

## Current Observed State (2026-09-09)

### Hydrogen / Helium already done (do not rebuild)

| Surface | Evidence |
|---------|----------|
| Password + OIDC RP + handoff | AUTH_FINALE shipped RP; last-method polish is OIDC-PLAN Phase 26 |
| `POST /api/conduit/script` | LUA_CLIENT complete |
| Chat WS + REST `auth_chat` / `auth_chats` | CHAT_FINALE complete; JWT `aud=hydrogen-chat` AND `roles=="chat"` **exact** |
| MCP Streamable HTTP + `#152`/`#153` | MCP complete; product tool = `System.Info` |
| Mail Relay outbound + templates + OTP C API | MAILRELAY complete; Lithium UI is not |
| Course learner Lua | `Enroll.*`, `Catalog.*`, `Account.*`, `Stripe.*`; staff writers `invokable=0` |
| QueryRefs | Scripting 87/89/90/129/130; delete **131**; Enrolment History **#150**; Management History **#151**; suggestions insert **#085**; MCP **#152/#153**; chat store **#067/#068/#069** |

### Lithium today

| Surface | Reality |
|---------|---------|
| Login OIDC | `renderOidcProviders` from `auth.oidc_providers` (500passwords) |
| Partner buttons | Hardcoded in `login.html`; `login partners` booleans unused; **no JS handlers** |
| Scripting ID 33 | List/edit/save; **no invoke**; `scripting.js` 1004 lines |
| `conduit.js` | Query only — no `invokeScript` |
| Crimson | Live WS chat; login JWT; hardcoded engine `"Crimson"` |
| Chats ID 18 | 49-line placeholder |
| Course Manager | **Does not exist** |
| Course Builder | **Does not exist** (sprint reserved 34; this plan moves it to 35) |
| Mail / Jobs / Dashboard / Roles | Placeholders |
| CM6 | Real: `codemirror-setup.js`, `cm6-virtual-columns.js`, `cm6-custom-scrollbars.js` |
| CM5 | `src/init/codemirror-init.js` dead (zero imports) |
| `closeManager` | Calls `destroy()` only — leaks Tabulator/CM |
| DOMPurify | Dependency unused |
| `escapeHtml` | Real in `src/core/utils.js`; Lookups has a local no-op |

---

## Phase Groups

| Group | Phases | Theme |
|-------|--------|-------|
| 0 | 0 | Design lock (no source) |
| A | 1–4 | Lifecycle, sanitize, invoke helper, XSS, honest UI |
| B | 5–6 | Login partners + OIDC verification |
| C | 7–11 | Scripting Manager + CodeMirror productize |
| **D** | **12–18** | **Course Manager — first deploy** |
| E | 19–23 | Course Builder operator (ID 35) |
| F | 24–28 | Crimson / Chats / MCP catch-up |
| G | 29–32 | Placeholders, Mail, Jobs, Dashboard/Roles |
| H | 33–34 | AUTH_FINALE + Notifications **consumers** (gated) |
| I | 35–38 | File splits, LithiumTable, docs/tests, closeout |

---

## Canonical ID map (lock in Phase 0, write into `LITHIUM-MGR.md`)

| ID | Name | Module | Notes |
|----|------|--------|-------|
| 1 | Login | `managers/login/` | Not in menu registry |
| 2 | Menu / Main | `managers/main/` | |
| 3 | User Profile | `profile-manager/` | Utility |
| 4 | Session Log | `session-log/` | Utility |
| 5 | Crimson | `crimson/` | **Popout only.** Not a slot manager. |
| 6 | Tour | `tour/` | Not a slot manager |
| 7 | Dashboard | `dashboard/` | Placeholder until Band G |
| 8 | Mail | `mail-manager/` | Placeholder until Band G |
| 9 | Server Profiles | `server-profiles.js` | |
| 10 | Server Sessions | `server-sessions.js` | |
| 11 | Version | `version-history/` | |
| 12–17, 19–21, 24, 26–28, 30–31 | Calendar…Camera | placeholders | Collapse in Phase 29 |
| 18 | Chats | `chats/` | History manager. **Not** Crimson. Band F. |
| 22 | Style | `style-manager/` | |
| 23 | Lookups | `lookups/` | |
| 25 | Role | `role-manager/` | Placeholder until Band G |
| 29 | Queries | `queries/` | Canonical LithiumTable consumer |
| 32 | Terminal | `terminal/` | Menu 32. Utility key `terminal` **without** numeric 5. |
| 33 | Scripting | `scripting/` | |
| **34** | **Course Manager** | `course-manager/` | **First deploy. Not created.** |
| **35** | **Course Builder** | `course-builder/` | Pipeline UI. Not created. |

Tour matching uses **numeric ID only**. Do not give Terminal utility
numeric 5 (collides with Crimson tours).

---

## Locked defaults (Phase 0 confirms; do not bikeshed)

| # | Decision | Default |
|---|---------|---------|
| L1 | Course Manager ID | **34** |
| L2 | Course Builder ID | **35** |
| L3 | Course Manager authz v1 | JWT `roles` contains `staff` or `admin` (document exact match vs substring in Phase 0 Working Log after reading a live token). Hide manager + disable invoke otherwise. |
| L4 | 2.23 `account_roles` | Not a v1 ship gate. Band H cutover later. |
| L5 | Terminal vs Crimson | Crimson = system 5 popout. Terminal = menu 32 + utility key `terminal` only. |
| L6 | Lifecycle | `closeManager` → `destroy` else `cleanup` else `teardown`. `show`/`hide` → `onActivate`/`onDeactivate`. |
| L7 | Partner buttons | Render only if `login partners.<id>` is true **and** a Hydrogen OIDC provider with that `id` exists (or Phase 0 records a different start path). Else **omit** (INS: no dead buttons). Order: Didit, Apple, Google, Microsoft, then `auth.oidc_providers`. |
| L8 | Last-method polish | OIDC-PLAN Phase 26 is done. Only extend `.is-recent` to partners if handlers exist. |
| L9 | Chat JWT | CATCHUP does **not** add C. Crimson keeps sending the login JWT until a Hydrogen mint exists; surface `JWT not authorized for chat` / `error_code` honestly. |
| L10 | Crimson vs Chats vs MCP | Three UIs. Crimson = in-app assistant. Chats = persisted threads (#069/#068). MCP = operator status/catalog, never browser JSON-RPC to `:3100`. |
| L11 | Engine list | New invokable/`mcp_access` Lua **without keys**. Never QueryRef #061 from the SPA. |
| L12 | Hosted MCP `allowed_tools` | Stay `["System.Info"]` until Phase 28 packet. Empty list = all tools (fail-closed today). |
| L13 | Script rename | Forbidden in v1 (edit in place). |
| L14 | Punchcard | Menu data is the gate plus JWT roles for Course Manager. `getPermittedManagers()` must not claim `[7..33]` after 34/35 exist. |
| L15 | Style Apply | Implement inject if CSS text exists; else disable buttons. No success toast on no-op. |
| L16 | `codemirror-init.js` | Dead CM5. Delete in Phase 11. |
| L17 | Staff script `invokable` | Flip to 1 only in the Band D packet, with Course Manager UI live. |
| L18 | CATCHUP vs sprint | CATCHUP wins. Sprint is history. |

---

# Phase 0 — Design lock

**Goal:** Freeze IDs, authz, login order, chat/MCP split, and Helium
QueryRefs so later phases do not invent a second scheme.

**Dependencies:** None.

**Entry gate:**

- [ ] This plan and `AGENTS.md` have been read.

**Reference:**

- [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md)
- [`/elements/003-lithium/src/app/manager-loader.js`](/elements/003-lithium/src/app/manager-loader.js)
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
- [`/docs/Li/LITHIUM-MGR.md`](/docs/Li/LITHIUM-MGR.md)
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)
- [`/docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)
- [`/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md`](/mnt/extra/Projects/500-Courses-Reception/PRIORITIZE.md) Part 5
- Helium `acuranzo_1204`/`1206`/`1207`/`1263`–`1265`/`1297` (scripts);
  `1326` (#150); `1335` (#151)

**Work items:**

- [ ] Write the canonical ID table (above) into `LITHIUM-MGR.md`.
- [ ] Confirm Scripting QueryRefs 87/89/90/129/130/131 and whether list/
      detail/update SELECT `invokable` (1297 added the column; CRUD may
      omit it). Log the table: column → QueryRef.
- [ ] Confirm Course Manager QueryRefs that already exist (#150, #151,
      #085, Catalog/Enroll/Stripe script names). Do not invent refs.
- [ ] Read one live Hydrogen JWT `roles` value (or Test 40 fixture) and
      write the exact v1 match rule (substring vs list vs exact).
- [ ] Confirm `login partners` key (space in JSON) vs `oidc_providers`.
      Record whether Didit/Apple/Google/Microsoft exist on
      `OIDC_RP.Providers[]` for lithium.philement.com and
      lithium.500courses.com.
- [ ] Record L1–L18 in Working Log `P0` as accepted or with a variance
      the user signed.
- [ ] Confirm `LITHIUM-TOC.md` / `LITHIUM-DEV.md` already say Vite runs
      without Hydrogen (login/data need it). Log, do not rewrite unless
      drift is found.

**Exit gate / validation:**

- [ ] `LITHIUM-MGR.md` ID table matches `manager-loader.js` 7–33 plus
      reserved **34** and **35**.
- [ ] Working Log `P0` has QueryRef table, JWT roles rule, partner-provider
      existence, and L1–L18.
- [ ] No Lithium/Hydrogen/Helium source changed except `LITHIUM-MGR.md`
      (docs-only phase).

**Status:** pending

**Lessons learned:**

---

# Band A — Honesty, XSS, invoke helper

## Phase 1 — Manager close / activate lifecycle

**Goal:** Closing a manager destroys Tabulator, CodeMirror, splitters,
and listeners.

**Dependencies:** Phase 0 lifecycle lock (L6).

**Entry gate:**

- [ ] Phase 0 Status complete.

**Reference:**

- [`/elements/003-lithium/src/app/manager-loader.js`](/elements/003-lithium/src/app/manager-loader.js)
- Queries/Lookups/Scripting `cleanup()` vs Login `teardown()` vs
  Terminal `destroy()`

**Work items:**

- [ ] `closeManager` tries `destroy`, then `cleanup`, then `teardown`
      (await, log failures).
- [ ] `showManager` / hide call `onActivate` / `onDeactivate` when present.
- [ ] Delete dead `_importManager` cases 1–6 (wrong modules, including
      Chats as case 6).
- [ ] Unit test: fake manager with only `cleanup()` is invoked; only
      `destroy()` still works.

**Exit gate / validation:**

- [ ] `npm test && npm run lint`
- [ ] Grep: `closeManager` no longer calls `destroy` exclusively.

**Status:** pending

**Lessons learned:**

---

## Phase 2 — Shared `escapeHtml` / `sanitizeHtml` / `invokeScript`

**Goal:** One sanitize path and one script-invoke client. No manager UI
yet.

**Dependencies:** Phase 0.

**Entry gate:**

- [ ] Phase 0 complete. May parallel Phase 1 if this phase does not
      touch `manager-loader.js`.

**Reference:**

- [`/elements/003-lithium/src/core/utils.js`](/elements/003-lithium/src/core/utils.js)
- [`/elements/003-lithium/src/shared/conduit.js`](/elements/003-lithium/src/shared/conduit.js)
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)
- `tests/unit/conduit.test.js`

**Work items:**

- [ ] Confirm `escapeHtml` in `utils.js`; tests for `& < > " '`.
- [ ] Add `sanitizeHtml(html)` wrapping the existing DOMPurify
      dependency. Use for markdown → HTML later.
- [ ] Add `invokeScript(api, { script, params, wait, timeoutSeconds })`
      and `getScriptJob(api, jobId)`. Radar blips + `error.serverError`
      parity with `authQuery`.
- [ ] Reject slash names; require `script`; never send `_hydrogen`;
      clamp timeout 1–60 on the client if you add a clamp.

**Exit gate / validation:**

- [ ] `npm test` includes utils + conduit script tests.
- [ ] `npm run lint`
- [ ] No manager calls the helper yet.

**Status:** pending

**Lessons learned:**

---

## Phase 3 — XSS sinks

**Goal:** Untrusted and markdown content cannot execute script.

**Dependencies:** Phase 2.

**Entry gate:**

- [ ] Phase 2 `escapeHtml` / `sanitizeHtml` shipped.

**Reference:**

- `src/managers/lookups/lookups.js` local no-op escape
- `src/managers/crimson/crimson-chat.js`
- `src/managers/version-history/`
- `src/init/highlight-init.js`

**Work items:**

- [ ] Lookups processors: real `escapeHtml` then highlight; sanitized
      assign.
- [ ] Crimson + Version History: `sanitizeHtml(marked.parse(...))`.
- [ ] `highlight-init.js`: do not interpolate raw `code` / `language`
      into `innerHTML`.
- [ ] Tour captions: `textContent` + icon nodes, or sanitize allowlist.
- [ ] Grouping / queries popup titles: `textContent`.
- [ ] Unit tests with `<img onerror>` / `<script>` fixtures.

**Exit gate / validation:**

- [ ] Grep Lookups shows real entity escapes or `escapeHtml`.
- [ ] `npm test && npm run lint`
- [ ] Manual: lookup summary `<img onerror=alert(1)>` does not fire
      (Working Log).

**Status:** pending

**Lessons learned:**

---

## Phase 4 — Honest UI (dialogs, saves, SunEditor JWT)

**Goal:** Buttons work or refuse. No native dialogs on hot paths. Upload
Authorization is a real Bearer token.

**Dependencies:** Phase 0. Phase 1 recommended for cleanup. Phase 2 for
nothing here.

**Entry gate:**

- [ ] Phase 0 complete. Existing toast / confirm pattern cited in log.

**Reference:**

- `lithium-table-ops.js`, `lithium-table-ui.js`, `queries-navigation.js`
- Style Manager `applyState`; Profile `page-*/save()`
- `src/init/suneditor-init.js`
- `jwt.js` `retrieveJWT`

**Work items:**

- [ ] Replace `window.confirm` / `prompt` / `alert` on the hottest
      paths (table ops, queries nav/templates, template-popup,
      jsoneditor-init, camera-popout).
- [ ] Remove Login / Main / Session Log / Profile `renderFallback` HTML
      clones; fail with toast + session log.
- [ ] Remove Lookups JSON textarea fallback.
- [ ] Style Manager: implement `applyState` **or** disable Apply / Undo /
      Redo / Copy (L15). `copyCssToClipboard` copies real CSS.
- [ ] Profile page `save()` stubs: return failure or disable. No
      `{ success: true }` on no-ops.
- [ ] `USERID` from `this.app.auth.user`, not `this.app.user` (Scripting,
      Queries, Lookups when touched).
- [ ] SunEditor: `retrieveJWT()`; missing token must not produce
      `"Bearer null"`.

**Exit gate / validation:**

- [ ] `rg "\\balert\\(|\\bconfirm\\(|\\bprompt\\(" src/` empty or
      leftovers listed with target phase.
- [ ] `rg renderFallback src/` empty.
- [ ] Profile stub `success: true` grep empty.
- [ ] Unit: missing JWT ≠ `"Bearer null"`.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

# Band B — Login

## Phase 5 — Ordered partner logins

**Goal:** Didit / Apple / Google / Microsoft are real buttons or absent.
500 Passwords stays `auth.oidc_providers`.

**Dependencies:** Phase 0 L7–L8.

**Entry gate:**

- [ ] Phase 0 Working Log records whether each partner id exists on
      Hydrogen `OIDC_RP.Providers[]` for the target deploys.

**Reference:**

- [`/elements/003-lithium/src/managers/login/login.js`](/elements/003-lithium/src/managers/login/login.js)
- [`/elements/003-lithium/src/managers/login/login.html`](/elements/003-lithium/src/managers/login/login.html)
- [`/elements/003-lithium/config/lithium.json`](/elements/003-lithium/config/lithium.json)
  `login partners`, `auth.oidc_providers`
- [`/docs/Li/LITHIUM-OIDC.md`](/docs/Li/LITHIUM-OIDC.md)
- [`/docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md)
- `tests/unit/managers/login/login.test.js`

**Work items:**

- [ ] Remove hardcoded partner markup as the live path. Render partners
      from config (same row as OIDC).
- [ ] Honor `login partners` booleans. `false` or missing Hydrogen
      provider → do not render (no greyed-out fake).
- [ ] Click → `startOidc(id)` (or the Phase 0-recorded start path).
      Record `auth.last_method`. Apply `.is-recent` like OIDC.
- [ ] Visual order: Didit, Apple, Google, Microsoft, then OIDC providers.
- [ ] Do not put Keycloak tokens in the SPA. Do not add C.
- [ ] If a partner has no Hydrogen provider, Working Log lists it as
      hidden; do not invent a second protocol.

**Exit gate / validation:**

- [ ] Unit tests: boolean off → no button; boolean on + provider →
      click calls `startOidc` with that id; order asserted.
- [ ] `npm test && npm run lint`
- [ ] Manual checklist if a provider is live (log which).

**Status:** pending

**Lessons learned:**

---

## Phase 6 — OIDC client verification

**Goal:** Lithium OIDC path still matches the Keycloak recipe after
Band A/B churn. Do not block on AUTH_FINALE Phase 11 OTP.

**Dependencies:** Phase 5 (partners must not have broken OIDC).

**Entry gate:**

- [ ] Hydrogen `OIDC_RP.Enabled` known true/false on the target (log).

**Reference:**

- [`/docs/Li/LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md)
- `src/core/oidc-client.js`
- AUTH_FINALE Phase 11 is **ops**, not this phase.

**Work items:**

- [ ] Walk KEYCLOAK client checklist against `oidc-client.js` + login.
- [ ] Integration: invalid handoff → 401; unknown provider does not
      fall back (mock or live).
- [ ] Confirm password login still works with IdP down (if testable).
- [ ] Provider `id` must match `OIDC_RP.Providers[].Name`.

**Exit gate / validation:**

- [ ] Working Log: enabled? start URL? provider id match?
- [ ] `npm test`. If real Keycloak E2E still OTP-blocked, `[~]` and do
      **not** block CATCHUP.

**Status:** pending

**Lessons learned:**

---

# Band C — Scripting Manager and CodeMirror

## Phase 7 — Scripts schema and QueryRef audit

**Goal:** Scripting matches live `scripts`, including `invokable`. Helium
packet ready if CRUD queries omit the column.

**Dependencies:** Phase 0 QueryRef confirm.

**Entry gate:**

- [ ] Phase 0 Working Log has the scripts QueryRef table.

**Reference:**

- Helium `acuranzo_1204` (87), `1206` (89), `1207` (90), `1263` (129),
  `1264` (130), `1265` (131), `1297` (`scripts.invokable`)
- Lookup 59 tableDef `scripts/script-manager`
- [`/docs/H/api/conduit/script.md`](/docs/H/api/conduit/script.md)

**Work items:**

- [ ] Inventory columns vs QueryRefs vs tableDef. Confirm `invokable`
      on list/detail/insert/update. Confirm delete 131 params
      (`GROUP_NAME` + `SCRIPT_NAME`, not INTEGER pk).
- [ ] If queries omit `invokable`, prepare Helium packet (human apply).
      Do not apply.
- [ ] List seeded `invokable=1` scripts (`Api.Echo`, …) in Working Log.
- [ ] Confirm Lookup 59 exposes `invokable` and `deleteQueryRef: 131`.

**Exit gate / validation:**

- [ ] Working Log table: column → QueryRef / tableDef field.
- [ ] Packet handed over **or** log that live queries already SELECT
      `invokable`.
- [ ] If Helium changed: human `mkt` after apply; name the migration
      test in the log.

**Status:** pending

**Lessons learned:**

---

## Phase 8 — Scripting CRUD honesty

**Goal:** Create / update / duplicate / delete are real. `scripting.js`
under 1000 **before** invoke UI lands.

**Dependencies:** Phases 1, 4 (`USERID`), 7.

**Entry gate:**

- [ ] Phase 7 schema table exists. Phase 1 close path exists.

**Reference:**

- `src/managers/scripting/scripting.js` (1004)
- `src/managers/scripting/scripting-editors.js`
- Queries delete pattern (`queries-navigation.js`)

**Work items:**

- [ ] Split footer / save-payload builder out of `scripting.js` so the
      file is under 1000 before Phase 9.
- [ ] Save/insert/update use confirmed QueryRefs and `app.auth.user`.
- [ ] Custom delete: composite PK + Phase 4 modal. Do not use generic
      INTEGER `selectedId`.
- [ ] Dirty prompt on row switch. Rename forbidden (L13).
- [ ] `cleanup()`: table, editors, splitter, **keydown** listener.
      `destroyCodeMirrorScrollbars` must not stay a no-op if scrollbars
      are created.
- [ ] Extract save-payload builder + unit tests.

**Exit gate / validation:**

- [ ] `wc -l src/managers/scripting/scripting.js` ≤ 1000
- [ ] Unit tests for save payload.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 9 — Invoke panel and async poll

**Goal:** Operator can run `Api.Echo` (wait true) and poll `wait: false`.
Course Manager will clone this pattern, not `scripting.js`.

**Dependencies:** Phases 2, 7, 8.

**Entry gate:**

- [ ] `invokeScript` exists. `Api.Echo` (or named fixture) is
      `invokable=1` in the target DB.

**Reference:**

- Queries Test tab + JSON collection editor (`queries-editors.js`)
- New `scripting-invoke.js` (do not grow `scripting.js`)

**Work items:**

- [ ] Run tab: JSON params via `buildEditorExtensions({ language: 'json' })`,
      wait default true, timeout 1–60, Run disabled unless `invokable`.
- [ ] Result pane: status, `job_id`, JSON / error. Treat 404
      `script_not_found` as not-runnable, not “row missing”.
- [ ] `wait: false` → `getScriptJob` backoff until
      `completed|failed|killed|timeout`. No fake cancel.
- [ ] Strip `_hydrogen`. Do not invent run-as-role.
- [ ] Unit tests: payload, invokable gating, poll machine.

**Exit gate / validation:**

- [ ] Manual: JWT → Scripting → `Api.Echo` → body in pane (log job_id).
- [ ] Manual: wait off → spinner → terminal status (log).
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 10 — Scripting UX, tests, docs

**Goal:** Band C Scripting closable without blocking Course Manager if
Phase 9 already works. Prefer finishing this before Band D unless the
user pulls Course Manager forward.

**Dependencies:** Phases 3, 8, 9.

**Entry gate:**

- [ ] Phase 9 sync invoke works against Hydrogen.

**Reference:**

- `docs/Li/LITHIUM-MGR-SCRIPT.md` (does not exist yet)

**Work items:**

- [ ] Preview = sanitized **summary** markdown (Phase 3 helper), not
      escaped Lua source.
- [ ] Fonts via `lithiumSettings`, not raw `lithium_scripting_font_*`.
- [ ] Implement or remove Ctrl+P (`handlePrettify` is missing).
- [ ] Optional meta tab: invokable, schedule, last run.
- [ ] `tests/unit/managers/scripting-*.test.js`.
- [ ] Write `LITHIUM-MGR-SCRIPT.md`; TOC + MGR links.
- [ ] All `src/managers/scripting/*` ≤ 1000 lines.

**Exit gate / validation:**

- [ ] `npm test && npm run lint`
- [ ] `wc -l src/managers/scripting/*` all ≤ 1000
- [ ] TOC lists `LITHIUM-MGR-SCRIPT.md`

**Status:** pending

**Lessons learned:**

---

## Phase 11 — Productize CodeMirror plugins

**Goal:** CM6 is the only stack; custom plugins are modules with tests;
Lua folds.

**Dependencies:** Phase 8 (scripting already uses `buildEditorExtensions`).
May proceed after Phase 8 even if Phase 9/10 are in flight **only** if
the user approves parallel — default is serial.

**Entry gate:**

- [ ] Phase 8 complete (editor facades stable).

**Reference:**

- `src/core/codemirror.js`, `codemirror-setup.js` (1019),
  `cm6-virtual-columns.js`, `cm6-custom-scrollbars.js`,
  `editor-footer.js`
- Dead: `src/init/codemirror-init.js`
- `package.json` CM packages (several runtime imports live in
  `devDependencies`)

**Work items:**

- [ ] Move runtime CM packages from `devDependencies` → `dependencies`.
      Declare `@codemirror/language` if imported. Drop or use
      `@codemirror/legacy-modes`.
- [ ] Delete `codemirror-init.js` if still unimported (L16).
- [ ] Split `codemirror-setup.js` under 1000: newline plugin, indent
      facet/filter/Tab-Enter, JSON sort helpers, fold-to-level.
      Leave `buildEditorExtensions` as the facade.
- [ ] Deduplicate virtual-columns registration (compartment **or**
      unconditional push, not both).
- [ ] Tests for `virtualColumns` pad/trim and scrollbar plugin destroy.
- [ ] Lua: replace homemade tokenizer with `@codemirror/legacy-modes`
      `lua` **or** fix StreamLanguage (long-bracket bug:
      `stream.match` assigned as function; `foldNodeProp` so fold-all
      works). Optional `H.` highlight.
- [ ] Footer error count stays 0 until a real lint surface exists. Do
      not pretend luacheck.

**Exit gate / validation:**

- [ ] `wc -l src/core/codemirror-setup.js` ≤ 1000
- [ ] `npm test && npm run lint && npm run build`
- [ ] Grep: no imports of `codemirror-init.js`

**Status:** pending

**Lessons learned:**

---

# Band D — Course Manager (first official deployment)

Requirements: PRIORITIZE Part 5. Operators must not need Lua, QueryRefs,
or `H.http`. Every action is JWT `invokeScript` or a staff QueryRef +
LithiumTable.

## Phase 12 — Course Manager scaffold (ID 34)

**Goal:** Loadable manager, JWT-roles gated, no fake success, no Course
Builder widgets.

**Dependencies:** Phases 0 (L1, L3), 1, 2.

**Entry gate:**

- [ ] Phase 0 reserved ID 34. Phase 1 close path exists. Phase 2
      `invokeScript` exists.

**Reference:**

- PRIORITIZE §5 (authz paragraph overridden by L3)
- New-manager checklist in AGENTS.md
- `src/app/manager-loader.js`, `config/lithium.json`

**Work items:**

- [ ] `src/managers/course-manager/` js/html/css. Register **34** in
      `managerRegistry`, `_importManager`, `lithium.json`.
- [ ] `cleanup()` implemented.
- [ ] Hide / refuse load when JWT `roles` fails L3. Honest empty/denied
      state — not a placeholder “under development” that looks like a
      bug.
- [ ] Helium menu/lookup seed packet **or** documented temporary load
      path. No silent dead ID.
- [ ] Docs stub `docs/Li/LITHIUM-MGR-COURSE.md` pointing at PRIORITIZE
      Part 5. Word “Lua” does not appear in the UI.
- [ ] Do not impersonate learners. Do not show secrets.

**Exit gate / validation:**

- [ ] `npm run build && npm test && npm run lint`
- [ ] Loader imports ID 34 without throw.
- [ ] Unit: roles gate helper (allow/deny fixtures).

**Status:** pending

**Lessons learned:**

---

## Phase 13 — Courses (brochure SKU)

**Goal:** List / create-link / edit Lithium-owned fields / retire /
sync-from-Canvas CTAs.

**Dependencies:** Phase 12. Phase 9 pattern recommended.

**Entry gate:**

- [ ] Phase 12 Status complete. Helium `courses` tableDef / list
      QueryRef identified in Working Log (do not invent).

**Reference:**

- PRIORITIZE §5.1
- `Catalog.SyncFromCanvas`, `Catalog.Retire` (still `invokable=0` until
  Phase 18 packet)
- QueryRefs **#150** Enrolment History, **#151** Management History

**Work items:**

- [ ] LithiumTable of all `courses` (published, unpublished, **retired**).
      Search code/slug/title/`canvas_course_id`. Retired chip.
- [ ] Create Lithium catalog row: `code`, `slug`, `canvas_course_id`,
      `pricing_type`, `image_path` URL. Sync must not auto-INSERT.
- [ ] Edit Lithium-owned fields only. Canvas unpublish is **not** a
      Lithium checkbox — CTA to open Canvas URL; sync flips `published`.
- [ ] Tags editor (JSON key/value) if tableDef allows; v1 may be a
      sanitized JSON editor.
- [ ] Buttons call `invokeScript` when `invokable`; else disabled with
      tooltip “not enabled on this server” (do not fake).
- [ ] History popups: #150 / #151 via LithiumTable or sanitized `<pre>`.
- [ ] Stripe ids read-only.

**Exit gate / validation:**

- [ ] Manual: list fixture courses; retired filter; denied role sees
      nothing useful.
- [ ] `npm test && npm run lint`
- [ ] All new files ≤ 1000 lines.

**Status:** pending

**Lessons learned:**

---

## Phase 14 — Catalog commerce (Stripe)

**Goal:** Prices and Stripe product ensure/refund actions without
exposing `sk_`.

**Dependencies:** Phase 13.

**Entry gate:**

- [ ] Phase 13 list/detail works.

**Reference:**

- PRIORITIZE §5.2
- `Stripe.EnsureProduct`, `Stripe.Refund`, `Stripe.DeactivateCustomer`
  (`invokable=0` until Phase 18)
- `Account.Orders`

**Work items:**

- [ ] View/edit `course_prices` (CAD/USD/EUR/GBP).
- [ ] **Ensure Product** CTA when ids missing (disabled until
      invokable).
- [ ] Price change = new Stripe Price (script), keep old id for
      receipts.
- [ ] Show test vs live from pod/config `STRIPE_MODE` — never `sk_`.
- [ ] Orders list for course or account (`pending|completed|failed`).
- [ ] Refund / deactivate: real script or Dashboard deep link. **No
      fake button.** Unpublish must not expose “delete Stripe Product.”

**Exit gate / validation:**

- [ ] Grep manager: no `sk_`, `rk_`, `whsec_`.
- [ ] Unit: EnsureProduct payload (`course_id`).
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 15 — Learners and Canvas links

**Goal:** Search accounts; seats; Canvas link/relink without a browser
Canvas token.

**Dependencies:** Phase 12.

**Entry gate:**

- [ ] Phase 12 complete. `account_canvas_links` QueryRef or script
      identified.

**Reference:**

- PRIORITIZE §5.3–5.4
- `Enroll.FreeCourse`, `Enroll.Archive`, `Enroll.RenewFree`,
  `Provision.EnsureCanvasUser`, `Stripe.SyncCustomer`

**Work items:**

- [ ] Account search (email, Keycloak sub, account id).
- [ ] Detail: prefs + `user_registration_meta` read-only. Enrollments,
      orders, Canvas link, Stripe `cus_` (not secrets).
- [ ] Grant/revoke seats via named scripts. No “fake paid” button.
- [ ] Canvas aliases via script + `H.http` on the server. Relink/merge
      UI only if the Helium script exists; else show both ids and a
      documented ops path.
- [ ] Open Canvas course/user in a new tab (URL only).
- [ ] Do not impersonate. Do not PATCH Keycloak.

**Exit gate / validation:**

- [ ] Manual: search one account; enroll/archive disabled-or-real
      (log which).
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 16 — Inbox (suggestions + contact)

**Goal:** Humans triage `course_suggestions` and `contact_submissions`
without raw SQL.

**Dependencies:** Phase 12.

**Entry gate:**

- [ ] Phase 12 complete. Insert QueryRef **#085** exists; list/triage
      QueryRefs confirmed or packet prepared.

**Reference:**

- PRIORITIZE §5.5
- COURSEBUILDER CB-9 (queue only — do not build the LLM pipeline)
- Helium `course_suggestions`, contact table + QueryRef #086 insert

**Work items:**

- [ ] LithiumTable list suggestions. Accept / decline / junk buttons
      via scripts or QueryRefs (packet if missing).
- [ ] LithiumTable contact submissions. Mark handled.
- [ ] Honest empty state. These queues are required once Course
      Manager exists — not a “later” tab with a fake badge.

**Exit gate / validation:**

- [ ] Manual: fixture suggestion visible; XSS in body does not fire.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 17 — Staff script console

**Goal:** Invokable scripts as labelled forms, not a JSON blob as the
only UI. Word “Lua” does not appear.

**Dependencies:** Phases 2, 9, 12.

**Entry gate:**

- [ ] Phase 9 invoke helper proven. Phase 12 roles gate works.

**Reference:**

- PRIORITIZE §5.7 button table
- Live `scripts` where `invokable=1`

**Work items:**

- [ ] List invokable scripts (group/name, one-line purpose).
- [ ] Param schemas: labelled fields / pulldowns (courses, accounts,
      currencies, `action` enums). JSON blob allowed as escape hatch,
      not the only path for the known button list.
- [ ] Run `wait: true`; show ok/error, counts, ids.
- [ ] Non-invokable (`Stripe.Webhook`, `Enroll.PaidCourse`,
      `Enroll.LogEvent`) listed system-only, not runnable.
- [ ] Small catalog of param schemas so a new migration can add a
      script without a Lithium rewrite (data, not C).

**Exit gate / validation:**

- [ ] Unit: schema → payload for `Catalog.SyncFromCanvas` and
      `Enroll.FreeCourse`.
- [ ] Manual: `Api.Echo` from the console if still invokable.
- [ ] `npm test && npm run lint`
- [ ] UI copy grep: no “Lua” in course-manager templates.

**Status:** pending

**Lessons learned:**

---

## Phase 18 — First deploy: invokable packet + 500 Courses smoke

**Goal:** Course Manager is shippable on `lithium.500courses.com`.

**Dependencies:** Phases 12–17. Phase 10 may still be open (Scripting
docs) — do not block deploy on Scripting polish.

**Entry gate:**

- [ ] Phases 12–17 Status complete or explicit `[~]` with user sign-off.
- [ ] Operator ready to apply Helium packet and roll
      `lithium-500courses`.

**Reference:**

- PRIORITIZE §5.7 known buttons
- Deploy: `npm run deploy:500courses` / operator roll notes in
  PRIORITIZE

**Work items:**

- [ ] Helium packet: flip `invokable=1` on the agreed staff list
      (`Catalog.SyncFromCanvas`, `Catalog.Retire`, `Stripe.EnsureProduct`,
      Stripe customer/refund scripts if ready, `Enroll.*` operator
      scripts, `Provision.EnsureCanvasUser`,
      `Mail.Notices.CourseExpiration` if Band G has not landed — **only
      scripts whose UI exists**). Hand to human. Do not apply.
- [ ] Finish `LITHIUM-MGR-COURSE.md` (IDs, QueryRefs, scripts, roles
      rule). TOC + MGR.
- [ ] All `course-manager` files ≤ 1000.
- [ ] Live checklist: JWT staff login, list courses, denied role,
      one sync **or** documented disabled tooltip if packet not yet
      applied, inbox open, no leaked Tabulator on close.
- [ ] Record Hydrogen version, Lithium `version.json`, DB design,
      whether 2.23 has landed (informational only).

**Exit gate / validation:**

- [ ] Packet delivered. Working Log lists exact `Group.Name` flips.
- [ ] `npm test && npm run lint && npm run build`
- [ ] Working Log `P18` smoke checklist complete or blockers named.
- [ ] User accepts “first official deployment” for Course Manager.

**Status:** pending

**Lessons learned:**

---

# Band E — Course Builder (Lithium operator)

Pipeline remains COURSEBUILDER.md. Do not start this band until the user
asks; it does not block Course Manager deploy.

## Phase 19 — Course Builder scaffold (ID 35)

**Goal:** Loadable manager with placeholder queue layout, no fake success.

**Dependencies:** Phases 0 (L2), 1, 12 pattern.

**Entry gate:**

- [ ] Phase 0 reserved ID 35. User asked to start Band E **or**
      COURSEBUILDER CB-0/CB-1 exists and Course Manager P18 is complete.

**Reference:**

- [`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)
- Sprint Band F items (absorbed; ID is **35**, not 34)

**Work items:**

- [ ] `src/managers/course-builder/` js/html/css. Register **35**.
- [ ] `cleanup()`. JWT roles gate (same L3 unless Phase 0 variance).
- [ ] Menu seed packet or documented load path.
- [ ] Docs stub `LITHIUM-MGR-COURSEBUILDER.md` pointing at
      COURSEBUILDER.md.
- [ ] Inbox “accept suggestion” may deep-link here later; do not merge
      managers.

**Exit gate / validation:**

- [ ] `npm run build && npm test && npm run lint`
- [ ] Loader imports ID 35 without throw.

**Status:** pending

**Lessons learned:**

---

## Phase 20 — Helium `course_build_*` gate

**Goal:** Lithium can query runs. Do not invent tables in the SPA.

**Dependencies:** COURSEBUILDER CB-1/CB-2.

**Entry gate:**

- [ ] CB-1 transition table exists **or** this phase authors the enum
      into COURSEBUILDER Working Log and this log.
- [ ] Operator agrees to apply Helium migrations.

**Reference:** COURSEBUILDER CB-2 (`course_build_runs`, artifacts, events).

**Work items:**

- [ ] Consume or prepare CB-2 tables + QueryRefs (list by state, get
      run + artifacts, events) + LithiumTable tableDef.
- [ ] Packet to human. `mkt` after they apply.

**Exit gate / validation:**

- [ ] QueryRef numbers in Working Log and
      `LITHIUM-MGR-COURSEBUILDER.md`.
- [ ] Named Hydrogen migration test or SQL fixture noted.

**Status:** pending

**Lessons learned:**

---

## Phase 21 — Queue UI and human-gate invoke

**Goal:** Operator sees runs and calls `Build.Accept` / `Decline` /
`Approve` / `Abandon`.

**Dependencies:** Phases 2, 19, 20. COURSEBUILDER CB-3 scripts.

**Entry gate:**

- [ ] At least one fixture `course_build_runs` row. Named `Build.*`
      scripts exist or this phase seeds minimal handlers (packet).

**Work items:**

- [ ] Table: state, title, updated_at, cost if column exists. Filters
      by state.
- [ ] Detail: suggestion payload + latest event + artifacts.
- [ ] Buttons → `invokeScript` with `run_id`. Refresh. Show errors.
- [ ] Hide Research until Approve (runner-side; UI hides the button).

**Exit gate / validation:**

- [ ] Fixture: Decline → declined + event. Separate fixture: Accept or
      Approve changes state.
- [ ] `npm test` for button→payload mapping.

**Status:** pending

**Lessons learned:**

---

## Phase 22 — Outline, artifacts, Inspect, Publish

**Goal:** Remaining human hexes that belong in Lithium.

**Dependencies:** Phase 21.

**Entry gate:**

- [ ] Phase 21 pattern works. Artifact kind `outline` or fixture JSON
      exists (may be stub).

**Work items:**

- [ ] Render outline JSON sanitized. List materials; version list.
- [ ] Inspect Pass/Fail → locked script names. Publish never auto.
- [ ] “Open LMS” URL from config if `canvas_course_id` present — no
      token.
- [ ] Disable Publish unless state allows.

**Exit gate / validation:**

- [ ] XSS fixture in artifact body does not execute.
- [ ] Buttons emit locked names (unit test).
- [ ] Manual against stub scripts if LMS load is not ready (variance
      logged; do not pretend Canvas published).

**Status:** pending

**Lessons learned:**

---

## Phase 23 — Course Builder tests and docs

**Goal:** Band E closable as a Lithium feature even if COURSEBUILDER
CB-34 E2E is later.

**Dependencies:** Phases 19–22.

**Entry gate:**

- [ ] Phases 19–22 complete or `[~]`.

**Work items:**

- [ ] Unit tests: filters, payloads, state→button visibility.
- [ ] Finish `LITHIUM-MGR-COURSEBUILDER.md`. TOC + MGR + COURSEBUILDER
      CB-9/CB-33 link back.
- [ ] All `course-builder` files ≤ 1000.

**Exit gate / validation:**

- [ ] `npm test && npm run lint && npm run build`
- [ ] TOC links resolve.

**Status:** pending

**Lessons learned:**

---

# Band F — Crimson, Chats, MCP

Hydrogen chat + MCP are done. Lithium talks an older WS shape.

## Phase 24 — Chat JWT honesty (Lithium)

**Goal:** Crimson does not lie when Hydrogen rejects the login JWT.
No product C.

**Dependencies:** Phase 0 L9–L10. Phase 3 XSS if touching markdown.

**Entry gate:**

- [ ] Phase 0 complete. Live Hydrogen chat policy known (Test 59
      vs production).

**Reference:**

- `src/shared/crimson-ws.js`, `src/core/jwt.js`
- [`/docs/H/api/chat/auth_chat.md`](/docs/H/api/chat/auth_chat.md)
- [`/docs/H/core/subsystems/websocket/websocket_chat.md`](/docs/H/core/subsystems/websocket/websocket_chat.md)
- `auth_jwt_helper.c` `validate_chat_jwt_claims` (read only)
- Do **not** start from [`/docs/H/core/CHAT_SYSTEM.md`](/docs/H/core/CHAT_SYSTEM.md)

**Work items:**

- [ ] Map `JWT not authorized for chat` and `chat_error` to a real
      toast. Do not retry blindly.
- [ ] If a Hydrogen mint endpoint already exists when this phase
      starts, use it (`aud=hydrogen-chat`, `roles=chat`). If not, log
      the gap; do not add C.
- [ ] Vitest: token selection / error mapping.
- [ ] Do not send `payload.context` (`gatherContext()` is not in
      `src/`).

**Exit gate / validation:**

- [ ] Working Log: production accepts login JWT or not; mint exists
      or not.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 25 — Align WS client with CHAT_FINALE wire

**Goal:** Rate limits, tokens, and errors are visible. Delimiter
protocol stays Lithium-specific.

**Dependencies:** Phase 24.

**Entry gate:**

- [ ] Phase 24 complete.

**Reference:**

- `crimson-ws.js`, `crimson-chat.js`, `crimson-ui.js`
- WS `error_code` 4291 (requests) / 4292 (tokens)

**Work items:**

- [ ] Parse 4291/4292 → status “rate limited”.
- [ ] Surface `result.tokens` / `response_time_ms` in debug, then
      footer if cheap.
- [ ] Abort remains client-side unless Hydrogen grows a cancel frame
      (do not fake).
- [ ] Keep `[LITHIUM-CRIMSON-JSON]` delimiter. Keep citations UI
      (xAI/RAG shaped).

**Exit gate / validation:**

- [ ] Unit: error_code mapping.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 26 — Engine picker

**Goal:** Dropdown of safe engine names. Never #061.

**Dependencies:** Phase 2 (invoke) or a new QueryRef without keys.
Phase 25.

**Entry gate:**

- [ ] Phase 25 complete. Helium packet for `Chat.ListEngines` (or
      equivalent) ready or already applied.

**Work items:**

- [ ] Packet: name, provider, default, `use_responses_api`, **no
      keys**. `invokable=1` and/or `mcp_access` as Phase 0 recorded.
- [ ] Crimson dropdown; persist last engine in `window.lithiumSettings`.
      Default `"Crimson"` if that CEC row exists.
- [ ] Do not call QueryRef #061 from the browser.

**Exit gate / validation:**

- [ ] Unit: default engine; settings round-trip.
- [ ] Packet handed over. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 27 — Chats manager (history)

**Goal:** ID 18 lists persisted threads. Crimson stays the composer.

**Dependencies:** Phase 24. Re-verify WS path writes #067.

**Entry gate:**

- [ ] Confirm Hydrogen writes conversation storage on the **WS** path
      (REST `auth_chat` does). If WS persist is off, honest empty
      state — do not fake history.

**Reference:**

- QueryRefs **#069** list, **#068** reconstruct, **#067** store
- `src/managers/chats/chats.js` (placeholder)
- LithiumTable pattern from Queries

**Work items:**

- [ ] Replace placeholder. List #069, open #068, new thread stays
      Crimson.
- [ ] `cleanup()`. JWT required.
- [ ] `LITHIUM-MGR.md`: Chats is no longer “Implemented” until this
      phase; then it is.

**Exit gate / validation:**

- [ ] Manual: empty vs one fixture thread.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 28 — MCP operator UI and product Lua tools

**Goal:** Observe MCP. Do not become a daemon. Optional Helium tools
beyond `System.Info`.

**Dependencies:** Phase 24 (token confusion settled). Phase 9 useful
for Scripting `mcp_access` column.

**Entry gate:**

- [ ] Phase 24 complete. User still wants MCP UI (default: yes, small).

**Reference:**

- [`/docs/H/core/subsystems/mcp/mcp.md`](/docs/H/core/subsystems/mcp/mcp.md)
- [`/docs/H/api/mcp/mcp_endpoints.md`](/docs/H/api/mcp/mcp_endpoints.md)
- `GET /api/mcp/status`; QueryRef **#152**
- Helium `acuranzo_1365`–`1376`

**Work items:**

- [ ] Operator panel (utility or Scripting tab): `/api/mcp/status` +
      tool catalog #152. JWT, no special role beyond existing staff
      if Phase 0 says so.
- [ ] Scripting: show `mcp_access` if column is in the tableDef.
- [ ] **Do not** JSON-RPC to `:3100` from the browser.
- [ ] Packet (optional, user ask): `Chat.ListEngines` (if not Phase
      26), `Lithium.AccessibleManagers`. Only then widen Crimson
      `allowed_tools`. Fixtures Echo/Sleep stay test-only.
- [ ] Rewrite `LITHIUM-MGR-CRIMSON.md` / `LITHIUM-WSS.md` to match
      `crimson-ws.js`. Freeze `CHAT_SYSTEM.md` as obsolete in Hydrogen
      docs only if that file is touched with user approval.

**Exit gate / validation:**

- [ ] Grep Lithium `src/`: no fetch to MCP port 3100.
- [ ] `npm test && npm run lint`
- [ ] Crimson unit tests exist for payload builder / delimiter /
      error_code (may live in Phase 24–25; confirm).

**Status:** pending

**Lessons learned:**

---

# Band G — Mail, Jobs, Dashboard, Roles

## Phase 29 — Placeholder collapse

**Goal:** One `PlaceholderManager` instead of ~22 shells. Honest
Implemented list.

**Dependencies:** Phase 0 ID table. Phase 27 must have pulled Chats
**out** of the collapse (or Phase 0 recorded Chats stays placeholder
until 27 — then collapse it now and Phase 27 replaces the wrapper).

**Entry gate:**

- [ ] Phase 0 complete. Band D deployed or user asked to clean shells
      earlier.

**Reference:** AGENTS.md Implemented vs placeholder.

**Work items:**

- [ ] `src/managers/_placeholder/placeholder-manager.js`.
- [ ] Each remaining placeholder folder is a thin default export
      (Vite `switch` paths stable).
- [ ] Collapse unfinished Profile `page-manager-N` stubs or hide nav.
- [ ] `LITHIUM-MGR.md` Implemented = real UIs only.
- [ ] Do not implement Calendar/Contacts/Files/… in this phase.

**Exit gate / validation:**

- [ ] Documented `rg "under development" src/managers | wc -l`
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 30 — Mail Manager

**Goal:** Templates + recent send results + Course Expiration notice.
Not inbound SMTP. Not a second mail stack.

**Dependencies:** Phase 2, 29. Mail Relay REST is shipped.

**Entry gate:**

- [ ] Phase 29 complete. PRIORITIZE §5.6 still wanted (Course Manager
      may deep-link here).

**Reference:**

- [`/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md`](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md)
- `src/api/mailrelay/`; Helium mail QueryRefs 093–128
- `Mail.Notices.CourseExpiration`

**Work items:**

- [ ] LithiumTable templates + recent queue/results (no secrets).
- [ ] Run `Mail.Notices.CourseExpiration` with account/course
      pulldowns (`invokeScript`).
- [ ] Do not spray concurrent same `idempotency_key`. Do not claim
      MySQL Persist if off.
- [ ] Course Manager §5.6 can link to this manager instead of
      duplicating UI.

**Exit gate / validation:**

- [ ] Manual: preview or status against Hydrogen (log).
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 31 — Job Manager

**Goal:** Honest view of `/api/conduit/script` jobs **if** Hydrogen
exposes a list. Otherwise show in-flight poll only and say there is
no durable history.

**Dependencies:** Phases 2, 9, 29.

**Entry gate:**

- [ ] Phase 29 complete. Working Log cites whether a job-list API
      exists (scoreboard is in-memory per `script.md`).

**Work items:**

- [ ] If no list API: Job Manager explains in-memory scoreboard +
      link to Scripting last-run columns. **No fake table.**
- [ ] If list API exists: LithiumTable + poll. No fake cancel.

**Exit gate / validation:**

- [ ] Working Log: which branch. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 32 — Dashboard and Role Manager

**Goal:** Dashboard is a real summary or a collapsed placeholder.
Role Manager reads JWT roles now; writes `account_roles` only when
2.23 exists.

**Dependencies:** Phase 29. AUTH_FINALE Phase 2 optional.

**Entry gate:**

- [ ] Phase 29 complete.

**Work items:**

- [ ] Dashboard: real cards from existing QueryRefs/scripts **or**
      stay placeholder. No success spinners on empty.
- [ ] Role Manager: display account + JWT `roles`. Assign/revoke
      staff/admin **only** if Helium write QueryRef exists (2.23).
      Else read-only + ops note.
- [ ] Do not build a second Course Manager authz UI.

**Exit gate / validation:**

- [ ] `LITHIUM-MGR.md` matches. `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

# Band H — Hydrogen-gated consumers

Do **not** start a phase here until the named Hydrogen plan Status is
complete. Lithium does not implement AUTH_FINALE or Notifications in C.

## Phase 33 — AUTH_FINALE SPA consumers

**Goal:** Lithium uses MFA / reset / session-revoke **only after**
Hydrogen ships them.

**Dependencies:**

| Lithium work | Wait for |
|--------------|----------|
| Login MFA OTP field | AUTH_FINALE Phase 8 complete |
| Password reset UI | AUTH_FINALE Phase 8b complete |
| Session list/revoke | AUTH_FINALE Phase 10b complete |
| Durable roles cutover | AUTH_FINALE Phase 2 complete — then update L3 |

**Entry gate:**

- [ ] At least one of the Hydrogen phases above is Status complete.
      Implement only those slices. Skip others with `[~]`.

**Reference:** [`/docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)

**Work items:**

- [ ] MFA: consume Mail Relay OTP via the **auth** API Hydrogen
      documents. SPA still holds Hydrogen JWT only.
- [ ] Reset: no email-enumeration in copy.
- [ ] Sessions: list + revoke own JWTs; reuse existing logout.
- [ ] If Phase 2 landed: Course Manager may additionally check
      `account_roles` without dropping JWT `roles` until cutover is
      logged.

**Exit gate / validation:**

- [ ] Only shipped Hydrogen endpoints used. `npm test && npm run lint`
- [ ] AUTH_FINALE Phase 11 still does not block CATCHUP.

**Status:** pending

**Lessons learned:**

---

## Phase 34 — Notifications Manager

**Goal:** Web Push subscribe/unsubscribe in Lithium after Hydrogen
Notifications exist.

**Dependencies:** [`/docs/H/plans/NOTIFICATIONS_PLAN.md`](/docs/H/plans/NOTIFICATIONS_PLAN.md)
Phase 0 approved **and** a Lithium-facing subscribe API.

**Entry gate:**

- [ ] NOTIFICATIONS_PLAN has a complete phase that documents the SPA
      contract. If still “proposed, not approved”, this CATCHUP phase
      stays pending — do not invent VAPID handling.

**Work items:**

- [ ] Notification Manager (ID 19): permission prompt, subscribe,
      list, unsubscribe. Service worker already exists — do not
      casually rewrite `index.html` splash.
- [ ] Never log VAPID private keys or subscription `auth`.
- [ ] Not a mail stack.

**Exit gate / validation:**

- [ ] Named Hydrogen test + `npm test && npm run lint` or Status
      `[~]` waiting.

**Status:** pending

**Lessons learned:**

---

# Band I — Tables, docs, tests, closeout

## Phase 35 — File-size splits (remaining >1000)

**Goal:** No `src/` JS/CSS/HTML over 1000 lines.

**Dependencies:** Phase 3 if splitting Lookups/Tour (sanitize first).
Phase 11 owns `codemirror-setup.js`. Phase 8 owns `scripting.js`.

**Entry gate:**

- [ ] `find src -name '*.js' -o -name '*.css' -o -name '*.html' | xargs wc -l`
      inventory in Working Log.

**Work items:**

- [ ] Split remaining offenders (2026-08-21 baseline 14 files; re-count).
      Tour, Lookups, Style, crimson.css, tour.css, lithium-table.css,
      queries.js, version-history, profile-manager, page-photo,
      main.css, components.css — whatever is still over.
- [ ] No behavior change. Keep public class default export.

**Exit gate / validation:**

- [ ] `wc -l` inventory: zero files over 1000.
- [ ] `npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 36 — Settings, log, tree hygiene

**Goal:** Prefs go through `lithiumSettings`. Logs do not silently drop.
Generated trees stop lying.

**Dependencies:** Phase 0. Touched managers from earlier bands.

**Entry gate:**

- [ ] Phase 0 complete.

**Reference:** Sprint Phases 10–11 (absorbed). `src/core/log.js`
`_flushToServer()`.

**Work items:**

- [ ] Route remaining user prefs through `window.lithiumSettings`.
- [ ] Replace live `console.*` on paths this plan touched with `log()`.
- [ ] `_flushToServer()`: wire to a real endpoint **or** remove the
      queue. Do not silently drop.
- [ ] `templates:copy` HTML only (or document CSS). Remove orphan
      `public/src/managers/user-profiles/`, `session-logs/`,
      `style-manager-v2.html` if unused. Wire copy into `build`.
- [ ] Punchcard: implement L14. Delete always-`[7..33]` fiction.

**Exit gate / validation:**

- [ ] Working Log: remaining `localStorage` pref writes (JWT/cache
      only) and log.js decision.
- [ ] `package.json` `build` runs `templates:copy` (or equivalent).
- [ ] `npm run build && npm test && npm run lint`

**Status:** pending

**Lessons learned:**

---

## Phase 37 — Docs, LithiumTable, test inventory

**Goal:** Docs describe the code. LithiumTable remaining honesty from
[`/docs/Li/LITHIUM-TAB-PLAN.md`](/docs/Li/LITHIUM-TAB-PLAN.md) is either
done or parked in `TODO.md` with effort tags — no second active table
plan.

**Dependencies:** Phases 12, 29 (Implemented list). Band D docs stubs.

**Entry gate:**

- [ ] Course Manager docs exist. Placeholder Implemented list honest.

**Work items:**

- [ ] Rewrite `LITHIUM-API.md`: Conduit query + **script**, auth, OIDC,
      health, WS, mailrelay REST used. Mark historical `/api/lookups`
      REST if unused.
- [ ] Fix `LITHIUM-MGR-NEW.md`: LithiumTable path, no static sidebar
      register, IDs 34/35.
- [ ] Fix `LITHIUM-INS.md` dead `REFACTORING_PLAN.md` link → this plan
      Phase 35.
- [ ] `LITHIUM-TST.md` counts from `npm run test:coverage`.
- [ ] Fix `event-bus.test.js` to `off` the same function reference.
- [ ] Delete tautological Queries tests or rewrite to hit production
      functions.
- [ ] TAB-PLAN: close leftover gates or move leftovers to `TODO.md`
      and banner TAB-PLAN as historical.

**Exit gate / validation:**

- [ ] TOC links resolve (`mkl` / Test 90 if Lithium markdown is in
      that suite; otherwise open the files).
- [ ] `LITHIUM-API.md` mentions `/api/conduit/script`.
- [ ] `npm test`

**Status:** pending

**Lessons learned:**

---

## Phase 38 — Coverage, live smoke, closeout

**Goal:** CATCHUP can move to `docs/Li/plans/complete/` when the user
agrees.

**Dependencies:** Phases 0–37 complete or `[~]` with rationale.

**Entry gate:**

- [ ] Pause point is this phase.

**Work items:**

- [ ] `npm run test:coverage` + TST table (core: jwt, utils, conduit
      including script).
- [ ] `npm run lint && npm run lint:css` zero errors.
- [ ] `npm audit` summary; fix or defer each critical/high with
      reason. No `audit fix --force`.
- [ ] Live smoke: password or OIDC login, Scripting Echo, Course
      Manager list, close without leaked DOM. Record versions.
- [ ] Sweep INS leftovers into `docs/Li/TODO.md`.
- [ ] Update AGENTS.md Last reviewed + Known defects (strike fixed).
- [ ] User accepts move to
      `docs/Li/plans/complete/CATCHUP_COMPLETE.md` (copy or pointer;
      canonical file may remain
      [`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)).

**Exit gate / validation:**

- [ ] AGENTS.md Known defects only lists remaining items.
- [ ] This plan’s Status blocks are complete or `[~]`.
- [ ] User accepts closeout.

**Status:** pending

**Lessons learned:**

---

# Open decisions

Defaults are L1–L18. Only reopen with a Phase 0 variance.

| # | Question | Default |
|---|---------|---------|
| 1 | JWT roles match rule | Confirm on a live token in Phase 0 |
| 2 | Partner ids missing on Hydrogen | Hide buttons |
| 3 | Chat mint C | Out of CATCHUP |
| 4 | REST `auth_chat` / `auth_chats` UI | Parked unless user asks after Phase 25 |
| 5 | `media_chunk` / store toggle | Parked |
| 6 | Didit = OIDC or other | OIDC provider id `didit` or hidden |

---

# Working Log

### P-plan-20260909 — CATCHUP authored

- What we did: Researched Lithium vs Hydrogen (login, chat/MCP,
  Scripting/CM6, 500 Courses Part 5). Wrote this 0–38 plan. Course
  Manager is ID 34 and the first deploy; Course Builder is ID 35.
  JWT `roles` gates v1 (not 2.23). Sprint superseded.
- Gate result: plan-only; Phase 0 next.
- Follow-ups: Phase 0 (ID table in `LITHIUM-MGR.md`, QueryRefs, live
  JWT roles shape, partner providers).

---

# Revision history

| Date | Change |
|------|--------|
| 2026-09-09 | Initial CATCHUP.md (Phases 0–38). Supersedes LITHIUM_SPRINT.md. |
