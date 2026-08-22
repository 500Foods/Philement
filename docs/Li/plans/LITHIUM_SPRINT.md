<!-- markdownlint-disable MD007 MD024 -->
# Lithium Sprint Plan

## Purpose

A gated, phase-by-phase plan for a lengthy Lithium sprint: close the
2026-08-22 architecture-review defects, make the manager/lifecycle/ID layer
honest, flesh out **Scripting Manager** so operators can manage and invoke
the growing Helium Lua catalog, and add a **Course Builder** operator
manager that drives the pipeline in
[`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md)
through `POST /api/conduit/script` — not new Hydrogen C.

Agent map (read first every session):
[`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).

Coding rules:
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md).

---

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**, **Work items**,
  **Exit gate / validation**, **Status**, **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and the target phase.
- After each phase: fill **Status**, append reusable discoveries to
  **Working Log**, then stop for review before the next phase.
- Never mark a phase complete with a failing or skipped gate unless the
  Status block records an explicit variance.

### Testing policy

| Layer | When | What |
|-------|------|------|
| **Vitest unit** | Every Lithium JS phase | Real functions, not tautological mocks |
| **Lint** | Every Lithium JS/CSS phase | `npm run lint` and `npm run lint:css` from `elements/003-lithium` |
| **Build** | Phases that touch Vite entry, templates, or new managers | `npm run build` |
| **Hydrogen / Helium** | Phases that add QueryRefs, `scripts` rows, or `course_build_*` | `zsh -ic 'mkt'` (or `mka`) then the named Hydrogen test |
| **Manual / live** | Auth, invoke, Course Builder buttons | Named checklist against Hydrogen + JWT; do not fake |

Lithium commands run in `elements/003-lithium`. Hydrogen aliases are C/Lua
only. See AGENTS.md.

---

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-22):** Plan authored. **Next: Phase 0**
(design lock). No implementation yet.

### Resume here next session

1. Confirm latest completed phase via Status blocks; active phase = first
   not complete.
2. Re-read Working Log decisions that affect the next chunk.
3. Re-read [`AGENTS.md`](/elements/003-lithium/AGENTS.md) Known defects +
   ID map.
4. Baseline: `cd elements/003-lithium && npm test && npm run lint`.
5. One phase: implement → verify exit gate → update this doc → stop.

---

## Goals

1. **Safe close / activate** — every manager tears down editors and tables.
2. **No XSS on HTML sinks** — DOMPurify or shared `escapeHtml` everywhere
   `innerHTML` meets untrusted or markdown content.
3. **Honest UI** — no success toast on no-ops; no stub `save()` that claims
   persistence.
4. **One ID scheme** — docs, registry, config, tours agree.
5. **File-size cap** — no `src/` JS/CSS/HTML over 1000 lines when the
   split phases finish.
6. **Scripting Manager** — list, edit, save, **invoke**, poll, invokable
   flag, tests, docs.
7. **Course Builder manager (ID 34)** — operator queue and human gates
   calling `Build.*` scripts.
8. **Docs match runtime** — API, TST, MGR, TOC, NEW-manager guide.

### Non-goals (this plan)

- Re-implementing COURSEBUILDER.md CB-0..35 inside this file. Helium tables
  and Lua pipeline stay that document’s job; this plan only **gates** on
  them when the Lithium UI needs them.
- Reception public SPA work.
- New Hydrogen C endpoints or Canvas-named routes.
- Filling in all ~22 placeholder managers (Dashboard, Mail, …). Collapse
  them; do not implement each.
- Hydrogen-as-IdP, Keycloak realm admin, or KEYCLOAK_PLAN Phase 5 MFA
  unblocking (ops).
- Committing `dist-deploy-500courses/` as a product.
- **Accessibility (a11y) audit/remediation.** ARIA usage is currently
  sparse and inconsistent (some on toast/login, none on most placeholders)
  with no lint rule or audit tooling. Explicitly deferred, not silently
  dropped — track as a follow-up in `docs/Li/TODO.md` after this sprint.
- **i18n/l10n.** The SPA is hardcoded English-only
  (`app.default_language` is display metadata only, no translation
  mechanism). Not in scope; do not add a partial i18n layer as a side
  effect of touched files.
- **CSP / security headers.** Left entirely to Hydrogen / the reverse
  proxy in front of `lithium.philement.com`. Lithium adds no
  `<meta http-equiv="Content-Security-Policy">` or equivalent this sprint.
- **Bundle/perf budget beyond the existing `chunkSizeWarningLimit`.** No
  Lighthouse CI or bundle-visualizer this sprint; Phase 29 only spot-checks
  `dist/assets/` chunk sizes against the current 1500 KB warning limit,
  it does not introduce a new enforced budget.

---

## Current Observed State (2026-08-22)

### Present and reusable

- Lazy manager `switch` + Vite chunking (`manager-loader.js`, `vite.config.js`)
- Conduit query helpers (`src/shared/conduit.js`)
- JWT + OIDC client (`jwt.js`, `oidc-client.js`)
- LithiumTable + Queries / Lookups / Scripting as patterns
- Scripting Manager list/editor/save (QueryRefs 89/90/87/129/130)
- Hydrogen `POST /api/conduit/script` (LUA_CLIENT complete)
- Dozens of Helium `scripts` seeds (`Api.Echo`, Enroll.*, Account.*, Stripe.*, Catalog.*, …)
- Session log, settings service, popup coordination (rules exist; compliance does not)

### Missing / broken

- `closeManager` ignores `cleanup()` / `teardown()`
- No `invokeScript` helper; Scripting cannot run Lua
- Course Builder manager does not exist; `course_build_*` tables not in Helium yet
- XSS no-op escapes; unused DOMPurify
- Style apply/copy and several Profile saves are lies
- ID / docs / placeholder drift (see AGENTS.md)

---

## Phase Groups

| Group | Phases | Theme |
|-------|--------|--------|
| A | 0–2 | Design lock, lifecycle, shared sanitize + invoke |
| B | 3–6 | XSS, native dialogs, honest saves, SunEditor JWT |
| C | 7–11 | File splits, placeholders, settings, punchcard, tree hygiene |
| D | 12–13 | Docs + test inventory |
| E | 14–19 | Scripting Manager |
| F | 20–26 | Course Builder operator manager |
| G | 27–31 | OIDC/integration, coverage, E2E, closeout |

---

# Band A — Foundation

## Phase 0 — Design lock

**Goal:** Freeze IDs, lifecycle names, Course Builder ID, and the
Lithium-vs-COURSEBUILDER split so later phases do not invent a second
scheme.

**Dependencies:** None.

**Entry gate:**

- [ ] This plan and `AGENTS.md` exist and have been read.

**Work items:**

- [ ] Write the canonical manager ID table into `LITHIUM-MGR.md` (copy from
      AGENTS.md; resolve Terminal=5 vs Crimson=5 — recommendation: keep
      Crimson as system 5 / popout, Terminal menu 32, utility key
      `terminal` **without** numeric 5).
- [ ] Lock lifecycle: `closeManager` calls `destroy` else `cleanup` else
      `teardown`; `showManager`/`hide` call `onActivate`/`onDeactivate`.
- [ ] Reserve menu ID **34** Course Builder in the written table (no code
      required this phase).
- [ ] Record Scripting QueryRefs 87/89/90/129/130 after confirming against
      Helium (fix the table if the live refs differ).
- [ ] Record: no product C; Course Builder buttons = `Build.*` via
      `/api/conduit/script`; pipeline phases stay in COURSEBUILDER.md.
- [ ] Verify `npm run dev` wording: **2026-08-21 re-check found
      `LITHIUM-TOC.md` §"Development server needs Hydrogen" (L21-26)
      already states Vite + HMR work and Hydrogen is only required for
      login/data** — it does not say "cannot run." Confirm `LITHIUM-DEV.md`
      says the same and close this item without a doc edit unless a
      different, older copy of the wording is found elsewhere.

**Exit gate / validation:**

- [ ] `LITHIUM-MGR.md` ID table matches `manager-loader.js` 7–33 plus
      reserved 34.
- [ ] `LITHIUM-TOC.md` dev-server paragraph matches `LITHIUM-DEV.md`
      (both already read this way as of 2026-08-21 — gate is "confirm and
      log," not "rewrite").
- [ ] Working Log `P0` lists the Terminal/Crimson ID decision and confirmed
      Scripting QueryRefs.

**Status:** pending

**Lessons learned:**

---

## Phase 1 — Manager close / activate lifecycle

**Goal:** Closing a manager actually destroys Tabulator, CodeMirror,
splitters, and listeners.

**Dependencies:** Phase 0 lifecycle lock.

**Entry gate:**

- [ ] Phase 0 Status complete.

**Work items:**

- [ ] `ManagerLoader.closeManager` tries `destroy`, then `cleanup`, then
      `teardown` (await, log failures).
- [ ] `showManager` / hide path call `onActivate` / `onDeactivate` when
      present.
- [ ] Delete dead `_importManager` cases 1–6 or map them only if Phase 0
      assigned those IDs (it should not).
- [ ] Unit test: a fake manager with only `cleanup()` is invoked on close;
      one with only `destroy()` still works.

**Exit gate / validation:**

- [ ] New/updated unit test in `tests/unit/` passes (`npm test`).
- [ ] `npm run lint`.
- [ ] Grep: `closeManager` no longer calls `destroy` exclusively.

**Status:** pending

**Lessons learned:**

---

## Phase 2 — Shared `escapeHtml` / DOMPurify / `invokeScript`

**Goal:** One sanitize path and one script-invoke client used by later
phases.

**Dependencies:** Phase 0.

**Entry gate:**

- [ ] Phase 0 complete. Phase 1 may proceed in parallel only if this phase
      does not touch `manager-loader.js`.

**Work items:**

- [ ] Confirm `escapeHtml` in `src/core/utils.js` (or add it) and unit
      tests for `& < > " '`.
- [ ] Add `sanitizeHtml(html)` wrapping DOMPurify (import the existing
      dependency). Use it for markdown → HTML.
- [ ] Add `invokeScript(api, { script, params, wait, timeoutSeconds })`
      and `getScriptJob(api, jobId)` to `src/shared/conduit.js` targeting
      `/api/conduit/script`. Radar blips + `error.serverError` parity with
      `authQuery`.
- [ ] Unit tests for payload shape (`script` required, reject slash names,
      never send `_hydrogen`).

**Exit gate / validation:**

- [ ] `npm test` includes new utils + conduit tests.
- [ ] `npm run lint`.
- [ ] No other manager switched to the helper yet (callers come later).

**Status:** pending

**Lessons learned:**

---

# Band B — Safety

## Phase 3 — XSS sinks

**Goal:** Untrusted and markdown content cannot execute script.

**Dependencies:** Phase 2.

**Entry gate:**

- [ ] Phase 2 `escapeHtml` / `sanitizeHtml` shipped.

**Work items:**

- [ ] Fix `lookups.js` `processContentWithHighlighting` /
      `basicContentProcessing` no-op replaces; use `escapeHtml` then
      highlight; assign via sanitized HTML.
- [ ] Crimson + Version History: `DOMPurify.sanitize(marked.parse(...))`.
- [ ] `highlight-init.js`: do not interpolate raw `code` / `language` into
      `innerHTML`.
- [ ] Tour captions: `textContent` + icon nodes, or sanitize allowlist.
- [ ] Grouping / queries popup labels: `textContent` for titles.
- [ ] Unit tests with `<img onerror=…>` / `<script>` fixtures for the
      lookup processors if extracted.

**Exit gate / validation:**

- [ ] Grep `lookups.js` shows real entity escapes or `escapeHtml` calls.
- [ ] `npm test && npm run lint`.
- [ ] Manual: a lookup summary containing `<img onerror=alert(1)>` does
      not fire (checklist in Working Log).

**Status:** pending

**Lessons learned:**

---

## Phase 4 — Native dialogs and primary-feature fallbacks

**Goal:** INS §1 and §4 compliance on the hottest paths.

**Dependencies:** Phase 1 (cleanup) recommended.

**Entry gate:**

- [ ] Existing toast / popup confirm pattern identified (cite file in log).

**Work items:**

- [ ] Replace `window.confirm` / `prompt` / `alert` in
      `lithium-table-ops.js`, `lithium-table-ui.js`,
      `queries-navigation.js`, `queries-templates.js`,
      `template-popup.js`, `jsoneditor-init.js`, `camera-popout.js`.
- [ ] Remove Login / Main / Session Log / Profile `renderFallback` HTML
      clones; fail with toast + session log if the template fetch fails.
- [ ] Remove Lookups JSON textarea fallback DOM/reader.

**Exit gate / validation:**

- [ ] `rg "\\balert\\(|\\bconfirm\\(|\\bprompt\\(" src/` is empty (or
      Working Log lists remaining with target phase).
- [ ] `rg renderFallback src/` is empty.
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 5 — Honest Style Manager and Profile saves

**Goal:** Buttons either work or refuse; they never toast success for a
no-op.

**Dependencies:** None (after Phase 0).

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] Style Manager: implement `applyState` (inject CSS custom properties
      / stylesheet the Style Manager already owns) **or** disable Apply /
      Undo / Redo / Copy and stop success toasts. Prefer implement if the
      existing editor already has CSS text.
- [ ] `copyCssToClipboard` copies real CSS, not a comment stub.
- [ ] Profile pages (`page-names`, `page-email`, `page-addresses`,
      `page-phone`, `page-number-formats`, others): `save()` returns
      failure or the control is disabled until an API exists. No
      `{ success: true }` on stubs.
- [ ] `scripting.js` / others: `USERID` from `this.app.auth.user`, not
      `this.app.user`.

**Exit gate / validation:**

- [ ] Grep Profile `save()` for `success: true` on stub bodies is empty.
- [ ] Manual: Style Apply either changes computed styles or the button is
      disabled (log which).
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 6 — SunEditor JWT header

**Goal:** Image/video upload Authorization is a real Bearer token.

**Dependencies:** None.

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] `suneditor-init.js`: use `retrieveJWT()` (or equivalent from
      `jwt.js`). Fix `+` vs `||` precedence.
- [ ] Never read `lithium_jwt` with a broken concat.

**Exit gate / validation:**

- [ ] Unit or focused test: missing token does not produce
      `"Bearer null"`.
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

# Band C — Architecture hygiene

## Phase 7 — Split files over 1000 lines (wave 1)

**Goal:** `tour.js` and `lookups.js` under 1000 lines each.

**Dependencies:** Phase 3 if those files still contain the XSS fixes
(split after sanitize so you do not move unsanitized code).

**Entry gate:**

- [ ] Phase 3 complete **or** wave-1 files already sanitized.

**Work items:**

- [ ] Split `src/managers/tour/tour.js` (2834) into modules (video, drag,
      shepherd adapter, caption render). CSS split if still >1000.
- [ ] Split `src/managers/lookups/lookups.js` (1888) (table, preview,
      editors, splitters).
- [ ] No behavior change. Keep public class default export.

**Verified baseline (2026-08-21 re-check, `find src -name '*.js' -o -name
'*.css' -o -name '*.html' | xargs wc -l`) — exactly 14 files over 1000
lines, confirming AGENTS.md's count. Full list (Phase 7 wave 1 + Phase 8
wave 2 must together account for all 14; do not close Phase 8 until every
row below is ≤ 1000):**

| File | Lines | Wave |
|------|------:|------|
| `src/managers/tour/tour.js` | 2834 | 7 |
| `src/managers/lookups/lookups.js` | 1888 | 7 |
| `src/managers/style-manager/style-manager.js` | 1678 | 8 |
| `src/managers/crimson/crimson.css` | 1450 | 8 |
| `src/managers/tour/tour.css` | 1357 | 8 |
| `src/tables/lithium-table.css` | 1344 | 8 |
| `src/managers/queries/queries.js` | 1192 | 8 |
| `src/managers/version-history/version-history.js` | 1173 | 8 |
| `src/managers/profile-manager/profile-manager.js` | 1145 | 8 |
| `src/managers/profile-manager/pages/photo/page-photo.js` | 1141 | 8 |
| `src/managers/main/main.css` | 1111 | 8 |
| `src/styles/components.css` | 1033 | 8 |
| `src/core/codemirror-setup.js` | 1019 | 8 |
| `src/managers/scripting/scripting.js` | 1004 | 8 (or 19 if Band E lands first) |

**Exit gate / validation:**

- [ ] `wc -l` on every new/remaining tour + lookups src file ≤ 1000.
- [ ] `npm test && npm run lint && npm run build`.

**Status:** pending

**Lessons learned:**

---

## Phase 8 — Split files over 1000 lines (wave 2)

**Goal:** Remaining cap breaches under 1000.

**Dependencies:** Phase 7 pattern.

**Entry gate:**

- [ ] Phase 7 complete.

**Work items:**

- [ ] Split as needed: `style-manager.js`, `queries.js`,
      `version-history.js`, `profile-manager.js`, `scripting.js` (if still
      over after Band E, defer remainder to Phase 19),
      `page-photo.js`, `tour.css`, `crimson.css`, `lithium-table.css`,
      `main.css`, `components.css`, `codemirror-setup.js`.
- [ ] Prefer extracting already-logical regions; do not rewrite.

**Exit gate / validation:**

- [ ] `find src -name '*.js' -o -name '*.css' -o -name '*.html' | xargs wc -l`
      shows **no** file > 1000 (Working Log pastes the offenders-or-none;
      cross-check against the Phase 7 baseline table — every row there
      must be gone or explicitly re-measured if the split changed a name).
- [ ] `npm test && npm run lint && npm run build`.

**Status:** pending

**Lessons learned:**

---

## Phase 9 — Placeholder manager collapse

**Goal:** One `PlaceholderManager` instead of ~22 copy-paste shells.

**Dependencies:** Phase 0 ID table.

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] Add `src/managers/_placeholder/placeholder-manager.js` (title, icon,
      “under development”).
- [ ] Each placeholder folder becomes a 5–15 line default export wrapping
      that class (keeps Vite `switch` paths stable).
- [ ] Collapse 28 profile `page-manager-N` stubs to one template +
      registry, or hide unfinished nav items.
- [ ] Update `LITHIUM-MGR.md` so “Implemented” is only true for real UIs.
- [ ] **Chats manager decision record.** `chats.js` is a plain 49-line
      placeholder, but a real app-wide WebSocket (`app-ws.js`) and a real
      consumer (Crimson) already exist and are documented in
      `LITHIUM-WSS.md` — that doc has **no chat-manager UI section**.
      Before folding `chats.js` into the generic `PlaceholderManager`,
      record an explicit decision in the Working Log: either (a) Chats
      stays a placeholder this sprint and `LITHIUM-WSS.md` gets a
      one-line note that no manager UI consumes the WS chat channel yet,
      or (b) Chats is pulled out of the collapse and scoped as its own
      follow-up phase. Do not let it become a real feature "by accident"
      of the collapse, and do not let it silently disappear either.

**Exit gate / validation:**

- [ ] Placeholder JS files no longer contain duplicated innerHTML shells
      (`rg "under development" src/managers | wc -l` documented).
- [ ] `npm test && npm run lint`.
- [ ] `LITHIUM-MGR.md` Implemented list matches reality.
- [ ] Working Log records the Chats/WebSocket decision (a) or (b) above.

**Status:** pending

**Lessons learned:**

---

## Phase 10 — Settings service + session log compliance (hot paths)

**Goal:** New and touched code uses `lithiumSettings` and `log()`.

**Dependencies:** None.

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] Route Scripting / Queries / Tour / languages / sidebar prefs through
      `window.lithiumSettings` where they are user prefs.
- [ ] Replace live `console.error`/`warn` in managers touched this sprint
      with `log(Subsystems.*, Status.ERROR|WARN, …)`.
- [ ] Delete the commented `console.log` pile in `tour.js` when splitting
      (Phase 7) or here if still present.
- [ ] Empty `catch` blocks in `lithium-app.js` / queries: at least
      `Status.WARN`.
- [ ] **Fix `log.js` `_flushToServer()` dead stub.** It queues logs into
      `_syncQueue` but the flush function's comment says "Server logging
      endpoint is not configured" and just clears the queue without
      sending. This is the same "for now" pattern this phase is otherwise
      cleaning up. Either wire it to a real endpoint if one already exists
      on Hydrogen, or remove the queue/flush machinery entirely and keep
      `log()` local-only — do not leave a client that silently drops data
      it appears to be collecting for upload.

**Exit gate / validation:**

- [ ] Working Log lists remaining `localStorage` pref writes (if any) with
      a reason (JWT / cache TTL only).
- [ ] Working Log states which `log.js` fix was taken (wired vs removed)
      and confirms no code path still assumes logs reach a server.
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 11 — Tree hygiene (public/src, gitignore, punchcard)

**Goal:** Generated trees and authz stubs stop lying.

**Dependencies:** Phase 0.

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] Add `dist-deploy-500courses/` (and `.tmp-*` if desired) to
      `.gitignore`. Do not delete the user’s deploy dir without asking;
      just stop tracking if currently tracked (`git check-ignore` / status).
- [ ] `templates:copy` copies **HTML only** (or document why CSS must
      exist under `public/src`). Remove orphan
      `public/src/managers/user-profiles/`, `session-logs/`,
      `style-manager-v2.html` if unused.
- [ ] Wire `npm run templates:copy` into `build` / `deploy` so FAQ memory
      is not required.
- [ ] Pass JWT punchcard / roles into `getPermittedManagers` at Main load
      **or** document that menu data is the only gate and delete the
      always-`[7..33]` fiction.

**Exit gate / validation:**

- [ ] `git check-ignore -v dist-deploy-500courses` succeeds.
- [ ] `package.json` `build` runs `templates:copy` (or equivalent).
- [ ] Working Log states the punchcard decision.
- [ ] `npm run build`.

**Status:** pending

**Lessons learned:**

---

# Band D — Docs and tests

## Phase 12 — Documentation refresh

**Goal:** Docs describe the code that exists.

**Dependencies:** Phases 0–1, 9 (ID + Implemented list).

**Entry gate:**

- [ ] Phase 0 and Phase 9 complete (so “Implemented” is true).

**Work items:**

- [ ] Rewrite `LITHIUM-API.md`: Conduit query + **script**, auth, OIDC,
      health, WS. Remove or mark historical `/api/lookups` and
      `/api/styles` REST if unused.
- [ ] Fix `LITHIUM-MGR-NEW.md`: LithiumTable path, no static sidebar
      register, import from `manager-loader.js`, ID table.
- [ ] Fix `LITHIUM-INS.md` dead `REFACTORING_PLAN.md` link (point at this
      plan’s split phases or add a short split note).
- [ ] Add `LITHIUM-MGR-SCRIPT.md` stub heading now; fill in Phase 19.
- [ ] Point TOC / README at `AGENTS.md` and this plan (if not already).

**Exit gate / validation:**

- [ ] TOC links resolve (`rg` / open the files).
- [ ] `LITHIUM-API.md` mentions `/api/conduit/script`.
- [ ] No remaining “register in `app.js` `managerRegistry`” instruction.

**Status:** pending

**Lessons learned:**

---

## Phase 13 — Test inventory and event-bus teardown

**Goal:** Honest TST numbers; no singleton listener leaks in tests.

**Dependencies:** None.

**Entry gate:**

- [ ] Phase 0 complete.

**Work items:**

- [ ] Run `npm run test:coverage` and replace `LITHIUM-TST.md` counts and
      file list.
- [ ] Fix `event-bus.test.js` to `off` the same function reference.
- [ ] Fix integration `itIfAvailable` to `it.skip` when Hydrogen is down.
- [ ] Delete tautological Queries tests that only assert a mock was
      called, or rewrite them to hit production functions.

**Exit gate / validation:**

- [ ] `LITHIUM-TST.md` file list matches `tests/unit`.
- [ ] With Hydrogen down, `npx vitest run tests/integration` shows skipped
      tests, not a silent pass (log the output).
- [ ] `npm test`.

**Status:** pending

**Lessons learned:**

---

# Band E — Scripting Manager

## Phase 14 — Scripts schema and QueryRef audit

**Goal:** Lithium Scripting matches the live `scripts` table, including
`invokable`.

**Dependencies:** Phase 0 QueryRef confirm; Helium Acuranzo.

**Entry gate:**

- [ ] Phase 0 Working Log has QueryRefs. Hydrogen + migrated DB available
      **or** Helium migration files read end-to-end if DB is down (log
      which).

**Work items:**

- [ ] Inventory `scripts` columns (group, name, type, schedule, status,
      code, summary, **invokable**, run timestamps, audit).
- [ ] Confirm tableDef `scripts/script-manager` exposes `invokable`.
      Add/adjust Helium tableDef + QueryRefs if missing (then `mkt`).
- [ ] List current seeded scripts (group.prefix counts) in Working Log.
- [ ] Document insert/update/delete QueryRefs. Add delete ref if missing.

**Exit gate / validation:**

- [ ] Working Log table: column → QueryRef / tableDef field.
- [ ] If Helium changed: `zsh -ic 'mkt'` and the relevant Hydrogen
      migration test named in the log.

**Status:** pending

**Lessons learned:**

---

## Phase 15 — Scripting CRUD completeness

**Goal:** Create / update / duplicate / delete are real and fail honestly.

**Dependencies:** Phases 1, 5 (`USERID`), 14.

**Entry gate:**

- [ ] Phase 14 schema table exists.

**Work items:**

- [ ] Save/insert/update use confirmed QueryRefs and `app.auth.user`.
- [ ] Delete with shared confirm (Phase 4 modal) + QueryRef.
- [ ] Dirty tracking: switching rows with unsaved Lua prompts.
- [ ] Composite PK `group_name` + `script_name` stable after rename
      policy: either forbid rename or implement as delete+insert
      (document).
- [ ] `cleanup()` destroys table, editors, splitter, keydown listener.

**Exit gate / validation:**

- [ ] Unit tests for save payload builder (extracted pure function).
- [ ] Manual or integration: insert fixture → reload list → delete
      (Working Log).
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 16 — Invoke panel (sync wait)

**Goal:** Operator can run `Api.Echo` (and any `invokable=1` script) from
the manager.

**Dependencies:** Phases 2, 14, 15.

**Entry gate:**

- [ ] `invokeScript` exists. `Api.Echo` (or named fixture) is `invokable=1`
      in the target DB.

**Work items:**

- [ ] UI: params JSON editor (CodeMirror JSON), Wait checkbox, timeout,
      Run button enabled only if `invokable`.
- [ ] Call `invokeScript`; render status + `result` JSON / error code.
- [ ] Non-invokable scripts: Run disabled + tooltip (Hydrogen would 403).
- [ ] Do not send `params._hydrogen`.
- [ ] Stay under 1000 lines (split `scripting-invoke.js` if needed).

**Exit gate / validation:**

- [ ] Manual: JWT login → Scripting → `Api.Echo` → result body in pane
      (log job_id + outcome).
- [ ] Unit tests for invoke payload (wait default true, timeout clamp
      client-side if you add one).
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 17 — Async job poll + scoreboard

**Goal:** `wait: false` returns a job id; UI polls `GET …/script/{job_id}`.

**Dependencies:** Phase 16.

**Entry gate:**

- [ ] Phase 16 sync invoke works against Hydrogen.

**Work items:**

- [ ] Poll with backoff until COMPLETED/FAILED/KILLED or timeout.
- [ ] Cancel UX: document if Hydrogen has no cancel; do not fake one.
- [ ] Show last run timestamps from the table after refresh.

**Exit gate / validation:**

- [ ] Manual: invoke with wait off → spinner → terminal status (log).
- [ ] Unit test poll state machine (fake `getScriptJob`).
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 18 — Scripting UX polish

**Goal:** Editor/preview/docs good enough for dozens of Lua files.

**Dependencies:** Phase 15.

**Entry gate:**

- [ ] Phase 15 complete.

**Work items:**

- [ ] Group filter / search already via table — verify against  dozens of
      rows; add group grouping if tableDef allows.
- [ ] Preview tab: sanitized markdown/HTML summary (Phase 3 helper).
- [ ] Optional: read-only “meta” tab (invokable, schedule, last run).
- [ ] Keyboard: reuse manager shortcut patterns (`LITHIUM-KEY.md`).
- [ ] Font prefs via `lithiumSettings` (Phase 10).

**Exit gate / validation:**

- [ ] `npm test && npm run lint`.
- [ ] Manual: open 3 scripts in different groups; preview does not XSS.

**Status:** pending

**Lessons learned:**

---

## Phase 19 — Scripting tests and docs

**Goal:** Band E closable.

**Dependencies:** Phases 16–18.

**Entry gate:**

- [ ] Phases 16–18 complete (17 if async shipped).

**Work items:**

- [ ] `tests/unit/managers/scripting-*.test.js` (or similar) for payload
      builders, invokable gating, poll machine.
- [ ] Write `docs/Li/LITHIUM-MGR-SCRIPT.md` (QueryRefs, invoke, cleanup).
- [ ] Link from TOC + MGR.
- [ ] Finish any leftover `scripting.js` split so all files ≤ 1000.

**Exit gate / validation:**

- [ ] `npm test && npm run lint`.
- [ ] `wc -l src/managers/scripting/*` all ≤ 1000.
- [ ] TOC lists `LITHIUM-MGR-SCRIPT.md`.

**Status:** pending

**Lessons learned:**

---

# Band F — Course Builder (Lithium operator)

Pipeline design remains COURSEBUILDER.md. These phases are the Lithium
surface + Helium gates those screens need.

## Phase 20 — Course Builder manager scaffold (ID 34)

**Goal:** Loadable manager with placeholder queue layout, no fake success.

**Dependencies:** Phases 0, 1, 9 pattern, 12 ID docs.

**Entry gate:**

- [ ] Phase 0 reserved ID 34. Phase 1 close path exists.

**Work items:**

- [ ] `src/managers/course-builder/` js/html/css. Register 34 in
      `managerRegistry`, `_importManager`, `lithium.json`.
- [ ] `cleanup()` implemented.
- [ ] Menu: Helium menu/lookup seed **or** documented temporary load path
      (utility/debug) until menu row exists — no silent dead ID.
- [ ] Docs: `LITHIUM-MGR-COURSEBUILDER.md` stub pointing at
      COURSEBUILDER.md.

**Exit gate / validation:**

- [ ] `npm run build && npm test && npm run lint`.
- [ ] Manual or unit: loader imports ID 34 without throw.

**Status:** pending

**Lessons learned:**

---

## Phase 21 — Helium `course_build_*` gate (COURSEBUILDER CB-2)

**Goal:** Lithium can query runs. **Do not** invent tables in the SPA.

**Dependencies:** COURSEBUILDER CB-1 (state machine) should be designed;
CB-2 migrations.

**Entry gate:**

- [ ] COURSEBUILDER.md CB-1 transition table exists **or** this phase
      authors the enum into that file’s Working Log and this plan’s log.
- [ ] Operator agrees to apply Helium migrations on the target DB.

**Work items:**

- [ ] Implement or consume CB-2: `course_build_runs`,
      `course_build_artifacts`, `course_build_events` in Acuranzo.
- [ ] QueryRefs: list runs by state, get run + artifacts, get events.
- [ ] LithiumTable tableDef for the queue.
- [ ] `mkt` / `mka` after migrations.

**Exit gate / validation:**

- [ ] SQL insert run + artifact + event on stage (COURSEBUILDER CB-2
      gate) **or** Hydrogen migration test named in the log.
- [ ] QueryRef numbers recorded here and in `LITHIUM-MGR-COURSEBUILDER.md`.

**Status:** pending

**Lessons learned:**

---

## Phase 22 — Queue UI (submission / needs_human)

**Goal:** Operator sees Cap-fed runs without raw SQL.

**Dependencies:** Phases 20–21. COURSEBUILDER CB-5 (intake) may still be
SQL-fed fixtures.

**Entry gate:**

- [ ] At least one fixture `course_build_runs` row exists (seed or SQL).

**Work items:**

- [ ] Table: state, title/suggestion, updated_at, cost estimate if column
      exists.
- [ ] Filters by state (`submission_pending`, `approve_pending`,
      `inspect_pending`, …).
- [ ] Detail pane: suggestion payload + latest event + artifact list.
- [ ] Staff-only: hide manager if punchcard/menu says so (Phase 11
      decision).

**Exit gate / validation:**

- [ ] Manual: fixture row visible and filterable (screenshot or log).
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 23 — Operator invoke: Accept / Decline / Approve / Abandon

**Goal:** Human gates call `Build.*` scripts.

**Dependencies:** Phase 16 invoke helper. COURSEBUILDER CB-3 job runner +
seeded `Build.Accept` / `Build.Decline` / `Build.Approve` /
`Build.Abandon` (`invokable=1`).

**Entry gate:**

- [ ] Named `Build.*` scripts exist in `scripts` **or** this phase seeds
      them in Helium (minimal handlers that transition state + write an
      event). Do not add C.

**Work items:**

- [ ] Buttons on the detail pane call `invokeScript` with `run_id` (and
      reason where needed).
- [ ] Refresh run after invoke. Show script error JSON.
- [ ] Enforce: Research must not start without Approve (runner-side;
      UI just hides the button).

**Exit gate / validation:**

- [ ] Fixture: Decline → terminal declined + event. Separate fixture:
      Accept (or Approve, whichever is implemented) changes state.
- [ ] `npm test` for button→payload mapping.
- [ ] COURSEBUILDER Working Log cross-link.

**Status:** pending

**Lessons learned:**

---

## Phase 24 — Outline and artifact viewer

**Goal:** Operators read outline / materials without DB spelunking.

**Dependencies:** Phase 22. Artifacts from CB-12/13 may be fixtures.

**Entry gate:**

- [ ] Artifact row kind `outline` (or fixture JSON) exists.

**Work items:**

- [ ] Render outline JSON (modules, outcomes) sanitized.
- [ ] List materials / checklist artifacts; open in a read-only pane.
- [ ] Version list if multiple artifacts of the same kind.

**Exit gate / validation:**

- [ ] Manual: outline visible for fixture.
- [ ] XSS fixture in artifact body does not execute.
- [ ] `npm test && npm run lint`.

**Status:** pending

**Lessons learned:**

---

## Phase 25 — Inspect / Publish / deep links

**Goal:** Remaining human hexes that belong in Lithium.

**Dependencies:** Phase 23. COURSEBUILDER CB-22+ may still be stub scripts.

**Entry gate:**

- [ ] Phase 23 pattern works.

**Work items:**

- [ ] Inspect Pass/Fail → `Build.InspectPass` / `Build.InspectFail` (names
      flexible; lock in log).
- [ ] Publish → `Build.Publish` (never auto; always human).
- [ ] If `canvas_course_id` / `lms_course_id` present, “Open LMS” link
      (URL from config, not a token).
- [ ] Disable Publish unless state is allowed.

**Exit gate / validation:**

- [ ] Buttons emit the locked script names (unit test).
- [ ] Manual against stub scripts that only write events if full LMS load
      is not ready (variance logged; do not pretend Canvas published).

**Status:** pending

**Lessons learned:**

---

## Phase 26 — Course Builder tests, docs, file-size

**Goal:** Band F closable as a Lithium feature even if COURSEBUILDER CB-34
E2E is later.

**Dependencies:** Phases 20–25.

**Entry gate:**

- [ ] Phases 20–25 Status complete or explicitly `[~]` deferred.

**Work items:**

- [ ] Unit tests: filters, invoke payloads, state→button visibility.
- [ ] Finish `LITHIUM-MGR-COURSEBUILDER.md` (IDs, QueryRefs, scripts,
      states).
- [ ] TOC + MGR + COURSEBUILDER.md link back to the manager.
- [ ] All `course-builder` files ≤ 1000 lines.

**Exit gate / validation:**

- [ ] `npm test && npm run lint && npm run build`.
- [ ] TOC links resolve.
- [ ] COURSEBUILDER.md CB-9/CB-33 notes Lithium manager path.

**Status:** pending

**Lessons learned:**

---

# Band G — Hardening

## Phase 27 — OIDC client verification

**Goal:** Lithium OIDC path still matches KEYCLOAK / LITHIUM-KEYCLOAK
recipe after sprint churn.

**Dependencies:** None (can run after Band A).

**Entry gate:**

- [ ] Hydrogen `OIDC_RP.Enabled` known true/false on the target (log).

**Work items:**

- [ ] Walk `LITHIUM-KEYCLOAK.md` client checklist against
      `oidc-client.js` + login button.
- [ ] Integration tests: handoff invalid → 401; start unknown provider
      does not fall back (mock or live).
- [ ] Confirm password login still works with IdP down (if testable).

**Exit gate / validation:**

- [ ] Working Log: enabled? start URL? provider id match?
- [ ] `npm test`. If real Keycloak E2E still blocked on OTP, record
      `[~]` and do **not** block the sprint on KEYCLOAK_PLAN Phase 5.

**Status:** pending

**Lessons learned:**

---

## Phase 28 — Integration suite honesty

**Goal:** CI cannot go green with zero integration tests.

**Dependencies:** Phase 13 skip fix.

**Entry gate:**

- [ ] Phase 13 `it.skip` shipped.

**Work items:**

- [ ] Auth integration uses documented base URL (align README vs 8080 vs
      lithium.philement.com).
- [ ] Add one JWT + `POST /api/conduit/script` `Api.Echo` integration
      test (skip if no server / scripting disabled).
- [ ] Add one `auth_query` QueryRef smoke if a stable public-safe ref
      exists.

**Exit gate / validation:**

- [ ] Hydrogen down → skipped, not empty-pass (log).
- [ ] Hydrogen up → Echo integration passes (log job).
- [ ] `npm test`.

**Status:** pending

**Lessons learned:**

---

## Phase 29 — Coverage, lint, and audit gate

**Goal:** Numbers in TST are current; lint is clean on `src/`; no
unreviewed critical/high dependency vulnerabilities.

**Dependencies:** Bands E–F preferred so new files are included.

**Entry gate:**

- [ ] Phases 19 and 26 complete or deferred.

**Work items:**

- [ ] `npm run test:coverage` + update TST table (core bars: jwt, utils,
      conduit including script helper).
- [ ] `npm run lint && npm run lint:css` zero errors.
- [ ] Do not chase 80% on Tour/placeholders.
- [ ] **`npm audit` gate.** No `npm audit` step exists anywhere today
      (script or otherwise). Run `npm audit` in `elements/003-lithium`,
      log the summary (critical/high/moderate/low counts), and either fix
      or record each critical/high finding with a reason it's deferred
      (e.g. dev-only dependency, no fix available). Do not add an
      `audit fix --force` that bumps majors as a side effect — this phase
      is a gate, not a dependency upgrade pass.

**Exit gate / validation:**

- [ ] TST coverage table dated in this phase’s log.
- [ ] Lint commands exit 0.
- [ ] Working Log has the `npm audit` summary and disposition of any
      critical/high findings.

**Status:** pending

**Lessons learned:**

---

## Phase 30 — Live Hydrogen smoke (operator path)

**Goal:** One sitting, password or OIDC login, Scripting Echo, Course
Builder fixture decline/accept if Band F live.

**Dependencies:** Phases 16, 22–23 if claiming CB; else Scripting only.

**Entry gate:**

- [ ] Deployed or local Hydrogen with migrated Acuranzo + JWT user.

**Work items:**

- [ ] Checklist: login, renew still quiet, open Scripting, run Echo,
      close manager (no leftover Tabulator DOM — Phase 1), open Course
      Builder if present.
- [ ] Record Hydrogen version, Lithium `version.json`, DB design.

**Exit gate / validation:**

- [ ] Working Log `P30` checklist all boxes, or explicit blockers.
- [ ] No SEGV / blank SPA / leaked dialogs.

**Status:** pending

**Lessons learned:**

---

## Phase 31 — Closeout

**Goal:** Plan can move to `docs/Li/plans/complete/` when the user agrees.

**Dependencies:** Phases 0–30 complete or `[~]` with rationale.

**Entry gate:**

- [ ] Pause point is this phase.

**Work items:**

- [ ] Sweep INS checklist against remaining grep smells; leftover items
      go to `docs/Li/TODO.md` with effort tags (Hydrogen TODO style).
- [ ] Ensure COURSEBUILDER.md pause point references Lithium manager.
- [ ] Update AGENTS.md “Last reviewed” + Known defects (strike fixed).
- [ ] Update `docs/Li/plans/README.md` / `docs/Li/TODO.md`.

**Exit gate / validation:**

- [ ] AGENTS.md Known defects only lists remaining items.
- [ ] This plan’s phase Status blocks are complete or `[~]`.
- [ ] User accepts move to `complete/LITHIUM_SPRINT_COMPLETE.md`.

**Status:** pending

**Lessons learned:**

---

# Open decisions

| # | Question | Default if unset |
|---|----------|------------------|
| 1 | Terminal utility numeric ID vs Crimson 5 | Utility key `terminal` only; no numeric 5 |
| 2 | Punchcard vs menu-only authz | Menu data is the gate; punchcard helper must not claim otherwise |
| 3 | Style Manager apply | Implement inject if CSS text exists; else disable buttons |
| 4 | Course Builder menu seed | Helium menu row in Phase 20/21; no dead ID 34 |
| 5 | Script rename | Forbidden in v1 (edit in place) |
| 6 | LMS open URL | Config key; no token in the browser |

---

# Working Log

### P-plan-20260822 — Sprint plan authored

- What we did: Architecture review of Lithium; wrote
  `elements/003-lithium/AGENTS.md`; authored this 32-phase plan (0–31)
  covering review remediations, Scripting invoke, and Course Builder
  operator UI. Course pipeline remains
  `/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`.
- Gate result: plan-only; Phase 0 next.
- Lessons: `closeManager`/`cleanup` mismatch and no-op HTML escapes are
  the highest Lithium defects. Scripting already has table+editor+save
  but no `/api/conduit/script` client. DOMPurify is already a dependency.
- Follow-ups: Start Phase 0 (ID table + lifecycle lock + QueryRef
  confirm).

### P-verify-20260821 — Plan-vs-code verification pass (no implementation)

- What we did: Re-checked every quantitative AGENTS.md/plan claim against
  the actual tree before Phase 0 starts, since the plan and AGENTS.md were
  still untracked (`git status` shows both as `??`) and unverified.
- Findings:
  - `closeManager` still calls only `destroy()` (`src/app/manager-loader.js`
    L372-379) — no `cleanup`/`teardown` fallback. Confirmed as described.
  - `src/shared/conduit.js` is still query/batch-query only — no
    `invokeScript`/`getScriptJob`. Confirmed.
  - `escapeHtml` exists in `src/core/utils.js:343`; no `sanitizeHtml` or
    DOMPurify usage anywhere in `src/`. Confirmed.
  - Files > 1000 lines: **exactly 14**, matching AGENTS.md's "~14" — full
    list now recorded as a table in Phase 7 (see above) instead of two
    named examples, so Phase 7/8 exit gates are checkable without
    re-deriving the inventory.
  - `.style.` assignments: 461 (`\.style\.\w+\s*=`); live `console.*`: 110
    total, 71 not commented out; direct `localStorage` call sites: ~132
    excluding `jwt.js`; `alert`/`confirm`/`prompt`: exactly 9 sites. All
    match AGENTS.md's approximate counts — no correction needed there.
  - `src/managers/course-builder/` does not exist yet (Phase 20 not
    started). All 32 phase Status blocks are literally "pending." No
    sprint work has landed in git history.
  - `docs/Li/LITHIUM-TOC.md` L21-26 ("Development server needs Hydrogen")
    already states Vite/HMR work locally and Hydrogen is only required
    for login/data — it does **not** say "cannot run." Phase 0's dev-server
    work item was rewritten from "fix the wording" to "confirm the wording
    is already correct," since the premise (an overstated TOC) does not
    match the current file.
  - `docs/Li/LITHIUM-MGR.md` disagreements with AGENTS.md's ID table are
    real and specific (IDs 9, 10, 32 wrong; 7 placeholder IDs mismarked
    "✅ Implemented" at exactly 49 lines each) — Phase 0/12 scope is
    correct as written.
- Gate result: no phase Status changed (still all pending); this was a
  verification pass on the plan document itself, not sprint work.
- Follow-ups: none new. Phase 0 is still next and its work items are now
  slightly more precise (dev-server item, Phase 7 baseline table).

### P-gaps-20260821 — Gap review: additions beyond the original 32 phases

- What we did: reviewed the wider codebase for work areas the original
  plan didn't cover (CI, E2E tooling, a11y, i18n, telemetry, WS/chat,
  bundle budget, CSP, dependency scanning, rollout/feature-flags) and
  added the ones the user confirmed as in-scope, without inventing new
  bands for things that don't need one.
- Added:
  - Phase 9: explicit Chats/WebSocket-manager decision record (placeholder
    vs. own follow-up phase) before the generic collapse swallows it.
  - Phase 10: fix (wire or remove) the dead `_flushToServer()` stub in
    `src/core/log.js` — logs are queued but never actually sent; same
    "for now" pattern this phase already targets.
  - Phase 29: `npm audit` gate (no such step exists anywhere today);
    renamed to "Coverage, lint, and audit gate."
  - Non-goals: explicit one-line entries for a11y, i18n, CSP, and
    bundle/perf budget so these are stated decisions instead of silent
    gaps (none of the three were previously mentioned anywhere in
    `docs/Li/`).
- Deliberately **not** added (user declined): a new CI band/phase
  (GitHub Actions) and a Playwright/E2E smoke-suite phase. Both remain
  real gaps (no CI exists anywhere in the repo; test layer is Vitest +
  happy-dom only, no real-browser tests) but are out of scope for this
  plan revision — worth revisiting if the user changes their mind later.
- Gate result: plan-only; no phase Status changed.
- Follow-ups: if CI or E2E tooling is wanted later, Band G (27–31) is the
  natural place — Phase 30's manual checklist is the direct analog of
  what a Playwright smoke test would automate.

---

# Revision history

| Date | Change |
|------|--------|
| 2026-08-22 | Initial LITHIUM_SPRINT.md (Phases 0–31) |
| 2026-08-21 | Verification pass: confirmed all quantitative claims against code; corrected Phase 0's dev-server item (TOC already reads correctly); added exact 14-file baseline table to Phase 7/8 |
| 2026-08-21 | Gap review: added Chats/WS decision record (Phase 9), log.js flush-stub fix (Phase 10), npm audit gate (Phase 29 renamed), and a11y/i18n/CSP/bundle Non-goals; declined CI band and Playwright E2E phase per user |
