# SchemaTool

Migration drift auditor for Hydrogen Lua migrations vs a live `queries` table.

**Full docs:** [`/docs/H/tools/SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md)  
**Plan:** [`/docs/H/plans/SCHEMATOOL_PLAN.md`](/docs/H/plans/SCHEMATOOL_PLAN.md)

## One-liner

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --from 1000 --to 1005 \
  --out-dir /tmp/schematool-out
```

Requires: `tables`, `jq`, `lua`, plus `sqlite3` / `psql` / `mysql` / `db2`.

## Layout

```text
schematool.sh              # CLI (v1.4+)
lua/
  schematool_discover.lua
  schematool_expect.lua
  schematool_normalize.lua
  schematool_compare.lua
  schematool_remediate.lua
db/
  query_pg.sh
  query_mysql.sh
  query_sqlite.sh
  query_db2.sh
testdata/
  expected_pg_demo_1000_1002.json
```

## Outputs

1. **stdout** — Hydrogen `tables` checklist (LOAD / L.match / APPLY / A.match)
2. **`.sql`** — fully commented remediation (never auto-executed)
3. **`.mig`** — plain-text orphan DB rows (when present)

Exit: `0` clean · `1` hard error · `2` drift/missing · `3` orphans/anomalies

## Env (when flags omitted)

| Engine | Primary env | Also |
| -------- | ------------- | ------ |
| postgresql | `ACURANZO_DB_*` | `SCHEMATOOL_DB_*` |
| mysql | `CANVAS_DB_*` | `SCHEMATOOL_DB_*` |
| db2 | `HYDROTST_DB_*` | `SCHEMATOOL_DB_*` |
| sqlite | `--database` path | `SCHEMATOOL_DB_NAME` as path |

Password: `--password-env VAR` preferred (never printed).

## Safety

Read-only. Remediation SQL is 100% commented. Updating `queries.code` does not
replay DDL — prefer a new forward migration for live schema fixes.
