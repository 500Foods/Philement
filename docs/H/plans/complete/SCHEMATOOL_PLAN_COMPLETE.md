# SchemaTool Plan — Migration Drift Auditor

## Purpose

Define a gated, phase-by-phase plan for a **standalone operator utility** (Bash + Lua under `extras/schematool/`) with **two audit tracks**:

| Track | Status | Answers |
| ------- | -------- | --------- |
| **v1 Metadata** | **Shipped** (Phases 0–5) | Did LOAD/APPLY happen? Does stored `queries.code`/`name`/`summary` still match on-disk Lua? |
| **v2 Live catalog** | **Shipped** (Phase 7; CLI 1.5.0→1.7.1) | Do **live database objects** (tables, columns, nullability, …) match net applied DDL (hybrid C fold)? |

v1 alone cannot answer “we CREATE’d a column NOT NULL, then later DROP NOT NULL — does live shape look right?” That is the catalog track.

This document is edited as work proceeds. Each phase has Objective, Entry Gate, work items, and Exit Gate.

## How To Use This Document

- Work one phase at a time. Each phase has Objective, Entry Gate, work items, and Exit Gate.
- Mark items `[x]` only when verification has actually passed. Defer with `[~]` plus rationale.
- After each phase, fill Status (date, result, variances) and append discoveries to the Working Log.
- Implementation language preference: **Bash + Lua** (not C / not Hydrogen server). After Bash/Lua changes: `zsh -ic 'mks'` and `zsh -ic` Test 98 luacheck as applicable.

## Resuming Work

CURRENT STATE (as of 2026-08-06): **COMPLETE for operator ship** — CLI **v1.7.1**.
Plan archived at `/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md`.

| Gate | Result |
| ------ | -------- |
| Track A metadata (Phases 0–5) | Shipped |
| Track B catalog (Phase 7) | Shipped |
| Test 40 catalog 1190 (all 7 engines) | **PASS** via `extras/schematool/smoke_test40_catalog.sh` |
| Yugabyte env isolation | Fixed — `YUGABYTE_DB_*` / `--engine yugabytedb` (no ACURANZO fallthrough) |
| Read-only client guards | PG `default_transaction_read_only`; MySQL session RO; SQLite `-readonly` |
| Post-table finding details | Shipped (CLI 1.7.1) — diffs + commented remediation; `--no-detail` |
| shellcheck / luacheck | Clean |
| Phase 6 automated unit tests | Still deferred (optional backlog) |
| Catalog remediation `.sql` | Still deferred (checklist + detail sufficient) |

**Production readiness:** read-only by design; prefer read-only DB role + wrappers/explicit flags; see `/docs/H/tools/SCHEMATOOL.md` Safety.

### Verify anytime

```bash
extras/schematool/smoke_test40_catalog.sh --out-dir /tmp/schematool-t40
# Expect: 7 pass / 0 fail
```

---

## Problem Statement

Hydrogen’s migration system works well for **forward progress**:

1. **LOAD** — run each Lua migration; insert metadata rows into `queries` (forward type `1000`, reverse `1001`, diagram `1002`).
2. **APPLY** — execute the stored forward `code`; the migration SQL itself flips that row to type `1003` (applied).

What it does **not** do is detect **historical edit drift**: someone changes an already-shipped `acuranzo_NNNN.lua` instead of adding `NNNN+1`. Existing databases keep the old LOAD/APPLY payload in `queries.code`. Later migrations that `ALTER` earlier tables can mask live-schema symptoms while the stored migration text is still wrong.

SchemaTool **v1** surfaces that class of problem with a small per-migration **metadata** checklist.

**v2 / Phase 7** addresses the harder, higher-value problem: **live object fidelity** — whether the running catalog matches the net effect of applied migrations (including later ALTERs that change nullability, columns, etc.).

---

## Two-track product model (locked 2026-07-29)

```text
                         ┌──────────────────────────┐
   Lua tree + DB ───────►│ schematool (read-only)   │
                         └────────────┬─────────────┘
                ┌─────────────────────┼─────────────────────┐
                ▼                     ▼                     ▼
         Track A (v1)          Track B (v2 / P7)      Artifacts
         queries 1000–1003     information_schema     tables + .sql
         code/name/summary     DESCRIBE / pg_catalog  (+ .mig orphans)
         LOAD/APPLY/match      live tables/cols/nulls
```

| | Track A — Metadata (shipped) | Track B — Live catalog (shipped) |
| -- | ------------------------------ | ------------------------------- |
| Input | Lua → expected payloads; `SELECT` from `queries` | Lua/DDL fold and/or applied `code`; engine catalog APIs |
| Signal | Edited-in-place migration files; missing LOAD/APPLY | Missing tables; wrong nullability; extra/missing columns |
| Example | 1280/1281 mail seed `code` ≠ Lua | 1190 made `password_hash` nullable — live col must allow NULL |
| False expectation | “Live CREATE matches migration 1005 alone” | “Lua file text equals `queries.code`” (that’s Track A) |
| CLI sketch | default full audit | additive **`--catalog`** (metadata default) |

**Priority:** Track B is the **next implementation focus**. Track A is maintenance/fixes only unless a regression blocks operators.

### Catalog access model — targeted probes, not bulk dump (locked 2026-08-02)

**Operator concern:** a full-schema catalog dump is costly and wasteful when auditing one migration (e.g. 1190 → only `accounts.password_hash`).

| Approach | When | Cost |
| ---------- | ------ | ------ |
| **Targeted probes (default)** | Catalog compare / acceptance | Issue engine-native queries **only for tables (and optionally columns) in scope** — driven by expected-shape set, `--only-tables`, or migration ref focus |
| **Full dump (`--dump-catalog`)** | Debug / golden fixtures | One-shot JSON of schema objects; optional, not the hot path |

**How probes work (interactive clients, same as Phase 3):**

- Still **native clients** (`sqlite3`, `psql`, `mysql`, `db2`) + credentials — connect and run **read-only SQL/PRAGMA**, not `pg_dump` / file export of the whole DB.
- “Interactive” here means **narrow SELECTs against catalog views** (or `PRAGMA table_info('accounts')`), not a human REPL session.
- Scope drivers (combine as implemented):
  1. Expected-shape builder lists tables/columns that matter (hybrid C from applied migrations).
  2. `--only-tables a,b` / skip lists shrink further.
  3. Optional later: `--from`/`--to` or `--focus-ref 1190` to limit which migrations feed expected shape (and thus which live probes run).
- SQLite 1190 path: `PRAGMA table_info(accounts)` → `password_hash.notnull == 0` — one pragma, not full `sqlite_master` walk unless needed for presence checks.

**Do not:** scan product row data; dump entire `information_schema` when only one table is in scope; treat bulk dump as required before compare.

---

## Background (Verified From Codebase)

### Lua migration layout (Helium / Acuranzo)

| Item | Location / pattern |
| ------ | -------------------- |
| Migration set | `/elements/002-helium/acuranzo/migrations/` |
| Numbered scripts | `acuranzo_1000.lua` … currently through ~`1282` (~283 numbered files) |
| Shared engine | `database.lua` + `database_postgresql.lua`, `database_mysql.lua`, `database_sqlite.lua`, `database_db2.lua` |
| Contract | Each file `return function(engine, design_name, schema_name, cfg) … return queries end` |
| Offline SQL gen (existing) | `/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua` + `get_migration.sh` |
| Diagram / decode (existing) | `/elements/001-hydrogen/hydrogen/tests/lib/get_diagram.sh` (base64 + brotli unwrap) |

### Query types (`database.lua`)

| Type | Name | Meaning |
| ------ | ------ | --------- |
| 1000 | `forward_migration` | Loaded, not yet applied (or re-loaded after reverse) |
| 1001 | `reverse_migration` | Reverse payload |
| 1002 | `diagram_migration` | ERD JSON payload |
| 1003 | `applied_migration` | Was 1000; APPLY succeeded and flipped type |

Unique key on `queries`: `(query_ref, query_type_a28)`.

### LOAD vs APPLY (Hydrogen)

| Phase | Code | Effect |
| ------- | ------ | -------- |
| LOAD | `src/database/migration/execute_load.c`, `lua_execute_load_metadata` → `run_migration` | Executes generated **INSERT** SQL into `queries` only. No product schema DDL yet. |
| APPLY | `src/database/dbqueue/lead_apply.c` | Reads QTC entry `ref=N, type=1000`, splits on `-- SUBQUERY DELIMITER`, runs statements in a transaction. Final statement typically `UPDATE … SET query_type_a28 = 1003 WHERE query_ref = N AND type = 1000`. |
| Status | `database_bootstrap.c` | `latest_loaded` = max `query_ref` with type 1000; `latest_applied` = max with type 1003. After APPLY, type 1000 is gone for that ref — **applied rows are type 1003 only**. Bootstrap does **not** cache type 1003 into QTC (status only), but the row remains in the table with `code` intact. |

### What is stored in `queries.code`

- Lua wraps migration bodies in `[=[ … ]=]`; `database.lua` `replace_query` expands `${…}` macros, then base64-encodes (and brotli-compresses if >1KB) with engine wrappers (`BASE64_START`/`COMPRESS_START`, etc.).
- The **INSERT evaluates** those wrappers, so the **persisted `code` column is plain SQL text** (decoded), not the base64 literal.
- Forward `code` is multi-statement: real DDL/DML + delimiter + status `UPDATE` to type 1003.
- Comparing “Lua vs DB” should target this **decoded forward body** (and optionally reverse/diagram), not the outer INSERT wrapper, unless a separate “LOAD SQL match” mode is added later.

### Bootstrap shape (for DB reads)

Typical config:

```sql
SELECT query_id id, query_ref ref, query_status_a27 status,
       query_type_a28 type, query_dialect_a30 engine,
       query_queue_a58 queue, query_timeout timeout,
       name, code query
FROM [schema.]queries
WHERE query_status_a27 = 1
ORDER BY query_type_a28 DESC;
```

SchemaTool can use a narrower query (migration types only) for speed.

### Credentials pattern

Operator env already used for Acuranzo-style DBs (example from shell config):

- `ACURANZO_DB_NAME`, `ACURANZO_DB_TYPE`, `ACURANZO_DB_HOST`, `ACURANZO_DB_PORT`, `ACURANZO_DB_USER`, `ACURANZO_DB_PASS`

SchemaTool should accept **explicit CLI flags** with optional **env fallbacks**, never hard-code secrets, never print passwords.

### Related Hydrogen docs / tests

- Migration subsystem: `/elements/001-hydrogen/hydrogen/src/database/migration/`
- Migration performance tests: `test_32`–`test_38`
- Test helper docs: `/docs/H/tests/migrations.md`, `get_migration.md`
- Diagram path already solved multi-engine decode of large `code` fields — reuse patterns from `get_diagram.sh`

---

## Home In The Repo

**Locked:** `/elements/001-hydrogen/hydrogen/extras/schematool/`

Rationale: operator utility alongside other extras (`hydrogen_flush.sh`, `comment-analysis.sh`, etc.); not a blackbox test and not production `src/`. Entry point: `extras/schematool/schematool.sh` (or `schematool` wrapper).

Dependency (same as Hydrogen test framework): system `tables` binary (`command -v tables`) — layout JSON + data JSON → ANSI table. Pattern already used in `tests/lib/framework.sh`, `coverage_table.sh`, `log_output.sh`, `extras/comment-analysis.sh`.

## What This Tool Is

- An **offline / side-channel auditor** for migration **metadata fidelity**.
- Input: migration folder (Lua), engine, design name, schema, DB connection parameters.
- **Three primary outputs** (see **Outputs** below):
  1. Console checklist rendered with the Hydrogen **`tables`** mechanism.
  2. A **`.sql` remediation file** of suggested fix statements, **all commented out**.
  3. A **`.mig` capture file** for orphan DB migration rows (in DB, not on disk) so operators can rebuild a new numbered migration if those changes should be kept.
- Runnable without starting Hydrogen (**native DB clients** + Lua — see **DB access model**).
- Multi-engine client adapters (v1): **all four** `database.lua` engines — PostgreSQL, MySQL, SQLite, DB2. MariaDB → mysql client; Cockroach/Yugabyte → postgresql dialect/client.
- Exit codes suitable for CI / ops scripts (0 = clean, non-zero = drift or hard failure).

## What This Tool Is Not

- **Not** a replacement for Hydrogen LOAD/APPLY.
- **Not** an auto-executor of remediation SQL — the `.sql` file is a **review artifact**; every statement starts commented so nothing runs if the file is fed to a client by mistake without deliberate uncommenting.
- **Not** a silent writer against the live DB (v1 connects **read-only** via normal clients).
- **Not** `pg_dump` / `mysqldump` / full-database export analysis — we never bulk-export the whole catalog for v1 metadata audit.
- **Not** (v1) a full live schema vs ideal-schema diff engine — `DESCRIBE` / `information_schema` is **optional later** (Phase 7 / catalog track). v1 still does **not** prove live table shape equals migration N in isolation (later ALTERs intentionally change earlier objects).
- **Not** a row-data auditor — we do **not** SELECT * huge product tables; empty or enormous tables are fine to skip for data content.
- **Not** C code inside `src/`; not a new Hydrogen subsystem or REST endpoint (unless a later plan says otherwise).
- **Not** responsible for QueryRef product SQL drift (types 0–11) except insofar as those rows were **inserted by** a migration’s forward `code` and that full `code` blob is compared.

---

## DB access model (native clients — not dump-and-analyze)

**Yes — use normal database clients and credentials.** Phase 3 “adapters” are thin wrappers around `psql`, `mysql`, `sqlite3`, and `db2`/`clpplus` (same tools operators already use). They are **not** a painful full-DB dump pipeline.

| Layer | What we run | Why |
| ------ | ------------- | ----- |
| **v1 metadata (required)** | Read-only `SELECT` on `[schema.]queries` for types `1000–1003` (ref, type, name, summary, code) | Answers LOAD/APPLY presence + **text fidelity** vs Lua. This is the only signal for “someone edited `acuranzo_NNNN.lua` after ship.” |
| **Live catalog (optional later)** | Engine-native catalog probes — e.g. DB2 `DESCRIBE TABLE`, PG `information_schema` / `\d`, MySQL `SHOW CREATE TABLE` / `DESCRIBE`, SQLite `PRAGMA table_info` | Answers “does the **live** object shape look sane?” Useful ops check; **orthogonal** to metadata drift. |

### Why metadata SELECT is not “dump-and-analyze”

- Scope is **one table** (`queries`), **migration types only**, not the whole database.
- Large product tables are never scanned for row content; missing seed data is out of scope and OK.
- Cost scales with **number of migration refs** (~hundreds of `code` blobs), not with fact-table size.
- Implementation is still “talk to the engine with the normal client” — same credentials as Acuranzo env (`ACURANZO_DB_*` / flags).

### Why DESCRIBE alone cannot replace v1

| Scenario | `queries` text compare | Live `DESCRIBE` |
| ---------- | ------------------------ | ----------------- |
| Lua edited after APPLY; later migration already ALTERed live table | **Detects** (code/name/summary drift) | Often **misses** (live shape still “looks fine”) |
| APPLY never ran | Detects missing 1003 | May show object missing or stale |
| Live table hand-patched; `queries.code` still matches Lua | Pass | Can **detect** shape drift (catalog track) |
| Cumulative ALTERs after CREATE | N/A by design | Needs **folded** expected schema through max APPLY — hard; false positives |

**v1 ships metadata path.** Catalog/`DESCRIBE` is welcome as a **second mode** once metadata is solid: run per schema, allow **skip lists** for huge or uninteresting tables, never require full table scans. Prefer one-schema-at-a-time if that keeps early iterations simple.

### Practical client notes (Phase 3)

- Prefer **SQL** that works in scripts (`SELECT … FOR JSON` / `-t -A` / `sqlite3 -json`) over interactive-only pretty-printers; use `DESCRIBE`-class commands where they are the cleanest catalog API for that engine.
- Connection: CLI flags + `--password-env`; never log secrets.
- Read-only intent: SELECT/catalog only; no DDL/DML from SchemaTool itself.

---

## Core Checklist (Per Migration Ref)

For each discovered numbered migration `design_NNNN.lua` with ref `N`:

| # | Check | Pass condition (v1) |
| --- | -------- | --------------------- |
| 1 | **LOAD performed?** | A `queries` row exists with `query_ref = N` and `query_type_a28 IN (1000, 1003)` (1003 implies it was loaded then applied). |
| 2 | **Loaded match Lua?** | Stored forward payload for that ref (type 1000 if present, else type 1003) **matches** expected forward **`code` + `name` + `summary`** from current Lua (loose/strict normalization). |
| 3 | **APPLY performed?** | A `queries` row exists with `query_ref = N` and `query_type_a28 = 1003`. |
| 4 | **Applied match Lua?** | Stored type-1003 **`code` + `name` + `summary`** match expected from Lua (same comparison as #2 when only 1003 exists). |

### Status matrix (informative)

| LOAD row | APPLY row | Typical meaning |
| ---------- | ----------- | ----------------- |
| none | none | Never loaded (DB behind AVAIL, or empty / wrong schema). |
| 1000 | none | Loaded, waiting APPLY (or APPLY failed mid-way — rare). |
| none | 1003 | Normal healthy applied migration. |
| 1000 | 1003 | Should not happen for same ref (unique on ref+type allows both only if both types exist; forward and applied are different types — **both can exist only if something re-INSERTed 1000 without removing 1003**; flag as anomaly). |
| match fail | any | Lua edited after LOAD/APPLY, or generator/engine/schema mismatch. |

### Later-migration caveat (must document in UX)

If migration 1050 creates `t`, and 1120 adds a column to `t`:

- Live `information_schema` for `t` will **not** match 1050’s CREATE alone.
- SchemaTool v1 **does not** fail check #2/#4 for that reason; it compares **stored migration text** to **Lua text**, which is the correct signal for “someone edited 1050 after the fact.”
- Optional later phase may offer **cumulative expected schema** vs live catalog; that is a different product.

---

## Locked Decisions (Phase 0)

| Topic | Decision |
| ------ | ---------- |
| Expected extraction | **Lua only** — same path as test_31 / `get_migration.lua`: `require('database')`, migration fn, `database:run_migration`, then **decode** INSERT field wrappers (base64 + optional brotli) like `get_diagram.sh`. No `database.lua` change required for v1. Implemented: `lua/schematool_expect.lua` + `--emit-expected`. |
| Content match scope | **`code` + `name` + `summary`** (forward row; reverse/diagram optional flags). |
| Orphan DB refs | Emit **`.mig` artifact** (structured capture of orphan rows: ref, type, name, summary, code) so operators can promote wanted changes into a new `design_NNNN.lua`. Also list in checklist Notes / exit 3. Commented DELETE guidance may still appear in `.sql`. |
| Engines (clients) | **PG + MySQL + SQLite + DB2** all required for Phase 3 exit. Aliases: mariadb→mysql; cockroach/yugabyte→postgresql. Native clients only (not full-DB dump tools). |
| Default normalize | **`loose`** (`strict` via `--normalize`). |
| Drift remediation | Commented **`UPDATE … code/name/summary`** + explicit **prefer new forward migration** guidance (updating metadata does not replay DDL). |
| Phase 0 PG spike | **Deferred** into early Phase 2 (user chose skip-to-Phase-1). Clients verified present: `tables`, `psql`, `mysql`, `sqlite3`, `db2`/`clpplus`. |
| Password | `--password-env VAR` preferred; never print secrets; never embed in `.sql` / `.mig`. |

## Outputs

SchemaTool always produces **checklist + remediation `.sql`** (plus exit code); **`.mig`** when orphans exist (or always empty stub under `--mig-out`). Console is primary for humans; SQL/MIG are review artifacts.

### Output 1 — Console checklist via `tables`

Use the same **`tables`** program as the Hydrogen build/test system:

```text
tables <layout.json> <data.json>  →  ANSI table on stdout
```

Implementation pattern (match `extras/comment-analysis.sh` / `tests/lib/coverage_table.sh`):

1. Build a **layout JSON** (title, subtitle, footer, column defs: header/key/datatype/justification/summary).
2. Build a **data JSON** array of row objects (one per migration ref).
3. Invoke `"${TABLES}" "${layout}" "${data}"` (fail hard if `tables` missing — same showstopper style as `framework.sh`).
4. Optional: also write the rendered table (or raw layout+data) under `--out-dir` for logs.

**Draft columns:**

| Column | Key | Notes |
| -------- | ----- | -------- |
| Ref | `ref` | Migration number |
| File | `file` | `acuranzo_1148.lua` |
| LOAD | `load` | Y / N |
| L.match | `load_match` | Y / N / `-` |
| APPLY | `apply` | Y / N |
| A.match | `apply_match` | Y / N / `-` |
| Notes | `notes` | Short reason / anomaly |

Title/subtitle/footer should carry design, engine, schema, database, disk AVAIL, DB max APPLY, timestamp — same spirit as other Hydrogen tables (`{BOLD}{WHITE}…{RESET}` tokens supported by `tables`).

Optional second table (footer summary or separate render): counts of drift / missing LOAD / missing APPLY / anomalies.

`--only-failures` filters the data JSON before render.  
`--format json` may dump the data JSON instead of/in addition to `tables` (layout still available for scripting). Default human path is **`tables` only** — not a hand-rolled ASCII grid.

### Output 2 — Remediation `.sql` (all statements commented)

Path draft: `--sql-out PATH` or default  
`./schematool_<design>_<engine>_<timestamp>.sql`  
(or under `--out-dir`).

**Purpose:** list the SQL an operator would need to run so the **database migration metadata (and, where we can express it, related state)** lines up with the **current Lua** set. This is a **proposal file**, not an applied migration.

**Hard rule:** every executable statement is emitted **commented out** (line comments appropriate to the engine, default `-- …`). File header states:

- Generated by SchemaTool; read-only audit; **do not pipe to psql/mysql unedited**.
- How to use: review, uncomment selected blocks, run in a transaction / maintenance window.
- Engine, schema, design, generation time, source migration dir, normalization mode.

**Per-finding blocks (draft taxonomy):**

| Finding | Suggested commented SQL (conceptual) |
| --------- | -------------------------------------- |
| Missing LOAD (on disk, no 1000/1003) | Comment block: “run Hydrogen LOAD for ref N” **or** paste expected INSERT(s) generated from Lua (same as LOAD phase would insert), fully commented. Prefer documenting Hydrogen path when INSERT generation is huge/fragile. |
| Loaded not applied (1000, no 1003) | Comment: “run Hydrogen APPLY through ref N” or note that APPLY must execute stored `code` (do not invent partial DDL). |
| Content drift (1000 and/or 1003 `code` ≠ Lua) | Commented `UPDATE … SET code = …` (and name/summary if in scope) for the affected type row(s), using properly escaped expected body; and/or commented `DELETE` + re-INSERT of metadata row. Prefer **UPDATE code** when row exists. |
| Orphan DB ref (in DB, not on disk) | Commented `DELETE FROM …queries WHERE query_ref = N AND query_type_a28 IN (1000–1003)` — dangerous; header must warn. |
| Anomaly both 1000 and 1003 | Commented cleanup guidance (keep 1003, delete stray 1000, or vice versa) with rationale. |
| Clean ref | Optional silent skip, or single-line `-- OK: 1148` (`--include-ok-comments`). |

**Commenting style (example):**

```sql
-- =============================================================================
-- SchemaTool remediation (NOT EXECUTED)
-- design=acuranzo engine=postgresql schema=demo
-- Generated: 2026-07-29T22:00:00Z
-- Rule: Uncomment deliberately. Prefer Hydrogen LOAD/APPLY when possible.
-- =============================================================================

-- ---------------------------------------------------------------------------
-- Ref 1148: forward code DRIFT (type 1003)
-- Disk: acuranzo_1148.lua  |  check: L.match=N A.match=N
-- ---------------------------------------------------------------------------
-- UPDATE demo.queries
--    SET code = $schematool$
-- ... expected forward body ...
-- $schematool$
--  WHERE query_ref = 1148
--    AND query_type_a28 = 1003;

-- ---------------------------------------------------------------------------
-- Ref 1282: missing LOAD and APPLY
-- Prefer: start Hydrogen with AutoMigration against this DB (LOAD then APPLY).
-- Optional manual LOAD insert (generated) left commented below:
-- ---------------------------------------------------------------------------
-- INSERT INTO demo.queries ( ... ) VALUES ( ... );
```

**Safety constraints for SQL generator:**

- No uncommented DML/DDL in the file body (header may be pure comments too).
- Use engine-safe dollar-quoting / escaping for large `code` payloads.
- Never embed passwords or connection strings with secrets in the `.sql` file.
- When the correct fix is “let Hydrogen APPLY,” emit **guidance comments only**, not a hand-expanded copy of hundreds of DDL statements that might double-apply — especially for missing APPLY on already-partially-correct live schema.
- Drift on **already-applied** rows: updating `queries.code` alone does **not** replay DDL against live tables; the file must say so. True schema repair may still need a **new** forward migration; SchemaTool can only align **stored metadata** unless a later phase adds live-schema repair (out of v1 scope).

### Output 3 — Orphan capture `.mig`

Path draft: `--mig-out PATH` or default under `--out-dir`:  
`schematool_<design>_<engine>_<timestamp>.mig`

**Purpose:** when the DB has migration refs **not** present on disk, dump those rows into a side-car file operators can use to **author a new Lua migration** (or decide to DELETE via commented `.sql`). Not executed by SchemaTool.

**Draft contents (text, review-friendly):** header metadata + one block per orphan ref/type with name, summary, and code (same fields as match scope). Prefer plain text / Lua-comment style over binary. Exact format locked in Phase 4 when first orphan path is implemented.

### Output relationship

```text
                    ┌─────────────────────────┐
  Lua tree + DB ──► │ schematool (read-only)  │
                    └───────────┬─────────────┘
                                │
          ┌─────────────────────┼─────────────────────┐
          ▼                     ▼                     ▼
 stdout: tables(layout,data)  file: *.sql           file: *.mig
 human checklist              commented remediation orphan capture
```

### Exit codes (draft)

| Code | Meaning |
| ------ | --------- |
| 0 | All selected disk migrations pass all four checks; SQL file may still be written (header-only or OK comments). |
| 1 | Usage / connection / Lua load / missing `tables` hard error. |
| 2 | Soft audit failure: missing LOAD/APPLY and/or content mismatch (SQL file has remediation blocks). |
| 3 | Anomalies (duplicate 1000+1003, orphan DB refs, etc.) with or without #2. |

## Proposed UX

### CLI (draft)

```bash
# From repo:
extras/schematool/schematool.sh \
  --migrations /path/to/acuranzo/migrations \
  --design acuranzo \
  --engine postgresql \
  --schema demo \
  --database "$ACURANZO_DB_NAME" \
  --host "$ACURANZO_DB_HOST" \
  --port "$ACURANZO_DB_PORT" \
  --user "$ACURANZO_DB_USER" \
  --password-env ACURANZO_DB_PASS \
  [--from 1000] [--to 1282] \
  [--only-failures] \
  [--out-dir ./schematool-out] \
   [--sql-out ./schematool-remediation.sql] \
   [--mig-out ./schematool-orphans.mig] \
   [--format tables|json|both] \
   [--normalize loose|strict] \
   [--include-reverse] [--include-diagram] \
   [--include-ok-comments]
```

Notes:

- Prefer `--password-env VAR` over `--password` on argv (avoids `ps` leakage).
- `--migrations` is the folder containing `database.lua` and `design_NNNN.lua`.
- Design name defaults from folder or `--design` (Hydrogen `extract_migration_name` / payload prefix semantics).
- SQLite: `--database` is file path; host/port/user ignored.
- Default `--format tables` (Hydrogen `tables` binary). `json` writes checklist data for scripting; `both` prints table and writes `checklist.json` under `--out-dir`.
- Default SQL path: `--out-dir/schematool_<design>_<engine>_<utc>.sql` if `--sql-out` omitted; always write SQL file unless `--no-sql`.
- Default MIG path: under `--out-dir` when orphans exist; `--mig-out` forces path. Phase 1 accepts `--mig-out` but does not write orphans yet.

### Example console report (via `tables`, illustrative)

```text
SchemaTool — acuranzo / postgresql / demo @ test
Disk AVAIL 1282 · DB APPLY 1280 · generated 2026-07-29

Ref   File                 LOAD  L.match  APPLY  A.match  Notes
1000  acuranzo_1000.lua    Y     Y        Y      Y
1001  acuranzo_1001.lua    Y     Y        Y      Y
…
1148  acuranzo_1148.lua    Y     N        Y      N        forward code drift
1281  acuranzo_1281.lua    Y     Y        N      -        loaded, not applied
1282  acuranzo_1282.lua    N     -        N      -        on disk only

283 rows · 2 drift · 1 loaded-not-applied · 1 missing-load
SQL: ./schematool-out/schematool_acuranzo_postgresql_….sql (all statements commented)
```

(Actual glyphs/colors come from `tables` theme tokens, not a custom renderer.)

## Architecture

```text
elements/001-hydrogen/hydrogen/extras/schematool/
  schematool.sh                 # CLI entry — help + parameter handling + dispatch
  lib/
    schematool_init.sh          # dependency checks + command path resolution
    schematool_runners.sh       # db/Lua adapter wrappers (query/catalog adapters, Lua extractors)
    schematool_audit.sh         # audit mode dispatch + orchestration (dump/emit/audits)
    schematool_render.sh        # tables rendering + render dispatch
  lua/
    schematool_discover.lua     # disk file discovery → checklist JSON  (Phase 1 ✓)
    schematool_expect.lua       # get_migration + decode → expected payloads  (Phase 2 ✓)
    schematool_normalize.lua    # loose/strict normalize  (Phase 4 ✓)
    schematool_compare.lua      # join + four checks + findings JSON  (Phase 4 ✓)
    schematool_remediate.lua    # commented .sql + plain-text .mig  (Phase 4 ✓)
    schematool_catalog_fold.lua # hybrid-C fold applied type-1003 DDL  (Phase 7 ✓)
    schematool_catalog_compare.lua  # expected ⟷ live catalog  (Phase 7 ✓)
  db/
    query_pg.sh                 # psql json_agg  (Phase 3 ✓)
    query_mysql.sh              # mysql JSON_ARRAYAGG  (Phase 3 ✓)
    query_sqlite.sh             # sqlite3 json_group_array  (Phase 3 ✓)
    query_db2.sh                # db2 EXPORT LOBS + python parse  (Phase 3 ✓)
    catalog_sqlite.sh           # PRAGMA table_info probes  (Phase 7 ✓)
    catalog_pg.sh               # information_schema filtered  (Phase 7 ✓)
    catalog_mysql.sh            # information_schema filtered  (Phase 7 ✓)
    catalog_db2.sh              # SYSCAT.COLUMNS  (Phase 7 ✓)
    common.sh                   # qualify helpers (optional)
  testdata/
    expected_pg_demo_1000_1002.json
```

**Reuse:**

- `tables` for console (mandatory dependency, same as test suite).
- Same `package.path` / `require('database')` approach as `get_migration.lua` / **test_31** (run with cwd = migrations dir).
- Native clients only for DB I/O (see **DB access model**).
- Decode lessons from `get_diagram.sh` only if any path still returns wrapped code (should not for normal SELECTs of stored column).
- Do **not** fork Hydrogen C migration code.
- Comment-analysis / coverage_table layout+data JSON style for maintainability.

### Expected payload extraction (Lua) — implemented (Phase 2)

For each `design_N.lua`:

1. cwd / `package.path` = migrations dir (same as `get_migration.sh`).
2. `require('database')`, `require(design_N)`, call with `(engine, design, schema, database.defaults[engine])` — **pass defaults by reference** (migrations set `cfg.TABLE` etc. in place; a shallow copy breaks `${TABLE}` expansion).
3. `database:run_migration(...)` → LOAD SQL string (identical to test_31).
4. Split on `-- QUERY DELIMITER`; keep **INSERT INTO …queries** parts only (bootstrap bare DDL parts are not the stored forward body).
5. Parse `AS query_type_a28` / `code` / `name` / `summary` expressions; decode first single-quoted base64; if expression contains brotli, `require('brotli')` decompress — same wrapper family as `get_diagram.sh`.
6. Result matches **persisted** DB column text (after LOAD evaluates wrappers). Trailing newline: normalize on compare (Phase 4 loose).

**No `database.lua` change.** Diagram `code` is often a short stub (“JSON Table Definition in collection”); real ERD may live in `collection` — forward match is primary for v1.

### DB metadata row contract (native client SELECT result)

Normalized intermediate format (JSON lines or single JSON array) — produced by engine client wrappers, **not** by `pg_dump`/`mysqldump`:

```json
{
  "query_ref": 1148,
  "query_type": 1003,
  "name": "…",
  "summary": "…",
  "code": "CREATE TABLE …\n-- SUBQUERY DELIMITER\nUPDATE …"
}
```

Only types `1000–1003` required for v1. Include `name` and `summary` (match scope).

### Comparison / normalization

Minimum **loose** normalizer:

- Unify newlines to `\n`
- Trim trailing whitespace per line
- Collapse runs of blank lines
- Optional: ignore differences only inside the trailing status `UPDATE … query_type_a28 = …` if engine quoting differs (prefer fixing expected generation instead)

**strict:** exact byte match after newline unify.

Emit short unified diff on failure (`diff -u` or Lua) gated by `--verbose`.

---

## Implementation Phases

### Phase 0 — Spike and decisions

**Objective:** Lock interface, comparison semantics, `tables` layout sketch, and remediation SQL taxonomy with zero production code commitment beyond notes in this plan.

**Entry gate:** This plan accepted as the working doc.

**Work items:**

- [x] Install path locked: `extras/schematool/` (not `tests/lib`).
- [x] Dual outputs locked: `tables` console + fully commented remediation `.sql`; **plus** orphan `.mig` capture.
- [x] Spike: PG equality on known-good DB — completed in Phase 2 (refs 1000/1001/1002/1148). Clients confirmed on maintainer host.
- [x] Expected-extraction strategy: **Lua / test_31 / get_migration style** (see Locked Decisions).
- [x] Engine clients available: `psql`, `mysql`, `sqlite3`, `db2`, `clpplus`, `tables`.
- [x] Checklist layout columns sketched (Ref/File/LOAD/L.match/APPLY/A.match/Notes) — implemented in Phase 1.
- [x] Password/env: `--password-env`; schema empty OK for SQLite.
- [x] Sample command (disk): see Phase 1 exit / Working Log.
- [x] Remediation taxonomy: UPDATE code+name+summary + new-migration note; missing APPLY → Hydrogen guidance; orphans → `.mig` + optional commented DELETE.

**Exit gate:** Working Log has decisions; no unresolved “how do we get expected code?” or output-format ambiguity.

**Status:** complete (2026-07-29) — formal equality spike carried into Phase 2

---

### Phase 1 — CLI skeleton + discovery (no DB)

**Objective:** Runnable tool under `extras/schematool/` that lists disk migrations via `tables` and writes a header-only commented `.sql`.

**Entry gate:** Phase 0 exit green.

**Work items:**

- [x] Create `extras/schematool/schematool.sh` with `--help`, required args validation, `shellcheck`-clean.
- [x] Require `tables` (showstopper if missing), same pattern as `framework.sh`.
- [x] Lua: discover `design_NNNN.lua`, sort by ref, honor `--from`/`--to` (`lua/schematool_discover.lua`).
- [x] Emit checklist **data JSON** (DB columns `-`) + **layout JSON**; render with `tables` to stdout.
- [x] Write remediation `.sql` with generation header only (all comments); path via `--sql-out` / `--out-dir`.
- [x] `mks` / luacheck clean.
- [x] Flags stubbed for later phases: `--mig-out`, `--normalize`, `--format`, connection flags, etc.

**Exit gate:**  
`extras/schematool/schematool.sh --migrations … --design acuranzo --engine postgresql --schema x --database x --dry-disk` prints a `tables` checklist of all refs and creates a commented SQL stub.

**Status:** complete (2026-07-29) — verified 283 Acuranzo refs; ranged 1000–1005 smoke OK

---

### Phase 2 — Expected payload extraction (Lua only)

**Objective:** For each migration, produce expected forward (and optional reverse/diagram) plain-text **`code` + `name` + `summary`** for a given engine/schema.

**Entry gate:** Phase 1 done.

**Work items:**

- [x] Implement `lua/schematool_expect.lua` using test_31 / `get_migration` pattern (`require('database')` + migration fn + `run_migration` + decode).
- [x] Decode INSERT wrappers (base64/brotli) so expected matches persisted DB column text — **no** `database.lua` change.
- [x] PG equality spike: refs **1000** (bootstrap INSERT type 1003), **1001** (CREATE/brotli), **1002**, **1148** (QueryRef) — code+name+summary match `demo.queries` after trailing-newline normalize.
- [x] Golden fixture hashes: `testdata/expected_pg_demo_1000_1002.json`.
- [x] Unsupported engine / missing module → clear Lua errors.
- [x] Macros via shared `replace_query` (same as Hydrogen LOAD).
- [x] CLI `--emit-expected [PATH]`.

**Exit gate:** Fixture tests pass; output stable across two runs; known-good PG sample shows equality for chosen refs.

**Status:** complete (2026-07-29)

---

### Phase 3 — Database client adapters (metadata SELECT)

**Objective:** Read-only fetch of migration rows from a live DB via **native clients** into the intermediate JSON contract (not full-DB dump tools).

**Entry gate:** Phase 2 done.

**Work items:**

- [x] PostgreSQL via `psql` (`json_agg` / `json_build_object`, `-v ON_ERROR_STOP=1`).
- [x] MySQL/MariaDB via `mysql` (`JSON_ARRAYAGG` / `JSON_OBJECT`).
- [x] SQLite via `sqlite3` (`json_group_array` / `json_object`; CAST text; hydrodemo.sqlite).
- [x] DB2 via `db2 EXPORT … LOBS TO` + Python DEL/LOB parse (CLOBs too large for VARCHAR cast).
- [x] Schema-qualified table (`demo.queries`, `DEMO.QUERIES`, bare `queries` for SQLite).
- [x] Connection failure → exit 1; password never printed; script files with CONNECT wiped ASAP.
- [x] CLI `--dump-db [PATH]` + env fallbacks (ACURANZO_/ CANVAS_ / HYDROTST_).
- [x] Do **not** scan product table row data; do **not** invoke `pg_dump`/`mysqldump`.
- [~] Filter `query_dialect_a30` — deferred (single-dialect rows in practice).

**Exit gate:** Against a known migrated test DB, result set contains expected refs/types; password not logged; all four engines exercised at least once.

**Status:** complete (2026-07-29) — verified PG/MySQL/SQLite/DB2 refs 1000–1001 (6 rows each); PG dump code+name+summary == expect for 1001

---

### Phase 4 — Checklist + content compare + remediation SQL + `.mig`

**Objective:** Full four-check evaluation, `tables` report, commented remediation `.sql`, and orphan `.mig`.

**Entry gate:** Phase 3 done.

**Work items:**

- [x] Join disk set ⟷ DB map by `query_ref`.
- [x] Implement checks 1–4 against **code + name + summary** (loose/strict).
- [x] Detect anomalies: orphan DB migration refs, both 1000 and 1003 present.
- [x] Normalization modes `strict` / `loose` (default loose).
- [x] Build data JSON from findings; render with `tables`; honor `--only-failures`, `--format tables|json|both`.
- [x] Implement remediate: **fully commented** SQL blocks (UPDATE code/name/summary + new-migration note; Hydrogen LOAD/APPLY guidance); lint = no uncommented executable SQL.
- [x] Implement **`.mig`** writer for orphan DB refs (plain-text blocks).
- [x] Exit codes 0/1/2/3 as specified.
- [x] `--include-reverse` / `--include-diagram` content checks when flags set.
- [x] Default path: connection ready → full audit (not dry-disk).
- [x] Full DB dump (no `--from`/`--to` filter) + full disk set for orphan detection; checklist still respects range.

**Exit gate:** On clean DB all Y + SQL header-only/OK; mutate DB code → L.match/A.match N + commented UPDATE; orphan → exit 3 + `.mig`.

**Status:** complete (2026-07-29) — verified on SQLite hydrodemo (clean 1000–1005 exit 0; drift exit 2; orphan 9999 exit 3)

---

### Phase 5 — UX polish, docs, operator packaging

**Objective:** Usable day-to-day tool under `extras/schematool` with docs linked from Hydrogen docs index.

**Entry gate:** Phase 4 done.

**Work items:**

- [x] Polish `tables` title/subtitle/footer (counts + exit label; Blue when clean / Red otherwise; shorten long sqlite paths).
- [~] Optional second summary table — deferred (footer carries counts; enough for v1).
- [~] `NO_COLOR` — deferred (depends on system `tables` binary behavior; not controlled here).
- [x] Doc page: `/docs/H/tools/SCHEMATOOL.md` + `extras/schematool/README.md`.
- [x] Link from plans README, SITEMAP, STRUCTURE, docs/H README, extras README.
- [x] Env: engine-specific then `SCHEMATOOL_DB_*`; documented precedence in help + SCHEMATOOL.md.
- [x] shellcheck on `schematool.sh`; markdownlint on new docs.

**Exit gate:** `--help` documents env; README one-liner works; remediation SQL remains 100% commented.

**Status:** complete (2026-07-29)

---

### Phase 6 — Automated tests (lightweight) — **deferred**

**Objective:** Regression safety for Track A without full Hydrogen blackbox dependency.

**Entry gate:** Phase 5 done. **Priority:** below Phase 7 unless a metadata regression blocks ops.

**Work items:**

- [ ] Unit: normalizer + compare + remediate (comment-prefix) Lua tests; include **comma-in-name** fixture (1172-class).
- [ ] Integration: SQLite temp DB — apply minimal LOAD SQL from two migrations via `sqlite3`, run schematool, assert green + SQL stub; mutate Lua, assert exit 2 and commented UPDATE present in `.sql`.
- [ ] Assert remediation file has no uncommented executable SQL (script check).
- [ ] Optional hook from a future blackbox or `extras` smoke script — **only if requested**; do not add `test_XX` unless asked.
- [ ] luacheck + shellcheck clean.

**Exit gate:** Documented one-command smoke passes.

**Status:** deferred (2026-07-29) — do Phase 7 first

---

### Phase 7 — Live catalog / object-shape audit (**NEXT — primary**)

**Objective:** Answer whether the **running database’s objects** match the **net effect of applied migrations**, not merely whether `queries` text matches Lua. This is the higher-value operator track.

**Entry gate:** Phase 5 done (v1 metadata usable). Phase 6 not required.

#### Why this is next

- Operators care that tables/columns/nullability/indexes exist and look right after hundreds of migrations.
- v1 correctly ignores “CREATE said NOT NULL, later migration dropped it” for **per-ref metadata** (each file still matches its own stored `code`). Live shape only appears when probing the catalog.
- Concrete tree example: **`acuranzo_1190.lua`** — `password_hash` DROP NOT NULL (and reverse SET NOT NULL). Metadata audit of 1190 is Y; live column nullability is the catalog signal.

#### Product constraints (carry forward)

- Still **read-only**; still native clients (`psql` / `mysql` / `sqlite3` / `db2`); no full-DB dump tools; **no product row-data scans**.
- **Targeted probes by default** (see **Catalog access model**); bulk `--dump-catalog` optional for debug.
- One schema at a time OK initially; **skip lists** / `--only-tables` for huge or uninteresting tables.
- Does **not** replace Track A — metadata remains default; **`--catalog`** adds catalog track (or catalog-only paths via dump/probe flags).
- Remediation stays **commented** guidance (Hydrogen new migration preferred over blind ALTER).
- **First engine: SQLite** (`hydrodemo.sqlite`); then PG → MySQL → DB2.

#### Locked Phase 7 product choices (2026-08-02)

| Topic | Decision |
| ------ | ---------- |
| First spike engine | **SQLite** (hydrodemo), then PG/MySQL/DB2 |
| Live access | **Targeted catalog probes** (narrow SQL/PRAGMA per in-scope table); not full-schema dump as default |
| Expected shape | **C hybrid** — presence + nullability/type for columns touched by CREATE / ALTER COLUMN / DROP NOT NULL-class ops |
| CLI | Additive **`--catalog`**; metadata path unchanged when flag omitted |
| Exit codes | Keep **0/1/2/3** simple; when both tracks run use **worst-wins** for now; **bitfield / separate codes later** if needed |
| Acceptance | **1190** `accounts.password_hash` live **nullable** after APPLY; no false fail from earlier CREATE NOT NULL text |

#### Phase 7a — Catalog probe adapters (spike SQLite → four engines)

**Work items:**

- [x] Define intermediate **live catalog JSON** contract — `{schema, tables:[{table, columns, primary_key, indexes}]}`.
- [x] **SQLite** `catalog_sqlite.sh`: `PRAGMA table_info` for named tables only.
- [x] **PostgreSQL** `catalog_pg.sh`: `information_schema` filtered by table list (Test 40 `demo`).
- [x] **MySQL** `catalog_mysql.sh`: `information_schema.COLUMNS` filtered.
- [x] **DB2** `catalog_db2.sh`: `SYSCAT.COLUMNS` EXPORT + Python parse.
- [x] CLI: `--dump-catalog [PATH]`; `--only-tables`; probe used by compare path.
- [x] Never SELECT fact-table row content; password hygiene same as Phase 3.

**Exit gate 7a:** SQLite + PG probes return stable JSON with `password_hash` nullability — **met 2026-08-02**.

#### Phase 7b — Expected shape (what “should” exist)

**Approaches (C locked 2026-08-02):** hybrid C — presence + nullability from folded applied type-1003 forward `code`.

**Work items:**

- [x] Lock approach **C hybrid** (2026-08-02).
- [x] Implement `lua/schematool_catalog_fold.lua` (CREATE/ALTER/DROP/MODIFY/RENAME/SQLite `_new` rebuild).
- [x] Anchor: 1190 `password_hash.nullable == true` on hydrodemo SQLite + Test 40 PG.
- [x] `--only-tables` keeps one-table audits cheap.

**Exit gate 7b:** **met 2026-08-02** — no false fail from earlier CREATE NOT NULL alone.

#### Phase 7c — Compare UX + remediation + multi-engine

**Work items:**

- [x] `lua/schematool_catalog_compare.lua` + `tables` catalog report.
- [x] CLI **`--catalog`**; with `--only-tables` skips metadata for speed; without runs after metadata; worst-wins exit.
- [~] Commented catalog remediation SQL blocks — deferred (checklist + notes sufficient for v2 first ship).
- [x] Probe adapters PG/MySQL/SQLite/DB2 present; SQLite+PG smoke verified; MySQL/DB2 await live Test 40 DBs in this env.
- [x] Docs: `/docs/H/tools/SCHEMATOOL.md` + extras README.
- [x] shellcheck (`mks`) + luacheck clean.

**Exit gate 7c:** **met 2026-08-02** for SQLite + PG 1190.

**Status:** **complete** (2026-08-02; multi-engine + prod guards + finding detail verified 2026-08-06 CLI **v1.7.1**)

---

### Phase 8 — Optional extensions (backlog)

**Objective:** Capture follow-ons without scope-creeping catalog v2.

Candidates:

- [ ] Full **cumulative DDL fold** if hybrid proves insufficient (Approach A deep).
- [ ] **Hydrogen-assisted mode:** call running server / reuse payload path (probably unnecessary if Lua path is solid).
- [ ] **Uncommented apply mode:** explicit `--emit-live-sql` (still default off).
- [ ] **Watch CI job** on Helium migration PRs (metadata exit 2 and/or catalog fail; attach SQL artifact).
- [ ] Compare reverse/diagram by default (metadata track).
- [ ] Multi-DB fanout (all seven test engines) wrapper script.
- [ ] SVG export of checklist table via existing Oh pipeline — nice-to-have only.
- [ ] Phase 6 unit/integration tests if still deferred.

**Status:** backlog

---

## Risk Register

| Risk | Impact | Mitigation |
| ------ | -------- | ------------ |
| Expected SQL ≠ stored SQL due to indent/base64/brotli pipeline differences | False drift | Phase 0 spike on known-good DB; compare decoded bodies; golden fixtures |
| Extending `database.lua` affects Hydrogen LOAD | Production migrations break | Prefer additive API; run migration tests 32–35 if `database.lua` changes |
| DB2 CLI awkwardness | Incomplete engine support | Prefer scripted SQL over interactive-only; spike `db2 -x` vs clpplus early in Phase 3 |
| Huge `code` blobs (mail templates, tours) | Slow / memory | Stream compare; optional hash-only mode (`--quick`) |
| Someone expects live schema audit only | Misses Lua edit drift | **Two tracks:** run metadata (v1) and catalog (P7); docs must not imply one replaces the other |
| Someone expects metadata checklist to show NOT NULL→NULL story | “Missing” 1190-class live drift | Catalog track + cumulative/hybrid expected shape; document in UX |
| Accidental full table scans | Slow / load on prod-like DBs | Never SELECT product row data; catalog mode uses skip lists |
| DDL fold false positives | Noisy catalog fails | Skip lists; start hybrid (C); lock acceptance on 1190 + sample CREATEs |
| Password on command line | Secret leak | `--password-env` only in examples; scrub debug logs; never put secrets in `.sql` |
| Dual 1000+1003 rows | Confusing checklist | Explicit anomaly column / exit 3 |
| Migrations that only touch data / QueryRefs | Still must match full `code` | Treat entire forward body as opaque text |
| Operator runs remediation `.sql` blindly | Accidental repair / damage | All statements commented; header warnings; prefer Hydrogen LOAD/APPLY guidance |
| `UPDATE queries.code` mistaken for schema fix | Live DDL still wrong | Explicit caveat in SQL blocks and docs |
| `tables` binary missing | No console report | Showstopper at startup (same as test framework) |

---

## Success Criteria (v1 metadata — largely met)

1. Against a fully migrated Acuranzo-compatible DB and matching Lua tree: **exit 0**, `tables` checklist all Y for every disk migration through APPLY (code+name+summary). *(Test 40 PG currently exit 2 only for known real 1280/1281 code drift.)*
2. Edit one historical Lua forward body without new migration file: **exit 2**, that ref shows L.match and/or A.match = N; remediation `.sql` contains a **commented** UPDATE (code/name/summary as needed) + new-migration guidance; no DB write.
3. DB missing latest APPLY only: APPLY = N for those refs; content checks skipped or marked `-`; SQL guidance prefers Hydrogen APPLY.
4. Orphan DB migration refs: listed in checklist, **exit 3**, captured in **`.mig`** for optional new-migration authoring.
5. No writes to the target database; remediation `.sql` has **zero** uncommented executable SQL.
6. Lives under `extras/schematool/`; console uses system `tables`; shellcheck + luacheck clean; no CMake/C changes required unless additive `database.lua` extract helper is chosen.
7. Names containing commas parse correctly (1172-class) — **fixed** expect 1.0.1.

## Success Criteria (v2 catalog — Phase 7)

1. Read-only **targeted catalog probes** for at least **SQLite** (hydrodemo) and **PostgreSQL** (Test 40 `demo`) — tables + columns + nullability for **in-scope** objects only.
2. **1190 acceptance:** live `password_hash` reported **nullable** after APPLY through 1190; tool does **not** fail solely because an earlier CREATE migration text still says NOT NULL.
3. Missing table or missing column (relative to expected net shape) → non-zero exit + clear checklist row; no product row scans; no mandatory full-schema dump.
4. Default (no `--catalog`) still behaves as v1 metadata; `--catalog` documented in `/docs/H/tools/SCHEMATOOL.md`.
5. Same safety bar: no uncommented live DDL from SchemaTool; secrets never in artifacts.
6. One-table audit (`--only-tables accounts`) stays cheap (pragma / filtered IS query, not whole-DB export).

---

## Open Questions (resolve in Phase 0)

1. ~~Install path: `extras/schematool` vs `tests/lib`?~~ **Resolved: `extras/schematool/`.**
2. ~~Console format?~~ **Resolved: Hydrogen `tables` (layout JSON + data JSON).**
3. ~~Second artifact?~~ **Resolved: commented remediation `.sql` (never auto-executed).**
4. ~~Extract strategy?~~ **Resolved: Lua (test_31 / get_migration); additive `database.lua` only if needed for pre-encode extract.**
5. ~~name/summary in scope?~~ **Resolved: match `code` + `name` + `summary`.**
6. ~~Orphan DB refs?~~ **Resolved: capture to `.mig` for optional new-migration rebuild; checklist + exit 3; commented DELETE may appear in `.sql`.**
7. ~~Engine aliases / scope?~~ **Resolved: four native client adapters (PG/MySQL/SQLite/DB2); Maria→mysql; CRDB/YB→postgresql.**
8. ~~Default normalization?~~ **Resolved: `loose`.**
9. ~~Drift remediation?~~ **Resolved: commented UPDATE code/name/summary + prefer-new-migration guidance.**

### Still open (resolve in Phase 7 catalog)

- ~~Exact `.mig` file format?~~ **Resolved: plain-text blocks** (header + per-ref type/name/summary/code).
- ~~Phase 4 default path?~~ **Resolved: full audit when connection ready.**
- ~~Phase 2 `database.lua` API?~~ **Resolved: not needed.**
- ~~DB2 client path?~~ **Resolved: EXPORT LOBS + Python.**
- ~~When to start live catalog?~~ **Resolved: Phase 7 is next primary** (operator priority over Phase 6 tests).
- ~~Expected-shape approach?~~ **Resolved: C hybrid** (2026-08-02).
- ~~CLI mode names?~~ **Resolved: additive `--catalog`** (metadata default).
- ~~Combined exit codes?~~ **Resolved for now: worst-wins; bitfield later** (do not add new codes yet).
- ~~Bulk dump vs probes?~~ **Resolved: targeted probes default; `--dump-catalog` optional debug.**
- ~~First spike engine?~~ **Resolved: SQLite hydrodemo, then PG.**
- Diagram payload: `code` stub vs `collection` ERD — optional for catalog expected shape (not primary for C).
- MySQL `--schema` vs connection database: multi-tenant layouts.
- Large DB2 catalog queries: always filter by table list; batch if needed.
- SQLite nullability: 1190 uses rebuild — probes must read post-rebuild `PRAGMA table_info`, not assume ALTER COLUMN exists.
- Optional later: `--focus-ref N` / migration-range limiting of expected-shape input (cheap one-migration audits).

---

## Lessons for remaining phases (read before Phase 7)

### Track A vs Track B (do not confuse)

1. **Metadata (v1)** = `queries` row text vs Lua. Detects edited-in-place migrations (1280/1281).
2. **Catalog (v2)** = live objects vs net expected shape. Detects missing cols, wrong nullability (1190).
3. A green metadata row does **not** prove live DDL; a green catalog does **not** prove `queries.code` matches Lua.
4. **1190** is the teaching example for catalog; **1280/1281** for metadata.

### Expected side (Lua)

1. **Reuse test_31** — `get_migration.lua` pattern: cwd=migrations, `require('database')`, pass **`database.defaults[engine]` by reference**, `run_migration`, decode wrappers like `get_diagram.sh`.
2. **Do not shallow-copy defaults** — migrations set `cfg.TABLE` / `cfg.MIGRATION` on the shared table.
3. **Bootstrap 1000** — stored forward is INSERT type **1003** payload, not bare DDL `-- QUERY DELIMITER` parts before the INSERT.
4. **Forward type in Lua INSERT is 1000**; after APPLY the DB row is **1003** with the **same code body** — compare expected type-1000 (or 1003 if bootstrap) body to DB type 1000 **or** 1003.
5. **Trailing newline** — loose normalize must strip trailing newlines (DB often one extra `\n` from client).
6. **Diagram type 1002** — `code` often short stub; real ERD may be in `collection`; default v1 checks = forward only unless `--include-diagram`.
7. **lua-brotli** required for compressed payloads (`require('brotli')`).
8. **`extract_as_field` must ignore commas inside SQL strings** — names like `a, b, metadata` broke
   backward comma scan (1172 → `metadata'`). Quote-aware scan in expect 1.0.1.
9. **v1 ≠ live schema timeline** — later `DROP NOT NULL` (e.g. 1190) does not make earlier CREATE
   “fail” metadata checks. That product is Phase 7 catalog / cumulative projection.

### DB side (native clients)

1. **Contract JSON** (all adapters):  
   `[{ "query_ref", "query_type", "name", "summary", "code" }, …]` types 1000–1003 only.
2. **Engine env defaults** (flags override):  
   PG→`ACURANZO_DB_*`; MySQL→`CANVAS_DB_*`; DB2→`HYDROTST_DB_*`; SQLite→file path (`hydrodemo.sqlite` has migrations; `hydrotst.sqlite` may be empty/minimal).
3. **Schema labels differ**: PG/MySQL often `demo`; DB2 `DEMO` (uppercase qualify); SQLite no schema.
4. **SQLite**: CAST name/summary/code to TEXT before `json_object`; build `json_group_array(json_object(...))` **inline** (subquery of json_object double-encodes).
5. **DB2**: use `EXPORT … OF DEL LOBS TO … LOBFILE … MODIFIED BY LOBSINFILE` then parse DEL + `.lob` offsets; wipe CONNECT script immediately; password scrub on errors.
6. **Never** log passwords; MySQL `-p"$PASS"` still warns on CLI — strip warning lines from stdout.
7. **Cross-engine code lengths differ** (dialect SQL) — compare expected generated **for that engine**, never PG dump vs MySQL expect.
8. **PG dump verified** equal to Lua expect for forward 1001; use same normalize in Phase 4.

### Phase 4 implementation (landed)

```text
disk refs (discover, ranged) ──┐
disk-all (discover, full)  ────┤ orphan membership
expected (expect.lua, ranged) ─┼─► compare → checklist + findings
db rows (dump ALL types 1000–1003) ─┘
render tables(layout, data)
write commented .sql + plain-text .mig (orphans)
```

- Join key: `query_ref`; DB forward for L.match: prefer type **1000**, else **1003**.
- Expected forward: prefer `query_type==1000`, else `1003` (bootstrap).
- A.match: type **1003** only.
- Full audit when connection ready; `--dry-disk` / `--dump-db` / `--emit-expected` stay specialized.
- Remediation SQL: every non-blank line `--` prefixed (lint in remediate).
- Exit: 0 all pass; 2 drift/missing; 3 anomalies/orphans; 1 hard error.
- jq filters via temp `-f` files (shell must not expand `$m` etc.).

### Phase 7 implementation sketch (fast start)

```text
applied max ref (type 1003) ──► which migrations count as "in effect"
expected shape builder (C) ───► tables/cols/nullable needed for compare
                                │
                                ▼
              table list (∩ --only-tables / skip)
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
  probe_sqlite (PRAGMA)   probe_pg (IS filtered)   … mysql/db2
        │                       │
        └───────────┬───────────┘
                    ▼
              live catalog JSON (same contract)
                    ▼
         compare ─► tables report + commented guidance
acceptance: 1190 password_hash nullable (SQLite hydrodemo first, then PG demo)
```

- **Not** “dump whole schema then analyze” on the default path — **derive probe list → query catalog for those objects only**.
- Reuse connection/env/password patterns from Phase 3 adapters.
- New files (draft): `db/catalog_sqlite.sh` (first), `db/catalog_pg.sh`, …; `lua/schematool_catalog_expect.lua` / compare; CLI `--catalog`, `--only-tables`, `--dump-catalog` in `schematool.sh`.
- Keep metadata path untouched unless sharing helpers (normalize, tables render).

### Operator sample commands

```bash
# Full metadata audit (Test 40 PG)
extras/schematool/schematool.sh --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo --engine postgresql --schema demo \
  --out-dir /tmp/schematool-out

# Expected only
extras/schematool/schematool.sh --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo --engine postgresql --schema demo --from 1000 --to 1002 \
  --emit-expected /tmp/exp.json --no-sql

# DB metadata only
extras/schematool/schematool.sh --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo --engine postgresql --schema demo --from 1000 --to 1002 \
  --dump-db /tmp/db.json --no-sql

# Disk checklist
extras/schematool/schematool.sh --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo --engine postgresql --dry-disk --from 1000 --to 1005

# Phase 7 (not implemented yet) — sketch:
# extras/schematool/schematool.sh ... --mode catalog --schema demo
# extras/schematool/schematool.sh ... --dump-catalog /tmp/cat.json
```

---

## Working Log

### 2026-07-29 — Plan authored

- Reviewed `/docs/H/INSTRUCTIONS.md`, `/docs/H/tests/TESTING.md`, `/docs/H/tests/TESTING_UNITY.md`.
- Traced LOAD (`execute_load.c` / `lua_execute_load_metadata` → `run_migration`), APPLY (`lead_apply.c`), bootstrap type 1000/1003 tracking (`database_bootstrap.c`).
- Confirmed query type constants and INSERT/`code` encoding in Helium `database.lua` + sample `acuranzo_1001.lua`.
- Noted existing offline helpers: `tests/lib/get_migration.{lua,sh}`, `get_diagram.sh` (decode patterns).
- ~283 numbered Acuranzo migrations; highest observed `acuranzo_1282.lua`.
- Tool scope locked to Bash/Lua read-only auditor; phased plan Phases 0–7 written.

### 2026-07-29 — Outputs and home directory

- **Home locked:** `/elements/001-hydrogen/hydrogen/extras/schematool/`.
- **Console:** use system `tables` binary (layout JSON + data JSON), same as `framework.sh` / `coverage_table.sh` / `comment-analysis.sh` — not a custom ASCII table.
- **Second artifact:** remediation `.sql` listing INSERT/UPDATE/DELETE (etc.) needed to align DB metadata with Lua; **every statement starts commented** so accidental execution is a no-op until an operator uncomments.
- SQL generator must warn that updating `queries.code` does not replay DDL; missing APPLY should prefer Hydrogen AutoMigration guidance over dumping full DDL.
- Phases 0–6, success criteria, architecture, CLI flags updated accordingly.

### 2026-07-29 — Phase 0 decisions + Phase 1 implemented

- User answers locked (see **Locked Decisions**): Lua extract like test_31; match code+name+summary; orphans → **`.mig`**; all four engines; loose normalize; UPDATE + new-migration note; skip formal PG spike → Phase 1.
- Confirmed clients: `tables`, `psql`, `mysql`, `sqlite3`, `db2`, `clpplus`.
- Implemented:
  - `extras/schematool/schematool.sh` (CLI, tables render, SQL stub, shellcheck clean via `mks`)
  - `extras/schematool/lua/schematool_discover.lua` (luacheck clean)
- Smoke: `--dry-disk --from 1000 --to 1005` → 6-row `tables` checklist + commented SQL; full disk discovery → **283** refs.
- Lesson: Lua `string.format` + `%` in patterns is fragile — build discovery patterns with concatenation + `gsub` escapes.
- Lesson: test_31 is the offline Lua migration generator path (`get_migration.sh` → `get_migration.lua`), not a separate blackbox server test for drift.
- Next: Phase 2 `schematool_expect.lua` + early PG equality check.

### 2026-07-29 — DB access model (native clients vs dump)

- Clarified operator question: **yes, use normal DB clients + credentials** (`psql`/`mysql`/`sqlite3`/`db2`), not a full dump-and-analyze pipeline.
- Plan wording “dump adapters” meant **fetch migration metadata rows** into JSON — renamed mentally/docs to **client adapters** / `query_*.sh`; never `pg_dump`/`mysqldump` for v1.
- v1 scope remains **`queries` types 1000–1003** (code+name+summary). That is what detects edited-in-place Lua after LOAD/APPLY.
- **Live catalog** (`DESCRIBE TABLE`, `information_schema`, etc.) is a valid **later** mode: per-schema, skip lists for huge/uninteresting tables, **no product row-data scans** (empty or huge tables OK to skip for data). Catalog does **not** replace metadata compare (see matrix in **DB access model**).
- Phase 3/7, risks, architecture paths, checklist match fields, and resume blurb updated accordingly.
- Still deferred until asked: starting catalog mode before Phase 4 completes.

### 2026-07-29 — Phase 2 expected extraction

- Reused **test_31** path: `get_migration.sh` / `get_migration.lua` → `database:run_migration`; decode INSERT `code`/`name`/`summary` like **get_diagram.sh** (first quoted base64; brotli if wrapper present).
- **Must pass `database.defaults[engine]` by reference** — migrations mutate `cfg.TABLE` / `cfg.MIGRATION`; copying defaults left `${TABLE}` unexpanded.
- Bootstrap **1000**: stored forward body is the **INSERT type 1003** payload, not the bare leading DDL QUERY DELIMITER parts.
- PG `demo` equality (trailing newline normalize): 1000, 1001, 1002, 1148 forward code+name; 1001 summary OK.
- Diagram type 1002 often has short stub `code` (ERD in `collection`) — optional later.
- Files: `lua/schematool_expect.lua`, CLI `--emit-expected`, `testdata/expected_pg_demo_1000_1002.json`; `mks` + luacheck clean; two-run stable.

### 2026-07-29 — Phase 3 native client dumps

- Adapters: `db/query_{pg,mysql,sqlite,db2}.sh`; CLI `--dump-db`; env fallbacks ACURANZO_/CANVAS_/HYDROTST_.
- JSON contract unified: `query_ref`, `query_type`, `name`, `summary`, `code`.
- **PG**: `json_agg` — dump 1001 forward matches expect (code+name+summary).
- **MySQL**: `JSON_ARRAYAGG` on `demo.queries` via CANVAS_*.
- **SQLite**: use **hydrodemo.sqlite** (has migrations); CAST to TEXT; inline `json_group_array(json_object(...))` to avoid double-encoded strings.
- **DB2**: `EXPORT OF DEL LOBS TO … LOBFILE … MODIFIED BY LOBSINFILE`; Python csv + LOB offset parse; CONNECT script deleted before parse; schema `DEMO`.
- `mks` clean (136 scripts). Password not in JSON artifacts.
- Added **Lessons for remaining phases** section for Phase 4 continuity.
- Next: Phase 4 join/compare/remediate.

### 2026-07-29 — Session stop (after Phase 3)

- Stopping here by request. Phases **0–3 complete**; Phase 4 not started.
- Resume: **Resuming Work** + **Lessons for remaining phases** + Phase 4 sketch.
- Tree under `extras/schematool/`: `schematool.sh` (v1.2.0), `lua/schematool_{discover,expect}.lua`, `db/query_{pg,mysql,sqlite,db2}.sh`, `testdata/expected_pg_demo_1000_1002.json`.

### 2026-07-29 — Phase 4 full audit

- Decisions locked this session: full audit by default when connection ready; `.mig` plain-text blocks; SQLite hydrodemo smoke; implement `--include-reverse`/`--include-diagram`.
- New Lua: `schematool_normalize.lua`, `schematool_compare.lua`, `schematool_remediate.lua`.
- CLI v**1.3.0**: default path discover+expect+dump+compare+tables+.sql/.mig; exit 0/2/3.
- **jq lesson:** never pass filters with `$vars` on a shell command line (even via Lua `%q` → double-quoted shell expands `$m`). Use **`jq -f tempfile`**.
- **Orphan lesson:** dump DB **without** `--from`/`--to`; orphan membership uses **full disk set** (`--disk-all`), while checklist/expect stay ranged.
- **Forward pick:** expected `1000` else `1003` (bootstrap); DB prefer `1000` else `1003` for L.match; A.match always type `1003`.
- SQLite exit-gate: clean 1000–1005 → exit 0; mutate `queries.code` on 1001 → exit 2 + commented UPDATE; insert orphan ref 9999 → exit 3 + `.mig`.
- Remediation SQL lint: every non-blank line must start with `--`.
- Next: Phase 5 docs/polish.

### 2026-07-29 — Phase 5 UX + docs

- CLI v**1.4.0**: env precedence flags → engine-specific → `SCHEMATOOL_DB_*`; default ports after env; tables footer with ok/drift/missL/missA/orphan + exit label; theme Blue (clean) / Red (else) — only themes supported by `tables`; short sqlite basename in title.
- Docs: `/docs/H/tools/SCHEMATOOL.md`, `extras/schematool/README.md`; links in SITEMAP, STRUCTURE, docs/H README, extras README, plans README.
- Deferred: second summary table; `NO_COLOR` (tables binary).
- Next: optional Phase 6 lightweight tests, or treat v1 as done for operators.

### 2026-07-29 — Test 40 PG audit + name parse fix

- Full audit against Test 40 PostgreSQL (`ACURANZO_DB_*`, schema `demo`): **283** refs, exit 2, **3** drifts initially.
- **1172 false positive:** `extract_as_field` walked commas without respecting SQL string quotes; name
  `'Extend … mime_type, metadata'` truncated to `metadata'`. Fixed in `schematool_expect.lua` 1.0.1
  (backward scan skips commas inside `'…'` / `''` escapes). Re-audit 1172 → exit 0.
- **1280 / 1281 real metadata drift (confirmed):** name+summary match; code differs after loose normalize.
  Lua has newer mail tokens (`%COUNT|1%x`, `%SUMMARY|1 event%`); DB still has older (`%COUNT%x`, `%SUMMARY%`).
  1281 handler seed code longer on disk (3864 vs 3728). Not a parse bug — disk Lua edited after APPLY.
- **1190 password_hash DROP NOT NULL:** metadata match Y (as designed). Live nullability change is **not**
  reported by v1 — that needs Phase 7 **live catalog / DESCRIBE** (or cumulative schema fold), not
  `queries.code` text compare. CREATE-time NOT NULL still matches its Lua file; later ALTER is a different ref.
- Operator expectation clarified: v1 answers “did someone edit migration N’s stored text?” not
  “does live table shape equal migration N’s CREATE alone?”

### 2026-07-29 — Plan priority: catalog next

- User confirmed understanding: 1280/1281 = stored query **content** drift; live objects = **not** v1.
- User priority: live object fidelity **more important** than `queries`-only consistency going forward.
- Plan updates: Purpose + **Two-track product model**; Phase 6 **deferred**; Phase 7 expanded to
  **7a dump / 7b expected shape / 7c UX** with **1190 acceptance**; Phase 8 = former optional backlog;
  Resuming Work points at Phase 7; v2 success criteria added.
- Do **not** treat Phase 6 tests as the default next session task.

### 2026-08-02 — Phase 7 locks (pre-implementation review)

- Reviewed INSTRUCTIONS, TESTING, SCHEMATOOL_PLAN; next work is Phase 7 catalog (not Phase 6).
- **First engine: SQLite** (hydrodemo), not PG-first — easier local spike; 1190 rebuild path must be documented.
- **Access model:** default = **targeted interactive probes** (connect + narrow `PRAGMA` / filtered `information_schema`), **not** generate-a-huge-metadata-dump-then-analyze. Full `--dump-catalog` remains optional debug.
  - Rationale: auditing one migration (1190 → `accounts` only) must stay cheap; whole-schema dumps do not scale as the default path.
- **Expected shape: C hybrid** locked (presence + nullability/type for CREATE/ALTER-touched columns).
- **CLI: `--catalog`** additive; metadata remains default when flag omitted.
- **Exit codes:** keep 0/1/2/3; worst-wins if both tracks run; **separate bitfield later** — do not invent new codes now.
- Plan sections updated: Resuming Work, Catalog access model, Phase 7a/b/c, open questions, success criteria, implementation sketch.

### 2026-08-02 — Phase 7 implemented (catalog v2)

- CLI **v1.5.0**: `--catalog`, `--dump-catalog`, `--only-tables`.
- New: `db/catalog_{sqlite,pg,mysql,db2}.sh`, `lua/schematool_catalog_{fold,compare}.lua`.
- Fold source: **DB type 1003 applied `code`** (what actually ran), ordered by ref; handles CREATE, ADD/DROP COLUMN, DROP/SET NOT NULL, MODIFY COLUMN, DROP TABLE, RENAME (SQLite `_new` rebuild).
- **1190 acceptance:** hydrodemo SQLite + Test 40 PG `demo` — `accounts.password_hash` expected=true live=true, exit 0, 16/16 column checks.
- `--only-tables` skips metadata audit (fast path); without filter, `--catalog` stacks on full metadata (worst-wins exit).
- `mks` clean; luacheck clean on new Lua.
- Docs updated: `/docs/H/tools/SCHEMATOOL.md`, `extras/schematool/README.md`.
- Lesson: catalog compare must flatten via `jq` TSV for reliable boolean parse (hand JSON bool scan was fragile).
- Lesson: Lua patterns have no `(?:…)` — write two DROP TABLE matches (IF EXISTS / plain).

### 2026-08-02 — All Test 40 engines catalog green

1190 `accounts.password_hash` nullable on **every** Test 40 layout (exit 0, 16/16 cols):

| Label | Engine flag | Schema / DB | Exit | password_hash |
| ------- | ------------- | ------------- | ------ | --------------- |
| postgres_demo | postgresql | demo @ ACURANZO | 0 | Y exp=true live=true |
| cockroach_democrdb | postgresql | democrdb @ ACURANZO (schema on PG host) | 0 | Y |
| mysql_demo | mysql | demo @ CANVAS (connect DB=schema) | 0 | Y |
| mariadb_demomrdb | mysql | demomrdb @ CANVAS | 0 | Y |
| sqlite_hydrodemo | sqlite | hydrodemo.sqlite | 0 | Y |
| db2_demo | db2 | DEMO @ HYDROTST | 0 | Y |
| yugabytedb_demo | postgresql | demo @ YUGABYTE | 0 | Y |

**MySQL lessons (fixed):**

1. Connect to **schema as database** (`demo` / `demomrdb`), not bootstrap name `canvas` (may be missing).
2. `JSON_ARRAYAGG` / long `JSON_OBJECT` rows **truncate** in mysql client — metadata dump uses **HEX(columns) + Python** decode; catalog probe uses flat COLUMNS TSV + Python assemble.
3. Nested `JSON_ARRAYAGG` + correlated `t.TABLE_NAME` failed on MariaDB (`Unknown column t.TABLE_NAME`) — flat join avoids it.

Deferred still: catalog remediation `.sql`; Phase 6 unit tests.

### 2026-08-06 — Prod guards, Test 40 smoke, finding detail, archive

- CLI **v1.7.0**: `ENGINE_REQUESTED` env map (`yugabytedb` → `YUGABYTE_DB_*`); read-only client guards (PGOPTIONS / MySQL session RO / SQLite `-readonly`); `smoke_test40_catalog.sh` — **7/7** PASS on 1190.
- CLI **v1.7.1**: post-table **finding details** (`lua/schematool_detail.lua`) — line diffs (− DB / + Lua) + commented UPDATE guidance; flags `--no-detail`, `--detail-max-lines`; artifacts `finding_detail.txt` / `catalog_finding_detail.txt`.
- Operator docs + extras README updated (Safety checklist, wrappers, detail section).
- Plan marked complete and moved to `plans/complete/SCHEMATOOL_PLAN_COMPLETE.md`.

---

## References

- Hydrogen migration headers: `/elements/001-hydrogen/hydrogen/src/database/migration/migration.h`
- LOAD: `/elements/001-hydrogen/hydrogen/src/database/migration/execute_load.c`
- APPLY: `/elements/001-hydrogen/hydrogen/src/database/dbqueue/lead_apply.c`
- Bootstrap: `/elements/001-hydrogen/hydrogen/src/database/database_bootstrap.c`
- Lua engine: `/elements/002-helium/acuranzo/migrations/database.lua`
- Sample migrations: `/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua`, `acuranzo_1001.lua`
- Offline generator: `/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua`
- Catalog acceptance sample: `/elements/002-helium/acuranzo/migrations/acuranzo_1190.lua` (DROP NOT NULL)
- Metadata drift samples: `acuranzo_1280.lua`, `acuranzo_1281.lua` (mail seeds; Lua ahead of DB on Test 40 PG)
- Operator docs: `/docs/H/tools/SCHEMATOOL.md`
- Extras quick start: `/elements/001-hydrogen/hydrogen/extras/schematool/README.md`
- Smoke: `/elements/001-hydrogen/hydrogen/extras/schematool/smoke_test40_catalog.sh`
- Completed migration perf plan (context only): `/docs/H/plans/complete/MIGRATIONS_COMPLETE.md`
- Plans index: `/docs/H/plans/README.md` / completed: `/docs/H/plans/complete/README.md`
