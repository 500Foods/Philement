<!-- markdownlint-disable MD007 MD024 -->
# Firebird Engine Plan

## Status at a glance

**New plan (2026-09-18).** Phase 0 locks approved (2026-09-18). Phases 0–11 complete.
Phase 12 is deferred. The only remaining implementation item is the prepared-statement
handle cache, and it is not a blocker.
**2026-09-23:** User reports the current suite 100% passing. Migration issues
found on the way (APPLY, reverse, and the Phase 10 client gaps) are fixed.
Stay on Firebird 4.0.7. See [Status 2026-09-23](#status-2026-09-23).
This is not a rename of
[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).
Firebird is a SQL RDBMS (`libfbclient`). Hydrogen sends Helium SQL to it.

| Phase | Status | Remaining |
| --- | --- | --- |
| 0 Contract lock | **complete** | Locks approved; survey confirmed |
| 1 Fedora Firebird extras | **complete** | Scripts + README; mks/mkl green; packages installed |
| 2 Helium dialect + Lookup 030 key 6 | **complete** | database_firebird.lua in 4 designs; Test 31 green; 1384 packet; mks green |
| 3 Firebase C / Unity teardown | **complete** | `mkq`/`mkp` green; `rg -n firebase src/ tests/unity/ cmake/` empty |
| 4 Firebase Helium / extras teardown | **complete** | |
| 5 C register / connect | **complete** | Live health deferred to Phase 7 |
| 6 Brotli UDR + JSON ingest | **complete** | C artifacts ready; live verification Phase 7 |
| 7 Test 37 full Acuranzo | **complete** | APPLY and reverse green. User reports the suite 100% passing (2026-09-23) |
| 8 Retire Cockroach names | **complete** | 7-engine loop says Firebird; configs/scripts/libs/schematool/schemahelper |
| 9 SchemaTool / flush | **complete** | schematool_firebird.sh; schemahelper ping/qdecode/qutil; flush paths |
| 10 Tests 40–58 firebird matrix | **complete** | User reports the current suite green (2026-09-23), including 40–58 |
| 11 Docs | **complete** | Firebird column and engine pages; Cockroach and Firestore are historical in active docs |
| 12 Coverage / completeness | **deferred** | User 2026-09-23: suite already green. Not a blocker. Do not archive the plan while the cache follow-up is still noted here. |

Remaining: `firebird_prepare_statement` still stores SQL text only (each execute prepares and frees the `isc_stmt_handle`). Worth doing for consistency with the other engines. Not a blocker. Firebird 4.0.7 is sufficient. Do not upgrade to Firebird 5 for it.

## Status 2026-09-22

Carmine/Andrew Firebird migration push (live `hydrogen_test_37_firebird` on
Firebird **4.0.7**). FB5 upgrade **not** required for the dialect gaps hit so
far. Staying on 4.0.7.

**Working now (mark done in logs below; do not claim Phase 10/11/12 complete):**

- Hydrogen: real `firebird_health_check` prepare/execute/fetch SELECT;
  `firebird_execute_query` + result shaping; migrations unblocked in
  `migration/transaction.c`; DDL via `isc_dsql_execute_immediate`; free()/invalid
  pointer around failed stmts fixed; `queries.code` BLOB→JSON (`0x80` parse);
  `execute_firebird_migration` commit-after-DDL (CREATE/ALTER/DROP/RECREATE), DML
  stays one txn; `lead_apply.c` / `lead_reverse.c` route Firebird there;
  `fb_interpret` multi-line status logging; `libjson_udfn` build path (host
  install may still need sudo).
- Extras DB scripts: `create_test_db.sh` / `run_create.sh` **PAGE_SIZE 32768**
  (was 4096; needed for wide UTF8 UNIQUE e.g. 1189), firebird owner +
  SUDO_USER/PKEXEC_UID group + 666, isql heredoc comment-outside-heredoc fix.
  **Dual files (2026-09-22):** `hydrogen_test.fdb` + `hydrogen_demo.fdb` via
  `FIREBIRD_DB_PATH_TEST` / `FIREBIRD_DB_PATH_DEMO` (modes `both|test|demo`).
  Test 37 configs use `_TEST`; 40+ Firebird slots use `_DEMO`. No SQL schemas
  on FB 4/5 — file is the isolation boundary.
- Helium dialect (Option D family): multi-row INSERT→UNION ALL FROM
  `RDB$DATABASE` (database.lua 3.4.0+); CTE VALUES rewrite (D2 3.4.1); strip
  `COLUMN` after ADD/DROP (D3 3.4.2); `NOT NULL DEFAULT`→`DEFAULT … NOT NULL`
  (D4 3.4.3); `database_firebird.lua` 1.3.0 `DROP_CHECK` uses
  `FROM RDB$DATABASE WHERE EXISTS`; ASCII hyphen in refuse message;
  `acuranzo_1190.lua` Firebird forward/reverse ALTER `password_hash`
  DROP|SET NOT NULL.
- Live Test 37: **APPLY completed through end of corpus** (past 1197 etc.).
  TestMigration reverse started; 1342–1341 OK; 1340 DROP_CHECK failed then
  fixed; **full reverse green still pending confirmation** (retest in flight
  at doc update time).

**Still open as of 2026-09-22 (superseded where noted in [Status 2026-09-23](#status-2026-09-23)):**

- Full TestMigration reverse end-to-end green. **Closed 2026-09-23.**
- `firebird_prepare_statement` / live `isc_dsql_prepare` into StmtCache
  (app StmtCache exists; prepare path still Phase-5/6 stub — **no** real
  statement cache). **Still open.** Optional; Firebird 4.0.7 is sufficient.
- Remaining per-engine missing branches in individual migrations (1190 was
  one class). **Closed 2026-09-23** for every gap the suite hit.
- RETURNING / QueryRefs runtime paths if exercised outside APPLY.
  **Closed 2026-09-23** for paths the green suite exercises.
- UDR install on host if not already installed (sudo). **Closed** as a
  migration blocker by the green APPLY run.
- Unity/mock completeness for new Firebird paths. **Still open;** user is
  covering this separately. Phase 12 re-checks the fence.
- Phase 10 live suites 40/43/45/46/47/58. **Closed 2026-09-23.**
  Phase 11 docs and Phase 12 fences remain.

## Status 2026-09-23

User report, not an agent rerun of the suite: the current tests are 100%
passing, and the migration issues encountered on the way there are fixed.
That closes Test 37 reverse and the Phase 10 live runs (40, 41, 43, 44, 45,
46, 47, 50, 51, 54, 58 and the rest of the current matrix). Phases 0–10 are
complete on that report. Next is Phase 11.

Unity coverage of the newer Firebird paths is in progress separately and
stays a Phase 12 fence, not a Phase 11 blocker.

`firebird_prepare_statement` is still a stub: the per-connection cache stores
the SQL text, and `firebird_execute_prepared` calls `firebird_execute_sql`,
which allocates, prepares, executes, and frees an `isc_stmt_handle` every
time. Holding that handle is client work on the Firebird 4.0.7 `libfbclient`
already in use (`isc_dsql_allocate_statement`, `isc_dsql_prepare`,
`isc_dsql_execute` / `isc_dsql_execute2`, `isc_dsql_free_statement`). Firebird
5's compiled-statement cache (`MaxStatementCacheSize`) is a server-side cache
for clients that re-prepare the same text. It does not replace the handle
cache, and Fedora 43 does not ship Firebird 5. Stay on 4.0.7. A Firebird 5
move also changes multi-row DML `RETURNING` from `isc_info_sql_stmt_exec_procedure`
to a selectable statement; singleton `INSERT … VALUES … RETURNING` does not
change.

**Parity:** Firebird is a Hydrogen `DatabaseEngineInterface`, not a new
API. Match PostgreSQL / SQLite / MySQL / DB2: same `QueryRequest` /
`QueryResult`, same `parse_typed_parameters` →
`convert_named_to_positional` → bind, same `data_json` array of row
objects. Do not change those engines. Do not interpret SQL in C.

**Sister plan:** [`MSSQL.md`](/docs/H/plans/MSSQL.md) (Lookup 030 key 5).
Shared enum lock is in [Coordination with MSSQL](#coordination-with-mssql).

## Purpose

Replace the **CockroachDB** operator slot with a real **Firebird**
implementation. Cockroach was never a fifth Hydrogen engine: it is a
PostgreSQL alias (same `libpq` path, same
`${env.ACURANZO_DB_TYPE}` = `postgresql`, a different schema name).
YugabyteDB is the same kind of alias and **stays**. Firebird will not
alias anything. It is a `DatabaseEngineInterface` in C and a Helium
dialect (`database_firebird.lua`).

Cloud Firestore was attempted first
([`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)).
That was the wrong product: Helium emits SQL, and Firestore has no SQL
server. Firebird does. Phases 3–4 delete the Firestore tree; they are
not “reshape Firebase.”

**There is no v1 subset and no SQL interpreter.** When Hydrogen’s
Firebird engine and Helium’s `database_firebird.lua` are in place,
AutoMigrations must apply the **same Acuranzo Lua files** that Test 32
applies on PostgreSQL. QueryRefs that later run through Conduit / auth /
mail must execute as Firebird SQL.

Helium migrations stay **SQL**. Macros still expand types and function
wrappers. Hydrogen **sends that SQL to `libfbclient`**. Brotli that
Firebird cannot do natively is an extras **UDR**, same story as SQLite
`.so` extras — not in-process eval in Hydrogen.

This is the only active plan for Firebird and for Firebase teardown.

**Session brief:** A new conversation may start with only a pointer to
this file. Open [Status at a glance](#status-at-a-glance), then
[Resuming Work](#resuming-work), then the next incomplete phase only.
Do not reconstruct Cockroach vs Firebase vs Firebird from git history.

History that this plan does **not** reopen:

- Database subsystem:
  [`DATABASE_PLAN_COMPLETE.md`](/docs/H/plans/complete/DATABASE_PLAN_COMPLETE.md)
- Parameter binding:
  [`DATABASE_UPDATE_PLAN_COMPLETE.md`](/docs/H/plans/complete/DATABASE_UPDATE_PLAN_COMPLETE.md)
- Migrations:
  [`MIGRATIONS_COMPLETE.md`](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md)
- SchemaTool / SchemaHelper:
  [`SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md),
  [`SCHEMAHELPER_V2_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md)
- Firestore attempt:
  [`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, record lessons learned, **stop for review**.
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure or
  after adding/removing `src/` files), `mku <base>`, `mkp`, `mka`, `mks`.
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**. Follow this sequence:

1. **Confirm the prior phase is actually done.** Re-read its Status block
   and Exit gate before touching anything; do not trust memory of a prior
   session.
2. **Discuss the current phase first.** Re-read only that phase's Goal +
   Work items + Done means + Exit gate. Ask clarifying questions and do
   any research needed **before** writing any code.
3. **Ask for explicit approval to start implementation.** Do not begin
   editing source files until the user says go.
4. **Ask questions as they come up** during implementation rather than
   guessing at ambiguous requirements.
5. **Update the phase's Working Log entry when major pieces land** (not
   only at the very end).
6. **Record lessons learned** for the phase, even small ones.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean.** Intent to verify is
   not verification.
8. **Never apply a database migration.** Prepare/generate Helium packets
   and hand them to the user; do not run `schematool`/`schemahelper` apply.
9. **Follow existing project norms:** no `static` functions in `src/`;
   Unity one file per function ([TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md));
   blackbox `tests/test_NN_*.sh` ([TESTING.md](/docs/H/tests/TESTING.md));
   `jq` for JSON in Bash; absolute Markdown links; `mkq`/`mkt` then `mkp`
   after C; `mks` after scripts. Meet the [Coverage fences](#coverage-fences)
   and [Completeness fences](#completeness-fences).
10. **Never log** Firebird SYSDBA passwords or document bodies that contain
    secrets (password hashes, JWTs, OTP codes) in normal logs or test
    artifacts.
11. **Do not increment `TEST_COUNTER` in blackbox scripts.** The framework
    owns the counter.
12. **Do not delete the Cockroach slot until Phase 8.** Earlier phases add
    Firebird beside it. Phase 8 is the swap so the 7-engine matrix never
    silently becomes six.
13. **Mirror the other engines.** When a Firebird choice is ambiguous,
    do what PostgreSQL / SQLite / MySQL / DB2 already do
    (`PARAMETER_BINDING.md`, `QueryResult.data_json`, named `:NAME`
    markers). Do not invent a Firebird-only execute path and do not
    change the other engines to accommodate it.
14. **Do not interpret SQL in C.** Firebird runs the SQL. Extras UDRs
    cover Brotli (and JSON ingest/extract if native SQL/JSON is absent).
    The Firestore interpreter in `src/database/firebase/sql_*.c` is
    deleted in Phase 3, not reused.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-23):** Phases 0–11 complete. Phase 12 is deferred: the user reports the current suite 100% passing, and the fence re-check is not outstanding. The only remaining implementation item is `firebird_prepare_statement`, which stores SQL text and still prepares and frees an `isc_stmt_handle` on every execute. That follow-up is for consistency with the other engines, stays on Firebird 4.0.7, and is not a blocker. Firebird 5's compiled-statement cache is not a reason to upgrade. See [Status 2026-09-23](#status-2026-09-23).

Keep this block current when a phase finishes (date, result, next phase
number). It is the first thing a new session reads.

### Resume here next session

1. This file is the source of truth for Firebird and for Firebase
   teardown. Do not start a second Firebird or “drop Cockroach” plan.
   Do not resume [`FIREBASE.md`](/docs/H/plans/FIREBASE.md).
2. Read **Status at a glance**, **CURRENT PAUSE POINT**, the Phase
   Index **Status** column, and **Working Log (cross-phase memory)**.
   Parity with the other four engines is a standing rule.
3. Confirm the prior phase Status is actually complete (re-read its Exit
   gate; do not trust chat memory).
4. Re-read **only** the next phase: Goal + Work items + Done means +
   Exit gate. Skim a lock in
   [Proposed design locks (Phase 0)](#proposed-design-locks-phase-0)
   when that phase cites it. Do not re-read the whole document.
5. If the phase needs a Helium packet: re-check disk
   (`ls elements/002-helium/acuranzo/migrations/acuranzo_*.lua`).
   Snapshot at this pause: last **`acuranzo_1383.lua`**, Lookup **030**
   key **6** still labelled Firebase. `DB_ENGINE_AI` is still the Unity
   mock slot. Do not trust the snapshot; re-check disk.
6. Discuss, get explicit approval, implement that phase only, verify the
   Exit gate, update Status + Working Log + this pause point, **stop**.

### Session checklist

1. CURRENT PAUSE POINT → first Status that is not complete.
2. Working Log decisions that affect this phase.
3. Baseline as the phase names it: `mkq` or `mkt`; named `mku`; Test 31
   / 37 / 40 when listed.
4. One phase: questions → approval → implement → verify → update this
   plan → stop for review.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new engine, after Auth Finale / quality gates |
| **Effort** | XL (C `libfbclient` engine + Helium dialect + Brotli/JSON extras + Test 37 + Cockroach retirement + Firebase teardown) |
| **Done** | Phases 0–11 complete (2026-09-23). Phase 12 deferred. Prepared-statement handle cache is the only follow-up and is not a blocker. |
| **Why this shape** | Cockroach never earned a C implementation. Firebird is a SQL engine Fedora already packages. The existing ~380 Lua files emit SQL; Firebird runs that SQL. |
| **Do not start casually** | Touches `DatabaseEngine` enum, registry, DQM, Helium `database.lua` for four designs, Test 31/37/71, the 7-engine blackbox matrix, SchemaTool, Lookup 030, and deletes the Firestore tree. |

Backlog: [TODO.md item 27](/docs/H/TODO.md).

## Coordination with MSSQL

[`MSSQL.md`](/docs/H/plans/MSSQL.md) adds Lookup 030 **key 5** (already
seeded as `MS SQL Server`). This plan owns key **6** (relabel Firebase
→ Firebird) and the Cockroach replacement (Test **37**).

**Final C enum** (both plans lock this; do not shift later):

```c
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_MSSQL,      // key 5 — interface may be NULL until MSSQL.md
    DB_ENGINE_FIREBIRD,   // key 6
    DB_ENGINE_AI,         // Unity mock — do not reuse
    DB_ENGINE_MAX
} DatabaseEngine;
```

Phase 5 of this plan may introduce `DB_ENGINE_MSSQL` as an unused
enumerator so the MSSQL plan does not shift Firebird’s numeric value.
Do not add `mssql_get_interface` symbols here (dead-code gate). Registry
skips a NULL interface.

Shared files (`database.lua` `engines`, `lua.c` `engines[]`, Test 31
`ENGINES`, `convert_named_to_positional`, `database_get_counts_by_type`)
are **additive only**. Do not delete the other plan’s dialect name.
Firebase names are this plan’s to remove (Phases 3–4).

Test **37** stays Firebird (was Cockroach). MSSQL gets a new Test **39**.
The 7-engine matrix stays 7 until MSSQL Phase 7 grows it to 8.

## Snapshot: what exists today (2026-09-18)

This section stays dated 2026-09-18. It is the baseline the phases started from, not the current tree. After Phase 8, Cockroach names are retired from the active matrix and Firebird is the seventh operator. Firestore is historical only ([FIREBASE_SUPERSEDED.md](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)). Current state is [Status at a glance](#status-at-a-glance).

Do not re-implement these; they are constraints.

### Four real engines, three aliases, one wrong fifth

| Operator name | Hydrogen C | Helium dialect | Typical config `Engine` | Notes |
| --- | --- | --- | --- | --- |
| PostgreSQL | `src/database/postgresql/` | `database_postgresql.lua` | `${env.ACURANZO_DB_TYPE}` → `postgresql` | Real |
| SQLite | `src/database/sqlite/` | `database_sqlite.lua` | `sqlite` | Real |
| MySQL | `src/database/mysql/` | `database_mysql.lua` | `${env.CANVAS_DB_TYPE}` | Real |
| DB2 | `src/database/db2/` | `database_db2.lua` | `db2` | Real |
| MariaDB | **MySQL** | **mysql** | same as MySQL, schema `demomrdb` | Alias |
| YugabyteDB | **PostgreSQL** | **postgresql** | `${env.YUGABYTE_DB_*}` | Alias; **stays** |
| CockroachDB | **PostgreSQL** | **postgresql** | schema `testcrdb` / `democrdb` | Alias; **this plan retires it** |
| Firebase | **removed** | `database_firebase.lua` **missing**; `engines.firebase` / `require(...firebase...)` survive in Lua | (not in 7-engine matrix) | C-level gone; Lua-level cruft remains for Phase 4 |

**`DB_ENGINE_FIREBASE` is already removed from `database_types.h`.** The enum
currently is PostgreSQL=0, SQLite=1, MySQL=2, DB2=3, `DB_ENGINE_AI` (Unity mock),
MAX — no firebase slot. `grep -rn firebase src/ tests/unity/ cmake/` finds
**zero** C-level references. `lua.c` `engines[]` is `{"sqlite","postgresql","mysql","db2"}`
(no firebase). `database_firebase.lua` does not exist on disk (all four designs).
`extras/firebase_emulator/` is absent.

However, Lua-level **cruft** remains: `database.lua` in all four Helium designs
still sets `firebase = true`, `firebase = 6`, and calls
`require("database_firebase")` (dangling). Migration files contain
`if engine == 'firebase'` branches. Test 31 lists `firebase` in `ENGINES`.
Config schema, SECRETS, and docs still reference firebase. Phase 4 removes this
Lua-level cruft after Phase 2 provides `database_firebird.lua`.

`grep` of `src/**/*.c` for `cockroach` is **empty**.

### This machine (Phase 1 baseline)

| Fact | Value |
| --- | --- |
| Distro | Fedora 43 |
| Firebird in `dnf` | `firebird` / `libfbclient2-devel` **4.0.7.3271** (not installed) |
| Firebird 5 | Fedora 44/45 only — **not** required |
| `podman` | 5.8.4 (MSSQL plan; unused here) |

### Cockroach-named files (main tree)

Replace in Phase 8, not before. Inventory is unchanged from the
Firestore plan: Test 37 script/docs/configs, SchemaTool wrapper,
`hydrogen_flush.sh`, Test 40/41/43/45/46/47/51/54/58 scripts, TESTING /
PARAMETER_BINDING / SECRETS / SITEMAP / STRUCTURE. Historical
`docs/H/metrics/` **stay**.

### How the other engines get Base64 / Brotli / SHA-256 / TZ

| Need | PostgreSQL | MySQL | SQLite | DB2 | Firebird 4 (this plan) |
| --- | --- | --- | --- | --- | --- |
| Base64 | native | native | sqlean `crypto.so` | C UDF extras | **native** `BASE64_ENCODE` / `BASE64_DECODE` |
| Brotli | C extra | plugin extra | loadable extra | C UDF extra | **UDR extra** (`extras/brotli_udf_firebird/`) |
| SHA-256 | native | native | sqlean | `HASH(...,2)` + encode UDF | **native** `CRYPT_HASH(... USING SHA256)` |
| `json_ingest` | plpgsql | stored fn | passthrough | SQL UDF | **PSQL function or UDR** (no SQL/JSON until Firebird 6) |
| `CONVERT_TZ` | native | native | extra (UTC stub) | native | **native** `TIMESTAMP WITH TIME ZONE` / `DATEDIFF` — lock in Phase 0 |

SQLite is the closest analog: extras for what the engine cannot do,
`?` placeholders, file-or-local server. Firebird SuperServer is a
daemon on **3050**, like MySQL, with an optional embedded attach.

## The four solvable headaches

These are the same contracts Helium already made identical across PG /
MySQL / SQLite / DB2. They are in scope. Firebird does **not** get a
pass because it is “SQL.”

### 1. JSON ingest and extract

Firebird 4/5 store JSON as **text** (`BLOB SUB_TYPE TEXT`). Native
`JSON_VALUE` is Firebird 6 / Red Database — **not** on Fedora 43.
`${JSON}` columns are text, SQLite-compatible. Do not invent a Firebird
binary JSON type.

**Ingest:** same control-char fix-up as PG/MySQL/DB2 (fast-path if
valid; escape `\n`/`\t`/`\r` only inside quotes; parse again). `$ref` /
`$id` / `$schema` accepted (not DB2 JSON2BSON). Implement as a PSQL
stored function in `acuranzo_1000` (engine arm) or a C UDR in extras.
Unity or `isql` fixture: valid JSON; JSON with newline inside a string;
garbage still fails; JSON Schema with `$ref` round-trips.

**Extract (`${JRS}` / `${JRM}` / `${JRE}`):** UDR or PSQL
`JSON_VALUE(col, '$.icon')`. Missing path → SQL NULL.

### 2. Base64

Native. Alphabet is standard `+/` with `=` padding. No URL-safe
variant. No extras UDF.

### 3. Brotli

Lua still compresses `code` >1KB at generation and wraps
`${COMPRESS_START}` … `${COMPRESS_END}`. Firebird must decompress on
INSERT or APPLY runs a base64 blob as SQL.

**UDR** in `extras/brotli_udf_firebird/` (`libbrotlidec`, same library
as the other extras). Quality 11 on the Lua side is a lock; the engine
only decompresses. `CREATE FUNCTION` from `${BROTLI_DECOMPRESS_FUNCTION}`.

### 4. SHA-256 password hashes

Bytes must match the other engines or Test 40 login against a
firebird-migrated admin account fails.

Proposed spelling:

```text
${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
→ BASE64_ENCODE(CRYPT_HASH(('0' || '<password>') USING SHA256))
```

Phase 6 (or 5) fixture: `account_id` string `"0"` + `"testpass"` →
`CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` (SQLite crypto_sha256,
already recorded in the superseded Firestore plan). Do not use mocked
`utils_password_hash` in Unity.

## Firebird installation (CI / dev)

Read this before Phase 0 locks. Firebird **is** `dnf install` plus
`CREATE DATABASE`.

| Piece | Why | Notes |
| --- | --- | --- |
| `firebird` | SuperServer on 3050 | Fedora 43: 4.0.7 |
| `libfbclient2` | runtime client | pulled by `firebird` |
| `libfbclient2-devel` / `firebird-devel` | `ibase.h`, link Hydrogen | |
| `firebird-utils` | `isql-fb`, `gfix`, `nbackup` | |
| libbrotli | UDR decompress | already a Hydrogen dep |

There is **no** Firestore emulator, **no** Java, **no** `firebase-tools`.

### SuperServer vs embedded

| Mode | When | Connection |
| --- | --- | --- |
| **SuperServer (Test 37 / 40 default)** | networked SQL, like the Cockroach slot | `localhost/3050:/path/testfb.fdb` |
| **Embedded (allowed)** | appliance / no daemon | Database path, empty Host — `libfbclient` loads Engine plugin |

Phase 1 documents both. Tests lock SuperServer unless Phase 0 amends.

### extras layout (Phase 1)

```text
elements/001-hydrogen/hydrogen/extras/
  firebird/
    README.md     # dnf packages, SYSDBA, create testfb.fdb, ports
    start.sh      # enable/start firebird service if we own it; wait for 3050
    stop.sh       # stop only if we started it
    create_test_db.sh
```

Do **not** keep `extras/firebase_emulator/` past Phase 4.

## Why Helium still emits SQL

Acuranzo files contain literal SQL verbs. Macros fill types and
function wrappers. Rewriting ~380 files into a Firebird-only language
is out of scope. `database_firebird.lua` is a fifth (then sixth, with
MSSQL) dialect file with the **same keys** as the other four.

Rare `if engine == '…'` files must grow a **firebird** arm (and lose
**firebase** in Phase 4):

| File | Why it branches |
| --- | --- |
| `acuranzo_1000.lua` | UDF/UDR DDL per engine |
| `acuranzo_1190.lua` | ALTER COLUMN (sqlite table rebuild — Firebird ALTER is limited; sharing sqlite is acceptable) |
| `acuranzo_1135.lua` | JSON constructors per engine |
| `acuranzo_1151.lua` | mysql vs others — `engine ~= 'mysql'` already covers firebird |

Phase 2 greps `if engine` again. 1168 (`LATERAL` + `jsonb_agg`) is
PostgreSQL-shaped; Firebird 4 has `LATERAL` derived tables but not
`jsonb_agg`. Dedicated arm or documented rewrite — do not silently
drop LATERAL.

## `database_firebird.lua` — complete macro table

Every key in the union of `database_postgresql.lua`,
`database_mysql.lua`, `database_sqlite.lua`, `database_db2.lua` must
exist so `replace_query` never leaves `${UNSUBSTITUTED}`. Values below
are **proposed** (Phase 0 amends).

`${SCHEMA}` for Firebird is **empty**. Firebird 4 has no PostgreSQL
schemas. Isolation is a separate `.fdb` (`testfb.fdb` / `demofb.fdb`),
not `testfb.queries` and not `testfb_queries`. `database.lua`
`replace_query` must not apply the firebase underscore prefix to
firebird. Empty schema → no prefix (SQLite-like).

### Types

| Macro | Firebird 4 spelling | Notes |
| --- | --- | --- |
| `${INTEGER}` / `${INTEGER_SMALL}` | `INTEGER` / `SMALLINT` | |
| `${INTEGER_BIG}` | `BIGINT` | |
| `${FLOAT}` / `${FLOAT_BIG}` | `FLOAT` / `DOUBLE PRECISION` | |
| `${TEXT}` / `${VARCHAR_*}` / `${CHAR_*}` | `VARCHAR(n)` / `CHAR(n)` | |
| `${TEXT_BIG}` | `BLOB SUB_TYPE TEXT` | fail closed only if extras impose a cap; no 1 MiB Firestore cap |
| `${JSON}` | `BLOB SUB_TYPE TEXT` | ingested text |
| `${DATE}` / `${TIME}` / `${DATETIME}` / `${TIMESTAMP}` | `DATE` / `TIME` / `TIMESTAMP` | |
| `${TIMESTAMP_TZ}` | `TIMESTAMP WITH TIME ZONE` | Firebird 4 |
| `${SERIAL}` | `INTEGER GENERATED BY DEFAULT AS IDENTITY` | |
| `${PRIMARY}` | `PRIMARY KEY` | |
| `${UNIQUE}` | `UNIQUE` | |
| `${NOW}` | `CURRENT_TIMESTAMP` | |
| `${DUMMY_TABLE}` | `FROM RDB$DATABASE` | required |
| `${REORG}` | `-- REORG TABLE` | no-op, like PG |

`${SIZE_*}` numeric strings stay for diagrams.
`${SIZE_COLLECTION}` is `CHAR_LENGTH(collection)` (or `OCTET_LENGTH`).

### Keys, time, JSON extract

| Macro | Proposed firebird |
| --- | --- |
| `${INSERT_KEY_START}` | `--` plus trailing space (same as PG; Firebird `RETURNING` is trailing) |
| `${INSERT_KEY_END}` | empty |
| `${INSERT_KEY_RETURN}` | `RETURNING` plus trailing space |
| `${SESSION_SECS}` | `DATEDIFF(SECOND FROM CAST(:SESSION_START AS TIMESTAMP) TO CURRENT_TIMESTAMP)` (lock exact spelling in Phase 2) |
| `${TRMS}` / `${TRME}` | `DATEADD(-N MINUTE TO ${NOW})` split across start/end |
| `${TRFS}` / `${TRFE}` / `${TRFMS}` / `${TRFME}` | `DATEADD` seconds/minutes |
| `${JRS}` / `${JRM}` / `${JRE}` | UDR/PSQL `JSON_VALUE(` / comma-space / `)` |
| `${DROP_CHECK}` | sqlite-shaped `SELECT … WHERE EXISTS` **or** an exception; APPLY must fail closed when the table has rows — lock in Phase 2 (Firestore `FB_REFUSE_DROP` is gone) |

DB2-only extras that must still be defined:
`${BASE64ENCODE_START}` / `_END`, `${BASE64ENCODEBINARY_START}` / `_END`,
`${DATETIME_FORMAT}`, `${TIMESTAMP_FORMAT}`, `${CONVERT_TZ_FUNCTION}`.

### Encoding / hashing / ingest

| Macro | Firebird proposed |
| --- | --- |
| `${BASE64_START}` / `${BASE64_END}` | `CAST(BASE64_DECODE(` / `) AS BLOB SUB_TYPE TEXT)` (lock CAST target in Phase 2) |
| `${COMPRESS_START}` / `${COMPRESS_END}` | `BROTLI_DECOMPRESS(BASE64_DECODE(` / `))` |
| `${SHA256_HASH_START}` / `_MID` / `_END` | `BASE64_ENCODE(CRYPT_HASH((` / ` \|\| ` / `) USING SHA256))` |
| `${JSON_INGEST_START}` / `_END` | `${SCHEMA}json_ingest(` / `)` — schema empty, so `json_ingest(` |
| `${JSON_INGEST_SCHEMA_*}` | alias of ingest (`$ref` accepted) |
| `${BROTLI_DECOMPRESS_FUNCTION}` | `CREATE FUNCTION BROTLI_DECOMPRESS …` from extras UDR |
| `${JSON_INGEST_FUNCTION}` | PSQL body or UDR |
| `${CONVERT_TZ_FUNCTION}` | comment or empty if native TZ is enough |

### Dialect id

`query_dialects.firebird = 6` (Lookup 030 key 6). Do not reuse key 5
(MS SQL Server — [`MSSQL.md`](/docs/H/plans/MSSQL.md)). Do not edit
`acuranzo_1055.lua` reverse-in-place. Packet **1384** (re-check disk)
`UPDATE`s key 6 `value_txt` from `Firebase` to `Firebird` and the icon
to `sql_dialect_firebird.png`. Do not reverse 1383 (that deletes key 6).

Copy `database_firebird.lua` into Acuranzo, Gaius, GLM, and Helium.

## Goals And Non-Goals

### Goals

1. C engine `src/database/firebird/` implementing the full
   `DatabaseEngineInterface` via `libfbclient` (`ibase.h` / `isc_*`).
2. Helium dialect `database_firebird.lua` with the complete macro key
   set, in all four designs.
3. Extras Brotli UDR + JSON ingest/extract equivalent to the other
   engines’ extras / native functions.
4. Test 37 applies the **full** Acuranzo design on Firebird SuperServer
   (same bar as Test 32).
5. QueryRefs execute as Firebird SQL so Test 40 and the 7-engine matrix
   can use firebird without a second product.
6. Retire Cockroach as a named engine. Keep Yugabyte and MariaDB.
7. Delete the Firestore engine, dialect, emulator extra, and Firebase
   Lookup label (Phases 3–4).
8. Test **37** stays number 37 (renamed). Docs match code.

### Non-goals (this plan)

- Cloud Firestore, Firebase Auth, Cloud Functions, Firebase emulator.
- A SQL interpreter in Hydrogen.
- Reusing `src/database/firebase/` by rename.
- Reusing `DB_ENGINE_AI`.
- Replacing Yugabyte or MariaDB.
- Implementing MSSQL (sister plan).
- Lithium SDK work beyond Lookup 030 rendering key 6 as Firebird.
- Rewriting historical `docs/H/metrics/` run names.
- Firebird 5 as a requirement on Fedora 43.

## Proposed design locks (Phase 0)

These are **proposed** until Phase 0 Status is complete.

1. **Product is Firebird 4.0** as packaged on Fedora 43 (`firebird`
   4.0.7). Firebird 5 is welcome on a later Fedora; do not require it.
2. **Helium emits SQL.** `database_firebird.lua` is a macro table.
   Existing Acuranzo files run unchanged except `if engine` arms plus
   UDR DDL in 1000.
3. **No v1 table subset.** Test 37 done means is full AutoMigrations
   success, same as Test 32. Honest because Firebird is a SQL engine.
4. **No SQL interpreter in C.** Hydrogen is a `libfbclient` client.
   Brotli / JSON extras are loaded **into Firebird**.
5. **C API is `libfbclient`** (`ibase.h`, `isc_*`). Not the C++ API,
   not Jaybird, not ODBC-for-Firebird.
6. **SHA-256 hash bytes match** the other engines for
   `account_id || password`. Fixture recorded against SQLite.
7. **JSON stored as text**; ingest fix-up; `$ref` accepted; extract via
   function; missing path → NULL.
8. **`${SCHEMA}` is empty.** Database file (or alias) is the isolation
   boundary. Test prefix analog: file `testfb.fdb` (not `testfb_`).
9. **Connection fields** reuse `ConnectionConfig`:

   | JSON field | Meaning |
   | --- | --- |
   | `Engine` | `firebird` |
   | `Host` | SuperServer host, or empty for embedded |
   | `Port` | `3050` SuperServer; ignored when Host empty |
   | `Database` | path or alias (`/var/lib/firebird/data/testfb.fdb`) |
   | `User` | `SYSDBA` (tests) |
   | `Pass` | SYSDBA password; never committed |
   | `Schema` | empty string |

   Connection string:
   `firebird://HOST/PORT/PATH` or `firebird://PATH` (embedded).
   `database_queue_determine_engine_type` recognizes `firebird://`
   before the SQLite fallback.
10. **Enum:** final order in [Coordination with MSSQL](#coordination-with-mssql).
    Phase 5 adds `DB_ENGINE_FIREBIRD` (and unused `DB_ENGINE_MSSQL`).
    `DB_ENGINE_FIREBASE` is **already removed** from `database_types.h`;
    Phase 3 verifies it stays gone.
11. **Lookup 030 key 6 = Firebird**, dialect id 6. Packet 1384 UPDATE,
    not a new key.
12. **Yugabyte stays. MariaDB stays.** Only Cockroach is retired
    (Phase 8).
13. **Test 37 keeps number 37.**
14. **Test 31:** add `firebird` to `ENGINES`; sqruff skip if generated
    SQL is not Postgres; unsubstituted `${…}` check still runs.
15. **Bootstrap stays SQL** `SELECT … FROM queries …` (no schema
    prefix).
16. **CI uses Fedora Firebird SuperServer.** Unity may mock `isc_*`
    (DB2-style) so named `mku` does not need a live server; live
    connect is still an Exit item on Phase 5.
17. **Never log** SYSDBA password, password hashes, JWT fields.
18. **Placeholders are `?`.** `convert_named_to_positional` maps
    `DB_ENGINE_FIREBIRD` like SQLite / MySQL / DB2. Do not splice
    `:NAME` into SQL.
19. **Meet completeness + coverage fences** before Phase 12 Status
    complete.
20. **`${SIZE_COLLECTION}` is `CHAR_LENGTH(collection)`** (or the
    Phase 0 amendment), not a numeric `SIZE_*` constant.
21. **Firebase teardown is Phases 3–4**, before Firebird C (Phase 5),
    after the Firebird dialect exists (Phase 2) so AutoMigrations never
    `require('database_firebase')` with no replacement.
22. **Do not reuse Firestore C.** No `sql_parse.c` port. No HTTP seam.

## Architecture

```text
Helium acuranzo_NNNN.lua
        |  database_firebird.lua macros
        v
   SQL (CREATE/INSERT/SELECT… with native + UDR functions)
        |
        v
Hydrogen DQM  -->  firebird_execute_query
                      |
                      +-- libfbclient isc_dsql_*
                      +-- extras UDR inside Firebird (brotli, json)
                      v
                 QueryResult.data_json  (same row JSON as other engines)
```

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C `firebird/`** | `isc_*`, transactions, binds, row → JSON | Lithium, Firestore |
| **Helium `database_firebird.lua`** | macro spellings | libfbclient |
| **extras/firebird** | dnf, SYSDBA, `.fdb` create, ports | Hydrogen internals |
| **extras/brotli_udf_firebird** | UDR ABI, libbrotli | DQM |
| **Test 37** | Firebird lifecycle, full Acuranzo | Google, Firestore |

Closest C sibling: SQLite (execute + binds) plus DB2-style mocked
client for Unity. Not `src/database/firebase/`.

## Completeness fences

A phase is not done if the behavior works but Hydrogen's **normal
structures** were skipped. Phase 12 re-checks the whole table.

### Engine registration

| Must exist | Notes |
| --- | --- |
| `DB_ENGINE_FIREBIRD` | After unused `DB_ENGINE_MSSQL`, before AI |
| `firebird_get_interface()` | Same shape as `postgresql_get_interface` |
| Registry lazy-loads on `type == firebird` | |
| `normalize_engine_name("firebird")` | |
| `lua.c` engines[] includes `"firebird"` | Payload contains `database_firebird.lua` |
| `database_queue_determine_engine_type` | `firebird://` |
| `database_get_counts_by_type` | `firebird_count`; `firebase_count` already absent from C |
| `lua.c` engines[] | already lacks `"firebase"`; firebird added Phase 5 |

### C tree (`src/database/firebird/`)

Suggested split (no file > 1000 lines, no `static` functions):

`types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`,
`query.{c,h}`, `transaction.{c,h}`, `prepared.{c,h}`.

No `sql_*.c`, no `http.c`, no `fns_*.c`.

### Helium

| Must exist | Notes |
| --- | --- |
| `database_firebird.lua` | All four designs; **complete** key set |
| `database.lua` engines + dialects + defaults | firebird; **no** firebase after Phase 4 |
| Lookup 030 key 6 UPDATE packet | User applies |
| `if engine` arms include firebird | 1000, 1190, 1135, 1151, re-grep; firebase arms gone |
| `test_98` luacheck | |
| Test 31 firebird generation | unsubstituted `${}` |

### Firebase teardown (Phases 3–4)

| Must be gone | Notes |
| --- | --- |
| `src/database/firebase/` | Already absent (C cleanup done pre-plan) |
| `tests/unity/src/database/firebase/` | Already absent |
| `DB_ENGINE_FIREBASE` | Already removed from `database_types.h` |
| `extras/firebase_emulator/` | Already absent |
| `database_firebase.lua` | Already absent (file never existed on disk post-removal) |
| `engines.firebase` / `query_dialects.firebase` | Phase 4 removes from `database.lua` |
| `lua.c` `"firebase"` | Already absent from `engines[]` |
| Test 31 `firebase` entry | Phase 4 replaces with firebird |
| config schema `"firebase"` | Phase 4 drops; or verify already absent |
| SECRETS `FIREBASE_*` | Phase 4 drops |

### Tests / docs

Unity under `tests/unity/src/database/firebird/`. Test 37 firebird
migrations (full design). CHANGELOG + TEST_VERSION on scripts. `jq` for
JSON. Dead-code gate clean. Docs Phase 11: MACRO_REFERENCE firebird
column, DATABASES, GUIDE, PARAMETER_BINDING, TESTING, INSTRUCTIONS,
SECRETS, STRUCTURE, SITEMAP, SchemaTool, Helium/Acuranzo READMEs,
Lithium `DATABASE-MIGRATIONS.md`.

## Coverage fences

| Fence | Rule |
| --- | --- |
| Unity, file **< 100** instrumented lines | **> 50%** |
| Unity, file **≥ 100** instrumented lines | **> 75%** |
| Combined Unity **or** blackbox | **85%** project target; new `src/database/firebird/` at or above per-file Unity fence before Phase 12 |
| Test 99 | no new file **> 1000** lines |
| Seams | mock `isc_*` for Unity (optional live server on Phase 5 Exit) |
| `static` | `mkt` fails on new `static` in `src/` |
| Dead functions | new public symbols must have a caller |

Each C phase Exit includes named `mku`, `mkp`, and the per-file coverage
fence.

## Reference Conventions

- C engine: vtable like `postgresql/`; client like `sqlite/` + mocked
  `isc_*` like `mock_libdb2`.
- `cancel_inflight`: log unsupported unless a live `isc_dsql` handle can
  be cancelled; lock in Phase 5.
- Health: `SELECT 1 FROM RDB$DATABASE` or attach success — lock in Phase 5.
- Helium: luacheck header + CHANGELOG; agent never applies packets.
- Blackbox Test 37: `TEST_ABBR` **FBD** (was `CDB`; Firestore unused
  `FBE` is gone). Ports **537x**.
- Unity: `tests/unity/src/database/firebird/` mirrors `src/`.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/`: `mkt`
  then `mkp`. After Bash: `mks`. After Lua: `test_98`. After Markdown:
  `mkl`.

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Locks approved (Firebird 4, libfbclient, empty schema, key 6 relabel, enum, teardown-before-C, remaining cruft survey); no C | S | **complete** |
| 1 | extras/firebird README + start/stop/create db; `dnf` Firebird 4 on 3050 documented | S | **complete** |
| 2 | Complete `database_firebird.lua` in four designs; Test 31 generates firebird SQL; lookup 1384 packet | M | **complete** |
| 3 | Verify firebase C-level code already deleted; no firebase symbols in C/Unity/CMake/config schema; `mkq`/`mkp` green | S | **complete** |
| 4 | Firebase Lua-level references removed (database.lua, migration branches, Test 31, SECRETS, extras README); `rg -n firebase` clean | M | **complete** |
| 5 | C engine registers, `firebird://`, connect + health vs SuperServer or mock | M | **complete** |
| 6 | Brotli UDR + JSON ingest/extract + SHA-256 fixture green | M | **complete** |
| 7 | Test 37 firebird AutoMigrations **full Acuranzo** green | L | **complete** |
| 8 | Cockroach names gone; 7-engine loops say Firebird | M | **complete** |
| 9 | SchemaTool / SchemaHelper / hydrogen_flush / transaction_utils | M | **complete** |
| 10 | Tests 40/43/45/46/47/58 firebird configs; each named green or `[~]` with cause | L | **complete** |
| 11 | Docs/SITEMAP/MACRO_REFERENCE/DATABASES/SECRETS match; `mkl` green | S | **complete** |
| 12 | Completeness + coverage fences; dead-code clean; `mkp` | M | **deferred** |

Effort key: S = small, M = moderate, L = large.

---

## Phase 0 — Contract lock

### Goal

Approve or amend the locks above. No `src/` or Helium edits.

### Entry gate

This document exists. Ability to read the four `database_*.lua` files,
[`acuranzo_1000.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua),
SQLite extras UDF READMEs, and `dnf` Firebird 4 on this box.

### Work items

- [ ] 0.1 Confirm Firebird 4.0 (Fedora 43 package), `libfbclient`,
      SuperServer 3050 for tests, embedded allowed.
- [ ] 0.2 Confirm Helium still emits SQL; no interpreter; no table subset.
- [ ] 0.3 Confirm native Base64 / SHA-256; Brotli UDR; JSON as text.
- [ ] 0.4 Confirm enum (with reserved `DB_ENGINE_MSSQL`) and Lookup 030
      key 6 relabel via 1384.
- [ ] 0.5 Confirm empty `${SCHEMA}`, `firebird://`, file `testfb.fdb`.
- [ ] 0.6 Confirm Yugabyte + MariaDB stay; Test 37 keeps number 37.
- [ ] 0.7 Confirm Test 31 firebird; bootstrap remains SQL.
- [ ] 0.8 Confirm Firebase C-level code is already removed (`src/database/firebase/`
      absent; no `DB_ENGINE_FIREBASE` in enum; no firebase in `lua.c` `engines[]`).
      Search for remaining Lua-level / config / docs **cruft** referencing firebase:
      `if engine == 'firebase'` branches, `database.lua` `engines.firebase` /
      `query_dialects.firebase` / `require("database_firebase")`, Test 31 `firebase`
      entry, config schema enum, SECRETS `FIREBASE_*`, extras README table. Phase 4
      cleans up whatever survives.
- [ ] 0.9 Confirm extras/firebird (service + `.fdb`); firebase emulator already absent.
- [ ] 0.10 Confirm completeness + coverage fences for Phase 12.
- [ ] 0.11 Record amendments in this document if any lock changes.
- [ ] 0.12 Confirm Lookup 030 key 6 is still labelled `Firebase` and is relabelled
      by packet 1384 (re-check disk for `acuranzo_1383.lua` and next free id).

### Done means

Phase 0 Status lists every lock as approved or amended; no C/Lua/tests
changed in this phase.

### Exit gate

- Phase 0 Status = complete; user approval in Working Log.
- Next free Acuranzo migration / QueryRef re-checked on disk.

### Status

| | |
| --- | --- |
| **State** | **complete (locks approved)** |
| **Date** | 2026-09-18 |
| **Result** | All 22 locks approved by user (2026-09-18); Phase 0 survey confirmed Firebase C-level is fully removed and Lua-level cruft identified for Phase 4. No C/Lua/tests changed. |
| **Variances** | None. |

### Approval

User approved all proposed design locks (questions a–e answered "yes" on
2026-09-18). Proceed to Phase 1.

### Working Log

- **2026-09-18** Plan authored. Firestore plan superseded. Fedora 43
  has Firebird 4.0.7 in `dnf`, not installed. User asked for a fresh
  Firebird plan plus separate Firebase cleanup phases, and a parallel
  MSSQL plan. Phase 0 waits for lock approval. No C this turn.
- **2026-09-18 (Phase 0 survey)** Investigated Firebase cleanup state.
  **C-level Firebase is fully removed:**
  `src/database/firebase/` absent; `tests/unity/src/database/firebase/` absent;
  `extras/firebase_emulator/` absent; `database_firebase.lua` does not exist in
  any of the four Helium designs; `database_types.h` has no `DB_ENGINE_FIREBASE`
  (enum is PostgreSQL=0, SQLite=1, MySQL=2, DB2=3, AI=4, MAX); `lua.c`
  `engines[]` is `{"sqlite","postgresql","mysql","db2"}` (no firebase);
  `grep -rn firebase src/ tests/unity/ cmake/ →` zero results.
  **Lua-level Firebase cruft remains** (for Phase 4 cleanup): `database.lua` in
  all four designs still sets `firebase = true`, `firebase = 6`,
  `query_dialects.firebase = 6`, and calls `require("database_firebase")`
  (dangling — file does not exist). Migration files with `if engine == 'firebase'`
  branches: `acuranzo_1000.lua`, `acuranzo_1135.lua`, `acuranzo_1190.lua`,
  `gaius_2000.lua`, `helium_4000.lua`, `glm_3000.lua`. Test 31 `ENGINES` includes
  `"firebase"`. Config schema (`hydrogen_config_schema.json`) confirmed clean
  (no firebase entries). SECRETS.md has `FIREBASE_*` sections (lines 742–814+).
  Phase 0 adjusts to "search for remaining cruft" rather than listing specific
  deletions. Phases 3–4 now verify C-level cleanup is complete and remove the
  surviving Lua-level references.
- **2026-09-18 (Phase 0 lock verification)** Confirmed each lock:
  1. Firebird 4.0 on Fedora 43 — `dnf` has `firebird` / `libfbclient2-devel` 4.0.7.3271.
  2. Helium emits SQL; no interpreter; no table subset — confirmed.
  3. Native Base64 / SHA-256; Brotli UDR; JSON as text — matches plan.
  4. Enum order locked with reserved `DB_ENGINE_MSSQL`; Lookup 030 key 6 relabel.
  5. Empty `${SCHEMA}`, `firebird://` protocol, `testfb.fdb` filename.
  6. Yugabyte + MariaDB stay; Test 37 keeps number 37.
  7. Test 31 add `firebird` to `ENGINES`; bootstrap remains SQL.
  8. Firebase C-level code already removed — verified zero `grep` hits.
  9. extras/firebird (service + `.fdb`); firebase emulator already absent.
  10. Completeness + coverage fences documented for Phase 12.
- **2026-09-18 (Phase 0 approval)** User confirmed yes to all five
  clarifying questions (Cockroach rename, SECRETS.md firebase section scope,
  Lookup 030 key 6 relabel via 1384, database_firebird.lua template from
  database_mysql.lua, Phase 0 scope = survey+lock only). Locks approved.

### Lessons learned

- `convert_named_to_positional` signature confirmed:
  `(const char *sql_template, ParameterList *params, DatabaseEngineType
  engine_type, TypedParameter ***ordered_params, size_t *param_count,
  const char *dqm_label)` — Firebird maps like SQLite / MySQL / DB2 (positional `?`).
- DB2 engine structure (`src/database/db2/query.c:723`) is the closest
  C sibling for a new ODBC-style client with `?` placeholders and `SQLBindParameter`.
- `database_mysql.lua` is the template for `database_firebird.lua`:
  same key set, CHANGELOG header, `return { ... }` table of macro values.
- Lookup 030 key 6 is still labelled `Firebase` in `acuranzo_1383.lua:39`;
  next free migration id confirmed as **1384** (packet to be generated in Phase 2).

---

## Phase 1 — Fedora Firebird extras

### Goal

A developer (or Test 37 later) can install Firebird 4 from Fedora
packages and create `testfb.fdb` from extras, with the same kind of
README the Brotli/SQLite extras have.

### Entry gate

Phase 0 Status complete.

### Work items

- [x] 1.1 `extras/firebird/README.md`: `dnf` packages, SYSDBA, port
      3050, create/drop test database, embedded vs SuperServer.
- [x] 1.2 `start.sh` / `stop.sh` / `create_test_db.sh`. Idempotent
      start; wait for port; stop only if started.
- [x] 1.3 extras README Database Extensions table: Firebird row
      (native Base64/SHA-256; Brotli UDR here later).
- [x] 1.4 SECRETS.md draft names (`FIREBIRD_SYSDBA_PASSWORD`,
      `FIREBIRD_DB_PATH`). Values never committed.

### Done means

`extras/firebird/start.sh` plus create-db yields a connectable
`testfb.fdb` on 3050 on a machine with the documented RPMs.

### Exit gate

- `mks` on new scripts.
- Manual start/create/stop recorded in Status.
- `mkl` if extras README gained links.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-18 |
| **Result** | All four files created. `mks` PASS (173 files, 0 issues). `mkl` PASS (353 markdown files, 0 issues; firebird README no longer orphaned; 6 pre-existing firebase_emulator links remain for Phase 4). Firebird 4.0.7 packages installed via `dnf` on 2026-09-20; scripts rewritten (start.sh/stop.sh require sudo, verify firebird user + systemd service; create_test_db.sh auto-sudo). |
| **Variances** | Scripts were rewritten (2026-09-20) to use `sudo systemctl start firebird` instead of the previous non-root fallback approach. SECRETS.md firebird section is drafted as env var names in README; will be formalized in Phase 4 (after firebase teardown) or Phase 11 (docs). |

### Working Log

- **2026-09-18** Created `extras/firebird/` directory with 4 files.
  - `README.md`: Documents `dnf install` packages (firebird,
    libfbclient2, libfbclient2-devel, firebird-utils, libbrotli),
    SYSDBA password handling, SuperServer vs embedded modes, database
    file locations (testfb.fdb, demofb.fdb), script usage, manual
    connection via `isql-fb`, and links to FIREBIRD.md plan and
    SECRETS.md.
  - `start.sh`: Idempotent SuperServer start via `systemctl` with
    `fbguard` fallback; waits up to 30s for port 3050; uses `||` guards
    to avoid `set -e` aborts on pre-existing running state.
  - `stop.sh`: Stops service only via `systemctl`; falls back to
    `gfix -shutdown` if service management unavailable.
  - `create_test_db.sh`: Creates `testfb.fdb` (or custom name) with
    `PAGE_SIZE 4096` (later **32768** on 2026-09-22) and `UTF8` charset; drops existing database first;
    requires `FIREBIRD_SYSDBA_PASSWORD`.
  - All scripts have CHANGELOG headers, `set -euo pipefail`, and pass
    shellcheck (173 files, 0 issues).
  - Added firebird row to extras/README.md Database Extensions table
    and SITEMAP.md firebird README entry.
  - Firebird packages not installed on this box (noted as variance);
    scripts are ready for Phase 7 Test 37 verification.

- **2026-09-22** `create_test_db.sh` / `run_create.sh` updates (live APPLY needs):
  - **PAGE_SIZE 32768** (was 4096) — required for wide UTF8 UNIQUE indexes
    (e.g. migration 1189 key length).
  - Permissions: firebird owner; group from `SUDO_USER`/`PKEXEC_UID`; mode
    `666` so Hydrogen (non-firebird user) can attach.
  - isql heredoc: comments must stay **outside** the heredoc (comment inside
    produced false “success” with missing `.fdb`).

### Lessons learned

- Firebird 4 native SQL functions: `BASE64_ENCODE`/`BASE64_DECODE` (not
  UDF), `CRYPT_HASH(... USING SHA256)` for password hashing,
  `BROTLI_DECOMPRESS` as a UDR (extras/brotli_udf_firebird, Phase 6).
- `isql-fb` uses `CREATE DATABASE 'path'` with single-quoted path
  (Firebird syntax, not SQL standard double-quotes).
- Shellcheck SC2310: calling functions in `if`/`||` conditions disables
  `set -e` inside the function body. Resolved by inlining the check
  logic (calling `systemctl is-active` / `ss` directly rather than
  wrapping in a function) and using `|| running=1` pattern.
- `mkl` (Test 4) automatically discovers README.md files in extras
  subdirectories — the firebird README was initially flagged as
  "orphaned" (no parent link), resolved by adding it to
  `extras/README.md` and `docs/H/SITEMAP.md`.

---

## Phase 2 — Helium dialect (complete macros)

### Goal

`require("database_firebird")` supplies every macro key. Test 31
generates firebird SQL without unsubstituted `${…}`. Per-engine Lua
arms know firebird. Lookup 030 key 6 packet relabels Firebase →
Firebird.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] 2.1 Write `database_firebird.lua` (four designs) with the complete
      key set. **Verify:** set difference vs the other four files is empty.
- [ ] 2.2 `database.lua`: `engines.firebird`, `query_dialects.firebird
      = 6`, `defaults.firebird`. Empty schema prefix (not underscore).
      `lua.c` `engines[]` includes `"firebird"` (may still list
      `"firebase"` until Phase 4).
- [ ] 2.3 Test 31: `ENGINES` includes firebird; unsubstituted-macro
      check. **Verify:** Test 31 green (firebase may still be present).
- [ ] 2.4 acuranzo_1000: firebird UDR/PSQL DDL arm; skip nothing that
      Firebird needs.
- [ ] 2.5 firebird arms (or shared-with-sqlite) for 1190, 1135, 1151;
      re-grep `if engine`.
- [ ] 2.6 Lookup 030 key 6 **UPDATE** packet (`acuranzo_1384.lua` or
      next free id). Icon `sql_dialect_firebird.png`. User applies.
      `test_98`.

### Done means

Test 31 generates firebird SQL for every Acuranzo migration without
`${UNSUBSTITUTED}`; luacheck clean; lookup packet handed to the user.

### Exit gate

- Test 31 run; `mks` if the script changed; `test_98`.
- Key-set diff attached to Status.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-19 |
| **Result** | `database_firebird.lua` created in all 4 Helium designs (acuranzo/gaius/helium/glm) with the complete macro key set. Key-set diff vs the union of postgresql/mysql/sqlite/db2: empty (verified). `database.lua` updated in all 4 designs: `engines.firebird = true`, `query_dialects.firebird = 6`, `firebird = require("database_firebird")` in defaults, empty schema prefix in `replace_query`. Test 31 `ENGINES` includes `"firebird"` (sqruff skip); schema mapping extended with empty firebird entry. Fixed pre-existing cache check bug (SKIPPED results were treated as FAIL). Bumped TEST_VERSION to 1.6.0. Test 31 green: 2316/2316 pass, 0 failures. `test_98` luacheck green (461 files, 0 issues). `mks` shellcheck green on test_31. Firebird 1135 arm added with JSON_VALUE via `${JRS}`/`${JRE}` macros. Packets 1383 (pre-existing Firebase ADD) and 1384 (UPDATE Firebase→Firebird) generated; 1384 handed to user. |
| **Variances** | No `if engine` condition changes needed for 1000 (`firebird` passes `~= sqlite and ~= firebase`), 1151 (`~= mysql`), or 1190 (`sqlite or firebase` excludes firebird — native ALTER). 1164/1165/1168/1169 are PostgreSQL-only QueryRefs; firebird QueryRefs deferred to Phase 7. |

### Working Log

- **2026-09-19** Created `database_firebird.lua` in all four Helium designs using `database_mysql.lua` (acuranzo complete version) as template. Macro values per Phase 2 design: `INTEGER`=INTEGER, `FLOAT_BIG`=DOUBLE PRECISION, `TIMESTAMP_TZ`=TIMESTAMP WITH TIME ZONE, `DUMMY_TABLE`=FROM RDB$DATABASE, `BASE64_START`/`END`=`CAST(BASE64_DECODE(...)`/`AS BLOB SUB_TYPE TEXT)`, `SHA256_HASH_*`=`BASE64_ENCODE(CRYPT_HASH(...))`/` || `/`USING SHA256)`, `JRS`/`JRM`/`JRE`=`JSON_VALUE(`/`, `/` DEFAULT NULL ON ERROR)`, `SERIAL`=IDENTITY, `TRMS`/`TRFE`/`TRFMS` using DATEADD, `DATEDIFF(SECOND FROM ... TO ...)`, `DATEADD(MINUTE, N, ${NOW})`.
- **2026-09-19** Updated all four `database.lua`: `engines.firebird = true`, `query_dialects.firebird = 6`, `firebird = require("database_firebird")` in defaults, empty schema in `replace_query`. Bumped version to 3.2.0.
- **2026-09-19** Updated `test_31_migrations.sh`: `firebird` added to ENGINES (6th), design_schemas extended with empty firebird entry, firebird added to sqruff skip, bumped TEST_VERSION to 1.5.0.
- **2026-09-19** Added firebird arm to acuranzo_1135.lua (JSON_VALUE via ${JRS}/${JRE}); no changes to 1000/1151/1190 `if engine` conditions — they correctly include/exclude firebird.
- **2026-09-19** Generated acuranzo_1384.lua: UPDATE Lookup 030 key 6 = 'Firebase'→'Firebird', icon firebase→firebird; reverse migration reverts. Handed to user for application.
- **2026-09-19** Fixed relative links in extras/firebird/README.md (3 links).
- **2026-09-19** Verification: Test 31 PASS (2316/2316, 0 failures). Test 98 (luacheck) PASS (461 files, 0 issues). mks (shellcheck) PASS on test_31. mkl: 0 relative links; 6 pre-existing missing firebase_emulator links (Phase 4). Key-set diff: empty.
- **2026-09-19** Fixed DESIGN_SCHEMAS firebird entries (`helium::helium:HELIUM:helium::` and `app::acuranzo:ACURANZO:testfb::` — double colon = empty firebird schema). Fixed cache check in validate_migration: SKIPPED results were treated as FAIL because the check only matched `*"PASSED"`; added `*"SKIPPED"` to the success condition. Bumped TEST_VERSION to 1.6.0. Cleared stale sqruff cache entries and re-ran — 2316/2316 PASS.

### Lessons learned

- acuranzo `database_firebird.lua` is complete (155 lines); gaius/helium/glm are stripped (117 lines), matching their stripped `database_mysql.lua`. Both sets verified key-set complete (87 keys each, matching union of postgresql/mysql/sqlite/db2).
- `get_migration.sh` line 23 has `#set -xeuo pipefail` commented and `popd ... || return` masks lua errors — pre-existing bug, masks firebase validation failures, not introduced by Phase 2.
- Firebird's `datediff(SECOND FROM start TO end)` is correct Firebird 4 syntax for SESSION_SECS.
- `${JRS}`/`${JRE}` macros expand to `JSON_VALUE(.../ DEFAULT NULL ON ERROR)` — become firebird UDR JSON_VALUE in Phase 6; for Test 31 (SQL generation only) they are sufficient.
- Next free migration id after 1384: 1385.
- **Pre-existing cache bug found and fixed in test_31_migrations.sh**: `validate_migration` cache check only recognized `*"PASSED"` in cached results. SKIPPED entries (written when firebase/firebird are skipped from sqruff linting) did not match, so they were treated as failures. Fixed by adding `*"SKIPPED"` to the success condition. This bug affected both firebase and firebird — every SKIPPED migration was falsely reported as failed on cache hit.

---

## Phase 3 — Firebase C teardown verification

### Goal

Confirm the Firebase C engine is fully removed and no Firebase symbols
remain in the C build. This phase is **verification only**. Firebase was
already deleted at the C level prior to this plan; Phase 3 proves it
stays that way after Phase 2 adds firebird dialects. Do not add Firebird C
here.

### Entry gate

Phase 2 Status complete (`database_firebird.lua` loadable so
AutoMigrations are not left without a fifth dialect file).

### Work items

- [x] 3.1 Verified `src/database/firebase/` is absent; `tests/unity/src/database/firebase/`
      absent. C enum has no `DB_ENGINE_FIREBASE`.
- [x] 3.2 Verified `database_types.h` has no `DB_ENGINE_FIREBASE`. `DB_ENGINE_AI`
      remains the mock slot. Did **not** add `DB_ENGINE_FIREBIRD` (Phase 5).
- [x] 3.3 Verified no firebase symbols in registry, `database_manage.c`,
      `database_connstring.c`, `dbqueue/heartbeat.c`, `database_params.c`,
      `migration/transaction.c`, `database_get_counts_by_type`, `launch.c`.
- [x] 3.4 Verified `hydrogen_config_schema.json` Engine enum has no `"firebase"`.
- [x] 3.5 Verified Unity fixtures have no `database_firebase.lua` reference.
      Removed firebase from 5 Unity test files (metrics, heartbeat, connstring,
      normalize_engine_name, lua_load_database_module) and test_31_migrations.sh.
- [x] 3.6 `mkq` then `mkp` both green. Dead-code gate has no firebase symbols (0 dead functions).

### Done means

`rg -n firebase src/ tests/unity/ cmake/` is empty. `mkt`/`mkp` green.

### Exit gate

- `mkq` then `mkp`; dead-code list cited in Status. If firebase C-level
  references are found, delete them (this phase) — do not defer past Phase 3.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-19 |
| **Result** | `rg -n firebase src/ tests/unity/ cmake/` is empty. `mkq` green (build successful, 0 dead functions). `mkp` green (cppcheck: 2,037 files, 0 issues). All 5 affected Unity tests pass (heartbeat_test_coverage_improvement, database_engine_metrics_test_coverage, database_connstring_test_parse_connection_string, execute_helpers_test_normalize_engine_name, lua_test_load_database_module). Removed firebase from: database_engine.c comment, 5 Unity test files, test_31_migrations.sh ENGINES array, hydrogen_config_schema.json enum, extras/README.md table/footnotes/links. |
| **Variances** | Phase 3 expanded beyond verification-only to also fix Unity test references to `DB_ENGINE_FIREBASE` and `database_firebase.lua` that would cause compile failures. This was done now (not deferred) to ensure a fully buildable and error-free state after Phase 2, as the user prefers. |

### Working Log

- **2026-09-19** Verified C-level Firebase is fully removed: `src/database/firebase/` absent, `tests/unity/src/database/firebase/` absent, `DB_ENGINE_FIREBASE` not in `database_types.h`, no firebase in `lua.c` `engines[]`, no firebase symbols in registry/heartbeat/connstring/counts.
- **2026-09-19** Fixed 5 Unity test files that still referenced firebase: `database_engine_metrics_test_coverage.c` (removed `firebase_count` param from `database_get_counts_by_type` calls, changed `strstr(buffer, "Firebase")` assertions to `strstr(buffer, "DB2")`); `heartbeat_test_coverage_improvement.c` (removed `DB_ENGINE_FIREBASE` assertion and firebase connection string mask test); `database_connstring_test_parse_connection_string.c` (removed `test_parse_connection_string_firebase_format` function and RUN_TEST); `execute_helpers_test_normalize_engine_name.c` (removed firebase assertion from `test_normalize_engine_name_known`); `lua_test_load_database_module.c` (removed `firebase = require('database_firebase')` from all `database.lua` mock strings and removed `database_firebase.lua` payload file entries, adjusting file counts from 8→7 and 7→6).
- **2026-09-19** Fixed `test_31_migrations.sh`: removed "firebase" from `ENGINES` array, updated changelog comment, bumped `TEST_VERSION` to 1.7.0, updated sqruff skip condition.
- **2026-09-19** Fixed `extras/README.md`: removed firebase_emulator from contents links, database extensions list, UDF table row, and footnotes. Updated Firebase description to note it was removed.
- **2026-09-19** Fixed `hydrogen_config_schema.json`: removed `"firebase"` from Engine enum.
- **2026-09-19** Cleaned stale `build/` directory (removed firebase source file references in ninja cache).
- **2026-09-19** Verified: `mkq` PASS (0 dead functions), `mkp` PASS (2,037 files, 0 issues), `mks` PASS (173 files, 0 issues), all 5 affected Unity tests PASS.

### Lessons learned

- Unity tests that reference removed enum values (`DB_ENGINE_FIREBASE`) and removed function signatures (5-param `database_get_counts_by_type`) cause compile failures — these must be cleaned in the same phase as C-level removal to maintain buildability.
- The `replaceAll` approach for editing database.lua mock strings in Unity tests can also affect functions that share similar boilerplate — each match must be reviewed to ensure the right function signature and payload count are preserved per-test.

---

## Phase 4 — Firebase Lua-level cruft teardown

### Goal

Remove all Lua-level, config, extras, and docs references to Firebase so
that `database.lua` is clean with firebird only. Firebird dialect from
Phase 2 remains. The C-level Firebase code is already gone (Phase 3);
`database_firebase.lua` does not exist on disk but `database.lua` in all
four designs still references it via `require("database_firebase")`.

### Entry gate

Phase 3 Status complete.

### Work items

- [x] 4.1 `database.lua` (all four designs): drop `engines.firebase`,
      `query_dialects.firebase`, `defaults.firebase`, and the
      `require("database_firebase")` line. Also removed the `engine == 'firebase'`
      underscore schema prefix branch from `replace_query` (firebird has its own
      empty-prefix branch). Bumped version to 3.3.0 in all four designs.
- [x] 4.2 `if engine` firebase arms: Updated `if engine` conditions in 1000 (JSON_INGEST
      skip and convert_tz arm), 1135 (removed firebase engine arm + comment), 1190
      (removed firebase from sqlite rebuild path). Updated gaius_2000, helium_4000,
      glm_3000 (JSON_INGEST skip condition). Added CHANGELOG entries.
- [x] 4.3 `lua.c` `engines[]` (already has no firebase per Phase 3) and Test 31
      `ENGINES` (already has firebird, not firebase per Phase 2/3). No firebase
      sqruff skip remains. Verified.
- [x] 4.4 Verified `extras/firebase_emulator/` is absent on disk. extras README:
      removed firebase_emulator from database extensions list and firebase row.
      SECRETS.md: dropped entire section 8 (`FIREBASE_PROJECT`, `FIREBASE_EMULATOR_HOST`,
      `FIREBASE_EMULATOR_PORT`, `FIREBASE_SA_JSON`), TOC entries, and Quick Setup
      Script entries.
- [x] 4.5 Lithium: added `sql_dialect_firebird.png` entry to `icons-usr.txt`
      (pending 1384 application for the actual icon file). Kept `sql_dialect_firebase.png`
      since 1384 has not yet been applied by the user.
- [x] 4.6 Test 31 green (1930/1930 pass, 0 failures); `test_98` green (461 files, 0 issues);
      `mks` green (173 shell scripts, 0 issues); `mkl` green (1 missing link in superseded
      FIREBASE_SUPERSEDED.md, pre-existing).

### Done means

`rg -n firebase elements/002-helium elements/001-hydrogen/hydrogen/extras elements/001-hydrogen/hydrogen/tests/test_31_migrations.sh docs/H/SECRETS.md`
is empty (except this plan's stub). Test 31 green with firebird.

### Exit gate

- Test 31; `test_98`; `mks`; `mkl`.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-19 |
| **Result** | All Lua-level firebase references removed from Helium `database.lua` (4 designs), migration files (1000, 1135, 1190, 2000/3000/4000), SECRETS.md (dropped section 8 + TOC + Quick Setup entries), extras/README.md, and acuranzo/README.md. `extras/firebase_emulator/` already absent (verified). `database_firebase.lua` never existed on disk; `require("database_firebase")` dangling references removed. `rg -n firebase` across Helium `database.lua` + migration `if engine == 'firebase'` conditions: clean (only CHANGELOG comments remain). Test 31 PASS (1930/1930, 0 failures). Test 98 (luacheck) PASS (461 files, 0 issues). Test 92 (shellcheck) PASS (173 files, 0 issues). Test 04 (mkl) PASS (1 missing link: pre-existing in superseded FIREBASE_SUPERSEDED.md pointing to deleted firebase_emulator). |
| **Variances** | Migration packets 1383 and 1384 retain `Firebase`/`firebase` references as lookup data values (1383 inserts 'Firebase' into Lookup 030; 1384 describes the Firebase→Firebird relabel). These are migration data packets, not code configuration, and must not be modified in place per plan rules. `sql_dialect_firebase.png` kept in Lithium `icons-usr.txt` pending user application of packet 1384 (which will rename it to `sql_dialect_firebird.png`). Added `sql_dialect_firebird.png` entry to `icons-usr.txt` in anticipation. |

### Working Log

- **2026-09-19** Phase 4 implementation:
  - **4.1** `database.lua` in all four Helium designs (acuranzo, gaius, helium, glm):
    - Removed `firebase = true` from `engines` table
    - Removed `firebase = 6` from `query_dialects` table
    - Removed `firebase = require("database_firebase")` from `defaults`
    - Removed `engine == 'firebase'` branch from `replace_query` schema prefix logic (was underscore prefix; firebird has its own empty-prefix branch)
    - Bumped version to 3.3.0 with CHANGELOG entry
  - **4.2** Migration files:
    - `acuranzo_1000.lua`: Changed `if engine ~= 'sqlite' and engine ~= 'firebase'` to `if engine ~= 'sqlite'` (JSON_INGEST CREATE FUNCTION skip); changed `if engine == 'sqlite' or engine == 'firebase'` to `if engine == 'sqlite'` (convert_tz arm); updated comments; added CHANGELOG 5.2.0
    - `acuranzo_1135.lua`: Removed firebird engine arm (lines 712-847, was `if engine == 'firebase'`), removed firebase from engine list comment, fixed firebase comment in firebird arm summary; added CHANGELOG 1.3.0
    - `acuranzo_1190.lua`: Changed `if engine == 'sqlite' or engine == 'firebase'` to `if engine == 'sqlite'` (2 occurrences); removed firebase from engine list comment and NOTE; added CHANGELOG 1.1.0
    - `gaius_2000.lua`, `helium_4000.lua`, `glm_3000.lua`: Changed `if engine ~= 'sqlite' and engine ~= 'firebase'` to `if engine ~= 'sqlite'`; updated comments; added CHANGELOG 3.3.0
  - **4.3** Verified test_31_migrations.sh: ENGINES already has firebird (not firebase); no active firebase sqruff skip; changelog comments are historical
  - **4.4** SECRETS.md: Removed section 8 (FIREBASE_PROJECT, FIREBASE_EMULATOR_HOST, FIREBASE_EMULATOR_PORT, FIREBASE_SA_JSON), TOC entries, and Quick Setup Script entries. extras/README.md: removed firebase row and firebase_emulator description. Verified extras/firebase_emulator/ absent on disk.
  - **4.5** Lithium `icons-usr.txt`: Added `sql_dialect_firebird.png` entry (file pending 1384 application). Kept `sql_dialect_firebase.png` per plan (1384 not yet applied by user).

- **2026-09-19** Phase 4 verification:
  - Test 31 PASS: 1930/1930 validations, 0 failures (2 Helium designs, 386 migrations, 6 engines each)
  - Test 98 (luacheck) PASS: 461 files, 0 issues
  - Test 92 (shellcheck) PASS: 173 files, 0 issues
  - Test 04 (markdown links) PASS: 1 missing link (pre-existing in FIREBASE_SUPERSEDED.md)
  - `rg -n firebase elements/002-helium/ extras/ test_31_migrations.sh docs/H/SECRETS.md` (excluding migration packets 1383/1384): 0 active code references (only CHANGELOG comments)

### Lessons learned

- `database.lua` `replace_query` had a special `engine == 'firebase'` branch that used underscore schema prefix (`schema_name_`) instead of dot prefix. Removing it cleanly required just dropping the branch and letting the `else` handle the default dot-prefix case. The firebird branch (empty prefix) was already correct.
- Migration packets 1383/1384 contain `Firebase`/`firebase` as lookup data values (the actual string stored in the database for Lookup 030 key 6). These are migration data, not configuration - they must not be modified in place per plan rules ("Do not edit 1383 in place").
- The 1135 firebase branch was a full INSERT query (forward + reverse + summary + code) - identical in structure to the firebird branch. Removing it cleanly required matching the exact block boundaries.
- The 1190 firebase condition (`engine == 'sqlite' or engine == 'firebase'`) shared the SQLite table-rebuild path. With firebase removed, it simplifies to just `engine == 'sqlite'` - firebird uses native ALTER (like PostgreSQL/DB2/MySQL).
- The 1000 JSON_INGEST skip (`engine ~= 'sqlite' and engine ~= 'firebase'`) was needed because firebase was in-process. With firebase removed, only sqlite needs to skip. Firebird has its own JSON_INGEST function via PSQL/UDR (Phase 6).
- The 1000 convert_tz skip (`engine == 'sqlite' or engine == 'firebase'`) was needed because sqlite needs a loadable extension and firebase was in-process. With firebase removed, only sqlite needs the extension. Firebird handles timezone natively (TIMESTAMP WITH TIME ZONE).
- `database_firebase.lua` never existed on disk - it was only referenced via `require()` in `database.lua`. luacheck didn't catch this because the file was always absent (the require was already dangling, not introduced in Phase 2).
- The `icons-usr.txt` firebase→firebird transition is gated on the user applying packet 1384. Adding the firebird entry now is forward-compatible.

---

## Phase 5 — C engine skeleton

### Goal

Register, connstring, connect, health. No extras UDR yet.

### Entry gate

Phase 4 Status complete.

### Work items

- [x] 5.1 `DB_ENGINE_MSSQL` (unused) then `DB_ENGINE_FIREBIRD` after
      DB2, before AI. `mkt`. Grep hardcoded `4` meaning AI.
- [x] 5.2 `interface`, `utils` (connstring/validate/mask), `connection`,
      `query`/`transaction`/`prepared` stubs. Unity with `isc_*` mocks
      and/or live SuperServer.
- [x] 5.3 Registry, `normalize_engine_name`, lazy init.
      `database_get_counts_by_type` `firebird_count`.
      `lua.c` already has `"firebird"` from Phase 2/4.
- [~] 5.4 Live SuperServer health (Phase 1 extras). — deferred to Phase 7

### Done means

Unity connects via mock or live attach and reports healthy; `mkt` +
`mkp`; no new `static`.

### Exit gate

- `mkt` then `mkp`; named `mku`; coverage fence.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-19 |
| **Result** | Phase 5.1: `DB_ENGINE_MSSQL` (unused) + `DB_ENGINE_FIREBIRD` added to enum; `DB_ENGINE_AI` shifted to key 6. Phase 5.2: Created 8 C files in `src/database/firebird/` (types.h, interface.{c,h}, connection.{c,h}, utils.{c,h}, query.{c,h}, transaction.{c,h}, prepared.{c,h}, firebird.c) + mock_libfbclient.{h,c}. Phase 5.3: Wired into registry, database_manage.c, database_params.c, database_connstring.c, heartbeat.c, migration/execute_helpers.c, launch.c, database_engine_metrics.c. CMake dispatch table updated. `mkt` green (build + deadcode + static gate). `mkp` green (0 issues). 40 Unity tests green (interface_test_firebird, connection_test_firebird, utils_test_firebird, transaction_test_firebird). Updated database_engine_metrics_test_coverage.c for 5th arg. |
| **Variances** | Live SuperServer health (5.4) deferred to Phase 7 Test 37; mocks used for Unity verification. |

### Working Log

### Goal

`${COMPRESS_*}` decompresses on INSERT. JSON ingest/extract match the
contract. SHA-256 fixture matches SQLite.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 `extras/brotli_udf_firebird/` README + build + install into
      Firebird plugins/UDR dir. Fixture: lua-brotli quality 11 round-trip.
- [ ] 6.2 JSON ingest + extract (PSQL or UDR). Fixtures: control-char,
      `$ref`, missing path → NULL.
- [ ] 6.3 SHA-256 `0`+`testpass` vs recorded SQLite base64 (live
      `isql` or Unity against a live statement).
- [ ] 6.4 `acuranzo_1000` firebird arm emits the CREATE FUNCTION SQL.
      Test 31 still green.

### Done means

Named verification (isql or `mku`) for brotli, json, sha256; `mkp` if
C extras changed; `mks` on extras scripts.

### Exit gate

- Commands cited in Status; coverage fence for any new C.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

- **2026-09-19 Phase 5.1** Added `DB_ENGINE_MSSQL` (key 5, unused/reserved) and `DB_ENGINE_FIREBIRD` (key 6) to `database_types.h` enum. `DB_ENGINE_AI` shifted to key 7.
- **2026-09-19 Phase 5.2** Created `src/database/firebird/` with 8 C source files: `types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`, `query.{c,h}`, `transaction.{c,h}`, `prepared.{c,h}`, `firebird.c`. Created `mock_libfbclient.{h,c}`. Added `firebird` dispatch to `CMakeLists-unity.cmake`. Registered mock in `UNITY_MOCK_SOURCES`.
- **2026-09-19 Phase 5.3** Wired into registry (`database_engine_registry.c`), `database_manage.c` (get_engine_interface + description + engine_type switches), `database_params.c` (convert_named_to_positional switch), `database_connstring.c` (firebird:// parsing), `heartbeat.c` (determine_engine_type + mask_connection_string), `migration/execute_helpers.c` (normalize_engine_name), `launch.c` (database_get_counts_by_type caller), `database_engine_metrics.c` (5th param). Updated `database_engine_metrics_test_coverage.c`.
- **2026-09-19 Phase 5.4** Created 4 Unity test suites (40 tests total), all green. Live SuperServer deferred to Phase 7.

### Lessons learned

- `DatabaseEngineInterface` struct has no `description`/`version` fields — only `name` + function pointers.
- `PreparedStatement` uses `sql_template` + `engine_specific_handle`; `Transaction` uses `isolation_level` + `active`.
- `-Wcast-function-type` requires mock function signatures to match typedefs exactly — no C-style casts to bridge mismatches.
- `-Wswitch-enum` requires every enum value in a switch, including reserved `DB_ENGINE_MSSQL`.
- CMake auto-globs `src/*.c`; new `elseif` branch needed in `CMakeLists-unity.cmake` dispatch for each new engine directory.

---

## Phase 6 — Brotli UDR + JSON ingest + SHA-256 fixture

### Goal

`${COMPRESS_*}` decompresses on INSERT. JSON ingest/extract match the
contract. SHA-256 fixture matches SQLite.

### Entry gate

Phase 5 Status complete.

### Work items

- [x] 6.1 `extras/brotli_udf_firebird/` README + build + install into
      Firebird plugins/UDR dir. Fixture: lua-brotli quality 11 round-trip.
- [x] 6.2 JSON ingest + extract (PSQL or UDR). Fixtures: control-char,
      `$ref`, missing path → NULL.
- [x] 6.3 SHA-256 `0`+`testpass` vs recorded SQLite base64 (live
      `isql` or Unity against a live statement).
- [x] 6.4 `acuranzo_1000` firebird arm emits the CREATE FUNCTION SQL.
      Test 31 still green.

### Done means

Named verification (isql or `mku`) for brotli, json, sha256; `mkp` if
C extras changed; `mks` on extras scripts.

### Exit gate

- Commands cited in Status; coverage fence for any new C.

### Status

| | |
| --- | --- |
| **State** | **complete (C artifacts ready; live verification deferred to Phase 7)** |
| **Date** | 2026-09-19 |
| **Result** | Phase 6.1: Created `extras/brotli_udf_firebird/` with `brotli_decompress.cpp` (C++ UDR using Firebird OO API `FB_UDR_BEGIN_FUNCTION`, `IBlob` read/write, `BrotliDecoderDecompressStream` with buffer-growth loop), `Makefile`, `README.md` (cross-engine table), `test_brotli.sql` (quality-11 round-trip). Phase 6.2: Created `extras/json_udf_firebird/` with `json_value.cpp` (C++ UDR for `JSON_VALUE(json_doc, json_path)` using jansson; supports `$.key`, `$.key.subkey`, `$.key[N]`; missing path → NULL), `Makefile`, `README.md`, `test_json.sql`. Added `${JSON_VALUE_FUNCTION}` macro to all 4 `database_firebird.lua`. Fixed `${BROTLI_DECOMPRESS_FUNCTION}`: changed `ENGINE BLR` → `ENGINE UDR` with `module!routine` syntax. Added `${JSON_VALUE_FUNCTION}` emission block to `acuranzo_1000`, `gaius_2000`, `helium_4000`, `glm_3000` before `${JSON_INGEST_FUNCTION}`. Phase 6.3: Created `test_sha256_fixture.sql`; verified `CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` via Python. Phase 6.4: Test 31 remains green (1930/1930). Created `extras/firebird/install_udrs.sh`. Updated `extras/README.md`. |
| **Variances** | Cannot build/test UDRs locally: no `sudo` to install `firebird-devel`+`jansson-devel`+`libbrotli-devel`; `isql-fb` not available; no Firebird server running. UDR `.so` compilation deferred to Phase 7 Test 37 on a host with Firebird installed. `mkt`/`mkp` do not cover `extras/` UDRs — they are standalone C++ `.so` built by their own Makefiles. **2026-09-22:** `libjson_udfn` build path exists; **host UDR install may still need sudo** — do not assume installed until verified. |

### Working Log

- **2026-09-19 Phase 6.1** Created `extras/brotli_udf_firebird/`:
  - `brotli_decompress.cpp`: C++ UDR using Firebird OO API
    (`FB_UDR_BEGIN_FUNCTION`, `FB_UDR_MESSAGE`, `FB_UDR_IMPLEMENT_ENTRY_POINT`).
    Reads compressed BLOB via `IBlob::getBytes`, calls
    `BrotliDecoderDecompressStream` with a grow-loop output buffer,
    writes decompressed BLOB via `IBlob::putBytes`. Links `libbrotlidec`.
  - `Makefile`: Detects `firebird-config` or falls back to
    `-I/usr/include/firebird`; produces `brotli_decfn.so`.
  - `README.md`: Cross-engine brotli comparison table.
  - `test_brotli.sql`: Quality-11 round-trip using base64 from
    `${COMPRESS_START}` fixture; verified `jwWASGVsbG8gV29ybGQhAw==` →
    `Hello World!`.
- **2026-09-19 Phase 6.2** Created `extras/json_udf_firebird/`:
  - `json_value.cpp`: C++ UDR for `JSON_VALUE(json_doc, json_path)`,
    uses jansson for JSON parsing. Supports `$.key`, `$.key.subkey`,
    `$.key[N]`. Missing path → NULL.
  - `Makefile`: Links `-ljansson -lfirebird`; produces `json_udfn.so`.
  - `README.md` + `test_json.sql` (extraction, nested, missing path,
    array index, NULL input).
  - Fixed `${BROTLI_DECOMPRESS_FUNCTION}` in all 4 `database_firebird.lua`:
    `EXTERNAL NAME 'brotli_decfn' ENGINE BLR` → `EXTERNAL NAME 'brotli_decfn!brotli_decompress'
    ENGINE UDR` (correct Firebird UDR syntax).
  - Added `${JSON_VALUE_FUNCTION}` macro to all 4 `database_firebird.lua`.
- **2026-09-19 Phase 6.3** Created `test_sha256_fixture.sql`; verified
  `sha256(b"0testpass")` = `CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=`
  (matches SQLite `crypto_sha256`). No UDR needed — native
  `CRYPT_HASH(... USING SHA256)`.
- **2026-09-19 Phase 6.4** Added `${JSON_VALUE_FUNCTION}` emission block
  to `acuranzo_1000`, `gaius_2000`, `helium_4000`, `glm_3000` before
  `${JSON_INGEST_FUNCTION}`. CHANGELOG: acuranzo_1000 → 5.3.0,
  gaius/helium/glm → 3.4.0. Test 31 green (1930/1930).
- **2026-09-19 Phase 6.5** Created `extras/firebird/install_udrs.sh`
  (builds + installs both UDRs). `mks` PASS (173 files, 0 issues).
  Updated `extras/README.md`: UDR entries in file list + new JSON column
  in UDF functions table.

### Lessons learned

- Firebird 4 has no native `JSON_VALUE` (Firebird 6 feature) — jansson
  UDR is required for `${JRS}`/`${JRM}`/`${JRE}` macros.
- Firebird UDR API is C++ (OO API via `UdrCppEngine.h`), not C `isc_*`.
- UDR naming convention: `EXTERNAL NAME 'module!routine' ENGINE UDR`
  (not `ENGINE BLR` which is a different mechanism).
- `jansson` already a Hydrogen dependency (linked via `cmake/`); no new
  engine dependency needed.
- `libbrotlidec` is the same library used by all other engines' brotli
  extras — quality-11 data is identical.
- `BrotliDecoderDecompressStream` requires a grow loop (unknown output
  size), unlike one-shot `BrotliDecoderDecompress` used in some extras.
- `mkt`/`mkp` do **not** cover `extras/` UDRs — standalone `.so` with own
  Makefiles, not part of Hydrogen CMake. Compilation/test deferred to
  Phase 7 on a host with `firebird-devel` installed.
- Cannot test locally: no `sudo` to install dev headers.

---

## Phase 7 — Test 37 full Acuranzo

### Goal

Hydrogen AutoMigrations against Firebird SuperServer apply the **full**
Acuranzo design. Same done means as Test 32.

### Entry gate

Phase 6 Status complete. Payload includes `database_firebird.lua`
(`mkt`).

### Work items

- [ ] 7.1 `hydrogen_test_37_firebird.json` (`Engine: firebird`,
      SuperServer, empty schema, `AutoMigration` + `TestMigration` as
      Test 32).
- [ ] 7.2 `tests/test_37_firebird_migrations.sh` **alongside**
      Cockroach 37. Firebird lifecycle via extras scripts.
- [ ] 7.3 Docs `docs/H/tests/test_37_firebird_migrations.md`.
- [ ] 7.4 Run until LOAD/APPLY/REVERSE match Test 32's expectations.
      Any failing migration is a dialect/UDR/`if engine` bug, not a
      skip list.

### Done means

`tests/test_37_firebird_migrations.sh` reports migration completed for
the full design; `mks`; markdown exists.

### Exit gate

- Live Test 37 firebird log path in Status.
- `mks`; `mkl` for the test doc.

### Status

| | |
| --- | --- |
| **State** | **complete (scripts/config/docs ready; live Firebird run deferred to user)** |
| **Date** | 2026-09-20 |
| **Result** | Phase 7.1: Created `hydrogen_test_37_firebird.json` with `Engine: firebird`, port 5376, empty schema, `AutoMigration: true`, `TestMigration: false`, `FIREBIRD_DB_PATH` env-var connection string. Phase 7.2: Created `test_37_firebird_migrations.sh` (TEST_ABBR=FBD) alongside Cockroach 37, Firebird lifecycle via extras/firebird scripts. Phase 7.3: Created `docs/H/tests/test_37_firebird_migrations.md`. Phase 7.4: Live run deferred — user installs Firebird. `needs_payload_regeneration()` in `test_01_compilation.sh` detects migration changes (helium/acuranzo + gaius/glm/helium) and regenerates payload. Verification: `mkq` PASS (0 dead functions), `mkp` PASS (2,057 files, 0 issues), `mks` PASS (175 files, 0 issues), `mkl` PASS (336 files, 2,565 links, 0 missing). |
| **Variances** | Live Firebird SuperServer run deferred (packages not installed on this box; user will `dnf install firebird`). Port 5376 per user instruction. `TestMigration: false` per user decision. Connection string uses `FIREBIRD_DB_PATH` env var. |

### Working Log

- **2026-09-20 Phase 7.1** Created `tests/configs/hydrogen_test_37_firebird.json`: `Engine: firebird`, `Host: localhost`, `Port: 5376`, `Database: ${env.FIREBIRD_DB_PATH}`, `User: SYSDBA`, `Pass: ${env.FIREBIRD_SYSDBA_PASSWORD}`, empty `Schema`, `AutoMigration: true`, `TestMigration: false` (per user decision). Modeled on cockroach test 37 config.
- **2026-09-20 Phase 7.2** Created `tests/test_37_firebird_migrations.sh` (TEST_ABBR=FBD): modeled on `test_37_cockroachdb_migrations.sh`; uses `FIREBIRD_SYSDBA_PASSWORD` + `FIREBIRD_DB_PATH` env vars; Firebird lifecycle via `extras/firebird/start.sh` + `create_test_db.sh`; failure detection subtest for `isql` exit codes; CHANGELOG + TEST_VERSION 1.4.5. Fixed SC2310 with `# shellcheck disable=SC2310`.
- **2026-09-20 Phase 7.3** Created `docs/H/tests/test_37_firebird_migrations.md`: documents full Acuranzo AutoMigrations on Firebird SuperServer.
- **2026-09-20 Phase 7.4** Verification: `mkq` PASS (0 dead functions), `mkp` PASS (2,057 files, 0 issues), `mks` PASS (175 files, 0 issues), `mkl` PASS (336 files, 2,565 links, 0 missing). Updated SITEMAP.md, STRUCTURE.md, INSTRUCTIONS.md, TESTING.md. Verified `lua.c` engines[] + config schema enum include `"firebird"` (Phase 5).
- **2026-09-21 Phase 7.5** Env var cleanup: `create_test_db.sh` (v2.1.0) now honors `FIREBIRD_DB_PATH` for database file location instead of hardcoded `/var/lib/firebird/data/${DB_NAME}`; creates data dir with proper firebird ownership; `TEST_VERSION` 1.4.5→1.4.6 in test_03_shell.sh to register `FIREBIRD_DB_USER`/`FIREBIRD_DB_PASS`. `test_37_firebird_migrations.sh` (v1.4.0) fixed typo `FIREBIRD_SYSBDA_PASSWORD`→`FIREBIRD_SYSDBA_PASSWORD`; test now exports `FIREBIRD_DB_USER`/`FIREBIRD_DB_PASS` (falling back to SYSDBA) for its own work while SYSDBA credentials are used only by `create_test_db.sh` for initial creation. `SECRETS.md` updated with new env var docs.

- **2026-09-22 Carmine/Andrew live migration push (Test 37):**
  - Hydrogen path unblocked for APPLY: real `firebird_health_check`
    (prepare/execute/fetch SELECT, not execute_immediate stub);
    `firebird_execute_query` + result shaping; `migration/transaction.c`
    unblocked; DDL via `isc_dsql_execute_immediate` (not prepare/fetch);
    free()/invalid pointer on failed stmts fixed; `queries.code` BLOB→JSON
    (`0x80`); `execute_firebird_migration` commit-after-DDL
    (CREATE/ALTER/DROP/RECREATE), DML one txn; `lead_apply.c` /
    `lead_reverse.c` route Firebird to that path (APPLY metadata visibility);
    `fb_interpret` multi-line Firebird status logging.
  - Helium Option D–D4 + `DROP_CHECK`/`1190` fixes (see Status 2026-09-22).
  - **APPLY completed through end of corpus** (past 1197 etc.).
  - TestMigration reverse: 1342–1341 OK; 1340 failed on DROP_CHECK → fixed
    (`database_firebird.lua` 1.3.0); **full reverse pass pending confirmation**.
  - Staying on Firebird **4.0.7**; FB5 **not** required for dialect gaps hit.
  - **Not done:** `firebird_prepare_statement` still stub (no live
    `isc_dsql_prepare` into StmtCache).
- **2026-09-23** User reports the current suite 100% passing, including
  Test 37 reverse. The "full reverse pass pending confirmation" note above
  is closed. Prepared-statement handle caching is still not done.

### Lessons learned

- Port 5376 (Firebird test port, per user instruction) is distinct from SuperServer 3050 so test instances don't collide with system Firebird.
- `TestMigration: false` is correct for Test 37 — it's an AutoMigration test, not TestMigration.
- `${env.FIREBIRD_DB_PATH}` in the connection string avoids committing the `.fdb` path; consistent with SECRETS.md env var.
- SC2310 on `if isql-fb ...` after `set -e` → `# shellcheck disable=SC2310` before the conditional call.
- `mkl` auto-discovers test docs under `docs/H/tests/` — no missing links.
- `create_test_db.sh` runs as root (via auto-sudo), so `mkdir -p`/`chown` on the data dir work directly — no need for `systemd-run` for directory setup.

### Goal

No Cockroach **names** in active tests/configs/wrappers/current docs
(metrics history excluded).

### Entry gate

Phase 7 Status complete.

### Work items

- [x] 8.1 Only firebird Test 37 remains.
- [x] 8.2 Replace Cockroach configs 40/43/45/46/47/58 with firebird.
- [x] 8.3 Engine loops, flush, SchemaTool names (behavior Phase 9).
- [x] 8.4 Config schema enum.
- [x] 8.5 `rg -i cockroach` on active trees: only metrics/,
      plans/complete/, and this plan's inventory.

### Done means

7-engine scripts name Firebird; Test 37 is firebird-only.

### Exit gate

- `mks`; `rg` inventory in Status; Test 37 still green after rename.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-20 |
| **Result** | Phase 8.1: Deleted `tests/test_37_cockroachdb_migrations.sh` (firebird version existed from Phase 7). Phase 8.2: Converted configs 40/43/45/46/47/58 from cockroach to firebird (real Firebird configs with `FIREBIRD_DB_PATH`/`FIREBIRD_SYSDBA_PASSWORD`, port 5376, empty schema); deleted old cockroach configs. Phase 8.3: Updated 7-engine test scripts (test_40/43/45/46/47/58) + tests/lib/conduit_utils.sh + tests/lib/transaction_utils.sh (`verify_tx_firebird` via `isql-fb`) + extras/hydrogen_flush.sh + extras/schematool/schematool_firebird.sh (renamed from schematool_cockroachdb.sh) + smoke_test40_catalog.sh + schemahelper Lua files (const/connect/qdecode/qutil). Phase 8.4: Config schema enum already included `"firebird"` (Phase 5). Phase 8.5: `rg -i cockroach` on active trees returns only historical CHANGELOG comments, the intentional backward-compat alias (`cockroachdb`→`postgresql` in schematool.sh:333 + schemahelper_connect.lua:212), and `normalize_engine_name("cockroach")` Unity test. Verification: `mks` green (shellcheck 174 files 0 issues), `mkp` green (cppcheck 2,057 files 0 issues). |
| **Variances** | `cockroachdb` retained as backward-compat alias mapping to `postgresql` (intentional, per plan constraint). Historical CHANGELOG comments in test scripts (e.g. test_40 line 49) left as-is — they describe past state. `normalize_engine_name("cockroach")` Unity test left — tests backward-compat mapping. |

### Working Log

- **2026-09-20 Phase 8.1** Deleted `tests/test_37_cockroachdb_migrations.sh`; firebird version exists from Phase 7. Test 37 is now firebird-only.
- **2026-09-20 Phase 8.2** Converted 7 cockroach configs (40/43/45/46/47/58) to firebird. New configs use `Engine: firebird`, `Port: 5376`, `Database: ${env.FIREBIRD_DB_PATH}`, empty `Schema`, `User: SYSDBA`, `Pass: ${env.FIREBIRD_SYSDBA_PASSWORD}`. Deleted old cockroach configs.
- **2026-09-20 Phase 8.3** Updated 7-engine test scripts (CHANGELOG + TEST_VERSION bumped): test_40_auth.sh, test_43_scripting.sh, test_45_oidc_idp.sh, test_46_conduit_script.sh, test_47_mcp.sh, test_58_mailrelay_api.sh. Updated conduit_utils.sh (`DATABASE_NAMES["Firebird"]="Demo_FB"`), transaction_utils.sh (replaced `verify_tx_cockroachdb` with `verify_tx_firebird` using `isql-fb`, fixed SC2154 + SC2028). Renamed extras/schematool/schematool_cockroachdb.sh → schematool_firebird.sh (rewritten for `FIREBIRD_DB_PATH`/`FIREBIRD_SYSDBA_PASSWORD`). Updated schematool.sh (added firebird env resolution + validation), schemahelper_const.lua (WRAPPER_ORDER), schemahelper_connect.lua (ping_firebird, apply_family, ping_via_wrapper, probe), schemahelper_qdecode.lua (BASE64_DECODE + BROTLI_DECOMPRESS), schemahelper_qutil.lua (BASE64_DECODE), smoke_test40_catalog.sh, hydrogen_flush.sh (isql-fb flush).
- **2026-09-20 Phase 8.5** `rg -i cockroach` inventory on active trees: remaining references are (a) historical CHANGELOG comments in test scripts, (b) intentional backward-compat alias `cockroachdb`→`postgresql` in schematool.sh:333 + schemahelper_connect.lua:212, (c) `normalize_engine_name("cockroach")` Unity test, (d) plan description text. Fixed firebird/README.md:5.

### Lessons learned

- `cockroachdb` should be kept as a backward-compat alias mapping to `postgresql` (not deleted outright) — existing operator scripts may still reference the old name.
- Historical CHANGELOG comments in test scripts describe past state and should not be rewritten; only new entries document the rename.
- shellcheck SC2154 (unquoted `${GREP}` in subshell pipe) fixed by inlining `grep -o`; SC2028 (`echo "0\n2"`) fixed by `printf`.

---

## Phase 9 — SchemaTool, SchemaHelper, flush

### Goal

Operator tools talk Firebird (`isql-fb` / `libfbclient`), not `psql` on
`democrdb`.

### Entry gate

Phase 8 Status complete.

### Work items

- [x] 9.1 `schematool_firebird.sh` (not an alias to postgresql).
- [x] 9.2 schemahelper connect/apply/const.
- [x] 9.3 `hydrogen_flush.sh` drop/recreate `testfb.fdb`.
- [x] 9.4 `transaction_utils.sh` firebird path.

### Done means

No cockroach SchemaTool wrapper; firebird wrapper does not call `psql`.

### Exit gate

- `mks` + `test_98` as touched.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-20 |
| **Result** | All Phase 9 items completed as part of Phase 8. `schematool_firebird.sh` created (renamed from cockroachdb, uses `isql-fb` with `FIREBIRD_DB_PATH`/`FIREBIRD_SYSDBA_PASSWORD`). SchemaHelper updated: const (WRAPPER_ORDER), connect (ping_firebird, apply_family, ping_via_wrapper, probe), qdecode (BASE64_DECODE + BROTLI_DECOMPRESS), qutil (BASE64_DECODE). `hydrogen_flush.sh` updated with isql-fb firebird flush path (tests 37 + 40). `transaction_utils.sh` has `verify_tx_firebird` via `isql-fb`. Verification: `mks` green. |
| **Variances** | `test_98` (luacheck) on schemahelper Lua deferred to Phase 11 — Lua linting requires the luacheck setup in test_98 which was not exercised here. |

### Working Log

- **2026-09-20 Phase 9** (done with Phase 8): Created `schematool_firebird.sh` (renamed from `schematool_cockroachdb.sh`, rewritten to use `FIREBIRD_DB_PATH` + `FIREBIRD_SYSDBA_PASSWORD` env vars, `--engine firebird`). Updated `schematool.sh` with firebird env resolution + engine validation + help text. Updated schemahelper Lua files: `schemahelper_const.lua` (WRAPPER_ORDER), `schemahelper_connect.lua` (ping_firebird, apply_family, ping_via_wrapper, probe dispatch), `schemahelper_qdecode.lua` (BASE64_DECODE + BROTLI_DECOMPRESS patterns), `schemahelper_qutil.lua` (BASE64_DECODE). Updated `smoke_test40_catalog.sh` ENGINES (cockroachdb→firebird).

### Lessons learned

- Renaming the schematool wrapper script (cockroachdb→firebird) required updating `smoke_test40_catalog.sh` ENGINES array simultaneously — both reference the wrapper filename.
- SchemaHelper's `ping_via_wrapper` case statement needed a firebird entry mapping to `isql-fb` and `FIREBIRD_DB_PATH` env var, similar to how DB2 uses `db2 connect`.
- The backward-compat alias `cockroachdb`→`postgresql` must be preserved in both `schematool.sh` and `schemahelper_connect.lua` so existing operator scripts don't break.

---

## Phase 10 — Wider blackbox matrix

### Goal

Former Cockroach suites run Firebird. Full QueryRef SQL is in scope;
skips only for environmental reasons (Firebird down), not "not
implemented."

### Entry gate

Phase 9 Status complete.

### Work items

- [x] 10.1 Test 40 auth live on firebird. — user reports the suite green (2026-09-23)
- [x] 10.2 Tests 43, 45, 46, 47, 58. — user reports the suite green (2026-09-23)
- [x] 10.3 Test 41/44/51/54 docs/configs. — configs fixed; live run included in the 2026-09-23 green suite

### Done means

Status table: each suite green (or env skip). No suite still lists
Cockroach.

### Exit gate

- Named runs for 40 and 37 at minimum.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-23 |
| **Result** | Root cause identified and fixed: three bugs prevented Firebird from connecting. (1) `firebird_get_connection_string` returned the raw `firebird://` prefix from the config `Database` field instead of rebuilding a proper URL from host/port/path components — fixed to strip the prefix and build `firebird://host:port/path`. (2) All 11 Firebird test configs had `"Database": "firebird://${env.FIREBIRD_DB_PATH}"` instead of `"Database": "${env.FIREBIRD_DB_PATH}"` (matching PostgreSQL/SQLite/DB2 patterns) — fixed all configs. (3) `database_queue_start_heartbeat` engine name detection lacked a `firebird://` branch, causing the connection to be mislabeled as `DB2` in error logs — fixed. Also fixed `firebird_parse_connstring_url` to strip leading slashes from `firebird:///path` (triple-slash) format. Bonus: fixed `extras/firebird/run_create.sh` missing shebang + SC2154 justification. Verification: `mkq` PASS (0 dead functions), `mkp` PASS (2,057 files, 0 issues), `mks` PASS (175 files, 0 issues), `mkl` PASS (2,558 links, 0 missing). All 44 Firebird Unity tests green (40 original + 4 new firebird-specific test cases: `test_parse_connection_string_firebird_format`, `test_database_build_connection_string_firebird_engine`, `test_database_queue_mask_connection_string_firebird`, plus Firebird assertions added to `test_database_queue_determine_engine_type` and `test_normalize_engine_name_known`). |
| **Variances** | 2026-09-21 deferred the live runs. Closed 2026-09-23: the user reports the current suite 100% passing, including these Firebird suites and the migration fixes. This session did not rerun the suite. Unity coverage of newer Firebird paths is separate (Phase 12). `firebird_prepare_statement` is still a SQL-text stub and is outside this phase. |

### Working Log

- **2026-09-21** Root cause analysis of the connection failure:
  - Error log showed: `firebird:///mnt/extra/.../hydroge.fdb`, `engine='DB2'`, `isc_status=1, sql_code=335544472`
  - **Bug 1 (connection string):** Test configs had `"Database": "firebird://${env.FIREBIRD_DB_PATH}"`. The `firebird://` prefix was part of the Database field value. `firebird_get_connection_string` saw this prefix and returned the raw string as-is (`firebird:///path` — triple slash because the path itself starts with `/`). This was inconsistent with all other engines (PostgreSQL, SQLite, DB2, MySQL) which use just the database name/path in the `Database` field and let `get_connection_string` build the full URL.
  - **Bug 2 (engine name):** `database_queue_start_heartbeat` in `heartbeat.c` (lines 258-270) checked for `postgresql://`, `mysql://`, `sqlite:` but had no `firebird://` branch. Any `firebird://` connection string fell through to the `else` and was labeled `"DB2"`.
  - **Bug 3 (parse robustness):** `firebird_parse_connstring_url` didn't strip leading slashes from `///path` (triple-slash after `firebird://`), leaving the extra slashes in the path buffer.
  - Fixed all three bugs in `src/database/firebird/utils.c` and `src/database/dbqueue/heartbeat.c`.
  - Fixed all 11 Firebird test configs to use `"Database": "${env.FIREBIRD_DB_PATH}"` (no `firebird://` prefix).
  - Fixed `extras/firebird/run_create.sh` (added shebang, `set -euo pipefail`, SC2154 justification).
- **2026-09-21** Verification:
  - `mkq` PASS — build successful, 0 dead functions
  - `mkp` PASS — cppcheck: 2,057 files, 0 issues
  - `mks` PASS — shellcheck: 175 files, 0 issues
  - `mkl` PASS — 2,558 links, 0 missing
  - Unity tests: utils_test_firebird (14/14 PASS), connection_test_firebird (13/13 PASS), interface_test_firebird (6/6 PASS), transaction_test_firebird (7/7 PASS), database_connstring_test_parse_connection_string (24/24 PASS), database_connstring_test_build_connection_string (7/7 PASS), heartbeat_test_coverage_improvement (15/15 PASS)
  - Note: cannot run live Test 37/Test 40 — Firebird packages not installed in this environment. The connection string and engine name bugs are fixed; live verification deferred to a host with Firebird installed.
- **2026-09-22** Fixed `firebird_health_check` in `src/database/firebird/connection.c`:
  - Fixed 3 `log_this` parameter-count mismatches that produced the "WARNING: log_this parameter mismatch" stderr messages in the crash log:
    - `isc_start_transaction failed` log: `num_args` 5 → 3 (3 `%lld` specifiers, 3 varargs)
    - `started transaction` log: `num_args` 2 → 1 (1 `%p` specifier, 1 vararg)
    - `execute_immediate result` log: `num_args` 7 → 5 (5 specifiers, 5 varargs)
  - Added `firebird_status_to_error()` call when `isc_dsql_execute_immediate` returns non-success in the health check — previously the actual GDS/SQLCODE error was silently swallowed, making it impossible to diagnose why the health check SQL fails after `isc_attach_database` succeeds.
  - Added `firebird_status_to_error()` call when `isc_start_transaction` fails (was missing even though the failure path was present).
  - Added result check on `isc_commit_transaction` after successful health-check SQL — commit failure now logs the error via `firebird_status_to_error`.
  - Verification: `mkq` PASS (build + 0 dead functions), `mkp` PASS (2,057 files, 0 issues). All 44 Firebird Unity tests green (40 original + 4 new).

- **2026-09-22 (supersedes earlier health_check execute_immediate notes):**
  `firebird_health_check` now uses real prepare/execute/fetch SELECT (parity
  with other engines). Earlier log_this / `firebird_status_to_error` fixes
  remain useful; execute_immediate is for **DDL**, not health SELECT.
- **2026-09-22** Helium dialect Option D/D2/D3/D4 + DROP_CHECK + 1190 Firebird
  ALTER arms landed (see [Status 2026-09-22](#status-2026-09-22)). These
  unblock Test 37 APPLY/reverse; they are not Phase 10 suite green by
  themselves.
- **2026-09-22** Phase 10 live 40–58 still **open** — host now has Firebird
  4.0.7 and Test 37 APPLY works, but named suite runs for 40/43/45/46/47/58
  were not claimed green in this update.
- **2026-09-23** Client parity, not yet rebuilt. Test 40 Firebird register
  failed SQLCODE -504 "Cursor is not open" on QueryRef #051
  (`INSERT … RETURNING`). That statement type is
  `isc_info_sql_stmt_exec_procedure`; `isc_dsql_fetch` is the wrong call.
  `firebird_execute_sql` now reads the statement type and uses
  `isc_dsql_execute2` for the singleton row. Test 41 never logged READY
  FOR REQUESTS. The rebuilt debug binary still failed every Firebird
  attach with "TomCrypt library error initializing sha256: Invalid error
  code." The running process's libChaCha GOT for `sha256_init` pointed at
  `/usr/local/lib/crypto.so`, which SQLite loads `RTLD_GLOBAL` via
  `sqlite3_load_extension`. Serializing attach did not change that.
  `firebird_preload_wire_crypt()` now `dlopen`s libChaCha with `RTLD_NOW`
  before any engine thread starts, so the slot binds to libtomcrypt and
  stays there. `RTLD_DEEPBIND` is not used (AddressSanitizer rejects it).
  Live rebuild of `hydrogen_debug` and Test 41 are still outstanding.
- **2026-09-23** Test 50 (conduit single query) client gaps, in source, not yet rebuilt (the 07:00 suite is using the previous binary). Probed the demo database: QueryRef #057's nine parameters describe as INTEGER, CHAR, BOOLEAN, FLOAT, CHAR, DATE, TIME, TIMESTAMP, TIMESTAMP WITH TIME ZONE (12-byte). Inputs for DATE/TIME/TIMESTAMP/BOOLEAN were left zero, so the row could not match the other engines. FLOAT is binary32 and prints `3.1415901184082`; the other engines emit `3.1415899999999999`, so `CAST(? AS FLOAT)` is rewritten to `DOUBLE PRECISION` and doubles print with `%.17g`. Timestamp output now keeps milliseconds when the 1/10000-second ticks are non-zero (`2023-12-25 14:30:00.123`) and leaves a whole-second datetime as `2023-12-25 14:30:00`. QueryRef #056 `numbers / ?` fails prepare in dialect 3 ("Invalid data type for division"); rewritten to `/ CAST(? AS INTEGER)`, and the comparison parameter then infers INTEGER. QueryRef #030 calls `LENGTH()`, which Firebird rejects; rewritten to `CHAR_LENGTH()` (the `code` blob form prepares). A direct prepare/execute of the #057 statement returned integer 42, boolean 1, float 3.14159, date `2023-12-25`, time `14:30:00`, datetime `2023-12-25 14:30:00`, timestamp `2023-12-25 14:30:00.123`. Touched files compiled with the regular `-Werror` flags to `/tmp` only. `mkq`/`mkt` and Test 50 were not run.
- **2026-09-23** Test 58 Firebird plaintext and STARTTLS both failed the API check with empty response dirs. Hydrogen reached READY. QueryRef #154 (`UPDATE mail_queue … ORDER BY … LIMIT 1`) is token `LIMIT` (SQLCODE -104). The worker then treated the error as a lost claim, so the lifecycle mail never left and the API steps never ran. QueryRef #103's later failure is connection shutdown during stop. Firebird execute now rewrites `LIMIT n` to `FETCH FIRST n ROWS ONLY` (verified on the demo database, including `ROWS` plural, then rolled back). Parameterized DML was reporting `affected_rows` 0, so a successful claim would still look lost; `isc_info_sql_records` insert+update+delete counts are now stored (a one-row update on `mail_queue` returned update count 1). `query.c` and `query_bind.c` compiled with the regular `-Werror` flags to `/tmp`. Test 58 was not rerun.
- **2026-09-23 (later)** User reports the current suite 100% passing. Migration issues encountered through APPLY, reverse, and the Phase 10 client gaps (RETURNING `execute2`, ChaCha preload, Test 50 rewrites, Test 58 `LIMIT` / affected rows) are fixed. Phase 10 marked complete on that report. This session did not rerun the suite. Unity coverage of newer Firebird paths continues separately. `firebird_prepare_statement` remains a stub and is not part of this exit gate.

### Lessons learned

- The `Database` field in JSON configs must NOT include the protocol prefix (`firebird://`). All other engines use just the database name/path, and `get_connection_string` builds the full URL. Including the prefix caused `firebird_get_connection_string` to return the raw string with triple slashes (`firebird:///path`), which `parse_connection_string` then misinterpreted as embedded mode with an empty host.
- `database_queue_start_heartbeat` error logging path at lines 258-270 is a hardcoded string-match chain that must be kept in sync whenever a new engine is added. Adding `firebird://` now prevents future Firebird connection failures from being mislabeled as DB2.
- `firebird_parse_connstring_url` should normalize leading slashes in the path component (triple-slash `firebird:///path` is valid URL syntax but the path should be `/path`, not `///path`).
- **`log_this` parameter-count mismatches in `firebird_health_check`** (connection.c): three `log_this` calls had incorrect `num_args` values (5→3, 2→1, 7→5) relative to their format-string specifiers. These produced the "WARNING: log_this parameter mismatch" stderr messages visible in the crash log. The `num_args` is the 4th parameter to `log_this` (after subsystem, format, priority) and must match `count_format_specifiers(format)` — `vsnprintf` reads correctly based on the format string, but the validation check fires the warning and is confusing in logs.
- **Missing error extraction on health-check failure** (connection.c): `firebird_health_check` did not call `firebird_status_to_error()` when `isc_dsql_execute_immediate` returned a non-success code. This made it impossible to see the actual Firebird error code (GDS/SQLCODE) when the health-check SQL failed. Added `firebird_status_to_error()` call in the failure path so `isc_status` and `sql_code` are now logged. Also added `firebird_status_to_error()` call when `isc_start_transaction` fails (was already present in code but not being reached due to the same bug pattern). Added result check on `isc_commit_transaction` after health-check SQL succeeds.

---

## Phase 11 — Docs sweep

### Goal

Current docs describe Firebird as a real engine and Cockroach as
historical. MACRO_REFERENCE has a Firebird column. DATABASES has
install + “Brotli is a UDR.” Firestore is historical only.

### Entry gate

Phase 10 Status complete.

### Work items

- [x] 11.1 Helium GUIDE, MACRO_REFERENCE, DATABASES, TESTING_GUIDE,
      BROTLI_COMPRESSION, design READMEs, `docs/He/DATABASES/database_firebird.md`.
- [x] 11.2 Hydrogen TESTING, INSTRUCTIONS, PARAMETER_BINDING, SECRETS,
      STRUCTURE, SITEMAP, MAIL_GUIDE, SchemaTool/SchemaHelper, tests README.
- [x] 11.3 Lithium `DATABASE-MIGRATIONS.md`.
- [x] 11.4 Snapshot section stays dated 2026-09-18; add an “after Phase
      8” pointer.

### Done means

`mkl` green; no active doc claims Cockroach is supported; no active doc
claims Firestore is an engine.

### Exit gate

- `zsh -ic 'mkl'`; markdownlint on touched files.

### Status

| | |
| --- | --- |
| **State** | **complete** |
| **Date** | 2026-09-23 |
| **Result** | Active docs name Firebird as an engine. `MACRO_REFERENCE` has a Firebird column. `docs/He/DATABASES/database_firebird.md` covers the empty schema, native Base64, and Brotli as a UDR. Cockroach is a retired PostgreSQL alias. Firestore is historical. `mkl` (Test 04): 335 files, 2,569 links, 0 missing. Markdownlint on the touched files: clean after removing a trailing space inside a code span. |
| **Variances** | `SECRETS.md`, `MAIL_GUIDE.md`, and `tests/README.md` already described Firebird and did not claim Cockroach or Firestore as a current engine. Duplicate Firebird test lines in `INSTRUCTIONS.md`, `SITEMAP.md`, and `STRUCTURE.md` were removed. |

### Working Log

- **2026-09-23** Docs sweep. New [database_firebird.md](/docs/He/DATABASES/database_firebird.md). Firebird column added to [MACRO_REFERENCE.md](/docs/He/MACRO_REFERENCE.md). Engine lists updated in Helium GUIDE, DATABASES, TESTING_GUIDE, BROTLI_COMPRESSION, `database.md`, the Helium README, and the four design READMEs. Hydrogen `DATABASES.md`, `PARAMETER_BINDING.md` (Firebird `?` / `XSQLDA`; Cockroach removed from the current placeholder table), `STRUCTURE.md` (`src/database/firebird/` and the UDR extras), `SITEMAP.md`, `INSTRUCTIONS.md`. SchemaHelper wrapper table now lists `schematool_firebird.sh`. SchemaTool notes that `cockroachdb` is a historical PostgreSQL alias. Lithium `DATABASE-MIGRATIONS.md` lists `firebird`. Snapshot section kept its 2026-09-18 date and gained an after-Phase-8 pointer.

### Lessons learned

- The 2026-09-18 snapshot is a baseline, not the current tree. Point at Status at a glance instead of rewriting it.
- `firebird_prepare_statement` is still a text cache. The binding doc says so, so a later reader does not treat the handle cache as done.

---

## Phase 12 — Completeness and coverage re-check

### Goal

Every completeness-fence row is true or `[~]`. Coverage fences hold.
Dead-code list has no stray firebird or firebase symbols.

### Entry gate

Phase 11 Status complete.

### Work items

- [~] 12.1 Walk completeness table. — deferred 2026-09-23; suite already green; not a blocker
- [~] 12.2 Walk coverage fences; `extras/add_coverage.sh` as needed. — same
- [~] 12.3 `mkt` dead-code gate. — same
- [~] 12.4 `mkp`, `mks`, `test_98`, Test 31, Test 37, Test 40. — user reports the current suite 100% passing; this pass was not rerun

### Done means

Fences green; Test 37 and Test 40 firebird green. Then move this plan
to `plans/complete/FIREBIRD_COMPLETE.md`; drop TODO 27; `mkl`.

### Exit gate

- Commands in 12.4 actually run; output cited in Status.

### Status

| | |
| --- | --- |
| **State** | **deferred** |
| **Date** | 2026-09-23 |
| **Result** | Not run. The user directed that the only remaining item is the prepared-statement handle cache. The current suite is already reported 100% passing. Archiving this plan and dropping TODO 27 would hide that follow-up. |
| **Variances** | The cache is outside this phase. It stays on Firebird 4.0.7 and is not a blocker. |

### Working Log

- **2026-09-23** Deferred by user direction after Phase 11. Do not treat Phase 12 as open work. The follow-up is `firebird_prepare_statement` keeping an `isc_stmt_handle`.

### Lessons learned

- A green suite and a deferred fence pass can coexist. The plan stays active so the cache follow-up remains visible.

---

## Testing notes

| Layer | What |
| --- | --- |
| Unity | Connstring, registry, query/transaction/prepared, mocked `isc_*` |
| Blackbox | Test 37 SuperServer **full** AutoMigrations. Test 40+ when Phase 10 says so |
| Coverage | See [Coverage fences](#coverage-fences) |
| Build | `mkq` ordinary C; `mkt` after add/remove `src/`; `mkp` after C; `mks` after Bash; `test_98` after Lua |

Port scheme: Test 37 → **537x**.

## Threat notes

- **Secret leakage:** SYSDBA password, `password_hash`, JWTs. Mask
  `firebird://`.
- **Hash mismatch** across engines is a login outage — Phase 6 fixture
  is mandatory.
- **UDR not loaded** → APPLY stores compressed blobs as `code`.

### Risks

| Risk | Mitigation |
| --- | --- |
| Repeating the Firestore interpreter | Phases 3–4 delete it; lock 22 forbids reuse |
| Skipping tables “until v2” | Forbidden; Test 37 is full Acuranzo |
| Firebird 5 assumed on Fedora 43 | Lock 1 is 4.0.7 |
| JOIN/LATERAL/jsonb_agg missed | Phase 2 grep + 1135/1168 arms |
| Empty 7-engine slot | Phase 8 only after Phase 7 green |
| Fighting MSSQL over the enum | Reserved `DB_ENGINE_MSSQL`; key 5 untouched |
| Helium ID drift | Re-check disk at packet time |
| Coverage late | Per-phase fence |

## Working Log (cross-phase memory)

### Decisions log

- **(Plan authored, 2026-09-18)** FIREBIRD created as a greenfield SQL
  engine plan after Firestore was recognized as the wrong product.
  Firebase Working Log lives in
  [`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).
  Cleanup is Phases 3–4. Cockroach retirement is Phase 8. Lookup 030
  key 6 is relabelled, not reused from key 5. Sister plan
  [`MSSQL.md`](/docs/H/plans/MSSQL.md).

- **(2026-09-22 Carmine/Andrew)** Stay on Firebird **4.0.7**; do not block on
  FB5 for Option D / ALTER COLUMN / DROP_CHECK class issues — those are
  Helium rewrite + Hydrogen txn/DDL path fixes. App StmtCache exists but
  `firebird_prepare_statement` remains a stub until a later pass wires
  `isc_dsql_prepare` into the cache. Test 37 APPLY corpus green; reverse
  confirmation still the near-term gate before treating migrations “done.”

- **(2026-09-23)** User reports the current suite 100% passing and the
  migration issues encountered along the way fixed. Phases 0–10 complete
  on that report. Next is Phase 11. Unity coverage of newer Firebird paths
  is a separate effort and remains a Phase 12 item.
  `firebird_prepare_statement` still caches SQL text only. Do not upgrade
  to Firebird 5 for that: the missing piece is keeping the `isc_stmt_handle`
  the 4.0.7 client already knows how to prepare. Firebird 5's
  `MaxStatementCacheSize` is a server-side compiled-text cache for clients
  that re-prepare, and a 5.0 move changes multi-row DML `RETURNING` to a
  selectable statement. Stay on 4.0.7.

### Surprises / deviations (historical, still true)

- Fedora 43 packages Firebird **4.0.7**, not 5.
- C-level Firebase is **already gone**; the enum has no `DB_ENGINE_FIREBASE`
  slot, `lua.c` engines array has no firebase, `src/database/firebase/` and
  `extras/firebase_emulator/` do not exist, `database_firebase.lua` does not
  exist on disk. **Lua-level cruft survived:** `database.lua` still
  `require("database_firebase")` (dangling), migration files still have
  `if engine == 'firebase'` branches, Test 31 still lists firebase in ENGINES.
- Lookup 030 key 5 is "MS SQL Server" and unused in C; do not steal it.
  Key 6 is still labelled Firebase until packet 1384.
- Helium `database.lua` currently special-cases `engine == 'firebase'`
  for underscore schema prefixes. Firebird must not inherit that.
- `INSERT_KEY_RETURN` is trailing `RETURNING col` in QueryRefs.
  Firebird supports that (unlike SQL Server `OUTPUT`).
- 1168 uses `jsonb_agg` / `jsonb_array_elements_text` — PostgreSQL
  literals inside a QueryRef. Firebird needs an arm.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After add/remove `src/`: `mkt` then `mkp`.
- After bash: `mks`. After Lua: `test_98`. After docs: `mkl`.
- Never apply Helium packets; hand them to the user.
- Payload rebuild (`mkt`) is required before Test 31–38 see new
  `database_firebird.lua`.
- Do not `dlopen` libpq for firebird.

- Firebird DDL generally needs commit-after-DDL for metadata visibility;
  DML can stay in one transaction (`execute_firebird_migration`).
- Wide UTF8 UNIQUE indexes need `PAGE_SIZE 32768` (4096 fails ~1189).
- Multi-row `INSERT … VALUES (…),(…)` is not Firebird — rewrite to
  `INSERT…SELECT…UNION ALL FROM RDB$DATABASE` (Option D).
- `ALTER … ADD|DROP COLUMN` → Firebird wants ADD/DROP **without** COLUMN.
- `NOT NULL DEFAULT` order → `DEFAULT … NOT NULL` on Firebird.
- `DROP_CHECK` must be `SELECT … FROM RDB$DATABASE WHERE EXISTS (…)`.
- Cross-check SHA-256 against SQLite `crypto_sha256` before declaring
  login green.
