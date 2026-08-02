# SchemaTool

Migration drift auditor for Hydrogen Lua migrations vs a live database.

**Two tracks:** metadata (`queries` text vs Lua) and catalog (`--catalog` live
object shape vs folded applied DDL).

**Full docs:** [`/docs/H/tools/SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md)  
**Plan:** [`/docs/H/plans/SCHEMATOOL_PLAN.md`](/docs/H/plans/SCHEMATOOL_PLAN.md)

## One-liner (metadata)

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --from 1000 --to 1005 \
  --out-dir /tmp/schematool-out
```

## Catalog (live shape — e.g. 1190 nullability)

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --catalog --only-tables accounts \
  --out-dir /tmp/schematool-cat --no-sql
```

Requires: `tables`, `jq`, `lua`, plus `sqlite3` / `psql` / `mysql` / `db2`.

## Layout

```text
schematool.sh                 # CLI (v1.5+)
lua/
  schematool_discover.lua
  schematool_expect.lua
  schematool_normalize.lua
  schematool_compare.lua
  schematool_remediate.lua
  schematool_catalog_fold.lua
  schematool_catalog_compare.lua
db/
  query_{pg,mysql,sqlite,db2}.sh
  catalog_{pg,mysql,sqlite,db2}.sh
testdata/
  expected_pg_demo_1000_1002.json
```

## Outputs

1. **stdout** — Hydrogen `tables` checklist (metadata and/or catalog)
2. **`.sql`** — fully commented remediation (metadata track; never auto-executed)
3. **`.mig`** — plain-text orphan DB rows (when present)
4. **catalog_*.json** under `--out-dir` when `--catalog` is used

Exit: `0` clean · `1` hard error · `2` drift/missing · `3` orphans/anomalies  
(When both tracks run: worst-wins.)

## Env (when flags omitted)

| Engine | Primary env | Also |
| -------- | ------------- | ------ |
| postgresql | `ACURANZO_DB_*` | `SCHEMATOOL_DB_*` |
| mysql | `CANVAS_DB_*` | `SCHEMATOOL_DB_*` |
| db2 | `HYDROTST_DB_*` | `SCHEMATOOL_DB_*` |
| sqlite | `--database` path | `SCHEMATOOL_DB_NAME` as path |

Password: `--password-env VAR` preferred (never printed).

## Safety

Read-only. Remediation SQL is 100% commented. Catalog uses **targeted probes**
(not full-DB dumps). Updating `queries.code` does not replay DDL — prefer a new
forward migration for live schema fixes.
