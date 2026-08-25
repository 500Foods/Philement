# SchemaTool

Migration drift auditor for Hydrogen Lua migrations vs a live database.

**Two tracks:** metadata (`queries` text vs Lua) and catalog (`--catalog` live
object shape vs folded applied DDL).

**Full docs:** [`/docs/H/tools/SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md)  
**Plan:** [`/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md)  
**SchemaHelper:** `schemahelper.sh` —
[`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md)
(plan: [`/docs/H/plans/SCHEMAHELPER.md`](/docs/H/plans/SCHEMAHELPER.md))

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

Requires: `tables`, `jq`, `lua`, `xxd` (MySQL/DB2 HEX), plus `sqlite3` / `psql` / `mysql` / `db2`.

## Row Grouping

By default, the checklist table inserts a horizontal separator after every 20
rows to make long tables easier to scan. Override with `--group-size N`
(`0` disables grouping entirely):

```bash
extras/schematool/schematool.sh \
  --migrations "$HELIUM_ROOT/acuranzo/migrations" \
  --design acuranzo \
  --engine sqlite \
  --database "$HYDROGEN_ROOT/tests/artifacts/database/sqlite/hydrodemo.sqlite" \
  --group-size 50
```

## Layout

```text
schematool.sh                 # CLI entry — help + parameter handling
schemahelper.sh               # SchemaHelper launcher (Lua 5.5 + terminal.lua)
schemahelper.lua              # SchemaHelper TUI (review + packets + apply)
lib/
  schematool_init.sh          # dependency checks + command path resolution
  schematool_runners.sh       # db/Lua adapter wrappers (query/catalog adapters, Lua extractors)
  schematool_audit.sh         # audit mode dispatch + orchestration (dump/emit/audits)
  schematool_render.sh        # tables rendering + render dispatch
lua/
  schemahelper_const.lua        # VERSION + terminal module refs + attribute tables
  schemahelper_ui.lua           # shared mutable UI state (hotspots, mouseover)
  schemahelper_mouse.lua        # SGR 1006 mouse parsing + hotspot mapping
  schemahelper_paint.lua        # low-level terminal painting + hotspot recording
  schemahelper_wrappers.lua     # wrapper discovery/metadata + path helpers
  schemahelper_invoke.lua       # SchemaTool invocation, progress, connect text
  schemahelper_explore.lua      # explore-mode line wrapping + cursor nav
  schemahelper_screens.lua      # per-screen content painters
  schemahelper_actions.lua      # apply / generate-packet / promote + queue lifecycle
  schemahelper_queue.lua        # findings merge (orchestrator over q* submodules)
  schemahelper_qutil.lua        # pure text/json helpers
  schemahelper_qstate.lua       # sidecar state load/create/update
  schemahelper_qload.lua        # metadata + catalog findings ingest
  schemahelper_qdecode.lua      # brotli/base64 decode + decode view
  schemahelper_connect.lua
  schemahelper_packet.lua
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
  common.sh
testdata/
  expected_pg_demo_1000_1002.json
```

## Outputs

1. **stdout** — Hydrogen `tables` checklist (metadata and/or catalog)
2. **stdout detail** — after the table on failures: field diffs (− DB / + Lua) and commented remediation SQL (disable with `--no-detail`)
3. **`.sql`** — fully commented remediation (metadata track; never auto-executed)
4. **`.mig`** — plain-text orphan DB rows (when present)
5. **catalog_*.json** / `finding_detail.txt` under `--out-dir` when used

Exit: `0` clean · `1` hard error · `2` drift/missing · `3` orphans/anomalies  
(When both tracks run: worst-wins.)

## Env (when flags omitted)

Chosen from **requested** `--engine` (before alias):

| Requested engine | Primary env | Also |
| ------------------ | ------------- | ------ |
| postgresql / cockroachdb | `ACURANZO_DB_*` | `SCHEMATOOL_DB_*` |
| yugabytedb | `YUGABYTE_DB_*` | `SCHEMATOOL_DB_*` |
| mysql / mariadb | `CANVAS_DB_*` | `SCHEMATOOL_DB_*` |
| db2 | `HYDROTST_DB_*` | `SCHEMATOOL_DB_*` |
| sqlite | `--database` path | `SCHEMATOOL_DB_NAME` as path |

Password: `--password-env VAR` preferred (never printed).

Test 40 wrappers: `schematool_{postgresql,mysql,mariadb,sqlite,db2,cockroachdb,yugabytedb}.sh`  
Smoke (all 7, 1190 catalog): `./smoke_test40_catalog.sh`

## Safety

Read-only client guards (PG `default_transaction_read_only`, MySQL session
read-only, SQLite `-readonly`). Remediation SQL is 100% commented. Catalog uses
**targeted probes** (not full-DB dumps). Updating `queries.code` does not replay
DDL — prefer a new forward migration for live schema fixes. See
[`SCHEMATOOL.md` Safety](/docs/H/tools/SCHEMATOOL.md) before production use.
