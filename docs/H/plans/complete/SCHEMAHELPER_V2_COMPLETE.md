<!-- markdownlint-disable MD007 MD024 -->
# SchemaHelper v2 Plan — Target, Instance, Progress

## Purpose

Gated follow-on to SchemaHelper v1. v1 is the review queue (dashboard,
1-by-1 actions, packets, confirmed apply). v2 is **operator chrome**:
named screens, a short shared **Instance** block, honest env-var labels,
and a progress bar that can actually be read while SchemaTool runs.

v1 archive:
[`SCHEMAHELPER_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_COMPLETE.md).

Operator guide:
[`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).

This is still **Bash + Lua 5.5** under `extras/schematool/`. No C, no
new blackbox number unless asked. After Lua/Bash: `zsh -ic 'mks'` and
Test 98. After docs: Test 04 / Test 90. Headless coverage stays Test 72.

## How To Use This Document

- Work **one phase at a time**. Do not start a phase until the previous
  exit gate is green.
- Mark `[x]` only after verification. Defer with `[~]` plus rationale.
- **Phase 0–5 are complete (0.6.5).** This plan is archived.
- Record lessons in the Working Log.

## Resuming Work

**ARCHIVED (2026-09-09).** v2 Definition of Done is met at SchemaHelper
**0.6.5**. Do not reopen work here. Operator guide:
[`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).

### Resume here next session

Do not resume this file. v2 is complete.

## Priority

| | |
| --- | --- |
| **Band** | P2 — operator tooling |
| **Effort** | M (TUI chrome + progress; optional accept-hash) |
| **Done** | v1 complete (0.5.8). v2 Phase 0–5 (0.6.5). |
| **Why this shape** | The queue works. The Target / SchemaTool screens and the expect bar do not. |
| **Backlog** | TODO item 25 dropped on archive (2026-09-09) |

---

## Relationship To v1

Keep: splash, wrapper picker, connect probe, SchemaTool invoke, dashboard,
review, explore, skip/accept/apply/packet/promote, sidecar, `--allow-write`,
`--work-dir`, mouse, Test 72.

Change: chrome titles, the session header (now **Instance**), picker
blurbs, the running-screen progress UI. Additive SchemaTool **stderr**
for expect description and compare per-ref class (see Phase 3).

Do not rebuild the auditor. Do not auto-author migrations.

---

## Carry-forward From v1

Incomplete v1 items. Phase 0 lock:

| Item | v2 | Why |
| --- | --- | --- |
| Dashboard accepted-list + **un-accept** | **In** (Phase 4) | Locked in v1 Phase 0; never built |
| Payload-**hash** accept invalidation | **In** (Phase 4) | Sidecar can store `hash`; accept never writes or compares it |
| Custom SchemaTool extra flags | **Out** unless asked | Wrappers already parse sourced `exec` |
| `--batch` | **Out** | Interactive-only lock |
| Bitfield SchemaTool exit | **Out** | SchemaTool-side |
| Full `design_NNNN.lua` author | **Out** | `[m]` stub remains a stub |

---

## Locked UX (operator)

Phase 0 approved 2026-09-08. Implement against this table, not a
different sketch.

### 1. Screen titles

Border title is no longer the generic ` SchemaHelper ` on every mode.

| Mode | Title |
| --- | --- |
| splash | `SchemaHelper` |
| picker | `SchemaHelper: Target` |
| running / result | `SchemaHelper: SchemaTool` |
| dashboard | `SchemaHelper: Dashboard` |
| review / note | `SchemaHelper: Review` |
| explore | `SchemaHelper: Explore` |
| apply | `SchemaHelper: Apply` |

### 2. Instance block (four lines, seven-letter labels)

Same block on every screen **except splash**, at the top of the body.
Labels are exactly seven letters so the values column lines up.
Label text uses `ATTR.COLHEAD`; values use `ATTR.PATH` (Connect uses
the live connect attr).

```text
Wrapper  schematool_sqlite.sh
Logging  schemahelper_schematool.log
Working  /tmp/schemahelper-20260908T215400Z-ab12
Connect  Not Connected
```

| Label | Value |
| --- | --- |
| Wrapper | **Filename** of the wrapper script (not a full path) |
| Logging | **Filename** of the SchemaTool log |
| Working | Temporary **folder** (`--work-dir`) |
| Connect | Target: always `Not Connected`. After Enter: last live connect text (no passwords, no re-ping on later screens) |

Shorter than today's session header (`wrapper` / `out-dir` / `work-dir` /
`state` / `track` / `log` / `connect`). Drop `out-dir`, `state`, and
`track` from this block.

On Target, Logging and Working are **planned** names even if the
directory is created later: Logging is always
`schemahelper_schematool.log`; Working is the session `/tmp/schemahelper-…`
path. `mkdir` happens on Enter, not on highlight.

### 3. Env vars, not families

Picker blurbs must not say `ACURANZO_DB_*`. Show the real **names**
(host, user, password-env, database, schema) resolved from the wrapper.
SQLite blurb stays the file basename. Never print password **values**.

### 4. Instance follows the highlighted target

Up/down (and mouse) on Target updates Wrapper / Logging / Working for
the **selected** row immediately. Connect stays `Not Connected` until
the SchemaTool screen. Do not ping the database on every highlight
change. First ping is SchemaTool (Enter).

### 5. Progress bar overhaul (SchemaTool screen)

Today: a full-inner-width `█`/`─` bar with `%` concatenated, so the
percent clips; only `expect N/M ref R`; no issue list.

v2:

| Rule | Detail |
| --- | --- |
| **a. Description** | While a ref is in flight, show its migration description (name, else summary), not only `ref N` |
| **b. Eighths** | One cell per 8 migrations. Fill with Unicode eighths (`▏▎▍▌▋▊▉█`). Width = `ceil(total / 8)` cells (378 refs → ~48 cells), not “as wide as the tty”. Last cell may be a partial eighth |
| **c. Issue color** | If any of the 8 refs in a cell had a problem, that cell uses a warning/error color |
| **d. Percent** | Do not clip. Same line, immediately after the short bar |
| **e. Issue pane** | Scrollable list **below** the bar as issues appear (ref + class). Newest at bottom. `j`/`k` and wheel scroll while SchemaTool still runs |
| **f. Classes** | `drift` / `missing` / `catalog` / `anomaly`, color-matched to the cell |

**Fill then tint:** the bar fills during **expect**. Cells tint and the
issue list grows during **compare** (issues are not known at extract
time). Console `tables` output stays unchanged.

Additive stderr (keep existing `expect N/M ref R` and the compare
summary line):

```text
expect N/M ref R name=<text>
compare N/M ref R ok|drift|missing_load|missing_apply|anomaly|orphan
```

Catalog track may add a matching per-object `catalog …` line. No code
blobs on stderr.

---

## Current Vs Desired

| Surface | Today (0.5.8) | v2 |
| --- | --- | --- |
| Chrome title | ` SchemaHelper ` every mode | Per-mode titles in Locked UX §1 |
| Picker | No session header | Instance on top; list below |
| Session header | 6–7 full paths | Instance 4 lines on every screen except splash |
| Connect on picker | N/A (probe after pick) | `Not Connected` |
| Connect on run | `ok/fail  family  target` | Same idea, Instance `Connect` line; kept on later screens |
| Picker blurb | `ACURANZO_DB_* / schema demo` | Concrete env var names |
| Highlight change | List mark only | Instance values refresh; planned Logging/Working |
| Progress | Wide `█` bar + clipped `%` | Eighths, description, fill-then-tint, `%` after bar, j/k list |

---

## Candidates (Phase 0 resolved)

All six candidates are **in** and folded into Locked UX:

1. Later-screen titles — in (§1).
2. Instance everywhere except splash — in (§2), including explore/apply.
3. Connect probe only after Enter — in (§4).
4. Issue classes — in (§5f).
5. `j`/`k`/wheel in the issue pane while running — in (§5e).
6. Logging/Working placeholders on Target — in (§2).

---

## Non-goals

- Replacing SchemaTool or Hydrogen LOAD/APPLY
- Auto-authoring a complete `acuranzo_NNNN.lua`
- `--batch` / headless SchemaHelper
- Catalog drop-table / extra-column apply
- ncurses or a second TUI stack
- Secrets in Instance, packets, or sidecar
- Unity / C

---

## Testing Policy

| Layer | When |
| --- | --- |
| luacheck (Test 98) | Every Lua change |
| shellcheck (`mks`) | Every `.sh` change |
| Test 72 | Queue/packet/apply fixtures; extend for Instance text + progress eighths if headless |
| Test 04 / 90 | Docs |
| Live TTY smoke | sqlite wrapper: Target Instance updates; SchemaTool bar + issue list |
| Unity / new `test_NN` | No |

---

## Phase 0 — Design Lock

### Goal

Lock titles, Instance sketch, env-var display, progress contract, and
which carry-forwards are in. No source edits.

### Dependencies

v1 archive.

### Entry gate

- [x] v1 archived at
      [`SCHEMAHELPER_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_COMPLETE.md).
- [x] Operator confirms Locked UX (2026-09-08).

### Work items

- [x] Confirm chrome titles for Target and SchemaTool.
- [x] Confirm Instance four lines / seven-letter labels / filename vs folder.
- [x] Confirm Connect is `Not Connected` on Target (no ping on highlight).
- [x] Confirm env-var display: which names (host/user/password-env/database/schema).
- [x] Confirm progress: eighths alphabet, cell coloring, `%` placement, issue-pane keys.
- [x] Confirm SchemaTool additive stderr shape for description + issue (or helper-only if log already enough — it is not, today).
- [x] Accept or drop Phase 4 carry-forwards (un-accept + hash).
- [x] Accept or drop Candidates (later titles, Instance after run, etc.).

### Exit gate / validation

- [x] Locked UX table is no longer “proposal.”
- [x] Review stop before Phase 1.

### Status

Complete.

### Lessons learned

- Expect stderr is only `expect N/M ref R`. Name/summary already exist in
  the expect JSON, but drift/missing/anomaly are computed later in
  `schematool_compare.lua`, which emits one summary line at the end.
  Live issue tint therefore cannot happen during extract — fill during
  expect, tint during compare.
- `save_decision` can persist `extra.hash`, but accept never passes a
  hash and the queue never compares it.

---

## Phase 1 — Titles And Instance

### Goal

Dynamic chrome title per Locked UX §1. Instance on every screen except
splash. Highlight updates Wrapper / Logging / Working without connecting.

### Dependencies

Phase 0 lock.

### Entry gate

- [x] Phase 0 Status complete.

### Work items

- [x] Border title from mode (splash / Target / SchemaTool / Dashboard /
      Review / Explore / Apply).
- [x] Shared Instance painter (four aligned lines); replaces
      `session_header` on every screen except splash.
- [x] Target: Instance above picker list.
- [x] Planned Logging filename + Working path on Target; `mkdir` on Enter.
- [x] Selected-row change refreshes Instance (filename, planned log,
      work-dir).
- [x] Connect on Target is `Not Connected`. After Enter, later screens
      keep the live Connect text (no re-ping).

### Exit gate / validation

- [x] sqlite picker: moving selection changes Wrapper filename in Instance
      (Test 72-0015 headless; live TTY not run this session).
- [x] After Enter, title is `SchemaHelper: SchemaTool` and Connect is live
      text (`CHROME_TITLE` + instance_block running-mode assert).
- [x] Dashboard title is `SchemaHelper: Dashboard` and still shows Instance
      (`session_header` → `instance_block`; dashboard renderer still green).
- [x] Test 98 clean.

### Status

Complete.

---

## Phase 2 — Env Var Labels

### Goal

Picker (and any Instance-adjacent blurb) shows real env var **names**,
not `FAMILY_*`.

### Dependencies

Phase 1.

### Entry gate

- [x] Phase 1 Status complete.

### Work items

- [x] Resolve wrapper flags / env names (already in `schemahelper_connect` parse).
- [x] Replace `WRAPPER_BLURB` `*_` families.
- [x] Never print password values; password-env **name** is OK.

### Exit gate / validation

- [x] postgresql row names `ACURANZO_DB_HOST` (etc.), not `ACURANZO_DB_*`.
- [x] Yugabyte still never implied as `ACURANZO_*`.
- [x] Test 98 clean.

### Status

Complete.

### Lessons learned

- Most wrappers do not put `--host ${FAMILY_DB_HOST}` on `exec` (postgresql /
  mysql / mariadb / db2). `parse_wrapper` therefore cannot harvest env
  **names**. Blurbs come from the same `apply_family` prefix map via
  `connect.picker_blurb`, not from expanded flag values.

---

## Phase 3 — Progress Bar

### Goal

SchemaTool screen: description, eighths bar sized to `ceil(N/8)`, cell
color on issues, unclipped `%`, scrolling issue list.

### Dependencies

Phase 1 (Instance already on this screen).

### Entry gate

- [x] Phase 1 Status complete. Phase 2 preferred.

### Work items

- [x] Additive expect stderr: `expect N/M ref R name=<text>` (`tables`
      path unchanged).
- [x] Additive compare stderr: `compare N/M ref R` plus class
      (`ok|drift|missing_load|missing_apply|anomaly|orphan`). Catalog
      track: matching per-object line if needed.
- [x] Helper parser: bar fills on expect; cells tint and issue list
      grows on compare.
- [x] Paint eighths (`▏`…`█`); width `ceil(total/8)`; last cell may be
      a partial eighth.
- [x] Color a cell if any of its 8 refs failed/drifted/anomalous.
- [x] `%` immediately after the short bar, same line.
- [x] Scrollable issue window below the bar; newest at bottom; `j`/`k`
      and wheel while running. Classes: drift / missing / catalog /
      anomaly.
- [x] Test 72 headless: eighths width for a known total; issue color
      given a fake log.

### Exit gate / validation

- [x] 378-ref scale: bar ~48 cells, `%` fully visible.
- [x] A known drifted ref tints its cell and appears in the list while the run continues.
- [x] `mks` + Test 98 + Test 72 green.

### Status

Complete.

### Lessons learned

- Expect JSON is redirected to a file; helper log is stderr. Compare
  findings JSON is also file-out. Additive per-ref stderr is enough;
  do not parse the tables dump.
- UI classes collapse `missing_load` / `missing_apply` / `orphan` →
  `missing`; catalog findings (including extras) → `catalog`. `ok`
  compare lines never enter the issue list.
- Last cell of 378 refs is 2/8 (`▎`), not a full `█`.
- `j`/`k`/wheel are handled in the invoke poll loop; other keys are
  ignored until SchemaTool exits. Live sqlite TTY smoke not run this
  session.

---

## Phase 4 — Accept Hash And Un-accept (carry-forward)

### Goal

v1 leftovers: accepted items can be listed and un-accepted; accept
invalidates when id + payload hash change.

### Dependencies

Phase 0 said this phase is **in**.

### Entry gate

- [x] Phase 0 included this phase.

### Work items

- [x] Dashboard (or a subview): list sidecar `accepted`; key to un-accept.
- [x] Compare stored hash to current expected/live hash on rebuild; mismatch returns the finding to review.
- [x] Test 72: accept then hash change reappears; un-accept returns to queue.

### Exit gate / validation

- [x] Second launch hides matching accepted ids; changed hash re-shows.
- [x] Un-accept restores the finding without deleting other decisions.
- [x] Test 72 + Test 98 green.

### Status

Complete.

### Lessons learned

- SchemaTool findings have no hash field. Accept hashes SHA-256 of the
  drifted field’s expected vs actual (catalog: expected vs live strings)
  via `sha256sum`. Empty/missing stored hash re-queues (legacy accepts
  without a real hash are not sticky).
- Un-accept is dashboard `[x]` on the highlighted accepted id (`j`/`k`);
  `remove_decision` drops that sidecar row only.
- Dashboard reloads sidecar from disk on each entry so accept/un-accept
  survive leaving review.

---

## Phase 5 — Docs

### Goal

Operator guide, extras READMEs, this plan Status, Test 04/90.

### Dependencies

Phases 1–3 minimum. Phase 4 if shipped.

### Entry gate

- [x] Target + SchemaTool Instance usable on sqlite. Progress bar usable.

### Work items

- [x] Update [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md) (titles, Instance, keys, progress).
- [x] extras READMEs if the plan link is the only stale bit (should already point here).
- [x] Test 04 + Test 90.

### Exit gate / validation

- [x] Test 04 / 90 / 98 / `mks` green.

### Status

Complete.

---

## Definition Of Done (v2)

- Titles match Locked UX §1 (including later screens).
- Every screen except splash shows the four-line Instance (Wrapper /
  Logging / Working / Connect), seven-letter labels in a different color
  than the values.
- Target Connect is `Not Connected`; highlighting a wrapper updates
  Instance without a DB ping.
- Env labels are real variable names.
- SchemaTool progress: migration description, eighths bar of width
  `ceil(N/8)`, fill-then-tint, `%` after the bar, scrolling issue list
  with `j`/`k`.
- SchemaTool stays read-only aside from additive stderr.
- Carry-forwards from Phase 0 are either done or explicitly dropped.
- Docs + luacheck + shellcheck + Test 72 green.

---

## Working Log

### 2026-09-08 — Plan drafted

- Archived v1 to
  [`SCHEMAHELPER_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_COMPLETE.md).
- Operator lock-in: Target/SchemaTool titles, Instance four-liner,
  real env names, picker updates Instance, progress eighths + issue
  pane.
- Carry-forward proposed in: un-accept + hash. Out: `--batch`, extra
  flags, bitfield exit, full Lua author.
- No code written. Next: Phase 0 confirm, then Phase 1.

### 2026-09-08 — Phase 0 locked

- Operator confirmed Locked UX as written, then took all six
  candidates **in**: later titles; Instance on **every** screen
  (including explore/apply); probe only after Enter; issue classes;
  `j`/`k`/wheel while running; planned Logging/Working on Target.
- Progress: fill during expect, tint + issue list during compare.
  `%` immediately after the short bar. Newest issues at bottom.
- Env blurb: real names (host/user/password-env/database/schema);
  sqlite file basename; never password values.
- Phase 4 **in** (un-accept + hash). Extra flags / `--batch` /
  bitfield / full Lua author stay **out**.
- No Lua. Next: approve Phase 1.

### 2026-09-08 — Phase 1 complete (0.6.0)

- Chrome titles from `C.CHROME_TITLE` (splash / Target / SchemaTool /
  Dashboard / Review / Explore / Apply).
- `instance_block` replaces `session_header` on every screen (four
  seven-letter labels). Target Connect is `Not Connected`. Planned
  Logging=`schemahelper_schematool.log` and Working path before mkdir.
- Picker highlight (selected row, and hover via leftover hotspots)
  updates Wrapper filename without a ping.
- Test 72 15/15, Test 98 436 files, `mks` 166 files. Live sqlite TTY
  smoke not run.
- Next: approve Phase 2 (real env-var names in picker blurbs).

### 2026-09-08 — Phase 1 chrome tweak (0.6.1)

- Splash no longer shows Instance (welcome table only).
- Instance labels (`Wrapper` / `Logging` / `Working` / `Connect`) paint
  in `ATTR.COLHEAD`; values stay `ATTR.PATH` (Connect keeps live attr).
- Test 72 Instance asserts split label/value. Next: approve Phase 2.

### 2026-09-08 — Phase 2 complete (0.6.2)

- Dropped `WRAPPER_BLURB` family wildcards. `connect.picker_blurb` prints
  `PREFIX_HOST USER PASS NAME schema …`; sqlite stays `hydrodemo.sqlite`.
- Password **values** never appear; `*_PASS` is the env name.
- Test 72 16/16, Test 98 436 files, `mks` 166 files.
- Next: approve Phase 3 (eighths bar + additive SchemaTool stderr).

### 2026-09-08 — Phase 3 complete (0.6.3)

- Expect: `expect N/M ref R name=<rest-of-line>` (name else summary).
- Compare: per-ref `compare N/M ref R ok|drift|missing_load|missing_apply|anomaly|orphan`; keep `Compare:` summary.
- Catalog: `catalog ref R class` for findings only (not ok objects).
- Helper: fill on expect, tint + issue list on compare; eighths
  `ceil(N/8)`; `%` after bar; j/k/wheel scroll (newest at bottom).
- Test 72 17/17, Test 98 436 files, `mks` 166 files. Live sqlite TTY
  smoke not run.
- Next: approve Phase 4 (un-accept + payload-hash invalidation).

### 2026-09-09 — Phase 4 complete (0.6.5)

- Accept writes SHA-256 of expected+live; `build()` re-queues on
  mismatch or missing hash.
- Dashboard lists accepted ids; `[x]` un-accepts the highlight; `j`/`k`
  move the mark. Other sidecar decisions stay.
- Test 72 18/18, Test 98 436 files, `mks` 166 files.
- Next: Phase 5 docs.

### 2026-09-09 — Phase 5 complete

- Operator guide: per-mode titles, Instance, real env names, eighths
  progress, dashboard `[x]` un-accept, accept hash.
- extras/schematool README layout lines for hash / un-accept.
- Test 04 2411/2411, Test 90 346 files, Test 98, `mks` already green.
- v2 Definition of Done met. Optional archive remains.

### 2026-09-09 — Archived as v2 complete

- Moved to
  [`SCHEMAHELPER_V2_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md).
- Dropped TODO item 25. Operator guide stays
  [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).
