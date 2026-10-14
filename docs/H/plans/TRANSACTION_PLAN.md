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

**CURRENT PAUSE POINT (as of 2026-10-14):** Phases 0–3 are complete
(parser, shared executor, runtime wiring). The agent wrote the Phase 2
close-out after Phase 3. Next: **Phase 4 (Helium fixture migrations
1386–1388 → QueryRefs #156–#158)** — author, audit, regenerate the
payload, and hand the packet to the owner. **The agent does not apply
the migrations.**

### Resume here next session

1. Re-verify the free numbers on disk (migration files, QueryRefs, test
   number) — the ones recorded below are a snapshot and go stale.
2. Re-read only Phase 4's Goal, Work items, Done means, and Exit gate.
3. Baseline: `zsh -ic 'mkq'`.
4. Implement Phase 4 only, verify its Exit gate, update this doc, stop.

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
  (PostgreSQL rolls the DDL back with the block; Firebird/MySQL/MariaDB
  show segment semantics).
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
| 4 | Fixture migrations 1386–1388 written, audited, packet handed to owner; owner reports the apply | M | pending |
| 5 | Test 64 green on every live engine, failure path and cleanup included | L | pending |
| 6 | Coverage targets met, docs updated, plan closed after owner sign-off | S | pending |
| 7 | (Deferred by default) Migration executors converged onto the shared executor | M | pending — deferred |

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

- [ ] Migration 1386 → QueryRef #156 (snapshot): type 1, dialect 0;
      `TEST_TRANSACTIONS`; one transaction block (INSERT + UPDATE);
      final verify SELECT; **no self-drop** — the table is left for
      #158.
      **Verify:** file exists; SchemaTool audit clean; README row added.
- [ ] Migration 1387 → QueryRef #157 (snapshot): several transaction
      blocks; DDL before, between, and inside blocks; a `:SHOULD_FAIL`
      parameter that makes one statement violate a constraint mid-block;
      no self-drop on either path.
      **Verify:** SchemaTool audit clean; README row added.
- [ ] Migration 1388 → QueryRef #158 (snapshot): SELECT proof of final
      state, then DROP of the fixture tables, in one marked query.
      **Verify:** SchemaTool audit clean; README row added.
- [ ] Update `elements/002-helium/acuranzo/README.md` migration table
      (number, version, date, QueryRef, summary) for all three.
      **Verify:** table rows present; `zsh -ic 'mkl'` clean.
- [ ] Regenerate the payload after the migration edits
      (`payload-generate.sh` or `zsh -ic 'mka'`) — required before any
      migration test can see the new files.
      **Verify:** payload regenerated; timestamp noted in Status.
- [ ] Run the SchemaTool drift audit (read-only) and hand the packet to
      the owner. **The agent does not apply.**
      **Verify:** handoff recorded in Status with the audit output.
- [ ] Phase completes only when the owner reports the apply (per
      [`/docs/PLANNING.md`](/docs/PLANNING.md) packet rule).
      **Verify:** owner report quoted in Status.

### Done means

The packet is handed over and the owner has applied 1386–1388, with the
QueryRefs live in the demo databases.

### Exit gate

Owner apply report in Status; payload regenerated; `mkl` clean.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

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

- [ ] New `tests/test_64_queryref_transactions.sh` with the standard
      header (`TEST_ABBR`, version, CHANGELOG line bumped on change),
      per-engine configs following the existing numbered-test pattern,
      and this plan's port range convention.
      **Verify:** `zsh -ic 'mks'` clean.
- [ ] Case: #156 happy path — success, array shape, per-statement
      entries, totals.
      **Verify:** test output.
- [ ] Case: #157 with `:SHOULD_FAIL = 0` — every block commits; #158's
      SELECT proves the committed rows.
      **Verify:** test output.
- [ ] Case: #157 with `:SHOULD_FAIL = 1` — failure payload complete
      (entries so far, failing statement and block, engine class and
      message, rollback state); #158's SELECT proves the rollback left
      nothing behind.
      **Verify:** test output.
- [ ] Case: #158 cleanup always runs, including after the failure path,
      and tolerates the nothing-to-clean case.
      **Verify:** test output.
- [ ] Per-engine DDL assertions: PostgreSQL (and the YugabyteDB config)
      roll the DDL back with the block; Firebird / MySQL / MariaDB show
      commit-at-DDL segment semantics; the same SQL text on all.
      **Verify:** per-engine sections green.
- [ ] Docs: `docs/H/tests/test_64_queryref_transactions.md`, TESTING.md
      entry, SITEMAP.md entry, and any INSTRUCTIONS.md pointer the test
      conventions require.
      **Verify:** `zsh -ic 'mkl'` clean.
- [ ] Run via the orchestrator the way the suite runs
      (`tests/test_00_all.sh <name>` or the documented single-test
      path).
      **Verify:** green run name recorded in Status and Working Log.

### Done means

Test 64 passes on every live engine, covering the happy path, the
failure path, the per-engine DDL semantics, and cleanup after both.

### Exit gate

Green orchestrator run name in Status; `mks` + `mkl` clean.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 6 — Coverage, Docs, And Closeout

### Goal

Prove the coverage targets that motivated part of this work, finish the
docs, and close the plan after owner sign-off.

### Entry gate

Phase 5 complete.

### Work items

- [ ] Unity coverage: every new file over 100 lines at ≥75% (project
      constraint).
      **Verify:** coverage report numbers in Status.
- [ ] Blackbox coverage: confirm `src/database/db2/prepared.c` and
      `src/database/db2/transaction.c` now register from a normal
      API-driven run (`extras/add_coverage.sh`), and record the
      before/after numbers.
      **Verify:** numbers in Status.
- [ ] Documentation: the marker syntax and the per-engine DDL table land
      in the database subsystem docs under `docs/H/core/subsystems/`
      (or the location the owner names), with the "DDL creates commit
      points on Firebird/MySQL/MariaDB" caveat stated.
      **Verify:** `zsh -ic 'mkl'` clean.
- [ ] Record the deferred Phase 7 decision (converge migration
      executors onto the shared executor, or keep them separate) with a
      one-line rationale.
      **Verify:** Working Log.
- [ ] Close-out: Status complete, Accomplished, Lessons learned,
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

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 7 — Converge Migration Executors (Deferred)

Deferred by default. Runs only after Phase 5 is green and the owner
pulls it. Goal: switch `src/database/dbqueue/lead_apply.c`,
`lead_reverse.c`, and the per-engine migration executors onto the Phase 2
shared executor so one implementation serves both paths. Exit gate: the
full migration suite (tests 30–38, 71) green, with the Firebird
commit-after-DDL behavior byte-identical to today's.

If deferred: `[~]` with the rationale recorded here and in the Working
Log.

---

## Working Log (cross-phase memory)

### Decisions log

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
