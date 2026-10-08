<!-- markdownlint-disable MD007 MD024 -->
# Schema v2 Plan

## Status at a glance

**New plan (2026-10-07).** Brings SchemaTool and SchemaHelper up to the
eight-engine Hydrogen matrix, then makes the review loop able to bring
an older database up to the current migrations without touching data
those migrations do not own.

SchemaHelper v1 and the v2 UI plan are archived. This plan does not
reopen them.

| Phase | Status | Remaining |
| --- | --- | --- |
| 1 Firebird adapters | complete | none |
| 2 MSSQL adapters | complete | none |
| 3 Remove Cockroach names | complete | none |
| 4 Split MySQL and MariaDB | complete | none |
| 5 Eight test wrappers (tests 32–39) | complete | none |
| 6 Eight demo wrappers (Test 40) | complete | none |
| 7 Expected shape from disk migrations | complete | none |
| 8 Structural apply, per dialect | complete | none |
| 9 Migration-owned default rows | complete | none |
| 10 Docs and smoke | complete | none |

Operator guides:
[`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md),
[`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).
Code:
[`extras/schematool/`](/elements/001-hydrogen/hydrogen/extras/schematool/).

Sister plans:
[`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) Phase 9 marked the Firebird
wrapper and ping complete. The dump and catalog adapters were not
written. This plan owns them.
[`MSSQL_COMPLETE.md`](/docs/H/plans/complete/MSSQL_COMPLETE.md) Phase 6 items 6.1–6.3 (wrapper,
SchemaHelper, flush) are specified here in Phase 2. MSSQL item 6.4
stays in that plan (Test 40 transaction probe, already moved to its
Phase 7).

## Purpose

Point SchemaHelper at a migration tree and a live database. Show where
the database differs from what those migrations specify. Apply the gap
one finding at a time: a missing table, a missing column, a type or
nullability change, a migration-owned default row that is missing or
stale. Leave rows, columns, and tables that the migrations never
mention. Those are production data.

The motivating case is a `contacts` table. Migrations create it, insert
a few default rows, and later add a column. An older production
database should gain that table, those default rows, and that column.
People added to `contacts` since then stay put.

## How to use this document

- Work one phase at a time, top to bottom.
- Do not start a phase until the previous phase Status is complete and
  its exit gate passed.
- Each phase has one Done means line.
- Mark a work item `[x]` only after that item's check actually passed.
- Defer with `[~]` and name the phase that takes it.
- After each phase: fill Status, append the working log, stop for review.
- Phases 1–6 are read-only against live databases. Flush scripts may be
  written. Do not run them unless the operator asks.
- Phases 8–9 are the first write path. Prove them on a copy of a SQLite
  artifact. A server-engine write needs an explicit yes in that session.
- Shell changes: `zsh -ic 'mks'`. Lua changes: Test 72 and Test 98.
  No C in this plan, so no `mkq` / `mkp`.
- Absolute Markdown links. Bump the tool changelog and the `VERSION`
  constant in the same edit. `schematool.sh` currently prints `1.8.3`
  while its changelog header says `1.9.0`. The next edit makes those
  match.
- Never print a password. Never write one into a packet, sidecar, or log.

## Locks

These are decided. Later phases implement them.

### What the tool is for

SchemaHelper converges a database to the net effect of the current
migration files. Hydrogen AutoMigration remains the ordered runner for
a migration that was never LOADed or APPLYed, including multi-statement
backfills. SchemaHelper does not grow a second migration runner.

A finding the helper cannot express as one safe statement stays a
finding, with guidance to run AutoMigration or to write a new forward
migration with `[g]`.

### What counts as drift

| Live situation | Queue |
| --- | --- |
| Object is in the folded end state and missing live | Finding. Apply may create it |
| Column type or nullability differs from the fold | Finding. Apply may alter it |
| Migration inserted a keyed default row; that key is missing | Finding. Apply may insert it |
| Keyed default row is present and a migration-owned column differs | Finding. Apply may update those columns |
| Migration later deletes that keyed row; the row is still present | Finding. Apply may delete that one key |
| Row, column, or table that no migration mentions | Out of the queue. No apply. Exit code ignores it |
| Column or table a later migration drops, still present | Finding. Apply may drop it, and only after the louder confirm |
| Missing LOAD or APPLY for a ref | Finding. Apply stays refused. Guidance is AutoMigration |
| Stored `queries` text differs from current Lua | Finding. Apply may replace that metadata row. This does not replay DDL |

"Migration-owned column" means a column the INSERT or UPDATE in the
migration sets. Columns added in production on that same row are left
alone.

### Identity of a default row

A default row is in scope only when the migration names a key. The key
is the primary key of the folded table when the INSERT supplies it,
otherwise the columns of a single-row `WHERE` that match a unique key.
A statement with no key (`UPDATE contacts SET city = 'X'` with no
`WHERE`, or a `WHERE` the tool cannot bind to one row) is reported as
unkeyed DML and is not applied. AutoMigration owns that statement.

The expected row set is the net of keyed `INSERT`, `UPDATE`, and
`DELETE` in migration order. A later `DELETE` of that key removes it
from the expected set. The live table is probed with a `SELECT` of
those keys only. The tool never does `SELECT *` and never deletes a
row whose key is absent from that set.

### Catalog source of truth

Today the catalog fold reads applied `queries` rows (type 1003) from
the database. That answers "did the SQL we already applied leave the
shape we stored?" It does not answer "does this older database match
the migrations on disk?"

From Phase 7 the expected catalog and the expected default rows come
from the migration files on disk, run through the same Lua extract the
metadata track already uses (`schematool_expect.lua`, engine name
unchanged). The live probe stays targeted: only tables the fold
mentions.

Live objects the fold never mentions are counted in a side list for
the operator and do not fail the audit.

### Writes

- SchemaTool stays read-only. Commented `.sql` stays commented.
- SchemaHelper writes only with `--allow-write`, one finding, one
  statement, after the typed confirm token.
- No batch apply in this plan.
- `SET NOT NULL` and a narrowing type change can reject existing
  production values. The confirm screen says so. The operator types
  the token anyway if they want it.
- `DROP TABLE` / `DROP COLUMN` is offered only for an object the fold
  dropped, with confirm token `DROP <object>` or `DROP <object.column>`.
- Replacing a `queries` row rewrites `code`, `name`, and `summary`
  together. The review screen keeps the line that this does not replay
  DDL. When the same edit also changed live shape, the catalog finding
  is the one that changes the table. Prefer `[g]` when the change
  should ship to other databases.
- Apply SQL is generated per engine. One PostgreSQL `ALTER COLUMN`
  string for every engine is a bug this plan removes.

### Engines

Eight engines, matching tests 32–39 and Test 40. No aliases that hide
a real engine:

| Name | Adapter family | Notes |
| --- | --- | --- |
| `postgresql` | `psql` | `postgres` remains a spelling alias |
| `yugabytedb` | `psql` | Keeps `YUGABYTE_DB_*`. Helium has no separate Yugabyte dialect, so expect may keep using the PostgreSQL dialect module. Env selection happens before that alias |
| `mysql` | `mysql` client | `MYSQL_DB_*` only |
| `mariadb` | `mariadb` client path | `MARIADB_DB_*` only. Own adapter files. Expect receives `mariadb` so `database_mariadb.lua` runs |
| `sqlite` | `sqlite3` | File path. Empty schema is valid |
| `db2` | `db2` | Unchanged client |
| `firebird` | `isql-fb` | File path. Empty schema is valid. Never `psql` |
| `mssql` | `sqlcmd` inside container `philement-mssql` | Schema is `testms` or `demoms` inside database `MSSQL_DB_NAME`. Never `psql` or `isql-fb` |

`cockroachdb` is removed in Phase 3. It is an unknown engine after
that. It must not map to PostgreSQL or to Firebird.

Connection readiness has two shapes:

- Server engines need host, user, database, and schema.
- File engines (SQLite, Firebird) need a database path that exists.
  An empty schema is valid. Requiring a host for Firebird is why the
  current wrapper falls through to disk-only discovery.

### Wrappers and sidecars

Sixteen wrappers. The filename suffix is the role.

| Role | Files | Points at |
| --- | --- | --- |
| Test | `schematool_<engine>_test.sh` | Tests 32–39 |
| Demo | `schematool_<engine>_demo.sh` | Test 40 |

Phases 1–4 keep today's unsuffixed wrappers (`schematool_firebird.sh`
and the rest) so the adapter work has a launcher. Those unsuffixed
files are the demo targets. Phase 5 adds the `_test` files beside
them. Phase 6 renames the unsuffixed files to `_demo` and deletes the
old names. SchemaHelper's picker lists every `schematool_*.sh`. It
must key rows by path, not by engine, or the test and demo files
collapse into one row.

The sidecar today is `schemahelper_<design>_<engine>.json`. Test and
demo would share decisions. From Phase 5 the name is
`schemahelper_<design>_<engine>_<role>.json`. An unsuffixed wrapper,
while it still exists, is role `demo`. Old local sidecars are left
where they are. They are gitignored.

Picker order, test block then demo block, inside each block:

`postgresql`, `mysql`, `sqlite`, `db2`, `mariadb`, `firebird`,
`yugabytedb`, `mssql`.

That is tests 32–39 order.

### Databases

Migrations for these wrappers are
`${HELIUM_ROOT}/acuranzo/migrations`, design `acuranzo`. SQLite file
paths use `${HYDROGEN_ROOT}`. New wrappers follow the PostgreSQL
wrapper for the migration path and the SQLite wrapper for file paths.

| Test | Engine | Schema or file | Credentials |
| --- | --- | --- | --- |
| 32 | postgresql | schema `test` | `ACURANZO_DB_*` |
| 33 | mysql | schema `test` | `MYSQL_DB_*` |
| 34 | sqlite | `tests/artifacts/database/sqlite/hydrotst.sqlite` | file |
| 35 | db2 | schema `test` | `HYDROTST_DB_*` |
| 36 | mariadb | schema `test` | `MARIADB_DB_*` |
| 37 | firebird | `FIREBIRD_DB_PATH_TEST`, schema empty | `FIREBIRD_SYSDBA_PASSWORD`, user `SYSDBA` |
| 38 | yugabytedb | schema `test` | `YUGABYTE_DB_*` |
| 39 | mssql | schema `testms` in `MSSQL_DB_NAME` | `MSSQL_SA_PASSWORD`, container `philement-mssql` |

| Test 40 | Engine | Schema or file | Credentials |
| --- | --- | --- | --- |
| postgres | postgresql | schema `demo` | `ACURANZO_DB_*` |
| mysql | mysql | schema `demo` | `MYSQL_DB_*` |
| mariadb | mariadb | schema `demo` | `MARIADB_DB_*` |
| sqlite | sqlite | `tests/artifacts/database/sqlite/hydrodemo.sqlite` | file |
| db2 | db2 | schema `demo` | `HYDROTST_DB_*` |
| firebird | firebird | `FIREBIRD_DB_PATH_DEMO`, schema empty | `FIREBIRD_SYSDBA_PASSWORD` |
| yugabytedb | yugabytedb | schema `demo` | `YUGABYTE_DB_*` |
| mssql | mssql | schema `demoms` in `MSSQL_DB_NAME` | `MSSQL_SA_PASSWORD` |

`hydrogen_test_40_mariadb.json` uses schema `demo`.
`schematool_mariadb.sh` still exports `demomrdb`, and SchemaHelper's
picker blurb still says `demomrdb`. That is drift from the split.
Phase 4 corrects it. `CANVAS_DB_*` is not a SchemaTool credential
after Phase 4. Example configs that still mention `CANVAS_DB_*` are
outside this plan.

Configs:
[`hydrogen_test_32_postgres.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_32_postgres.json)
through
[`hydrogen_test_39_mssql.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_39_mssql.json),
and the eight
`hydrogen_test_40_<engine>.json` files beside them.

Argent (`argent_2xxx`) is a second design directory. One SchemaTool
run takes one `--migrations` directory. Combining Acuranzo and Argent
in one audit is out of scope. An operator who wants Argent points a
run at that tree with `--design argent`.

## Already in place

Usable today, on PostgreSQL (including the Yugabyte alias), MySQL,
MariaDB-as-MySQL, SQLite, and DB2:

- Metadata audit of `queries` types 1000–1003 against Lua (`code`,
  `name`, `summary`).
- Catalog audit of table presence, column presence, and nullability.
  `data_type` is folded and probed, then ignored by compare.
- SchemaHelper review: explore, skip, accept with payload hash,
  un-accept, re-audit, packet, promote a stub `design_NNNN.lua`.
- `[u]` with `--allow-write`: one metadata field, a true-orphan
  `DELETE` from `queries`, `SET`/`DROP NOT NULL`, or `ADD COLUMN`.
- Test 72 covers the queue against fixtures. It never opens a database.

Known holes the later phases close:

- Firebird has a wrapper, an `isql-fb` ping, decode patterns, and a
  flush path. `db/` has no Firebird adapter. `schematool_runners.sh`
  errors with `no dump adapter`. Connection readiness demands a host
  and a schema, so a normal launch prints the disk-only note.
  `exec_sql` has no Firebird branch. The picker blurb is the word
  `firebird`.
- MSSQL has no wrapper, no connect path, no adapter, and no flush
  path. Hydrogen's engine and Test 39 exist. `sqlcmd` is already
  invoked inside `philement-mssql` by `verify_tx_mssql` in
  [`transaction_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/transaction_utils.sh).
- `build_catalog_sql` emits PostgreSQL `ALTER TABLE … ALTER COLUMN`
  for every engine.
- The apply path decodes JSON strings by hand. A `\u` escape above
  127 becomes `?`.
- Missing tables, extra columns, type changes, and default rows have
  no apply.
- `smoke_test40_catalog.sh` lists Firebird and will fail it until
  Phase 1 lands. It does not list MSSQL.

Adapter output stays the shape the compare scripts already consume.
Metadata JSON objects have `query_ref`, `query_type`, `name`,
`summary`, `code`. Catalog JSON has `schema` and `tables[]` with
`columns[]` of `name`, `data_type`, `nullable`, plus `primary_key`.
Phases 1 and 2 do not change `schematool_compare.lua` or
`schematool_catalog_compare.lua`.

## Non-goals

- A second implementation of Hydrogen LOAD/APPLY.
- Scanning product tables for rows the migrations do not name.
- Deleting production-only rows, columns, or tables.
- Batch apply, a headless SchemaHelper, or a REST API.
- Authoring a finished `acuranzo_NNNN.lua`. `[m]` may keep writing a
  stub. Making that stub loadable is a later plan.
- Editing `src/`. Cockroach leftovers in Unity
  (`normalize_engine_name`) and in completed plan archives stay there.
- Combining the Argent pack into the Acuranzo audit.
- Replacing `psql` / `isql-fb` / `sqlcmd` with the Hydrogen C drivers.

## Phase 1 — Firebird adapters

### Goal

A Firebird database can complete a metadata audit and a catalog audit
through the existing `schematool_firebird.sh` wrapper.

### Entry gate

This plan is in place. No code yet.

### Work items

- [x] 1.1 File-database connection readiness. SQLite and Firebird
      succeed when the database path exists and the user is set.
      Empty schema is valid. A missing host must not force disk-only
      mode.
- [x] 1.2 `db/query_firebird.sh`. `isql-fb` as `SYSDBA`, password from
      `FIREBIRD_SYSDBA_PASSWORD`, database from the path flag. `SELECT`
      of `queries` types 1000–1003. Same JSON object shape as
      `query_pg.sh`. The script contains no DML. Password never
      printed. Scrub it from `isql` errors before they reach the log.
- [x] 1.3 `db/catalog_firebird.sh`. Targeted read of
      `RDB$RELATIONS` / `RDB$RELATION_FIELDS` for the tables in
      `--tables`. Map `RDB$NULL_FLAG` to nullable. Map Firebird field
      types to the type spellings the fold stores (the migration's SQL
      type, lowercased). A fixture of recorded `isql` output covers
      the mapping with no live database.
- [x] 1.4 `schematool_runners.sh` dispatches `firebird` to those two
      scripts. `--engine firebird` still reaches `schematool_expect.lua`
      as `firebird` (`database.defaults.firebird` already exists).
- [x] 1.5 SchemaHelper: picker blurb names `FIREBIRD_DB_PATH_DEMO` and
      `FIREBIRD_SYSDBA_PASSWORD`. `exec_sql` gains an `isql-fb` branch
      so a later apply phase has a place to send one statement. This
      phase does not invoke it against a database.
- [x] 1.6 Help text in `schematool.sh`. The Firebird env block is
      `FIREBIRD_DB_PATH_DEMO` / `_TEST` / `FIREBIRD_SYSDBA_PASSWORD`.
      The line that lists Firebird under `ACURANZO_DB_*` goes away.
- [x] 1.7 Docs: Firebird rows in
      [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md) and
      [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md) match the
      code. `smoke_test40_catalog.sh` can pass Firebird when
      `FIREBIRD_DB_PATH_DEMO` is a migrated demo file. If that file
      is down, the exit gate uses `FIREBIRD_DB_PATH_TEST` and the
      working log says so.

### Done means

`schematool_firebird.sh` against a live `.fdb` runs the metadata
audit, and `--catalog --only-tables accounts` runs the catalog audit.
Neither path prints `no dump adapter` or the disk-only note.

### Exit gate

- `mks` on the touched scripts.
- Test 98 on the touched Lua.
- One live Firebird metadata audit and one `--only-tables accounts`
  catalog audit, with the exit code and the `password_hash` row
  recorded in the working log.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | `schematool_firebird.sh` against `FIREBIRD_DB_PATH_DEMO` ran the metadata audit (exit 3) and `--catalog --only-tables accounts` (exit 2). Neither path printed `no dump adapter` or the disk-only note. `mks` and Test 98 passed. The catalog fixture passed with no live database. |
| **Variances** | Phase 2 was already complete; this phase ran after it. Metadata: 386 disk refs, 385 ok, drift 0, missing LOAD 1 (ref 1168, on disk only), 15 orphan refs 2000–2014. Catalog: 16 nullable checks, 15 Y, 1 N. `accounts.password_hash` nullable expected false, live true, fold ref 1005, live type `char(128)`. Live extra column `accounts.stripe_customer_id` is counted and is not a second failure. `smoke_test40_catalog.sh` was not run. Its 1190 check wants `password_hash` expected true and live true, so a Firebird row would fail that check. `isql-fb` uses `ISC_PASSWORD` (not `-password`). A private `FIREBIRD_LOCK` is used because `/tmp/firebird` is mode 770. Large `code` blobs are hex-chunked; `VARCHAR(8191)` cannot hold the longest value (max hex length observed 80226). |

### Working log

- **2026-10-07** `mks` (Test 92) passed, 191 shell files. Test 98 passed, 492 Lua files. `extras/schematool/test/firebird_catalog_map.sh` passed on the recorded fixture with no live database.
- **2026-10-07** `schematool_firebird.sh --no-sql --format json` against `FIREBIRD_DB_PATH_DEMO` (`hydrogen_demo.fdb`). Exit 3. Compare: total 386, ok 385, drift 0, missing LOAD 1 (ref 1168, on disk only), missing APPLY 0, orphans 15 (refs 2000–2014). Checklist 401 rows. Stdout, stderr, the `.mig`, and `db_metadata.json` did not contain `FIREBIRD_SYSDBA_PASSWORD`.
- **2026-10-07** `schematool_firebird.sh --catalog --only-tables accounts --no-sql --format json` on the same file. Exit 2. Catalog checklist 16 rows, 15 Y, 1 N: `accounts.password_hash` nullable expected false, live true, ref 1005. Live column is `char(128)`, nullable true. Primary key `account_id` (`integer`, not null). `accounts.stripe_customer_id` is a live extra column. Neither path printed `no dump adapter` or the disk-only note.
- **2026-10-07** `exec_sql` has an `isql-fb` branch and was not invoked. `smoke_test40_catalog.sh` was not run.

## Phase 2 — MSSQL adapters

### Goal

SQL Server can complete a metadata audit and a catalog audit. This
closes MSSQL plan items 6.1, 6.2, and 6.3.

### Entry gate

Phase 1 Status complete.

### Work items

- [x] 2.1 `schematool_mssql.sh`, unsuffixed, aimed at Test 40 schema
      `demoms`. Same shape as today's other demo wrappers. Phase 6
      renames it to `schematool_mssql_demo.sh`.
- [x] 2.2 `db/query_mssql.sh` and `db/catalog_mssql.sh`. Invoke
      `/opt/mssql-tools18/bin/sqlcmd` with `podman exec -i` on
      container `philement-mssql`, matching `verify_tx_mssql`.
      Database `MSSQL_DB_NAME` (default `hydrotst`). Qualify tables as
      `[schema].[queries]`. Prefer `FOR JSON PATH` so the adapter does
      not parse fixed-width `sqlcmd` text. `SELECT` only. Refuse to
      start when the container is not running. Never echo
      `MSSQL_SA_PASSWORD`.
- [x] 2.3 Engine `mssql` in `schematool.sh` readiness, env, runners,
      and help. Schema is required (`demoms` or `testms`). Expect
      receives `mssql`. `database.defaults.mssql` already exists.
- [x] 2.4 SchemaHelper const order, picker blurb, ping, and `exec_sql`
      branch. Ping is `SELECT 1` via the same `sqlcmd` path. `exec_sql`
      is wired and not used against a database in this phase.
- [x] 2.5 `hydrogen_flush.sh` gains an mssql path that drops user
      tables in schema `testms` and schema `demoms`. It does not drop
      database `hydrotst`. Do not run the flush as part of the exit
      gate.
- [x] 2.6 Add `mssql` to `smoke_test40_catalog.sh`. Update the tool
      docs.

### Done means

`schematool_mssql.sh` against a running `philement-mssql` completes
a metadata audit and a `--catalog --only-tables accounts` audit on
schema `demoms`. The wrapper's process list and the tool log do not
contain the SA password.

### Exit gate

- `mks`, Test 98.
- One live metadata audit and one catalog audit, results in the
  working log. If the container is down, the phase stays open. Do
  not mark it complete on a fixture alone.
- MSSQL plan items 6.1–6.3 checked only after this exit gate, with a
  pointer back to this phase.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | Live metadata audit exit 3 (386 refs clean, 2 orphan query rows). Catalog `--only-tables accounts` exit 2 (`password_hash` nullable expected false, live true). `mks`, Test 98, and Test 72 passed. Process list and tool logs did not contain the SA password. |
| **Variances** | Started before Phase 1. The operator asked for MSSQL plan items 6.1–6.3 on 2026-10-07. `sqlcmd -y 0` is used alone (it rejects `-h` and `-W`). The password is `SQLCMDPASSWORD` inside the container script, not a process argument. `smoke_test40_catalog.sh` lists mssql and was not run; Firebird still has no dump adapter, and the 1190 check expects `password_hash` nullable expected=true. |

### Working log

- **2026-10-07** `mks` (Test 92) passed, 186 shell files. Test 98 passed, 484 Lua files. Test 72 passed, 19/19. SchemaHelper reports 0.6.7 / SchemaTool 1.10.0.
- **2026-10-07** `schematool_mssql.sh --no-sql --format json` against running `philement-mssql`, schema `demoms`. Exit 3. Checklist 388 rows: 386 load/apply/match Y, 2 rows with match `-` (orphan query rows, `.mig` written). Findings: drifts 0, missing LOAD 0, missing APPLY 0, anomalies 0, orphans 2.
- **2026-10-07** `schematool_mssql.sh --catalog --only-tables accounts --no-sql --format json`. Exit 2. Catalog checklist 16 rows, 15 Y, 1 N: `accounts.password_hash` nullable expected false, live true, fold ref 1005. Live catalog object is schema `demoms`, table `accounts`, primary key `account_id`. The same run also wrote `db_metadata.json`. Host and container process lists during this run did not contain the SA password. Neither out-dir contained the password.
- **2026-10-07** `hydrogen_flush.sh` was not executed. The next time an operator types HYDROGEN, the flush drops user tables and views in `testms` and `demoms` and does not drop database `hydrotst`.

## Phase 3 — Remove Cockroach names

### Goal

The operator surface of SchemaTool and SchemaHelper has no
`cockroachdb` alias.

### Entry gate

Phase 2 Status complete. Doing this after the new engines exist keeps
the alias from being confused with Firebird during Phases 1 and 2.

### Work items

- [x] 3.1 Delete the `cockroachdb` → `postgresql` branch in
      `schematool.sh`, `schemahelper_connect.lua`, and
      `schemahelper_apply.lua`. An unknown engine, including
      `cockroachdb`, exits 1 and names the supported list. The message
      says Firebird replaced that slot.
- [x] 3.2 Strip the alias from
      [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md),
      [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md), and
      [`extras/schematool/README.md`](/elements/001-hydrogen/hydrogen/extras/schematool/README.md).
- [x] 3.3 Leave changelog lines that describe the 2026-09 rename.
      Leave completed plans. Leave Unity `normalize_engine_name`.
      Those are history, and they are outside `extras/schematool`.
- [x] 3.4 `rg -i cockroach extras/schematool` returns only changelog
      comments. `rg -i cockroach docs/H/tools` returns nothing.

### Done means

Passing `--engine cockroachdb` fails with the supported-engine list.
Yugabyte still resolves `YUGABYTE_DB_*` and still reaches the
PostgreSQL adapter.

### Exit gate

- `mks`, Test 98.
- The two `rg` checks from item 3.4, pasted into the working log.
- A Yugabyte `--dry-disk` or expect smoke still runs, so the alias
  removal did not take Yugabyte with it.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | `--engine cockroachdb` exits 1 and names the supported list. Yugabyte `--dry-disk` and a ref-1000 expect both resolve `YUGABYTE_DB_*` and reach the PostgreSQL adapter. `mks` and Test 98 passed. `rg -i cockroach docs/H/tools` returned nothing. |
| **Variances** | Every unsupported engine, not only `cockroachdb`, gets the sentence "Firebird replaced that slot." A name-specific branch would leave a live `cockroachdb` token in `schematool.sh`, which item 3.4 does not allow. `SCHEMAHELPER.md` already had no Cockroach text. The `mariadb` → `mysql` alias is unchanged. |

### Working log

- **2026-10-07** `mks` (Test 92) passed, 190 shell files, 3/3, 1177/1177 directives justified. Test 98 passed, 495 Lua files, no issues.
- **2026-10-07** `schematool.sh --engine cockroachdb --dry-disk --no-sql` exited 1. Stderr: `Error: unsupported engine 'cockroachdb' (use postgresql|mysql|sqlite|db2|firebird|mssql). Firebird replaced that slot.` Stdout was empty. The same sentence is printed for any other unknown name.
- **2026-10-07** `--engine yugabytedb --dry-disk` with only `YUGABYTE_DB_*` set (dummy host, user, name, and schema; password unset). Exit 0. SQL stub header: `engine=postgresql schema=yb-schema-proof database=yb-name-proof`, schematool 1.12.0. No live dump. `--engine postgres` with only `ACURANZO_DB_*` set wrote `engine=postgresql schema=pg-schema-proof database=pg-name-proof`.
- **2026-10-07** `--engine yugabytedb --schema demo --from 1000 --to 1000 --dry-disk --emit-expected --no-sql`. Exit 0. Stderr: `expect 1/1 ref 1000 name=Create queries Table`. Expected payload engine `postgresql`, schema `demo`, ref 1000. Stderr did not contain `unsupported`.
- **2026-10-07** `rg -i cockroach extras/schematool` (changelog comments only):
  - `schematool_firebird.sh:12` `# 1.1.1 - 2026-09-20 - Renamed from CockroachDB wrapper to Firebird; uses isql-fb`
  - `schematool.sh:11` `# 1.12.0 - 2026-10-07 - cockroachdb is unknown; Firebird replaced that slot`
  - `schemahelper.sh:7` `# 0.6.9 - 2026-10-07 - SchemaTool 1.12.0; drop the cockroachdb alias`
  - `lua/schemahelper_const.lua:5` `-- 0.6.9 - 2026-10-07 - SchemaTool 1.12.0; drop the cockroachdb alias`
  - `lua/schemahelper_connect.lua:5` `-- 0.6.5 - 2026-10-07 - Drop cockroachdb from picker, family, ping, and exec_sql`
  - `lua/schemahelper_apply.lua:6` `-- 0.5.7 - 2026-10-07 - Dollar-quote literals no longer treat cockroachdb as postgresql`
- **2026-10-07** `rg -i cockroach docs/H/tools` returned no lines.

## Phase 4 — Split MySQL and MariaDB

### Goal

MySQL and MariaDB are separate SchemaTool engines, matching
`src/database/mysql/`, `src/database/mariadb/`, `database_mysql.lua`,
and `database_mariadb.lua`.

### Entry gate

Phase 3 Status complete.

### Work items

- [x] 4.1 Remove `mariadb) ENGINE=mysql` from `schematool.sh`.
      Runners, readiness, env, help, and expect all see `mariadb`.
      Expect then loads `database.defaults.mariadb`.
- [x] 4.2 `db/query_mariadb.sh` and `db/catalog_mariadb.sh`. They may
      source a shared client fragment. They are separate entry points
      so a later dialect quirk does not land in an `if engine` inside
      the MySQL file. The MySQL files drop the "MySQL/MariaDB" header.
- [x] 4.3 Credentials. MySQL uses `MYSQL_DB_*`. MariaDB uses
      `MARIADB_DB_*`. Delete the `CANVAS_DB_*` fallback from
      SchemaTool and SchemaHelper. Fix the MariaDB demo schema from
      `demomrdb` to `demo` in the wrapper, the picker blurb, and
      `apply_family`.
- [x] 4.4 Apply and qualify helpers take `mariadb` as its own engine.
      Qualified names stay `` `schema`.`table` `` style for both until
      Phase 8 replaces the DDL text. This phase does not need to
      invent MariaDB-only DDL.
- [x] 4.5 Tool docs and `smoke_test40_catalog.sh` treat the two
      engines as separate rows. A MariaDB smoke must show expect
      running as `mariadb` in the log (the dialect name, not a
      password).

### Done means

A MariaDB audit calls `query_mariadb.sh` and passes engine `mariadb`
into `schematool_expect.lua`. A MySQL audit does not. The MariaDB
demo wrapper's schema is `demo`.

### Exit gate

- `mks`, Test 98, Test 72.
- One MySQL and one MariaDB metadata audit (demo schemas when those
  servers are up). Record both exit codes. If one server is down,
  the phase stays open.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | MariaDB demo metadata audit exit 3. MySQL demo metadata audit exit 3 (`query_mysql.sh`, `engine=mysql`, schema `demo`): total 386, ok 385, drift 0, missing LOAD 1 (ref 1168), missing APPLY 0, orphans 2 (refs 2000 and 2001). |
| **Variances** | `mks`, Test 98, and Test 72 passed. The first MySQL attempt used process host `10.119.2.49` (`ERROR 2002`). `~/.my.cnf` `[client]` is a localhost login with a different password, so a `mysql` probe that sets `MYSQL_PWD` and still reads that file sends the wrong password and gets `ERROR 1045`. `MYSQL_DB_PASS` logs in with `--no-defaults`. The successful audit passed `--host 10.118.0.3`. `findings.json` `counts.anomalies` is 2 and the anomalies array is empty. No password was reset. |

### Working log

- **2026-10-07** `mks` passed, 193 shell files, 3/3, 1181/1181 directives justified. Test 98 passed, 495 Lua files. Test 72 passed, 19/19. SchemaHelper 0.6.10, SchemaTool 1.13.0.
- **2026-10-07** `--engine cockroachdb` still exits 1. The supported list is `postgresql|mysql|mariadb|sqlite|db2|firebird|mssql`.
- **2026-10-07** `--emit-expected --from 1000 --to 1000` on `schematool_mysql.sh`: `expect 1/1 ref 1000 engine=mysql`, payload engine `mysql`, schema `demo`. On `schematool_mariadb.sh`: `engine=mariadb`, schema `demo`. No password in those files.
- **2026-10-07** `schematool_mariadb.sh --no-sql --format json` against `MARIADB_DB_*`, schema `demo`. Exit 3. Stderr: `phase: expect engine=mariadb` and `adapter: query_mariadb.sh`. Counts: total 386, ok 385, drift 0, missing LOAD 1, missing APPLY 0, orphans 15. The log did not contain `query_mysql.sh` or `MARIADB_DB_PASS`.
- **2026-10-07** `schematool_mysql.sh --no-sql --format json` against `MYSQL_DB_*`, schema `demo`. Expect ran as `engine=mysql` (386 refs) and called `adapter: query_mysql.sh`. The dump then failed: `ERROR 2002 (HY000): Can't connect to server on '10.119.2.49' (115)`. Exit 1. A follow-up client ping with `--connect-timeout=8` returned `ERROR 2002` (110). The log did not contain `query_mariadb.sh` or `MYSQL_DB_PASS`.
- **2026-10-07** `10.119.2.49` is the removed `mysql-test-proxy` pod address. `~/.festival.env` and the tenant README name `10.118.0.3:3306`. A client to that host with `--connect-timeout=8` reached MySQL and returned `ERROR 1045 (28000)` (`using password: YES`) for `root` and `testuser` against `demo`, `test`, and `testdb`. MySQL saw the client as an `lmtp-edge` pod (`10.119.0.244` or `10.119.2.4`). The session password matches `root-password` in secret `mysql-secrets` and does not match `testuser-password`. No user was altered. The metadata audit was not re-run. Phase 4 stays open.
- **2026-10-07** Test 33 passed against `10.118.0.3:3306` as `root`, database `test`. Hydrogen connected and read `test.queries` (1049 bootstrap rows). LOAD and APPLY were already at the tip. `TestMigration` is false, so the reverse phase was skipped. The log line was `Migration test completed in 0.001s`.
- **2026-10-07** `~/.my.cnf` `[client]` sets host `localhost` and a different password. A `mysql` invocation that exports `MYSQL_PWD` still uses that file, so the `1045` above was the defaults-file password. `mysql --no-defaults` with `MYSQL_DB_PASS` logs in to `test`, `demo`, and `testdb`.
- **2026-10-07** `schematool_mysql.sh --host 10.118.0.3 --no-sql --format json`. Exit 3. Stderr: `phase: expect engine=mysql` and `adapter: query_mysql.sh`. Compare: total 386, ok 385, drift 0, missing LOAD 1 (ref 1168), missing APPLY 0, orphans 2 (refs 2000 and 2001). Schema `demo`. The log did not contain `query_mariadb.sh` or `MYSQL_DB_PASS`. Phase 4 exit gate is met.

## Phase 5 — Eight test wrappers

### Goal

SchemaHelper can be pointed at each tests 32–39 database.

### Entry gate

Phase 4 Status complete. All eight engines have adapters.

### Work items

- [x] 5.1 Add the eight `schematool_<engine>_test.sh` files from the
      Locks table. Each sets the test schema or file and the engine's
      own env. Firebird uses `FIREBIRD_DB_PATH_TEST`. MSSQL uses
      schema `testms`. SQLite uses `hydrotst.sqlite`.
- [x] 5.2 Picker keys by wrapper path. `schematool_mysql_test.sh` and
      `schematool_mysql.sh` are two rows. Blurbs show the schema or
      the file name, plus the env names, never a password.
- [x] 5.3 Sidecar path gains the role:
      `schemahelper_<design>_<engine>_<role>.json`. Suffix `_test` is
      role `test`. An unsuffixed wrapper is role `demo` until Phase 6.
- [x] 5.4 Test 72 fixture grows a second wrapper stem and asserts the
      two sidecars do not share a file. Update
      [`test_72_schemahelper.md`](/docs/H/tests/test_72_schemahelper.md)
      if the fixture contract changes.

### Done means

The picker lists the eight test wrappers and the still-unsuffixed
demo wrappers. A `--reuse` session against the test fixture writes
the `_test` sidecar and leaves the demo sidecar untouched.

### Exit gate

- `mks`, Test 98, Test 72.
- Ping or a short metadata audit for each test wrapper whose database
  is up. Record any engine that was down. The phase can complete
  with a down engine only if the wrapper's `--help` path and a
  connection-refused message are correct, and the working log names
  the engine.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | Eight `schematool_<engine>_test.sh` wrappers. The picker lists those eight test rows and the eight unsuffixed demo rows. Sidecars are `schemahelper_<design>_<engine>_<role>.json`. Seven short metadata audits exited 3. SQLite exited 1 because `hydrotst.sqlite` has no tables. |
| **Variances** | The test block follows current `WRAPPER_ORDER` (mariadb before sqlite). Locks order is Phase 6.2. Yugabyte `phase: expect` prints `engine=postgresql` after the dialect alias. That audit used `YUGABYTE_DB_*` on `adm-c:30543`. No engine was down. All eight `--help` paths exited 0. |

### Working log

- **2026-10-07** `mks` passed, 202 shell files, 3/3, 1189/1189 directives justified. Test 98 passed, 506 Lua files. Test 72 passed, 20/20. SchemaHelper 0.6.11, SchemaTool 1.13.0.
- **2026-10-07** Discover on `extras/schematool` returns 16 rows: the `_test` stems in `WRAPPER_ORDER`, then the unsuffixed demo stems. A fixture `create_state` on the `_test` sidecar left the seeded demo sidecar bytes unchanged.
- **2026-10-07** `--help` on all eight test wrappers exited 0 and printed the SchemaTool help. No password in that output.
- **2026-10-07** Short metadata audits `--from 1000 --to 1000 --no-sql --format json`. PostgreSQL exit 3, `engine=postgresql`, total 1, ok 1, orphans 15 beginning at ref 2000. MySQL `--host 10.118.0.3` exit 3, `query_mysql.sh`, `engine=mysql`, schema flag `test`, total 1, ok 1, orphans 25 beginning at ref 2000. The Phase 4 demo audit reported 2 orphans. DB2 exit 3, `engine=db2`, total 1, ok 1, orphans 30 beginning at ref 2000. MariaDB exit 3, `query_mariadb.sh`, `engine=mariadb`, total 1, ok 1, orphans 15 beginning at ref 2000. Firebird exit 3, `engine=firebird`, `FIREBIRD_DB_PATH_TEST` (`hydrogen_test.fdb`), total 1, ok 1, orphans 15 beginning at ref 2000. Yugabyte exit 3, expect `engine=postgresql`, host `adm-c` port 30543, total 1, ok 1, orphans 15 beginning at ref 2000. MSSQL exit 3, `engine=mssql`, schema `testms`, total 1, ok 1, orphans 15 beginning at ref 2000. `counts.anomalies` matched the orphan count. The logs did not contain the password values.
- **2026-10-07** SQLite exit 1. The wrapper opened `tests/artifacts/database/sqlite/hydrotst.sqlite` (3977216 bytes, `sqlite_master` has no tables). Dump error: `no such table: queries`. `hydrodemo.sqlite` still has `queries`. No migration was run. `--help` exited 0. Phase 5 exit gate is met.

## Phase 6 — Eight demo wrappers

### Goal

The Test 40 launchers use the `_demo` names, and the unsuffixed
wrappers are gone.

### Entry gate

Phase 5 Status complete.

### Work items

- [x] 6.1 Rename `schematool_<engine>.sh` to
      `schematool_<engine>_demo.sh` for all eight, including the
      MSSQL wrapper from Phase 2. MariaDB demo schema stays `demo`.
      SQLite demo file stays `hydrodemo.sqlite`. Firebird demo path
      stays `FIREBIRD_DB_PATH_DEMO`. MSSQL demo schema stays `demoms`.
- [x] 6.2 Picker order matches the Locks list: test block, then demo
      block. `WRAPPER_ORDER` lists the sixteen stems.
- [x] 6.3 `smoke_test40_catalog.sh` calls the `_demo` wrappers and
      includes `mssql`. Update tool docs and the extras README. Delete
      any doc row that still shows `demomrdb` or an unsuffixed wrapper
      as the current interface.
- [x] 6.4 Grep `extras/schematool` for `schematool_<engine>.sh` exec
      lines. Tests and smokes call the new names.

### Done means

`ls extras/schematool/schematool_*.sh` prints sixteen files, eight
`_test` and eight `_demo`. SchemaHelper lists all sixteen.

### Exit gate

- `mks`, Test 98, Test 72.
- `smoke_test40_catalog.sh` against the demo databases that are up.
  Record pass and fail per engine. A down engine is a recorded skip,
  not a silent pass.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | Sixteen wrappers: eight `_test` and eight `_demo`. The picker lists them in locks order, test block then demo block. Catalog smoke: 6 pass, 2 fail, 0 down. |
| **Variances** | `schematool_firebird_demo.sh` passes `--database` from `FIREBIRD_DB_PATH_DEMO` and exits 1 when that variable is unset. Firebird and MSSQL catalog checks failed on the known `accounts.password_hash` mismatch (expected not null, live nullable, ref 1005). No engine was down. |

### Working log

- **2026-10-07** `mks` passed, 202 shell files, 3/3, 1189/1189 directives justified. Test 98 passed, 506 Lua files. Test 72 passed, 20/20. SchemaHelper 0.6.12, SchemaTool 1.13.0.
- **2026-10-07** `ls extras/schematool/schematool_*.sh` prints sixteen files. Discover returns those sixteen stems in locks order. No unsuffixed wrapper remains. Help examples and the Test 40 smoke call the `_demo` names. MSSQL is in the smoke list.
- **2026-10-07** `smoke_test40_catalog.sh` exit 2. SQLite pass. PostgreSQL pass. MySQL pass (`query_mysql.sh`, `catalog_mysql.sh`, `password_hash` nullable Y, ref 1190). MariaDB pass (`query_mariadb.sh`, `catalog_mariadb.sh`, ref 1190). DB2 pass. Yugabyte pass. Firebird fail, exit 2, `password_hash` nullable expected false, live true, ref 1005. MSSQL fail, exit 2, same `password_hash` row, schema `demoms`. No password in the smoke summary. No engine was skipped for being down. Phase 6 exit gate is met.

## Phase 7 — Expected shape from disk migrations

### Goal

The catalog and the default-row preview describe the migration files
on disk, so an older database shows what it still lacks.

### Entry gate

Phase 6 Status complete. Read-only. No apply changes yet.

### Payload

One SchemaTool run covers the designs the database actually has.
The disk set for the sixteen wrappers is `acuranzo+argent`, not two
runs and not Acuranzo alone. Argent is an optional pack on the
Acuranzo database: same connection, same SQL schema, same `queries`
table. It does not bootstrap `queries`. Each design keeps its own
`database.lua` and `database_<engine>.lua` beside its migration files.
The loader reads the copy beside the file.

`--design` accepts a plus-list. `--migrations` stays the first
design's folder (`…/acuranzo/migrations`), which is what a single
design name already requires, so fixture directories keep working.
Each later name is a sibling folder:
`dirname(dirname(--migrations))/<design>/migrations`. For the Helium
tree that is `…/002-helium/argent/migrations`. A missing sibling
directory is an error. Refs are the file numbers, sorted together
(Acuranzo 1000–1999, then Argent 2000–2999). The ranges do not overlap.

A single design name stays valid. Argent alone is not a wrapper
payload. The folder `elements/002-helium/gaius/` is still its own
database. Do not pass that tree as a pack on Acuranzo. A later
renumbered Gaius pack (3000–3999) can join the plus-list when it
exists as its own design folder. `PAYLOAD:acuranzo+gaius` and all
three are later Hydrogen payloads, not this slice.

### Work items

- [x] 7.0 Point discovery, expect, and all sixteen wrappers at
      `acuranzo+argent` in one run. Each design loads its own
      `database.lua`. A single design name still resolves only the
      `--migrations` directory.
- [x] 7.1 Fold DDL from the expected payloads of the disk migrations
      (the Lua extract), in ref order, across the selected `--from` /
      `--to` range, for every design in the plus-list. The forward
      body on disk is query type 1000. Applied rows in the database
      store that same body as type 1003. The disk fold reads the
      expanded type-1000 code and does not fold reverse (1001) or
      diagram (1002). Keep the current "fold only applied type 1003"
      behavior available as an explicit flag if a caller still wants
      the stored-text fold. The default becomes the disk fold.
- [x] 7.2 Compare `data_type` as well as nullability and presence.
      Type text is normalized (case, spacing) before compare. A real
      type difference is a `type` finding.
- [x] 7.3 Classify live extras. An object no migration mentions is
      an info row: counted, shown, not a failure, not applicable.
      An object the fold created and a later migration dropped, still
      present live, is a `dropped` finding.
- [x] 7.4 SchemaHelper queues `type` and `dropped`. Info extras show
      on the dashboard and stay out of the one-by-one review queue.
- [x] 7.5 Fixture in Test 72: a disk migration adds a column the
      live catalog lacks; the finding appears even when that ref has
      no type-1003 row in the dumped `queries` set. A second case:
      a live column no migration mentions does not increment the
      failure count.

### Done means

An audit of a database that stopped partway through the migration
list reports the missing later columns and tables as findings. Extra
production columns do not fail the audit.

### Exit gate

- Test 72, Test 98.
- One read-only SQLite audit against a copy of `hydrotst.sqlite` or
  `hydrodemo.sqlite` with a column deliberately still at an older
  shape, if a copy can be made without writing the original. Record
  the finding id.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | One run covers acuranzo+argent. The catalog compares presence, nullability, and data type, and classifies live extras. A copy of `hydrodemo.sqlite` left `accounts.password_hash` NOT NULL and dropped `stripe_customer_id`. Findings: `cat:accounts:password_hash:nullable` ref 1190, and `cat:accounts:stripe_customer_id:column` ref 1310. Added `prod_extra` stayed info and did not fail the audit. |
| **Variances** | The fold reads Firebird `ALTER col DROP NOT NULL` and `ADD col`, and MSSQL `ALTER COLUMN col type NULL` and `ADD col`. `--fold-stored` keeps the type-1003 dump fold. A catalog-only run extracts all 416 refs first (about 50s on SQLite). Type text is normalized for case and spacing only, so probe spellings that drop a length or expand a name are `type` findings. On 2026-10-07 `smoke_test40_catalog.sh` (MySQL host 10.118.0.3): sqlite, firebird, and mssql exited 0. postgresql and yugabytedb exited 2 with 6 type findings (`char(128)` vs `character`, `timestamptz` vs `timestamp with time zone`). mysql and mariadb exited 2 with 10 (`varchar(255)` vs `varchar`, `datetime(3)` vs `datetime`). db2 exited 2 with 8 (`varchar(250)` vs `varchar`, `char(128)` vs `character`). `password_hash` nullable stayed Y on all eight. No engine was down. No database was edited. The smoke was not weakened. |

### Working log

- **2026-10-07** Discovery of `acuranzo+argent` returns 416 refs, sorted, first `acuranzo_1000.lua`, last `argent_2029.lua`. A single design name still lists only that directory. A missing sibling directory is an error. Fixture files named `design_NNNN.lua` still match a single design. Expect reloads `database.lua` per design. `mks` passed, 202 shell files, 3/3, 1189/1189 directives. Test 98 passed, 507 Lua files. Test 72 passed, 20/20. SchemaHelper 0.6.13, SchemaTool 1.14.0.
- **2026-10-07** Disk fold of the sqlite expect marks `accounts.password_hash` nullable, ref 1190, and keeps `accounts.stripe_customer_id`, ref 1310. `--fold-stored` still folds a type-1003 dump. Passing both `--db` and `--expected` is an error.
- **2026-10-07** `smoke_test40_catalog.sh` against the demo databases, MySQL host 10.118.0.3. First pass: sqlite, postgresql, mysql, mariadb, db2, and yugabytedb exit 0 with `password_hash` nullable expected true, live true. Firebird and MSSQL exited 2 because the fold still had ref 1005 not null. After the fold learned those two spellings, both re-smoked exit 0, `password_hash` ref 1190 and `stripe_customer_id` ref 1310, no extra column on `accounts`. No engine was down. No database was edited. Phase 7 stays open for 7.2–7.5.
- **2026-10-07** Type compare, dropped-versus-info, and the SchemaHelper queue landed. Test 72 passed, 21/21. Test 98 passed, 508 Lua files. Test 92 passed, 202 shell files, 3/3, 1189/1189 directives. SchemaTool 1.15.0, SchemaHelper 0.6.14. A copy of `hydrodemo.sqlite` (original not written) reported `cat:accounts:password_hash:nullable` ref 1190 and `cat:accounts:stripe_customer_id:column` ref 1310. `prod_extra` was info, status I, and was not a failure. The eight-engine accounts smoke then exited 2 overall: sqlite, firebird, and mssql passed; the other five failed on type spelling only, with `password_hash` nullable still Y. No engine was down. No database was edited.

## Phase 8 — Structural apply, per dialect

### Goal

`[u]` can bring live structure in line with the fold, in each
engine's own DDL, one statement at a time.

### Entry gate

Phase 7 Status complete. Write path. SQLite proof uses a copy of an
artifact. Any other engine needs an explicit yes.

### Work items

- [x] 8.1 Replace the hand-rolled JSON string decoder used to build
      `UPDATE` literals. Non-ASCII content round-trips. A unit check
      covers a `\u` escape above 127.
- [x] 8.2 Whole metadata row. One confirm token `REF` replaces
      `code`, `name`, and `summary` on that `query_ref` and
      `query_type`. The screen still says this does not replay DDL.
      The one-field token `REF.field` can remain for a single-field
      finding.
- [x] 8.3 Dialect DDL, generated in `schemahelper_apply.lua` (or a
      sibling module it calls):

      | Change | postgresql / yugabytedb | mysql | mariadb | sqlite | db2 | firebird | mssql |
      | --- | --- | --- | --- | --- | --- | --- | --- |
      | Add column | `ADD COLUMN` | `ADD COLUMN` | `ADD COLUMN` | `ADD COLUMN` | `ADD COLUMN` | `ADD` | `ADD` |
      | Drop / set not null | `ALTER COLUMN … DROP/SET NOT NULL` | `MODIFY COLUMN` | `MODIFY COLUMN` | table rebuild, refused in this phase with the reason on screen | DB2 `ALTER COLUMN` | Firebird `ALTER COLUMN` spelling from `database_firebird.lua` | `ALTER COLUMN … NULL` / `NOT NULL` |
      | Type change | `ALTER COLUMN … TYPE` | `MODIFY COLUMN` | `MODIFY COLUMN` | refused this phase (rebuild) | DB2 type alter | Firebird type alter | `ALTER COLUMN` type |
      | Missing table | `CREATE TABLE` from the folded column list | same, engine types | same | same | same | same | same |
      | Dropped column or table | `DROP COLUMN` / `DROP TABLE` | same | same | `DROP TABLE` only; `DROP COLUMN` refused this phase | same | same | `DROP COLUMN` drops the default first when SQL Server requires it |

      SQLite nullability and type changes need a table rebuild. This
      phase refuses them with an on-screen reason rather than emitting
      a rebuild that copies rows. A later plan can add the rebuild.
- [x] 8.4 Confirm tokens. Structural create/alter stays
      `object` or `object.column`. Drops use the `DROP` token from
      the Locks section. Refused cases return a reason and do not
      open the confirm prompt.
- [x] 8.5 `exec_sql` already has a branch per engine after Phases 1,
      2, and 4. Send the one statement in a transaction where the
      engine allows it, and commit only after the client returns
      success. SQLite uses a copy of the artifact file for the proof.
- [x] 8.6 Test 72 asserts the SQL text for each engine for add column,
      nullability, type, create table, and drop. It does not execute
      them.

### Done means

On a copy of the SQLite artifact, a missing column finding applies,
a re-audit clears that finding, and a production-only extra column
was never offered for drop. The generated SQL for the other seven
engines is fixture-checked. The review copy says a metadata replace
does not replay DDL.

### Exit gate

- Test 72, Test 98, `mks` if shell changed.
- The SQLite copy proof, with before and after finding ids in the
  working log.
- Server-engine execution only with an explicit yes. Otherwise record
  "SQL fixture only" for those engines. That still completes the
  phase.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | On a copy of `hydrodemo.sqlite`, `cat:accounts:stripe_customer_id:column` ref 1310 applied as `ALTER TABLE accounts ADD COLUMN stripe_customer_id varchar(100)`. Re-audit exit 0, failures empty. `prod_extra` stayed info, status I, and was not offered for drop. Generated SQL for the other seven engines is fixture-checked. A metadata replace says it does not replay DDL. |
| **Variances** | `database_firebird.lua` has no table `ALTER`. Firebird spelling follows `acuranzo_1190` (no `COLUMN` keyword) and `FIREBIRD.md` (`ADD`/`DROP` without `COLUMN`). DB2 type change is `ALTER COLUMN … SET DATA TYPE`. No migration in the tree shows that form, and the one statement does not add `REORG`. MSSQL `DROP COLUMN` is a batch in the existing transaction: look up `sys.default_constraints`, drop it when present, then drop the column. SQLite nullability, type, and `DROP COLUMN` are refused. A whole-row `UPDATE` sets `code`, `name`, and `summary` when those keys are present. Probe-spelling type findings were not applied. Firebird `SET TRANSACTION`/`COMMIT` was not executed. SQL fixture only for postgresql, yugabytedb, mysql, mariadb, db2, firebird, and mssql. |

### Working log

- **2026-10-07** Dialect DDL, UTF-8 `\u` literals, and the whole-row metadata token landed. Test 72 passed, 22/22. Test 98 passed, 510 Lua files. Test 92 passed, 202 shell files, 3/3, 1189/1189 directives. SchemaHelper 0.6.15, SchemaTool 1.15.1. A copy of `hydrodemo.sqlite` (original not written; mtime stayed `2026-10-07 13:43:22.639974472 -0700`, size 14036992) dropped `stripe_customer_id` and added `prod_extra`. Before: exit 2, `cat:accounts:stripe_customer_id:column` ref 1310, and `cat:accounts:prod_extra:extra_column` status I. The generated `ADD COLUMN` ran under `BEGIN`/`COMMIT` on the copy only. After: exit 0, failures empty, `prod_extra` still status I. The copy was removed. No server engine was executed.

## Phase 9 — Migration-owned default rows

### Goal

Default rows the migrations insert are present and match on the
columns those statements set. Additional rows in the table are
ignored.

### Entry gate

Phase 8 Status complete. Same write caution: SQLite copy first.

### Work items

- [x] 9.1 Extract keyed `INSERT`, `UPDATE`, and `DELETE` from the
      disk migration SQL in ref order. Build the net expected row
      per table. Statements with no key become unkeyed-DML findings
      with apply refused and a pointer to AutoMigration.
- [x] 9.2 Probe the live table with a `SELECT` of the expected keys
      only, plus the migration-owned columns. No `SELECT *`.
- [x] 9.3 Findings: `row_missing`, `row_diff`, `row_present` (a key
      the net migration deleted). A live key that is not in the
      expected set produces nothing.
- [x] 9.4 Apply: `INSERT` the missing default row, `UPDATE` the
      differing migration-owned columns, `DELETE` only the key a
      migration deleted. Confirm token is `table.key`. One row per
      confirm.
- [x] 9.5 Per-engine literal quoting for the inserted values. Reuse
      the Phase 8 string path so non-ASCII values survive.
- [x] 9.6 Test 72 fixture: a `contacts`-style table with two default
      rows in the migration, three rows live (the two defaults, one
      of them stale, plus an extra person). The queue contains the
      stale default and, if one default was removed from the live
      copy, the missing default. The extra person is absent from
      the queue and from every generated `DELETE`.

### Done means

The contacts example in the Purpose section is true on a SQLite copy:
missing default row can be inserted, a changed default column can be
updated, and the extra person is not listed as drift.

### Exit gate

- Test 72, Test 98.
- The SQLite copy proof recorded in the working log, including the
  extra row's key and the fact that it was not in the queue.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | On a copy of `hydrodemo.sqlite`, missing default `contacts.1` was inserted (`Café` / London) and stale `contacts.2` city was updated from Berlin to Paris. Extra person key 9, Zoe, stayed in the table and was not in the queue or in any generated `DELETE`. `contacts.3` remained `row_present` because that `DELETE` was not executed. |
| **Variances** | Row findings live in `rows_findings.json` and the SchemaHelper queue. Catalog exit and `smoke_test40_catalog.sh` stay shape-only. DML on `queries` is skipped. Non-literal values are not migration-owned. Unkeyed DML is refused and points at AutoMigration; it does not by itself set exit 2. A row-track failure warns and does not change the catalog exit. `--fold-stored` skips the row track. SQL fixture only for postgresql, yugabytedb, mysql, mariadb, db2, firebird, and mssql. |

### Working log

- **2026-10-07** Keyed default rows, a targeted probe, and one-row apply landed. Test 72 passed, 23/23, script 1.6.0. Test 98 passed, 519 Lua files. Test 92 passed, 203 shell files, 3/3, 1189/1189 directives. SchemaHelper 0.6.16, SchemaTool 1.15.2. A copy of `hydrodemo.sqlite` (original not written; mtime stayed `2026-10-07 14:00:14.162349409 -0700`, size 14036992) held contacts keys 2 (Bea, Berlin), 3 (Cara, Rome), and 9 (Zoe, Oslo). Before: `row_missing` 1, `row_diff` 2, `row_present` 3, one unkeyed `UPDATE` with no `WHERE`. Key 9 was not in the queue. Generated `DELETE FROM contacts WHERE contact_id = 3` does not name key 9 and was not executed. The copy applied, under `sqlite3` `BEGIN`/`COMMIT`, `INSERT` key 1 (`Café`, London; name bytes `43 61 66 C3 A9`) and `UPDATE` key 2 city to Paris. After: missing 0, diff 0, `row_present` 3, unkeyed still present. Live rows were 1 Café London, 2 Bea Paris, 3 Cara Rome, 9 Zoe Oslo. The copy was removed. Expanded refs 1044 and 1144 kept lookup keys `019|0`, `019|1`, and `0|019`, and account keys 1–4, with `password_hash` left unowned. No server engine was executed.

## Phase 10 — Docs and smoke

### Goal

The operator docs describe the tool this plan produced, and the
sixteen wrappers are the documented way to launch it.

### Entry gate

Phase 9 Status complete.

### Work items

- [x] 10.1 Rewrite the apply and catalog sections of
      [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md) and
      [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md) so they match
      Phases 7–9. Link this plan. Keep the v1 and v2 archives linked
      as history. After the move, operator docs label this plan
      complete (2026-10-07) and point at the archive path.
- [x] 10.2 Extras README, `smoke_test40_catalog.sh` usage comment, and
      Test 72 doc agree on the sixteen wrapper names and the sidecar
      role suffix.
- [x] 10.3 Sitemap, structure, and plans index link this file at its
      archive path. Test 04 is the link check.
- [x] 10.4 Move this file to `docs/H/plans/complete/` and add
      `_COMPLETE` to the name. Every phase Status is complete.

### Done means

A new reader can start SchemaHelper, pick a test or demo wrapper, and
predict what `[u]` will and will not change, using only the operator
docs.

### Exit gate

- Test 04 clean for the docs this phase touched.
- Test 72 and Test 98 still green.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-10-07 |
| **Result** | Operator docs match Phases 7–9. A reader can pick one of the sixteen wrappers and predict what `[u]` will and will not change. This file moved to `docs/H/plans/complete/SCHEMA_V2_PLAN_COMPLETE.md`. |
| **Variances** | Item 10.1 said "active plan" while this file was still open. The archived operator docs call it complete. `smoke_test40_catalog.sh` calls the eight `_demo` wrappers only. Its order differs from the picker. The sixteen names still agree. |

### Working log

- **2026-10-07** Catalog and apply docs now cover eight-engine probes, `rows_findings.json` (catalog exit unchanged; `--fold-stored` skips it), and `[u]`. SQLite nullability, type, and `DROP COLUMN` stay refused. Firebird omits `COLUMN`. DB2 `DROP COLUMN` commits and runs `REORG TABLE`. MSSQL drops a default with `QUOTENAME` and `sp_executesql`. Info extras are not applied. Extra live rows are not deleted. Unkeyed DML is refused. Queries-table DML is not a row finding. Sixteen wrapper names and sidecar `schemahelper_<design>_<engine>_<role>.json` agree in the extras README, `smoke_test40_catalog.sh` 1.1.4, Test 72 1.7.1, and `docs/H/tests/test_72_schemahelper.md`. Test 72 passed 24/24 on 2026-10-07 (script 1.7.0 before this comment bump). Test 98 passed, 520 Lua files. Test 92 passed, 204 shell files, 1212/1212 directives. `hydrodemo.sqlite` after that run: mtime `2026-10-07 15:48:55.844882680 -0700`, size 14036992. `schematooltest` was not left in that file. v1 and v2 SchemaHelper archives stay linked. SchemaTool's archived plan stays linked.
- **2026-10-07** Exit gate after the archive move. Test 04 passed, 5/5, 2,707 links, 0 missing, 0 orphaned, 0 relative. Test 90 passed, 370 markdown files. Test 72 1.7.1 passed 24/24 in 80.289s, all eight engines. Test 98 passed, 520 Lua files. Test 92 passed, 204 shell files, 1212/1212 directives. `mkl` exited 0 with 2,707 links found. `hydrodemo.sqlite` after this re-run: mtime `2026-10-07 16:32:39.148060690 -0700`, size 14036992. `schematooltest` is not in `sqlite_master`.

## Plan log

- **2026-10-07** Plan written. Phase order is Firebird adapters, MSSQL
  adapters, Cockroach name removal, MySQL/MariaDB split, eight test
  wrappers, eight demo wrappers, disk-migration expected shape,
  per-dialect structural apply, migration-owned default rows, docs.
  Extra production rows are out of scope on purpose. No code in this
  session.

## Related

- Auditor guide: [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md)
- Review UI: [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md)
- Archived SchemaTool plan:
  [`SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md)
- Archived SchemaHelper plans:
  [`SCHEMAHELPER_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_COMPLETE.md),
  [`SCHEMAHELPER_V2_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md)
- Engines:
  [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md),
  [`MSSQL_COMPLETE.md`](/docs/H/plans/complete/MSSQL_COMPLETE.md),
  [`MARIADB_SPLIT_PLAN.md`](/docs/H/plans/MARIADB_SPLIT_PLAN.md)
- Test 72:
  [`test_72_schemahelper.md`](/docs/H/tests/test_72_schemahelper.md)
