<!-- markdownlint-disable MD007 MD024 -->
# Microsoft SQL Server Engine Plan

## Status at a glance

**New plan (2026-09-18).** Phase 0 locks not approved. Do not write C.
Lookup 030 **key 5** is already `MS SQL Server` (`acuranzo_1055.lua`).
This plan implements that dialect. It does **not** replace Cockroach
(that is [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) / Test 37).

Fedora does not package `mssql-server`. The local path is the official
**Linux** container (`mcr.microsoft.com/mssql/server`) under Podman on
this Fedora box — not Windows, not Azure, not DOKS unless Phase 1
proves local RAM/image cannot run.

| Phase | Status | Remaining |
| --- | --- | --- |
| 0 Contract lock | pending | **Quick** |
| 1 Fedora Podman SQL Server + ODBC | pending | **Moderate** |
| 2 Helium dialect | pending | **Moderate** |
| 3 C register / connect (unixODBC) | pending | **Moderate** |
| 4 T-SQL helpers + Brotli CLR | pending | **Difficult** |
| 5 Test 39 full Acuranzo | pending | **Difficult** |
| 6 SchemaTool / flush | pending | **Moderate** |
| 7 Grow matrix 7 → 8 | pending | **Difficult** |
| 8 Docs | pending | **Quick** |
| 9 Coverage / completeness | pending | **Moderate** |

Remaining: 3 Difficult (4, 5, 7), 5 Moderate (1, 2, 3, 6, 9), 2 Quick
(0, 8).

**Parity:** MSSQL is a Hydrogen `DatabaseEngineInterface`, not a new
API. Match PostgreSQL / SQLite / MySQL / DB2: same `QueryRequest` /
`QueryResult`, same `parse_typed_parameters` →
`convert_named_to_positional` → bind, same `data_json` array of row
objects. Do not change those engines. Do not interpret SQL in C
(one documented exception: trailing `RETURNING` → `OUTPUT INSERTED`,
lock 21).

**Sister plan:** [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) (key 6,
Test 37, Cockroach retirement, Firebase teardown). Shared enum lock is
in [Coordination with Firebird](#coordination-with-firebird).

## Purpose

Implement the **MS SQL Server** Helium dialect that Lookup 030 key 5
has advertised since `acuranzo_1055.lua`, as a real sixth C engine
(`src/database/mssql/`) talking TDS through **unixODBC + Microsoft
ODBC Driver 18**.

This is additive. After Firebird replaces Cockroach the operator matrix
is still **7** (SQLite, PostgreSQL, MySQL, MariaDB, DB2, Firebird,
Yugabyte). This plan grows it to **8**. Test number **39** (37 is
Firebird). Do not steal Test 37.

**There is no v1 subset.** AutoMigrations must apply the same Acuranzo
Lua files as Test 32. Helium stays SQL. Hydrogen sends that SQL to
SQL Server. Helpers SQL Server lacks (Brotli; convenient Base64) are
T-SQL functions and one extras CLR assembly — not a Hydrogen SQL VM.

**Free, Linux, Fedora-local:**

1. **Preferred:** Podman + official Linux image
   `mcr.microsoft.com/mssql/server:2022-latest`,
   `MSSQL_PID=Developer` (free for development/test, not production).
   This box already has Podman 5.8.4. Fedora 43.
2. **Client:** Fedora `unixODBC` / `unixODBC-devel` plus Microsoft
   `msodbcsql18` (RHEL 9 packages; Phase 1 must prove they install on
   Fedora 43).
3. **Not preferred:** DOKS Linux node. Festival nodes are already
   RAM-bound with Yugabyte. Use only if Phase 1 records that the
   container cannot run locally (SQL Server wants ≥2 GiB).
4. **Forbidden:** Windows SQL Server, paid Azure SQL as the CI engine,
   Babelfish (another PostgreSQL alias — the Cockroach pattern).

This is the only active plan for MSSQL.

**Session brief:** Open [Status at a glance](#status-at-a-glance), then
[Resuming Work](#resuming-work), then the next incomplete phase only.

History that this plan does **not** reopen: same completed database /
binding / migrations / SchemaTool plans listed in FIREBIRD.md.
Firestore attempt:
[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status, append Working Log, record lessons,
  **stop for review**.
- Build aliases: `mkq` / `mkt` / `mku` / `mkp` / `mks` / `mkl` as in
  [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**:

1. Confirm the prior phase Status is actually complete.
2. Discuss the current phase first (Goal + Work items + Done means +
   Exit gate). Research before writing code.
3. Get explicit approval before editing source.
4. Ask questions as they come up.
5. Update the Working Log when major pieces land.
6. Record lessons learned.
7. Mark `[x]` / Status complete only after verification commands ran.
8. Never apply a database migration; hand packets to the user.
9. Project norms: no `static` in `src/`; Unity one file per function;
   `jq` in Bash; absolute Markdown links; completeness + coverage
   fences.
10. Never log SA passwords, password hashes, JWTs.
11. Do not increment `TEST_COUNTER`.
12. Do not retire Cockroach and do not delete Firebase — those belong
    to [`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md).
13. Mirror the other engines. Do not change PG / SQLite / MySQL / DB2
    to accommodate MSSQL.
14. Do not interpret SQL in C except the documented `RETURNING` →
    `OUTPUT INSERTED` rewrite (lock 21).

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-18):** Phase 0 not approved. No C,
no Helium packet, no Podman SQL Server on this box yet. Fedora 43;
`unixODBC` / `msodbcsql18` / `freetds` **not installed**; Podman 5.8.4
present. Last numbered Acuranzo file: `acuranzo_1383.lua`. Lookup 030
key 5 is already MS SQL Server (1055). Next free id **1384** is owned
by FIREBIRD (key 6 relabel) unless disk says otherwise. Sister plan
Firebird is also at Phase 0.

### Resume here next session

1. This file is the source of truth for MSSQL. Do not start a second
   “add SQL Server” plan. Do not implement Firebird or Firebase teardown
   from here.
2. Read Status at a glance, CURRENT PAUSE POINT, Phase Index Status,
   Working Log.
3. Confirm prior phase Exit gate.
4. Re-read **only** the next phase.
5. Re-check disk before any Helium packet (`acuranzo_*.lua`).
6. Discuss, get approval, implement that phase only, verify, update
   this plan, **stop**.

### Session checklist

1. CURRENT PAUSE POINT → first Status that is not complete.
2. Working Log decisions for this phase (especially Phase 1: container
   vs DOKS, ODBC 18 vs FreeTDS).
3. Baseline as the phase names it.
4. One phase: questions → approval → implement → verify → update → stop.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new engine, after Auth Finale; parallel with Firebird, not a substitute |
| **Effort** | XL (unixODBC engine + Helium dialect + T-SQL/CLR extras + Test 39 + 8-engine matrix) |
| **Done** | 0% — plan authored, Phase 0 not approved |
| **Why this shape** | Key 5 has been a lookup row without a C engine. Fedora has no mssql-server RPM; the official Linux container is the local free path. |
| **Do not start casually** | Touches enum (reserved slot), registry, DQM, Helium four designs, Test 31/39, every 7-engine loop (becomes 8), SchemaTool. |

Backlog: [TODO.md item 28](/docs/H/TODO.md).

## Coordination with Firebird

[`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md) owns key **6**, Test **37**,
Cockroach retirement, and Firebase teardown.

**Final C enum** (both plans lock this):

```c
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_MSSQL,      // key 5 — this plan
    DB_ENGINE_FIREBIRD,   // key 6 — FIREBIRD.md
    DB_ENGINE_AI,
    DB_ENGINE_MAX
} DatabaseEngine;
```

If Firebird Phase 5 lands first, it may introduce `DB_ENGINE_MSSQL` as
an unused enumerator. This plan **fills** that slot; it does not insert
and shift Firebird. If this plan’s Phase 3 lands first, add both names
in the final order and leave `firebird_get_interface` to FIREBIRD.md
(no dead mssql/firebird symbols from the other plan).

Shared files are **additive only**. Test 31 `ENGINES` gains `mssql`
without removing `firebird`. `lua.c` `engines[]` likewise.

Test **39** is MSSQL migrations. Ports **539x**. `TEST_ABBR` **MSQ**.

After both plans: 6 real engines (PG, SQLite, MySQL, DB2, MSSQL,
Firebird) + MariaDB + Yugabyte = **8** operator names.

## Snapshot: what exists today (2026-09-18)

### Engines

See FIREBIRD.md snapshot. MSSQL is **not** in C. Dialect id 5 exists
only as a Lookup row and `sql_dialect_mssql.png`.

### This machine (Phase 1 baseline)

| Fact | Value |
| --- | --- |
| Distro | Fedora 43 |
| `podman` | 5.8.4 |
| `unixODBC` / `unixODBC-devel` | not installed (Fedora packages exist) |
| `msodbcsql18` / `mssql-tools18` | not installed (not in Fedora repos) |
| `freetds` | not installed (Fedora package exists; **fallback client only**) |
| `mssql-server` RPM | **not in Fedora** |

### How Cockroach is irrelevant here

Cockroach stays until FIREBIRD Phase 8. This plan never deletes
`test_37_cockroachdb_*`. It adds Test 39 beside 32–38.

### How the other engines get Base64 / Brotli / SHA-256 / TZ

| Need | SQL Server 2022 Linux (this plan) |
| --- | --- |
| Base64 | **not native** — T-SQL helper in `acuranzo_1000` (XML `xs:base64Binary`) or extras |
| Brotli | **not native; no `.so` UDF** — SQL CLR assembly in `extras/brotli_udf_mssql/` (Linux CLR) |
| SHA-256 | native `HASHBYTES('SHA2_256', …)` then Base64 helper |
| `json_ingest` | T-SQL function (control-char walk), store `NVARCHAR(MAX)` |
| JSON extract | native `JSON_VALUE` (2016+) |
| `CONVERT_TZ` | `AT TIME ZONE` |

SQL Server on Linux **cannot** load the C extras UDFs the other engines
use. That is headache 3.

Closest C sibling: **DB2** (`src/database/db2/` already speaks
ODBC-shaped `SQLHANDLE` / `SQLExecDirect`). MSSQL is unixODBC +
`libmsodbcsql-18.so` instead of libdb2.

## The four solvable headaches

### 1. JSON ingest and extract

Store `${JSON}` as `NVARCHAR(MAX)` text (SQLite-shaped, not a MSSQL
`JSON` type). Ingest: T-SQL function, same control-char contract,
`$ref` accepted. Extract: `JSON_VALUE(col, '$.icon')`. Missing path →
NULL (`JSON_VALUE` returns NULL). `${JSON_INGEST_SCHEMA_*}` aliases
ingest.

### 2. Base64

No `BASE64_ENCODE` / `FROM_BASE64`. Phase 4 lands
`${SCHEMA}base64_encode` / `base64_decode` as T-SQL (xml method) so
`${BASE64_*}` and `${COMPRESS_*}` have something to call. Alphabet
`+/` with padding.

### 3. Brotli (the expensive one)

`${COMPRESS_START}` must decompress on INSERT. Options, in order:

1. **SQL CLR** extras assembly wrapping `libbrotlidec`,
   `CREATE ASSEMBLY` + `CREATE FUNCTION brotli_decompress`. Developer
   edition in the Linux container must allow CLR. Phase 4 proves this
   **inside the Podman container**.
2. If CLR-on-Linux-in-container fails: **narrow Hydrogen pre-eval** of
   only the `${COMPRESS_*}` wrapper (replace with a string literal
   before `SQLExecDirect`). Not a SQL interpreter. Record as a Status
   variance and lock it.
3. Do **not** skip compression in Helium for mssql only unless (2)
   also fails — that would store wrapped blobs and break APPLY.

Quality 11 on the Lua compressor stays. Engine/CLR only decompresses.

### 4. SHA-256 password hashes

```text
base64(HASHBYTES('SHA2_256', CONCAT(N'0', N'<password>')))
```

Bytes must match SQLite
`CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` for `"0"` +
`"testpass"`. Watch NVARCHAR vs VARCHAR / UTF-16 vs UTF-8: **hash the
UTF-8 bytes** of the concatenated strings, same as the other engines.
If `CONCAT` + `HASHBYTES` uses UTF-16, the T-SQL helper must convert
to UTF-8 (`CAST … AS VARCHAR` with a UTF-8 collation, or
`COMPRESS`/`CAST` trick) before `HASHBYTES`. This is a login
compatibility gate.

## SQL Server installation (CI / dev)

| Piece | Why | Notes |
| --- | --- | --- |
| Podman | run official Linux SQL Server | already on this box |
| `mcr.microsoft.com/mssql/server:2022-latest` | engine | `ACCEPT_EULA=Y`, `MSSQL_PID=Developer` |
| ≥2 GiB RAM for the container | Microsoft minimum | prefer 4 GiB |
| Port 1433 | TDS | Test 39 uses **539x** for Hydrogen; SQL Server stays 1433 or a documented extras port |
| `unixODBC` + `unixODBC-devel` | C client | Fedora RPMs |
| `msodbcsql18` | Microsoft ODBC Driver 18 | RHEL 9 repo/rpm on Fedora — **Phase 1 proof** |
| `mssql-tools18` (`sqlcmd`) | extras create-db / health | optional if `sqlcmd` inside the container is enough |

### extras layout (Phase 1)

```text
elements/001-hydrogen/hydrogen/extras/
  mssql_server/
    README.md       # podman pull/run, EULA, memory, ODBC 18 on Fedora
    start.sh        # idempotent container start, wait for 1433
    stop.sh         # stop only if we started it
    create_test_db.sh
```

Do **not** install SQL Server as a host RPM. Do **not** require Docker
Engine if Podman works.

### DOKS fallback (Phase 1 Status only)

If local Podman cannot keep SQL Server healthy (OOM, cgroup), record
the failure and a DOKS Linux pod spec (not Windows). Do not start DOKS
work in the same turn as the local attempt. Festival 8 GiB nodes with
Yugabyte are a last resort.

## Why Helium still emits SQL

Same as Firebird: Acuranzo files are SQL. `database_mssql.lua` is a
complete macro table.

`if engine` files must grow a **mssql** arm:

| File | Why |
| --- | --- |
| `acuranzo_1000.lua` | T-SQL helpers + CLR `CREATE ASSEMBLY` |
| `acuranzo_1190.lua` | ALTER COLUMN — SQL Server ALTER is capable; dedicated arm likely |
| `acuranzo_1135.lua` | JSON constructors — `JSON_OBJECT` / `JSON_VALUE` |
| `acuranzo_1151.lua` | `engine ~= 'mysql'` already covers mssql |
| QueryRefs with `\|\|` concat | SQL Server is `+` / `CONCAT` — grep in Phase 2 |
| 1168 `LATERAL` / `jsonb_agg` | SQL Server `OUTER APPLY`; dedicated arm |

Phase 2 greps `if engine`, `||`, `LIMIT`, `RETURNING`, `LATERAL`,
`TRUE`/`FALSE`.

## `database_mssql.lua` — complete macro table

Every key in the union of the four current dialect files must exist.
Values below are **proposed**.

`${SCHEMA}` is a SQL Server schema prefix with a dot (`testms.`), like
PostgreSQL, **not** Firebird-empty and **not** firebase underscore.
`database.lua` `replace_query` already does `name.` for non-firebase
engines; mssql rides that path. DB2 still uppercases.

### Types

| Macro | MSSQL spelling | Notes |
| --- | --- | --- |
| `${INTEGER}` / `${INTEGER_SMALL}` | `INT` / `SMALLINT` | |
| `${INTEGER_BIG}` | `BIGINT` | |
| `${FLOAT}` / `${FLOAT_BIG}` | `REAL` / `FLOAT` | |
| `${TEXT}` / `${VARCHAR_*}` | `NVARCHAR(n)` | Unicode |
| `${CHAR_*}` | `NCHAR(n)` | |
| `${TEXT_BIG}` | `NVARCHAR(MAX)` | |
| `${JSON}` | `NVARCHAR(MAX)` | ingested text |
| `${DATE}` / `${TIME}` / `${DATETIME}` / `${TIMESTAMP}` | `DATE` / `TIME` / `DATETIME2` | |
| `${TIMESTAMP_TZ}` | `DATETIMEOFFSET` | |
| `${SERIAL}` | `INT IDENTITY(1,1)` | LOAD still uses MAX+1, not IDENTITY, same as other engines |
| `${PRIMARY}` | `PRIMARY KEY` | |
| `${UNIQUE}` | `UNIQUE` | |
| `${NOW}` | `SYSUTCDATETIME()` | lock vs `CURRENT_TIMESTAMP` in Phase 2 |
| `${DUMMY_TABLE}` | empty | `SELECT 1` needs no FROM |
| `${REORG}` | `-- REORG TABLE` | |

`${SIZE_COLLECTION}` is `LEN(collection)` (or `DATALENGTH`).
Other `SIZE_*` stay numeric strings.

### Keys, time, JSON extract

| Macro | Proposed mssql |
| --- | --- |
| `${INSERT_KEY_START}` | `--` plus trailing space |
| `${INSERT_KEY_END}` | empty |
| `${INSERT_KEY_RETURN}` | `RETURNING` plus trailing space (C rewrites to `OUTPUT INSERTED.`, lock 21) |
| `${SESSION_SECS}` | `DATEDIFF(SECOND, :SESSION_START, SYSUTCDATETIME())` |
| `${TRMS}` / `${TRME}` | `DATEADD(MINUTE, -(` … `), ${NOW})` |
| `${TRFS}` / `${TRFE}` / `${TRFMS}` / `${TRFME}` | `DATEADD` seconds/minutes |
| `${JRS}` / `${JRM}` / `${JRE}` | `JSON_VALUE(` / comma-space / `)` |
| `${DROP_CHECK}` | raise if rows exist (lock spelling in Phase 2; MySQL CHAR(0) analog or `THROW`) |

DB2-only keys still defined so a copy-paste of `database.lua` keys
never misses.

### Encoding / hashing / ingest

| Macro | Proposed mssql |
| --- | --- |
| `${BASE64_START}` / `${BASE64_END}` | `${SCHEMA}base64_decode(` / `)` |
| `${COMPRESS_START}` / `${COMPRESS_END}` | `${SCHEMA}brotli_decompress(${SCHEMA}base64_decode(` / `))` |
| `${SHA256_HASH_START}` / `_MID` / `_END` | `${SCHEMA}sha256_b64(` / comma-space / `)` |
| `${JSON_INGEST_START}` / `_END` | `${SCHEMA}json_ingest(` / `)` |
| `${JSON_INGEST_SCHEMA_*}` | alias of ingest |
| `${BROTLI_DECOMPRESS_FUNCTION}` | `CREATE ASSEMBLY` + `CREATE FUNCTION` from extras |
| `${JSON_INGEST_FUNCTION}` | T-SQL body in 1000 |
| `${CONVERT_TZ_FUNCTION}` | comment |

### Dialect id

`query_dialects.mssql = 5`. **No lookup packet** unless an icon/path
fix is needed — key 5 already says `MS SQL Server` with
`sql_dialect_mssql.png`. Do not edit 1055 reverse-in-place.

Engine name in Helium / config / Test 31: `mssql` (not `sqlserver`,
not `microsoft`). `normalize_engine_name` accepts `mssql` and
`sqlserver` as aliases if Phase 3 wants the second; default `mssql`.

Copy `database_mssql.lua` into all four designs.

## Goals And Non-Goals

### Goals

1. C engine `src/database/mssql/` via unixODBC + ODBC Driver 18.
2. Helium dialect `database_mssql.lua` complete key set, four designs.
3. T-SQL ingest/base64/sha256 helpers + Brotli CLR (or lock-21/22
   fallback).
4. Test **39** full Acuranzo AutoMigrations on the Linux container.
5. 8-engine matrix (Phase 7) so auth/conduit/mail can use mssql.
6. Fedora-local Podman documented; no Windows.

### Non-goals (this plan)

- Windows SQL Server, SSMS-as-required, Azure SQL as CI.
- Babelfish / Cosmos DB / Azure SQL Edge (retired).
- Firebird, Firebase teardown, Cockroach retirement.
- Reusing `DB_ENGINE_AI`.
- A Hydrogen SQL interpreter.
- Production paid edition runbook (park like other optional prod
  phases; Developer EULA is test-only — say so in SECRETS/DATABASES).
- Rewriting historical metrics.

## Proposed design locks (Phase 0)

These are **proposed** until Phase 0 Status is complete.

1. **Product is Microsoft SQL Server 2022 Linux**, Developer edition,
   official container image. Not Windows. Not Fedora `mssql-server`
   (it does not exist).
2. **Helium emits SQL.** `database_mssql.lua` is a macro table.
3. **No v1 table subset.** Test 39 = full Acuranzo, same as Test 32.
4. **C client is unixODBC + Microsoft ODBC Driver 18.** Closest sibling
   is DB2’s ODBC-shaped code. FreeTDS is a Phase 1 **amendment only**
   if `msodbcsql18` will not install on Fedora 43.
5. **Local first:** extras Podman on this Fedora box. DOKS only after
   a recorded local failure.
6. **SHA-256 bytes match** other engines (UTF-8 concat, then SHA-256,
   then standard base64). Fixture vs SQLite `testpass`.
7. **JSON stored as `NVARCHAR(MAX)` text**; ingest fix-up; `$ref`
   accepted; `JSON_VALUE`; missing path → NULL.
8. **`${SCHEMA}` is `testms.`** (dot prefix). Lock test schema
   `testms` (analogous to `testcrdb` / `testfb` file).
9. **Connection fields** reuse `ConnectionConfig`:

   | JSON field | Meaning |
   | --- | --- |
   | `Engine` | `mssql` |
   | `Host` | container host (`127.0.0.1`) |
   | `Port` | `1433` |
   | `Database` | catalog (`hydrotst`) |
   | `User` | `sa` (tests) |
   | `Pass` | SA password from SECRETS; never committed |
   | `Schema` | `testms` |

   Connection string: `mssql://HOST:PORT/DATABASE` or ODBC
   `Driver=ODBC Driver 18 for SQL Server;Server=HOST,PORT;…`.
   `database_queue_determine_engine_type` recognizes `mssql://` (and
   `sqlserver://` if aliased) **before** the SQLite fallback.
   Encrypt: Driver 18 defaults to Encrypt=yes; extras/docs must set
   `TrustServerCertificate=yes` on the local container. Lock in Phase 1.
10. **Enum:** `DB_ENGINE_MSSQL` after DB2, before `DB_ENGINE_FIREBIRD`,
    as in [Coordination with Firebird](#coordination-with-firebird).
11. **Lookup 030 key 5 = MS SQL Server**, dialect id 5. No new key.
12. **Yugabyte stays. MariaDB stays. Cockroach stays until FIREBIRD
    Phase 8.** This plan does not delete them.
13. **Test 39** is MSSQL migrations. Test 37 is Firebird.
14. **Test 31:** add `mssql` to `ENGINES`; sqruff skip; unsubstituted
    `${…}` check still runs.
15. **Bootstrap stays SQL** `SELECT … FROM testms.queries …`.
16. **CI uses the local Linux container.** Unity mocks ODBC (DB2
    pattern) so `mku` does not need SQL Server; live connect is a
    Phase 3 Exit item.
17. **Never log** SA password, hashes, JWTs.
18. **Placeholders are `?`** (ODBC). `convert_named_to_positional`
    maps `DB_ENGINE_MSSQL` like SQLite / MySQL / DB2. Do not splice
    `:NAME`. Do not emit `@name` unless Phase 3 proves ODBC named
    binds are required — default is `?`.
19. **Meet completeness + coverage fences** before Phase 9 Status
    complete.
20. **`${SIZE_COLLECTION}` is `LEN(collection)`**, not a numeric
    `SIZE_*` constant.
21. **`RETURNING col` rewrite:** Helium keeps trailing
    `${INSERT_KEY_RETURN}` like PostgreSQL. The MSSQL C engine
    rewrites `INSERT … SELECT … RETURNING col` into
    `INSERT … OUTPUT INSERTED.col SELECT …` (OUTPUT after the column
    list). Bounded rewriter; fail closed on shapes it cannot parse.
    Do not change Helium templates globally.
22. **Brotli:** CLR extras first; Hydrogen COMPRESS-only pre-eval only
    if CLR fails (Status variance).
23. **Developer edition is not production.** Docs say so. No paid
    license in CI.

## Architecture

```text
Helium acuranzo_NNNN.lua
        |  database_mssql.lua macros
        v
   SQL (T-SQL + helpers; trailing RETURNING)
        |
        v
Hydrogen DQM  -->  mssql_execute_query
                      |
                      +-- optional RETURNING → OUTPUT rewrite
                      +-- unixODBC + msodbcsql18
                      +-- extras T-SQL / CLR inside SQL Server
                      v
                 QueryResult.data_json
```

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C `mssql/`** | ODBC, binds, OUTPUT rewrite, row → JSON | Lithium, Windows SSPI |
| **Helium `database_mssql.lua`** | macro spellings | unixODBC |
| **extras/mssql_server** | Podman, EULA, ports | Hydrogen internals |
| **extras/brotli_udf_mssql** | CLR / libbrotli | DQM |
| **Test 39** | container lifecycle, full Acuranzo | Azure portal |

## Completeness fences

Phase 9 re-checks the whole table.

### Engine registration

| Must exist | Notes |
| --- | --- |
| `DB_ENGINE_MSSQL` | After DB2, before FIREBIRD |
| `mssql_get_interface()` | Same shape as `postgresql_get_interface` |
| Registry lazy-loads on `type == mssql` | |
| `normalize_engine_name("mssql")` | |
| `lua.c` engines[] includes `"mssql"` | Payload contains `database_mssql.lua` |
| `database_queue_determine_engine_type` | `mssql://` |
| `database_get_counts_by_type` | `mssql_count` |

### C tree (`src/database/mssql/`)

`types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`,
`query.{c,h}`, `transaction.{c,h}`, `prepared.{c,h}`, optional
`rewrite.{c,h}` for RETURNING. No Firestore `sql_*.c`. No file > 1000
lines. No `static` functions.

### Helium

| Must exist | Notes |
| --- | --- |
| `database_mssql.lua` | All four designs; complete key set |
| `database.lua` engines + dialects + defaults | mssql = 5 |
| `if engine` arms include mssql | 1000, 1190, 1135, re-grep |
| Test 31 mssql generation | unsubstituted `${}` |
| `test_98` | |

### Config / extras

| Must exist | Notes |
| --- | --- |
| `hydrogen_config_schema.json` Engine enum | `mssql` |
| SECRETS.md | `MSSQL_SA_PASSWORD`, container host/port |
| `extras/mssql_server/` | README + start/stop/create-db |
| extras README | MSSQL row (container + ODBC 18 + CLR Brotli) |

### Tests / docs

Unity under `tests/unity/src/database/mssql/`. Test 39 mssql
migrations. Docs Phase 8: MACRO_REFERENCE mssql column, DATABASES,
GUIDE, PARAMETER_BINDING, TESTING, SECRETS, STRUCTURE, SITEMAP,
SchemaTool, Helium/Acuranzo READMEs, Lithium DATABASE-MIGRATIONS.

## Coverage fences

Same numeric fences as FIREBIRD.md (50% / 75% / 85% / 1000-line /
no new `static` / no dead symbols). Mock ODBC in Unity.

## Reference Conventions

- C engine: vtable like `postgresql/`; ODBC like `db2/` +
  `mock_libdb2` analog `mock_libodbc` or shared mock — lock in Phase 3
  (prefer a mssql-owned mock so DB2 tests do not collide).
- Health: `SELECT 1` (no FROM).
- Blackbox Test 39: `TEST_ABBR` **MSQ**, ports **539x**.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/`: `mkt`
  then `mkp`. After Bash: `mks`. After Lua: `test_98`. After Markdown:
  `mkl`.

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Locks approved (SQL Server 2022 Linux container, ODBC 18, key 5, Test 39, RETURNING rewrite, enum); no C | S | pending |
| 1 | extras/mssql_server start/stop; `sqlcmd` against local container; ODBC 18 (or FreeTDS amendment) on Fedora 43 | M | pending |
| 2 | Complete `database_mssql.lua` in four designs; Test 31 generates mssql SQL | M | pending |
| 3 | C engine registers, `mssql://`, connect + health vs container or ODBC mock | M | pending |
| 4 | T-SQL helpers + Brotli CLR (or COMPRESS pre-eval variance); SHA-256 fixture matches SQLite | L | pending |
| 5 | Test 39 mssql AutoMigrations **full Acuranzo** green | L | pending |
| 6 | SchemaTool / SchemaHelper / hydrogen_flush / transaction_utils | M | pending |
| 7 | Tests 40/43/45/46/47/58 include mssql; 8-engine loops | L | pending |
| 8 | Docs/SITEMAP/MACRO_REFERENCE/DATABASES/SECRETS match; `mkl` green | S | pending |
| 9 | Completeness + coverage fences; dead-code clean; `mkp` | M | pending |

---

## Phase 0 — Contract lock

### Goal

Approve or amend the locks above. No `src/` or Helium edits.

### Entry gate

This document exists. Ability to read the four `database_*.lua` files,
`acuranzo_1000.lua`, `src/database/db2/` ODBC usage, and this box’s
Podman / `dnf` state.

### Work items

- [ ] 0.1 Confirm SQL Server 2022 Linux Developer via Podman; no
      Windows; no Fedora mssql-server RPM.
- [ ] 0.2 Confirm Helium still emits SQL; no table subset; RETURNING
      rewrite in C (lock 21).
- [ ] 0.3 Confirm ODBC 18 + unixODBC; FreeTDS only as Phase 1 amendment.
- [ ] 0.4 Confirm enum slot and Lookup 030 key 5 (no new packet).
- [ ] 0.5 Confirm `testms.` schema, `mssql://`, SA + Encrypt/TrustServerCertificate.
- [ ] 0.6 Confirm Test **39** (not 37); Cockroach/Firebird untouched.
- [ ] 0.7 Confirm Test 31 mssql; bootstrap remains SQL.
- [ ] 0.8 Confirm Brotli CLR first, COMPRESS pre-eval only on failure.
- [ ] 0.9 Confirm extras/mssql_server; SHA-256 UTF-8 fixture.
- [ ] 0.10 Confirm completeness + coverage fences for Phase 9.
- [ ] 0.11 Record amendments if any lock changes.

### Done means

Phase 0 Status lists every lock as approved or amended; no C/Lua/tests
changed in this phase.

### Exit gate

- Phase 0 Status = complete; user approval in Working Log.
- Next free Acuranzo id re-checked (FIREBIRD may have taken 1384).

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

- **2026-09-18** Plan authored alongside FIREBIRD.md. Fedora 43 has
  Podman, no unixODBC, no Microsoft ODBC driver, no mssql-server RPM.
  Preferred path is official Linux container + ODBC 18. Phase 0 waits
  for lock approval. No C this turn.

### Lessons learned

(empty until the phase runs)

---

## Phase 1 — Fedora Podman SQL Server + ODBC

### Goal

This Fedora box can start SQL Server 2022 Linux in Podman and talk to
it with `sqlcmd` and an ODBC 18 DSN (or a documented FreeTDS
amendment).

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] 1.1 `extras/mssql_server/README.md`: image tag, EULA, memory,
      port, `MSSQL_SA_PASSWORD` via env, Encrypt/TrustServerCertificate,
      how to install `msodbcsql18` on Fedora 43.
- [ ] 1.2 `start.sh` / `stop.sh` / `create_test_db.sh`. Idempotent
      start; wait for 1433; stop only if started.
- [ ] 1.3 Prove ODBC 18 install **or** amend lock 4 to FreeTDS with
      rationale in Status.
- [ ] 1.4 extras README table row. SECRETS.md names.
- [ ] 1.5 If local container fails: Status variance + DOKS note; do
      not silently switch.

### Done means

`start.sh` yields `sqlcmd -Q "SELECT 1"` success against 1433 on this
box (or a recorded DOKS fallback with the same extras scripts pointed
at that host).

### Exit gate

- `mks` on new scripts.
- Manual start/query/stop recorded in Status.
- `mkl` if extras README gained links.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 2 — Helium dialect (complete macros)

### Goal

`require("database_mssql")` supplies every macro key. Test 31 generates
mssql SQL without unsubstituted `${…}`. Per-engine arms know mssql.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] 2.1 Write `database_mssql.lua` (four designs). Key-set diff empty.
- [ ] 2.2 `database.lua`: `engines.mssql`, `query_dialects.mssql = 5`,
      `defaults.mssql`. Dot schema prefix (existing non-firebase path).
      `lua.c` `engines[]` includes `"mssql"`.
- [ ] 2.3 Test 31: `ENGINES` includes mssql; sqruff skip; Test 31 green.
- [ ] 2.4 acuranzo_1000 mssql arm (helpers; CLR CREATE may wait for
      Phase 4 but the skip/create shape must not emit PG/MySQL UDF DDL).
- [ ] 2.5 mssql arms for 1190, 1135; re-grep `if engine`, `||`,
      `LIMIT`, `LATERAL`, `RETURNING`.
- [ ] 2.6 `test_98`. No lookup packet unless icon path is wrong.

### Done means

Test 31 generates mssql SQL for every Acuranzo migration without
`${UNSUBSTITUTED}`; luacheck clean.

### Exit gate

- Test 31; `mks` if the script changed; `test_98`.
- Key-set diff in Status.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 3 — C engine skeleton

### Goal

Register, connstring, connect, health. RETURNING rewrite may be a stub
that fails closed until Phase 4/5 needs it.

### Entry gate

Phase 2 Status complete.

### Work items

- [ ] 3.1 `DB_ENGINE_MSSQL` in the locked enum order. `mkt`. Grep
      hardcoded AI numerics.
- [ ] 3.2 `interface`, `utils`, `connection`, query/transaction/prepared
      stubs. Unity ODBC mock. Live container connect.
- [ ] 3.3 Registry, `normalize_engine_name`, `mssql_count`,
      `mssql://` before SQLite fallback.
- [ ] 3.4 Driver 18 Encrypt / TrustServerCertificate from extras docs.

### Done means

Unity connects via mock; live `SELECT 1` vs container recorded;
`mkt` + `mkp`; no new `static`.

### Exit gate

- `mkt` then `mkp`; named `mku`; coverage fence.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 4 — T-SQL helpers + Brotli

### Goal

Ingest, Base64, SHA-256 fixture, Brotli decompress on INSERT. RETURNING
rewrite implemented for the QueryRef `INSERT … SELECT … RETURNING col`
shape.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 T-SQL `json_ingest`, `base64_encode`/`decode`, `sha256_b64`
      (UTF-8). 1000 arm emits them.
- [ ] 4.2 SHA-256 fixture vs SQLite `0`+`testpass`.
- [ ] 4.3 Brotli CLR extras **or** COMPRESS pre-eval variance (lock 22).
      lua-brotli quality 11 round-trip.
- [ ] 4.4 `RETURNING` rewriter + Unity fixtures (1194-shaped INSERT).
- [ ] 4.5 JSON extract `JSON_VALUE`; `$ref` ingest fixture.
- [ ] 4.6 Test 31 still green.

### Done means

Named verification for sha256, json, brotli, RETURNING rewrite; `mkp`;
`mks` on extras.

### Exit gate

- Commands cited in Status; coverage fence for new C.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 5 — AutoMigrations / Test 39 (full Acuranzo)

### Goal

Hydrogen AutoMigrations against the Linux container apply the **full**
Acuranzo design. Same done means as Test 32. Number **39**.

### Entry gate

Phase 4 Status complete. Payload includes `database_mssql.lua` (`mkt`).

### Work items

- [ ] 5.1 `hydrogen_test_39_mssql.json` (`Engine: mssql`, schema
      `testms`, AutoMigration + TestMigration as Test 32).
- [ ] 5.2 `tests/test_39_mssql_migrations.sh`. Container lifecycle via
      extras. Do not modify Test 37.
- [ ] 5.3 Docs `docs/H/tests/test_39_mssql_migrations.md`.
- [ ] 5.4 Run until LOAD/APPLY/REVERSE match Test 32 expectations.
      Failures are dialect/UDF/`if engine`/rewrite bugs, not a skip list.

### Done means

`tests/test_39_mssql_migrations.sh` reports migration completed for the
full design; `mks`; markdown exists.

### Exit gate

- Live Test 39 log path in Status.
- `mks`; `mkl` for the test doc.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 6 — SchemaTool, SchemaHelper, flush

### Goal

Operator tools talk TDS/ODBC, not `psql`.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 `schematool_mssql.sh`.
- [ ] 6.2 schemahelper connect/apply/const.
- [ ] 6.3 `hydrogen_flush.sh` mssql path (drop/recreate schema or db).
- [ ] 6.4 `transaction_utils.sh` mssql path.

### Done means

mssql SchemaTool wrapper does not call `psql` or `isql-fb`.

### Exit gate

- `mks` + `test_98` as touched.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 7 — Grow the blackbox matrix 7 → 8

### Goal

MSSQL is a first-class operator engine in Tests 40/43/45/46/47/58.
Skips only for environmental reasons (container down), not "not
implemented."

### Entry gate

Phase 6 Status complete. Firebird may or may not have retired Cockroach;
this phase **adds** mssql either way. Do not drop an existing engine to
keep the count at 7.

### Work items

- [ ] 7.1 Test 40 auth live on mssql.
- [ ] 7.2 Tests 43, 45, 46, 47, 58 configs + loops.
- [ ] 7.3 Test 41/44/51/54 docs/configs as needed.
- [ ] 7.4 Document 8-engine order (recommend: existing 7 then mssql
      last, or MAILRELAY_API_ENGINE_ORDER analog).

### Done means

Status table: each suite green (or env skip). Loops print eight names.

### Exit gate

- Named runs for 40 and 39 at minimum.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 8 — Docs sweep

### Goal

Current docs describe six real dialects (plus MariaDB/Yugabyte aliases)
and MSSQL as implemented. MACRO_REFERENCE has an MSSQL column.
DATABASES says “Linux container + ODBC 18; Developer is not production.”

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 Helium GUIDE, MACRO_REFERENCE, DATABASES, TESTING_GUIDE,
      BROTLI_COMPRESSION, design READMEs, `docs/He/DATABASES/database_mssql.md`.
- [ ] 8.2 Hydrogen TESTING, INSTRUCTIONS, PARAMETER_BINDING, SECRETS,
      STRUCTURE, SITEMAP, MAIL_GUIDE, SchemaTool/SchemaHelper, tests README.
- [ ] 8.3 Lithium `DATABASE-MIGRATIONS.md` (key 5 already named).

### Done means

`mkl` green; no active doc claims key 5 is unimplemented.

### Exit gate

- `zsh -ic 'mkl'`; markdownlint on touched files.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Phase 9 — Completeness and coverage re-check

### Goal

Every completeness-fence row is true or `[~]`. Coverage fences hold.
Dead-code list has no stray mssql symbols.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 Walk completeness table.
- [ ] 9.2 Walk coverage fences.
- [ ] 9.3 `mkt` dead-code gate.
- [ ] 9.4 `mkp`, `mks`, `test_98`, Test 31, Test 39, Test 40 mssql.

### Done means

Fences green; Test 39 and Test 40 mssql green. Then move this plan to
`plans/complete/MSSQL_COMPLETE.md`; drop TODO 28; `mkl`.

### Exit gate

- Commands in 9.4 actually run; output cited in Status.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

(empty until the phase runs)

### Lessons learned

(empty until the phase runs)

---

## Testing notes

| Layer | What |
| --- | --- |
| Unity | Connstring, registry, ODBC mock, RETURNING rewrite |
| Blackbox | Test 39 container **full** AutoMigrations. Test 40+ when Phase 7 says so |
| Coverage | See coverage fences |
| Build | `mkq` / `mkt` / `mkp` / `mks` / `test_98` |

Port scheme: Test 39 → **539x**. SQL Server TDS **1433** (or extras
override).

## Threat notes

- **Secret leakage:** SA password, hashes, JWTs. Mask `mssql://`.
- **EULA:** Developer edition is not production; do not imply it is.
- **Encrypt:** Driver 18 defaults Encrypt=yes; local container needs
  TrustServerCertificate.
- **Hash mismatch** is a login outage — UTF-8 SHA-256 fixture is
  mandatory.
- **CLR / COMPRESS failure** stores wrapped blobs as `code`.

### Risks

| Risk | Mitigation |
| --- | --- |
| Treating SQL Server as libpq | ODBC client; never alias to postgresql |
| Windows-only extras | Forbidden; Linux container + CLR or COMPRESS pre-eval |
| ODBC 18 will not install on Fedora | Phase 1 amendment to FreeTDS, recorded |
| Local RAM < 2 GiB for the container | Phase 1 DOKS fallback, recorded, not silent |
| RETURNING templates vs OUTPUT | Bounded C rewrite (lock 21) |
| Concat `\|\|` / LATERAL QueryRefs | Phase 2 grep + arms |
| Fighting Firebird over Test 37 / enum | Test 39; reserved enum order |
| 7-engine loops miss mssql | Phase 7 grows to 8; do not drop Yugabyte |
| Helium ID drift | Re-check disk at packet time |

## Working Log (cross-phase memory)

### Decisions log

- **(Plan authored, 2026-09-18)** MSSQL created as a greenfield sixth
  C engine. Key 5 already seeded. Fedora-local official Linux container
  preferred over DOKS. Sister plan FIREBIRD.md. No C this turn.

### Surprises / deviations (historical, still true)

- Fedora 43 does not ship `mssql-server`. Podman is already installed.
- Lookup 030 key 5 has been “MS SQL Server” since 1055; no additive
  lookup is required.
- DB2 C already uses ODBC typedefs — copy that shape, not Firestore.
- `INSERT_KEY_RETURN` is trailing `RETURNING` in QueryRefs (1194).
  SQL Server `OUTPUT` is mid-statement. Lock 21 exists because changing
  every Lua file is worse.
- SQL Server `HASHBYTES` + `NVARCHAR` is UTF-16; other engines hash
  UTF-8. The T-SQL helper must not silently diverge.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After add/remove `src/`: `mkt` then `mkp`.
- After bash: `mks`. After Lua: `test_98`. After docs: `mkl`.
- Never apply Helium packets; hand them to the user.
- Payload rebuild (`mkt`) is required before Test 31/39 see
  `database_mssql.lua`.
- Do not `dlopen` libpq for mssql.
- Cross-check SHA-256 against SQLite `crypto_sha256` before declaring
  login green.
