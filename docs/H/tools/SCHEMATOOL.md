# SchemaTool — Migration Drift Auditor

Standalone **Bash + Lua** operator utility under
[`extras/schematool/`](/elements/001-hydrogen/hydrogen/extras/schematool/).

**Two tracks:**

| Track | Flag | Answers |
| ------- | ------ | --------- |
| **Metadata (v1)** | default | Did LOAD/APPLY happen? Does stored `queries` text match Lua? |
| **Catalog (v2)** | `--catalog` | Do live tables/columns/nullability match net applied DDL? |

Quick start (local extras README):
[`extras/schematool/README.md`](/elements/001-hydrogen/hydrogen/extras/schematool/README.md).

Active plan:
[`/docs/H/plans/SCHEMA_V2_PLAN.md`](/docs/H/plans/SCHEMA_V2_PLAN.md).
Archived plan:
[`/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md).

## Purpose

Hydrogen LOAD/APPLY advances schema well, but does **not** detect
**historical edit drift**: someone changes an already-shipped
`design_NNNN.lua` instead of adding `NNNN+1`. Existing databases keep the old
payload in `queries.code`. SchemaTool **metadata** track surfaces that class
of problem with a per-migration checklist.

Separately, later migrations can change live shape (e.g. `acuranzo_1190.lua`
drops `NOT NULL` on `accounts.password_hash`) while each migration’s stored
text still matches its own Lua file. The **catalog** track folds forward DDL
from the migration files and probes the live catalog (targeted, not a full-DB dump).

## What it is / is not

| Is | Is not |
| ---- | -------- |
| Read-only auditor (native clients) | Replacement for Hydrogen LOAD/APPLY |
| Metadata fidelity (types 1000–1003) | Row-data auditor / product table scans |
| Live catalog probes (`--catalog`) | Bulk `pg_dump` / full-schema export default |
| Console checklist via `tables` | Auto-executor of remediation SQL |
| Commented `.sql` + orphan `.mig` | C code in `src/` or a REST endpoint |
| Bash + Lua under `extras/` | Auto-uncommented live DDL |

## Requirements

- `tables`, `jq`, `lua` (and `lua-brotli` for compressed migration payloads)
- Engine client for the target: `psql` / `mysql` / `sqlite3` / `db2` / in-container `sqlcmd` (MSSQL)
- Migrations folder containing `database.lua` and `design_NNNN.lua`.
  `--design acuranzo+argent` keeps that folder as the Acuranzo anchor and
  also reads the sibling `argent/migrations` directory. A single design
  name still uses only `--migrations`.

## Quick start

```bash
# From hydrogen root (or any cwd with absolute paths)
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo+argent \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --from 1000 --to 1005 \
  --out-dir /tmp/schematool-out
```

PostgreSQL (env fallbacks `ACURANZO_DB_*`):

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo+argent \
  --engine postgresql \
  --schema demo \
  --from 1000 --to 1010 \
  --out-dir /tmp/schematool-out
```

## Default behavior

When connection parameters resolve (flags or env) and `--dry-disk` is not set,
SchemaTool runs a **full audit**:

1. Discover disk migrations (`design_NNNN.lua`)
2. Extract expected payloads (same path as test_31 / `get_migration`)
3. Dump DB metadata (`queries` types 1000–1003) via native client
4. Compare (LOAD / L.match / APPLY / A.match)
5. Render checklist with `tables`
6. Write commented remediation `.sql` (and `.mig` if orphans exist)

Specialized modes:

| Flag | Behavior |
| ------ | ---------- |
| `--dry-disk` | Disk discovery + stub SQL only |
| `--emit-expected [PATH]` | Expected payloads JSON only |
| `--dump-db [PATH]` | DB metadata JSON only |
| `--catalog` | Live catalog audit (disk DDL fold + targeted probes) |
| `--fold-stored` | Catalog fold uses applied type-1003 text from the DB dump |
| `--dump-catalog [PATH]` | Live catalog JSON only |
| `--only-tables a,b` | Catalog: limit fold output + probes (cheap one-table path) |

### Catalog track (`--catalog`)

```bash
# 1190 acceptance — accounts.password_hash must be live-nullable
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo+argent \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --catalog --only-tables accounts \
  --out-dir /tmp/schematool-cat --no-sql
```

PostgreSQL (Test 40 / `ACURANZO_DB_*`):

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo+argent \
  --engine postgresql \
  --schema demo \
  --catalog --only-tables accounts \
  --out-dir /tmp/schematool-cat --no-sql
```

How it works:

1. Extract engine-expanded payloads for every design in `--design` (same path as metadata expect). `--from` / `--to` limit the refs.
2. **Disk fold** — forward body is query type **1000** (the text the database stores as type **1003** after apply). Reverse (1001) and diagram (1002) are not folded. Parse CREATE/ALTER/DROP NOT NULL / MODIFY / SQLite rebuild rename in ref order, including Firebird and MSSQL spellings that omit `COLUMN` and MSSQL `ALTER COLUMN … NULL`.
3. **Targeted probe** — only tables in the expected set (or `--only-tables`):  
   SQLite `PRAGMA table_info`; PG/MySQL `information_schema`; DB2 `SYSCAT.COLUMNS`
4. Compare presence, **nullability**, and **data type**. Type text is
   compared after case and spacing are normalized. A real type difference
   is a `type` finding. A live object no migration mentions is info:
   counted, shown, not a failure. An object the fold created and a later
   migration dropped, still present live, is a `dropped` finding.

`--fold-stored` skips the disk extract and folds applied type-1003 `code` from the queries dump instead.

With `--only-tables`, metadata audit is skipped (fast path). Without it,
`--catalog` runs **after** the default metadata audit; exit is **worst-wins**.
If the catalog fold, probe, or compare fails after a successful metadata
audit, SchemaTool **1.8.2** skips the catalog track, keeps metadata
artifacts, and exits with the metadata code (0 / 2 / 3) instead of 1.
Catalog-only (`--only-tables`) still exits 1 on catalog failure.

**Teaching examples:** catalog → **1190** `password_hash` nullable; metadata → **1280/1281** mail seed text drift.

## Checklist columns

| Column | Meaning |
| -------- | --------- |
| Ref | Migration number |
| File | `design_NNNN.lua` (or `(orphan)`) |
| LOAD | Y if type 1000 or 1003 exists |
| L.match | Forward `code`+`name`+`summary` vs Lua (Y/N/`-`) |
| APPLY | Y if type 1003 exists |
| A.match | Type 1003 payload vs Lua (Y/N/`-`) |
| Notes | Drift reason, missing load/apply, anomaly, orphan |

## Outputs

### Console (`tables`)

Hydrogen `tables` layout+data JSON → ANSI table. Theme is Blue when exit 0,
Red otherwise (dry-disk / drift / anomalies). Footer shows counts and exit.

### Finding details (after the table)

When the audit exit is non-zero (and `--format` is not `json`), SchemaTool
prints a **detail section** under the checklist:

| Track | Content |
| ------- | --------- |
| Metadata | Per drift: line-oriented **DB actual (−) vs Lua expected (+)** diffs for `code`/`name`/`summary`, plus **commented** `UPDATE …queries` remediation to align metadata |
| Catalog | Per failed check: live vs expected, nullability/presence guidance |

Also written under `--out-dir` as `finding_detail.txt` /
`catalog_finding_detail.txt`. Suppress with `--no-detail`. Cap diff size with
`--detail-max-lines N` (default 80).

**Caveat:** commented UPDATEs fix **stored migration text only** — they do not
replay DDL. Prefer a new forward migration when live objects must change.

### Remediation `.sql`

Path: `--sql-out` or `--out-dir/schematool_<design>_<engine>_<utc>.sql`.

**Hard rule:** every executable statement is **commented out** (`-- …`).
Do not pipe to a client unedited.

Typical blocks:

- Content drift → commented `UPDATE … SET code/name/summary`
- Missing LOAD → guidance to run Hydrogen AutoMigration
- Missing APPLY → guidance to run Hydrogen APPLY
- Orphan DB ref → commented `DELETE` (dangerous; review `.mig` first)
- Both 1000+1003 → commented cleanup of stray 1000

**Caveat:** updating `queries.code` does **not** replay DDL against live tables.
Prefer a **new forward migration** when the live schema must change.

### Orphan `.mig`

Plain-text capture of DB migration rows **not** present on disk (for authoring a
new `design_NNNN.lua` if those changes should be kept). Written when orphans
exist (`--mig-out` or under `--out-dir`).

## Exit codes

| Code | Meaning |
| ------ | --------- |
| 0 | All selected disk migrations pass |
| 1 | Hard error (usage, connection, missing tools, Lua failure) |
| 2 | Soft audit failure (missing LOAD/APPLY and/or content mismatch) |
| 3 | Anomalies (orphans, both 1000+1003, etc.), with or without drift |

## Connection and env precedence

CLI flags always win. For each empty field, env is chosen from the **requested**
`--engine` name **before** dialect aliasing (so `yugabytedb` does not steal
`ACURANZO_DB_*`):

1. **Requested-engine env**
   - `postgresql` / `postgres` → `ACURANZO_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`.
   - `yugabytedb` → `YUGABYTE_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
   - `mysql` → `MYSQL_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
   - `mariadb` → `MARIADB_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
   - `db2` → `HYDROTST_DB_{USER,NAME,PASS,SCHEMA}`
   - `firebird` → `FIREBIRD_DB_PATH_DEMO` (or `_TEST` / deprecated `FIREBIRD_DB_PATH`) + `FIREBIRD_SYSDBA_PASSWORD`
   - `mssql` → `MSSQL_DB_{HOST,PORT,USER,NAME}` and `MSSQL_SA_PASSWORD`. `sqlcmd` runs inside container `philement-mssql` and talks to localhost there. `--host` and `--port` fill readiness only.
2. **Generic** `SCHEMATOOL_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
3. Default ports: postgresql 5432, mysql 3306, mariadb 3306, mssql 1433

Password: prefer `--password-env VAR` (never printed; never written into `.sql`).

SQLite: `--database` is the file path (or `SCHEMATOOL_DB_NAME`); host/user unused.

Firebird: `--database` is the `.fdb` path (`FIREBIRD_DB_PATH_DEMO`, else
`FIREBIRD_DB_PATH_TEST`, else deprecated `FIREBIRD_DB_PATH`). The user is
`SYSDBA`. An empty schema is valid. A missing host does not force
disk-only mode. `isql-fb` reads `ISC_PASSWORD` from
`FIREBIRD_SYSDBA_PASSWORD`. That value is never printed.

### Test 40 convenience wrappers

| Wrapper | Schema | Primary credentials |
| --- | --- | --- |
| `schematool_postgresql_demo.sh` | `demo` | `ACURANZO_DB_*` |
| `schematool_mysql_demo.sh` | `demo` | `MYSQL_DB_*` |
| `schematool_sqlite_demo.sh` | _(empty)_ | `hydrodemo.sqlite` path |
| `schematool_db2_demo.sh` | `demo` | `HYDROTST_DB_*` + localhost:55555 |
| `schematool_mariadb_demo.sh` | `demo` | `MARIADB_DB_*` |
| `schematool_firebird_demo.sh` | _(empty)_ | `FIREBIRD_DB_PATH_DEMO` + `FIREBIRD_SYSDBA_PASSWORD` |
| `schematool_yugabytedb_demo.sh` | `demo` | **`YUGABYTE_DB_*`** (explicit flags; never ACURANZO) |
| `schematool_mssql_demo.sh` | `demoms` | `MSSQL_DB_*` + `MSSQL_SA_PASSWORD` (`sqlcmd` in `philement-mssql`) |

### Tests 32–39 wrappers

`schematool_<engine>_test.sh` points at the migration-test database.
`schematool_<engine>_demo.sh` is the Test 40 launcher.

| Wrapper | Schema or file | Primary credentials |
| --- | --- | --- |
| `schematool_postgresql_test.sh` | `test` | `ACURANZO_DB_*` |
| `schematool_mysql_test.sh` | `test` | `MYSQL_DB_*` |
| `schematool_sqlite_test.sh` | _(empty)_ | `hydrotst.sqlite` path |
| `schematool_db2_test.sh` | `test` | `HYDROTST_DB_*` + localhost:55555 |
| `schematool_mariadb_test.sh` | `test` | `MARIADB_DB_*` |
| `schematool_firebird_test.sh` | _(empty)_ | `FIREBIRD_DB_PATH_TEST` + `FIREBIRD_SYSDBA_PASSWORD` |
| `schematool_yugabytedb_test.sh` | `test` | **`YUGABYTE_DB_*`** |
| `schematool_mssql_test.sh` | `testms` | `MSSQL_DB_*` + `MSSQL_SA_PASSWORD` |

Multi-engine 1190 catalog smoke:

```bash
extras/schematool/smoke_test40_catalog.sh --out-dir /tmp/schematool-t40
# Eight `_demo` wrappers, including MSSQL. Firebird requires FIREBIRD_DB_PATH_DEMO.
```

## Normalization

| Mode | Behavior |
| ------ | ---------- |
| `loose` (default) | Unify newlines; trim trailing WS; collapse blanks |
| `strict` | Newline unify only |

## Optional flags

| Flag | Effect |
| ------ | -------- |
| `--from` / `--to` | Limit checklist/expect (DB dump stays full) |
| `--only-failures` | Checklist rows that are not fully OK |
| `--include-reverse` | Also compare type 1001 |
| `--include-diagram` | Also compare type 1002 (`code` often a stub) |
| `--include-ok-comments` | `-- OK: N` lines in `.sql` |
| `--normalize loose\|strict` | Comparison mode |
| `--format tables\|json\|both` | Console output |
| `--no-sql` | Skip remediation file |
| `--work-dir DIR` | Use DIR for intermediate JSON/detail/log files (default: a `mktemp -d` tmpdir; auto-cleaned unless `--keep-work-dir`) |
| `--keep-work-dir` | Do not remove `--work-dir` on exit |

## Cumulative ALTER caveat

If migration 1050 creates `t` and 1120 adds a column to `t`, live
`information_schema` for `t` will **not** match 1050’s CREATE alone.
SchemaTool v1 compares **stored migration text** to **Lua text** — the correct
signal for “someone edited 1050 after the fact.” It does not fail checks because
later ALTERs changed live objects.

## Engines

| Engine flag | Client | Notes |
| ------------- | -------- | ------- |
| `postgresql` | `psql` | Aliases: `postgres`, `yugabytedb`. |
| `mysql` | `mysql` | `MYSQL_DB_*`. Schema `demo` |
| `mariadb` | `mariadb` | `MARIADB_DB_*`. Schema `demo`. Expect receives `mariadb` |
| `sqlite` | `sqlite3` | No schema qualifier |
| `db2` | `db2` EXPORT LOBS | Schema often uppercase (`DEMO`) |
| `firebird` | `isql-fb` | File path. Empty schema is valid. No host. `BASE64_DECODE`/`BROTLI_DECOMPRESS` UDRs |
| `mssql` | `sqlcmd` in `philement-mssql` | Schema `demoms` or `testms` inside `MSSQL_DB_NAME`. `SELECT` only. |

## Safety (production checklist)

SchemaTool is intended to be safe against production databases when used as
documented. Confirm these before pointing at prod:

| Guard | Behavior |
| ------- | ---------- |
| No auto-apply | Tool never runs remediation SQL; `.sql` is 100% commented |
| Metadata I/O | `SELECT` on `queries` types 1000–1003 only |
| Catalog I/O | Targeted `information_schema` / `PRAGMA` / `SYSCAT` for in-scope tables |
| No row scans | Never `SELECT *` product tables |
| Secrets | `--password-env`; passwords never printed or written to artifacts |
| PG / YB / CRDB | `PGOPTIONS=-c default_transaction_read_only=on` on client sessions |
| MySQL / MariaDB | `SET SESSION TRANSACTION READ ONLY` before probes |
| SQLite | `sqlite3 -readonly` |
| DB2 | EXPORT LOBS read path only (no DML from SchemaTool) |
| MSSQL | `SELECT` via in-container `sqlcmd`. Password is not a process argument. Errors are scrubbed. |
| Wrong-host risk | Use correct `--engine` / wrapper / explicit `--host` — especially Yugabyte vs local PG |

**Operator tips for prod:**

1. Start with `--catalog --only-tables <few>` (cheap) before full-schema catalog.
2. Prefer a **read-only DB role** when the engine allows it (defense beyond client guards).
3. Review any `.sql` / `.mig` offline; never pipe unedited to a client.
4. Metadata exit 2 on known drift (e.g. 1280/1281 mail seeds) is audit signal, not a write.

## SchemaHelper

Interactive review of SchemaTool findings (dashboard, skip / accept,
migration packets) is
[`SchemaHelper`](/docs/H/tools/SCHEMAHELPER.md). SchemaTool itself stays
read-only.

## Related

- Active plan: [`SCHEMA_V2_PLAN.md`](/docs/H/plans/SCHEMA_V2_PLAN.md)
- Archived plan: [`SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md)
- Offline SQL gen: [`tests/lib/get_migration.lua`](/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua)
- Migration performance tests: `test_32`–`test_38`
- Migrations complete plan: [`MIGRATIONS_COMPLETE.md`](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md)
