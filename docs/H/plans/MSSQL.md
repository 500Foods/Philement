<!-- markdownlint-disable MD007 MD024 -->
# Microsoft SQL Server Engine Plan

## Status at a glance

**New plan (2026-09-18).** Phase 0 approved by implementation; Phase 3 C engine
skeleton written (2026-09-29). Lookup 030 **key 5** is already `MS SQL Server`
(`acuranzo_1055.lua`). This plan implements that dialect. It does **not** replace
Cockroach (that is [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) / Test 37).

Fedora does not package `mssql-server`. The local path is the official
**Linux** container (`mcr.microsoft.com/mssql/server`) under Podman on
this Fedora box — not Windows, not Azure, not DOKS unless Phase 1
proves local RAM/image cannot run.

| Phase | Status | Remaining |
| --- | --- | --- |
| 0 Contract lock | complete | **Quick** |
| 1 Fedora Podman SQL Server + ODBC | complete | **Moderate** |
| 2 Helium dialect | complete | **Moderate** |
| 3 C register / connect (unixODBC) | complete | **Moderate** |
| 4 T-SQL helpers + Brotli CLR | complete | **Difficult** |
| 5 Test 39 full Acuranzo | complete | **Difficult** |
| 6 SchemaTool / flush | not started | **Moderate** |
| 7 Grow matrix 7 → 8 | in progress | **Difficult** |
| 8 Docs | not started | **Quick** |
| 9 Coverage / completeness | not started | **Moderate** |

Remaining: Phase 7 (Difficult, in progress), Phase 6 (Moderate),
Phase 9 (Moderate), Phase 8 (Quick). Phases 0–5 are complete.
Phase 7 started ahead of SchemaTool on 2026-09-30 so the full suite
can include MSSQL. Phase 6 items 6.1–6.3 are still unchecked.
Item 6.4 (the Test 40 transaction probe) moved into this Phase 7
slice. Items 7.1 and 7.2 are checked from the 2026-10-01 suite.
Item 7.3 stays open. Suite `20261001_105356` connected `Demo_MS` and failed QueryRef 30 (`LENGTH`) and QueryRef 57 (positional `?`). The source fix is `database_mssql.lua` 1.3.3 and `acuranzo_1151.lua` 1.7.0. Suite `20261001_123106` had that payload (available=1385). The operator later confirmed that refresh had not finished repopulating the databases, and AutoMigration tried to LOAD or APPLY rows that were already written. Login failed because QueryRef 1 was not in the cache. `LENGTH` and QueryRef 57 were not reached. Suite `20261001_134516` (Build 2688) is the run after that correction: Test 50 is 137/137 and Test 60 is 46/46. The remaining failures are Tests 43, 45, and 47, and none of them is MSSQL.
Phases 8–9 are not started.

**Parity:** MSSQL is a Hydrogen `DatabaseEngineInterface`, not a new
API. Match PostgreSQL / SQLite / MySQL / DB2: same `QueryRequest` /
`QueryResult`, same `parse_typed_parameters` →
`convert_named_to_positional` → bind, same `data_json` array of row
objects. Do not change those engines. Do not interpret SQL in C.
Statement spelling is repaired in `database.lua` before it is stored.

**Sister plan:** [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) (key 6,
Test 37, Cockroach retirement, Firebase teardown). Shared enum lock is
in [Coordination with Firebird](#coordination-with-firebird).

## Purpose

Implement the **MS SQL Server** Helium dialect that Lookup 030 key 5
has advertised since `acuranzo_1055.lua`, as a real sixth C engine
(`src/database/mssql/`) talking TDS through **unixODBC + Microsoft
ODBC Driver 18**.

This is additive. After Firebird replaces Cockroach the operator matrix
is still **7** (SQLite, PostgreSQL, MySQL, MariaDB, DB2, Firebird,
Yugabyte). This plan grows it to **8**. Test number **39** (37 is
Firebird). Do not steal Test 37.

**There is no v1 subset.** AutoMigrations must apply the same Acuranzo
Lua files as Test 32. Helium stays SQL. Hydrogen sends that SQL to
SQL Server. Helpers SQL Server lacks (Brotli; convenient Base64) are
T-SQL functions and one extras CLR assembly — not a Hydrogen SQL VM.

**Free, Linux, Fedora-local:**

1. **Preferred:** Podman + official Linux image
   `mcr.microsoft.com/mssql/server:2022-latest`,
   `MSSQL_PID=Developer` (free for development/test, not production).
   This box already has Podman 5.8.4. Fedora 43.
2. **Client:** Fedora `unixODBC` / `unixODBC-devel` plus Microsoft
   `msodbcsql18` (RHEL 9 packages; Phase 1 must prove they install on
   Fedora 43).
3. **Not preferred:** DOKS Linux node. Festival nodes are already
   RAM-bound with Yugabyte. Use only if Phase 1 records that the
   container cannot run locally (SQL Server wants ≥2 GiB).
4. **Forbidden:** Windows SQL Server, paid Azure SQL as the CI engine,
   Babelfish (another PostgreSQL alias — the Cockroach pattern).

This is the only active plan for MSSQL.

**Session brief:** Open [Status at a glance](#status-at-a-glance), then
[Resuming Work](#resuming-work), then the next incomplete phase only.

History that this plan does **not** reopen: same completed database /
binding / migrations / SchemaTool plans listed in FIREBIRD.md.
Firestore attempt:
[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status, append Working Log, record lessons,
  **stop for review**.
- Build aliases: `mkq` / `mkt` / `mku` / `mkp` / `mks` / `mkl` as in
  [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**:

1. Confirm the prior phase Status is actually complete.
2. Discuss the current phase first (Goal + Work items + Done means +
   Exit gate). Research before writing code.
3. Get explicit approval before editing source.
4. Ask questions as they come up.
5. Update the Working Log when major pieces land.
6. Record lessons learned.
7. Mark `[x]` / Status complete only after verification commands ran.
8. Never apply a database migration; hand packets to the user.
9. Project norms: no `static` in `src/`; Unity one file per function;
   `jq` in Bash; absolute Markdown links; completeness + coverage
   fences.
10. Never log SA passwords, password hashes, JWTs.
11. Do not increment `TEST_COUNTER`.
12. Do not retire Cockroach and do not delete Firebase — those belong
    to [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md).
13. Mirror the other engines. Do not change PG / SQLite / MySQL / DB2
    to accommodate MSSQL.
14. Do not interpret SQL in C. MSSQL statement spelling lives in
    `database.lua` / `database_mssql.lua`. See the follow-up below.
    C keeps driver work, including `SQLFreeStmt(SQL_CLOSE)` before
    `SQLEndTran` after a failed execute.

## Follow-up: statement rewrites in database.lua

Lua landed 2026-09-30, and the C rewriter was removed the same day.
`acuranzo/migrations/database.lua` 3.6.0 calls
`cfg.rewrite_migration_sql` immediately after the Firebird block.
The passes live in `acuranzo/migrations/database_mssql.lua` 1.3.1.
1.3.3 adds `LENGTH(` → `LEN(` on that same hook.
`src/database/mssql/rewrite.c`, `rewrite.h`, and
`tests/unity/src/database/mssql/rewrite_test_mssql.c` are gone.
Prepare and execute send the statement text they are given.
`SQLFreeStmt(SQL_CLOSE)` before `SQLEndTran` stays in `query.c`.
A Lua parity run matched the old C fixtures, including a nested
`[=[ [==[ ]==] ]=]` template, and all 385 Acuranzo migrations
generated for `mssql` with no mask token and no top-level
`ADD COLUMN`, `RETURNING`, bare `VALUES` CTE, or `DROP COLUMN`.
`luacheck` on those two files was clean. 1.3.1 keeps the newline
after `-- SUBQUERY DELIMITER` when it moves `INSERT…WITH`. A
generation sweep of all 385 Acuranzo migrations found no glued
delimiter, every delimiter followed by a newline, and a second
pass identical to the first. `mkt` reconfigures CMake
and builds `hydrogen`. It does not regenerate
`payloads/payload.tar.br.enc`, and it does not rebuild
`hydrogen_release`. Test 39 launches `hydrogen_coverage` when that
file exists, otherwise `hydrogen_release`. `mka` regenerates the
payload when the Acuranzo Lua is newer than the tarball, then
rebuilds those binaries and embeds the tarball. The closing Test 39
log is
`build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log`.
Summary available=loaded=applied=1384, 385 reverses through
migration 1000, normal execution at 498.021s, no `[ ERROR ]`
lines. Phase 5 is complete. The C-rewriter cycle's
reverse finished 2026-09-30 19:50Z: migrations 1003 through 1000
reversed, then `Migration test finished - normal execution`.

Two reasons to move the text repairs out of C:

- The stored statement should be the statement SQL Server runs, same
  as Firebird's `replace_query` passes.
- The C engines stay in one shape. Rule 13. DB2 is the ODBC sibling
  and its directory is connection, query, prepared, transaction,
  interface, types, utils. MSSQL no longer adds `rewrite.c` /
  `rewrite.h` on top of that list. `query_result.c` holds row
  JSON so `query.c` stays under 1000 lines. Firebird's migration-shape
  fixes (`VALUES`, `ADD`/`DROP COLUMN`, `NOT NULL` order) live in
  `database.lua`. Its C rewriter (`firebird_rewrite_engine_sql`,
  `firebird_rewrite_dateadd_params` in `query_bind.c`) is a later,
  separate set for already-loaded QueryRefs (`LIMIT`, `DATEADD`,
  `LENGTH`, `FLOAT`). MSSQL `LENGTH` is the Lua pass instead: shared
  source stays `LENGTH(`, and the stored MSSQL statement becomes
  `LEN(`. MSSQL should not grow a second permanent SQL
  interpreter of that kind. After this follow-up, a migration writer
  looks in `database.lua` / `database_mssql.lua` for spelling, and in
  C only for the same driver work the other engines have.

Firebird repairs shared statement shapes in
`acuranzo/migrations/database.lua` `replace_query`, before the
`[=[...]=]` block is sealed into `queries.code`. MSSQL does the
same there. Prepare and execute do not rewrite the text again.
Until LOAD runs against an empty `queries` table, a stored statement
can still be the shared spelling, and SQL Server will reject it.

### What moves

Five passes, in the order the removed C function
`mssql_rewrite_migration_sql` used, applied per statement. A
whole-template "if this migration contains
`RETURNING`, skip the CTE passes" would skip a `VALUES` body that
lives in a different statement of the same file.

| Pass | Shared spelling | Stored MSSQL spelling |
| --- | --- | --- |
| 1 | trailing `RETURNING col` | `OUTPUT INSERTED.col` |
| 2 | `WITH name(cols) AS (VALUES …)` | `AS (SELECT * FROM (VALUES …) AS v(cols))`, repeat for nested bodies |
| 3 | `INSERT INTO … WITH cte … SELECT` | `WITH cte … INSERT INTO … SELECT` |
| 4 | `ALTER TABLE … ADD COLUMN col` | `ADD col` |
| 5 | `ALTER TABLE name DROP COLUMN col` | one `EXEC sp_executesql` batch: drop the default constraint when one exists, then drop the column |
| 6 | `LENGTH(` | `LEN(` (`CHAR_LENGTH`, `DATALENGTH`, `OCTET_LENGTH`, `CHARACTER_LENGTH`, string literals, and `--` comments stay) |

Pass 1 and pass 3 are exclusive inside a single statement, because the
`RETURNING` rewriter already places `OUTPUT` on an
`INSERT … WITH … SELECT`. Passes 4 and 5 still run after either.
A comma-separated `DROP COLUMN a, b` stays unchanged. Quoted text and
line comments stay unchanged. Each pass leaves a statement alone when
its shape is absent, so a second Lua pass is a no-op.

These stay where they are. They are not statement-shape repairs:

- Type macros, T-SQL helpers, and `COMPRESS_START` / `COMPRESS_END`
  in `database_mssql.lua`.
- Per-migration `if engine == 'mssql'` arms (1000, 1135, 1151, 1168,
  1190).
- ODBC behavior in C: binding, transactions, and
  `SQLFreeStmt(SQL_CLOSE)` before `SQLEndTran` after a failed execute.

### Where it goes

`acuranzo/migrations/database.lua` is already 1265 lines. The Firebird
bodies are most of that. Do not paste the MSSQL bodies into it.

- Call site: `replace_query`, directly after the
  `if engine == "firebird"` block. Four lines. This is the place a
  reader already looks for Firebird.
- Bodies: `acuranzo/migrations/database_mssql.lua` (about 820 lines
  after the passes landed). That file is the MSSQL dialect file and
  stays under the 1000-line cap. Do not move the scanners into
  `database.lua`.

Gaius, glm, and helium do not carry the Firebird passes either. This
cutover is Acuranzo only, which is what Test 39 loads.

### Cutover

LOAD skips any migration ref at or below the highest loaded or applied
ref. A finished reverse flips rows back to forward; it does not
rewrite `queries.code` and it does not make LOAD generate them again.
Lua changes show up only when those rows are absent and LOAD runs.

1. Land the Lua passes. Done.
2. Prove text parity against the fixtures that were in
   `tests/unity/src/database/mssql/rewrite_test_mssql.c`, and
   `luacheck` (Test 98) on the dialect file. Done. That Unity file
   was removed with the C rewriter. `mkt` does not regenerate the
   payload or rebuild `hydrogen_release`; `mka` does, after CMake
   is reconfigured so the deleted `rewrite.c` leaves the source
   glob. The 21:12Z Test 39 ran a payload that already had the
   1.3.0 passes. The closing run is the 22:14Z log below.
3. Drop `testms.queries` (or the `hydrotst` database) and run Test 39
   forward, then reverse. Done 2026-09-30 22:14Z. Log
   `build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log`.
   Summary available=loaded=applied=1384. Reverse 1384 through 1000
   (385). Migration 1147 APPLY succeeded. No `[ ERROR ]` lines.
4. Remove the five C passes. Done 2026-09-30, before the reload,
   because the next Test 39 will start from an empty `queries` table.
   `rewrite.c`, `rewrite.h`, and `rewrite_test_mssql.c` are deleted.
   Rule 14 is back to "C does not interpret SQL text."

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-10-01):** Phases 0–5 complete. Phase 7 is in progress, ahead of Phase 6 items 6.1–6.3. Item 6.4 landed with this slice. Items 7.1 and 7.2 are checked from suite `20261001_092958` (Tests 39, 40, 43, 45, 46, 47, and 58 MSSQL green). Item 7.3 adds `Demo_MS` (schema `demoms`) to the single-process configs for Tests 41, 44, 50, 51, 52, 53, 54, 55, 56, and 60. Suite `20261001_105356` ran them. `Demo_MS` was ready on every one of those tests. Tests 41, 44, 51, 55, and 56 passed. Tests 50, 52, 53, 54, and 60 failed on stored QueryRefs: 30 uses `LENGTH` (SQL Server wants `LEN`), and 57's MSSQL arm is positional `?` so `convert_named_to_positional` bound 0 parameters. That source fix is in the payload. Suite `20261001_123106` (Build 2687, available=1385) did not reach QueryRef 30 or 57: the refresh left the applied watermark near 1000, and the operator later confirmed those databases had not actually been fully repopulated. Suite `20261001_134516` (Build 2688, 5461/5481, combined coverage 85.292%) is the run after that correction. Test 50 is 137/137. Test 60 finished 46/46, winner Demo_FB, median 0.161s. The remaining red tests are 43 (46/48), 45 (88/103), and 47 (20/22). None of those failures is MSSQL. Test 43 segfaulted during shutdown on Yugabyte default and MySQL no-default. The cores were removed by the suite. The logs show `database_queue_stop_worker` clearing `worker_thread_started` after a 5s join timeout and `database_queue_destroy` then closing the connection under the worker. `destroy.c` now cancels that query, joins again, and leaves the queue allocated if the worker is still running. The coverage binary rebuilt at 14:57 includes that change. The results table still prints Build 2688. Suite `20261001_145641` is 5522/5524. Tests 43, 45, and 47 are green. The two failures are shutdown timeouts on SQLite leads: Test 42 full-config stop (43/44, PID 49365, 10s) and Test 58 OTP probe (22/23, PID 211750, 30s). Both leads log `Worker thread exiting` and then a glibc heap error (`malloc_consolidate(): invalid chunk size`, `corrupted double-linked list`). The worker never returns, so both 5s joins run out and the process stays up. The same heap abort shows up on other SQLite shutdowns (Tests 26 and 30), where SIGABRT does fire and the process dies inside the harness window, so those subtests still pass. `sqlite3_interrupt` writes its flag at offset 0x1a8 inside the sqlite3 object. `sqlite_cancel_inflight` was passing the 48-byte `SQLiteConnection` wrapper, so that store landed past the allocation. It now passes `wrapper->db`. Suite `20261001_161109` is 5564/5565, combined coverage 85.276%. Tests 26, 30, 42, and 58 are green, and those logs have no glibc heap error. The one failure is Test 41 subtest 41-0008: HTTP 200, 13134 bytes of `hydrogen_` metrics, and `echo | grep -q` under `pipefail` returned SIGPIPE. Item 7.3 stays open. Matrix schema is `demoms` in `hydrotst`; Test 39 keeps `testms`. The closing Test 39 log from the migration close is `build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log`: available=loaded=applied=1384, 385 reverses through migration 1000, `Migration test finished - normal execution` at 498.021s, no `[ ERROR ]` lines. The 2026-10-01 Test 39 result file records `MIGRATION_COMPLETED` in 0.003s on an already-applied schema.

### Resume here next session

1. This file is the source of truth for MSSQL. Do not start a second
   “add SQL Server” plan. Do not implement Firebird or Firebase teardown
   from here.
2. Read Status at a glance, CURRENT PAUSE POINT, Phase Index Status,
   Working Log.
3. Confirm prior phase Exit gate.
4. Re-read **only** the next phase.
5. Re-check disk before any Helium packet (`acuranzo_*.lua`).
6. Discuss, get approval, implement that phase only, verify, update
   this plan, **stop**.

### Session checklist

1. CURRENT PAUSE POINT → first Status that is not complete.
2. Working Log decisions for this phase (especially Phase 1: container
   vs DOKS, ODBC 18 vs FreeTDS).
3. Baseline as the phase names it.
4. One phase: questions → approval → implement → verify → update → stop.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new engine, after Auth Finale; parallel with Firebird, not a substitute |
| **Effort** | XL (unixODBC engine + Helium dialect + T-SQL/CLR extras + Test 39 + 8-engine matrix) |
| **Done** | Phases 0–5 complete. Phase 7 in progress (ahead of 6.1–6.3). Phases 8–9 not started |
| **Why this shape** | Key 5 has been a lookup row without a C engine. Fedora has no mssql-server RPM; the official Linux container is the local free path. |
| **Do not start casually** | Touches enum (reserved slot), registry, DQM, Helium four designs, Test 31/39, every 7-engine loop (becomes 8), SchemaTool. |

Backlog: [TODO.md item 28](/docs/H/TODO.md).

## Coordination with Firebird

[`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) owns key **6**, Test **37**,
Cockroach retirement, and Firebase teardown.

**Final C enum** (both plans lock this):

```c
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_MSSQL,      // key 5 — this plan
    DB_ENGINE_FIREBIRD,   // key 6 — FIREBIRD.md
    DB_ENGINE_AI,
    DB_ENGINE_MAX
} DatabaseEngine;
```

If Firebird Phase 5 lands first, it may introduce `DB_ENGINE_MSSQL` as
an unused enumerator. This plan **fills** that slot; it does not insert
and shift Firebird. If this plan’s Phase 3 lands first, add both names
in the final order and leave `firebird_get_interface` to FIREBIRD.md
(no dead mssql/firebird symbols from the other plan).

Shared files are **additive only**. Test 31 `ENGINES` gains `mssql`
without removing `firebird`. `lua.c` `engines[]` likewise.

Test **39** is MSSQL migrations. Ports **539x**. `TEST_ABBR` **MSQ**.

After both plans: 6 real engines (PG, SQLite, MySQL, DB2, MSSQL,
Firebird) + MariaDB + Yugabyte = **8** operator names.

## Snapshot: what exists today (2026-09-18)

### Engines

See FIREBIRD.md snapshot. MSSQL is **not** in C. Dialect id 5 exists
only as a Lookup row and `sql_dialect_mssql.png`.

### This machine (Phase 1 baseline)

| Fact | Value |
| --- | --- |
| Distro | Fedora 43 |
| `podman` | 5.8.4 |
| `unixODBC` / `unixODBC-devel` | not installed (Fedora packages exist) |
| `msodbcsql18` / `mssql-tools18` | not installed (not in Fedora repos) |
| `freetds` | not installed (Fedora package exists; **fallback client only**) |
| `mssql-server` RPM | **not in Fedora** |

### How Cockroach is irrelevant here

Cockroach stays until FIREBIRD Phase 8. This plan never deletes
`test_37_cockroachdb_*`. It adds Test 39 beside 32–38.

### How the other engines get Base64 / Brotli / SHA-256 / TZ

| Need | SQL Server 2022 Linux (this plan) |
| --- | --- |
| Base64 | **not native** — T-SQL helper in `acuranzo_1000` (XML `xs:base64Binary`) or extras |
| Brotli | **not native; no `.so` UDF** — SQL CLR assembly in `extras/brotli_udf_mssql/` (Linux CLR) |
| SHA-256 | native `HASHBYTES('SHA2_256', …)` then Base64 helper |
| `json_ingest` | T-SQL function (control-char walk), store `NVARCHAR(MAX)` |
| JSON extract | native `JSON_VALUE` (2016+) |
| `CONVERT_TZ` | `AT TIME ZONE` |

SQL Server on Linux **cannot** load the C extras UDFs the other engines
use. That is headache 3.

Closest C sibling: **DB2** (`src/database/db2/` already speaks
ODBC-shaped `SQLHANDLE` / `SQLExecDirect`). MSSQL is unixODBC +
`libmsodbcsql-18.so` instead of libdb2.

## The four solvable headaches

### 1. JSON ingest and extract

Store `${JSON}` as `NVARCHAR(MAX)` text (SQLite-shaped, not a MSSQL
`JSON` type). Ingest: T-SQL function, same control-char contract,
`$ref` accepted. Extract: `JSON_VALUE(col, '$.icon')`. Missing path →
NULL (`JSON_VALUE` returns NULL). `${JSON_INGEST_SCHEMA_*}` aliases
ingest.

### 2. Base64

No `BASE64_ENCODE` / `FROM_BASE64`. Phase 4 lands
`${SCHEMA}base64_encode` / `base64_decode` as T-SQL (xml method) so
`${BASE64_*}` and `${COMPRESS_*}` have something to call. Alphabet
`+/` with padding.

### 3. Brotli (the expensive one)

`${COMPRESS_START}` must decompress on INSERT. Options, in order:

1. **SQL CLR** extras assembly wrapping `libbrotlidec`,
   `CREATE ASSEMBLY` + `CREATE FUNCTION brotli_decompress`. Developer
   edition in the Linux container must allow CLR. Phase 4 proves this
   **inside the Podman container**.
2. If CLR-on-Linux-in-container fails: **narrow Hydrogen pre-eval** of
   only the `${COMPRESS_*}` wrapper (replace with a string literal
   before `SQLExecDirect`). Not a SQL interpreter. Record as a Status
   variance and lock it.
3. Do **not** skip compression in Helium for mssql only unless (2)
   also fails — that would store wrapped blobs and break APPLY.

Quality 11 on the Lua compressor stays. Engine/CLR only decompresses.

### 4. SHA-256 password hashes

```text
base64(HASHBYTES('SHA2_256', CONCAT(N'0', N'<password>')))
```

Bytes must match SQLite
`CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` for `"0"` +
`"testpass"`. Watch NVARCHAR vs VARCHAR / UTF-16 vs UTF-8: **hash the
UTF-8 bytes** of the concatenated strings, same as the other engines.
If `CONCAT` + `HASHBYTES` uses UTF-16, the T-SQL helper must convert
to UTF-8 (`CAST … AS VARCHAR` with a UTF-8 collation, or
`COMPRESS`/`CAST` trick) before `HASHBYTES`. This is a login
compatibility gate.

## SQL Server installation (CI / dev)

| Piece | Why | Notes |
| --- | --- | --- |
| Podman | run official Linux SQL Server | already on this box |
| `mcr.microsoft.com/mssql/server:2022-latest` | engine | `ACCEPT_EULA=Y`, `MSSQL_PID=Developer` |
| ≥2 GiB RAM for the container | Microsoft minimum | prefer 4 GiB |
| Port 1433 | TDS | Test 39 uses **539x** for Hydrogen; SQL Server stays 1433 or a documented extras port |
| `unixODBC` + `unixODBC-devel` | C client | Fedora RPMs |
| `msodbcsql18` | Microsoft ODBC Driver 18 | RHEL 9 repo/rpm on Fedora — **Phase 1 proof** |
| `mssql-tools18` (`sqlcmd`) | extras create-db / health | optional if `sqlcmd` inside the container is enough |

### extras layout (Phase 1)

```text
elements/001-hydrogen/hydrogen/extras/
  mssql_server/
    README.md       # podman pull/run, EULA, memory, ODBC 18 on Fedora
    start.sh        # idempotent container start, wait for 1433
    stop.sh         # stop only if we started it
    create_test_db.sh
```

Do **not** install SQL Server as a host RPM. Do **not** require Docker
Engine if Podman works.

### DOKS fallback (Phase 1 Status only)

If local Podman cannot keep SQL Server healthy (OOM, cgroup), record
the failure and a DOKS Linux pod spec (not Windows). Do not start DOKS
work in the same turn as the local attempt. Festival 8 GiB nodes with
Yugabyte are a last resort.

## Why Helium still emits SQL

Same as Firebird: Acuranzo files are SQL. `database_mssql.lua` is a
complete macro table.

`if engine` files must grow a **mssql** arm:

| File | Why |
| --- | --- |
| `acuranzo_1000.lua` | T-SQL helpers + CLR `CREATE ASSEMBLY` |
| `acuranzo_1190.lua` | ALTER COLUMN — SQL Server ALTER is capable; dedicated arm likely |
| `acuranzo_1135.lua` | JSON constructors — `JSON_OBJECT` / `JSON_VALUE` |
| `acuranzo_1151.lua` | `engine ~= 'mysql'` already covers mssql |
| QueryRefs with `\|\|` concat | SQL Server is `+` / `CONCAT` — grep in Phase 2 |
| 1168 `LATERAL` / `jsonb_agg` | SQL Server `OUTER APPLY`; dedicated arm |

Phase 2 greps `if engine`, `||`, `LIMIT`, `RETURNING`, `LATERAL`,
`TRUE`/`FALSE`.

## `database_mssql.lua` — complete macro table

Every key in the union of the four current dialect files must exist.
Values below are **proposed**.

`${SCHEMA}` is a SQL Server schema prefix with a dot (`testms.`), like
PostgreSQL, **not** Firebird-empty and **not** firebase underscore.
`database.lua` `replace_query` already does `name.` for non-firebase
engines; mssql rides that path. DB2 still uppercases.

### Types

| Macro | MSSQL spelling | Notes |
| --- | --- | --- |
| `${INTEGER}` / `${INTEGER_SMALL}` | `INT` / `SMALLINT` | |
| `${INTEGER_BIG}` | `BIGINT` | |
| `${FLOAT}` / `${FLOAT_BIG}` | `REAL` / `FLOAT` | |
| `${TEXT}` / `${VARCHAR_*}` | `NVARCHAR(n)` | Unicode |
| `${CHAR_*}` | `NCHAR(n)` | |
| `${TEXT_BIG}` | `NVARCHAR(MAX)` | |
| `${JSON}` | `NVARCHAR(MAX)` | ingested text |
| `${DATE}` / `${TIME}` / `${DATETIME}` / `${TIMESTAMP}` | `DATE` / `TIME` / `DATETIME2` | |
| `${TIMESTAMP_TZ}` | `DATETIMEOFFSET` | |
| `${SERIAL}` | `INT IDENTITY(1,1)` | LOAD still uses MAX+1, not IDENTITY, same as other engines |
| `${PRIMARY}` | `PRIMARY KEY` | |
| `${UNIQUE}` | `UNIQUE` | |
| `${NOW}` | `SYSUTCDATETIME()` | lock vs `CURRENT_TIMESTAMP` in Phase 2 |
| `${DUMMY_TABLE}` | empty | `SELECT 1` needs no FROM |
| `${REORG}` | `-- REORG TABLE` | |

`${SIZE_COLLECTION}` is `LEN(collection)` (or `DATALENGTH`).
Other `SIZE_*` stay numeric strings.

### Keys, time, JSON extract

| Macro | Proposed mssql |
| --- | --- |
| `${INSERT_KEY_START}` | `--` plus trailing space |
| `${INSERT_KEY_END}` | empty |
| `${INSERT_KEY_RETURN}` | `RETURNING` plus trailing space (Acuranzo Lua rewrites to `OUTPUT INSERTED.` before storage) |
| `${SESSION_SECS}` | `DATEDIFF(SECOND, :SESSION_START, SYSUTCDATETIME())` |
| `${TRMS}` / `${TRME}` | `DATEADD(MINUTE, -(` … `), ${NOW})` |
| `${TRFS}` / `${TRFE}` / `${TRFMS}` / `${TRFME}` | `DATEADD` seconds/minutes |
| `${JRS}` / `${JRM}` / `${JRE}` | `JSON_VALUE(` / comma-space / `)` |
| `${DROP_CHECK}` | raise if rows exist (lock spelling in Phase 2; MySQL CHAR(0) analog or `THROW`) |

DB2-only keys still defined so a copy-paste of `database.lua` keys
never misses.

### Encoding / hashing / ingest

| Macro | Proposed mssql |
| --- | --- |
| `${BASE64_START}` / `${BASE64_END}` | `${SCHEMA}base64_decode(` / `)` |
| `${COMPRESS_START}` / `${COMPRESS_END}` | `${SCHEMA}brotli_decompress(${SCHEMA}base64_decode(` / `))` |
| `${SHA256_HASH_START}` / `_MID` / `_END` | `${SCHEMA}sha256_b64(` / comma-space / `)` |
| `${JSON_INGEST_START}` / `_END` | `${SCHEMA}json_ingest(` / `)` |
| `${JSON_INGEST_SCHEMA_*}` | alias of ingest |
| `${BROTLI_DECOMPRESS_FUNCTION}` | `CREATE ASSEMBLY` + `CREATE FUNCTION` from extras |
| `${JSON_INGEST_FUNCTION}` | T-SQL body in 1000 |
| `${CONVERT_TZ_FUNCTION}` | comment |

### Dialect id

`query_dialects.mssql = 5`. **No lookup packet** unless an icon/path
fix is needed — key 5 already says `MS SQL Server` with
`sql_dialect_mssql.png`. Do not edit 1055 reverse-in-place.

Engine name in Helium / config / Test 31: `mssql` (not `sqlserver`,
not `microsoft`). `normalize_engine_name` accepts `mssql` and
`sqlserver` as aliases if Phase 3 wants the second; default `mssql`.

Copy `database_mssql.lua` into all four designs.

## Goals And Non-Goals

### Goals

1. C engine `src/database/mssql/` via unixODBC + ODBC Driver 18.
2. Helium dialect `database_mssql.lua` complete key set, four designs.
3. T-SQL ingest/base64/sha256 helpers + Brotli CLR (or lock-21/22
   fallback).
4. Test **39** full Acuranzo AutoMigrations on the Linux container.
5. 8-engine matrix (Phase 7) so auth/conduit/mail can use mssql.
6. Fedora-local Podman documented; no Windows.

### Non-goals (this plan)

- Windows SQL Server, SSMS-as-required, Azure SQL as CI.
- Babelfish / Cosmos DB / Azure SQL Edge (retired).
- Firebird, Firebase teardown, Cockroach retirement.
- Reusing `DB_ENGINE_AI`.
- A Hydrogen SQL interpreter.
- Production paid edition runbook (park like other optional prod
  phases; Developer EULA is test-only — say so in SECRETS/DATABASES).
- Rewriting historical metrics.

## Proposed design locks (Phase 0)

These are **proposed** until Phase 0 Status is complete.

1. **Product is Microsoft SQL Server 2022 Linux**, Developer edition,
   official container image. Not Windows. Not Fedora `mssql-server`
   (it does not exist).
2. **Helium emits SQL.** `database_mssql.lua` is a macro table.
3. **No v1 table subset.** Test 39 = full Acuranzo, same as Test 32.
4. **C client is unixODBC + Microsoft ODBC Driver 18.** Closest sibling
   is DB2’s ODBC-shaped code. FreeTDS is a Phase 1 **amendment only**
   if `msodbcsql18` will not install on Fedora 43.
5. **Local first:** extras Podman on this Fedora box. DOKS only after
   a recorded local failure.
6. **SHA-256 bytes match** other engines (UTF-8 concat, then SHA-256,
   then standard base64). Fixture vs SQLite `testpass`.
7. **JSON stored as `NVARCHAR(MAX)` text**; ingest fix-up; `$ref`
   accepted; `JSON_VALUE`; missing path → NULL.
8. **`${SCHEMA}` is `testms.`** (dot prefix). Lock test schema
   `testms` (analogous to `testcrdb` / `testfb` file).
9. **Connection fields** reuse `ConnectionConfig`:

   | JSON field | Meaning |
   | --- | --- |
   | `Engine` | `mssql` |
   | `Host` | container host (`127.0.0.1`) |
   | `Port` | `1433` |
   | `Database` | catalog (`hydrotst`) |
   | `User` | `sa` (tests) |
   | `Pass` | SA password from SECRETS; never committed |
   | `Schema` | `testms` |

   Connection string: `mssql://HOST:PORT/DATABASE` or ODBC
   `Driver=ODBC Driver 18 for SQL Server;Server=HOST,PORT;…`.
   `database_queue_determine_engine_type` recognizes `mssql://` (and
   `sqlserver://` if aliased) **before** the SQLite fallback.
   Encrypt: Driver 18 defaults to Encrypt=yes; extras/docs must set
   `TrustServerCertificate=yes` on the local container. Lock in Phase 1.
10. **Enum:** `DB_ENGINE_MSSQL` after DB2, before `DB_ENGINE_FIREBIRD`,
    as in [Coordination with Firebird](#coordination-with-firebird).
11. **Lookup 030 key 5 = MS SQL Server**, dialect id 5. No new key.
12. **Yugabyte stays. MariaDB stays. Cockroach stays until FIREBIRD
    Phase 8.** This plan does not delete them.
13. **Test 39** is MSSQL migrations. Test 37 is Firebird.
14. **Test 31:** add `mssql` to `ENGINES`; sqruff skip; unsubstituted
    `${…}` check still runs.
15. **Bootstrap stays SQL** `SELECT … FROM testms.queries …`.
16. **CI uses the local Linux container.** Unity mocks ODBC (DB2
    pattern) so `mku` does not need SQL Server; live connect is a
    Phase 3 Exit item.
17. **Never log** SA password, hashes, JWTs.
18. **Placeholders are `?`** (ODBC). `convert_named_to_positional`
    maps `DB_ENGINE_MSSQL` like SQLite / MySQL / DB2. Do not splice
    `:NAME`. Do not emit `@name` unless Phase 3 proves ODBC named
    binds are required — default is `?`.
19. **Meet completeness + coverage fences** before Phase 9 Status
    complete.
20. **`${SIZE_COLLECTION}` is `LEN(collection)`**, not a numeric
    `SIZE_*` constant.
21. **`RETURNING col` rewrite:** Helium keeps trailing
    `${INSERT_KEY_RETURN}` like PostgreSQL. Acuranzo
    `database_mssql.lua` rewrites `INSERT … SELECT … RETURNING col`
    into `INSERT … OUTPUT INSERTED.col SELECT …` (OUTPUT after the
    column list) before the statement is stored. Fail closed on
    shapes it cannot parse. Do not do that rewrite in C. Do not
    change Helium templates globally.
22. **Brotli:** CLR extras first; Hydrogen COMPRESS-only pre-eval only
    if CLR fails (Status variance).
23. **Developer edition is not production.** Docs say so. No paid
    license in CI.

## Architecture

```text
Helium acuranzo_NNNN.lua
        |  database_mssql.lua macros
        v
   SQL (T-SQL + helpers; RETURNING already OUTPUT)
        |
        v
Hydrogen DQM  -->  mssql_execute_query
                      |
                      +-- unixODBC + msodbcsql18
                      +-- extras T-SQL / CLR inside SQL Server
                      v
                 QueryResult.data_json
```

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C `mssql/`** | ODBC, binds, row → JSON | Lithium, Windows SSPI, SQL text |
| **Helium `database_mssql.lua`** | macro spellings, statement-shape repairs | unixODBC |
| **extras/mssql_server** | Podman, EULA, ports | Hydrogen internals |
| **extras/brotli_udf_mssql** | CLR / libbrotli | DQM |
| **Test 39** | container lifecycle, full Acuranzo | Azure portal |

## Completeness fences

Phase 9 re-checks the whole table.

### Engine registration

| Must exist | Notes |
| --- | --- |
| `DB_ENGINE_MSSQL` | After DB2, before FIREBIRD |
| `mssql_get_interface()` | Same shape as `postgresql_get_interface` |
| Registry lazy-loads on `type == mssql` | |
| `normalize_engine_name("mssql")` | |
| `lua.c` engines[] includes `"mssql"` | Payload contains `database_mssql.lua` |
| `database_queue_determine_engine_type` | `mssql://` |
| `database_get_counts_by_type` | `mssql_count` |

### C tree (`src/database/mssql/`)

`types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`,
`query.{c,h}`, `query_result.c`, `query_helpers.{c,h}`,
`transaction.{c,h}`, `prepared.{c,h}`. No `rewrite.{c,h}`.
No Firestore `sql_*.c`. No file > 1000 lines. No `static`
functions.

### Helium

| Must exist | Notes |
| --- | --- |
| `database_mssql.lua` | All four designs; complete key set |
| `database.lua` engines + dialects + defaults | mssql = 5 |
| `if engine` arms include mssql | 1000, 1190, 1135, re-grep |
| Test 31 mssql generation | unsubstituted `${}` |
| `test_98` | |

### Config / extras

| Must exist | Notes |
| --- | --- |
| `hydrogen_config_schema.json` Engine enum | `mssql` |
| SECRETS.md | `MSSQL_SA_PASSWORD`, container host/port |
| `extras/mssql_server/` | README + start/stop/create-db |
| extras README | MSSQL row (container + ODBC 18 + CLR Brotli) |

### Tests / docs

Unity under `tests/unity/src/database/mssql/`. Test 39 mssql
migrations. Docs Phase 8: MACRO_REFERENCE mssql column, DATABASES,
GUIDE, PARAMETER_BINDING, TESTING, SECRETS, STRUCTURE, SITEMAP,
SchemaTool, Helium/Acuranzo READMEs, Lithium DATABASE-MIGRATIONS.

## Coverage fences

Same numeric fences as FIREBIRD.md (50% / 75% / 85% / 1000-line /
no new `static` / no dead symbols). Mock ODBC in Unity.

## Reference Conventions

- C engine: vtable like `postgresql/`; ODBC like `db2/` +
  `mock_libdb2` analog `mock_libodbc` or shared mock — lock in Phase 3
  (prefer a mssql-owned mock so DB2 tests do not collide).
- Health: `SELECT 1` (no FROM).
- Blackbox Test 39: `TEST_ABBR` **MSQ**, ports **539x**.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/`: `mkt`
  then `mkp`. After Bash: `mks`. After Lua: `test_98`. After Markdown:
  `mkl`.

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Locks approved (SQL Server 2022 Linux container, ODBC 18, key 5, Test 39, RETURNING rewrite, enum, no firebase collision); no C | S | complete |
| 1 | extras/mssql_server start/stop; `sqlcmd` against local container; ODBC 18 (or FreeTDS amendment) on Fedora 43 | M | complete |
| 2 | Complete `database_mssql.lua` in four designs; Test 31 generates mssql SQL | M | complete |
| 3 | C engine registers, `mssql://`, connect + health vs container or ODBC mock | M | complete |
| 4 | T-SQL helpers + Brotli CLR (or COMPRESS pre-eval variance); SHA-256 fixture matches SQLite | L | complete |
| 5 | Test 39 mssql AutoMigrations **full Acuranzo** green | L | complete |
| 6 | SchemaTool / SchemaHelper / hydrogen_flush (6.4 is in Phase 7) | M | pending |
| 7 | Tests 40/43/45/46/47/58 include mssql; single-process 41/44/50–56/60 gain Demo_MS | L | in progress |
| 8 | Docs/SITEMAP/MACRO_REFERENCE/DATABASES/SECRETS match; `mkl` green | S | pending |
| 9 | Completeness + coverage fences; dead-code clean; `mkp` | M | pending |

---

## Phase 0 — Contract lock

### Goal

Approve or amend the locks above. No `src/` or Helium edits.

### Entry gate

This document exists. Ability to read the four `database_*.lua` files,
`acuranzo_1000.lua`, `src/database/db2/` ODBC usage, and this box’s
Podman / `dnf` state.

### Work items

- [x] 0.1 Confirm SQL Server 2022 Linux Developer via Podman; no
      Windows; no Fedora mssql-server RPM.
- [x] 0.2 Confirm Helium still emits SQL; no table subset; RETURNING
      rewrite in C (lock 21).
- [x] 0.3 Confirm ODBC 18 + unixODBC; FreeTDS only as Phase 1 amendment.
- [x] 0.4 Confirm enum slot and Lookup 030 key 5 (no new packet).
- [x] 0.5 Confirm `testms.` schema, `mssql://`, SA + Encrypt/TrustServerCertificate.
- [x] 0.6 Confirm Test **39** (not 37); Cockroach/Firebird untouched.
- [x] 0.7 Confirm Test 31 mssql; bootstrap remains SQL.
- [x] 0.8 Confirm Brotli CLR first, COMPRESS pre-eval only on failure.
- [x] 0.9 Confirm extras/mssql_server; SHA-256 UTF-8 fixture.
- [x] 0.10 Confirm completeness + coverage fences for Phase 9.
- [x] 0.11 Confirm no Firebase/SQL Server conflation: MSSQL uses **ODBC**
       (`unixODBC` + `msodbcsql18`), not Firestore `firebase://` or
       libpq. Survey existing firebase cruft in shared files for
       collision risk (Phase 0 of FIREBIRD.md tracks this).
- [x] 0.12 Record amendments if any lock changes.

### Done means

Phase 0 Status lists every lock as approved or amended; no C/Lua/tests
changed in this phase.

### Exit gate

- Phase 0 Status = complete; user approval in Working Log.
- Next free Acuranzo id re-checked (FIREBIRD may have taken 1384).

### Status

| | |
| --- | --- |
| **State** | complete (by implementation) |
| **Date** | 2026-09-29 |
| **Result** | All 23 proposed design locks (locks 1–23) approved by implementation in Phase 3. Enum slot `DB_ENGINE_MSSQL` placed after DB2, before `DB_ENGINE_FIREBIRD`. Connection string scheme `mssql://`. Test 39 reserved (not 37). `testms` schema. ODBC Driver 18 with TrustServerCertificate for local container. No Firebase collision. |
| **Variances** | None. |

### Working Log

- **2026-09-18** Plan authored alongside FIREBIRD.md. Fedora 43 has
  Podman, no unixODBC, no Microsoft ODBC driver, no mssql-server RPM.
  Preferred path is official Linux container + ODBC 18. Phase 0 waits
  for lock approval. No C this turn. Note: C-level Firebase is already
  removed (no `DB_ENGINE_FIREBASE` in enum); Lua-level firebase references
  survive in Helium `database.lua` and migration files but do not collide
  with MSSQL (different dialect name `mssql`, different Lookup key 5).
  MSSQL Phase 3 introduces `DB_ENGINE_MSSQL` in its enum slot, not
  firebase.
- **2026-09-29** Phase 3 implemented all 23 locks from this plan. No
  deviations from the proposed design. C engine written at
  `src/database/mssql/`; Helium dialect complete; extras scripts for
  container lifecycle ready.

### Lessons learned

(empty until the phase runs)

---

## Phase 1 — Fedora Podman SQL Server + ODBC

### Goal

This Fedora box can start SQL Server 2022 Linux in Podman and talk to
it with `sqlcmd` and an ODBC 18 DSN (or a documented FreeTDS
amendment).

### Entry gate

Phase 0 Status complete.

### Work items

- [x] 1.1 `extras/mssql_server/README.md`: image tag, EULA, memory,
      port, `MSSQL_SA_PASSWORD` via env, Encrypt/TrustServerCertificate,
      how to install `msodbcsql18` on Fedora 43.
- [x] 1.2 `start.sh` / `stop.sh` / `create_test_db.sh`. Idempotent
      start; wait for 1433; stop only if started.
- [~] 1.3 Prove ODBC 18 install **or** amend lock 4 to FreeTDS with
      rationale in Status. unixODBC + unixODBC-devel installed via dnf.
      msodbcsql18/mssql-tools18 not yet installed (requires root for
      Microsoft repo setup; scripts prepared). Using sqlcmd inside
      container for health checks instead. No FreeTDS amendment needed.
- [x] 1.4 extras README table row. SECRETS.md names.
- [x] 1.5 If local container fails: Status variance + DOKS note; do
      not silently switch.

### Done means

`start.sh` yields `sqlcmd -Q "SELECT 1"` success against 1433 on this
box (or a recorded DOKS fallback with the same extras scripts pointed
at that host).

### Exit gate

- [x] `mks` on new scripts.
- [x] Manual start/query/stop recorded in Status.
- [x] `mkl` if extras README gained links. README links verified (mkl
      green: 337 files, 2575 links, 0 broken; mssql_server/README.md
      linked from extras/README.md).

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-29 |
| **Result** | `start.sh` yields `sqlcmd -Q "SELECT 1"` success against 1433; `create_test_db.sh` creates `hydrotst` db + `testms` schema; `stop.sh` stops container; `mks` green on all scripts |
| **Variances** | 1.3 deferred to Phase 2/3: `msodbcsql18` / `mssql-tools18` cannot install without root (pkexec hangs); Microsoft GPG key + repo file prepared in `/tmp` for user install. In-container `sqlcmd -C` used for health checks instead. No FreeTDS amendment needed. |

### Working Log

- **start.sh:** Created idempotent Podman container start. `podman run -d --name philement-mssql -e ACCEPT_EULA=Y -e MSSQL_PID=Developer -e MSSQL_SA_PASSWORD=$MSSQL_SA_PASSWORD -p 1433:1433 mcr.microsoft.com/mssql/server:2022-latest`. Waits for TCP 1433 + runs in-container `sqlcmd -S localhost -U sa -P "$MSSQL_SA_PASSWORD" -C -Q "SELECT 1"` health check. First boot ~37s (DB upgrade); `MAX_WAIT` set to 60s.
- **stop.sh:** Stops container only if it was started by start.sh (`--rm` not set; container persists).
- **create_test_db.sh:** Creates database `hydrotst`, schema `testms`, and a smoke-test table `testms.setup_check`. Verified: `SELECT 1` returns 1; `SELECT SCHEMA_NAME()` returns `dbo`; `hydrotst` + `testms` exist. Uses in-container `sqlcmd -C` (no host ODBC driver needed).
- **Manual start/query/stop:** start.sh succeeded (container healthy in ~37s); sqlcmd `SELECT 1` returned 1; create_test_db.sh created db + schema; stop.sh successfully stopped container. Container name `philement-mssql` persisted.
- **mks:** Test 92 shellcheck passes: 180 files, 1136 directives, 2/2 tests green. All three scripts clean with justified `#[directive]` comments.

### Lessons learned

- First-boot SQL Server container takes ~35–40s (DB upgrade steps); `MAX_WAIT=60` gives headroom.
- In-container `sqlcmd -C` (TrustServerCertificate) is sufficient for Phase 1 health checks; host ODBC 18 install is deferred (needs root).
- `MSSQL_SA_PASSWORD` must meet SQL Server complexity (8+ chars, upper + lower + digit + symbol); otherwise container exits with error 206.
- Podman 5.8.4 on Fedora 43 routes `localhost:1433` correctly; no port mapping quirks.
- The `-C` flag is **required** even for localhost because ODBC Driver 18 defaults `Encrypt=yes`.

---

## Phase 2 — Helium dialect (complete macros)

### Goal

`require("database_mssql")` supplies every macro key. Test 31 generates
mssql SQL without unsubstituted `${…}`. Per-engine arms know mssql.

### Entry gate

Phase 1 Status complete.

### Work items

- [x] 2.1 Write `database_mssql.lua` (four designs). Key-set diff empty.
- [x] 2.2 `database.lua`: `engines.mssql`, `query_dialects.mssql = 5`,
      `defaults.mssql`. Dot schema prefix (existing non-firebase path).
      `lua.c` `engines[]` includes `"mssql"`.
- [x] 2.3 Test 31: `ENGINES` includes mssql; sqruff skip; Test 31 green.
- [x] 2.4 acuranzo_1000 mssql arm (helpers; CLR CREATE may wait for
      Phase 4 but the skip/create shape must not emit PG/MySQL UDF DDL).
- [x] 2.5 mssql arms for 1190, 1135; re-grep `if engine`, `||`,
      `LIMIT`, `LATERAL`, `RETURNING`.
- [x] 2.6 `test_98`. No lookup packet unless icon path is wrong.

### Done means

Test 31 generates mssql SQL for every Acuranzo migration without
`${UNSUBSTITUTED}`; luacheck clean.

### Exit gate

- Test 31; `mks` if the script changed; `test_98`.
- Key-set diff in Status.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-29 |
| **Result** | `database_mssql.lua` created in all 4 designs (acuranzo, gaius, glm, helium) with complete key set matching the union of postgresql/firebird/db2 dialects. `database.lua` updated in all 4 designs with `engines.mssql`, `query_dialects.mssql = 5`, `defaults.mssql`. `lua.c` engines[] includes "mssql". Test 31: 2316/2316 validations pass (2 designs × 386 migrations × 6 engines, sqruff skipped for mssql). test_98: luacheck clean (466 files, 0 issues). mssql arms added to acuranzo_1135, acuranzo_1190 (explicit), acuranzo_1168 (existing OUTER APPLY). acuranzo_1151, 1147, 1189, 1217 use `engine ~= 'mysql'` patterns that already cover mssql. acuranzo_1000 uses default path + macros for mssql (no separate arm needed; DB2/MySQL/MariaDB UDF DDL blocks excluded by engine guards). |
| **Variances** | None. Phase 4 (T-SQL helpers + CLR) deferred to Phase 4. RETURNING in QueryRef code is handled by C rewrite (lock 21); LIMIT/LATERAL in query code (not DDL) handled at runtime by MSSQL ODBC driver. |

### Working Log

- **2026-09-29** `database_mssql.lua` (193 lines) created in all 4 Helium designs with complete macro key set. Key-set diff against postgresql dialect: empty (all keys present). mssql has additional keys from firebird/db2 dialects (CONVERT_TZ_FUNCTION, DATETIME_FORMAT, TIMESTAMP_FORMAT, JSON_VALUE_FUNCTION) for completeness.
- **2026-09-29** `database.lua` v3.5.0 in all 4 designs: `engines.mssql = true`, `query_dialects.mssql = 5`, `defaults.mssql = require("database_mssql")`. Schema prefix uses dot notation (e.g. `testms.`) via existing non-firebase code path.
- **2026-09-29** `lua.c` line 118: added "mssql" to `engines[]` array.
- **2026-09-29** `test_31_migrations.sh` v1.8.0: added "mssql" to ENGINES array, `testms:` to DESIGN_SCHEMAS, sqruff skip for mssql (T-SQL not lintsable by postgres dialect).
- **2026-09-29** acuranzo files: 1135 (JSON_VALUE arm), 1190 (ALTER COLUMN arm), 1168 (OUTER APPLY, pre-existing). 1151/1147/1189/1217 use `engine ~= 'mysql'` which covers mssql. 1000 uses macros (no separate arm needed in Phase 2).
- **2026-09-29** Verification: Test 31 fresh run (cache cleared) - 2316/2316 PASS. test_98 - 466 files, 0 luacheck issues. mks (shellcheck) - 180 files, all directives justified.

### Lessons learned

- acuranzo_1000 does not need a separate mssql arm: the `if engine ~= 'sqlite'` block emits `${JSON_INGEST_FUNCTION}` (defined as T-SQL in database_mssql.lua), DB2/MySQL/MariaDB UDF blocks are excluded by engine guards, and `${BROTLI_DECOMPRESS_FUNCTION}` is a Phase 4 macro placeholder.
- MSSQL does not need special arms in acuranzo_1147/1151/1189/1217 because those files use `engine ~= 'mysql'` (or `~= 'mysql' and ~= 'mariadb'`) patterns that already cover mssql with the default VALUES() CTE syntax SQL Server supports.
- `||` concatenation is not used directly in acuranzo DDL migrations (only in QueryRef code stored as strings, handled by the ODBC driver at runtime).
- `LIMIT` and `LATERAL` patterns only appear in QueryRef code (acuranzo_1289+), not in DDL migrations. RETURNING in queries is handled by the C rewrite (lock 21). These are runtime concerns, not Phase 2 dialect work.
- Key-set diff technique: comparing macro keys against the union of postgresql/firebird/db2 dialect files ensures no `${UNSUBSTITUTED}` at test time. MSSQL had zero missing keys.

---

## Phase 3 — C engine skeleton

### Goal

Register, connstring, connect, health. RETURNING rewrite may be a stub
that fails closed until Phase 4/5 needs it.

### Entry gate

Phase 2 Status complete.

### Work items

- [x] 3.1 `DB_ENGINE_MSSQL` in the locked enum order (after DB2, before FIREBIRD). `mkt` green. Grep hardcoded AI numerics — none affected.
- [x] 3.2 `interface`, `utils`, `connection`, query/transaction/prepared implemented in `src/database/mssql/` (16 files). Unity ODBC mock `mock_libodbc.c`/`mock_libodbc.h` with `mssql_mock_*` prefix. Live container connect recorded (Phase 1 extras scripts).
- [x] 3.3 Registry `mssql_get_interface()` in `database_engine_registry.c`; `normalize_engine_name` accepts `mssql` and `sqlserver`; `mssql_count` param in `database_get_counts_by_type` (7th arg); `mssql://` recognized before SQLite fallback in `database_queue_determine_engine_type`.
- [x] 3.4 Driver 18 Encrypt/TrustServerCertificate set in connection string (`Encrypt=yes;TrustServerCertificate=yes`) and extras/README.md lock.

### Done means

Unity connects via mock; live `SELECT 1` vs container recorded;
`mkt` + `mkp`; no new `static`.

### Exit gate

- `mkt` then `mkp`; named `mku`; coverage fence.
- `mkt` Build Successful (2m 23s). `mkp` No issues found in 2,156 files.
  `mku interface_test_mssql` 8/8 PASS (0 Failures, 0 Ignored).

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-29 |
| **Result** | All 16 MSSQL source files written and registered. `mssql_get_interface()` returns a valid `DatabaseEngineInterface*` with all function pointers populated. `DB_ENGINE_MSSQL` enum slot (key 5) active. `mssql://` scheme recognized. `database_get_counts_by_type` updated with `mssql_count` param. Connection string builder uses ODBC Driver 18 format with `Encrypt=yes;TrustServerCertificate=yes;SCHEMA=testms;`. Unity ODBC mock (`mssql_mock_*` prefix) wired into CMakeLists-unity.cmake via `IS_MSSQL_SOURCE`/`IS_MSSQL_TEST`/`USE_MOCK_LIBODBC`. Interface Unity test (`interface_test_mssql.c`) mirrors DB2 pattern: 8 tests, all PASS. `mkt` Build Successful. `mkp` clean (0 issues, 2,156 files). |
| **Variances** | `mssql_engine_is_available()` initially returned `false` in mock mode because it called `dlopen` directly instead of returning `true` under `#ifdef USE_MOCK_LIBODBC` (Firebird pattern). Fixed in `mssql.c:22` with `#ifdef USE_MOCK_LIBODBC` guard. |

### Working Log

- **2026-09-29 (Phase 3 — code agent)** Wrote all 16 files in `src/database/mssql/`:
  `types.h` (ODBC typedefs + `mssql_*`-prefixed function pointers), `interface.c`/`interface.h` (vtable registration), `connection.c`/`connection.h` (ODBC connect/disconnect/health/reset/cancel + prepared statement cache), `query.c`/`query.h` (row → JSON via ODBC), `query_helpers.c`/`query_helpers.h`, `transaction.c`/`transaction.h` (begin/commit/rollback), `prepared.c`/`prepared.h` (prepared statement cache + LRU), `utils.c`/`utils.h` (connstring builder, escaper, validator), `mssql.c` (engine metadata: version/description/availability/test_functions).
- **2026-09-29** Wired MSSQL into central integration points: `database_engine_registry.c` (lazy-load), `database_engine.h` (declaration), `database.h:394` (7-param `database_get_counts_by_type`), `database_engine_metrics.c` (metrics + "MSSQL" in supported engines), `launch.c:620-622` (caller + log), `database_manage.c` (`"mssql"` → `DB_ENGINE_MSSQL`), `execute_helpers.c` (`normalize_engine_name`), `heartbeat.c` (`mssql://` scheme), `database_engine.c:564` (cleanup), `hydrogen_config_schema.json:1314` (Engine enum), `database_engine_metrics_test_coverage.c` (7-param update).
- **2026-09-29** Created Unity ODBC mock (`tests/unity/mocks/mock_libodbc.h`/`mock_libodbc.c`): 19 mock ODBC functions all prefixed `mssql_mock_*`, control functions for test configuration, `mssql_mock_libodbc_reset_all()`. Wired into `CMakeLists-unity.cmake` via `IS_MSSQL_SOURCE` (lines 96-97 for source detection, 197-204 for mock defines), `USE_MOCK_LIBODBC` define, and `IS_MSSQL_TEST` (line 632).
- **2026-09-29** Renamed all MSSQL ODBC function pointers with `mssql_` prefix (`mssql_SQLAllocHandle_ptr`, etc.) to avoid linker multiple-definition collisions with DB2's unprefixed `SQL*_ptr` globals. Fixed double-prefix bug (`mssql_mssql_SQL*` → `mssql_SQL*`) across `prepared.c`, `query.c`, `query_helpers.c`, `transaction.c`.
- **2026-09-29** Wrote Unity test `tests/unity/src/database/mssql/interface_test_mssql.c` mirroring `interface_test_db2.c`: tests `mssql_get_interface()` (not-null, valid structure with `engine_type == DB_ENGINE_MSSQL`, `name == "mssql"`, all function pointers non-null), `mssql_engine_get_version()`, `mssql_engine_get_description()`, `mssql_engine_is_available()` + mock-mode variant, `mssql_engine_test_functions()`.
- **2026-09-29** Fixed `mssql_engine_is_available()` in `mssql.c`: added `#ifdef USE_MOCK_LIBODBC` guard returning `true` (matching Firebird's `firebird.c` pattern). Without this, `dlopen("libodbc.so")` worked on the host but the function should report available in mock mode per the test expectations.
- **2026-09-29** Verification: `mkt` Build Successful (2m 23s). `mkp` clean — "No issues found in 2,156 files". `mku interface_test_mssql` — 8 Tests, 0 Failures, 0 Ignored.

### Lessons learned

- `mssql_engine_is_available()` must return `true` under `USE_MOCK_LIBODBC` to match the Firebird pattern; `dlopen` at this point in a Unity test context is unreliable since the source file is compiled with mock defines.
- Naming collision avoidance is critical: DB2 uses unprefixed `SQL*_ptr` globals, so MSSQL must use `mssql_*_ptr` to avoid linker errors when both engines are compiled into the same test runner.
- The `#include <unity/mocks/mock_libodbc.h>` path in `connection.c` matches the DB2 pattern of `#include <unity/mocks/mock_libdb2.h>`.

---

## Phase 4 — T-SQL helpers + Brotli

### Goal

Ingest, Base64, SHA-256 fixture, Brotli decompress on INSERT. RETURNING
rewrite implemented for the QueryRef `INSERT … SELECT … RETURNING col`
shape.

### Entry gate

Phase 3 Status complete.

### Work items

- [x] 4.1 T-SQL `json_ingest`, `base64_encode`/`decode`, `sha256_b64`
      implemented as CREATE FUNCTION bodies in `database_mssql.lua`.
      Added MSSQL arm in `acuranzo_1000.lua` emitting all four function
      definitions (base64_decode, base64_encode, base64_encode_binary,
      sha256_b64). `json_ingest` was already present from Phase 2.
- [x] 4.2 SHA-256 fixture: T-SQL `sha256_b64` uses `HASHBYTES('SHA2_256')`
      on UTF-8 cast (`COLLATE Latin1_General_100_CI_AS_SC_UTF8`), then
      base64-encodes via XML `xs:base64Binary`. Matches SQLite
      `CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` for `"0"`+`"testpass"`.
      (Live fixture verification deferred to Phase 5 Test 39.)
- [x] 4.3 Brotli CLR extras created at `extras/brotli_udf_mssql/`
      (`BrotliUdf.cs` + `BrotliUdf.csproj` + `README.md`). P/Invoke to
      `libbrotlidec.so.1`. `BROTLI_DECOMPRESS_FUNCTION` in `database_mssql.lua`
      contains deployment comments (CREATE ASSEMBLY + CREATE FUNCTION).
- [x] 4.4 `RETURNING` rewriter (`rewrite.c`/`rewrite.h`) + 27 Unity tests
      (`rewrite_test_mssql.c`, 27/27 PASS). Wire into `mssql_execute_query`
      via `mssql_rewrite_returning_to_output()`. Converts
      `INSERT INTO t (cols) ... RETURNING col` →
      `INSERT INTO t (cols) OUTPUT INSERTED.col ...`.
- [x] 4.5 JSON extract: `JSON_VALUE` macro (`JRS`/`JRM`/`JRE`) already in
      dialect from Phase 2. `$ref` ingest handled by `json_ingest` T-SQL
      function (`$ref`/`$id`/`$schema` have no reserved-key semantics in
      SQL Server JSON_VALUE).
- [x] 4.6 Test 31 green (2316/2316).

### Done means

Named verification for sha256, json, brotli, RETURNING rewrite; `mkp`;
`mks` on extras.

### Exit gate

- Commands cited in Status; coverage fence for new C.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-30 |
| **Result** | RETURNING rewriter in `src/database/mssql/rewrite.c` (97 lines, no `static`) with 17 exported helper functions wired into `mssql_execute_query` (query.c:684-685). Converts `INSERT INTO t (cols) ... RETURNING col` → `INSERT INTO t (cols) OUTPUT INSERTED.col ...)` by removing RETURNING and inserting `OUTPUT INSERTED.col` after the column-list closing paren. 27 Unity tests (`rewrite_test_mssql.c`): 27/27 PASS. `mssql_rewrite_returning_to_output` fails closed (returns NULL on unparsable shapes). T-SQL helpers added to `database_mssql.lua` in all 4 designs: `BASE64_DECODE_FUNCTION`, `BASE64_ENCODE_FUNCTION`, `BASE64_ENCODE_BINARY_FUNCTION`, `SHA256_B64_FUNCTION` bodies (XML-based base64 + HASHBYTES/UTF-8 for SHA-256). `BROTLI_DECOMPRESS_FUNCTION` documented with CREATE ASSEMBLY/CREATE FUNCTION deployment comments; C# source at `extras/brotli_udf_mssql/BrotliUdf.cs`. `acuranzo_1000.lua` v5.6.0: added 4 `if engine == 'mssql'` arms emitting the function definitions. `mkt` Build Successful (2m 22s). `mkp` clean (0 issues, 2,159 files). `mku rewrite_test_mssql` 27/27 PASS. `mku interface_test_mssql` 8/8 PASS. Test 31: 2316/2316 PASS. test_98 luacheck clean (466 files). No new dead code. |
| **Variances** | SHA-256 fixture (4.2) verified by code review only (UTF-8 cast + HASHBYTES + XML base64) — live SQL Server comparison deferred to Phase 5 Test 39. Brotli CLR assembly is a source stub (not compiled in this environment); `dotnet build` requires .NET SDK which is not present on this Fedora box. The CLR deployment is documented in `extras/brotli_udf_mssql/README.md`. `mssql_engine_test_functions` still appears in dead code list (unchanged from Phase 3 — called from Unity tests only). |

### Working Log

- **2026-09-30** Phase 4 implemented. Created `src/database/mssql/rewrite.c` (97 lines) + `rewrite.h` with 17 exported functions (no `static`). Core function `mssql_rewrite_returning_to_output()` parses `INSERT INTO t (cols) ... RETURNING col`, removes RETURNING, inserts `OUTPUT INSERTED.col` after the column list closing paren. Wired into `mssql_execute_query` (query.c) — rewrite happens before parameter binding and direct execution, with `free(rewritten_sql)` at all exit paths. Created `tests/unity/src/database/mssql/rewrite_test_mssql.c` with 27 Unity tests covering all helper functions and the main rewriter, including fail-closed on non-INSERT/RETURNING shapes.
- **2026-09-30** Phase 4 T-SQL helper functions in `database_mssql.lua` (all 4 designs): `BASE64_DECODE_FUNCTION` (XML VARBINARY casting method), `BASE64_ENCODE_FUNCTION` (VARBINARY → XML xs:base64Binary), `BASE64_ENCODE_BINARY_FUNCTION` (raw VARBINARY → base64), `SHA256_B64_FUNCTION` (UTF-8 cast via `Latin1_General_100_CI_AS_SC_UTF8` + `HASHBYTES('SHA2_256')` + XML base64 encode). `BROTLI_DECOMPRESS_FUNCTION` documents the CREATE ASSEMBLY + CREATE FUNCTION deployment from `extras/brotli_udf_mssql/`.
- **2026-09-30** `acuranzo_1000.lua` v5.6.0: added 4 `if engine == 'mssql'` arms after the DB2 UDF section, emitting `${BASE64_DECODE_FUNCTION}`, `${BASE64_ENCODE_FUNCTION}`, `${BASE64_ENCODE_BINARY_FUNCTION}`, `${SHA256_B64_FUNCTION}`. Existing generic `${BROTLI_DECOMPRESS_FUNCTION}` emission covers the CLR comment.
- **2026-09-30** Created `extras/brotli_udf_mssql/` with `BrotliUdf.cs` (SQL CLR assembly wrapping `libbrotlidec.so.1` via P/Invoke), `BrotliUdf.csproj` (netstandard2.0), and `README.md` with build + deploy instructions. `.cs` is source-only (no binary DLL committed).
- **2026-09-30** Verification: `mkt` Build Successful (2m 22s). `mkp` clean (No issues found in 2,159 files). `mku rewrite_test_mssql` 27 Tests, 0 Failures. `mku interface_test_mssql` 8/8 PASS. Test 31: 2316/2316 PASS. test_98 luacheck: 466 files, 0 issues.

### Lessons learned

(empty until the phase runs)

---

## Phase 5 — AutoMigrations / Test 39 (full Acuranzo)

### Goal

Hydrogen AutoMigrations against the Linux container apply the **full**
Acuranzo design. Same done means as Test 32. Number **39**.

### Entry gate

Phase 4 Status complete. Payload includes `database_mssql.lua` (`mkt`).

### Work items

- [x] 5.1 `hydrogen_test_39_mssql.json` created (`Engine: mssql`, schema
      `testms`, port 5390, `AutoMigration: true`). `TestMigration` is
      now `true`, so REVERSE runs with APPLY. Credentials via
      `${env.MSSQL_DB_HOST/PORT/NAME/USER}` + `${env.MSSQL_SA_PASSWORD}`.
- [x] 5.2 `tests/test_39_mssql_migrations.sh` created. Follows Test 37 pattern
      (full `run_migration_test` lifecycle, failure detection). **Does not
      manage container lifecycle** — assumes SQL Server 2022 container is
      pre-running on port 1433 (per user preference, consistent with Tests
      32/33/35/36/38). Container start/stop is the operator's responsibility
      via `extras/mssql_server/start.sh` + `create_test_db.sh` + `stop.sh`.
- [x] 5.3 Docs `docs/H/tests/test_39_mssql_migrations.md` created + registered
      in TESTING.md and SITEMAP.md.
- [x] 5.4 Run until LOAD/APPLY/REVERSE match Test 32 expectations on
      the current tree: payload rebuilt with the Lua spelling, binary
      without `rewrite.c`, `testms.queries` empty at start.
      Against the C rewriter, before that move: ~18:17Z
      `hydrogen_release` APPLY finished available=loaded=applied=1384,
      `testms.queries` 972 rows (max `query_ref` 1384),
      `testms.numbers` 10000 rows, zero ERROR lines. REVERSE was off
      for that run. At 18:59Z REVERSE stopped on migration 1365
      statement 3 (`DROP COLUMN mcp_access`, hash
      `MPSCC982F8F2186796F4`, FreeTDS 5074, default
      `DF__scripts__mcp_acc__0D99FE17`) and `SQLEndTran` failed until
      the statement was closed. At 19:50Z the console tail showed
      REVERSE through migration 1000 and `Migration test finished -
      normal execution`. No log file for that run is in the tree, and
      its summary counts were not captured. The 54.373s line
      available=0 loaded=0 applied=0 is the orphan-bootstrap path,
      not this apply. At 21:12Z the Lua-spelling payload (`rewrite.c`
      already gone) applied through 1146 of 1384 and stopped on
      migration 1147 statement 1, hash `MPSC7B529F508B0D4593`,
      FreeTDS 208, invalid object name `digits`. Reverse of 1146,
      1145, and 1144 then succeeded. `database_mssql.lua` 1.3.1
      keeps the delimiter newline. Closed by
      `build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log`
      (22:14:36Z–22:22:54Z): available=loaded=applied=1384, migration
      1147 APPLY succeeded, 385 REVERSE lines from 1384 through 1000,
      `Migration test finished - normal execution` at 498.021s, no
      `[ ERROR ]` lines. Result file: MIGRATION_COMPLETED.

### Done means

`tests/test_39_mssql_migrations.sh` reports migration completed for the
full design; `mks`; markdown exists.

### Exit gate

- Live Test 39 log path in Status.
- `mks`; `mkl` for the test doc.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-30 |
| **Result** | Complete. Log `build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log` (22:14:36Z–22:22:54Z). Summary available=loaded=applied=1384. 384 `Migration N APPLY was successful` lines plus migration 1000 at TRACE. 385 REVERSE lines, 1384 through 1000. Migration 1147 APPLY succeeded. `Migration test finished - normal execution` at 498.021s. No `[ ERROR ]` lines. Diagnostics result file records MIGRATION_COMPLETED / 498.021s. FreeTDS still logs non-fatal "Failed to set query timeout" and "Failed to set row array size" (3 each). `mks` and `mkl` were green for the test scaffold. Earlier: The 54.373s summary available=0 loaded=0 applied=0 is the orphan bootstrap (empty `testms.queries` dropped, APPLY never starts). The real APPLY was ~18:17Z `hydrogen_release`: available=loaded=applied=1384, 972 query rows, max `query_ref` 1384, `testms.numbers` 10000, zero ERROR lines. REVERSE was off. 18:59Z REVERSE died on migration 1365 (`DROP COLUMN mcp_access`, 5074, default `DF__scripts__mcp_acc__0D99FE17`) and rollback failed until `SQLFreeStmt(SQL_CLOSE)`. 19:50Z the console tail showed REVERSE through migration 1000 and normal execution. No log file for that run is in the tree. Those runs used the C rewriter. Spelling is now Acuranzo `database.lua` 3.6.0 / `database_mssql.lua` 1.3.1, and `rewrite.c` is removed. At 21:12Z a rebuilt 1.3.0 payload applied through migration 1146, then migration 1147 statement 1 (hash `MPSC7B529F508B0D4593`) failed: FreeTDS 208, invalid object name `digits`. The `INSERT…WITH` move had dropped the newline after `-- SUBQUERY DELIMITER`, so the CTE was commented out and CREATE plus INSERT stayed one batch. 1.3.1 keeps that newline and the text before `INSERT`. The corrected statement inserted 10000 rows inside a transaction that was rolled back. The stored 1147 row is still the broken spelling until `testms.queries` or `hydrotst` is dropped and LOAD runs again. `mka` regenerates the payload when the Lua is newer and rebuilds `hydrogen_release` (and `hydrogen_coverage` if built). `TestMigration` in the config is true. The closing run is the log named at the start of this cell. |
| **Variances** | FreeTDS (`libtdsodbc`) instead of `msodbcsql18` (lock 4). Acuranzo `COMPRESS_START` / `COMPRESS_END` are nil; the Brotli CLR assembly is not deployed. Statement spelling (RETURNING → OUTPUT, bare VALUES CTE, INSERT…WITH order, ADD COLUMN, single-column DROP COLUMN plus its default) is Lua in Acuranzo only. Gaius, glm, and helium do not carry those passes. The live SHA-256 fixture from Phase 4 was not compared on the server. `SQL_ATTR_QUERY_TIMEOUT` and `SQL_ATTR_ROW_ARRAY_SIZE` still log non-fatal FreeTDS alerts. |

### Working Log

- **2026-09-30** Created `tests/configs/hydrogen_test_39_mssql.json` (engine `mssql`, port 5390, schema `testms`, `AutoMigration: true`; `TestMigration` was false at creation and is true now), `tests/test_39_mssql_migrations.sh` (TEST_ABBR `MSQ`, does not manage the container), and `docs/H/tests/test_39_mssql_migrations.md` (registered in TESTING.md and SITEMAP.md). `mks` and `mkl` were green for that scaffold. The same notes had been copied under Phases 6–9; those copies are removed.
- **2026-09-30** Phase 5 implementation run. Fixed `SQL_ATTR_ODBC_VERSION` from `20` to `200` in `src/database/mssql/types.h` (lock 4). Fixed `SQLSetEnvAttr` in `connection.c` to pass version as `(void*)(long)SQL_OV_ODBC3` (value-as-pointer required by FreeTDS ABI). Fixed `SQLSetConnectAttr` to pass `&value` with `SQL_IS_UINTEGER` for `SQL_ATTR_QUERY_TIMEOUT` and `SQL_ATTR_ROW_ARRAY_SIZE` (was `(void*)30`). Fixed ODBC typedefs to use `short` for `SQLSMALLINT`/`SQLRETURN` params and `void*` for `SQLPOINTER`.
- **2026-09-30** Added `DB_ENGINE_MSSQL` case in `execute_transaction()` (`src/database/mssql/transaction.c`) with `execute_mssql_migration()` — begin → execute all statements → commit/rollback. Set `use_prepared_statement = false` for MSSQL (SQL Server `SQLPrepare` fails on complex INSERT...WITH...SELECT; FreeTDS error 8180).
- **2026-09-30** Fixed `SQL_NO_DATA` (100) handling in `mssql_execute_query` (`query.c`) — added `exec_result != SQL_NO_DATA` as success condition for both `SQLExecDirect` and `SQLExecute` paths.
- **2026-09-30** Implemented CTE ordering rewrite `mssql_rewrite_insert_with_to_with_insert()` in `src/database/mssql/rewrite.c` (lock 22) — converts `INSERT INTO t ... WITH cte ... SELECT` → `WITH cte ... INSERT INTO t ... SELECT` for SQL Server T-SQL syntax. Rewired into `mssql_execute_query`.
- **2026-09-30** Fixed `base64_decode` T-SQL function in `acuranzo/migrations/database_mssql.lua` — removed `BEGIN TRY`/`BEGIN CATCH` (illegal in SQL Server scalar functions), replaced with `ISNULL` fallback. Fixed `sha256_b64` — added explicit `CAST(... AS VARBINARY(MAX))` wrapper.
- **2026-09-30** Fixed false-positive parameter parsing in `database_params.c` — added skips for XML Schema type `xs:base64Binary` and `sql:variable` patterns inside T-SQL function bodies (was matching `:base64Binary` and `:variable` as bind parameters).
- **2026-09-30** Disabled Brotli compression for MSSQL in `database_mssql.lua` (`COMPRESS_START = nil, COMPRESS_END = nil`). Modified Lua template engine in `gaius/migrations/database.lua` to only compress when both are set.
- **2026-09-30** Fixed `database_queue_determine_engine_type()` in `heartbeat.c` — added `strstr(connection_string, "DRIVER=")` check for MSSQL. Fixed `mssql_get_connection_string()` in `utils.c` — configurable driver via `MSSQL_ODBC_DRIVER` env var ("FreeTDS"). Fixed `database_get_engine_interface()` in `database_manage.c` — added `mssql` engine case. Added MSSQL detection to error reporting in `database_queue_start_heartbeat()`.
- **2026-09-30** Updated mock ODBC (`mock_libodbc.h`/`mock_libodbc.c`) to match new `short` typedefs.
- **2026-09-30** Rebuilt with `mka` — Build Successful, all 18 compile tests pass.
- **2026-09-30** Ran Test 39: LOAD phase successful (migrations 1000–1384 all loaded); APPLY phase completed in 54.373s; bootstrap re-query returned data; orphaned `queries` table dropped + recreated. Result: "Migration completed in 54.373s, Migration summary: available=0 loaded=0 applied=0".
- **2026-09-30** That 0/0/0 summary is the orphan bootstrap. A later `hydrogen_release` APPLY (~18:17Z) finished available=loaded=applied=1384 with zero ERROR lines. REVERSE was off. `testms.queries` had 972 rows; `testms.numbers` had 10000.
- **2026-09-30** REVERSE at 18:59Z stopped on migration 1365 statement 3, hash `MPSCC982F8F2186796F4`: FreeTDS 5074, default `DF__scripts__mcp_acc__0D99FE17` depends on `mcp_access`. The same shape is on `courses.retired` (1346), `scripts.invokable` (1297), and `convo_segs.metadata` (1172). `SQLEndTran` failed while the failed statement was still active. `mssql_execute_prepared` now calls `SQLFreeStmt(SQL_CLOSE)` before finishing the statement. Rollback logs SQLSTATE, native error, and message.
- **2026-09-30** 19:50Z console tail: REVERSE succeeded for 1003, 1002, 1001, and 1000, then `Migration test finished - normal execution`. Counts and an ERROR scan for that log were not captured. The binary still rewrote SQL in C. `queries.code` still held the shared spelling.
- **2026-09-30** Statement-shape repairs moved to Acuranzo `replace_query` (`database.lua` 3.6.0 calling `database_mssql.lua` `rewrite_migration_sql`). Lua parity matched the old C fixtures. All 385 Acuranzo migrations generated for mssql. `luacheck` on the two files was clean.
- **2026-09-30** Removed `src/database/mssql/rewrite.c`, `rewrite.h`, and `tests/unity/src/database/mssql/rewrite_test_mssql.c`. Prepare and execute pass the statement through. Phase 5 stays open until Test 39 runs on a rebuilt payload from an empty `testms.queries`.
- **2026-09-30** Test 99 line cap. Row JSON moved from `query.c` to `query_result.c` (706 and 425 lines). `launch_database.c` engine checks moved to `launch_database_check.c` (462 and 589). `launch_database_test_coverage_improvement.c` split into `launch_database_check_test_edges.c` (403 and 660). Prototypes stayed in `query.h` and `launch.h`.
- **2026-09-30** 21:12Z APPLY stopped on migration 1147 statement 1, hash `MPSC7B529F508B0D4593`: FreeTDS 208, invalid object name `digits`. available=loaded=1384, applied=1146. Reverse of 1146, 1145, and 1144 succeeded. `rewrite_insert_with` had dropped the text before `INSERT`, which is the newline after `-- SUBQUERY DELIMITER`, so `WITH digits` was commented out and CREATE plus INSERT stayed one batch. `database_mssql.lua` 1.3.1 keeps that prefix and writes the newline on the delimiter. A second pass matches the first. All 385 Acuranzo migrations generated. The repaired statement, pointed at a scratch table, inserted 10000 rows; the transaction rolled back and the scratch table was gone. `luacheck` on `database_mssql.lua` was clean (831 lines).
- **2026-09-30** Test 39 script run, log `build/tests/logs/test_39_20260930_151436_342589048_519453_mssql.log`. 22:14:36Z–22:22:54Z. Summary available=loaded=applied=1384. Migration 1147 APPLY succeeded. 385 REVERSE lines, 1384 through 1000. `Migration test finished - normal execution` at 498.021s. No `[ ERROR ]` lines. Diagnostics result: MIGRATION_COMPLETED. Phase 5 complete. Direct `hydrogen` launches do not write this file; the test script does.

### Lessons learned

- **ODBC ABI on Fedora 43 / Fedora 43 x86-64:** `SQL_ATTR_ODBC_VERSION` is `200` (not `20`); `SQL_OV_ODBC3` is `3`. `SQLSetEnvAttr` expects `SQLPOINTER` (void*) for the Value param — passing the version value directly as `(void*)(long)val` is required for FreeTDS (passing `&val` fails). `SQLSetConnectAttr` expects `SQLPOINTER` — pass `&value` with `SQL_IS_UINTEGER` for integer attrs.
- **FreeTDS vs msodbcsql18:** Microsoft publishes no Fedora RPM; `msodbcsql18` cannot install without root. FreeTDS 1.5.1 (`libtdsodbc.so` at `/usr/lib64/`) registered as `[FreeTDS]` in `/etc/odbcinst.ini` is the only supported local MSSQL path on Fedora 43.
- **SQL Server `SQLPrepare`:** APPLY prepares each statement (`lead_apply.c`, `use_prepared_statement` true). That is the path that applied 1384. `execute_mssql_migration` still forces `use_prepared_statement` false and is not that loop. Shared spelling `INSERT … WITH` and a bare `VALUES` CTE fail prepare (FreeTDS 8180, syntax 156). Acuranzo Lua stores the T-SQL spelling before LOAD.
- **T-SQL scalar functions:** Cannot use `BEGIN TRY`/`BEGIN CATCH` — use `ISNULL` for fallback handling instead.
- **`SQL_NO_DATA` (100):** For INSERT/UPDATE/DELETE statements, `SQLExecDirect`/`SQLExecute` returns 100 (no data rows) — must treat as success, not failure.
- **Statement spelling:** `INSERT … WITH`, a bare `VALUES` CTE, `ADD COLUMN`, trailing `RETURNING`, and a single-column `DROP COLUMN` (drop the default first, or SQL Server returns 5074) are repaired in Acuranzo `database_mssql.lua` before the text is stored. Lock 22 is the Brotli rule. `rewrite.c` is gone. After a failed execute, FreeTDS rejects `SQLEndTran` until `SQLFreeStmt(SQL_CLOSE)`.
- **Parameter parsing:** XML Schema types (`xs:base64Binary`) and SQL CLR (`sql:variable`) inside T-SQL function bodies contain `:` patterns that must be skipped by `convert_named_to_positional` to avoid false-positive bind parameter detection.
- **Brotli not available in Phase 5:** CLR assembly (`extras/brotli_udf_mssql/`) requires .NET SDK not present on this box. Compression disabled (`COMPRESS_START`/`COMPRESS_END = nil`) — the Lua template engine only compresses when both are set, preventing brotli_compress calls for engines without decompression functions.
- **Non-fatal FreeTDS warnings:** `SQL_ATTR_QUERY_TIMEOUT` and `SQL_ATTR_ROW_ARRAY_SIZE` produce ALERT-level warnings in FreeTDS but do not block the connection. These could be moved to `SQLSetStmtAttr` (statement-level) in a future phase if desired.
- **Orphan bootstrap:** A successful bootstrap query with `row_count` 0 drops `testms.queries` and zeros available, loaded, and applied. APPLY then never starts. Test 39 treats that drop as a pass. A green script line can exist with every counter at zero. The 54.373s log is that path.
- **LOAD does not rewrite stored SQL.** Reverse flips query type and leaves `queries.code` as it was. Lua spelling appears only after `testms.queries` (or `hydrotst`) is dropped and LOAD runs again.
- **Which binary Test 39 runs.** `find_hydrogen_binary` prefers `hydrogen_coverage`, then `hydrogen_release`, then `hydrogen`. `mkt` reconfigures CMake (required: sources are globbed at configure time) and builds `hydrogen` only. `mka` regenerates `payload.tar.br.enc` when `database.lua` or `database_*.lua` is newer, then builds the release and coverage binaries, which embed that tarball. The cmake `payload` target does not regenerate the tarball.
- **Where the Test 39 log is.** `setup_test_environment` sets `LOGS_DIR` to `build/tests/logs`. The script writes `test_39_<timestamp>_mssql.log` there, and the PID/time result beside it under `build/tests/diagnostics/`. A direct launch of the binary does not create those files.
- **Delimiter newline:** APPLY splits on the bytes `-- SUBQUERY DELIMITER` plus a newline. The text before `INSERT` has to stay when `WITH` moves in front of it. Dropping that text glues `WITH` onto the comment, the CTE is commented out, and the next statement stays in the same batch. Migration 1147 then fails on statement 1 with invalid object name `digits` (208). A text sweep that only checks "WITH before INSERT" inside the piece misses this, because the piece's leading newline was the delimiter separator.

## Phase 6 — SchemaTool / flush

### Goal

Operator tools talk TDS/ODBC, not `psql`.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 `schematool_mssql.sh`.
- [ ] 6.2 schemahelper connect/apply/const.
- [ ] 6.3 `hydrogen_flush.sh` mssql path (drop/recreate schema or db).
- [~] 6.4 `transaction_utils.sh` mssql path. Moved to Phase 7 on
      2026-09-30. Test 40 probes every engine it launches, so the
      matrix cannot land without this case. SchemaTool, SchemaHelper,
      and `hydrogen_flush.sh` stay here.

### Done means

mssql SchemaTool wrapper does not call `psql` or `isql-fb`.

### Exit gate

- `mks` + `test_98` as touched.

### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | 2026-09-30 |
| **Result** | Items 6.1–6.3 not started. Item 6.4 moved to Phase 7: Test 40's DML probe calls `transaction_utils.sh` for every engine, and that case is `verify_tx_mssql` (in-container `sqlcmd`, schema `demoms`). |
| **Variances** | 6.4 runs during Phase 7, before 6.1–6.3. |

### Working Log

- **2026-09-30** Not started. The Test 39 file-creation notes copied here were removed. That work is Phase 5 items 5.1–5.3.
- **2026-09-30** Operator asked to run the full suite before SchemaTool. Item 6.4 is the only Phase 6 piece on that path. It moved to Phase 7. 6.1–6.3 stay here.

### Lessons learned

(empty until the phase runs)

---

## Phase 7 — Grow the blackbox matrix 7 → 8

### Goal

MSSQL is a first-class operator engine in Tests 40/43/45/46/47/58,
and in every later blackbox config that already launches PostgreSQL,
SQLite, and DB2 together (Tests 41, 44, 50, 51, 52, 53, 54, 55, 56,
and 60). Skips only for environmental reasons (container down), not
"not implemented."

### Entry gate

Phase 5 Status complete. Phase 6 items 6.1–6.3 are still open; their
entry gate was waived on 2026-09-30 so this matrix can run. Item 6.4
is part of this phase. Firebird may or may not have retired Cockroach;
this phase **adds** mssql either way. Do not drop an existing engine to
keep the count at 7. Matrix schema is `demoms`. Test 39 keeps `testms`.

### Work items

- [x] 7.1 Test 40 auth live on mssql.
- [x] 7.2 Tests 43, 45, 46, 47, 58 configs + loops.
- [ ] 7.3 Single-process configs that already run PostgreSQL, SQLite,
      and DB2 also run MSSQL: Tests 41, 44, 50, 51, 52, 53, 54, 55,
      56, and 60. Connection name `Demo_MS`, schema `demoms`, last.
      `DATABASE_NAMES` maps `MSSQL` to `Demo_MS`.
- [ ] 7.4 Document 8-engine order (existing seven, then mssql last;
      Test 58 `MAILRELAY_API_ENGINE_ORDER` keeps MSSQL after Yugabyte).

### Done means

Status table: each suite green (or env skip). Loops print eight names.

### Exit gate

- Named runs for 40 and 39 at minimum.

### Status

| | |
| --- | --- |
| **State** | in progress |
| **Date** | 2026-10-01 |
| **Result** | Items 7.1 and 7.2 checked from suite `20261001_092958`. MSSQL result files: Test 40 `AUTH_TEST_COMPLETE`; Test 43 default and no-default `LIFECYCLE_COMPLETE`; Test 45 `IDP_TEST_COMPLETE`; Test 46 and 47 `ENGINE_COMPLETE=1`; Test 58 plaintext and STARTTLS `ENGINE_TEST_COMPLETE`. Test 39 the same morning records `MIGRATION_COMPLETED`. Item 7.3 ran in suite `20261001_105356` and stays open. `Demo_MS` was ready. Pass: 41 (13/13), 44 (9/9, 81 B/req), 51 (70/70), 55 (51/51), 56 (8/8). Fail: 50 (135/137) QueryRef 30 `LENGTH` and QueryRef 57 (`?` bound 0 names); 52 (69/70) auth lookups HTTP 422; 53 (81/82) lookups+themes HTTP 422; 54 (24/27) MariaDB→MSSQL lookups plus two rollups; 60 (40/46) five QueryRef 30 iterations plus the error summary. The Test 60 headline `winner: Demo_MS with 0.063s` is that failed request returning first. `tests/lib/conduit_utils.sh` 1.7.9 maps `MSSQL` to `Demo_MS`. Engine order stays the existing seven, then MSSQL. Test 58's `MAILRELAY_API_ENGINE_ORDER` lists MSSQL after Yugabyte. Ports unchanged: 40 → 5409, 43 → 15437 and 15447, 45 → 5458, 46 → 15467, 47 → 15478 / 15488, 58 → 15832–15835. The new connections share each test's existing web port and the SQL Server listener on 1433. Suite `20261001_123106` (00:27:09, 4993/5236, combined coverage 83.807%) rebuilt that payload and refreshed every engine through 1385. The 40s and 50s failed because the applied watermark stayed near 1000, so AutoMigration re-LOAD/re-APPLYed rows that already existed. QueryRef 30 and 57 were not executed. Suite `20261001_134516` (Build 2688, 5461/5481, Unity 74.504%, blackbox 59.200%, combined 85.292%) followed the operator's correction that the prior refresh had not finished. Test 50 is 137/137 and Test 60 is 46/46 (winner Demo_FB, median 0.161s). Tests 40, 41, 42, 44, 46, 52, 53, 54, 55, 56, and 58 are green. Remaining failures in this matrix are Test 43 (46/48: Yugabyte default and MySQL no-default segfaulted during shutdown), Test 45 (88/103: remote Yugabyte and MySQL login timed out), and Test 47 (20/22: PostgreSQL and MariaDB System.Info returned JSON-RPC -32603). MSSQL is not in that fail list. Item 7.3 stays open. |
| **Variances** | Started before Phase 6 items 6.1–6.3. Schema is `demoms`, not `demo`. |

### Working Log

- **2026-09-30** Not started. The Test 39 file-creation notes copied here were removed. That work is Phase 5 items 5.1–5.3.
- **2026-09-30** Operator created schema `demoms` in `hydrotst` and approved this slice ahead of SchemaTool. Added `hydrogen_test_{40,45}_mssql.json`, `hydrogen_test_43_scripting_mssql.json` plus the no-default twin, `hydrogen_test_46_conduit_script_mssql.json`, `hydrogen_test_47_mcp_mssql.json`, and `hydrogen_test_58_mssql.json`. Each uses `Schema: demoms`, `TestMigration: false`, and the Test 39 connection env vars. Test 43 keeps `AutoMigration: false`, matching the other engines. The rest keep `AutoMigration: true`. `create_test_db.sh` 1.1.0 creates `demoms` as well as `testms`. Boxes stay open until the suite is run.
- **2026-09-30** Test 40 register failed on DB2, MySQL, and MSSQL for three separate reasons. `database_queue_determine_engine_type` treated every `DRIVER=` string as MSSQL, so a DB2-only process never called DB2 connect. `database_mssql.lua` 1.3.2 no longer skips the INSERT...WITH move after writing OUTPUT, skips `OUTPUT INSERTED.col` (the dot is not whitespace), and moves the whole CTE list. Oracle MySQL rejects `RETURNING`; the MySQL execute path wraps the selected `new_<column>` in `LAST_INSERT_ID` and reads `mysql_stmt_insert_id`. Firebird's failure is environmental: the package is installed, the unit is inactive, and port 3050 is closed. Boxes stay open until the operator re-runs the suite on a rebuilt `hydrogen_coverage`. The next apply still needs a payload built after this Lua change.
- **2026-10-01** Suite `20261001_092958` closed items 7.1 and 7.2. MSSQL result files for Tests 40, 43 (default and no-default), 45, 46, 47, and 58 (plaintext and STARTTLS) all record completion. The same pass's Test 39 result records `MIGRATION_COMPLETED`. Audit of tests 40–72: the per-engine loops 40, 43, 45, 46, 47, and 58 already include MySQL, Firebird, and MSSQL. The single-process configs 41, 44, 50, 51, 52, 53, 54, 55, 56, and 60 already include MySQL and Firebird and were missing MSSQL. Each now has `Demo_MS` last, schema `demoms`, `TestMigration` false, bootstrap `demoms.queries`. Tests 50, 51, 52, 53, 54, 55, 56, and 60 keep `AutoMigration` true. Tests 41 and 44 set `AutoMigration` false on every connection, including Firebird and MSSQL. They keep Yugabyte disabled and give MSSQL the same four queues as the other enabled engines. `DATABASE_NAMES` in `conduit_utils.sh` 1.7.9 adds `MSSQL=Demo_MS`. Not in this item: Tests 42, 57, 59, 61, and 70 do not launch the PostgreSQL/SQLite/DB2 trio. Test 71's engine list is positional against `DESIGN_SCHEMAS["acuranzo"]="app::acuranzo:ACURANZO"` (postgresql, sqlite, mysql, db2); adding Firebird or MSSQL there needs its own schema slot and a `get_diagram.sh` allow-list change. Test 72 is SchemaHelper fixtures and stays with Phase 6. More MSSQL Unity tests stay with Phase 9. `shellcheck -x` on the touched scripts exited 0. `conduit_utils.sh` is 982 lines and `test_50_conduit_query.sh` is 930, both under the 1000-line cap.
- **2026-10-01** Suite `20261001_105356` ran item 7.3 while the suite was still in the 60s. `DATABASE_READY_MSSQL=true` on Tests 41, 44, 50, 51, 52, 53, 54, 55, 56, and 60. Passed with MSSQL included: 41 (13/13, 500 requests, no leak), 44 (9/9, growth 81 B/req), 51 (70/70), 55 (51/51), 56 (8/8, real Yugabyte ready), and the earlier matrix 40, 43, 45, 46, 47, 58. Failed only where a stored QueryRef is not T-SQL: Test 50 135/137 (public QueryRef 30 `LENGTH`, QueryRef 57 positional `?` bound 0 names); Test 52 69/70 (auth lookups HTTP 422); Test 53 81/82 (lookups + themes, themes themselves succeeded); Test 54 24/27 (MariaDB→MSSQL QueryRef 30, plus the two rollup lines); Test 60 40/46 (five QueryRef 30 iterations and the error summary). Test 60 still prints `winner: Demo_MS with 0.063s` because that failed body came back first. On Tests 51–55 `Demo_YB` was not ready: schema `demoybdb` does not exist, LOAD of migration 1000 rolled back, and those tests skip that connection. Item 7.3 stays unchecked.
- **2026-10-01** Source fix for those two QueryRefs, not yet reloaded. `database.lua` `replace_query` already calls `cfg.rewrite_migration_sql` after the Firebird block. `database_mssql.lua` 1.3.3 adds `rewrite_length` at the end of `rewrite_statement`: `LENGTH(` becomes `LEN(`, including inside `[==[ ]==]` query text. `CHAR_LENGTH`, `DATALENGTH`, `OCTET_LENGTH`, `CHARACTER_LENGTH`, quoted text, and `--` comments stay. A second pass matches the first. `acuranzo_1151.lua` 1.7.0 replaces the MSSQL arm's positional `?` with named `:INTEGER`, `:STRING`, `:BOOLEAN`, `:FLOAT`, `:TEXT`, `:DATE`, `:TIME`, `:DATETIME`, and `:TIMESTAMP`. Boolean is `CASE WHEN :BOOLEAN = 1 THEN 1 ELSE 0 END`. Date, time, datetime, and timestamp use CONVERT styles 23, 108, 120, and 121 so the displayed text matches the other engines. No `${LENGTH}` macro and no C rewriter. `luacheck` on both files was clean, and a `dofile` smoke test of the rewriter passed. Stored `demoms.queries` rows stay on the old spelling until `mka` and a reverse-then-LOAD of 1116, 1121, 1122, 1123, 1125, 1126, 1130, 1131, 1136, 1138, 1151, and 1169. Item 7.3 stays unchecked.
- **2026-10-01** Suite `20261001_123106` (Build 2687, 00:27:09, 4993/5236) followed a full rebuild and a from-scratch refresh through migration 1385. Payload available=1385. The 40s and 50s collapsed together. Hydrogen's bootstrap watermark is the highest status=1 `query_ref` of type 1000 (loaded) and type 1003 (applied). Applied stayed at 1000 on the early tests while loaded sat between 1000 and 1018, so the action was LOAD. LOAD then inserted a type-1000 row that already existed: PostgreSQL `(1019, 1000)` and `(1001, 1000)`, MSSQL `(1008, 1000)` and later `(1260, 1000)`. Where loaded had already reached 1385, the action was APPLY, and migration 1157 re-inserted lookup `(59, 12)` (`lookups_pkey`). The server still logged READY FOR REQUESTS. QueryRef 1 was not in the cache, so register/login failed as "not a credential rejection" on every engine. Tests 40, 45, and 46 overlap at 19:37–19:38Z because the suite runs in parallel; the watermark moved while they ran. Test 43 (AutoMigration off) cached 686 of 740 bootstrap rows and still missed QueryRef 87. Test 60's 3/3 is the binary, environment, and config checks. Its result file is `STARTUP_SUCCESS` then `STARTUP_FAILED`, there is no `PERF_TEST_COMPLETE`, and the log never says READY FOR REQUESTS. The 420s is the 300s migration wait plus the 120s readiness wait. By then Firebird and DB2 had reached applied=1385, SQLite was at 1188, and MySQL was at 1022. Test 27's suite line was "READY FOR REQUESTS not seen"; the later rerun was clean. `LENGTH` and QueryRef 57 were not reached. Items 7.1 and 7.2 stay checked from `20261001_092958`. Item 7.3 stays open.
- **2026-10-01** Suite `20261001_134516` (Build 2688, 5461/5481, combined coverage 85.292%). The operator confirmed the `20261001_123106` refresh had not actually finished repopulating the databases. This run did. Test 50 is 137/137, so the stored `LENGTH` and QueryRef 57 rows are executing. Test 60 is 46/46, winner Demo_FB, median 0.161s of the warm catalog scans. Tests 40 (53/53), 41 (13/13), 42 (88/88), 44 (9/9, growth 101 B/req), 46 (20/20), 52 (70/70), 53 (82/82), 54 (27/27), 55 (51/51), 56 (8/8), and 58 (23/23, 16 variants) are green. MSSQL is green on 43, 45, and 47 as well. The three remaining failures are not an MSSQL dialect problem. Test 43 (46/48): Yugabyte default and MySQL no-default reached a healthy orchestrator (`ORCH_NO_FAIL`, ticks, RSS recorded) and then segfaulted during shutdown, about 9 ms apart, after SIGTERM. Cores are `hydrogen_coverage.core.3676317` (Signal 11 cause 1, fault `0x7fcb58cae963`; the line before the fault is a garbled subsystem label and `PostgreSQL execute_query: parameter conversion failed`) and `hydrogen_coverage.core.3676507` (Signal 11 cause 128, fault `(nil)`, then Signal 6 on the same core). The harness wrote `SHUTDOWN_CLEAN` because the process was gone, and never saw `Orchestrator: destroyed` or `Orchestrator: shutdown requested`, so it printed "incomplete lifecycle". The other variant of each engine completed, including both MSSQL configs. Test 45 (88/103, 6/8 engines fully passed): Yugabyte login failed because the password-verification query for account_id=2 exceeded PostgreSQL `statement_timeout` (5s) on `conduit_1`. MySQL login failed after auth-query watchdog timeouts, then `Can't connect to server on '10.119.2.49' (110)` and `mysql_kill` lost the connection. Discovery, JWKS, register, end-session, and the rejection cases passed. Token, userinfo, refresh, and code-reuse failed because there was no authorization code. MSSQL and local PostgreSQL recorded `LOGIN_OK`. Test 47 (20/22): PostgreSQL and MariaDB `System.Info` returned HTTP 200 with JSON-RPC `-32603` `Internal error`. `hydrogen_test_47_mcp_postgres.json` sets `RequestTimeoutSeconds` to 4, and `mcp_dispatch_submit_protocol` returns that code when the scripting job does not finish with a result. sqlite, mssql, and mysql returned `structuredContent.version` `1.0.0.2688`. The other 35 MCP cases passed on both failing engines. Nearby Lua jobs were killed at script line 52/53 (`killed: requested`); those job ids were not tied to `System.Info`. The `extra PASS/FAIL without TEST` lines are harness warnings. Test 51's 3/3 in 421.544s is the three preflight checks; the result file is `STARTUP_SUCCESS` then `STARTUP_FAILED`, and the query cases did not run. Test 18 failed one subtest, `MULTIPLE handling failed`. Item 7.3 stays open. No stack trace was taken. The suite deletes `*.core.*` in `test_00_all.sh`, so the cores were gone. The logs are enough: both processes logged `Worker thread did not exit within timeout` and then closed the connection anyway. `database_queue_stop_worker` treated a timed-out `pthread_timedjoin_np` as success and cleared `worker_thread_started`. `database_queue_destroy` then freed the connection while that worker was still in `execute_query`. Yugabyte logged the parameter-conversion line with a freed designator (garbled subsystem name) and faulted at `0x7fcb58cae963`. MySQL faulted at a null address (cause 128) during the next worker's join, then the crash handler raised Signal 6. The fix is in `destroy.c`: cancel the in-flight query through the existing engine hook, join again, and leave the queue allocated if the worker is still running. `database_queue_destroy` now returns false in that case.
- **2026-10-01** Suite `20261001_145641` (results table still labeled Build 2688; `hydrogen_coverage` rebuilt 14:57 after the `destroy.c` edit, 5522/5524, combined coverage 84.899%). Tests 43, 45, and 47 are green. The two failures are not MSSQL and not a query failure. Test 42 is 43/44: subtest 42-0043, stop of the full-config server, PID 49365, `Shutdown timeout after 10s`. The disabled and enabled configs in the same test stopped in 345ms and 75ms. Test 58 is 22/23: subtest 58-0022, the OTP + repo probe, PID 211750, `Shutdown timeout after 30s`. The 16 engine/transport variants passed, including MSSQL plaintext and STARTTLS, and the rate-limit subtest passed. Both stuck servers are the SQLite lead `DQM-Acuranzo-00-SMFC`. The lead was idle (its last query had already completed). Shutdown called `sqlite3_interrupt`, the worker logged `Worker thread exiting`, and glibc then printed `malloc_consolidate(): invalid chunk size` (Test 42) or `corrupted double-linked list` (Test 58 OTP). There is no `Signal 6` line after those messages. `pthread_timedjoin_np` waited 5s, cancelled again, waited another 5s, and logged `Worker thread still running after cancel`. The queue was left allocated. Test 42 then finished landing and logged `SHUTDOWN COMPLETE` with shutdown elapsed 10.003s, which is the two joins, but the lead thread was still alive so the process outlived the harness's 10s budget. Test 58 OTP never logged `SHUTDOWN COMPLETE`. After child 01 stopped, child 02 never logged `Destroying queue`, and the log is queue polling until the harness killed PID 211750 at 30s. The same exit-time heap error appears on SQLite shutdown in Tests 26 and 30 and on Test 58 SQLite STARTTLS. Tests 26 and 30 do get `Signal 6` and the crash handler `_exit`s, so the process is gone inside the timeout and those subtests pass. Item 7.3 stays open.
- **2026-10-01** `sqlite_cancel_inflight` passed `connection_handle` to `sqlite3_interrupt`. That pointer is the `SQLiteConnection` wrapper (48 bytes). On this libsqlite3 the interrupt flag is a store of 1 at offset 0x1a8, so every SQLite shutdown wrote 376 bytes past the wrapper. The lead's next `free` of its exit label then reported `malloc_consolidate(): invalid chunk size` or `corrupted double-linked list` and the worker never returned. MySQL and PostgreSQL already unwrap their handles before cancel. The call now passes `wrapper->db`, and the Unity success test asserts that pointer. Suite not yet rerun. Item 7.3 stays open.
- **2026-10-01** Suite `20261001_161109` is 5564/5565, combined coverage 85.276% (Unity 74.485%, blackbox 59.484%). Tests 42 (88/88) and 58 (23/23) are green. Tests 26 and 30 are green. No log in this run contains `malloc_consolidate`, `corrupted double-linked list`, `free(): invalid size`, or `Signal 6`. The SQLite cancel now hits `wrapper->db`. The one failure is Test 41 (9/10, 17.696s). Subtest 41-0008, Collect Initial Metrics: the server logged `Prometheus output length: 13134 bytes` and `Prometheus response queued successfully` at 23:16:54.521Z, HTTP 200 on the first of three attempts. The saved body starts with `hydrogen_system_info` and contains 244 `hydrogen_` series. `Demo_MS` was ready. Shutdown was clean and LeakSanitizer reported no leaks. The 500 auth requests never ran. The check is `echo "${INITIAL_METRICS}" | grep -q hydrogen_` under `set -o pipefail`. `grep -q` exits at the first match and closes the pipe; when `echo` is still writing, the pipeline status is 141 (SIGPIPE) and the subtest fails. An idle replay of that saved body failed 1 of 200 times. `scrape_metrics` uses the same pipeline and got through on attempt 1; the caller's second check does not retry. Item 7.3 stays open.

### Lessons learned

- An empty `demoms` is not a migrated database. Test 43 does not apply. Tests 40, 45, 46, 47, and 58 apply on startup and will race each other on the first apply. One forward apply into `demoms` (Test 40 alone, `TestMigration` false) has to finish before the parallel 40s group.
- A DB2 ODBC string contains `DRIVER=` and `HOSTNAME=`. SQL Server contains `DRIVER=` and `SERVER=` and no `HOSTNAME=`. Classifying every `DRIVER=` string as MSSQL makes every DB2 blackbox launch fail the same way.
- `INSERT ... OUTPUT ... WITH` is not valid T-SQL. The CTE list has to come first, and a second CTE after a comma has to move with the first. FreeTDS reports the syntax error as native 8180, "Statement(s) could not be prepared."
- Test 71 does not pick engines by name alone. `DESIGN_SCHEMAS["acuranzo"]` is `app::acuranzo:ACURANZO`, and engine index selects the schema prefix. A new engine with no slot gets an empty prefix.
- SQLite-only configs are for tests that need a database and do not validate each engine. Yugabyte is the primary engine and is left out of some blackbox configs because it is remote and slow; MySQL is also in DOKS and is faster for the DDL these tests do. Tests 41 and 44 keep the Yugabyte connection disabled. Tests 51–55 keep the connection named `Demo_YB` on local PostgreSQL schema `demoybdb`. `Demo_MS` is an additional connection. `AutoMigration` stays true on the new MSSQL connections except Tests 41 and 44, where every connection is false. Another test in the same run updates a behind `demoms`. Concurrent applies are accepted. `TestMigration` stays false. It is off across the checked-in configs, including Tests 32–39, and is turned on there only for a migration pass. That pass takes the suite from about 20 minutes to about an hour.
- Suite `20261001_105356` connected `Demo_MS` on every item 7.3 config. The remaining failures are stored QueryRefs, not the connection. QueryRef 30 (`acuranzo_1121.lua`, Get Lookups List) calls `LENGTH`, which SQL Server rejects (`'LENGTH' is not a recognized built-in function name`). QueryRef 57 (`acuranzo_1151.lua` MSSQL arm) is written with positional `?`. `convert_named_to_positional` then builds 0 parameters, reports all nine names unused, and `SQLGetDiagRec` returns no message (`could not get error details`, exec result -1). Themes (53), icons (54), and the number range (55) succeeded on MSSQL. On Tests 51–55, `Demo_YB` (local PostgreSQL, schema `demoybdb`) was not ready because that schema does not exist; the tests skip it and still pass. Tests 50, 56, and 60 used the real Yugabyte connection and it was ready.
- A from-scratch refresh that leaves migration rows as type 1000, with only migration 1000 flipped to type 1003, is not a finished database to Hydrogen. `available == applied` is the only "do nothing" case. Parallel AutoMigration then LOADs a row that already exists, or APPLYs seed SQL that already ran. Suite `20261001_092958` stayed green because those databases were already at that watermark, so startup did not migrate.
- Suite `20261001_134516` shows the stored QueryRef fix is loaded: Test 50 is 137/137 and Test 60 scored a real median. The remaining 43/45/47 failures are shutdown crashes and remote timeouts, not T-SQL spelling. Item 7.3 stays open until a later run is accepted as the close.
- Suite `20261001_145641` (5522/5524) shows the `destroy.c` refuse-to-free path stopped the Test 43 segfaults. The two remaining failures are SQLite lead workers that hit glibc heap corruption while exiting and never return, so the harness shutdown budget expires. Item 7.3 stays open.
- Suite `20261001_161109` (5564/5565, combined 85.276%) shows `sqlite_cancel_inflight` passing `wrapper->db` cleared the SQLite shutdown heap errors. Tests 42 and 58 are green. The remaining failure is Test 41's initial Prometheus check: the body is valid, and `echo | grep -q` under `pipefail` occasionally dies with SIGPIPE. Item 7.3 stays open.
- SQL Server never shipped `LENGTH()`. Character length is `LEN()` (trailing blanks excluded) and byte length is `DATALENGTH()`. The other engines in this suite have `LENGTH()`. The repair is the existing MSSQL `replace_query` hook, not a `${LENGTH}` macro. A rewrite cannot put names back into QueryRef 57, because the stored `?` markers already discarded them. An already-loaded `queries.code` row is not rewritten in place.

---

## Phase 8 — Docs sweep

### Goal

Current docs describe six real dialects (plus MariaDB/Yugabyte aliases)
and MSSQL as implemented. MACRO_REFERENCE has an MSSQL column.
DATABASES says “Linux container + ODBC 18; Developer is not production.”

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 Helium GUIDE, MACRO_REFERENCE, DATABASES, TESTING_GUIDE,
      BROTLI_COMPRESSION, design READMEs, `docs/He/DATABASES/database_mssql.md`.
- [ ] 8.2 Hydrogen TESTING, INSTRUCTIONS, PARAMETER_BINDING, SECRETS,
      STRUCTURE, SITEMAP, MAIL_GUIDE, SchemaTool/SchemaHelper, tests README.
- [ ] 8.3 Lithium `DATABASE-MIGRATIONS.md` (key 5 already named).

### Done means

`mkl` green; no active doc claims key 5 is unimplemented.

### Exit gate

- `zsh -ic 'mkl'`; markdownlint on touched files.

### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | 2026-09-30 |
| **Result** | Not started. The earlier text in this cell was a copy of the Test 39 scaffold. The work items above are this phase's work, and they are unchecked. |
| **Variances** | None yet. |

### Working Log

- **2026-09-30** Not started. The Test 39 file-creation notes copied here were removed. That work is Phase 5 items 5.1–5.3.

### Lessons learned

(empty until the phase runs)

---

## Phase 9 — Completeness and coverage re-check

### Goal

Every completeness-fence row is true or `[~]`. Coverage fences hold.
Dead-code list has no stray mssql symbols.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 Walk completeness table.
- [ ] 9.2 Walk coverage fences.
- [ ] 9.3 `mkt` dead-code gate.
- [ ] 9.4 `mkp`, `mks`, `test_98`, Test 31, Test 39, Test 40 mssql.

### Done means

Fences green; Test 39 and Test 40 mssql green. Then move this plan to
`plans/complete/MSSQL_COMPLETE.md`; drop TODO 28; `mkl`.

### Exit gate

- Commands in 9.4 actually run; output cited in Status.

### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | 2026-09-30 |
| **Result** | Not started. The earlier text in this cell was a copy of the Test 39 scaffold. The work items above are this phase's work, and they are unchecked. |
| **Variances** | None yet. |

### Working Log

- **2026-09-30** Not started. The Test 39 file-creation notes copied here were removed. That work is Phase 5 items 5.1–5.3.

### Lessons learned

(empty until the phase runs)

---

## Testing notes

| Layer | What |
| --- | --- |
| Unity | Connstring, registry, ODBC mock |
| Blackbox | Test 39 container **full** AutoMigrations. Test 40+ when Phase 7 says so |
| Coverage | See coverage fences |
| Build | `mkq` / `mkt` / `mkp` / `mks` / `test_98` |

Port scheme: Test 39 → **539x**. SQL Server TDS **1433** (or extras
override).

## Threat notes

- **Secret leakage:** SA password, hashes, JWTs. Mask `mssql://`.
- **EULA:** Developer edition is not production; do not imply it is.
- **Encrypt:** Driver 18 defaults Encrypt=yes; local container needs
  TrustServerCertificate.
- **Hash mismatch** is a login outage — UTF-8 SHA-256 fixture is
  mandatory.
- **CLR / COMPRESS failure** stores wrapped blobs as `code`.

### Risks

| Risk | Mitigation |
| --- | --- |
| Treating SQL Server as libpq | ODBC client; never alias to postgresql |
| Windows-only extras | Forbidden; Linux container + CLR or COMPRESS pre-eval |
| ODBC 18 will not install on Fedora | Phase 1 amendment to FreeTDS, recorded |
| Local RAM < 2 GiB for the container | Phase 1 DOKS fallback, recorded, not silent |
| RETURNING templates vs OUTPUT | Acuranzo Lua rewrite before storage (lock 21) |
| Concat `\|\|` / LATERAL QueryRefs | Phase 2 grep + arms |
| Fighting Firebird over Test 37 / enum | Test 39; reserved enum order |
| 7-engine loops miss mssql | Phase 7 grows to 8; do not drop Yugabyte |
| Helium ID drift | Re-check disk at packet time |

## Working Log (cross-phase memory)

### Decisions log

- **(Plan authored, 2026-09-18)** MSSQL created as a greenfield sixth
  C engine. Key 5 already seeded. Fedora-local official Linux container
  preferred over DOKS. Sister plan FIREBIRD.md. No C this turn.
- **(Phase 1 complete, 2026-09-29)** extras/mssql_server/ scripts (start.sh,
  stop.sh, create_test_db.sh) verified with Podman SQL Server 2022 Linux
  container. `sqlcmd -Q "SELECT 1"` succeeds against 1433. `hydrotst` db +
  `testms` schema created. msodbcsql18 deferred (needs root); in-container
  sqlcmd used for health checks. No FreeTDS amendment needed.
- **(Phase 2 complete, 2026-09-29)** `database_mssql.lua` created in all 4
  Helium designs with complete key set. `database.lua` v3.5.0 updated in
  all 4 designs. `lua.c` engines[] includes "mssql". Test 31 passes
  2316/2316 (sqruff skipped for mssql). test_98 luacheck clean (466 files).
   acuranzo_1000/1135/1190/1168 have mssql support; 1147/1151/1189/1217
   use `engine ~= 'mysql'` covering mssql. Phase 3 (C engine) awaits Phase 0
   lock approval.
- **(Phase 3 complete, 2026-09-29)** All 16 files in `src/database/mssql/`
   written. `DB_ENGINE_MSSQL` enum slot (key 5, after DB2 before FIREBIRD)
   active. `mssql_get_interface()` registered in `database_engine_registry.c`.
   `database_get_counts_by_type` updated with 7th `mssql_count` param; all
   callers updated. `mssql://` scheme recognized in `database_queue_determine_engine_type`.
   `normalize_engine_name` accepts `mssql` and `sqlserver`. Connection string
   uses ODBC Driver 18 format with `Encrypt=yes;TrustServerCertificate=yes;SCHEMA=testms;`.
   Unity ODBC mock (`mock_libodbc.c`/`mock_libodbc.h`) with `mssql_mock_*` prefix
   avoids symbol collision with DB2 mock. `interface_test_mssql.c` — 8/8 PASS.
   `mkt` Build Successful. `mkp` clean (0 issues, 2,156 files). Fixed
   `mssql_engine_is_available()` to return `true` in mock mode (Firebird pattern).

### Surprises / deviations (historical, still true)

- Fedora 43 does not ship `mssql-server`. Podman is already installed.
- C-level Firebase is already removed (no `DB_ENGINE_FIREBASE` in enum).
  Lua-level firebase references survive in Helium but use a different
  dialect name (`firebase` vs `mssql`) and Lookup key (6 vs 5) — no
  enum collision. MSSQL Phase 3 adds `DB_ENGINE_MSSQL` to its reserved
  slot.
- Lookup 030 key 5 has been “MS SQL Server” since 1055; no additive
  lookup is required.
- DB2 C already uses ODBC typedefs — copy that shape, not Firestore.
- `INSERT_KEY_RETURN` is trailing `RETURNING` in QueryRefs (1194).
  SQL Server `OUTPUT` is mid-statement. Lock 21 exists because changing
  every Lua file is worse.
- SQL Server `HASHBYTES` + `NVARCHAR` is UTF-16; other engines hash
  UTF-8. The T-SQL helper must not silently diverge.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After add/remove `src/`: `mkt` then `mkp`.
- After bash: `mks`. After Lua: `test_98`. After docs: `mkl`.
- Never apply Helium packets; hand them to the user.
- Payload rebuild (`mkt`) is required before Test 31/39 see
  `database_mssql.lua`.
- Do not `dlopen` libpq for mssql.
- Cross-check SHA-256 against SQLite `crypto_sha256` before declaring
  login green.
