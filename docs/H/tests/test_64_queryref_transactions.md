# Test 64: QueryRef Transactions

## Overview

The [`test_64_queryref_transactions.sh`](/elements/001-hydrogen/hydrogen/tests/test_64_queryref_transactions.sh)
script drives QueryRefs 156, 157, and 158 through
`POST /api/conduit/alt_query` on one Hydrogen process. The config is
[`hydrogen_test_64_queryref_transactions.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_64_queryref_transactions.json)
and the web port is 5640.

The fixture SQL is installed by `acuranzo_1386.lua`, `acuranzo_1387.lua`,
and `acuranzo_1388.lua`. The plan is
[`TRANSACTION_PLAN_COMPLETE.md`](/docs/H/plans/complete/TRANSACTION_PLAN_COMPLETE.md).

## Purpose

On each of the eight demo connections the test runs:

1. QueryRef 156. One transaction block inserts a row and updates it.
   The trailing SELECT is the last entry. Firebird runs that
   parameter-free UPDATE with `execute_immediate` and reports `affected`
   0. The SELECT note is the proof the update ran.
2. QueryRef 158. The SELECT sees that row. The primary drop commits.
   The staging drop fails because `TEST_TXN_STAGING` was never created.
3. QueryRef 157 with `SHOULD_FAIL = 0`. Both blocks commit. The SELECT
   returns `txn_id` 1 and `txn_id` 2.
4. QueryRef 158. Both drops succeed. The SELECT still shows both rows.
5. QueryRef 157 with `SHOULD_FAIL = 1`. Block one's row stays. Block
   two's duplicate key rolls that block back. The trailing SELECT does
   not run.
6. QueryRef 158 again. On PostgreSQL, SQLite, DB2, and MSSQL the
   staging table rolled back with the block, so the staging drop fails.
   On Firebird, MySQL, MariaDB, and YugabyteDB the empty staging table
   survived, the SELECT shows only `txn_id` 1, and both drops succeed.

## Request

```json
POST /api/conduit/alt_query
{
  "token": "<jwt>",
  "database": "Demo_PG",
  "query_ref": 157,
  "params": {
    "STRING": {"RUN_LABEL": "t64-PostgreSQL-commit"},
    "INTEGER": {"SHOULD_FAIL": 0}
  }
}
```

QueryRef 156 takes `RUN_LABEL` only. QueryRef 158 takes no parameters.

## Response

A success body puts the statement array in `rows`. Each entry has
`statement`, `block`, `rows`, `affected`, `success`, and `error`.
`statement` restarts at 1 inside each block. Bare statements count as
blocks, so QueryRef 156 is four entries (blocks 1, 2, and 3) and a
successful QueryRef 157 is seven entries.

A database error is HTTP 422:

```json
{
  "success": false,
  "error": "Database error",
  "message": "transaction block 4 failed at statement 3 (rolled back)"
}
```

That body does not include `error_class` or the statement array. The
engine text stays on the executor entry, which this endpoint does not
return after a failure. The forced-failure message for QueryRef 157 is
`transaction block 4 failed at statement 3 (rolled back)`. The missing
staging table on QueryRef 158 is
`transaction block 3 failed at statement 1 (rolled back)`.

## Connections

The eight names match the conduit helper map: `Demo_PG`, `Demo_MY`,
`Demo_SQL` (`hydrodemo.sqlite`), `Demo_DB2`, `Demo_MR`, `Demo_FB`,
`Demo_YB`, and `Demo_MS`. `Demo_YB` is the YugabyteDB schema `demo`.
Migrations are `PAYLOAD:acuranzo`. `TestMigration` is false.

## Run

From the Hydrogen root, through the orchestrator:

```bash
zsh -ic 'tests/test_00_all.sh 64_queryref_transactions'
```
