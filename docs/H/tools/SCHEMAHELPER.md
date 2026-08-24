# SchemaHelper — Interactive SchemaTool Front-End

Lua **5.5** TUI under
[`extras/schematool/`](/elements/001-hydrogen/hydrogen/extras/schematool/).
It sits in front of the read-only SchemaTool auditor and turns a batch of
drift findings into operator decisions.

Implementation plan:
[`/docs/H/plans/SCHEMAHELPER.md`](/docs/H/plans/SCHEMAHELPER.md).

SchemaTool operator guide:
[`/docs/H/tools/SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md).

## Purpose

SchemaTool answers “what is different?” SchemaHelper walks those leftovers
one by one:

| Need | Action |
| --- | --- |
| See the whole picture | Dashboard: migrations found / perfect / accepted / findings for review |
| Look closer | `[e]` explore (paged detail) |
| Not now | `[s]` skip for now (still subject next launch) |
| Known divergence | `[a]` accept permanent variance (sidecar) |
| Live DB should become a migration | `[g]` reserve next ref and write a packet |
| Official Lua should win | `[u]` update one field (`--allow-write`, type `REF.field`) |
| DB ref is a keeper, not a refix | `[u]` delete an orphan ref (`--allow-write`, type `REF`; true orphans only) |
| Live catalog shape drifted | `[u]` apply catalog DDL (`--allow-write`, type `object.column`; nullable→SET/DROP NOT NULL, missing col→ADD COLUMN) |
| Need a Helium migration | `[g]` reserve next ref and write a packet, then `[m]` promote to `design_NNNN.lua` stub |

## What it is / is not

| Is | Is not |
| --- | --- |
| Interactive review queue | A second auditor |
| Packet writer (`NNNN` reserved) | Author of `acuranzo_NNNN.lua` |
| Sidecar of skip / accept / packet | A Hydrogen subsystem or REST API |
| Default read-only | Auto-apply of SchemaTool `.sql` |

## Requirements

- Lua **5.5** and the `terminal` rock (`luasystem`; built-in `utf8`)
- `jq`, plus SchemaTool’s own deps (`tables`, engine client, `lua-brotli`)
- A tty (stdout must be a terminal)

```bash
luarocks --lua-version=5.5 install luasystem
luarocks --lua-version=5.5 install terminal --nodeps
```

## Quick start

SQLite is the usual local path (Test 40 `hydrodemo.sqlite`):

```bash
extras/schematool/schemahelper.sh schematool_sqlite.sh
```

Or pick a wrapper after the splash:

```bash
extras/schematool/schemahelper.sh
```

Reuse an existing workspace without invoking SchemaTool:

```bash
extras/schematool/schemahelper.sh schematool_sqlite.sh \
  --reuse --out-dir /tmp/schemahelper-out
```

## Wrappers

`schemahelper.sh` discovers `extras/schematool/schematool_*.sh`. Each
wrapper is an engine + env family:

| Wrapper | Env family | Typical local target |
| --- | --- | --- |
| `schematool_sqlite.sh` | file | `tests/artifacts/database/sqlite/hydrodemo.sqlite` |
| `schematool_postgresql.sh` | `ACURANZO_DB_*` | schema `demo` |
| `schematool_mysql.sh` | `CANVAS_DB_*` | schema `demo` |
| `schematool_mariadb.sh` | `CANVAS_DB_*` | schema `demomrdb` |
| `schematool_db2.sh` | `HYDROTST_DB_*` | `localhost:55555` / `HYDROTST` |
| `schematool_cockroachdb.sh` | `ACURANZO_DB_*` | schema `democrdb` |
| `schematool_yugabytedb.sh` | `YUGABYTE_DB_*` | never `ACURANZO_DB_*` |

A failed ping does **not** start SchemaTool. Press `[w]` to pick another
wrapper, `[q]` to quit, or Enter to review artifacts already in
`--out-dir`. Custom wrappers (any `schematool_*.sh` path) are probed from the
expanded `exec` line (so jq-computed host/user/database/password-env
work). Sidecar names still use the filename stem. Passwords stay in
the wrapper process and are never printed.

If only SQLite is up, pick `schematool_sqlite.sh`. The other wrappers
need that engine listening and the matching env vars (see
[SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md)).

## Flags

| Flag | Meaning |
| --- | --- |
| `--wrapper PATH` | Same as the positional wrapper |
| `--migrations DIR` | Override Helium migrations (default acuranzo tree) |
| `--out-dir DIR` | SchemaTool workspace (default: directory of wrapper) |
| `--state-file PATH` | Sidecar JSON override |
| `--packet-dir DIR` | Packet workspace (default: same as `--out-dir`) |
| `--ref N` | Force the next packet number |
| `--track metadata\|catalog\|both` | Queue filter (default `both`) |
| `--reuse` | Load existing artifacts; skip SchemaTool |
| `--allow-write` | Enable `[u]` apply (metadata / orphan / catalog DDL) and `[m]` promote |

Default `--out-dir` next to a Test 40 wrapper is inside the git tree.
SchemaHelper warns once. Prefer `--out-dir /tmp/…` for real sessions.

## Screens and keys

1. **Splash** — versions. Enter continues, Esc exits.
2. **Wrapper picker** — if no wrapper was passed. Up/down, Enter, Esc.
3. **Connect / SchemaTool** — live ping, then audit unless `--reuse`.
   Expect prints `expect N/M ref R` to the log; the running screen shows
   a progress bar from those lines.
4. **Dashboard** — migration totals, then **findings for review** (one
    per drifted field plus catalog rows; not a migration count) +
    variance classes + reserved packet refs. If the catalog track fails
    after a successful metadata compare, the dashboard still opens on
    metadata findings (warning on the dashboard).
5. **Review** — one field at a time. A SchemaTool drift with
   `code`+`name` is two items. Explore shows the stored text; Enter on a
   `BROTLI`/`CRYPTO` line opens the decoded payload (both sides, or one).

| Key | Where | Effect |
| --- | --- | --- |
| Enter | Dashboard | Begin review |
| `r` | Dashboard / review | Re-run SchemaTool; sidecar decisions kept |
| `q` / Esc | Most screens | Back or quit |
| `e` | Review | Explore one field: Migration vs Database |
| `s` | Review | Skip for now |
| `a` | Review | Accept permanent variance |
| `u` | Review | Update a field (`--allow-write`; type `1223.code`) OR delete an orphan ref (type `1290`; true orphans only; anomalies refused) OR apply catalog DDL (type `accounts.id`; nullable/add-column only) |
| `g` | Review | Generate a migration packet |
| `m` | Review | Promote current packet to a `design_NNNN.lua` stub in Helium (`--allow-write`; requires an existing packet) |
| `n` / `p` | Review | Next / previous |
| `j` / `k` / arrows | Explore | Move line highlight (both panes) |
| Enter | Explore | Decode highlighted brotli/crypto line |
| PgUp / PgDn | Explore | Page by line |
| Enter | Packet note | Write packet (empty note is OK) |
| Esc | Packet note | Cancel packet |

## Sidecar

Selections live in
`<out-dir>/schemahelper_<design>_<engine>.json` (no timestamp). Several
engines can share one folder. Actions: `skipped`, `accepted`, `applied`,
`packet`. No passwords and no full `code` blobs.

Skipped items stay in **findings for review**. Accepted / packet /
applied drop out of the 1-by-1 queue.

## Packets

`[g]` reserves `NNNN = max(disk {design}_NNNN.lua, reserved packets) + 1`
unless `--ref N` is set. Collision with an existing migration file or
packet directory is refused.

```text
<packet-dir>/schemahelper_acuranzo_sqlite_1291/
  MANIFEST.json
  PACKET.md
  FINDING.json
  DETAIL.txt
  SUGGESTED.sql
```

A packet is **not** a migration. `SUGGESTED.sql` is review-only and is
never applied. Confirm the number is free before promoting into Helium.

`[m]` (with `--allow-write`) writes a `design_NNNN.lua` **stub** from the
packet's `SUGGESTED.sql`. The stub is not a full Lua author — a human must
complete the INSERT-into-queries pattern before loading. The packet's
`MANIFEST.json` status flips `reserved`→`promoted`.

## Safety

- SchemaTool stays read-only. SchemaHelper writes only with
  `--allow-write`, after a typed confirm, and only one finding at a time.
  Two write shapes exist:
  - **Update database `[u]`** — one metadata field (`code` / `name` /
     `summary`) from official Lua. Confirm is `REF.field` (e.g. `1223.code`).
  - **Delete orphan `[u]`** — removes a true orphan ref from `queries`
     (`DELETE WHERE query_ref = N AND query_type_a28 BETWEEN 1000 AND 1003`,
     matching SchemaTool remediation). Confirm is the bare `REF`
     (e.g. `1290`). Anomalies (1000+1003 on disk) are **not** deletable —
     review and skip instead.
  - **Apply catalog DDL `[u]`** — single-statement `ALTER TABLE` on a
     `nullable` mismatch (SET/DROP NOT NULL) or `missing column`
     (ADD COLUMN, type from expected fold). Confirm is `object.column`
     (e.g. `accounts.id`) — louder than `REF.field`. Refused on missing
     tables, live-only extras, and extra columns.
  Catalog, missing LOAD/APPLY, and decoded views are refused. A metadata
  update does not replay DDL.
- Secrets inherit SchemaTool `--password-env`; never printed; never
  written into packets or the sidecar.
- Catalog DDL apply is limited to nullable and add-column; other catalog
  checks are refused. Missing LOAD/APPLY is guidance to run Hydrogen
  AutoMigration, not a helper `UPDATE`.

## Related

- Plan: [`SCHEMAHELPER.md`](/docs/H/plans/SCHEMAHELPER.md)
- Auditor: [`SCHEMATOOL.md`](/docs/H/tools/SCHEMATOOL.md)
- Extras quick start:
  [`extras/schematool/README.md`](/elements/001-hydrogen/hydrogen/extras/schematool/README.md)
