# SchemaTool — Migration Drift Auditor

Standalone **Bash + Lua** operator utility under
[`extras/schematool/`](/elements/001-hydrogen/hydrogen/extras/schematool/).
Compares the **on-disk Lua migration set** to a **running database** `queries`
table and reports whether each migration was LOADed/APPLYed and whether stored
`code` / `name` / `summary` still match the Lua sources.

Quick start (local extras README):
[`extras/schematool/README.md`](/elements/001-hydrogen/hydrogen/extras/schematool/README.md).

Implementation plan:
[`/docs/H/plans/SCHEMATOOL_PLAN.md`](/docs/H/plans/SCHEMATOOL_PLAN.md).

## Purpose

Hydrogen LOAD/APPLY advances schema well, but does **not** detect
**historical edit drift**: someone changes an already-shipped
`design_NNNN.lua` instead of adding `NNNN+1`. Existing databases keep the old
payload in `queries.code`. SchemaTool surfaces that class of problem with a
per-migration checklist.

## What it is / is not

| Is | Is not |
| ---- | -------- |
| Read-only auditor (native clients) | Replacement for Hydrogen LOAD/APPLY |
| Metadata fidelity (types 1000–1003) | Full live-schema / `DESCRIBE` (backlog) |
| Console checklist via `tables` | Auto-executor of remediation SQL |
| Commented `.sql` + orphan `.mig` | Full-DB dump tools (`pg_dump`, etc.) |
| Bash + Lua under `extras/` | C code in `src/` or a REST endpoint |

## Requirements

- `tables`, `jq`, `lua` (and `lua-brotli` for compressed migration payloads)
- Engine client for the target: `psql` / `mysql` / `sqlite3` / `db2`
- Migrations folder containing `database.lua` and `design_NNNN.lua`

## Quick start

```bash
# From hydrogen root (or any cwd with absolute paths)
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --from 1000 --to 1005 \
  --out-dir /tmp/schematool-out
```

PostgreSQL (env fallbacks `ACURANZO_DB_*`):

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
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

CLI flags always win. For each empty field:

1. **Engine-specific env**
   - postgresql → `ACURANZO_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
   - mysql → `CANVAS_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
   - db2 → `HYDROTST_DB_{USER,NAME,PASS,SCHEMA}`
2. **Generic** `SCHEMATOOL_DB_{HOST,PORT,USER,NAME,PASS,SCHEMA}`
3. Default ports: postgresql 5432, mysql 3306

Password: prefer `--password-env VAR` (never printed; never written into `.sql`).

SQLite: `--database` is the file path (or `SCHEMATOOL_DB_NAME`); host/user unused.

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

## Cumulative ALTER caveat

If migration 1050 creates `t` and 1120 adds a column to `t`, live
`information_schema` for `t` will **not** match 1050’s CREATE alone.
SchemaTool v1 compares **stored migration text** to **Lua text** — the correct
signal for “someone edited 1050 after the fact.” It does not fail checks because
later ALTERs changed live objects.

## Engines

| Engine flag | Client | Notes |
| ------------- | -------- | ------- |
| `postgresql` | `psql` | Aliases: `postgres`, `cockroachdb`, `yugabytedb` |
| `mysql` | `mysql` | Alias: `mariadb` |
| `sqlite` | `sqlite3` | No schema qualifier |
| `db2` | `db2` EXPORT LOBS | Schema often uppercase (`DEMO`) |

## Safety

- Read-only SELECTs / catalog probes only — SchemaTool never applies DDL/DML
- Remediation `.sql` is 100% commented (linted at generation time)
- Never embeds passwords in artifacts
- Does not scan product table row data

## Related

- Plan: [`SCHEMATOOL_PLAN.md`](/docs/H/plans/SCHEMATOOL_PLAN.md)
- Offline SQL gen: [`tests/lib/get_migration.lua`](/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua)
- Migration performance tests: `test_32`–`test_38`
- Migrations complete plan: [`MIGRATIONS_COMPLETE.md`](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md)
