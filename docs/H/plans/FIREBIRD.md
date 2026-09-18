<!-- markdownlint-disable MD007 MD024 -->
# Firebird Engine Plan

## Status at a glance

**New plan (2026-09-18).** Phase 0 locks not approved. Do not write C.
This is not a rename of
[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).
Firebird is a SQL RDBMS (`libfbclient`). Hydrogen sends Helium SQL to it.

| Phase | Status | Remaining |
| --- | --- | --- |
| 0 Contract lock | pending | **Quick** |
| 1 Fedora Firebird extras | pending | **Quick** |
| 2 Helium dialect + Lookup 030 key 6 | pending | **Moderate** |
| 3 Firebase C / Unity teardown | pending | **Moderate** |
| 4 Firebase Helium / extras teardown | pending | **Quick** |
| 5 C register / connect | pending | **Moderate** |
| 6 Brotli UDR + JSON ingest | pending | **Moderate** |
| 7 Test 37 full Acuranzo | pending | **Difficult** |
| 8 Retire Cockroach names | pending | **Quick** |
| 9 SchemaTool / flush | pending | **Moderate** |
| 10 Tests 40–58 firebird matrix | pending | **Difficult** |
| 11 Docs | pending | **Quick** |
| 12 Coverage / completeness | pending | **Moderate** |

Remaining: 2 Difficult (7, 10), 6 Moderate (2, 3, 5, 6, 9, 12), 5 Quick
(0, 1, 4, 8, 11).

**Parity:** Firebird is a Hydrogen `DatabaseEngineInterface`, not a new
API. Match PostgreSQL / SQLite / MySQL / DB2: same `QueryRequest` /
`QueryResult`, same `parse_typed_parameters` →
`convert_named_to_positional` → bind, same `data_json` array of row
objects. Do not change those engines. Do not interpret SQL in C.

**Sister plan:** [`MSSQL.md`](/docs/H/plans/MSSQL.md) (Lookup 030 key 5).
Shared enum lock is in [Coordination with MSSQL](#coordination-with-mssql).

## Purpose

Replace the **CockroachDB** operator slot with a real **Firebird**
implementation. Cockroach was never a fifth Hydrogen engine: it is a
PostgreSQL alias (same `libpq` path, same
`${env.ACURANZO_DB_TYPE}` = `postgresql`, a different schema name).
YugabyteDB is the same kind of alias and **stays**. Firebird will not
alias anything. It is a `DatabaseEngineInterface` in C and a Helium
dialect (`database_firebird.lua`).

Cloud Firestore was attempted first
([`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)).
That was the wrong product: Helium emits SQL, and Firestore has no SQL
server. Firebird does. Phases 3–4 delete the Firestore tree; they are
not “reshape Firebase.”

**There is no v1 subset and no SQL interpreter.** When Hydrogen’s
Firebird engine and Helium’s `database_firebird.lua` are in place,
AutoMigrations must apply the **same Acuranzo Lua files** that Test 32
applies on PostgreSQL. QueryRefs that later run through Conduit / auth /
mail must execute as Firebird SQL.

Helium migrations stay **SQL**. Macros still expand types and function
wrappers. Hydrogen **sends that SQL to `libfbclient`**. Brotli that
Firebird cannot do natively is an extras **UDR**, same story as SQLite
`.so` extras — not in-process eval in Hydrogen.

This is the only active plan for Firebird and for Firebase teardown.

**Session brief:** A new conversation may start with only a pointer to
this file. Open [Status at a glance](#status-at-a-glance), then
[Resuming Work](#resuming-work), then the next incomplete phase only.
Do not reconstruct Cockroach vs Firebase vs Firebird from git history.

History that this plan does **not** reopen:

- Database subsystem:
  [`DATABASE_PLAN_COMPLETE.md`](/docs/H/plans/complete/DATABASE_PLAN_COMPLETE.md)
- Parameter binding:
  [`DATABASE_UPDATE_PLAN_COMPLETE.md`](/docs/H/plans/complete/DATABASE_UPDATE_PLAN_COMPLETE.md)
- Migrations:
  [`MIGRATIONS_COMPLETE.md`](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md)
- SchemaTool / SchemaHelper:
  [`SCHEMATOOL_PLAN_COMPLETE.md`](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md),
  [`SCHEMAHELPER_V2_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md)
- Firestore attempt:
  [`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md)

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, record lessons learned, **stop for review**.
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure or
  after adding/removing `src/` files), `mku <base>`, `mkp`, `mka`, `mks`.
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**. Follow this sequence:

1. **Confirm the prior phase is actually done.** Re-read its Status block
   and Exit gate before touching anything; do not trust memory of a prior
   session.
2. **Discuss the current phase first.** Re-read only that phase's Goal +
   Work items + Done means + Exit gate. Ask clarifying questions and do
   any research needed **before** writing any code.
3. **Ask for explicit approval to start implementation.** Do not begin
   editing source files until the user says go.
4. **Ask questions as they come up** during implementation rather than
   guessing at ambiguous requirements.
5. **Update the phase's Working Log entry when major pieces land** (not
   only at the very end).
6. **Record lessons learned** for the phase, even small ones.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean.** Intent to verify is
   not verification.
8. **Never apply a database migration.** Prepare/generate Helium packets
   and hand them to the user; do not run `schematool`/`schemahelper` apply.
9. **Follow existing project norms:** no `static` functions in `src/`;
   Unity one file per function ([TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md));
   blackbox `tests/test_NN_*.sh` ([TESTING.md](/docs/H/tests/TESTING.md));
   `jq` for JSON in Bash; absolute Markdown links; `mkq`/`mkt` then `mkp`
   after C; `mks` after scripts. Meet the [Coverage fences](#coverage-fences)
   and [Completeness fences](#completeness-fences).
10. **Never log** Firebird SYSDBA passwords or document bodies that contain
    secrets (password hashes, JWTs, OTP codes) in normal logs or test
    artifacts.
11. **Do not increment `TEST_COUNTER` in blackbox scripts.** The framework
    owns the counter.
12. **Do not delete the Cockroach slot until Phase 8.** Earlier phases add
    Firebird beside it. Phase 8 is the swap so the 7-engine matrix never
    silently becomes six.
13. **Mirror the other engines.** When a Firebird choice is ambiguous,
    do what PostgreSQL / SQLite / MySQL / DB2 already do
    (`PARAMETER_BINDING.md`, `QueryResult.data_json`, named `:NAME`
    markers). Do not invent a Firebird-only execute path and do not
    change the other engines to accommodate it.
14. **Do not interpret SQL in C.** Firebird runs the SQL. Extras UDRs
    cover Brotli (and JSON ingest/extract if native SQL/JSON is absent).
    The Firestore interpreter in `src/database/firebase/sql_*.c` is
    deleted in Phase 3, not reused.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-18):** Phase 0 not approved. No C,
no Helium packet, no Firebase deletes. This box is Fedora 43: `dnf`
has `firebird` / `libfbclient2-devel` **4.0.7.3271**; packages are not
installed. Last numbered Acuranzo file on disk: `acuranzo_1383.lua`
(Lookup 030 key 6 = **Firebase**, applied). Next free id **1384**
(re-check disk). Sister plan MSSQL is also at Phase 0.

Keep this block current when a phase finishes (date, result, next phase
number). It is the first thing a new session reads.

### Resume here next session

1. This file is the source of truth for Firebird and for Firebase
   teardown. Do not start a second Firebird or “drop Cockroach” plan.
   Do not resume [`FIREBASE.md`](/docs/H/plans/FIREBASE.md).
2. Read **Status at a glance**, **CURRENT PAUSE POINT**, the Phase
   Index **Status** column, and **Working Log (cross-phase memory)**.
   Parity with the other four engines is a standing rule.
3. Confirm the prior phase Status is actually complete (re-read its Exit
   gate; do not trust chat memory).
4. Re-read **only** the next phase: Goal + Work items + Done means +
   Exit gate. Skim a lock in
   [Proposed design locks (Phase 0)](#proposed-design-locks-phase-0)
   when that phase cites it. Do not re-read the whole document.
5. If the phase needs a Helium packet: re-check disk
   (`ls elements/002-helium/acuranzo/migrations/acuranzo_*.lua`).
   Snapshot at this pause: last **`acuranzo_1383.lua`**, Lookup **030**
   key **6** still labelled Firebase. `DB_ENGINE_AI` is still the Unity
   mock slot. Do not trust the snapshot; re-check disk.
6. Discuss, get explicit approval, implement that phase only, verify the
   Exit gate, update Status + Working Log + this pause point, **stop**.

### Session checklist

1. CURRENT PAUSE POINT → first Status that is not complete.
2. Working Log decisions that affect this phase.
3. Baseline as the phase names it: `mkq` or `mkt`; named `mku`; Test 31
   / 37 / 40 when listed.
4. One phase: questions → approval → implement → verify → update this
   plan → stop for review.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new engine, after Auth Finale / quality gates |
| **Effort** | XL (C `libfbclient` engine + Helium dialect + Brotli/JSON extras + Test 37 + Cockroach retirement + Firebase teardown) |
| **Done** | 0% — plan authored, Phase 0 not approved |
| **Why this shape** | Cockroach never earned a C implementation. Firebird is a SQL engine Fedora already packages. The existing ~380 Lua files emit SQL; Firebird runs that SQL. |
| **Do not start casually** | Touches `DatabaseEngine` enum, registry, DQM, Helium `database.lua` for four designs, Test 31/37/71, the 7-engine blackbox matrix, SchemaTool, Lookup 030, and deletes the Firestore tree. |

Backlog: [TODO.md item 27](/docs/H/TODO.md).

## Coordination with MSSQL

[`MSSQL.md`](/docs/H/plans/MSSQL.md) adds Lookup 030 **key 5** (already
seeded as `MS SQL Server`). This plan owns key **6** (relabel Firebase
→ Firebird) and the Cockroach replacement (Test **37**).

**Final C enum** (both plans lock this; do not shift later):

```c
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_MSSQL,      // key 5 — interface may be NULL until MSSQL.md
    DB_ENGINE_FIREBIRD,   // key 6
    DB_ENGINE_AI,         // Unity mock — do not reuse
    DB_ENGINE_MAX
} DatabaseEngine;
```

Phase 5 of this plan may introduce `DB_ENGINE_MSSQL` as an unused
enumerator so the MSSQL plan does not shift Firebird’s numeric value.
Do not add `mssql_get_interface` symbols here (dead-code gate). Registry
skips a NULL interface.

Shared files (`database.lua` `engines`, `lua.c` `engines[]`, Test 31
`ENGINES`, `convert_named_to_positional`, `database_get_counts_by_type`)
are **additive only**. Do not delete the other plan’s dialect name.
Firebase names are this plan’s to remove (Phases 3–4).

Test **37** stays Firebird (was Cockroach). MSSQL gets a new Test **39**.
The 7-engine matrix stays 7 until MSSQL Phase 7 grows it to 8.

## Snapshot: what exists today (2026-09-18)

Do not re-implement these; they are constraints.

### Four real engines, three aliases, one wrong fifth

| Operator name | Hydrogen C | Helium dialect | Typical config `Engine` | Notes |
| --- | --- | --- | --- | --- |
| PostgreSQL | `src/database/postgresql/` | `database_postgresql.lua` | `${env.ACURANZO_DB_TYPE}` → `postgresql` | Real |
| SQLite | `src/database/sqlite/` | `database_sqlite.lua` | `sqlite` | Real |
| MySQL | `src/database/mysql/` | `database_mysql.lua` | `${env.CANVAS_DB_TYPE}` | Real |
| DB2 | `src/database/db2/` | `database_db2.lua` | `db2` | Real |
| MariaDB | **MySQL** | **mysql** | same as MySQL, schema `demomrdb` | Alias |
| YugabyteDB | **PostgreSQL** | **postgresql** | `${env.YUGABYTE_DB_*}` | Alias; **stays** |
| CockroachDB | **PostgreSQL** | **postgresql** | schema `testcrdb` / `democrdb` | Alias; **this plan retires it** |
| Firebase | `src/database/firebase/` | `database_firebase.lua` | (not in the 7-engine matrix) | Wrong product; **Phases 3–4 delete** |

C enum today still has `DB_ENGINE_FIREBASE` after DB2, before AI.
`grep` of `src/**/*.c` for `cockroach` is **empty**.

### This machine (Phase 1 baseline)

| Fact | Value |
| --- | --- |
| Distro | Fedora 43 |
| Firebird in `dnf` | `firebird` / `libfbclient2-devel` **4.0.7.3271** (not installed) |
| Firebird 5 | Fedora 44/45 only — **not** required |
| `podman` | 5.8.4 (MSSQL plan; unused here) |

### Cockroach-named files (main tree)

Replace in Phase 8, not before. Inventory is unchanged from the
Firestore plan: Test 37 script/docs/configs, SchemaTool wrapper,
`hydrogen_flush.sh`, Test 40/41/43/45/46/47/51/54/58 scripts, TESTING /
PARAMETER_BINDING / SECRETS / SITEMAP / STRUCTURE. Historical
`docs/H/metrics/` **stay**.

### How the other engines get Base64 / Brotli / SHA-256 / TZ

| Need | PostgreSQL | MySQL | SQLite | DB2 | Firebird 4 (this plan) |
| --- | --- | --- | --- | --- | --- |
| Base64 | native | native | sqlean `crypto.so` | C UDF extras | **native** `BASE64_ENCODE` / `BASE64_DECODE` |
| Brotli | C extra | plugin extra | loadable extra | C UDF extra | **UDR extra** (`extras/brotli_udf_firebird/`) |
| SHA-256 | native | native | sqlean | `HASH(...,2)` + encode UDF | **native** `CRYPT_HASH(... USING SHA256)` |
| `json_ingest` | plpgsql | stored fn | passthrough | SQL UDF | **PSQL function or UDR** (no SQL/JSON until Firebird 6) |
| `CONVERT_TZ` | native | native | extra (UTC stub) | native | **native** `TIMESTAMP WITH TIME ZONE` / `DATEDIFF` — lock in Phase 0 |

SQLite is the closest analog: extras for what the engine cannot do,
`?` placeholders, file-or-local server. Firebird SuperServer is a
daemon on **3050**, like MySQL, with an optional embedded attach.

## The four solvable headaches

These are the same contracts Helium already made identical across PG /
MySQL / SQLite / DB2. They are in scope. Firebird does **not** get a
pass because it is “SQL.”

### 1. JSON ingest and extract

Firebird 4/5 store JSON as **text** (`BLOB SUB_TYPE TEXT`). Native
`JSON_VALUE` is Firebird 6 / Red Database — **not** on Fedora 43.
`${JSON}` columns are text, SQLite-compatible. Do not invent a Firebird
binary JSON type.

**Ingest:** same control-char fix-up as PG/MySQL/DB2 (fast-path if
valid; escape `\n`/`\t`/`\r` only inside quotes; parse again). `$ref` /
`$id` / `$schema` accepted (not DB2 JSON2BSON). Implement as a PSQL
stored function in `acuranzo_1000` (engine arm) or a C UDR in extras.
Unity or `isql` fixture: valid JSON; JSON with newline inside a string;
garbage still fails; JSON Schema with `$ref` round-trips.

**Extract (`${JRS}` / `${JRM}` / `${JRE}`):** UDR or PSQL
`JSON_VALUE(col, '$.icon')`. Missing path → SQL NULL.

### 2. Base64

Native. Alphabet is standard `+/` with `=` padding. No URL-safe
variant. No extras UDF.

### 3. Brotli

Lua still compresses `code` >1KB at generation and wraps
`${COMPRESS_START}` … `${COMPRESS_END}`. Firebird must decompress on
INSERT or APPLY runs a base64 blob as SQL.

**UDR** in `extras/brotli_udf_firebird/` (`libbrotlidec`, same library
as the other extras). Quality 11 on the Lua side is a lock; the engine
only decompresses. `CREATE FUNCTION` from `${BROTLI_DECOMPRESS_FUNCTION}`.

### 4. SHA-256 password hashes

Bytes must match the other engines or Test 40 login against a
firebird-migrated admin account fails.

Proposed spelling:

```text
${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
→ BASE64_ENCODE(CRYPT_HASH(('0' || '<password>') USING SHA256))
```

Phase 6 (or 5) fixture: `account_id` string `"0"` + `"testpass"` →
`CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` (SQLite crypto_sha256,
already recorded in the superseded Firestore plan). Do not use mocked
`utils_password_hash` in Unity.

## Firebird installation (CI / dev)

Read this before Phase 0 locks. Firebird **is** `dnf install` plus
`CREATE DATABASE`.

| Piece | Why | Notes |
| --- | --- | --- |
| `firebird` | SuperServer on 3050 | Fedora 43: 4.0.7 |
| `libfbclient2` | runtime client | pulled by `firebird` |
| `libfbclient2-devel` / `firebird-devel` | `ibase.h`, link Hydrogen | |
| `firebird-utils` | `isql-fb`, `gfix`, `nbackup` | |
| libbrotli | UDR decompress | already a Hydrogen dep |

There is **no** Firestore emulator, **no** Java, **no** `firebase-tools`.

### SuperServer vs embedded

| Mode | When | Connection |
| --- | --- | --- |
| **SuperServer (Test 37 / 40 default)** | networked SQL, like the Cockroach slot | `localhost/3050:/path/testfb.fdb` |
| **Embedded (allowed)** | appliance / no daemon | Database path, empty Host — `libfbclient` loads Engine plugin |

Phase 1 documents both. Tests lock SuperServer unless Phase 0 amends.

### extras layout (Phase 1)

```text
elements/001-hydrogen/hydrogen/extras/
  firebird/
    README.md     # dnf packages, SYSDBA, create testfb.fdb, ports
    start.sh      # enable/start firebird service if we own it; wait for 3050
    stop.sh       # stop only if we started it
    create_test_db.sh
```

Do **not** keep `extras/firebase_emulator/` past Phase 4.

## Why Helium still emits SQL

Acuranzo files contain literal SQL verbs. Macros fill types and
function wrappers. Rewriting ~380 files into a Firebird-only language
is out of scope. `database_firebird.lua` is a fifth (then sixth, with
MSSQL) dialect file with the **same keys** as the other four.

Rare `if engine == '…'` files must grow a **firebird** arm (and lose
**firebase** in Phase 4):

| File | Why it branches |
| --- | --- |
| `acuranzo_1000.lua` | UDF/UDR DDL per engine |
| `acuranzo_1190.lua` | ALTER COLUMN (sqlite table rebuild — Firebird ALTER is limited; sharing sqlite is acceptable) |
| `acuranzo_1135.lua` | JSON constructors per engine |
| `acuranzo_1151.lua` | mysql vs others — `engine ~= 'mysql'` already covers firebird |

Phase 2 greps `if engine` again. 1168 (`LATERAL` + `jsonb_agg`) is
PostgreSQL-shaped; Firebird 4 has `LATERAL` derived tables but not
`jsonb_agg`. Dedicated arm or documented rewrite — do not silently
drop LATERAL.

## `database_firebird.lua` — complete macro table

Every key in the union of `database_postgresql.lua`,
`database_mysql.lua`, `database_sqlite.lua`, `database_db2.lua` must
exist so `replace_query` never leaves `${UNSUBSTITUTED}`. Values below
are **proposed** (Phase 0 amends).

`${SCHEMA}` for Firebird is **empty**. Firebird 4 has no PostgreSQL
schemas. Isolation is a separate `.fdb` (`testfb.fdb` / `demofb.fdb`),
not `testfb.queries` and not `testfb_queries`. `database.lua`
`replace_query` must not apply the firebase underscore prefix to
firebird. Empty schema → no prefix (SQLite-like).

### Types

| Macro | Firebird 4 spelling | Notes |
| --- | --- | --- |
| `${INTEGER}` / `${INTEGER_SMALL}` | `INTEGER` / `SMALLINT` | |
| `${INTEGER_BIG}` | `BIGINT` | |
| `${FLOAT}` / `${FLOAT_BIG}` | `FLOAT` / `DOUBLE PRECISION` | |
| `${TEXT}` / `${VARCHAR_*}` / `${CHAR_*}` | `VARCHAR(n)` / `CHAR(n)` | |
| `${TEXT_BIG}` | `BLOB SUB_TYPE TEXT` | fail closed only if extras impose a cap; no 1 MiB Firestore cap |
| `${JSON}` | `BLOB SUB_TYPE TEXT` | ingested text |
| `${DATE}` / `${TIME}` / `${DATETIME}` / `${TIMESTAMP}` | `DATE` / `TIME` / `TIMESTAMP` | |
| `${TIMESTAMP_TZ}` | `TIMESTAMP WITH TIME ZONE` | Firebird 4 |
| `${SERIAL}` | `INTEGER GENERATED BY DEFAULT AS IDENTITY` | |
| `${PRIMARY}` | `PRIMARY KEY` | |
| `${UNIQUE}` | `UNIQUE` | |
| `${NOW}` | `CURRENT_TIMESTAMP` | |
| `${DUMMY_TABLE}` | `FROM RDB$DATABASE` | required |
| `${REORG}` | `-- REORG TABLE` | no-op, like PG |

`${SIZE_*}` numeric strings stay for diagrams.
`${SIZE_COLLECTION}` is `CHAR_LENGTH(collection)` (or `OCTET_LENGTH`).

### Keys, time, JSON extract

| Macro | Proposed firebird |
| --- | --- |
| `${INSERT_KEY_START}` | `--` plus trailing space (same as PG; Firebird `RETURNING` is trailing) |
| `${INSERT_KEY_END}` | empty |
| `${INSERT_KEY_RETURN}` | `RETURNING` plus trailing space |
| `${SESSION_SECS}` | `DATEDIFF(SECOND FROM CAST(:SESSION_START AS TIMESTAMP) TO CURRENT_TIMESTAMP)` (lock exact spelling in Phase 2) |
| `${TRMS}` / `${TRME}` | `DATEADD(-N MINUTE TO ${NOW})` split across start/end |
| `${TRFS}` / `${TRFE}` / `${TRFMS}` / `${TRFME}` | `DATEADD` seconds/minutes |
| `${JRS}` / `${JRM}` / `${JRE}` | UDR/PSQL `JSON_VALUE(` / comma-space / `)` |
| `${DROP_CHECK}` | sqlite-shaped `SELECT … WHERE EXISTS` **or** an exception; APPLY must fail closed when the table has rows — lock in Phase 2 (Firestore `FB_REFUSE_DROP` is gone) |

DB2-only extras that must still be defined:
`${BASE64ENCODE_START}` / `_END`, `${BASE64ENCODEBINARY_START}` / `_END`,
`${DATETIME_FORMAT}`, `${TIMESTAMP_FORMAT}`, `${CONVERT_TZ_FUNCTION}`.

### Encoding / hashing / ingest

| Macro | Firebird proposed |
| --- | --- |
| `${BASE64_START}` / `${BASE64_END}` | `CAST(BASE64_DECODE(` / `) AS BLOB SUB_TYPE TEXT)` (lock CAST target in Phase 2) |
| `${COMPRESS_START}` / `${COMPRESS_END}` | `BROTLI_DECOMPRESS(BASE64_DECODE(` / `))` |
| `${SHA256_HASH_START}` / `_MID` / `_END` | `BASE64_ENCODE(CRYPT_HASH((` / ` \|\| ` / `) USING SHA256))` |
| `${JSON_INGEST_START}` / `_END` | `${SCHEMA}json_ingest(` / `)` — schema empty, so `json_ingest(` |
| `${JSON_INGEST_SCHEMA_*}` | alias of ingest (`$ref` accepted) |
| `${BROTLI_DECOMPRESS_FUNCTION}` | `CREATE FUNCTION BROTLI_DECOMPRESS …` from extras UDR |
| `${JSON_INGEST_FUNCTION}` | PSQL body or UDR |
| `${CONVERT_TZ_FUNCTION}` | comment or empty if native TZ is enough |

### Dialect id

`query_dialects.firebird = 6` (Lookup 030 key 6). Do not reuse key 5
(MS SQL Server — [`MSSQL.md`](/docs/H/plans/MSSQL.md)). Do not edit
`acuranzo_1055.lua` reverse-in-place. Packet **1384** (re-check disk)
`UPDATE`s key 6 `value_txt` from `Firebase` to `Firebird` and the icon
to `sql_dialect_firebird.png`. Do not reverse 1383 (that deletes key 6).

Copy `database_firebird.lua` into Acuranzo, Gaius, GLM, and Helium.

## Goals And Non-Goals

### Goals

1. C engine `src/database/firebird/` implementing the full
   `DatabaseEngineInterface` via `libfbclient` (`ibase.h` / `isc_*`).
2. Helium dialect `database_firebird.lua` with the complete macro key
   set, in all four designs.
3. Extras Brotli UDR + JSON ingest/extract equivalent to the other
   engines’ extras / native functions.
4. Test 37 applies the **full** Acuranzo design on Firebird SuperServer
   (same bar as Test 32).
5. QueryRefs execute as Firebird SQL so Test 40 and the 7-engine matrix
   can use firebird without a second product.
6. Retire Cockroach as a named engine. Keep Yugabyte and MariaDB.
7. Delete the Firestore engine, dialect, emulator extra, and Firebase
   Lookup label (Phases 3–4).
8. Test **37** stays number 37 (renamed). Docs match code.

### Non-goals (this plan)

- Cloud Firestore, Firebase Auth, Cloud Functions, Firebase emulator.
- A SQL interpreter in Hydrogen.
- Reusing `src/database/firebase/` by rename.
- Reusing `DB_ENGINE_AI`.
- Replacing Yugabyte or MariaDB.
- Implementing MSSQL (sister plan).
- Lithium SDK work beyond Lookup 030 rendering key 6 as Firebird.
- Rewriting historical `docs/H/metrics/` run names.
- Firebird 5 as a requirement on Fedora 43.

## Proposed design locks (Phase 0)

These are **proposed** until Phase 0 Status is complete.

1. **Product is Firebird 4.0** as packaged on Fedora 43 (`firebird`
   4.0.7). Firebird 5 is welcome on a later Fedora; do not require it.
2. **Helium emits SQL.** `database_firebird.lua` is a macro table.
   Existing Acuranzo files run unchanged except `if engine` arms plus
   UDR DDL in 1000.
3. **No v1 table subset.** Test 37 done means is full AutoMigrations
   success, same as Test 32. Honest because Firebird is a SQL engine.
4. **No SQL interpreter in C.** Hydrogen is a `libfbclient` client.
   Brotli / JSON extras are loaded **into Firebird**.
5. **C API is `libfbclient`** (`ibase.h`, `isc_*`). Not the C++ API,
   not Jaybird, not ODBC-for-Firebird.
6. **SHA-256 hash bytes match** the other engines for
   `account_id || password`. Fixture recorded against SQLite.
7. **JSON stored as text**; ingest fix-up; `$ref` accepted; extract via
   function; missing path → NULL.
8. **`${SCHEMA}` is empty.** Database file (or alias) is the isolation
   boundary. Test prefix analog: file `testfb.fdb` (not `testfb_`).
9. **Connection fields** reuse `ConnectionConfig`:

   | JSON field | Meaning |
   | --- | --- |
   | `Engine` | `firebird` |
   | `Host` | SuperServer host, or empty for embedded |
   | `Port` | `3050` SuperServer; ignored when Host empty |
   | `Database` | path or alias (`/var/lib/firebird/data/testfb.fdb`) |
   | `User` | `SYSDBA` (tests) |
   | `Pass` | SYSDBA password; never committed |
   | `Schema` | empty string |

   Connection string:
   `firebird://HOST/PORT/PATH` or `firebird://PATH` (embedded).
   `database_queue_determine_engine_type` recognizes `firebird://`
   before the SQLite fallback.
10. **Enum:** final order in [Coordination with MSSQL](#coordination-with-mssql).
    Phase 5 adds `DB_ENGINE_FIREBIRD` (and unused `DB_ENGINE_MSSQL`).
    Phase 3 **removes** `DB_ENGINE_FIREBASE`.
11. **Lookup 030 key 6 = Firebird**, dialect id 6. Packet 1384 UPDATE,
    not a new key.
12. **Yugabyte stays. MariaDB stays.** Only Cockroach is retired
    (Phase 8).
13. **Test 37 keeps number 37.**
14. **Test 31:** add `firebird` to `ENGINES`; sqruff skip if generated
    SQL is not Postgres; unsubstituted `${…}` check still runs.
15. **Bootstrap stays SQL** `SELECT … FROM queries …` (no schema
    prefix).
16. **CI uses Fedora Firebird SuperServer.** Unity may mock `isc_*`
    (DB2-style) so named `mku` does not need a live server; live
    connect is still an Exit item on Phase 5.
17. **Never log** SYSDBA password, password hashes, JWT fields.
18. **Placeholders are `?`.** `convert_named_to_positional` maps
    `DB_ENGINE_FIREBIRD` like SQLite / MySQL / DB2. Do not splice
    `:NAME` into SQL.
19. **Meet completeness + coverage fences** before Phase 12 Status
    complete.
20. **`${SIZE_COLLECTION}` is `CHAR_LENGTH(collection)`** (or the
    Phase 0 amendment), not a numeric `SIZE_*` constant.
21. **Firebase teardown is Phases 3–4**, before Firebird C (Phase 5),
    after the Firebird dialect exists (Phase 2) so AutoMigrations never
    `require('database_firebase')` with no replacement.
22. **Do not reuse Firestore C.** No `sql_parse.c` port. No HTTP seam.

## Architecture

```text
Helium acuranzo_NNNN.lua
        |  database_firebird.lua macros
        v
   SQL (CREATE/INSERT/SELECT… with native + UDR functions)
        |
        v
Hydrogen DQM  -->  firebird_execute_query
                      |
                      +-- libfbclient isc_dsql_*
                      +-- extras UDR inside Firebird (brotli, json)
                      v
                 QueryResult.data_json  (same row JSON as other engines)
```

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C `firebird/`** | `isc_*`, transactions, binds, row → JSON | Lithium, Firestore |
| **Helium `database_firebird.lua`** | macro spellings | libfbclient |
| **extras/firebird** | dnf, SYSDBA, `.fdb` create, ports | Hydrogen internals |
| **extras/brotli_udf_firebird** | UDR ABI, libbrotli | DQM |
| **Test 37** | Firebird lifecycle, full Acuranzo | Google, Firestore |

Closest C sibling: SQLite (execute + binds) plus DB2-style mocked
client for Unity. Not `src/database/firebase/`.

## Completeness fences

A phase is not done if the behavior works but Hydrogen's **normal
structures** were skipped. Phase 12 re-checks the whole table.

### Engine registration

| Must exist | Notes |
| --- | --- |
| `DB_ENGINE_FIREBIRD` | After unused `DB_ENGINE_MSSQL`, before AI |
| `firebird_get_interface()` | Same shape as `postgresql_get_interface` |
| Registry lazy-loads on `type == firebird` | |
| `normalize_engine_name("firebird")` | |
| `lua.c` engines[] includes `"firebird"` | Payload contains `database_firebird.lua` |
| `database_queue_determine_engine_type` | `firebird://` |
| `database_get_counts_by_type` | `firebird_count`; keep/replace `firebase_count` in Phase 3 |

### C tree (`src/database/firebird/`)

Suggested split (no file > 1000 lines, no `static` functions):

`types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`,
`query.{c,h}`, `transaction.{c,h}`, `prepared.{c,h}`.

No `sql_*.c`, no `http.c`, no `fns_*.c`.

### Helium

| Must exist | Notes |
| --- | --- |
| `database_firebird.lua` | All four designs; **complete** key set |
| `database.lua` engines + dialects + defaults | firebird; **no** firebase after Phase 4 |
| Lookup 030 key 6 UPDATE packet | User applies |
| `if engine` arms include firebird | 1000, 1190, 1135, 1151, re-grep; firebase arms gone |
| `test_98` luacheck | |
| Test 31 firebird generation | unsubstituted `${}` |

### Firebase teardown (Phases 3–4)

| Must be gone | Notes |
| --- | --- |
| `src/database/firebase/` | Entire tree |
| `tests/unity/src/database/firebase/` | Entire tree |
| `DB_ENGINE_FIREBASE` | |
| `extras/firebase_emulator/` | |
| `database_firebase.lua` | All four designs |
| `engines.firebase` / `query_dialects.firebase` | |
| `lua.c` `"firebase"` | |
| Test 31 `firebase` | |
| config schema `"firebase"` | |
| SECRETS `FIREBASE_*` | |

### Tests / docs

Unity under `tests/unity/src/database/firebird/`. Test 37 firebird
migrations (full design). CHANGELOG + TEST_VERSION on scripts. `jq` for
JSON. Dead-code gate clean. Docs Phase 11: MACRO_REFERENCE firebird
column, DATABASES, GUIDE, PARAMETER_BINDING, TESTING, INSTRUCTIONS,
SECRETS, STRUCTURE, SITEMAP, SchemaTool, Helium/Acuranzo READMEs,
Lithium `DATABASE-MIGRATIONS.md`.

## Coverage fences

| Fence | Rule |
| --- | --- |
| Unity, file **< 100** instrumented lines | **> 50%** |
| Unity, file **≥ 100** instrumented lines | **> 75%** |
| Combined Unity **or** blackbox | **85%** project target; new `src/database/firebird/` at or above per-file Unity fence before Phase 12 |
| Test 99 | no new file **> 1000** lines |
| Seams | mock `isc_*` for Unity (optional live server on Phase 5 Exit) |
| `static` | `mkt` fails on new `static` in `src/` |
| Dead functions | new public symbols must have a caller |

Each C phase Exit includes named `mku`, `mkp`, and the per-file coverage
fence.

## Reference Conventions

- C engine: vtable like `postgresql/`; client like `sqlite/` + mocked
  `isc_*` like `mock_libdb2`.
- `cancel_inflight`: log unsupported unless a live `isc_dsql` handle can
  be cancelled; lock in Phase 5.
- Health: `SELECT 1 FROM RDB$DATABASE` or attach success — lock in Phase 5.
- Helium: luacheck header + CHANGELOG; agent never applies packets.
- Blackbox Test 37: `TEST_ABBR` **FBD** (was `CDB`; Firestore unused
  `FBE` is gone). Ports **537x**.
- Unity: `tests/unity/src/database/firebird/` mirrors `src/`.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/`: `mkt`
  then `mkp`. After Bash: `mks`. After Lua: `test_98`. After Markdown:
  `mkl`.

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Locks approved (Firebird 4, libfbclient, empty schema, key 6 relabel, enum, teardown-before-C); no C | S | pending |
| 1 | extras/firebird README + start/stop/create db; `dnf` Firebird 4 on 3050 documented | S | pending |
| 2 | Complete `database_firebird.lua` in four designs; Test 31 generates firebird SQL; lookup 1384 packet | M | pending |
| 3 | `src/database/firebase/` and Unity gone; enum/registry/connstring have no FIREBASE; `mkt`/`mkp` green | M | pending |
| 4 | No `database_firebase.lua`; no emulator extra; Test 31 ENGINES has firebird not firebase | S | pending |
| 5 | C engine registers, `firebird://`, connect + health vs SuperServer or mock | M | pending |
| 6 | Brotli UDR + JSON ingest/extract + SHA-256 fixture green | M | pending |
| 7 | Test 37 firebird AutoMigrations **full Acuranzo** green | L | pending |
| 8 | Cockroach names gone; 7-engine loops say Firebird | M | pending |
| 9 | SchemaTool / SchemaHelper / hydrogen_flush / transaction_utils | M | pending |
| 10 | Tests 40/43/45/46/47/58 firebird configs; each named green or `[~]` with cause | L | pending |
| 11 | Docs/SITEMAP/MACRO_REFERENCE/DATABASES/SECRETS match; `mkl` green | S | pending |
| 12 | Completeness + coverage fences; dead-code clean; `mkp` | M | pending |

Effort key: S = small, M = moderate, L = large.

---

## Phase 0 — Contract lock

### Goal

Approve or amend the locks above. No `src/` or Helium edits.

### Entry gate

This document exists. Ability to read the four `database_*.lua` files,
[`acuranzo_1000.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua),
SQLite extras UDF READMEs, and `dnf` Firebird 4 on this box.

### Work items

- [ ] 0.1 Confirm Firebird 4.0 (Fedora 43 package), `libfbclient`,
      SuperServer 3050 for tests, embedded allowed.
- [ ] 0.2 Confirm Helium still emits SQL; no interpreter; no table subset.
- [ ] 0.3 Confirm native Base64 / SHA-256; Brotli UDR; JSON as text.
- [ ] 0.4 Confirm enum (with reserved `DB_ENGINE_MSSQL`) and Lookup 030
      key 6 relabel via 1384.
- [ ] 0.5 Confirm empty `${SCHEMA}`, `firebird://`, file `testfb.fdb`.
- [ ] 0.6 Confirm Yugabyte + MariaDB stay; Test 37 keeps number 37.
- [ ] 0.7 Confirm Test 31 firebird; bootstrap remains SQL.
- [ ] 0.8 Confirm Phases 3–4 teardown Firestore **before** Firebird C.
- [ ] 0.9 Confirm extras/firebird (service + `.fdb`), not firebase emulator.
- [ ] 0.10 Confirm completeness + coverage fences for Phase 12.
- [ ] 0.11 Record amendments in this document if any lock changes.

### Done means

Phase 0 Status lists every lock as approved or amended; no C/Lua/tests
changed in this phase.

### Exit gate

- Phase 0 Status = complete; user approval in Working Log.
- Next free Acuranzo migration / QueryRef re-checked on disk.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

- **2026-09-18** Plan authored. Firestore plan superseded. Fedora 43
  has Firebird 4.0.7 in `dnf`, not installed. User asked for a fresh
  Firebird plan plus separate Firebase cleanup phases, and a parallel
  MSSQL plan. Phase 0 waits for lock approval. No C this turn.

### Lessons learned

(empty until the phase runs)

---

## Phase 1 — Fedora Firebird extras

### Goal

A developer (or Test 37 later) can install Firebird 4 from Fedora
packages and create `testfb.fdb` from extras, with the same kind of
README the Brotli/SQLite extras have.

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] 1.1 `extras/firebird/README.md`: `dnf` packages, SYSDBA, port
      3050, create/drop test database, embedded vs SuperServer.
- [ ] 1.2 `start.sh` / `stop.sh` / `create_test_db.sh`. Idempotent
      start; wait for port; stop only if started.
- [ ] 1.3 extras README Database Extensions table: Firebird row
      (native Base64/SHA-256; Brotli UDR here later).
- [ ] 1.4 SECRETS.md draft names (`FIREBIRD_SYSDBA_PASSWORD`,
      `FIREBIRD_DB_PATH`). Values never committed.

### Done means

`extras/firebird/start.sh` plus create-db yields a connectable
`testfb.fdb` on 3050 on a machine with the documented RPMs.

### Exit gate

- `mks` on new scripts.
- Manual start/create/stop recorded in Status.
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

`require("database_firebird")` supplies every macro key. Test 31
generates firebird SQL without unsubstituted `${…}`. Per-engine Lua
arms know firebird. Lookup 030 key 6 packet relabels Firebase →
Firebird.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] 2.1 Write `database_firebird.lua` (four designs) with the complete
      key set. **Verify:** set difference vs the other four files is empty.
- [ ] 2.2 `database.lua`: `engines.firebird`, `query_dialects.firebird
      = 6`, `defaults.firebird`. Empty schema prefix (not underscore).
      `lua.c` `engines[]` includes `"firebird"` (may still list
      `"firebase"` until Phase 4).
- [ ] 2.3 Test 31: `ENGINES` includes firebird; unsubstituted-macro
      check. **Verify:** Test 31 green (firebase may still be present).
- [ ] 2.4 acuranzo_1000: firebird UDR/PSQL DDL arm; skip nothing that
      Firebird needs.
- [ ] 2.5 firebird arms (or shared-with-sqlite) for 1190, 1135, 1151;
      re-grep `if engine`.
- [ ] 2.6 Lookup 030 key 6 **UPDATE** packet (`acuranzo_1384.lua` or
      next free id). Icon `sql_dialect_firebird.png`. User applies.
      `test_98`.

### Done means

Test 31 generates firebird SQL for every Acuranzo migration without
`${UNSUBSTITUTED}`; luacheck clean; lookup packet handed to the user.

### Exit gate

- Test 31 run; `mks` if the script changed; `test_98`.
- Key-set diff attached to Status.

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

## Phase 3 — Firebase C / Unity teardown

### Goal

Hydrogen no longer compiles or registers a Firestore engine. This phase
is **cleanup only**. Do not add Firebird C here.

### Entry gate

Phase 2 Status complete (`database_firebird.lua` loadable so
AutoMigrations are not left without a fifth dialect file).

### Work items

- [ ] 3.1 Delete `src/database/firebase/` and
      `tests/unity/src/database/firebase/`.
- [ ] 3.2 Remove `DB_ENGINE_FIREBASE` from `database_types.h`. Do
      **not** add `DB_ENGINE_FIREBIRD` yet (Phase 5). AI numeric value
      returns to the pre-Firestore slot until Phase 5.
- [ ] 3.3 Registry, `database_manage.c`, `database_connstring.c`,
      `dbqueue/heartbeat.c`, `database_params.c`,
      `migration/transaction.c`, `database_get_counts_by_type`,
      `launch.c` — no firebase symbols.
- [ ] 3.4 `hydrogen_config_schema.json` Engine enum: drop `"firebase"`.
- [ ] 3.5 Unity fixtures that shipped `database_firebase.lua` for
      `lua_test_load_database_module` switch to firebird (Phase 2
      payload) or drop the firebase extra file.
- [ ] 3.6 `mkt` then `mkp`. Dead-code gate has no firebase symbols.
      Named `mku` that used to cover firebase are gone; do not leave
      broken CMake/Unity globs.

### Done means

`rg -n firebase src/ tests/unity/ cmake/` is empty (except comments
pointing at this plan if any). `mkt`/`mkp` green.

### Exit gate

- `mkt` then `mkp`; dead-code list cited in Status.

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

## Phase 4 — Firebase Helium / extras teardown

### Goal

No Helium dialect, Test 31 engine, extras emulator, or SECRETS names
for Firestore. Firebird dialect from Phase 2 remains.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 Delete `database_firebase.lua` from all four designs.
      `database.lua`: drop `engines.firebase`, `query_dialects.firebase`,
      `defaults.firebase`, underscore schema branch.
- [ ] 4.2 `if engine` firebase arms: 1000, 1135, 1190, gaius/glm/helium
      JSON_INGEST skips. Re-grep `firebase`.
- [ ] 4.3 `lua.c` `engines[]` and Test 31 `ENGINES`: firebird not
      firebase. Remove firebase sqruff skip.
- [ ] 4.4 Delete `extras/firebase_emulator/`. extras README: drop
      Firestore row. SECRETS.md: drop `FIREBASE_*`.
- [ ] 4.5 Lithium: keep `sql_dialect_firebase.png` only until 1384 is
      applied and `sql_dialect_firebird.png` exists; then drop the
      firebase asset and `icons-usr.txt` row if unused.
- [ ] 4.6 Test 31 green; `test_98`; `mks`; `mkl` as touched.

### Done means

`rg -n firebase elements/002-helium elements/001-hydrogen/hydrogen/extras elements/001-hydrogen/hydrogen/tests/test_31_migrations.sh docs/H/SECRETS.md`
is empty or only historical metrics / this plan’s stub.
Test 31 green with firebird.

### Exit gate

- Test 31; `test_98`; `mks`; `mkl`.

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

## Phase 5 — C engine skeleton

### Goal

Register, connstring, connect, health. No extras UDR yet.

### Entry gate

Phase 4 Status complete.

### Work items

- [ ] 5.1 `DB_ENGINE_MSSQL` (unused) then `DB_ENGINE_FIREBIRD` after
      DB2, before AI. `mkt`. Grep hardcoded `4` meaning AI.
- [ ] 5.2 `interface`, `utils` (connstring/validate/mask), `connection`,
      `query`/`transaction`/`prepared` stubs. Unity with `isc_*` mocks
      and/or live SuperServer.
- [ ] 5.3 Registry, `normalize_engine_name`, lazy init.
      `database_get_counts_by_type` `firebird_count`.
      `lua.c` already has `"firebird"` from Phase 2/4.
- [ ] 5.4 Live SuperServer health (Phase 1 extras).

### Done means

Unity connects via mock or live attach and reports healthy; `mkt` +
`mkp`; no new `static`.

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

## Phase 6 — Brotli UDR, JSON, SHA-256 fixture

### Goal

`${COMPRESS_*}` decompresses on INSERT. JSON ingest/extract match the
contract. SHA-256 fixture matches SQLite.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 `extras/brotli_udf_firebird/` README + build + install into
      Firebird plugins/UDR dir. Fixture: lua-brotli quality 11 round-trip.
- [ ] 6.2 JSON ingest + extract (PSQL or UDR). Fixtures: control-char,
      `$ref`, missing path → NULL.
- [ ] 6.3 SHA-256 `0`+`testpass` vs recorded SQLite base64 (live
      `isql` or Unity against a live statement).
- [ ] 6.4 `acuranzo_1000` firebird arm emits the CREATE FUNCTION SQL.
      Test 31 still green.

### Done means

Named verification (isql or `mku`) for brotli, json, sha256; `mkp` if
C extras changed; `mks` on extras scripts.

### Exit gate

- Commands cited in Status; coverage fence for any new C.

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

## Phase 7 — AutoMigrations / Test 37 (full Acuranzo)

### Goal

Hydrogen AutoMigrations against Firebird SuperServer apply the **full**
Acuranzo design. Same done means as Test 32.

### Entry gate

Phase 6 Status complete. Payload includes `database_firebird.lua`
(`mkt`).

### Work items

- [ ] 7.1 `hydrogen_test_37_firebird.json` (`Engine: firebird`,
      SuperServer, empty schema, `AutoMigration` + `TestMigration` as
      Test 32).
- [ ] 7.2 `tests/test_37_firebird_migrations.sh` **alongside**
      Cockroach 37. Firebird lifecycle via extras scripts.
- [ ] 7.3 Docs `docs/H/tests/test_37_firebird_migrations.md`.
- [ ] 7.4 Run until LOAD/APPLY/REVERSE match Test 32's expectations.
      Any failing migration is a dialect/UDR/`if engine` bug, not a
      skip list.

### Done means

`tests/test_37_firebird_migrations.sh` reports migration completed for
the full design; `mks`; markdown exists.

### Exit gate

- Live Test 37 firebird log path in Status.
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

## Phase 8 — Retire Cockroach

### Goal

No Cockroach **names** in active tests/configs/wrappers/current docs
(metrics history excluded).

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 Only firebird Test 37 remains.
- [ ] 8.2 Replace Cockroach configs 40/43/45/46/47/58 with firebird.
- [ ] 8.3 Engine loops, flush, SchemaTool names (behavior Phase 9).
- [ ] 8.4 Config schema enum.
- [ ] 8.5 `rg -i cockroach` on active trees: only metrics/,
      plans/complete/, and this plan's inventory.

### Done means

7-engine scripts name Firebird; Test 37 is firebird-only.

### Exit gate

- `mks`; `rg` inventory in Status; Test 37 still green after rename.

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

## Phase 9 — SchemaTool, SchemaHelper, flush

### Goal

Operator tools talk Firebird (`isql-fb` / `libfbclient`), not `psql` on
`democrdb`.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 `schematool_firebird.sh` (not an alias to postgresql).
- [ ] 9.2 schemahelper connect/apply/const.
- [ ] 9.3 `hydrogen_flush.sh` drop/recreate `testfb.fdb`.
- [ ] 9.4 `transaction_utils.sh` firebird path.

### Done means

No cockroach SchemaTool wrapper; firebird wrapper does not call `psql`.

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

## Phase 10 — Wider blackbox matrix

### Goal

Former Cockroach suites run Firebird. Full QueryRef SQL is in scope;
skips only for environmental reasons (Firebird down), not "not
implemented."

### Entry gate

Phase 9 Status complete.

### Work items

- [ ] 10.1 Test 40 auth live on firebird.
- [ ] 10.2 Tests 43, 45, 46, 47, 58.
- [ ] 10.3 Test 41/44/51/54 docs/configs.

### Done means

Status table: each suite green (or env skip). No suite still lists
Cockroach.

### Exit gate

- Named runs for 40 and 37 at minimum.

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

## Phase 11 — Docs sweep

### Goal

Current docs describe Firebird as a real engine and Cockroach as
historical. MACRO_REFERENCE has a Firebird column. DATABASES has
install + “Brotli is a UDR.” Firestore is historical only.

### Entry gate

Phase 10 Status complete.

### Work items

- [ ] 11.1 Helium GUIDE, MACRO_REFERENCE, DATABASES, TESTING_GUIDE,
      BROTLI_COMPRESSION, design READMEs, `docs/He/DATABASES/database_firebird.md`.
- [ ] 11.2 Hydrogen TESTING, INSTRUCTIONS, PARAMETER_BINDING, SECRETS,
      STRUCTURE, SITEMAP, MAIL_GUIDE, SchemaTool/SchemaHelper, tests README.
- [ ] 11.3 Lithium `DATABASE-MIGRATIONS.md`.
- [ ] 11.4 Snapshot section stays dated 2026-09-18; add an “after Phase
      8” pointer.

### Done means

`mkl` green; no active doc claims Cockroach is supported; no active doc
claims Firestore is an engine.

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

## Phase 12 — Completeness and coverage re-check

### Goal

Every completeness-fence row is true or `[~]`. Coverage fences hold.
Dead-code list has no stray firebird or firebase symbols.

### Entry gate

Phase 11 Status complete.

### Work items

- [ ] 12.1 Walk completeness table.
- [ ] 12.2 Walk coverage fences; `extras/add_coverage.sh` as needed.
- [ ] 12.3 `mkt` dead-code gate.
- [ ] 12.4 `mkp`, `mks`, `test_98`, Test 31, Test 37, Test 40.

### Done means

Fences green; Test 37 and Test 40 firebird green. Then move this plan
to `plans/complete/FIREBIRD_COMPLETE.md`; drop TODO 27; `mkl`.

### Exit gate

- Commands in 12.4 actually run; output cited in Status.

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
| Unity | Connstring, registry, query/transaction/prepared, mocked `isc_*` |
| Blackbox | Test 37 SuperServer **full** AutoMigrations. Test 40+ when Phase 10 says so |
| Coverage | See [Coverage fences](#coverage-fences) |
| Build | `mkq` ordinary C; `mkt` after add/remove `src/`; `mkp` after C; `mks` after Bash; `test_98` after Lua |

Port scheme: Test 37 → **537x**.

## Threat notes

- **Secret leakage:** SYSDBA password, `password_hash`, JWTs. Mask
  `firebird://`.
- **Hash mismatch** across engines is a login outage — Phase 6 fixture
  is mandatory.
- **UDR not loaded** → APPLY stores compressed blobs as `code`.

### Risks

| Risk | Mitigation |
| --- | --- |
| Repeating the Firestore interpreter | Phases 3–4 delete it; lock 22 forbids reuse |
| Skipping tables “until v2” | Forbidden; Test 37 is full Acuranzo |
| Firebird 5 assumed on Fedora 43 | Lock 1 is 4.0.7 |
| JOIN/LATERAL/jsonb_agg missed | Phase 2 grep + 1135/1168 arms |
| Empty 7-engine slot | Phase 8 only after Phase 7 green |
| Fighting MSSQL over the enum | Reserved `DB_ENGINE_MSSQL`; key 5 untouched |
| Helium ID drift | Re-check disk at packet time |
| Coverage late | Per-phase fence |

## Working Log (cross-phase memory)

### Decisions log

- **(Plan authored, 2026-09-18)** FIREBIRD created as a greenfield SQL
  engine plan after Firestore was recognized as the wrong product.
  Firebase Working Log lives in
  [`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).
  Cleanup is Phases 3–4. Cockroach retirement is Phase 8. Lookup 030
  key 6 is relabelled, not reused from key 5. Sister plan
  [`MSSQL.md`](/docs/H/plans/MSSQL.md).

### Surprises / deviations (historical, still true)

- Fedora 43 packages Firebird **4.0.7**, not 5.
- Lookup 030 key 5 is "MS SQL Server" and unused in C; do not steal it.
  Key 6 is still labelled Firebase until packet 1384.
- Helium `database.lua` currently special-cases `engine == 'firebase'`
  for underscore schema prefixes. Firebird must not inherit that.
- `INSERT_KEY_RETURN` is trailing `RETURNING col` in QueryRefs.
  Firebird supports that (unlike SQL Server `OUTPUT`).
- 1168 uses `jsonb_agg` / `jsonb_array_elements_text` — PostgreSQL
  literals inside a QueryRef. Firebird needs an arm.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After add/remove `src/`: `mkt` then `mkp`.
- After bash: `mks`. After Lua: `test_98`. After docs: `mkl`.
- Never apply Helium packets; hand them to the user.
- Payload rebuild (`mkt`) is required before Test 31–38 see new
  `database_firebird.lua`.
- Do not `dlopen` libpq for firebird.
- Cross-check SHA-256 against SQLite `crypto_sha256` before declaring
  login green.
