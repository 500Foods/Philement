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
| 1 Firebird adapters | not started | Moderate |
| 2 MSSQL adapters | complete | Moderate |
| 3 Remove Cockroach names | not started | Quick |
| 4 Split MySQL and MariaDB | not started | Moderate |
| 5 Eight test wrappers (tests 32–39) | not started | Moderate |
| 6 Eight demo wrappers (Test 40) | not started | Moderate |
| 7 Expected shape from disk migrations | not started | Difficult |
| 8 Structural apply, per dialect | not started | Difficult |
| 9 Migration-owned default rows | not started | Difficult |
| 10 Docs and smoke | not started | Quick |

Operator guides:
[`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md),
[`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).
Code:
[`extras/schematool/`](/elements/001-hydrogen/hydrogen/extras/schematool/).

Sister plans:
[`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) Phase 9 marked the Firebird
wrapper and ping complete. The dump and catalog adapters were not
written. This plan owns them.
[`MSSQL.md`](/docs/H/plans/MSSQL.md) Phase 6 items 6.1–6.3 (wrapper,
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

- [ ] 1.1 File-database connection readiness. SQLite and Firebird
      succeed when the database path exists and the user is set.
      Empty schema is valid. A missing host must not force disk-only
      mode.
- [ ] 1.2 `db/query_firebird.sh`. `isql-fb` as `SYSDBA`, password from
      `FIREBIRD_SYSDBA_PASSWORD`, database from the path flag. `SELECT`
      of `queries` types 1000–1003. Same JSON object shape as
      `query_pg.sh`. The script contains no DML. Password never
      printed. Scrub it from `isql` errors before they reach the log.
- [ ] 1.3 `db/catalog_firebird.sh`. Targeted read of
      `RDB$RELATIONS` / `RDB$RELATION_FIELDS` for the tables in
      `--tables`. Map `RDB$NULL_FLAG` to nullable. Map Firebird field
      types to the type spellings the fold stores (the migration's SQL
      type, lowercased). A fixture of recorded `isql` output covers
      the mapping with no live database.
- [ ] 1.4 `schematool_runners.sh` dispatches `firebird` to those two
      scripts. `--engine firebird` still reaches `schematool_expect.lua`
      as `firebird` (`database.defaults.firebird` already exists).
- [ ] 1.5 SchemaHelper: picker blurb names `FIREBIRD_DB_PATH_DEMO` and
      `FIREBIRD_SYSDBA_PASSWORD`. `exec_sql` gains an `isql-fb` branch
      so a later apply phase has a place to send one statement. This
      phase does not invoke it against a database.
- [ ] 1.6 Help text in `schematool.sh`. The Firebird env block is
      `FIREBIRD_DB_PATH_DEMO` / `_TEST` / `FIREBIRD_SYSDBA_PASSWORD`.
      The line that lists Firebird under `ACURANZO_DB_*` goes away.
- [ ] 1.7 Docs: Firebird rows in
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

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

- [ ] 3.1 Delete the `cockroachdb` → `postgresql` branch in
      `schematool.sh`, `schemahelper_connect.lua`, and
      `schemahelper_apply.lua`. An unknown engine, including
      `cockroachdb`, exits 1 and names the supported list. The message
      says Firebird replaced that slot.
- [ ] 3.2 Strip the alias from
      [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md),
      [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md), and
      [`extras/schematool/README.md`](/elements/001-hydrogen/hydrogen/extras/schematool/README.md).
- [ ] 3.3 Leave changelog lines that describe the 2026-09 rename.
      Leave completed plans. Leave Unity `normalize_engine_name`.
      Those are history, and they are outside `extras/schematool`.
- [ ] 3.4 `rg -i cockroach extras/schematool` returns only changelog
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 4 — Split MySQL and MariaDB

### Goal

MySQL and MariaDB are separate SchemaTool engines, matching
`src/database/mysql/`, `src/database/mariadb/`, `database_mysql.lua`,
and `database_mariadb.lua`.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 Remove `mariadb) ENGINE=mysql` from `schematool.sh`.
      Runners, readiness, env, help, and expect all see `mariadb`.
      Expect then loads `database.defaults.mariadb`.
- [ ] 4.2 `db/query_mariadb.sh` and `db/catalog_mariadb.sh`. They may
      source a shared client fragment. They are separate entry points
      so a later dialect quirk does not land in an `if engine` inside
      the MySQL file. The MySQL files drop the "MySQL/MariaDB" header.
- [ ] 4.3 Credentials. MySQL uses `MYSQL_DB_*`. MariaDB uses
      `MARIADB_DB_*`. Delete the `CANVAS_DB_*` fallback from
      SchemaTool and SchemaHelper. Fix the MariaDB demo schema from
      `demomrdb` to `demo` in the wrapper, the picker blurb, and
      `apply_family`.
- [ ] 4.4 Apply and qualify helpers take `mariadb` as its own engine.
      Qualified names stay `` `schema`.`table` `` style for both until
      Phase 8 replaces the DDL text. This phase does not need to
      invent MariaDB-only DDL.
- [ ] 4.5 Tool docs and `smoke_test40_catalog.sh` treat the two
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 5 — Eight test wrappers

### Goal

SchemaHelper can be pointed at each tests 32–39 database.

### Entry gate

Phase 4 Status complete. All eight engines have adapters.

### Work items

- [ ] 5.1 Add the eight `schematool_<engine>_test.sh` files from the
      Locks table. Each sets the test schema or file and the engine's
      own env. Firebird uses `FIREBIRD_DB_PATH_TEST`. MSSQL uses
      schema `testms`. SQLite uses `hydrotst.sqlite`.
- [ ] 5.2 Picker keys by wrapper path. `schematool_mysql_test.sh` and
      `schematool_mysql.sh` are two rows. Blurbs show the schema or
      the file name, plus the env names, never a password.
- [ ] 5.3 Sidecar path gains the role:
      `schemahelper_<design>_<engine>_<role>.json`. Suffix `_test` is
      role `test`. An unsuffixed wrapper is role `demo` until Phase 6.
- [ ] 5.4 Test 72 fixture grows a second wrapper stem and asserts the
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 6 — Eight demo wrappers

### Goal

The Test 40 launchers use the `_demo` names, and the unsuffixed
wrappers are gone.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 Rename `schematool_<engine>.sh` to
      `schematool_<engine>_demo.sh` for all eight, including the
      MSSQL wrapper from Phase 2. MariaDB demo schema stays `demo`.
      SQLite demo file stays `hydrodemo.sqlite`. Firebird demo path
      stays `FIREBIRD_DB_PATH_DEMO`. MSSQL demo schema stays `demoms`.
- [ ] 6.2 Picker order matches the Locks list: test block, then demo
      block. `WRAPPER_ORDER` lists the sixteen stems.
- [ ] 6.3 `smoke_test40_catalog.sh` calls the `_demo` wrappers and
      includes `mssql`. Update tool docs and the extras README. Delete
      any doc row that still shows `demomrdb` or an unsuffixed wrapper
      as the current interface.
- [ ] 6.4 Grep `extras/schematool` for `schematool_<engine>.sh` exec
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 7 — Expected shape from disk migrations

### Goal

The catalog and the default-row preview describe the migration files
on disk, so an older database shows what it still lacks.

### Entry gate

Phase 6 Status complete. Read-only. No apply changes yet.

### Work items

- [ ] 7.1 Fold DDL from the expected payloads of the disk migrations
      (the Lua extract), in ref order, across the selected `--from` /
      `--to` range. Keep the current "fold only applied type 1003"
      behavior available as an explicit flag if a caller still wants
      the stored-text fold. The default becomes the disk fold.
- [ ] 7.2 Compare `data_type` as well as nullability and presence.
      Type text is normalized (case, spacing) before compare. A real
      type difference is a `type` finding.
- [ ] 7.3 Classify live extras. An object no migration mentions is
      an info row: counted, shown, not a failure, not applicable.
      An object the fold created and a later migration dropped, still
      present live, is a `dropped` finding.
- [ ] 7.4 SchemaHelper queues `type` and `dropped`. Info extras show
      on the dashboard and stay out of the one-by-one review queue.
- [ ] 7.5 Fixture in Test 72: a disk migration adds a column the
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 8 — Structural apply, per dialect

### Goal

`[u]` can bring live structure in line with the fold, in each
engine's own DDL, one statement at a time.

### Entry gate

Phase 7 Status complete. Write path. SQLite proof uses a copy of an
artifact. Any other engine needs an explicit yes.

### Work items

- [ ] 8.1 Replace the hand-rolled JSON string decoder used to build
      `UPDATE` literals. Non-ASCII content round-trips. A unit check
      covers a `\u` escape above 127.
- [ ] 8.2 Whole metadata row. One confirm token `REF` replaces
      `code`, `name`, and `summary` on that `query_ref` and
      `query_type`. The screen still says this does not replay DDL.
      The one-field token `REF.field` can remain for a single-field
      finding.
- [ ] 8.3 Dialect DDL, generated in `schemahelper_apply.lua` (or a
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
- [ ] 8.4 Confirm tokens. Structural create/alter stays
      `object` or `object.column`. Drops use the `DROP` token from
      the Locks section. Refused cases return a reason and do not
      open the confirm prompt.
- [ ] 8.5 `exec_sql` already has a branch per engine after Phases 1,
      2, and 4. Send the one statement in a transaction where the
      engine allows it, and commit only after the client returns
      success. SQLite uses a copy of the artifact file for the proof.
- [ ] 8.6 Test 72 asserts the SQL text for each engine for add column,
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 9 — Migration-owned default rows

### Goal

Default rows the migrations insert are present and match on the
columns those statements set. Additional rows in the table are
ignored.

### Entry gate

Phase 8 Status complete. Same write caution: SQLite copy first.

### Work items

- [ ] 9.1 Extract keyed `INSERT`, `UPDATE`, and `DELETE` from the
      disk migration SQL in ref order. Build the net expected row
      per table. Statements with no key become unkeyed-DML findings
      with apply refused and a pointer to AutoMigration.
- [ ] 9.2 Probe the live table with a `SELECT` of the expected keys
      only, plus the migration-owned columns. No `SELECT *`.
- [ ] 9.3 Findings: `row_missing`, `row_diff`, `row_present` (a key
      the net migration deleted). A live key that is not in the
      expected set produces nothing.
- [ ] 9.4 Apply: `INSERT` the missing default row, `UPDATE` the
      differing migration-owned columns, `DELETE` only the key a
      migration deleted. Confirm token is `table.key`. One row per
      confirm.
- [ ] 9.5 Per-engine literal quoting for the inserted values. Reuse
      the Phase 8 string path so non-ASCII values survive.
- [ ] 9.6 Test 72 fixture: a `contacts`-style table with two default
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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

## Phase 10 — Docs and smoke

### Goal

The operator docs describe the tool this plan produced, and the
sixteen wrappers are the documented way to launch it.

### Entry gate

Phase 9 Status complete.

### Work items

- [ ] 10.1 Rewrite the apply and catalog sections of
      [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md) and
      [`SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md) so they match
      Phases 7–9. Link this plan as the active plan. Keep the v1 and
      v2 archives linked as history.
- [ ] 10.2 Extras README, `smoke_test40_catalog.sh` usage comment, and
      Test 72 doc agree on the sixteen wrapper names and the sidecar
      role suffix.
- [ ] 10.3 Sitemap, structure, and plans index already link this file
      from the day it was added. Confirm they still do after any
      rename. Run Test 04.
- [ ] 10.4 Move this file to `docs/H/plans/complete/` and add
      `_COMPLETE` to the name only when every phase Status is
      complete. Until then it stays here.

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
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working log

(none yet)

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
  [`MSSQL.md`](/docs/H/plans/MSSQL.md),
  [`MARIADB_SPLIT_PLAN.md`](/docs/H/plans/MARIADB_SPLIT_PLAN.md)
- Test 72:
  [`test_72_schemahelper.md`](/docs/H/tests/test_72_schemahelper.md)
