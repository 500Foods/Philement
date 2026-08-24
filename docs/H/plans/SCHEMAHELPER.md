<!-- markdownlint-disable MD007 MD024 -->
# SchemaHelper Plan — Interactive SchemaTool Front-End

## Purpose

Define a gated, phase-by-phase plan for **SchemaHelper**: a Lua TUI that sits
in front of the shipped SchemaTool auditor and turns a batch of drift findings
into **operator decisions**.

SchemaTool already answers “what is different?” and even suggests commented
SQL. It does **not** help an operator work the queue: push official Lua onto
the database, pull a live divergence into a new migration packet, skip it, or
accept it as a known difference.

This document is the working plan. Edit it as work proceeds. Each phase is
numbered, focused, and gated. Do not start a phase until the previous phase’s
exit gate is green. Record learnings in the Working Log.

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**, **Work items**,
  **Exit gate / validation**, **Status**, **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and target phase.
- After each phase: fill **Status**, append reusable discoveries to
  **Working Log**, then stop for review before the next phase.
- This is **Bash + Lua 5.5 under `extras/`**, not C. After script/Lua
  changes: `zsh -ic 'mks'` and Test 98 (`luacheck`). See
  [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md). TUI: terminal.lua.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-24):** Phase 5 complete (both slices,
v0.5.0 + v0.5.4). Phase 7 partial: catalog DDL apply (nullable/add-column with
`object.column` confirm) and promote-packet helper both shipped (v0.5.5).
Remaining Phase 7 items deferred/rejected by design: `--batch` (interactive-only
lock), group-catalog-rows (confirmed not needed by operator), bitfield exit
(SchemaTool-side). v1 Definition of Done is met. Lint: shellcheck + luacheck +
Test 04 all clean.

Dashboard count wording is in: migration totals stay migration counts;
the queue line is **Findings for review** (field-level + catalog).
Dashboard and review `[r]` re-run SchemaTool, then rebuild the queue
from the new artifacts; sidecar decisions stay.

Post-v1 field hardening is in (see Working Log): catalog fold Lua 5.5
const + catalog degrade; custom-wrapper connect (exec flags + sourced
`exec`); dashboard `q` / result-screen paint; explore decode.

### Resume here next session

1. Confirm this document is the source of truth.
2. Smoke: `schemahelper.sh --allow-write schematool_sqlite.sh`
   — Enter review, `[u]` on a `code`/`name`/`summary` drift, type
   `REF.field`. `[u]` on an orphan ref, type bare `REF` (e.g. `1290`).
   `[u]` on a catalog nullable mismatch, type `object.column` (e.g.
   `accounts.id`) — DDL runs in a transaction. `[g]` generates a packet;
   `[m]` promotes it into Helium. Without the flag, `[u]`/`[m]` stay
   disabled. `[r]` on the dashboard re-runs SchemaTool.
3. Phase 5 fully closed (both slices + orphan DELETE). Phase 7 partial:
   catalog DDL apply + promote-packet helper. Remaining Phase 7 items
   deferred or rejected by design — see CURRENT PAUSE POINT.

### Session checklist

1. Read **CURRENT PAUSE POINT** and last **Working Log** entries.
2. Confirm previous phase **Status** is complete.
3. Re-read next phase Goal + Exit gate only.
4. Implement → verify gates → update this doc → stop.

## Priority

| | |
| --- | --- |
| **Band** | P2 — operator tooling, not a Hydrogen subsystem |
| **Effort** | L (TUI + packet writer + optional confirmed metadata apply) |
| **Done** | v1 (Phases 0–4 + 5 both slices + 6) + Phase 7 partial (catalog DDL apply + promote). |
| **Why this shape** | SchemaTool is complete and read-only. The missing piece is a decision loop over its findings, including “this live DB should become a new migration.” |
| **Do not start casually** | Write paths can mutate `queries` or (later) live DDL. Phase 0 must lock safety before any apply code exists. |

Backlog entry: [TODO.md item 25](/docs/H/TODO.md).

---

## Recommendation

**Yes. Build this as a front-end, not a second auditor.**

SchemaTool already does the hard work: Lua expect extract, native-client
metadata dump, hybrid-C catalog fold, targeted probes, `tables` checklist,
commented remediation `.sql`, orphan `.mig`, and `--format json`. A second
tool that re-implements compare would rot immediately.

What operators actually need next is a **review queue** that starts with
totals, then walks leftovers one by one:

| Need | SchemaTool today | SchemaHelper |
| --- | --- | --- |
| See the whole picture | Batch table + footer counts | Opening dashboard: total / perfect / accepted / findings for review + variance classes |
| See one disparity | Scroll the table + detail | 1-by-1: “this is the variance, what would you like to do?” |
| Official Lua wins | Commented `UPDATE` / LOAD/APPLY guidance | **Apply to database** (confirmed, per finding) |
| Live DB wins | Orphan `.mig` only | **Generate a migration** (assigned next ref, packet) |
| Look closer | Post-table detail dump | **Explore in more detail** (paged, then back to the prompt) |
| Not now | Re-run the whole audit | **Skip for now** (still subject for review next time) |
| Known divergence | Re-reported every run | **Accept permanent variance** (persisted; counts as accepted) |

The Lua TUI idea matches the existing extras stack (`lua/` already owns
expect/compare/remediate). A thin `schemahelper.sh` launcher plus
`schemahelper.lua` is the Hydrogen-shaped version of this: no C, no
subsystem, no REST, no ncurses. The TUI is
[terminal.lua](https://lunarmodules.github.io/terminal.lua/) on **Lua 5.5**.

**Do not generate a complete `design_NNNN.lua` in v1.** Assign the next
number and write an informational packet (finding, diffs, suggested SQL,
notes). A human still authors the real migration. That is enough to stop
losing context and colliding on refs.

**Keep SchemaTool read-only forever.** SchemaHelper is the only writer, and
only when `--allow-write` is set and the operator confirms the specific
finding. Default SchemaHelper is review + packets + accepted variances.

---

## Problem Statement

Two legitimate sources of truth exist in this project:

1. **Official migrations** — `design_NNNN.lua` under Helium (e.g.
   [`/elements/002-helium/acuranzo/migrations/`](/elements/002-helium/acuranzo/migrations/)).
   When a live DB has drifted *behind* or *away* from these, the fix is to
   align the database (metadata `UPDATE`, Hydrogen LOAD/APPLY, or a new
   official forward migration that repairs live shape).
2. **A living database** — a demo, canvas, or production-adjacent schema
   where someone made useful changes that are **not** on disk. Those changes
   should become the *next* numbered migration if they are worth keeping.

SchemaTool surfaces both situations as the same kind of row: “not the same.”
It cannot tell the operator which direction to go. The `.sql` file is a
review artifact, not a workflow. The `.mig` file only covers **orphan refs**
(in DB, not on disk), not catalog extras (extra column, different
nullability) or “this applied row is *better* than Lua.”

Without a helper, operators:

- Re-run SchemaTool and scroll a long table.
- Hand-copy detail/SQL into notes.
- Guess the next migration number.
- Accidentally treat “update `queries.code`” as if it replayed DDL.
- Re-triage the same accepted differences every session.

---

## Relationship To SchemaTool

```text
  wrappers (schematool_*.sh) or custom flags
                 │
                 ▼
        schematool.sh  ── read-only ──► findings.json
                 │                      catalog checklist
                 │                      finding_detail.txt
                 │                      commented .sql / .mig
                 ▼
        schemahelper.lua  (TUI decision loop)
                 │
     ┌───────────┼───────────────┬────────────────┐
     ▼           ▼               ▼                ▼
  apply to DB  packet NNNN    skip for now    accept permanent
  (opt-in)     (not a .lua)   (still review)  (sidecar state)
```

SchemaHelper **invokes** SchemaTool. It does not fold DDL, probe catalogs, or
decode brotli payloads itself.

Completed SchemaTool plan:
[SCHEMATOOL_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md).

Operator guide: [SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md).

### What SchemaTool already emits (reuse)

| Artifact | Path under `--out-dir` | Helper use |
| --- | --- | --- |
| Metadata findings | `findings.json` | Queue: drifts, missing LOAD/APPLY, anomalies, orphans |
| Metadata checklist | layout/data JSON | Optional summary pane |
| Catalog checklist | catalog data JSON | Queue: missing table/column, nullability |
| Catalog findings | `catalog_findings.json` | Counts + exit only today — **too thin** (see contract) |
| Detail text | `finding_detail.txt` / `catalog_finding_detail.txt` | Review pane |
| Remediation | commented `.sql` | Source for **Update database** (metadata) |
| Orphans | `.mig` | Seed for **Create migration packet** |

### Contract gap (Phase 0 / 1)

`catalog_findings.json` currently stores **counts only**. Per-row catalog
failures live in the checklist JSON. SchemaHelper should not scrape the
human `tables` output.

**Additive SchemaTool change (preferred):** attach a `failures[]` (or
`rows[]`) array to catalog findings, each with `object`, `column`, `check`,
`expected`, `live`, `notes`, and optional commented `remediation`. Metadata
`findings.json` already has structured drifts; optionally attach the
per-finding commented SQL block there too so the helper does not parse the
`.sql` file.

SchemaTool remains an auditor. The contract change is JSON-only and
backward-compatible.

---

## Home In The Repo

**Locked proposal (confirm in Phase 0):**

| Item | Path |
| --- | --- |
| Launcher | [`extras/schematool/schemahelper.sh`](/elements/001-hydrogen/hydrogen/extras/schematool/) |
| TUI entry | [`extras/schematool/schemahelper.lua`](/elements/001-hydrogen/hydrogen/extras/schematool/) |
| Lua modules | `extras/schematool/lua/schemahelper_*.lua` |
| Operator docs | `/docs/H/tools/SCHEMAHELPER.md` |
| This plan | `/docs/H/plans/SCHEMAHELPER.md` |

Rationale: same folder as the auditor and the Test 40 wrappers; same Lua
runtime SchemaTool already requires; not `src/`; not a Hydrogen subsystem.

Entry for humans: `schemahelper.sh` (deps, `stty`, wrapper discovery).
`schemahelper.lua` is the interactive app. The user-facing name is
**SchemaHelper**; the Lua file name is `schemahelper.lua` as requested.

---

## Goals And Non-Goals

### Goals

1. **Pick a SchemaTool invocation** — list `schematool_*.sh` wrappers plus a
   custom-flags path; run with `--format json --out-dir <workspace>`.
2. **Open on an analysis dashboard** — totals first (migrations found,
    perfect, accepted variations, findings for review) plus a classification
   breakdown of variance kinds. Then enter the 1-by-1 queue.
3. **Walk every leftover disparity** — one finding at a time: “this is the
   variance, what would you like to do?”
4. **Five review actions** — Explore in more detail, Skip for now, Accept
   permanent variance, Apply to database, Generate a migration.
5. **Assign the next migration number** when generating a packet, without
   colliding with on-disk `design_NNNN.lua` or earlier packets.
6. **Persist selections in a sidecar state file** next to the SchemaTool
   artifacts, named like the source run, so several engines/designs can
   share one folder and later runs resume.
7. **Stay pretty and keyboard-driven** — Lua 5.5 + terminal.lua full-screen
   UI, not a web UI, not a wall of raw JSON.
8. **Default read-only** — packets and accepts do not touch the database.

### Non-goals (this plan)

- Replacing SchemaTool or Hydrogen LOAD/APPLY.
- Auto-authoring a complete, shippable `acuranzo_NNNN.lua`.
- Auto-executing the full commented `.sql` file.
- Live catalog DDL apply for missing tables / live-only extras (Phase 7 did
  NOT add drop-table / drop-column / extra-column apply — only nullable and
  add-column).
- Scanning product row data.
- A new blackbox test number, C code, or REST endpoint.
- ncurses, ltui, or a second TUI stack. The one allowed extra rock is
  `terminal` (and its deps `luasystem`, `utf8`) on Lua 5.5.
- Vendoring C rocks into `extras/`. `luasystem` stays a LuaRocks install.
- Writing secrets, passwords, or connection strings into packets.

---

## Locked Decisions

| Topic | Proposal |
| --- | --- |
| Product split | SchemaTool = read-only auditor. SchemaHelper = interactive front-end. |
| Language | Bash launcher + Lua 5.5 TUI. No C. |
| Lua version | **Lua 5.5 only** (same as Hydrogen host / [LUA_55_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_55_PLAN_COMPLETE.md)). Refuse 5.4 and earlier. |
| TUI stack | [terminal.lua](https://lunarmodules.github.io/terminal.lua/) (`luarocks --lua-version=5.5 install terminal`). Dep: `luasystem` >= 0.7.0. Lua 5.5 already has `utf8`; do **not** require the `utf8` rock (it fails to compile on 5.5). Install `terminal` with `--nodeps` after `luasystem` if the rockspec still lists `utf8`. No hand-rolled `stty`/ANSI, no ncurses. |
| Ingest | SchemaTool `--format json --out-dir`. Never scrape ANSI tables. |
| Wrapper picker | Discover `extras/schematool/schematool_*.sh` (postgresql, mysql, mariadb, sqlite, db2, cockroachdb, yugabytedb). **Custom / SchemaTool pass-through deferred** (Phase 2+). |
| Finding identity | Stable id: `meta:{kind}:{ref}:{type}:{field}` or `cat:{object}:{column}:{check}` or `orphan:{ref}`. |
| Opening screen | **Splash first** (full-terminal single border, version + SchemaTool version, Enter only), then analysis dashboard (totals + variance classes). Enter review only after the dashboard. |
| Chrome | Red single-line box + `SchemaHelper` title stay on every screen. Later surfaces (picker, running, dashboard, review) are inset in that body. Wrapper picker is top-aligned, not a bottom `cli.Select`. |
| Session header | After a wrapper is known: `wrapper`, `out-dir`, `state`, `track`, `log`, `connect`, then a full-width rule. No “SchemaTool run” title. |
| Connect probe | As soon as a wrapper is chosen, Lua pings the live DB with the wrapper’s env family (never print passwords). Fail status blocks a new SchemaTool run. |
| Splash look | Single-line box around the whole tty; bright red border. Left: SchemaHelper / version / release. Right: SchemaTool / version / release. Blank, then Lua `_VERSION` left and `terminal.lua` version right. Yellow names, green versions, cyan dates, magenta runtimes. |
| Splash dismiss | Enter only. Other keys ignored. Ctrl-C / tty restore via `terminal.initwrap`. |
| Versions | SchemaHelper **0.4.0** (released 2026-08-23) fronts SchemaTool **1.8.0** (2026-08-23; additive catalog JSON). |
| Skip for now | Persist as `skipped` in the sidecar so quit/resume works; still **subject for review** on the next launch (not accepted). |
| Accept permanent variance | Persist as `accepted` in the sidecar. Counts as “Accepted variations”; hidden from the 1-by-1 queue until the **migration payload changes** (id + expected/live hash). Dashboard (Phase 3) must list accepted items and allow **un-accept** (returns the finding to subject-for-review). |
| Apply to database (v1) | **Metadata only**: uncomment the single finding’s SchemaTool `UPDATE`/`DELETE` guidance and run it via the native client, transaction + typed confirm. Requires `--allow-write`. Record as `applied` in the sidecar. |
| Apply to database (catalog) | **Not in v1.** Offer explore / generate-migration instead. |
| Generate a migration | Write a **packet directory**, assign `NNNN = max(disk refs, reserved packets) + 1`. Do not write into Helium `migrations/`. Record as `packet` in the sidecar. |
| Workspace | Default `--out-dir` is the directory of the chosen wrapper (`extras/schematool/` for Test 40 scripts is OK). Override with `--out-dir`. |
| Sidecar state name | `schemahelper_<design>_<engine>.json` next to `schematool_<design>_<engine>_<utc>.sql`. Stable stem (no timestamp) so re-runs resume. Optional `--state-file` override. |
| Packet location | Same workspace as `--out-dir`: `schemahelper_<design>_<engine>_<ref>/`. Override with `--packet-dir`. Warn once if that path is inside the git tree. |
| Next-ref scan | `design_NNNN.lua` in `--migrations` plus reserved packet refs in the workspace. |
| Re-audit | After apply or packet, operator can re-run SchemaTool and reload; sidecar decisions still apply. |
| Secrets | Inherit SchemaTool `--password-env`; never print; never write into packets or state. |
| SchemaTool edits | Additive JSON only (`failures[]`, `live_extras[]`). No behavior change to default `tables` path. Live extras do **not** change catalog `exit_code`. |
| Interactive only | No `--print-summary` / `--print-queue`. Headless operators use SchemaTool directly. |
| Re-run | Always invoke SchemaTool unless `--reuse` and artifacts for the selected `--track` already exist. |
| Track | `--track metadata\|catalog\|both` (default `both`). No extra SchemaTool flag pass-through in Phase 1. |
| Sidecar engine | Taken from the wrapper **basename** (`schematool_mariadb.sh` → `mariadb`), not SchemaTool's aliased `--engine`. |

---

## Runtime — Lua 5.5 And terminal.lua

SchemaHelper is an **operator** Lua program (system `lua` on PATH), not a
Hydrogen-embedded script. It still must be **Lua 5.5**, the same version
the project already standardized on.

| Piece | Requirement |
| --- | --- |
| Interpreter | Lua **5.5** (`_VERSION` starts with `Lua 5.5`, or `LUA_VERSION_NUM >= 505`) |
| Docs | [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md), [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md), [LUA_55_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_55_PLAN_COMPLETE.md) |
| TUI library | [terminal.lua](https://lunarmodules.github.io/terminal.lua/) |
| Install | `luarocks install terminal` into the Lua 5.5 tree |
| Rock deps | `luasystem` >= 0.7.0 (C). Built-in Lua 5.5 `utf8` replaces the `utf8` rock. |
| Not used | ncurses, ltui, raw `stty`, hand-rolled ANSI boxes |

`schemahelper.sh` fails fast if `lua` is not 5.5 or if
`require("terminal")` fails, with the install one-liner. SchemaTool’s
existing Lua (expect/compare) keeps working on the same 5.5 interpreter.

### Why terminal.lua

It is mechanisms-over-policies, UTF-8, no curses, and already has the
widgets this TUI needs:

| SchemaHelper surface | terminal.lua piece |
| --- | --- |
| Wrapper / track picker | `cli.Select` |
| SchemaTool run | `progress.Bar` / `terminal.progress` spinner |
| Analysis dashboard | `ui.panel.Screen` + `ui.panel.Bar` header + `ui.panel.Text` |
| 1-by-1 review | `ui.panel.Text` (variance) + `ui.panel.KeyBar` (e/s/a/u/g) |
| Explore in more detail | `ui.panel.Text` (scrollable diff / SQL) |
| Apply confirm | `ui.panel.Confirm` or `cli.Confirm` (plus typed ref) |
| Optional note on packet | `cli.Prompt` |
| Resize | `ui.panel.Screen` reflow |

Do **not** use the Canvas / braille graph layer. Do not vendor
`luasystem` (C) into `extras/`.

---

## Opening Dashboard

Launch always shows the **splash** first (full-terminal single red border,
SchemaHelper 0.1.0 + SchemaTool 1.7.1, Enter to continue). After SchemaTool
finishes (or after loading an existing `--out-dir`), the TUI **does not**
jump into the first finding. It shows the current state:

```text
┌ SchemaHelper  acuranzo / postgresql / demo ─────────────────────────┐
│ Source: schematool_postgresql.sh     workspace: /tmp/st-pg          │
│ State:  schemahelper_acuranzo_postgresql.json                       │
├─────────────────────────────────────────────────────────────────────┤
│ Total migrations found          213                                 │
│ Perfect migrations                4                                 │
│ Accepted variations              12                                 │
│ Findings for review              12                                 │
├─────────────────────────────────────────────────────────────────────┤
│ Variance classes (findings for review)                              │
│   metadata content drift          5                                 │
│   missing LOAD                    2                                 │
│   missing APPLY                   1                                 │
│   orphan DB ref                   1                                 │
│   catalog nullability             2                                 │
│   catalog missing column          1                                 │
├─────────────────────────────────────────────────────────────────────┤
│ [Enter] begin review   [r]e-audit   [q]uit                          │
└─────────────────────────────────────────────────────────────────────┘
```

### Count definitions

| Line | Meaning |
| --- | --- |
| Total migrations found | Disk `design_NNNN.lua` refs in scope, plus orphan DB refs not on disk |
| Perfect migrations | All SchemaTool checks pass and no permanent accept is recorded |
| Accepted variations | Sidecar `accepted` (and still matching payload hash if gated) |
| Findings for review | Queue items: one per drifted field, plus catalog rows. Not a migration count (24 drifted refs + 38 catalog rows = 62). Excludes accepted / applied / packet. |

Skipped items stay in **findings for review**. Applied / packet items drop
out of that line (they are handled, not “perfect”). Footer or a second
block may show `applied` / `packet reserved` counts so the operator can
see work already done in this workspace.

### Variance classes

Built from SchemaTool finding kinds (metadata + catalog). Only **subject
for review** rows feed the class table. Draft classes:

| Class | Source |
| --- | --- |
| metadata content drift | `findings.drifts` (code / name / summary) |
| missing LOAD | `findings.missing_load` |
| missing APPLY | `findings.missing_apply` |
| orphan DB ref | `findings.orphans` |
| anomaly 1000+1003 | `findings.anomalies` |
| catalog missing table | catalog check `table` |
| catalog missing column | catalog check `column` |
| catalog nullability | catalog check `nullable` |
| catalog live extra | if Phase 0 adds live-only extras |

---

## Sidecar State File

Selections survive quit and later launches. The file lives **in the same
folder** as the SchemaTool artifacts and uses the same stem pattern so
several runs can share one directory:

| SchemaTool artifact | SchemaHelper sidecar |
| --- | --- |
| `schematool_acuranzo_postgresql_20260823T190000Z.sql` | `schemahelper_acuranzo_postgresql.json` |
| `schematool_acuranzo_mysql_20260823T190100Z.sql` | `schemahelper_acuranzo_mysql.json` |
| `schematool_acuranzo_sqlite_….sql` | `schemahelper_acuranzo_sqlite.json` |

The helper stem **omits the UTC stamp** so a second launch against the
same design+engine resumes the same decisions. SchemaTool may write a new
timestamped `.sql` each audit; the helper still opens the stable sidecar.

Override: `--state-file PATH`. Default: `<out-dir>/schemahelper_<design>_<engine>.json`
(cwd if `--out-dir` is unset).

Do **not** default into the git tree. If the operator’s `--out-dir` is
inside the repo, warn once; prefer `/tmp/…` or `~/.cache/hydrogen/schemahelper/`.

### Draft sidecar shape

```json
{
  "version": 1,
  "design": "acuranzo",
  "engine": "postgresql",
  "schema": "demo",
  "updated_utc": "2026-08-23T19:10:00Z",
  "cursor_id": "meta:drift:1148:1003:code",
  "decisions": [
    {
      "id": "meta:drift:1280:1003:code",
      "action": "accepted",
      "hash": "…",
      "note": "mail seed known drift",
      "at": "2026-08-23T19:05:00Z"
    },
    {
      "id": "orphan:1290",
      "action": "packet",
      "ref": 1290,
      "packet": "schemahelper_acuranzo_postgresql_1290",
      "at": "2026-08-23T19:08:00Z"
    }
  ]
}
```

`action` values: `skipped` | `accepted` | `applied` | `packet`.

No passwords, no full `code` blobs — ids, hashes, notes, packet paths only.

---

## Actions

Each finding is shown with a recommended default based on kind, which the
operator can override.

| Key | Action | Source of truth | Effect |
| --- | --- | --- | --- |
| `e` | Explore in more detail | — | Page the full diff / suggested SQL; return to the same prompt |
| `s` | Skip for now | Neither | Record `skipped`; stay in subject-for-review next launch |
| `a` | Accept permanent variance | Neither | Record `accepted`; dashboard accepted++; hidden until id/hash changes |
| `u` | Apply to database | Official Lua | Apply one remediation: metadata `UPDATE` (`REF.field` confirm), orphan `DELETE` (`REF` confirm), or single-statement catalog DDL (`object.column` confirm). Disabled unless `--allow-write` |
| `g` | Generate a migration | Live DB | Reserve next `NNNN`, write packet, record `packet` |
| `m` | Promote packet to Helium | Packet | Write `design_NNNN.lua` stub from packet `SUGGESTED.sql`; mark packet `promoted`. Disabled unless `--allow-write` |
| `n` / `p` | Next / previous | — | Move in the review queue (does not decide) |
| `r` | Re-audit | — | Re-run SchemaTool, rebuild dashboard + queue, keep sidecar decisions |
| `q` | Quit | — | Flush sidecar; return to dashboard or exit |

### Recommended default by finding kind

| Kind | Default | Why |
| --- | --- | --- |
| Metadata drift (Lua ≠ stored `code`) | Update database **or** Create packet | Operator must choose direction. Prompt; do not guess. |
| Missing LOAD / missing APPLY | Update database is **wrong** | Guidance only: run Hydrogen AutoMigration. Action `u` disabled; show help. |
| Orphan DB ref | Create packet | This is the original `.mig` use case. |
| Catalog missing column on live | Create packet **or** Apply DDL | Phase 7: `[u]` applies `ADD COLUMN` (confirm `object.column`; type from expected fold). Missing **table** on live stays packet-only (too structural for a helper ALTER). |
| Catalog extra / live-ahead (locked for Phase 1, see Contract gap) | Create packet | Live has something official Lua does not. |
| Catalog nullability mismatch | Apply to database **or** Create packet | Phase 7: `[u]` applies `ALTER COLUMN SET/DROP NOT NULL` (confirm `object.column`). Packet is safer alternative. |
| Anomaly both 1000+1003 | Detail + skip | Needs a human; helper must not auto-delete. |

**Verified against source (2026-08-23):** `schematool_catalog_compare.lua`
only iterates `exp_names` (tables/columns present in the **expected/disk**
fold) and looks up the matching `live` entry — it never walks
`live_tables`/`live_map` for names absent from `exp_map`. So today, a table
or column that exists live but not on disk is silently invisible to the
catalog track. v1 catalog compare does **not** emit “extra live column” as a
first-class finding.

**Locked decision (no longer deferred): yes, add it in Phase 1.** This is
not an optional nice-to-have — it is the mechanism for the plan’s second
motivating use case (“build a migration from a database that diverged in a
way worth keeping”). Without it, `g` (generate a migration) only ever fires
from **metadata orphans** (`queries` rows not on disk); a live table/column
that was hand-added and never written back to Lua stays permanently
invisible to SchemaHelper. Phase 1 must add either a small SchemaTool
additive change (a `live_extras[]` array alongside `catalog_findings.json`,
populated by a reverse pass — walk `live_tables`/`live_map` for names absent
from `exp_map`/`exp_tables`) **or**, if that would touch too much of
`schematool_catalog_compare.lua` for one phase, a helper-side pass that
loads the same `catalog_expected.json` / `catalog_live.json` SchemaTool
already writes under `--out-dir` and diffs them the same way. Prefer the
SchemaTool-side change: it is one additive loop next to code that already
has both maps in scope, and it keeps SchemaHelper from re-implementing
catalog comparison. Do not ship Phase 4 (migration packets) as “done” for
the catalog track until this exists — otherwise the packet feature only
covers half of what it was built for.

---

## Migration Packet

A packet is **not** a migration. It is the briefing a human (or a later
authoring pass) needs to write `design_NNNN.lua`.

### Directory

```text
<out-dir>/schemahelper_acuranzo_postgresql_1283/
  MANIFEST.json
  PACKET.md
  FINDING.json
  DETAIL.txt
  SUGGESTED.sql
```

### `MANIFEST.json` (draft)

```json
{
  "ref": 1283,
  "design": "acuranzo",
  "engine": "postgresql",
  "schema": "demo",
  "created_utc": "2026-08-23T19:00:00Z",
  "status": "reserved",
  "finding_ids": ["cat:accounts:new_col:column"],
  "source": "schemahelper",
  "schemahelper_version": "0.4.0",
  "schematool_version": "1.8.0"
}
```

`status`: `reserved` → `promoted` (human copied into Helium) or `abandoned`.

### `PACKET.md` (draft)

Human-readable: assigned ref, design, engine, schema, finding ids, short
operator notes typed in the TUI, and pointers to the sibling files.

### `SUGGESTED.sql`

Direction is **database → official**, the inverse of SchemaTool’s usual
“make DB match Lua” remediation. For orphans, copy the `.mig` payload. For
catalog live-ahead, emit the live `CREATE`/`ALTER` fragment we can infer
(best-effort; may be comments + `DESCRIBE` capture). Never claim this SQL
is a complete multi-engine migration.

### Number assignment

1. Scan `--migrations` for `design_(\d+)\.lua`.
2. Scan `--packet-dir` for `MANIFEST.json` `ref` values still `reserved`.
3. `next = max + 1`.
4. Create the packet directory **before** showing the number as assigned.
5. Do not leave holes unless the operator passes `--ref N` to force one.

Collisions with a ref someone is authoring by hand in Helium are still
possible; the packet header must say “confirm the number is free before
promoting.”

---

## TUI Sketch

Splash (first screen, Phase 0):

```text
┌ SchemaHelper ───────────────────────────────────────────────────────┐
│                      Welcome to SchemaHelper                        │
│                      Frontend to Schema Tool                        │
│                                                                     │
│               SchemaHelper   0.1.0   2026-08-23                     │
│                  SchemaTool   1.7.1   2026-08-06                    │
│                         Lua   5.5.1   2026-08-03                    │
│                terminal.lua   0.1.0   2026-06-07                    │
│                                                                     │
│                       Migration Comparator                          │
│            …/002-helium/acuranzo/migrations                         │
│                                                                     │
│                       Database Comparator                           │
│            …/extras/schematool/schematool_db2.sh                    │
│                                                                     │
│                     Press Enter to continue                         │
│                        Press ESC to exit                            │
└─────────────────────────────────────────────────────────────────────┘
```

One-by-one prompt (after the dashboard). Short summary first; explore
opens the long diff.

```text
┌ SchemaHelper  review  3 of 12 subject ──────────────────────────────┐
│ This is the variance                                                │
│   id:     cat:accounts.password_hash / nullability                  │
│   class:  catalog nullability                                       │
│   expect: nullable=true     live: nullable=false                    │
│   note:   1190 fold says NULL; live still NOT NULL                  │
├─────────────────────────────────────────────────────────────────────┤
│ What would you like to do?                                          │
│   [e] explore in more detail                                        │
│   [s] skip for now                                                  │
│   [a] accept permanent variance                                     │
│   [u] apply to database          (disabled — catalog / no --allow-write) │
│   [g] generate a migration       (next ref 1283)                    │
│   [n]ext  [p]rev  [r]e-audit  [q]uit to dashboard                   │
└─────────────────────────────────────────────────────────────────────┘
```

Wrapper picker (inset at the top of the persistent chrome, after splash):

```text
┌ SchemaHelper ───────────────────────────────────────────────────────┐
│ Select SchemaTool target                                            │
│                                                                     │
│   ● schematool_postgresql.sh     ACURANZO_DB_* / schema demo        │
│   ○ schematool_mysql.sh          CANVAS_DB_* / schema demo          │
│   ○ schematool_mariadb.sh        CANVAS_DB_* / schema demomrdb      │
│   ○ schematool_sqlite.sh         hydrodemo.sqlite                   │
│   ○ schematool_db2.sh            HYDROTST_DB_*                      │
│   ○ schematool_cockroachdb.sh    ACURANZO_DB_* / schema democrdb    │
│   ○ schematool_yugabytedb.sh     YUGABYTE_DB_*  (never ACURANZO)    │
│                                                                     │
│ Press Enter to select                                               │
│ Press ESC to exit                                                   │
└─────────────────────────────────────────────────────────────────────┘
```

Implementation notes:

- `schemahelper.sh` checks Lua 5.5 + `require("terminal")`, then execs
  `schemahelper.lua`. terminal.lua / luasystem own raw mode and restore.
- Wrapper picker is inset in the chrome body (up/down/enter/esc), not
  `cli.Select` (that widget draws from the cursor and drops the border).
- Full-screen path uses `ui.panel.Screen` (resize-safe). Do not assume
  256-color; use terminal.lua text stacks.
- If stdout is not a tty, refuse the TUI and print usage. No headless
  dump flags; use SchemaTool directly for batch JSON.
- Explore uses `ui.panel.Text` scrolling; honor SchemaTool
  `--detail-max-lines` when stuffing the panel.

---

## Safety

SchemaTool’s production bar stays intact. SchemaHelper adds a **narrow**
write path.

| Guard | Rule |
| --- | --- |
| Default | No DB writes. Packets + sidecar accepts only. |
| `--allow-write` | Required before `u` / `m` is offered. |
| Per-finding confirm | Type the ref (or object.column) to apply. No “apply all.” Catalog DDL uses louder `object.column` confirm. |
| Transaction | All apply SQL (metadata UPDATE, orphan DELETE, catalog DDL) runs in one transaction. |
| One statement family | Only the uncommented block for **this** finding id. |
| Catalog DDL scope | Only `nullable` (SET/DROP NOT NULL) and `column` (ADD COLUMN); refused on `table`, `extra_table`, `extra_column`. |
| No full-file apply | Never pipe the SchemaTool `.sql` wholesale. |
| Read-only SchemaTool | Helper runs SchemaTool with the same RO client guards it has today. Apply uses a **separate** client session without RO, only after confirm. |
| Wrong-host | Wrapper picker must show engine + env family (especially Yugabyte vs ACURANZO). |
| Secrets | `--password-env`; never in packets, sidecar state, or screen dumps. |
| Prefer Hydrogen | Missing LOAD/APPLY → tell the operator to run AutoMigration; do not invent INSERT/DDL. |
| Metadata ≠ live shape | After a metadata `UPDATE`, the TUI must remind: this does not replay DDL. |

---

## Scope And Repo Areas

Primary: `/elements/001-hydrogen/hydrogen/extras/schematool/`

Related:

- `/docs/H/tools/SCHEMATOOL.md` — note the helper; keep SchemaTool RO
- `/docs/H/tools/SCHEMAHELPER.md` — new operator guide (docs phase)
- `/elements/001-hydrogen/hydrogen/extras/schematool/README.md`
- `/elements/001-hydrogen/hydrogen/extras/README.md`
- `/docs/H/SITEMAP.md`, `/docs/H/STRUCTURE.md`, `/docs/H/TODO.md`
- Optional additive edits to SchemaTool Lua compare/remediate for JSON
  contract only

Date of snapshot: 2026-08-23

---

## Testing Policy

| Layer | When | What |
| --- | --- | --- |
| **luacheck** | Every Lua change | Test 98 |
| **shellcheck** | Every `.sh` change | `zsh -ic 'mks'` |
| **markdown** | Docs | Test 90 + Test 04 after new pages |
| **Unity / C** | Never for this plan | No `src/` |
| **Blackbox** | Not required for v1 | Do not add `test_NN` unless later asked |
| **Smoke** | Phase 2+ | Fixture findings JSON → queue build → packet write (no live DB required). Optional: run against Test 40 sqlite wrapper in review-only mode |

---

## Phase 0 — Design Lock

### Goal

Lock product split, TUI stack, finding identity, packet format, write
policy, and the SchemaTool JSON contract. Ship a runnable splash so the
look can be reviewed before Phase 1.

### Dependencies

None. SchemaTool CLI 1.7.1 is shipped.

### Entry gate

- [x] This plan exists and is linked from
      [plans/README.md](/docs/H/plans/README.md) and
      [TODO.md](/docs/H/TODO.md) (both already link here as of 2026-08-23).

### Work items

- [x] Confirm or amend **Locked Decisions** (splash-first, Enter-only,
      red single border + restrained color, versions, accept-until-hash
      plus un-accept list).
- [x] Confirm Lua **5.5** + [terminal.lua](https://lunarmodules.github.io/terminal.lua/)
      as the TUI stack; record the installed rock versions in the Working
      Log. **Installed 2026-08-23**: `lua 5.5.1`, `luarocks 3.9.2`,
      `terminal 0.1.0-1`, `luasystem 0.7.1-1`. The `utf8` 1.3 rock does
      **not** compile on 5.5 (`lua_assert`); use the built-in `utf8`.
- [x] **Visual spike is the real splash** in
      `extras/schematool/schemahelper.lua` (not a disposable
      `spike_terminal.lua`): `ui.panel.Screen` + body `Panel` with
      `border.format = box_fmt.single`. KeyBar/Text still de-risked in
      Phase 2 when the dashboard lands.
- [x] Confirm dashboard count definitions and variance class list
      (as drafted in Opening Dashboard; subject-for-review excludes
      accepted / applied / packet).
- [x] Confirm sidecar name `schemahelper_<design>_<engine>.json` in
      `--out-dir` (stable stem, no timestamp).
- [x] Decide catalog **live-only extras**: **locked to SchemaTool-side
      additive change**, done in Phase 1 (see Contract gap section above).
      Not deferred — it is required for the catalog half of “generate a
      migration.”
- [x] Lock accept invalidation: **id + payload hash**. Re-show if the
      migration/live content changes. Phase 3 must offer an accepted-list
      review that can un-accept.
- [x] Lock packet path (`schemahelper_<design>_<engine>_<ref>/`) and
      next-ref algorithm (`max(disk refs, reserved packets) + 1`).
- [x] Lock apply surface: metadata-only v1, no catalog DDL. Phase 5
      remains optional for v1 done.
- [x] Draft the catalog `failures[]` JSON shape (see Contract gap:
      `object`, `column`, `check`, `expected`, `live`, `notes`, optional
      commented `remediation`) plus `live_extras[]` for the reverse pass.
- [x] Write `schemahelper.sh` + splash-only `schemahelper.lua` (amended:
      user asked to see the look before Phase 1).

### Exit gate / validation

- [x] Locked-decisions table in this doc is no longer “proposal.”
- [x] Live-extras question has an explicit yes (Phase 1, SchemaTool-side).
- [x] Review stop before Phase 1 (splash is the look review).

### Status

Complete (2026-08-23). Splash runnable via
`extras/schematool/schemahelper.sh`.

### Lessons learned

- LuaRocks 3.9.2 `lua_h_exists` greps `LUA_VERSION_NUM 505`. Lua 5.5
  defines that as a macro expression, so `luarocks install terminal`
  fails until `LUA_INCDIR` points at a header that also contains the
  literal `LUA_VERSION_NUM 505`.
- The `utf8` 1.3 rock fails on 5.5 (`lua_assert`). Lua 5.5 already has
  `utf8`. Install `luasystem`, then `terminal --nodeps`.
- Require paths are snake_case (`terminal.ui.panel.screen`), not the
  docs' `Screen` class name.
- `ui.Panel` `border.format` + `border.attr` is the full-tty single
  box. `skip_width_detection = true` avoids the init cursor probe.
- A PTY with unset winsize draws a 0-size box; a real tty is fine.

---

## Phase 1 — Launcher, Picker, Ingest

### Goal

`schemahelper.sh` starts, picks a wrapper or custom flags, runs SchemaTool
into a session `--out-dir`, and `schemahelper.lua` loads a unified finding
queue from JSON.

### Dependencies

Phase 0 lock. SchemaTool `--format json --out-dir`.

### Entry gate

- [x] Phase 0 Status complete.

### Work items

- [x] Extend `schemahelper.sh` (splash launcher already exists):
      `--allow-write`, `--packet-dir`, `--state-file`, `--out-dir`,
      `--track`, `--reuse`. Lua 5.5 + `terminal` checks already fail-fast.
- [x] Discover and list `schematool_*.sh` with engine/env one-liners
      (`cli.Select` after splash when no wrapper is passed).
- [~] Custom path / extra SchemaTool flags — deferred (no pass-through
      this phase; operators pick a wrapper).
- [x] Invoke SchemaTool with `--format json --out-dir <workspace>` (and
      `--catalog` when both/catalog selected). Always invoke unless
      `--reuse`.
- [x] Implement SchemaTool additive JSON: `failures[]` on catalog findings
      (per-row object/column/check/expected/live/notes) **and**
      `live_extras[]` (table/column present live but absent from the
      expected/disk fold). Both are additive; `tables` console output is
      unchanged. SchemaTool **1.8.0**.
- [x] `lua/schemahelper_queue.lua`: merge metadata + catalog into ordered
      findings with stable ids; compute dashboard totals + classes;
      apply sidecar decisions (`accepted` / `applied` / `packet` drop
      out of subject-for-review).
- [x] Load/create `schemahelper_<design>_<engine>.json` in the workspace.
- [~] Headless `--print-summary` / `--print-queue` — rejected. This is an
      interactive tool; headless operators use SchemaTool directly.
      Queue verified against
      `extras/schematool/testdata/schemahelper_queue/`.

### Exit gate / validation

- [x] Picker lists each Test 40 wrapper; a selected (or CLI) wrapper is
      invoked in review-only mode (`--format json`, no writes).
- [~] `--print-queue` fixture — replaced by `schemahelper_queue` smoke on
      checked-in testdata (no live DB).
- [x] Yugabyte wrapper still does not fall through to `ACURANZO_DB_*`
      (unchanged; picker blurb says never ACURANZO).
- [x] `mks` + luacheck clean.
- [x] Secrets never appear in session JSON (queue does not copy `code`
      or passwords).

### Status

Complete (2026-08-23). Interactive invoke + result totals.

### Lessons learned

- `--track catalog` still runs SchemaTool metadata as well: `--catalog`
  without `--only-tables` sets `FULL_AUDIT=1`. The helper queue filters
  by track; SchemaTool may do more work than the queue shows.
- Default workspace beside the wrapper dirties `extras/schematool/` for
  Test 40 scripts. That is accepted; production wrappers live elsewhere.
- Sidecar engine must come from the wrapper basename. `schematool_mariadb.sh`
  and `schematool_cockroachdb.sh` pass aliased `--engine mysql` /
  `--engine postgresql`.
- `cli.Select` is not a `ui.panel.Screen` child; it draws from the cursor
  and drops the chrome. Keep one bordered Screen and paint picker /
  running / result into the body.
- SchemaTool exits 2/3 on drift/anomaly; the helper treats those as a
  successful audit, not a hard failure.
- Live extras stay out of the catalog checklist and do not change
  `exit_code` (JSON-only contract).

---

## Phase 2 — Review TUI

### Goal

Interactive flow: analysis dashboard first, then 1-by-1 “what would you
like to do?” with explore / skip / quit. Read-only this phase.

### Dependencies

Phase 1 queue.

### Entry gate

- [x] Phase 1 Status complete.

### Work items

- [x] `schemahelper.lua` full-screen UI via terminal.lua
      (`ui.panel.Screen` / `Bar` / `Text` / `KeyBar`).
- [x] Wrapper picker via `cli.Select` (if not already done in Phase 1).
- [x] Dashboard: total / perfect / accepted / subject for review + class
      table. Enter begins review; `q` quits.
- [x] Review: short variance summary + the five action labels (`e` `s`
      `a` `u` `g`). `u`/`g` may be shown disabled until later phases.
- [x] `e` opens a scrollable `ui.panel.Text` (diff / SQL) and returns.
- [x] Keys: `n` `p` `r` `q` (quit returns to dashboard).
- [x] terminal.lua restores the tty on all exit paths (verify Ctrl-C).
- [x] Refuse to start when stdout is not a tty.

### Exit gate / validation

- [x] Can walk a fixture queue and a real sqlite Test 40 catalog+meta run.
- [x] Terminal is usable after quit / Ctrl-C.
- [x] Test 98 clean.

### Status

Complete (2026-08-23). Dashboard with Enter→review, explore via TextPanel,
skip/accept persistence, re-audit reload.

### Lessons learned

- `key_map[raw]` is nil for character keys (e/s/a/u/g/n/p/r/q); must check
  raw directly for single-char bindings. Enter maps to `ctrl_j` (ICRNL),
  Escape maps to `ctrl_[`.
- `TextPanel` requires `calculate_layout` before `set_lines`/`render`;
  pass the Screen body inner geometry.
- `--version` must be handled before `initwrap` since terminal init
  fails without a TTY.
- Dashboard rebuild on re-audit re-runs `queue.build` and refreshes `app.built`.

---

## Phase 3 — Skip And Accept

### Goal

`s` records skip-for-now (still subject for review next launch). `a`
records a permanent accept in the sidecar so the next run counts it as
an accepted variation and hides it from the 1-by-1 queue until identity
(and hash, if locked) changes.

### Dependencies

Phase 2 TUI.

### Entry gate

- [x] Phase 2 Status complete.

### Work items

- [x] Write/update the sidecar on every `s` / `a` / quit (and later `u`
      / `g`).
- [x] `skipped` does not change dashboard “subject for review.”
- [x] `accepted` increments accepted variations and leaves the queue.
- [x] Resume: reopen the same `--out-dir` + design + engine; cursor and
      decisions come back; a second engine in the same folder uses its
      own `schemahelper_<design>_<engine>.json`.
- [~] Accepted-list review on the dashboard: inspect accepted items and
      un-accept (returns the finding to subject-for-review).
       (Deferred — not required for Phase 2/3 minimal; cursor reset on quit
       handles the common case.)
- [x] Sidecar contains no secrets and no full `code` blobs.

### Exit gate / validation

- [x] Second launch against the same fixture hides accepted ids.
- [~] Changing expected/live hash re-shows the finding if hash-gated.
      (Hash field is stored but not yet compared; Phase 3+ will add
      payload-hash comparison for invalidation.)
- [x] Waiver file contains no secrets and no full `code` blobs.

### Status

Complete (2026-08-23). `s` records skip-for-now; `a` records accepted;
both persist to sidecar JSON. Decisions are reapplied on rebuild.

### Lessons learned

- `save_decision` uses `jq` to atomically update the JSON sidecar,
  rebuilding the `decisions` array to avoid duplicates by id.
- `load_state` now loads `note` and `hash` from each decision record so
  the review pane can display operator annotations.

---

## Phase 4 — Migration Packets

### Goal

`g` reserves the next ref and writes the packet directory in the same
workspace as the sidecar. No Helium tree writes. No DB writes.

### Dependencies

Phase 3 (can start after Phase 2 if skip/accept slips; prefer after 3).

### Entry gate

- [x] Phase 2 Status complete. Phase 3 preferred.

### Work items

- [x] Next-ref scan (disk `{design}_NNNN.lua` / `design_NNNN.lua` +
      reserved packets). `--ref N` forces a number.
- [x] Write `MANIFEST.json`, `PACKET.md`, `FINDING.json`, `DETAIL.txt`,
      `SUGGESTED.sql`.
- [x] Prompt for an optional one-line operator note (inset chrome;
      Enter = empty note, Esc = cancel).
- [x] Record `packet` in the sidecar; drop from subject-for-review.
- [x] Collision: refuse if `schemahelper_<design>_<engine>_<ref>/` or
      `{design}_NNNN.lua` / `design_NNNN.lua` appears mid-session.
- [x] Dashboard lists reserved refs; warn once if packet path is inside
      the git tree.

### Exit gate / validation

- [x] Two packets in one session get consecutive unused refs (fixture
      disk max 1290 → 1291 then 1292).
- [x] Default packet path is the workspace (`--out-dir`); warn if that
      path is inside the git tree (locked: not `/tmp` / `~/.cache`).
- [x] Packet SQL is review-only (commented header: not applied).
- [x] Test 98 / luacheck clean. `mks` clean.

### Status

Complete (2026-08-23). `[g]` writes
`schemahelper_<design>_<engine>_<ref>/` via
`lua/schemahelper_packet.lua`.

### Lessons learned

- Disk refs must match SchemaTool discover (`{design}_NNNN.lua`) **and**
  the fixture `design_NNNN.lua` name. Helium uses `acuranzo_NNNN.lua`.
- `cli.Prompt` would drop chrome the same way `cli.Select` did. The note
  field is typed inside the bordered body.
- Sidecar `load_state` must keep `ref` / `packet` on each decision or
  the dashboard cannot list reserved packets after resume.

---

## Phase 5 — Update Database (Metadata Only)

**Scope note:** this phase is **optional for v1**. The plan's two driving
use cases — catching a drifted DB up to spec, and turning a useful live
change into a migration packet — are both satisfiable by Phases 1–4 alone
(review, skip, accept, packet) plus the operator running SchemaTool's own
suggested `.sql` by hand, which is exactly today's workflow minus the
manual scrolling/renumbering. Writing to a live database from a TUI is the
single highest-risk, highest-review-burden piece of this whole plan (new
non-RO client session, transaction handling, typed confirm, partial-apply
recovery). Ship and use Phases 1–4 for a few real sessions first; only pick
up Phase 5 if that experience shows the `u` action is actually needed
often enough to justify it. Keep it in the plan (do not delete it — the
design work above is still useful) but do not block "v1 done" on it. See
the updated Definition Of Done.

### Goal

With `--allow-write`, `u` applies **one** metadata remediation (content
drift `UPDATE` of `code`/`name`/`summary`, or a carefully confirmed orphan
`DELETE`) using the native client.

### Dependencies

Phase 2. Strongly prefer Phase 4 so “create packet” exists as the other
direction before writes are possible.

### Entry gate

- [x] Phase 4 Status complete (or explicit variance).
- [x] Phase 0 write policy still “metadata only.”

### Work items

- [x] Generate a **single-field** `UPDATE` from official Lua
      (`finding.expected`), not the whole SchemaTool commented block
      (queue is one field per item).
- [x] Refuse Hydrogen-guidance (missing LOAD/APPLY), catalog, orphan,
      anomaly, and `:decoded` views.
- [x] Open a **non-RO** client session; begin transaction; execute; commit
      (all seven engines; wrapper-sourced when password-env is local).
- [x] Typed confirm = `REF.field` (e.g. `1223.code`).
- [x] On success, record `applied` and drop finding from queue.
- [x] Log apply SQL to `--out-dir` (no password).
  - [x] Orphan / anomaly `DELETE` — orphan slice shipped: true orphans delete
      with `BETWEEN 1000 AND 1003`; anomalies refused.

### Exit gate / validation

- [x] Without `--allow-write`, `u` is disabled.
- [x] Wrong confirm string does not execute.
- [x] sqlite fixture: one-field `UPDATE` then read-back matches official
      Lua.
- [x] SchemaTool itself still cannot write.
- [x] `mks` + Test 98 clean.

### Status

Complete (2026-08-24). Both slices shipped: one-field `UPDATE` (drifts) and
confirmed orphan `DELETE` (true orphans only; anomalies refused).
SchemaHelper **0.5.4**. DELETE uses `BETWEEN 1000 AND 1003` (matches SchemaTool
remediation); confirm is `REF.field` for drifts, bare `REF` for orphans;
sidecar records `applied` for both.

### Lessons learned

- Field-level queue items (0.4.13) make SchemaTool's whole-ref
  commented `UPDATE` the wrong unit. Generate `SET <field>` from
  official Lua; never apply a `:decoded` payload into a wrapped column.
- Confirm is `REF.field`, not just the ref, so `1223.code` cannot
  silently write `name`.
- Orphan DELETE needs its own confirm token (bare `REF`), since orphan
  findings carry no `field`. The `refuse_reason` gate must check
  kind/class **before** the field check, otherwise orphans (no field)
  were dead-coded into "not a metadata field" and never reached the
  orphan branch.
- DELETE pattern `WHERE query_ref = N AND query_type_a28 BETWEEN 1000 AND 1003`
  matches SchemaTool's own remediation (`schematool_remediate.lua`); no queue
  change needed since orphan findings already carry `ref` + `kind`.
- Anomalies (1000+1003 present on disk) stay refused — "do not auto-delete".

---

## Phase 6 — Docs And Polish

### Goal

Operator docs, index links, extras README, and a review-only smoke path.

### Dependencies

Phases 1–4 minimum. Phase 5 if shipped.

### Entry gate

- [x] TUI + packets usable on Test 40 sqlite (connect probe + `[g]`).

### Work items

- [x] `/docs/H/tools/SCHEMAHELPER.md` (purpose, keys, safety, packets).
- [x] Update extras schematool README + extras README.
- [x] Cross-link from [SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md).
- [x] Test 04 + Test 90 clean.
- [x] `extras/schematool/smoke_schemahelper_queue.sh` — headless smoke against
      checked-in fixture JSON in `test/fixtures/sample_project/`: queue build
      totals (4/1/1/4), field-level finding id `:name` (not `:code`),
      next_ref=1291, `--ref 1148` collision, `--ref 2000` packet write. Driver:
      `lua/schemahelper_smoke_queue.lua`.

### Exit gate / validation

- [x] Test 04 / Test 90 / Test 98 / `mks` green (`mkl` not required;
      no C).
- [x] Plan Status for 0–4 and 6 filled; Phase 5 optional; smoke script
      `smoke_schemahelper_queue.sh` + `lua/schemahelper_smoke_queue.lua`
      complete (marks the `[~]` item in Phase 6 work items).

### Status

Complete (2026-08-23). Operator guide at
[`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).

### Lessons learned

- Failed ping used to be a dead end. `[w]` returns to the wrapper
  picker. SQLite is the usual local target; DB2/PG/MySQL need that
  engine up.
- `abs_file` must run only for SQLite. Applying it to `HYDROTST` turned
  the DB2 database name into a filesystem path and the ping died.

---

## Phase 7 — Optional Follow-Ons

Not required for v1 “done.” Pick up only after Phase 6.

- [x] Catalog live-only extras (done in Phase 1, SchemaTool-side `live_extras[]`).
- [x] Confirmed single-statement catalog DDL apply (nullable / add column)
      with a louder confirm than metadata. SchemaHelper **0.5.5**.
- [ ] `--batch` JSON decisions for non-interactive use.
- [x] Promote-packet helper that copies a stub into Helium (still not a
      full Lua author). SchemaHelper **0.5.5**.
- [x] Group related catalog rows (one table) into a single packet —
      confirmed not required by operator (2026-08-24).
- [ ] Bitfield SchemaTool exit (noted as future in SchemaTool itself).

### Status

Partial. Catalog DDL apply + promote-packet helper complete.
`--batch` and bitfield exit remain deferred; group-catalog-rows confirmed
not needed.

---

## Definition Of Done (v1)

- Operator can pick a SchemaTool wrapper, run metadata and/or catalog, and
  see an analysis dashboard (total / perfect / accepted / subject for
  review + variance classes).
- 1-by-1 review offers: explore, skip for now, accept permanent variance,
  generate a migration, and apply to database (`u`). `[u]` updates one
   metadata field (`REF.field` confirm) or deletes a true orphan ref
   (`REF` confirm; anomalies refused), or applies single-statement catalog
   DDL (`object.column` confirm: nullable→SET/DROP NOT NULL, missing
   column→ADD COLUMN). Requires `--allow-write`; sidecar records `applied`;
   SchemaTool stays read-only.
- Catalog track can surface **live-only extras** (table/column present in
  the DB but not in the expected/disk fold), not just missing/mismatched
  ones — otherwise "generate a migration" only ever fires from metadata
  orphans and the plan's second use case is half-delivered.
- Selections persist in `schemahelper_<design>_<engine>.json` beside the
  SchemaTool artifacts; several engines can share one `--out-dir`.
- Packets get a reserved number and enough detail to author a migration
  later, for **both** metadata orphans and catalog live-only extras.
- SchemaTool is unchanged in spirit (read-only auditor) aside from additive
  JSON.
- No complete `design_NNNN.lua` is invented by the tool. `[m]` writes a **stub**
   (not a full Lua author) that a human completes.
- Docs + luacheck + shellcheck green.

v1 done = Phases 0–4 + 5 (both slices) + 6. Phase 7 partial: catalog DDL
apply + promote-packet helper. Remaining Phase 7 items (`--batch`, group
catalog rows, bitfield exit) stay deferred.

---

## Working Log

### 2026-08-24 — Phase 6 smoke script closure

- Created `lua/schemahelper_smoke_queue.lua` (headless Lua driver) and
  `smoke_schemahelper_queue.sh` (bash wrapper) as the Phase 6 optional smoke.
- Uses `test/fixtures/sample_project/` (not the stale `testdata/schemahelper_queue/`
  sidecar which references `meta:drift:1148:1003:code` — the field-level split
  produces `:name`).
- Verified: totals 4/1/1/4; finding id `meta:drift:1148:1003:name` accepted;
  next_ref=1291 (max disk 1290 + 1); `--ref 1148` collides with `design_1148.lua`;
  `--ref 2000` writes `MANIFEST.json` / `PACKET.md` / `FINDING.json` /
  `DETAIL.txt` / `SUGGESTED.sql` (orphan:1290 packet, then cleaned up).
- shellcheck clean (1 file); luacheck clean (1 file, 0 issues).

### 2026-08-24 — Phase 5 confirmed complete; Phase 7 partial closed

- Phase 5 confirmed complete both slices: one-field metadata `UPDATE` (0.5.0)
  and confirmed orphan `DELETE` (0.5.4). Lint: shellcheck PASS (157 files),
  luacheck clean (0 issues with project config `--no-unused-args`/`--no-self`),
  Test 04 links clean (0 issues).
- Phase 7 partial closed: catalog DDL apply (nullable / add column, louder
  `object.column` confirm) and promote-packet helper both shipped (0.5.5).
  Fixed one new luacheck `unused argument` on `promote_finding` by renaming
  `screen` → `_screen` (matching the existing `_opts` convention).
- Remaining Phase 7 items status:
  - `--batch` JSON decisions: deferred by design (interactive-only lock;
    headless operators use SchemaTool directly).
  - Group related catalog rows into a single packet: operator confirmed not
    required (2026-08-24). Each catalog finding keeps its own packet.
  - Bitfield SchemaTool exit: deferred (SchemaTool-side change, not SchemaHelper).
- v1 Definition of Done is met: Phases 0–4 + 5 (both slices) + 6, plus
  Phase 7 partial (catalog DDL apply + promote).

### 2026-08-24 — Phase 5 orphan DELETE slice

- `[u]` now handles orphan findings (`kind == "orphan"`): builds
  `DELETE FROM queries WHERE query_ref = N AND query_type_a28 BETWEEN 1000 AND 1003`
  (matches SchemaTool remediation at `schematool_remediate.lua:445`).
  Confirm token is bare `REF` (e.g. `1290`); `refuse_reason` checks
  kind/class before the field check so orphans are no longer dead-coded
  into "not a metadata field".
- Anomalies (`both_1000_1003`, class "anomaly 1000+1003") stay refused
  ("do not auto-delete"). Only true orphans delete.
- `[u]` label adapts: "delete orphan ref (type REF)" on the review prompt,
  `[u]elete` in the footer; "Press Enter to delete" on the confirm screen.
- Sidecar records `applied` for both UPDATE and DELETE (no new enum value).
- SQL log: `schemahelper_apply_<ref>_delete_<utc>.sql` for orphans; header
  comments note "does not author a migration".
- SchemaHelper **0.5.4**. `mks` + luacheck clean (one pre-existing warning).

### 2026-08-24 — Phase 7 partial: catalog DDL apply + promote-packet

- `[u]` on catalog `nullable` findings now allowed with `--allow-write`.
  Generates `ALTER TABLE {schema}.{table} ALTER COLUMN {col} SET/DROP NOT NULL`
  per engine. Confirm token is `object.column` (e.g. `accounts.id`) — louder
  than the metadata `REF.field` because it requires typing a real schema object
  name, not a number. DDL shown on the confirm screen before execution.
  `schemahelper_apply.lua:build_catalog_sql`. Refused on `table` (missing),
  `extra_table`, `extra_column` — too structural for a helper ALTER.
- `[u]` on catalog `column` (missing column) findings now allowed. Generates
  `ALTER TABLE {schema}.{table} ADD COLUMN {col} {type}`. Column type is
  looked up from `catalog_expected.json` (the expected fold produced by
  SchemaTool). If the type cannot be resolved, the apply is refused with an
  error message.
- `[u]` label adapts per finding kind: `[u]pdate` (metadata), `[u]elete`
  (orphan), `[u]pply DDL` (catalog nullable/missing-column).
- SQL log for catalog DDL: `schemahelper_apply_ddl_{object}_{column}_{utc}.sql`
  with header "catalog DDL" and caveat "This ALTER statement mutates live DDL
  shape."
- Sidecar records `applied` for catalog DDL (same action name as metadata
  update; note field is `object.column`).
- `[m]` promote-packet helper writes a `design_NNNN.lua` stub into Helium
  migrations from the packet's `SUGGESTED.sql`. Stub is intentionally not a
  full Lua author — human must complete the INSERT pattern. Packet
  `MANIFEST.json` status flips `reserved`→`promoted`. Requires
  `--allow-write`. Only offered when a packet decision exists for the finding.
- SQL is pre-generated before the confirm loop and displayed as "Proposed DDL
  (review before confirming)" on the apply screen.
- SchemaHelper **0.5.5**. `mks` + luacheck + Test 98 clean.

### 2026-08-24 — Live [r] re-audit

- Dashboard and review `[r]` re-probe connect, re-run SchemaTool, ingest
  JSON, keep sidecar decisions. Failed ping skips the run. SchemaHelper
  **0.5.3**.

### 2026-08-24 — Dashboard findings wording

- Queue line is **Findings for review**, not Subject/migrations. Migration
  totals stay ref counts. Classes header matches. SchemaHelper **0.5.2**.

### 2026-08-23 — [u] is update; catalog fold ref

- Review label is `[u] update database` so it is not confused with
  `[a] accept`. Catalog failures now carry the last fold `ref` (the
  migration that last set that column). Review shows table/column and
  that ref. Schema DDL apply is still not offered; packet `[g]` is the
  schema path. SchemaHelper **0.5.1**, SchemaTool **1.8.3**.

### 2026-08-23 — Phase 5 first slice: one-field apply

- `[u]` + `--allow-write` applies one metadata field from official Lua.
  Confirm `REF.field`. Catalog / missing LOAD/APPLY / orphan / anomaly /
  decoded refused. Native client, one transaction, SQL log in
  `--out-dir`. Sidecar `applied`. Reminder: metadata ≠ DDL.
- SchemaHelper **0.5.0**. Modules: `schemahelper_apply.lua`,
  `connect.exec_sql`.

### 2026-08-23 — Explore: Enter decodes brotli line

- Enter on a highlighted line with `BROTLI_DECOMPRESS` / `CRYPTO_DECODE`
  replaces the view with the decoded text (both panes if both sides
  wrap, else one full-width pane). Esc pops back. `keys.page_up` is not
  a terminal.lua name (`pageup` / `pagedown`). SchemaHelper **0.4.14**.

### 2026-08-23 — Explore: one field, full text, decoded item

- Queue splits each drifted field into its own finding. If the stored
  field is brotli/crypto wrapped and the decoded text also differs, a
  second id `…:field:decoded` is added so encoded vs uncompressed are
  not mixed in one explore.
- Explore shows the whole field (no `…` window), line numbers on both
  panes, wrap-aligned blanks, highlight starts on the first differing
  line. SchemaHelper **0.4.13**.

### 2026-08-23 — Review crash after SchemaTool (malformed pattern)

- Enter on the dashboard crashed: `review_content` used
  `line:match("^  %[[esaugn%]")`. `%]` inside a character class does
  not close it, so Lua raised `malformed pattern (missing ']')`.
- Filter key-legend lines with prefix checks, not patterns. **0.4.12**.

### 2026-08-23 — Explore panes, rules, highlight, brotli

- Explore hides the session header. Red hlines join the outer box
  (`├`/`┤`); a red vline splits Migration | Database (`┬`/`┴`).
- Keys are pinned under a footer rule. `j`/`k` move a full-width
  highlight across both panes.
- `BROTLI_DECOMPRESS(CRYPTO_DECODE('…'))` is decoded in Lua (`brotli`
  rock + base64) so the compare shows plaintext, not the blob.
- SchemaHelper **0.4.11**.

### 2026-08-23 — Explore: Migration vs Database; 1000→1003 ignored

- SchemaTool never compares `query_type` (only code/name/summary).
  Helper was treating Lua 1000 vs applied 1003 as a defect. That pair is
  APPLY promotion and is now labeled ignored, not a variance.
- Explore/review explain APPLY vs LOAD and show Migration │ Database
  side-by-side around the first differing byte. Real leftover on 1223
  is still the `code` SQL (one line). SchemaHelper **0.4.10**.

### 2026-08-23 — Explore crash + unreadable expected/actual

- `[e]` crashed: `screen.body` is nil (terminal.lua Screen does not
  store `opts.body` as `self.body`; chrome is named `chrome`). Explore
  now paints inside the existing border with j/k / PgUp/PgDn scroll.
- Review no longer dumps the whole expected/actual JSON on one truncated
  line. It shows first-difference + payload field deltas (`query_type`,
  `name`, `summary`, `code`). Explore shows the line-level field diffs.
- SchemaHelper **0.4.9**. Phase 5 still not started.

### 2026-08-23 — Post-v1 field hardening; hold here

- v1 (0–4 + 6) still closed. Phase 5 not started. SchemaHelper **0.4.8**,
  SchemaTool **1.8.2**. Operator-confirmed: Test 40 wrappers and the
  Lithium custom wrappers (`500courses`, `philement`) complete a
  SchemaTool run and open the dashboard.
- Catalog fold: Lua 5.5 forbids assigning the generic-for variable
  (`schematool_catalog_fold.lua`). After a successful metadata audit,
  catalog fold/probe/compare failure skips catalog (stale catalog JSON
  removed) and keeps metadata exit 0/2/3. Helper opens the dashboard
  with “Catalog track failed; metadata findings kept.”
- Connect: do not infer ping from the filename stem alone. Read
  `--engine` / `--host` / `--database` / `--password-env` from the
  wrapper. Wrappers that compute those with `jq`/locals must be
  **sourced** with `exec` intercepted so the expanded `exec` line and
  password-env are used; leftover parent `${DB_*}` must not win.
  Sidecar engine remains the filename stem (`philement`, `500courses`).
  Passwords never printed / never written to sidecar.
- TUI: `keys.r` is not a terminal.lua key — dashboard `[q]` evaluated
  it and threw. Use `raw == "r"`. Result/log paint: strip tabs/ANSI;
  `utf8cwidth` returns nil for tab and crashed the result screen.
  Connect-fail must not show a leftover SchemaTool log tail.
- Dashboard counts: 364 / 340 perfect / 62 subject is correct as
  **findings** (24 metadata drifts + 37 live extras + 1 nullability).
  Wording vs “migrations” is deferred polish.
- `mks` + Test 98 green on the touched scripts. Next: Phase 5 only if
  `u` is needed; else count wording or live `[r]`.

### 2026-08-23 — Catalog fold crash; degrade to metadata

- P4/P6 remain closed. Phase 5 not started.
- DB2 helper run: metadata compare exit 2 (2 drifts), then catalog fold
  died (`schematool_catalog_fold.lua`: assign to Lua 5.5 const `for`
  variable `name`). SchemaTool exited 1; helper treated the audit as
  failed.
- Fold uses a trimmed local instead of assigning the loop variable.
- After a successful metadata audit, catalog fold/probe/compare failure
  skips the catalog track (warning, stale catalog JSON removed) and
  keeps metadata artifacts / exit 0/2/3. Catalog-only still exits 1.
- SchemaHelper 0.4.4 opens the dashboard when `findings.json` exists and
  shows “Catalog track failed; metadata findings kept.”

### 2026-08-23 — Phase 6 docs; connect dead-end fixed

- Operator guide: [`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).
  Linked from SCHEMATOOL.md, extras READMEs, SITEMAP, STRUCTURE, docs/H
  README, plans/README.
- Connect: only SQLite database names are resolved as file paths. DB2
  `HYDROTST` is a database name, not a path. Failed ping offers `[w]`
  pick another wrapper, `[q]` quit, Enter if artifacts already exist.
- Test 04 / 90 / 98 / `mks` green. Phase 6 closed.

### 2026-08-23 — Phase 4 closed; migration packets

- `[g]` is live for every subject-for-review finding. Next ref is
  `max(disk {design}_NNNN.lua / design_NNNN.lua, reserved packets) + 1`.
  `--ref N` forces a number; collisions with an on-disk migration or an
  existing packet directory are refused.
- Packet files: `MANIFEST.json`, `PACKET.md`, `FINDING.json`,
  `DETAIL.txt`, `SUGGESTED.sql` under
  `<packet-dir>/schemahelper_<design>_<engine>_<ref>/`. Default
  `--packet-dir` is `--out-dir`. Warn if that path is inside the git
  tree; do not redirect to `/tmp`.
- Optional note is an inset chrome prompt (Enter empty, Esc cancel).
  Sidecar records `action=packet` with `ref` / `packet` / `note`. The
  finding leaves subject-for-review. Dashboard lists reserved refs.
- `SUGGESTED.sql` is review-only. Orphans copy the SchemaTool `.mig`
  excerpt when present; catalog live extras get best-effort comments,
  not invented DDL.
- Headless smoke on
  `extras/schematool/test/fixtures/sample_project/migrations`: 1291 then
  1292; `--ref 1148` collides with `design_1148.lua`; `--ref 2000` writes.
- `mks` clean. luacheck clean. Next: Phase 6 docs.

### 2026-08-23 — Plan drafted

- Reviewed [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md),
  [TESTING.md](/docs/H/tests/TESTING.md),
  [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md),
  [SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md), and
  [SCHEMATOOL_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md).
- SchemaTool CLI **1.7.1** already has structured `findings.json`,
  commented remediation, orphan `.mig`, Test 40 wrappers, and `--format
  json`. Catalog `catalog_findings.json` is counts-only; checklist holds
  the rows.
- Recommendation: front-end TUI, not a second auditor; packets not full
  Lua; SchemaTool stays read-only; live-extras detection is the Phase 0
  question that decides how good “create migration from the DB” can be.
- UX lock-in (same day): open on analysis dashboard (total / perfect /
  accepted / subject for review + variance classes); then 1-by-1 with
  explore / skip / accept-permanent / apply / generate-migration.
  Selections persist in `schemahelper_<design>_<engine>.json` beside
  SchemaTool artifacts so many engines can share one folder.
- TUI stack lock-in (same day): **Lua 5.5** (Hydrogen host version) and
  [terminal.lua](https://lunarmodules.github.io/terminal.lua/) via
  `luarocks install terminal` (`luasystem` + `utf8`). Not hand-rolled
  ANSI/`stty`, not ncurses. Rock allows `lua >= 5.1, < 5.6`.
- No code written.

### 2026-08-23 — Plan review pass

- Re-read this plan plus [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md),
  [TESTING.md](/docs/H/tests/TESTING.md), and
  [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) (the latter two are
  N/A here — no `src/`, no Unity — the plan already said so correctly).
- Verified against actual source, not just the plan's prose:
  `schematool_catalog_compare.lua` only walks `exp_names`/`exp_map`
  (expected/disk side) and looks up the matching live entry; it never
  iterates `live_tables`/`live_map` for names absent from the expected set.
  So "live-only extras are invisible today" is confirmed true, not a
  guess. Locked the open Phase 0 question: **build this in Phase 1**
  (`live_extras[]`), not defer to Phase 7 — without it, `g` only ever
  fires from metadata orphans and the plan's stated second use case
  ("turn a useful live DB into a migration") is half-delivered for the
  catalog track, which is the more common way schemas actually drift.
  Confirmed `catalog_findings.json` really is counts-only today (matches
  the plan's Contract gap claim) — `schematool_catalog_compare.lua` writes
  only a `counts` object to `findings_out`; the per-row detail lives in
  `checklist_out`.
- Verified the environment: `lua 5.5.1` + `luarocks 3.9.2` present;
  `luarocks --lua-version=5.5 search terminal` lists `terminal 0.1.0-1`
  with `lua >= 5.1, < 5.6` — genuinely Lua-5.5-compatible, not a
  guess-and-hope dependency. Fetched the published terminal.lua doc index
  and confirmed every widget class this plan names by name
  (`cli.Select`, `cli.Confirm`, `cli.Prompt`, `cli.MultiSelect`,
  `progress.Bar`, `ui.panel.Screen`, `.Bar`, `.KeyBar`, `.Text`,
  `.Confirm`, `.ButtonBar`, `.TabStrip`, `.Set`) actually exists in the
  library. Added a Phase 0 work item to spike the riskiest widgets
  (`Screen`/`KeyBar`/`Text`) in ~30 disposable lines before Phase 2, since
  the rock is young (0.1.0, ~38 downloads) — cheap insurance, not a
  blocker.
- Confirmed both [plans/README.md](/docs/H/plans/README.md) and
  [TODO.md](/docs/H/TODO.md) already link this plan — checked off that
  entry-gate item, it was already true.
- Descoped Phase 5 (apply to database) out of "v1 done." It is the
  highest-risk, highest-review-burden phase (non-RO client session,
  transactions, typed confirm) and is not required by either driving use
  case — Phases 1–4 (review, skip, accept, packet) already let an
  operator triage everything and hand-run SchemaTool's own suggested SQL,
  which is strictly better than today (no more scrolling/renumbering by
  hand). Recommend shipping v1 as Phases 0–4 + 6, then deciding on Phase 5
  from real usage instead of designing a write path speculatively.
- No code written yet; still design-only. Next: begin Phase 0 for real —
  install `terminal` via `luarocks install terminal` (search confirmed
  the rock exists for 5.5; it is not yet installed), run the spike
  script, then start the Phase 1 chunk (launcher + wrapper picker + JSON
  ingest, including the `live_extras[]` additive change).

### 2026-08-23 — Phase 1 closed; invoke + ingest

- Phase 0 splash look already accepted. Phase 1 is interactive, not
  headless: no `--print-summary` / `--print-queue`.
- Locked this slice: always invoke SchemaTool unless `--reuse`; default
  `--out-dir` is the wrapper directory; `--track` only (no SchemaTool
  pass-through); sidecar engine from wrapper basename.
- SchemaTool **1.8.0**: `schematool_catalog_compare.lua` writes
  `failures[]` + `live_extras[]` and extra count fields. Checklist /
  `exit_code` unchanged for live-only extras.
- Added `lua/schemahelper_queue.lua` and fixture
  `extras/schematool/testdata/schemahelper_queue/`. Queue smoke: totals
  4 / perfect 1 / accepted 1 / subject 4 (drift hidden by sidecar).
- Look pass 0.2.2: session header (`wrapper` / `out-dir` / `state` /
  `track` / `log` / `connect`) with a full-width rule; live ping in
  `lua/schemahelper_connect.lua` as soon as a wrapper is known.
  Passwords never printed (env name only). Failed ping skips a new
  SchemaTool invoke. `--reuse` can still load artifacts. App logic
  stays in Lua; `schemahelper.sh` is only the launcher.
- `cli.Select` dropped: it wrote from the cursor and lost the splash
  chrome. Picker is top-aligned inside the body.
- `mks` clean. luacheck clean on the three Lua files.
- Next: Phase 2 review TUI on the existing queue.

### 2026-08-23 — Phase 0 closed; splash shipped

- Reviewed [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md),
  [TESTING.md](/docs/H/tests/TESTING.md),
  [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) (still N/A: no
  `src/`, no Unity). This slice is Bash + Lua; `mks` + luacheck only.
- User look lock: splash first, full-terminal **single red border**,
  multiple (not insane) colors, **Enter only**, versions SchemaHelper
  **0.1.0** + SchemaTool **1.7.1**. Accept = permanent until the
  migration payload changes; later UI must list accepted items and
  allow un-accept.
- Installed `luasystem 0.7.1-1` and `terminal 0.1.0-1` into the user
  Lua 5.5 tree. See Phase 0 lessons for the LuaRocks header / `utf8`
  rock workarounds.
- Added
  [`schemahelper.sh`](/elements/001-hydrogen/hydrogen/extras/schematool/schemahelper.sh)
  and
  [`schemahelper.lua`](/elements/001-hydrogen/hydrogen/extras/schematool/schemahelper.lua).
  `mks` clean. luacheck clean. PTY 24x80 render shows title, facts,
  and "Press Enter to continue".
- Next: look review of the splash, then Phase 1 (flags, picker, ingest,
  `failures[]` / `live_extras[]`).
