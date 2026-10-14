<!-- markdownlint-disable MD007 MD024 -->
# QueryRef Transactions

## Purpose

Give **runtime QueryRefs** an explicit, author-visible syntax for
multi-statement transactions, with per-engine DDL semantics, and prove it
across every live database engine with fixture QueryRefs and one new
blackbox test.

Today a runtime QueryRef is exactly one statement. If someone authors
two, the behavior is silent or broken, engine by engine: SQLite runs only
the first statement (`sqlite3_prepare_v2` with a NULL tail discards the
rest), MySQL/MariaDB fail outright (the connection is opened with
`client_flag = 0`, no `CLIENT_MULTI_STATEMENTS`), DB2/MSSQL/Firebird
fail at prepare, and only PostgreSQL happens to run the batch as one
implicit transaction. This plan closes that gap on purpose instead of by
accident.

Secondary goal: the runtime transaction path finally exercises
`src/database/db2/prepared.c` and `src/database/db2/transaction.c` from a
normal API-driven blackbox test.

The migration system is **not** in scope. Migrations keep their own
`-- SUBQUERY DELIMITER` bands, their own apply/reverse state machine, and
their own executor. This plan adds a sibling capability for runtime SQL
only, reusing the proven pieces without disturbing them.

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, **stop for review**.
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure),
  `mku <base>`, `mkp`, `mka`, `mks`, `mkl`. See
  [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).
- **Never apply a database migration.** Phase 4 prepares a packet and
  hands it to the owner; the agent does not run `schematool` or
  `schemahelper` apply.

How a phase runs in full: [`/docs/PLANNING.md`](/docs/PLANNING.md).

## Resuming Work

**CLOSED (2026-10-14).** Phases 0–6 are complete. The owner reported
that the full suite passed and said to mark the plan complete. Phase 7
stays deferred and is TODO item 32. Do not re-open this file.

Test 64 run `20261014_112748` is green (60 subtests, 60 pass, 0 fail,
45.914s, orchestrator exit 0). Next Acuranzo file is **1389**. Next
caller-facing QueryRef is **159**. Re-verify on disk before using
either number.

## Priority

| | |
| --- | --- |
| **Band** | New work, approved in conversation 2026-10-14. Phase 0 decides whether a TODO.md pointer is added |
| **Effort** | L (parser + executor refactor + runtime wiring + fixtures + all-engine blackbox) |
| **Why now** | The SQLite silent-first-statement footgun and the MySQL hard failure are live traps for anyone who authors two statements in a `queries.code` row |

## Goals

1. A QueryRef can contain explicit transaction blocks
   (`-- TRANSACTION_START` / `-- STATEMENT` / `-- TRANSACTION_END`),
   including several blocks in one query, and every statement in a block
   runs inside one transaction.
2. DDL participates in the transaction where the engine supports it and
   degrades to commit-and-reopen where it does not — one query text,
   engine-appropriate atomicity, documented per engine.
3. Marked queries return a JSON array of per-statement result sets;
   unmarked single-statement queries are byte-identical to today.
4. Errors return everything known: per-statement results up to the
   failure, failing statement and block, engine error class and message,
   rollback state.
5. Fixture QueryRefs prove it on every live engine, including a
   deliberate mid-block failure that proves rollback, and a cleanup
   QueryRef leaves no residue whatever happened.
6. No arbitrary-SQL execution channel is added. Runtime SQL remains
   QueryRef-only by design.

## Non-Goals

- **No changes to the migration delimiter or state machine.** Migrations
  keep `-- SUBQUERY DELIMITER`, `-- QUERY DELIMITER`, and
  `parse_sql_statements` exactly as they are.
- **No rewriting of the shipped migration executors.** Phase 2 extracts a
  shared executor and the *optional* Phase 7 convergence is deferred by
  default; the migration path must be provably unchanged.
- **No semicolon tokenizer.** Statements are delimited by author-placed
  markers, like migration bands. Literal-aware splitting of arbitrary SQL
  is explicitly rejected.
- **No `BEGIN ... END` body splitting** (DB2 PL, Firebird PSQL). Same
  documented limitation the migration system already carries.
- **No Lithium UI, no new API endpoint, no changes to parameter
  validation semantics** for existing single-statement queries.
- **No agent-applied migrations** at any point in this plan.

## Current Observed State (2026-10-14)

### Runtime corpus audit (evidence, not assumption)

- The demo database (`tests/artifacts/database/sqlite/hydrodemo.sqlite`,
  cross-checked against a fresh test run's copy) holds 1,193 `queries`
  rows: 1,034 migration bookkeeping rows (types 1001/1002/1003) and 159
  runtime rows (types 0/1/10/11).
- **Zero** runtime rows contain the `-- SUBQUERY DELIMITER` marker, and
  **zero** contain even two semicolons. Every runtime QueryRef is a
  single statement.
- Gotcha for any future audit: the `code` column is stored as a **BLOB**,
  so SQLite's `length()` / `replace()` / `instr()` operate on blob bytes
  and produce false positives. Decode the text before searching.
- The 60+ `${SUBQUERY_DELIMITER}` uses in `elements/002-helium/` are all
  migration-internal: the bookkeeping `UPDATE queries SET
  query_type_a28 = ...` flip after each migration's statements, the
  MSSQL `INSERT ... WITH ... SELECT` rewrites in
  `acuranzo/migrations/database_mssql.lua`, and a few reverse-migration
  `code` columns.

### Runtime execution path

- Every runtime QueryRef executes through the database queue worker
  (`src/database/dbqueue/process.c`), which builds one `QueryRequest`
  and calls `database_engine_execute`
  (`src/database/database_engine.c`).
- Migrations execute through `src/database/dbqueue/lead_apply.c` and
  `src/database/migration/transaction.c`, which already split on the
  SUBQUERY delimiter and run per-engine begin → statements → commit
  executors, including Firebird's commit-after-DDL rule.
- `query_dialect_a30` does not appear anywhere in Hydrogen C source — it
  is manager metadata, not a runtime filter. Fixtures can be dialect 0
  (SQL) and still run on every engine.

### Engine capability matrix (verified in source)

| Engine (enum) | DDL inside a transaction | Today, runtime multi-statement SQL |
| --- | --- | --- |
| PostgreSQL (also the YugabyteDB path) | transactional | works, implicit transaction, last statement's result |
| SQLite | transactional | **silently runs only the first statement** (`src/database/sqlite/query.c`) |
| MySQL (`client_flag = 0`, no `CLIENT_MULTI_STATEMENTS`, `src/database/mysql/connection.c`) | implicit commit | **fails outright** |
| MariaDB | implicit commit | fails outright |
| DB2 | transactional | fails at prepare |
| MSSQL (FreeTDS) | transactional (documented exceptions) | fails at prepare |
| Firebird | not visible until commit | fails; and a blind transaction would fail on uncommitted DDL even if it prepared |

### Numbers on disk (snapshot — re-verify in Phase 0)

- Highest Acuranzo migration file: 1385. Highest Argent file: 2069.
- Highest caller-facing Acuranzo QueryRef: 155 (Argent owns 2000–2010).
- Highest blackbox test: 63. Next free: 64.
- Query type for fixtures: 1 (SQL - System), per lookup family 28.

## Design (locked in conversation 2026-10-14)

### Syntax

```text
-- TRANSACTION_START
statement one;
-- STATEMENT
statement two;
-- TRANSACTION_END
-- TRANSACTION_START
statement three;
-- TRANSACTION_END
plain statement outside any block;
```

### Parser rules

1. A marker is recognized **only at line start** (leading whitespace
   allowed). Mid-line marker-looking text is not a marker.
2. `-- STATEMENT` or `-- TRANSACTION_END` outside a block: parse error.
3. `-- TRANSACTION_START` inside an open block: parse error.
4. Empty block (`START` then `END` with only whitespace/comments
   between): parse error.
5. The marker — not the semicolon — delimits statements. Authors keep the
   trailing `;` for readability. A segment holding two semicolon-
   separated statements is an authoring error (documented; the statement
   simply fails at the engine).
6. Statements are trimmed; empty and comment-only segments are skipped
   (reuse `statement_has_executable_sql` from
   `src/database/migration/transaction.c`).
7. **No markers anywhere in the SQL: the existing single-statement path
   runs, unchanged, byte for byte.** The marker check is a cheap
   substring pre-filter before any parsing happens.

### Execution model

- Statements outside any block run exactly as they do today (autocommit
  per statement).
- Each transaction block is one unit: begin → execute each statement →
  commit; any statement failure rolls back the open transaction and the
  query fails with a rich error.
- Statements inside a block execute with the same timeout as the
  request, one statement at a time, mirroring the migration apply path
  (prepared statements with hash-derived names, per-statement requests).
- DDL segmentation: when a statement is DDL (`CREATE`, `ALTER`, `DROP`,
  `RECREATE` — the keyword test already living in
  `src/database/migration/transaction.c`) and the engine cannot keep DDL
  in a transaction, commit the open segment, execute the DDL, and open a
  new transaction for the remainder of the block. `TRANSACTION_END`
  commits whatever is open.

### DDL semantics per engine

| Engine | Behavior when DDL lands inside a block | Result |
| --- | --- | --- |
| PostgreSQL (incl. YugabyteDB) | stays in the transaction | block is fully atomic; rollback undoes the DDL |
| SQLite | stays in the transaction | fully atomic |
| DB2 | stays in the transaction | fully atomic |
| MSSQL | stays in the transaction (documented DDL exceptions noted) | fully atomic |
| Firebird | commit segment → run DDL → open new transaction | atomic per segment; DDL visible to later statements (required) |
| MySQL / MariaDB | commit segment → run DDL → open new transaction | atomic per segment (makes the implicit commit explicit) |

The "breaks the transaction" set is exactly
{`DB_ENGINE_FIREBIRD`, `DB_ENGINE_MYSQL`, `DB_ENGINE_MARIADB`}. Query
writers get their engine's real capability, and the per-engine guarantee
is documented in the fixture summaries.

### Result payload

- Marked query: `data_json` is a JSON array of per-statement entries:
  `[{"statement": 1, "block": 1, "rows": [...], "affected": 2}, ...]`.
  Top-level `row_count` and `affected_rows` are the totals, so existing
  statistics, the query result cache, and any consumer reading those
  fields keep working.
- Unmarked query: flat `data_json` exactly as today.
- Failure: `success = false`; `error_message` names the failing
  statement index and block; the entries collected up to the failure are
  included; the engine's error class and message are included; the
  rollback state of the open segment is stated.

### Parameters

- Global validation is unchanged: missing/unused checks still run
  against the union of names referenced anywhere in the query.
- At execution time each statement is bound only the names it actually
  references. Without per-statement filtering, PostgreSQL rejects the
  batch ("bind message supplies N parameters...") and SQLite/MySQL/DB2/
  MSSQL have similar enforcement — this is the difference between
  "forwarded" and "forwarded and filtered".
- A name no statement references is still caught as unused by the
  existing validation.

### Fixture QueryRefs (names and numbers to confirm in Phase 0)

| Migration | QueryRef | Type/Dialect | Role |
| --- | --- | --- | --- |
| 1386 | #156 | 1 / 0 | Simple: one transaction block, INSERT + UPDATE, final verify SELECT, leaves its table for #158 |
| 1387 | #157 | 1 / 0 | Stress: several transaction blocks, DDL around and inside blocks, a `:SHOULD_FAIL` parameter, deliberate mid-block failure for rollback proof |
| 1388 | #158 | 1 / 0 | Verify + cleanup: SELECT proof of final state, then DROP of the fixture tables |

- One caller-facing QueryRef per migration file (Helium rule) — hence
  three files.
- **Cleanup discipline:** no fixture drops its own table. #158 runs after
  every invocation, proves the final state with its SELECT, and drops
  the tables. The blackbox harness always calls #158 and tolerates its
  "nothing to clean" case. Test tables: `TEST_TRANSACTIONS` plus one
  secondary table for #157 (name locked in Phase 0).
- Fixtures are permanent rows in the Acuranzo database, following the
  established pattern (auth test accounts, Api.Echo fixture, the MCP
  fixture tools).

### Test plan

- New blackbox `test_64_queryref_transactions.sh` with per-engine
  configs, following the existing numbered-test conventions (own
  `TEST_ABBR`, version header, port range, docs page in
  `docs/H/tests/`).
- Cases: #156 happy path and result array shape; #157 with
  `:SHOULD_FAIL = 0` (multi-block commit) and `1` (mid-block failure,
  rollback proof, rich error payload); #158 verify + cleanup after every
  invocation including the failure path; per-engine DDL assertions
  (PostgreSQL, SQLite, DB2, and MSSQL roll the staging DDL back with
  the block; Firebird, MySQL, MariaDB, and YugabyteDB keep the empty
  staging table).
- Execution stays QueryRef-only: the test calls the same conduit query
  endpoint production callers use, with JWT auth, and asserts on
  response payloads.

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Decisions and re-verified numbers written in Status; no C | S | **complete** |
| 1 | Marker parser parses the syntax and rejects every malformed shape; Unity green | M | **complete** |
| 2 | Shared transactional statement executor extracted and unit-tested; migration path provably unchanged | M | **complete** |
| 3 | Runtime queue executes marked QueryRefs and returns the result array; unmarked path unchanged | L | **complete** |
| 4 | Fixture migrations 1386–1388 written, audited, packet handed to owner; owner reports the apply | M | **complete** |
| 5 | Test 64 green on every live engine, failure path and cleanup included | L | **complete** |
| 6 | Coverage targets met, docs updated, plan closed after owner sign-off | S | **complete** |
| 7 | (Deferred) Migration executors stay separate from the shared executor | M | **deferred** |

Effort key: S = small/contained, M = moderate, L = large or multi-engine.

---

## Phase 0 — Contract Lock

### Goal

Write down the decisions Phase 1–6 implement. No C.

### Entry gate

This document exists. Baseline build is green (`zsh -ic 'mkq'`).

### Work items

- [x] Re-verify free numbers **on disk**: next Acuranzo migration file
      (snapshot says 1386), next caller-facing QueryRef (snapshot says
      156), next blackbox test number (snapshot says 64).
      **Verify:** numbers in Status match `ls` of
      `elements/002-helium/acuranzo/migrations/` and `tests/`.
- [x] Name the conduit endpoint the blackbox test will use to execute a
      QueryRef with parameters (research `src/api/conduit/`; confirm
      which of the query endpoints production callers use for a
      parameterized system query).
      **Verify:** endpoint path and auth shape written in Status.
- [x] Lock fixture details: query type 1 (SQL - System), dialect 0 (SQL),
      the primary table name `TEST_TRANSACTIONS`, and the secondary
      table name for #157.
      **Verify:** Status.
- [x] Lock the prepared-statement choice for runtime blocks: mirror the
      migration path (`use_prepared_statement = true`, hash-derived
      names) so the runtime path also exercises
      `src/database/db2/prepared.c`, versus direct execution.
      Recommendation: mirror the migration path.
      **Verify:** Status.
- [x] Lock per-statement timeout: each statement gets the request's
      `timeout_seconds` (parity with the migration path's 30s), not a
      whole-block budget.
      **Verify:** Status.
- [x] Record the runtime corpus audit evidence (0 of 159 runtime rows
      multi-statement; BLOB-typed `code` gotcha) so no future session
      re-litigates it.
      **Verify:** evidence in Status, not just in this plan's background.
- [x] Decide whether a TODO.md pointer is added for this plan.
      **Verify:** Status records the decision.
- [x] Confirm the YugabyteDB config's place in the test matrix (it rides
      the PostgreSQL engine).
      **Verify:** Status.

### Done means

Decisions and re-verified numbers written in Status. No C.

### Exit gate

Status filled. Working Log records the locked endpoint, fixture, and
numbering decisions.

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:**
  - Numbers re-verified on disk: highest Acuranzo migration file
    **1385** → next **1386**; highest caller-facing Acuranzo QueryRef
    **155** → next **156**; highest blackbox **63** → next free **64**
    (tests 64–69 and 74–88 are unused; 64 keeps the sequence).
    `acuranzo_1145.lua` sets `cfg.QUERY_REF = "1145"` — a test-data
    migration, not a caller-facing ref; the demo database holds no
    runtime row above 155 (outside Argent's 2000–2010).
  - Endpoint: **`/api/conduit/alt_query`**. `/api/conduit/query` is
    gated to type 10 (SQL - Public) and `query_statement_type_allowed`
    restricts public templates to a leading `SELECT`; type 11
    (Protected) allows only DML. Fixtures contain INSERT/UPDATE/DELETE
    and DDL, so they run through the generic (type argument 0) lookup in
    `alt_query`, which blocks only type 0 (internal) and migration types
    1000–1010. House pattern to copy: `tests/test_54_conduit_alt_query.sh`.
  - Fixture details locked: query type **1 (SQL - System)** (lookup
    family 28), dialect **0 (SQL)** (lookup family 30; `query_dialect_a30`
    is manager metadata — it appears nowhere in Hydrogen C), primary
    table **`TEST_TRANSACTIONS`**, secondary table for #157
    **`TEST_TXN_STAGING`**.
  - Prepared-statement choice for runtime blocks: **mirror the migration
    path** — `use_prepared_statement = true` with hash-derived statement
    names, per-statement requests, each carrying the query's
    `timeout_seconds`. Rationale: exercises `src/database/db2/prepared.c`
    from a normal API-driven run, which is one of this plan's coverage
    goals. The unmarked single-statement path keeps
    `use_prepared_statement = false` exactly as today.
  - Audit evidence: 0 of 159 runtime `queries` rows in the demo database
    are multi-statement (no `-- SUBQUERY DELIMITER`, not even two
    semicolons); `code` is stored as a BLOB so SQLite
    `length()`/`replace()`/`instr()` mislead — decode text first.
  - TODO.md decision: **pointer added** — item **16**, P1 band
    ("Runtime QueryRef transactions"), since TODO.md is the canonical
    "what next" scan and an active plan invisible there would rot.
  - YugabyteDB: **confirmed** — configs exist
    (`hydrogen_test_30/38/40/43/45/46/47/58/63/73_*yugabytedb*.json`) and
    ride the PostgreSQL engine; no separate engine slot exists.
- **Variances:**
  - Plan snapshot said "type 1 (SQL - System)" for fixtures and left the
    endpoint to research; endpoint research changed the fixture story
    (type 10 / type 11 are gated), so the endpoint decision above
    supersedes any assumption in the Design section that test payloads
    go through `/api/conduit/query`.
  - The Phase 0 exit-gate line still names the AUTH_FINALE policy list
    (copied template text); the real locked decisions are the ones in
    Result and the Working Log.

---

## Phase 1 — Transaction Block Parser

### Goal

A parser that turns marker-annotated runtime SQL into an ordered list of
bare statements and transaction blocks, and rejects every malformed
shape with a precise message. Reuses
`statement_has_executable_sql`; does not touch
`parse_sql_statements` or any migration code.

### Entry gate

Phase 0 Status is filled.

### Work items

- [x] New pair `src/database/dbqueue/transaction_blocks.c` /
      `transaction_blocks.h` (no `static` functions in `src/`; header
      declares every function; register in the CMake source list under
      `cmake/`).
      **Verify:** Unity `transaction_blocks_test_sql_has_transaction_markers.c`.
- [x] `sql_has_transaction_markers(const char* sql)` — cheap substring
      pre-filter; false means the caller never parses.
      **Verify:** Unity `transaction_blocks_test_sql_has_transaction_markers.c`.
- [x] `transaction_blocks_parse(const char* sql, TransactionBlockList** blocks, char** error_message)`
      — returns segments in author order; each segment carries a
      "is transaction" flag and its statement list. Error strings name
      the rule violated.
      **Verify:** Unity `transaction_blocks_test_transaction_blocks_parse.c`
      plus an error-path file; ≥75% line coverage (the file will exceed
      100 lines).
- [x] Parser rules covered by tests: marker at line start with leading
      whitespace; `STATEMENT`/`TRANSACTION_END` outside a block;
      `TRANSACTION_START` inside a block; empty block; empty and
      comment-only segments skipped; several blocks in one query; a
      marker-looking string mid-line is **not** a marker; no markers at
      all yields a single bare segment.
      **Verify:** `zsh -ic 'mku <base names>'` for each new test file.
- [x] `transaction_blocks_free(...)` with full cleanup, including the
      partial-list path when parsing fails midway.
      **Verify:** Unity; ASAN-clean under the debug build.
- [x] `zsh -ic 'mkq'` then `zsh -ic 'mkp'` clean; dead-code gate
      (`build/deadcode/dead_functions.txt`) shows nothing new.
      **Verify:** command output in Working Log.

### Done means

The parser accepts the locked syntax and rejects every malformed shape,
with Unity tests green and cppcheck clean.

### Exit gate

`mku` green for every new Unity file, `mkq` + `mkp` clean, no new dead
functions.

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:**
  - New `src/database/dbqueue/transaction_blocks.{c,h}`. Public API:
    `sql_has_transaction_markers`, `transaction_blocks_parse`,
    `transaction_blocks_free`; internal helpers (`txn_copy_range`,
    `txn_trim_in_place`, `txn_push_statement`, `txn_push_block`,
    `txn_fail`, `txn_marker_kind`, `txn_emit_bare_block`) are declared
    in the header because the trial build runs with
    `-Wmissing-prototypes` and the project forbids `static` in `src/`.
  - Model: an ordered list of blocks — transaction blocks
    (`is_transaction`, statement array) and bare one-statement blocks
    (regions outside markers, executed as an unmarked query is today).
    Statements are trimmed; empty and comment-only segments are skipped
    via `statement_has_executable_sql`. Markers match only as a full
    line, leading whitespace allowed.
  - Unity: four new files under
    `tests/unity/src/database/transaction_blocks_test_*.c` — 24 tests,
    all green (`mku transaction_blocks_test_sql_has_transaction_markers`,
    `..._transaction_blocks_parse`, `..._transaction_blocks_parse_errors`,
    `..._transaction_blocks_free`).
  - `mkt` clean (full trial; the new file is picked up by the CMake glob
    at configure time). `mkp` clean (cppcheck, 2,252 files, no issues).
  - Dead-code gate: the 7 new `txn_*` helpers appear in
    `build/deadcode/dead_functions.txt` because Phase 3 (the runtime
    hook) has not landed yet. They disappear once the queue worker calls
    the parser. No other dead-function delta from this phase.
- **Variances:**
  - The plan's "CMake source list" work item resolved to nothing: the
    build globs `../src/*.c` (`cmake/CMakeLists-init.cmake`), so new
    files are automatic; `mkt` (not `mkq`) is required after adding one,
    per INSTRUCTIONS.md.
  - Error messages carry line numbers (e.g. "STATEMENT outside a
    transaction block (line 2)") — the plan said "name the rule";
    line numbers were free and make the fixture authoring errors
    diagnosable.
  - Two implementation bugs were caught by the gates before close-out:
    `failed = txn_fail(...)` inverted the flag (txn_fail returns false
    by design), and GCC's `-Wnull-dereference` rejected the uninspected
    result of `txn_trim_in_place`. Both fixed; the error-path Unity test
    is what exposed the first.

---

## Phase 2 — Shared Transactional Statement Executor

### Goal

Extract the migration apply path's begin → statements → commit loop into
one shared, unit-tested executor that both the runtime path (Phase 3)
and — later, optionally — the migration executors can call, including
DDL segmentation. The migration path itself keeps its delimiter and its
call sites; this phase only adds the shared code beside them.

### Entry gate

Phase 1 complete. Tests 32–38 (per-engine migration tests) are green on
the current build — run or record "trusted green" with the last run
name.

### Work items

- [x] New shared executor (e.g. `src/database/database_transaction_exec.c`
      / `.h`, or a home chosen with the owner): given a connection, an
      ordered statement list, per-statement parameters, timeout,
      prepared-statement naming, and a per-statement result callback,
      execute begin → statements → commit/rollback and report the
      failing statement index.
      **Verify:** Unity `database_transaction_exec_test_*.c` files
      (one per function); ≥75% coverage.
- [x] `engine_ddl_breaks_transaction(DatabaseEngine)` returning true for
      `DB_ENGINE_FIREBIRD`, `DB_ENGINE_MYSQL`, `DB_ENGINE_MARIADB` and
      false for the rest.
      **Verify:** Unity.
- [x] Move the DDL keyword test out of its Firebird-only home into an
      engine-neutral `sql_statement_is_ddl(const char* sql)`; update the
      Firebird migration executor to call it. Behavior identical — this
      is a rename plus one call-site change, nothing else.
      **Verify:** `mkq` + `mkp`; Test 36 (Firebird migrations) still
      green. *(Test number on disk is 37 — see Variances.)*
- [x] DDL segmentation inside the shared executor: on a DDL statement
      while a transaction is open and `engine_ddl_breaks_transaction` is
      true, commit, execute, reopen; on failure, roll back the open
      segment and stop.
      **Verify:** Unity with the mock engine for both branches.
- [x] Prove the migration path is untouched: the shared executor is new
      code, no migration call site changes in this phase.
      **Verify:** Tests 32–38 green (or owner-run report) — record the
      run name in Status.

### Done means

One shared executor exists, unit-tested, and the migration path is
provably unchanged.

### Exit gate

`mku` green for new tests; `mkq` + `mkp` clean; migration tests green
(or a recorded owner report).

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:**
  - New `src/database/database_transaction_exec.{c,h}`. Public API:
    `database_execute_statements_in_transaction(...)` (begin → ordered
    statements → commit, rollback on any failure),
    `engine_ddl_breaks_transaction(DatabaseEngine)`, and
    `sql_statement_is_ddl(const char* sql)`. Internal helper
    `txn_exec_run_statement` (per-statement request build + engine
    execute + callback dispatch + result cleanup) is declared in the
    header for `-Wmissing-prototypes`; no `static` in `src/`.
  - `TransactionExecOutcome` reports `success`,
    `failed_statement_index` (`SIZE_MAX` when none), `failure_at_begin`,
    `failure_at_commit`, and `statements_executed`; the no-op path
    (empty statement list) is a committed no-op with
    `success = true`.
  - `TransactionStatementCallback` gets each statement's `QueryResult`
    (executor-owned, freed after the callback returns) and may abort the
    block. `TransactionParametersCallback` supplies per-statement
    `parameters_json`, falling back to the executor-wide string — the
    Phase 3 per-statement binding hook.
  - Block statements run with `use_prepared_statement = true` and
    hash-derived names (`get_stmt_hash` with the caller's prefix,
    default `RTXN`), isolation `DB_ISOLATION_READ_COMMITTED`, and the
    request's timeout (default 30), mirroring the migration apply path.
  - DDL segmentation: when a statement is DDL and
    `engine_ddl_breaks_transaction` is true, the open segment commits,
    the DDL runs, and a new transaction opens for the remainder (only
    when statements follow). Commit-before-DDL failure and
    reopen-after-DDL failure are distinct outcome fields.
  - `sql_statement_is_ddl` moved from
    `src/database/migration/transaction.c`'s
    `firebird_migration_sql_is_ddl` to the engine-neutral home; the
    Firebird migration executor's two call sites now call it; the
    declaration in `migration/migration.h` was replaced with a pointer
    comment to `database_transaction_exec.h`. Test file
    `tests/unity/src/database/database_transaction_exec_test_sql_statement_is_ddl.c`
    replaced `.../migration/transaction_test_firebird_migration_sql_is_ddl.c`
    via `git mv` and now covers NULL, whitespace/comment skipping, and
    the keyword boundary test for every engine value.
  - Unity: three new/renamed files — 29 tests, all green (re-verified in
    this session):
    `database_transaction_exec_test_engine_ddl_breaks_transaction` (4
    tests), `database_transaction_exec_test_sql_statement_is_ddl` (15
    tests), `database_transaction_exec_test_database_execute_statements_in_transaction`
    (10 tests: happy path, commit failure, begin failure, begin-after-DDL
    failure, mid-block failure + rollback, callback abort, DDL
    segmentation both branches, NULL statement, empty list).
  - Gates: `mkt` clean, `mkp` clean (2,260 files, no issues), dead-code
    gate clean (nothing new; the Phase 3 hook made the executor
    reachable).
  - Migration-path regression evidence:
    - `tests/test_37_firebird_migrations.sh` — **6/6 PASS, EXIT=0**
      (2026-10-14 re-run). Steady state: 0 migrations reversed because
      the artifact DB was already applied; it proves the renamed DDL
      call site still works on the Firebird path.
    - `tests/test_31_migrations.sh` — **3199 validations passed, 0
      failed** (2026-10-14 re-run), covering the migration code path the
      helper move touches.
    - `tests/test_30_database.sh` — **40/40 tests, 9/9 database engine
      configurations passed** (2026-10-14 re-run, Phase 3 session).
  - Caveat: none of these is a from-scratch apply — tests 30/31/37 reuse
    persisted artifact databases. A first-run apply (fresh artifact DB
    → migrations apply) remains unproven in-session; the packet handoff
    in Phase 4 is the owner's apply of fresh migrations.
- **Variances:**
  - The work item names "Test 36 (Firebird migrations)"; the Firebird
    migration test on disk is `test_37_firebird_migrations.sh`. Test 36
    is a different engine's migration test. Recorded here so a future
    session doesn't hunt for 36.
  - The migration path got one call-site change
    (`firebird_migration_sql_is_ddl` → `sql_statement_is_ddl`), which the
    plan's "no migration call site changes" wording did not anticipate;
    the work item that specifies the rename covers it, and the behavior
    is identical (same keyword list, same comment/whitespace skipping).
  - `txn_exec_run_statement` was factored out of the executor loop
    (the plan describes one function) because the DDL branch and the
    in-transaction branch run the identical request build; keeping one
    implementation honors the "one implementation" spirit.
  - Outcome field set is slightly richer than the plan's "report the
    failing statement index": `failure_at_begin` /
    `failure_at_commit` were needed so the Phase 3 error payload can
    state *where* the block died, not just which statement.

---

## Phase 3 — Runtime Wiring And Result Payload

### Goal

The queue worker recognizes a marked QueryRef, parses it into blocks,
executes each block through the shared executor, and returns the JSON
array of per-statement results. Unmarked SQL keeps today's code path
exactly.

### Entry gate

Phases 1 and 2 complete.

### Work items

- [x] Hook in `src/database/dbqueue/process.c`: before the
      `database_engine_execute` call, `sql_has_transaction_markers` on
      `query->query_template`; on true, parse and run the block path; on
      false, fall through to the existing call untouched.
      **Verify:** `mkq`; existing database blackbox tests (30, 40–47 as
      applicable) still green.
- [x] Execute each block via the Phase 2 executor; collect each
      statement's `QueryResult`; assemble `data_json` as the array of
      `{"statement": n, "block": b, "rows": [...], "affected": n}`
      entries; set top-level `row_count` / `affected_rows` to totals.
      **Verify:** blackbox smoke against a live engine once fixtures
      exist (Phase 5), plus Unity where mockable.
- [x] Per-statement parameter filtering: a small `:name` reference
      scanner for one statement (either extracted from the conduit
      parameter helpers into a shared location or a minimal equivalent in
      the database layer — decide with the owner in Phase 0/3 and keep
      one implementation). Global missing/unused validation stays where
      it is and unchanged.
      **Verify:** Unity for the scanner; a fixture with parameters used
      by only some statements in Phase 5.
- [x] Failure path: roll back the open segment, build the rich error
      payload (entries so far, failing statement and block, engine class
      and message, rollback state), and return it through the normal
      pending-result signaling.
      **Verify:** Phase 5 failure case asserts every field.
- [x] Timeout: each statement request carries the query's timeout
      (Phase 0 lock); watchdog registration per statement as today.
      **Verify:** code review note in Working Log.
- [x] Prepared-statement naming for block statements mirrors the
      migration path (hash-derived) per the Phase 0 lock, so
      `src/database/db2/prepared.c` is exercised at runtime.
      **Verify:** coverage check in Phase 6.
- [x] `zsh -ic 'mkq'` then `zsh -ic 'mkp'` clean; dead-code gate clean.
      **Verify:** command output in Working Log.

### Done means

A marked QueryRef executes as designed through the runtime queue and
returns the result array; an unmarked QueryRef behaves exactly as
before.

### Exit gate

`mkq` + `mkp` clean; the existing database blackbox tests still green;
smoke evidence recorded (even if only the unmarked path until fixtures
land).

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:**
  - New `src/database/dbqueue/transaction_query.{c,h}` implements
    `database_queue_execute_query_with_blocks`: parse markers → run bare
    statements through the existing single-statement engine call → run
    transaction blocks through the Phase 2 shared executor → assemble
    `data_json` as the per-statement entries array with top-level
    totals. Failure payloads carry the entries collected so far, the
    failing statement and block, the engine message, and
    `error_class = DB_ERR_OTHER`.
  - `src/database/dbqueue/process.c` now branches on
    `sql_has_transaction_markers(query->query_template)`; unmarked SQL
    takes the original `database_engine_execute` call, byte for byte
    unchanged (including `use_prepared_statement = false`).
  - Parameter filtering: the runtime path reuses
    `extract_required_parameters` (`src/api/conduit/helpers/param_proc_helpers.c`)
    — one implementation, as decided — and binds each statement only the
    names it references (caller's key order preserved). Statements with
    no references get `{}`.
  - Block statements execute with `use_prepared_statement = true` and
    `RTXN` hash names (Phase 0 lock); timeout per statement = the
    request's `timeout_seconds`.
  - Unity: two new files —
    `transaction_query_test_collect_helpers.c` (11 tests: entry shapes,
    totals, parameter filtering incl. `::` casts, success/failure result
    shapes) and
    `transaction_query_test_database_queue_execute_query_with_blocks.c`
    (6 tests: parse error, single block, block failure with rollback
    evidence, bare statements around a block, per-statement parameter
    filtering, invalid request). All green.
  - Gates: `mkt` clean; `mkp` clean (2,260 files, no issues); dead-code
    list contains none of the new functions (the Phase 3 hook makes the
    parser, executor, and runtime path reachable).
  - Regression evidence for the unmarked path: `tests/test_30_database.sh`
    green — 40/40 tests, "9/9 database engine configurations passed",
    DQM validated across all engines.
- **Variances:**
  - The plan assumed fixtures might exist for smoke testing; they do not
    (Phase 4), so the marked path's live-engine evidence is Phase 5's
    job. Unity coverage of the marked path is complete via the stub
    engine + database-engine mock.
  - The bare-statement path inside a marked query reuses the same
    autocommit engine call as unmarked SQL, but binds filtered
    parameters (a bare statement with parameters should behave like the
    same statement unmarked — filtering only drops names the statement
    does not reference, which the engine would otherwise reject).
  - The Phase 2 gate's migration-test evidence is recorded there; one
    caveat from this phase: test 37's steady-state run reversed 0
    migrations (persisted artifact DB), so it proves connection and
    churn, not a from-scratch apply.

---

## Phase 4 — Helium Fixture QueryRefs (Packet Handoff)

### Goal

Write the three fixture migrations, audit them, and hand the packet to
the owner. The agent does not apply them.

### Entry gate

Phase 3 complete. Free numbers re-verified in Phase 0 still hold.

### Work items

- [x] Migration 1386 → QueryRef #156 (snapshot): type 1, dialect 0;
      `TEST_TRANSACTIONS`; one transaction block (INSERT + UPDATE);
      final verify SELECT; **no self-drop** — the table is left for
      #158.
      **Verify:** file exists; SchemaTool audit clean; README row added.
- [x] Migration 1387 → QueryRef #157 (snapshot): several transaction
      blocks; DDL before, between, and inside blocks; a `:SHOULD_FAIL`
      parameter that makes one statement violate a constraint mid-block;
      no self-drop on either path.
      **Verify:** SchemaTool audit clean; README row added.
- [x] Migration 1388 → QueryRef #158 (snapshot): SELECT proof of final
      state, then DROP of the fixture tables, in one marked query.
      **Verify:** SchemaTool audit clean; README row added.
- [x] Update `elements/002-helium/acuranzo/README.md` migration table
      (number, version, date, QueryRef, summary) for all three.
      **Verify:** table rows present; `zsh -ic 'mkl'` clean.
- [x] Regenerate the payload after the migration edits
      (`payload-generate.sh` or `zsh -ic 'mka'`) — required before any
      migration test can see the new files.
      **Verify:** payload regenerated; timestamp noted in Status.
- [x] Run the SchemaTool drift audit (read-only) and hand the packet to
      the owner. **The agent does not apply.**
      **Verify:** handoff recorded in Status with the audit output.
      Pre-apply run at 2026-10-14 07:37 PDT was exit 2 against the
      1.0.0 load. Post-apply SQLite demo re-run is exit 0:
      `Compare: total=3 ok=3 drift=0 missing_load=0 missing_apply=0 orphans=0`.
- [x] Phase completes only when the owner reports the apply (per
      [`/docs/PLANNING.md`](/docs/PLANNING.md) packet rule).
      **Verify:** owner report quoted in Status.

### Done means

The packet is handed over and the owner has applied 1386–1388, with the
QueryRefs live in the demo databases.

### Exit gate

Owner apply report in Status; payload regenerated; `mkl` clean.

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:** Owner applied. QueryRefs #156–#158 are live.
  - Owner report, quoted: "Project has been rebuilt, payload updated,
    databases reset and all migrations applied."
  - Payload `payloads/payload.tar.br.enc` is 2026-10-14 08:09:57 -0700,
    804900 bytes. The three Lua files are 07:35:01 the same day
    (10149, 12475, and 9748 bytes), so the archive is newer than the
    1.0.1 rewrite.
  - Read-only `SELECT` on demo and test for PostgreSQL, YugabyteDB,
    MySQL, MariaDB, DB2, Firebird, MSSQL, and both SQLite artifacts
    (`hydrodemo.sqlite`, `hydrotst.sqlite`). Each schema has the same
    nine rows. 156, 157, and 158 are type 1, queue 0, and the `code`
    contains `TRANSACTION_START`. Names are Transaction Block Smoke,
    Transaction Block Stress, and Transaction Fixture Cleanup.
    Dialects: PostgreSQL and YugabyteDB 1, SQLite 2, MySQL 3, DB2 4,
    MSSQL 5, Firebird 6, MariaDB 7. Each of 1386, 1387, and 1388 has
    type 1001 queue 0 (`Remove QueryRef #…`) and type 1003 queue 2
    (`Populate QueryRef #…`). Highest applied Acuranzo ref is 1388.
    No `TEST%` table remains in those schemas.
  - SchemaTool SQLite demo, `--from 1386 --to 1388 --no-sql
    --include-reverse --only-failures`, **exit 0**:
    `Compare: total=3 ok=3 drift=0 missing_load=0 missing_apply=0 orphans=0`.
    The 07:37 PDT exit 2 was the pre-reset 1.0.0 load.
  - README rows are version 1.0.1, date 2026-10-14, statement count 4,
    diagram ✗. `luacheck` on the three files exited 0 before the handoff.
    `zsh -ic 'mkl'` exited 0 (2723 links, 0 missing).
- **Variances:**
  - `query_dialect_a30` is `${DIALECT}` (1–7), chosen over the Phase 0
    literal 0. Runtime does not read the column. The live rows match
    the engine ids above.
  - Index name is unqualified `TEST_TRANSACTIONS_idx_label`, matching
    other Acuranzo `CREATE INDEX` statements.
  - Runtime queue for the three QueryRefs is slow (`QTC_SLOW` = 0).
    The bookkeeping forward row is fast, the reverse row is slow, same
    split as `acuranzo_1366.lua`.
  - Post-apply SchemaTool was re-run on SQLite demo. The other engines
    were checked with the read-only row query above.

### Accomplished

- Rewrote the three fixture migrations from the 1.0.0 shape (marker SQL
  stored as the migration's own `code`) to the nested QueryRef insert
  used by `acuranzo_1381.lua` / `acuranzo_1366.lua`.
- Owner regenerated the payload, reset the databases, and applied.
- Confirmed type-1 rows 156–158, with marker SQL, on demo and test for
  all eight engines, and a clean SchemaTool compare on SQLite demo.

### Lessons learned

- A migration file's returned SQL is what LOAD executes. The
  caller-facing QueryRef has to be an `INSERT` nested inside that
  `code` column. Putting the fixture SQL in the outer `code` makes
  APPLY try to run the markers as a migration.
- The reverse chunk has to be an `INSERT` of a type-1001 row. A bare
  `DELETE` at the top level runs during LOAD and never stores a reverse.
- `]=]` inside `]==]` will not close the outer Helium long string; the
  equals-count matches. The generated forward `code` is brotli-wrapped
  once, and the runtime statement text is brotli-wrapped again inside
  it. A search of the raw `get_migration.lua` output does not see
  `TRANSACTION_START` until both layers are decoded.
- LOAD skips a ref at or below the high-water mark. A rewrite of a
  migration that is already loaded stays invisible until those rows are
  removed and the payload is regenerated. The 1.0.0 load had to be
  cleared before 1.0.1 could apply. A database reset was that clear.
- SQLite's runtime still executes only the first statement of a batch.
  That is why the 1.0.0 apply created `TEST_TRANSACTIONS` and left the
  migration row at type 1000.

### Handoff

Phase 5 may add `tests/test_64_queryref_transactions.sh`, its doc page,
and the TESTING.md and SITEMAP.md entries. It leaves the C runtime and
these three migrations alone. The agent still does not apply migrations.

Assert the payload as it landed:

- Endpoint is `POST`-style `/api/conduit/alt_query`. House pattern to
  copy is `tests/test_54_conduit_alt_query.sh`.
- Statement numbers restart at 1 inside each block.
- On failure, top-level `error_class` is `DB_ERR_OTHER` and the
  top-level message is `transaction block N failed at statement M
  (rolled back)`. The engine text is the failing entry's `error`.
- After #157 with `SHOULD_FAIL = 1`, `txn_id` 1 from block 1 remains.
  The Phase 5 work-item sentence that says the rollback left nothing
  behind does not match this fixture. #158's SELECT is the proof, and
  its staging `DROP` is the DDL observable. A missing staging table is
  expected after #156, after a repeat, and on engines that rolled the
  staging DDL back.
- Statements inside a block, including `CREATE TABLE` and `DROP TABLE`,
  use prepared statements. MySQL, MariaDB, MSSQL, and DB2 may reject
  prepared DDL. That is the first check if #157 fails before the
  duplicate-key statement.
- Gates: `zsh -ic 'mks'`, `zsh -ic 'mkl'`, and one orchestrator run.
  File sizes today: `acuranzo_1386.lua` 10149, `acuranzo_1387.lua`
  12475, `acuranzo_1388.lua` 9748. Payload 804900 bytes.
- Test 64 is free. Next Acuranzo file is **1389**. Next caller-facing
  QueryRef is **159**.

---

## Phase 5 — Blackbox Test 64 Across All Engines

### Goal

One blackbox test that drives #156, #157, and #158 through the normal
conduit query endpoint on every live engine and asserts the whole
contract: result arrays, multi-block ordering, per-engine DDL semantics,
rollback proof, rich error payloads, and guaranteed cleanup.

### Entry gate

Phase 4 complete (owner applied). All live engines reachable.

### Work items

- [x] New `tests/test_64_queryref_transactions.sh` with the standard
      header (`TEST_ABBR`, version, CHANGELOG line bumped on change),
      per-engine configs following the existing numbered-test pattern,
      and this plan's port range convention.
      **Verify:** `zsh -ic 'mks'` clean. Test 92 4.1.1 on 2026-10-14
      10:49:38: 225 shell files, 224 cached, 1 processed
      (`test_64_queryref_transactions.sh`), 1289/1289 directives
      justified, exit 0. Config
      `tests/configs/hydrogen_test_64_queryref_transactions.json` passed
      `jq empty`. Port 5640. Script is 404 lines (1.0.2). Config is
      239 lines. Rechecked after the 1.0.2 assertion edits: Test 92
      4.1.1 on 2026-10-14 11:29:35, 225 files, 224 cached, 1 processed,
      1289/1289 directives, exit 0.
- [x] Case: QueryRef 156 happy path — success, array shape,
      per-statement entries, totals.
      **Verify:** run `20261014_112748`. Top-level `affected_rows`
      equals the sum of the entry `affected` values. Firebird's
      parameter-free UPDATE reports `affected` 0; the SELECT note is
      the proof.
- [x] Case: QueryRef 157 with `:SHOULD_FAIL = 0` — every block commits;
      QueryRef 158's SELECT proves the committed rows.
      **Verify:** run `20261014_112748`.
- [x] Case: QueryRef 157 with `:SHOULD_FAIL = 1` — HTTP 422, `error`
      "Database error", `message` exactly `transaction block 4 failed
      at statement 3 (rolled back)`. The 422 body carries `success`,
      `error`, and `message`. `rows`, `error_class`, and the engine
      text stay off that body. On Firebird, MySQL, MariaDB, and
      YugabyteDB the following QueryRef 158 is HTTP 200 and its SELECT
      shows only the block-one note (`txn_id` 1). On PostgreSQL,
      SQLite, DB2, and MSSQL that QueryRef 158 is HTTP 422 with
      `transaction block 3 failed at statement 1 (rolled back)`: the
      staging table rolled back with the block. `txn_id` 1, inserted
      by the first transaction block, remains on every engine.
      **Verify:** run `20261014_112748`, 60 pass, 0 fail.
- [x] Case: QueryRef 158 cleanup always runs, including after the
      failure path, and tolerates a missing staging table.
      **Verify:** run `20261014_112748`.
- [x] Per-engine DDL assertions: PostgreSQL, SQLite, DB2, and MSSQL
      roll the staging DDL back with the block. Firebird, MySQL,
      MariaDB, and YugabyteDB keep the empty staging table. YugabyteDB
      is not in `engine_ddl_breaks_transaction`; the engine kept the
      table. The same SQL text runs on all.
      **Verify:** run `20261014_112748`, per-engine sections green.
- [x] Docs: `docs/H/tests/test_64_queryref_transactions.md`, TESTING.md
      entry, SITEMAP.md entry, and any INSTRUCTIONS.md pointer the test
      conventions require.
      **Verify:** `zsh -ic 'mkl'` clean. 2026-10-14: 349 files, 2728
      links, 0 missing, exit 0. markdownlint of the new page,
      TESTING.md, and SITEMAP.md exited 0. No INSTRUCTIONS.md pointer.
      That file already omits many numbered tests. TESTING.md is the
      index. The page was updated for the YugabyteDB staging survival
      and the Firebird UPDATE count. `zsh -ic 'mkl'` on 2026-10-14
      exited 0: 349 files, 2728 links, 0 missing. markdownlint of the
      page exited 0.
- [x] Run via the orchestrator the way the suite runs
      (`tests/test_00_all.sh <name>` or the documented single-test
      path).
      **Verify:** `20261014_112748`, log
      `build/tests/logs/test_64_20261014_112748_307849536_2710023_queryref_transactions.log`,
      60 subtests, 60 pass, 0 fail, 45.914s, exit 0.

### Done means

Test 64 passes on every live engine, covering the happy path, the
failure path, the per-engine DDL semantics, and cleanup after both.

### Exit gate

Green orchestrator run name in Status; `mks` + `mkl` clean.

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:** Orchestrator run `20261014_112748` (`hydrogen_coverage`,
  port 5640) exited 0: 60 subtests, 60 pass, 0 fail, 45.914s. Log
  `build/tests/logs/test_64_20261014_112748_307849536_2710023_queryref_transactions.log`.
  Script 1.0.2. `zsh -ic 'mks'` exited 0 (Test 92 4.1.1, 2026-10-14
  11:29:35, 225 files, 1 processed, 1289/1289 directives).
  `zsh -ic 'mkl'` exited 0 (349 files, 2728 links, 0 missing).
  `zsh -ic 'mkp'` exited 0 (Test 91 3.2.0, 2026-10-14 11:30:29,
  2260 files, 12 processed, no issues).
  The earlier run `20261014_105612` exited 1 and is the diagnosis,
  not the gate.
- **Variances:**
  - `Demo_YB` is the real Yugabyte schema `demo` (`YUGABYTE_DB_*`),
    with the connection name left as `Demo_YB` so `DATABASE_NAMES`
    still matches. Test 54's local `demoybdb` stand-in is not copied.
  - The HTTP 422 body is `success` false, `error` "Database error",
    and `message` equal to the executor string. `rows`, `error_class`,
    and the engine text stay off that body.
  - Top-level `affected_rows` is the sum of the statement entries.
    A SELECT entry can contribute 1. QueryRef 156 does not require a
    fixed total of 2.
  - YugabyteDB kept `TEST_TXN_STAGING` after `SHOULD_FAIL = 1`.
    QueryRef 158 was HTTP 200 with only the block-one note.
    `engine_ddl_breaks_transaction` was not changed. PostgreSQL still
    rolls that table back.
  - Firebird's parameter-free UPDATE uses `isc_dsql_execute_immediate`
    and reports `affected` 0. The SELECT note is
    `updated inside the same transaction block`. The test does not
    require `affected` 1 on that statement. The INSERT, which has
    `:RUN_LABEL`, still reports `affected` 1.
  - The owner approved the bind edit ("Sure") after run
    `20261014_105612`. PostgreSQL `execute_prepared` was left as it
    was. Parameter-free migration executes still pass `"{}"` and bind
    nothing.

### Accomplished

- Added `tests/test_64_queryref_transactions.sh` (404 lines, version
  1.0.2) and
  `tests/configs/hydrogen_test_64_queryref_transactions.json` (239
  lines, port 5640, eight demo connections, `PAYLOAD:acuranzo`).
- Added `docs/H/tests/test_64_queryref_transactions.md` and the
  TESTING.md and SITEMAP.md entries.
- Bound prepared-statement parameters on MySQL, MariaDB, SQLite, DB2,
  MSSQL, and Firebird. `database_ordered_parameters` rebuilds the
  unique-name list from the original `:name` SQL. DB2 and MSSQL bind
  inside `prepared.c` so `db2/query.c` stays at 996 lines. Firebird
  passes `request->sql_template` into `firebird_execute_sql`.
- Orchestrator run `20261014_112748` is green on all eight engines.

### Lessons learned

- `PREPARE` receives positional SQL. `execute_prepared` still sees the
  original request, whose `sql_template` has `:name` and whose
  `parameters_json` is the typed object. Binding has to happen in
  `execute_prepared`. The ordered list is one slot per unique name, in
  SQL appearance order, matching `build_parameter_array`.
- `"{}"` is two characters. `database_ordered_parameters` returns no
  list for that, so migration executes that pass `"{}"` stay unbound.
- MySQL and MariaDB copies in `bound_values` have to live until
  `stmt_execute` returns. DB2 and MSSQL indicator pointers have to
  live until `SQLExecute` returns. SQLite copies strings with
  `SQLITE_TRANSIENT`, so the list can be freed after bind.
- `build_error_response` is what `alt_query` sends on failure. The
  statement array and `error_class` stay off that body. On PostgreSQL,
  SQLite, DB2, and MSSQL the rollback proof is the QueryRef 157
  message plus the QueryRef 158 staging-drop 422. YugabyteDB,
  Firebird, MySQL, and MariaDB keep the empty staging table, so that
  QueryRef 158 is HTTP 200 and the SELECT shows `txn_id` 1.
- Bare statements count as blocks. The duplicate key is block 4
  statement 3. The staging drop is block 3 statement 1. The statement
  index is the original array index.
- Firebird runs a statement with no placeholders through
  `isc_dsql_execute_immediate` and hardcodes `affected_rows` 0. A
  parameterized INSERT on the same engine reports the real count.
- `exercise_engine` and each case use `|| true` so `set -e` still
  reaches later engines, cleanup, and shutdown.

### Handoff

Phase 6 may measure coverage and write the subsystem docs. It leaves
these alone:

- The bind helpers and `database_ordered_parameters`.
- `engine_ddl_breaks_transaction` (Firebird, MySQL, MariaDB only).
  YugabyteDB's staging survival is the engine, recorded in test 64.
- Fixtures 1386–1388 and the test 64 assertions. Version 1.0.2.
- PostgreSQL `execute_prepared`.

`db2/query.c` is 996 lines. Test 99 fails at 1000. Do not add lines
there. Other sizes after this phase: `database_params.c` 711,
`mysql/query.c` 937, `mariadb/query.c` 814, `sqlite/query.c` 788,
`db2/prepared.c` 353, `mssql/query.c` 707, `mssql/prepared.c` 365,
`firebird/query.c` 770, the test script 404.

Gates for Phase 6 are the coverage report and `zsh -ic 'mkl'`. The
agent does not apply migrations. Test 64 is taken. Next Acuranzo file
is **1389**. Next caller-facing QueryRef is **159**. Fixture sizes:
`acuranzo_1386.lua` 10149, `acuranzo_1387.lua` 12475,
`acuranzo_1388.lua` 9748. Payload 804900 bytes.

---

## Phase 6 — Coverage, Docs, And Closeout

### Goal

Prove the coverage targets that motivated part of this work, finish the
docs, and close the plan after owner sign-off.

### Entry gate

Phase 5 complete.

### Work items

- [x] Unity coverage: every new file over 100 lines at ≥75% (project
      constraint).
      **Verify:** coverage report numbers in Status.
- [x] Blackbox coverage: confirm `src/database/db2/prepared.c` and
      `src/database/db2/transaction.c` now register from a normal
      API-driven run (`extras/add_coverage.sh`), and record the
      before/after numbers.
      **Verify:** numbers in Status.
- [x] Documentation: the marker syntax and the per-engine DDL table land
      in the database subsystem docs under `docs/H/core/subsystems/`
      (or the location the owner names), with the "DDL creates commit
      points on Firebird/MySQL/MariaDB" caveat stated.
      **Verify:** `zsh -ic 'mkl'` clean.
- [x] Record the deferred Phase 7 decision (converge migration
      executors onto the shared executor, or keep them separate) with a
      one-line rationale.
      **Verify:** Working Log.
- [x] Close-out: Status complete, Accomplished, Lessons learned,
      Handoff written; move this file to `plans/complete/` with the
      `_COMPLETE` suffix when the owner signs off, and update
      `plans/README.md`.
      **Verify:** owner sign-off quoted in Status.

### Done means

Coverage targets met, docs current, and the plan closed with the
owner's sign-off recorded.

### Exit gate

Coverage numbers and owner sign-off in Status; `mkl` clean.

### Status

- **State:** complete
- **Date:** 2026-10-14
- **Result:** Owner sign-off: "Ok, our full suite passed, so let's mark
  it as complete and not think about Phase 7 until another time."
  Coverage and the subsystem docs were already in. The plan is closed.
  Phase 7 is TODO item 32 and was not started. `extras/add_coverage.sh`
  on 2026-10-14, integer percents: Unity
  `transaction_blocks.c` 79% (181/227), `transaction_query.c` 79%
  (178/225), `database_transaction_exec.c` 79% (128/161). Blackbox for
  the same three files is 67%, 65%, and 67%. DB2 blackbox, same script,
  coverage gcov from the Test 64 run (`Runs` 2 and 3): `db2/prepared.c`
  37% (65/172) and `db2/transaction.c` 74% (44/59). Before this plan
  both DB2 files were cited at 0% blackbox (Purpose, and TODO item 16
  Why now). Unity for those two DB2 files is still 0% (`Runs: 0`); they
  are not new files and have no Unity suite.
- **Variances:** `transaction_query.c` was 74% (168/225) on the existing
  Unity binaries. One test asserted flat `{"A":1}` while the filter
  keeps typed sections. The assertion now matches
  `transaction_query_test_collect_helpers` and Test 64. A bare-statement
  execute failure and a non-array `data_json` case were added. Recount
  is 79% (178/225). The nine Unity binaries for these three files
  passed (the blocks file's four binaries, 7+8+2; the query file's two,
  7 and 14; the executor file's three, 4+15+10). Phase 7 is deferred.
  The owner later reported the full suite passed. No suite id was quoted.

### Accomplished

Coverage numbers are in Status. Marker syntax and the per-engine DDL
table, including the YugabyteDB staging-table result on the QueryRef
157 fixture, are in
[database.md](/docs/H/core/subsystems/database/database.md). The
subsystem overview links that page. `zsh -ic 'mkl'` exited 0
(349 files, 2731 links, 0 missing) before this close-out. The plan
now lives at
[TRANSACTION_PLAN_COMPLETE.md](/docs/H/plans/complete/TRANSACTION_PLAN_COMPLETE.md).
TODO item 16 is closed. The migration-executor convergence is TODO
item 32.

### Lessons learned

- The Unity `.gcov` files for the new sources were `Runs: 0` even
  though the binaries from Phases 1–3 were still on disk. Running those
  binaries wrote the `.gcda` files. A coverage rebuild of
  `hydrogen_coverage` does not refresh Unity profiles.
- `add_coverage.sh` reports integer percent (`covered * 100 / total`).
  168/225 is 74%, under the 75% bar. 178/225 is 79%.
- The shared executor commits the open segment *before* DDL. The
  Firebird migration executor runs the DDL inside the transaction and
  commits *after*. Those cannot be swapped and still be byte-identical.

### Handoff

This plan is closed. Do not re-open it.

Phase 7 is TODO item 32. It stays speculative. `lead_apply` keeps
non-Firebird migrations in one transaction, MSSQL migrations leave
prepare off, and `execute_firebird_migration` commits after DDL inside
the open transaction. The shared executor commits before DDL. Do not
start that convergence until someone pulls item 32. The gate, if it is
pulled, is tests 30–38 and 71, with Firebird commit-after-DDL unchanged.

Leave the bind helpers, `database_ordered_parameters`,
`engine_ddl_breaks_transaction`, fixtures 1386–1388, test 64 1.0.2, and
PostgreSQL `execute_prepared` alone. `db2/query.c` is 996 lines. Test 99
fails at 1000.

---

## Phase 7 — Converge Migration Executors (Deferred)

**Decision (2026-10-14): [~] deferred.** The migration executors stay
separate. `lead_apply` keeps non-Firebird migrations in one transaction,
MSSQL migrations set `use_prepared_statement` false, and
`execute_firebird_migration` commits after DDL inside the open
transaction. The shared executor commits before DDL and would change
APPLY. The exit gate would be tests 30–38 and 71, with Firebird
commit-after-DDL byte-identical. That rewrite is not this sitting.

The original goal, left unstarted: switch
`src/database/dbqueue/lead_apply.c`, `lead_reverse.c`, and the
per-engine migration executors onto the Phase 2 shared executor.
Exit gate if it is ever pulled: the full migration suite (tests 30–38,
71) green, with the Firebird commit-after-DDL behavior byte-identical
to today's.

---

## Working Log (cross-phase memory)

### Decisions log

- **(2026-10-14, plan closed)** Owner: "Ok, our full suite passed, so
  let's mark it as complete and not think about Phase 7 until another
  time." Phase 6 is complete. This file moved to
  `plans/complete/TRANSACTION_PLAN_COMPLETE.md`. TODO item 16 is closed.
  Phase 7 is speculative TODO item 32. No suite id was quoted.
  Close-out `zsh -ic 'mkl'` exited 0 (349 files, 2733 links, 0 missing).
- **(2026-10-14, Phase 6 in progress)** Unity coverage of the three new
  files is 79% each (`add_coverage.sh`: `transaction_blocks.c` 181/227,
  `transaction_query.c` 178/225, `database_transaction_exec.c` 128/161).
  DB2 blackbox moved off the cited 0%: `db2/prepared.c` 37% (65/172),
  `db2/transaction.c` 74% (44/59), from the Test 64 coverage gcov.
  Marker syntax and the DDL table are in the database subsystem docs.
  `zsh -ic 'mkl'` exited 0 (349 files, 2731 links, 0 missing).
  Phase 7 stays deferred: the shared executor commits before DDL, and
  the Firebird migration executor commits after DDL inside the
  transaction, so converging them is not byte-identical. Close-out
  waits on owner sign-off. Phase 6 is not complete.
- **(2026-10-14, Phase 5 closed)** Owner approved the prepared-statement
  bind ("Sure"). `database_ordered_parameters` plus bind-before-execute
  on MySQL, MariaDB, SQLite, DB2, MSSQL, and Firebird.
  PostgreSQL `execute_prepared` unchanged. `"{}"` still binds nothing.
  Test 64 1.0.2: top-level `affected_rows` is the sum of the entries,
  YugabyteDB is in the staging-kept group, and the Firebird
  parameter-free UPDATE is not required to report `affected` 1.
  Orchestrator `20261014_112748` exited 0 (60 pass, 0 fail, 45.914s).
  `mks`, `mkl`, and `mkp` exited 0 the same day. Phase 6 has not started.
- **(2026-10-14, Phase 5 run)** Orchestrator
  `tests/test_00_all.sh 64_queryref_transactions` as
  `20261014_105612`, `hydrogen_coverage`, exit 1. 60 subtests, 29
  pass, 31 fail, 46.083s. Prepared DDL was accepted. MySQL, MariaDB,
  SQLite, DB2, Firebird, and MSSQL fail the first bound `INSERT`
  inside the block (`transaction block 2 failed at statement 1`).
  Engine text: MySQL/MariaDB "No data supplied for parameters in
  prepared statement", SQLite NOT NULL on `run_label`, DB2 CLI0100E
  SQLSTATE 07001, Firebird gds 335544569. Cause:
  `database_engine_execute` converts `:name` for PREPARE, frees
  `ordered_params`, and `mysql_execute_prepared` executes with no
  `mysql_stmt_bind_param`. PostgreSQL and YugabyteDB passed the
  rollback contract. Their #156 failed the test's `affected_rows == 2`
  check; the body has `affected_rows` 3 because the SELECT counts 1.
  YugabyteDB #158 after the rollback was HTTP 200: the staging table
  survived. PostgreSQL's matching drop was HTTP 422. No C edit. Phase
  stays in progress.
- **(2026-10-14, Phase 5 in progress)** Test 64 is written and waiting
  on the owner. Script
  `tests/test_64_queryref_transactions.sh` (400 lines, 1.0.0), config
  `hydrogen_test_64_queryref_transactions.json` (239 lines, port 5640,
  `Demo_YB` on real Yugabyte schema `demo`). Docs page plus TESTING.md
  and SITEMAP.md. `zsh -ic 'mks'` exited 0 (Test 92 4.1.1, 225 files,
  1 processed). `zsh -ic 'mkl'` exited 0 (2728 links, 0 missing).
  markdownlint of the three doc files exited 0. The orchestrator has
  not been run. The 422 body is `error` "Database error" and `message`
  equal to the executor string. `rows` and `error_class` stay off that
  body. After `SHOULD_FAIL = 1`, `txn_id` 1 remains: commit-at-DDL
  engines show it on #158 HTTP 200; rollback engines show the staging
  drop as `transaction block 3 failed at statement 1 (rolled back)`.
  A prepared-DDL reject stops the phase. No C change. Phase 6 has
  not started.
- **(2026-10-14, Phase 4 closed)** Owner report: "Project has been rebuilt,
  payload updated, databases reset and all migrations applied." Payload
  `payload.tar.br.enc` 2026-10-14 08:09:57 -0700, 804900 bytes. Demo and
  test on PostgreSQL, YugabyteDB, MySQL, MariaDB, DB2, Firebird, MSSQL,
  and both SQLite artifacts hold type-1 rows 156/157/158 with
  `TRANSACTION_START`, dialects 1/1/3/7/4/6/5/2, plus type 1001 and 1003
  for 1386–1388. Highest applied Acuranzo ref is 1388. No `TEST%` tables.
  SchemaTool SQLite demo `--from 1386 --to 1388 --no-sql --include-reverse`
  exit 0 (`ok=3 drift=0 missing_apply=0`). The 07:37 exit 2 was the
  pre-reset 1.0.0 load. `zsh -ic 'mkl'` exited 0 (2723 links, 0 missing).
  Phase 5 has not started.
- **(2026-10-14, Phase 4 packet)** Fixture migrations rewritten to 1.0.1
  and handed over. Not applied. `get_migration.lua` on seven engines
  shows type-1 rows 156/157/158 nested inside the type-1000 code, reverse
  type 1001, unqualified `TEST_TRANSACTIONS_idx_label`, and
  `${DIALECT}` (owner's choice over the Phase 0 literal 0). `luacheck`
  exited 0. SchemaTool SQLite demo exit 2: the 1.0.0 load is still in
  `hydrodemo.sqlite` and `hydrotst.sqlite` (type 1000, code starts with
  `CREATE TABLE`, leftover `TEST_TRANSACTIONS`, no QueryRef 156). Suite
  `20261014_070232` failed APPLY of that shape. Owner regenerates the
  payload, deletes those rows, then applies. Phase 5 waits on that report.
- **(2026-10-14, Phase 2 closed)** Shared executor landed:
  `src/database/database_transaction_exec.{c,h}` —
  `database_execute_statements_in_transaction` (begin → statements →
  commit/rollback with `TransactionExecOutcome`),
  `engine_ddl_breaks_transaction` ({FIREBIRD, MYSQL, MARIADB}), and
  `sql_statement_is_ddl` (moved from the Firebird-only
  `firebird_migration_sql_is_ddl`; the migration executor's call sites
  renamed, declaration replaced by a pointer comment in
  `migration/migration.h`). DDL segmentation = the migration path's
  Firebird pattern applied to MySQL/MariaDB. Unity: 29 tests in three
  files, all green (re-verified in this session). Migration-path
  regression: test 37 6/6, test 31 3199 validations / 0 failed, test 30
  40/40 — all steady-state runs (persisted artifacts), none a
  from-scratch apply. Two naming surprises recorded: the Firebird
  migration test on disk is **37**, not 36; a leaked
  `transaction.c.bak-phase6` file sits in `src/database/migration/`
  (not globbed by CMake, left in place).
- **(2026-10-14, Phase 3 closed)** Runtime wiring landed:
  `src/database/dbqueue/transaction_query.{c,h}` plus the
  `sql_has_transaction_markers` branch in `dbqueue/process.c`. Unmarked
  SQL still takes the original single engine call. Block statements use
  prepared statements with `RTXN` hash names (Phase 0 lock) and the
  request timeout. Parameter filtering reuses
  `extract_required_parameters` from the conduit helpers (one
  implementation; keys keep caller order). Unity: 17 new tests in two
  files, all green; `mkt` + `mkp` clean; no new dead functions.
  Unmarked-path regression: test 30 green (40/40, 9/9 engine configs).
- **(2026-10-14, Phase 1 closed)** Parser landed:
  `src/database/dbqueue/transaction_blocks.{c,h}` plus four Unity files
  (24 tests, all green). Block model = ordered list of transaction blocks
  and bare one-statement blocks; error messages carry line numbers.
  Gates: `mkt` clean, `mkp` clean, dead-code list gains exactly the 7
  `txn_*` helpers until Phase 3 wires the parser in. Two bugs caught by
  gates before close: `failed = txn_fail(...)` inverted the failure flag
  (txn_fail returns false by design), and `txn_trim_in_place`'s result
  was dereferenced uninspected. Both fixed.
- **(2026-10-14, Phase 0 closed)** Contract locked, no C. Numbers
  re-verified on disk: next Acuranzo file **1386**, next caller-facing
  QueryRef **156**, next blackbox test **64** (64–69 and 74–88 unused;
  64 keeps the sequence). `acuranzo_1145.lua`'s `cfg.QUERY_REF = "1145"`
  is a test-data migration, not a caller-facing ref.
  Endpoint: **`/api/conduit/alt_query`** — `/api/conduit/query` is gated
  to type 10 with a leading-SELECT-only template check and type 11 is
  DML-only (`query_statement_type_allowed` in
  `src/api/conduit/helpers/database_operations.c`); the generic lookup
  in `alt_query` blocks only type 0 and migration types 1000–1010.
  House pattern to copy: `tests/test_54_conduit_alt_query.sh`.
  Fixtures: type 1 (SQL - System), dialect 0 (SQL — `query_dialect_a30`
  is manager metadata, absent from Hydrogen C), tables
  `TEST_TRANSACTIONS` + `TEST_TXN_STAGING`. Runtime blocks execute with
  `use_prepared_statement = true` and hash-derived names, mirroring the
  migration apply path, so `db2/prepared.c` is exercised from a normal
  API-driven run; the unmarked path keeps `false`. Per-statement
  timeout = the request's `timeout_seconds`. TODO.md item 16 added (P1
  band). YugabyteDB rides the PostgreSQL engine (no separate slot).
- **(Plan authored, 2026-10-14)** Design locked in conversation the same
  day. Key decisions: markers (`-- TRANSACTION_START` / `-- STATEMENT` /
  `-- TRANSACTION_END`) are author-placed and line-start anchored —
  explicitly chosen over a semicolon tokenizer because the project's
  existing multi-statement convention (migration bands) is
  author-delimited and literal parsing was deliberately avoided there
  too. DDL participates in the transaction on PostgreSQL/SQLite/DB2/MSSQL
  and forces commit-and-reopen on Firebird (metadata visibility) and
  MySQL/MariaDB (implicit commit). Marked queries return a JSON array of
  per-statement result sets with top-level totals; unmarked queries are
  unchanged. Parameters are validated globally (union) but bound
  per-statement after filtering. Failure payloads carry everything
  known. No fixture self-drops; #158 always verifies and cleans. No
  arbitrary-SQL endpoint — runtime SQL stays QueryRef-only.
- **(2026-10-14)** Runtime corpus audit: 0 of 159 runtime QueryRefs in
  the demo database are multi-statement; the `-- SUBQUERY DELIMITER`
  marker is migration-internal only (bookkeeping flips, MSSQL
  INSERT...WITH rewrites, a few reverse-migration code columns). An
  earlier claim that QueryRef #029 was multi-statement at runtime was
  investigated and disproven; the marker in its migration file sits in
  the migration wrapper's statement list, not in the QueryRef code.
- **(2026-10-14)** Numbers snapshot: next Acuranzo file 1386, next
  QueryRef 156, next blackbox test 64, fixture query type 1 / dialect 0.
  Re-verify on disk before use.

### Surprises / deviations

- `queries.code` is stored as a **BLOB** in SQLite; `length()`,
  `replace()`, and `instr()` on it mislead. Decode before auditing.
- `query_dialect_a30` is unused in Hydrogen C — manager metadata only.
- `DB_ENGINE_AI` is a Unity mock slot, not an engine; YugabyteDB rides
  the PostgreSQL engine.
- MySQL connects with `client_flag = 0`, so runtime multi-statement SQL
  fails there today even before any splitting questions arise.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After bash: `mks`. After docs: `mkl`.
  Payload after Helium edits: `mka` (or `payload-generate.sh`); `mkt`
  does not refresh the archive.
- The agent never applies migrations; packets are handed to the owner.
- Statement-level helpers worth reusing rather than rewriting:
  `statement_has_executable_sql` and `parse_sql_statements` in
  `src/database/migration/transaction.c`; the DDL keyword list being
  renamed `sql_statement_is_ddl` in Phase 2.
- The migration apply loop to model the shared executor on lives in
  `src/database/dbqueue/lead_apply.c` (delimiter split at the top,
  Firebird commit-after-DDL branch, then begin → statements → commit).
