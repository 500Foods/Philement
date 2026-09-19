<!-- markdownlint-disable MD007 MD024 -->
# Firebase Engine Plan (superseded)

> **Superseded 2026-09-18.** Cloud Firestore was the wrong fifth engine:
> Helium emits SQL, and Firestore is not a SQL server. The replacement is
> Firebird ([`FIREBIRD.md`](/docs/H/plans/FIREBIRD.md)). MS SQL Server
> (Lookup 030 key 5) is a separate plan ([`MSSQL.md`](/docs/H/plans/MSSQL.md)).
> Do not resume Phase 8 or add C to `src/database/firebase/`. Firebase
> teardown is FIREBIRD Phases 3–4. This file is the historical Working Log
> only.

# Firebase Engine Plan

## Status at a glance

Phases **0–7 complete** (2026-09-17). Next: **Phase 8** — discussed,
locks in that phase's Working Log, **wait for go** (no C this pause).
Eight of seventeen phases done. What is left is SELECT/JOINs, then the
live Test 37 + 7-engine matrix — that is why this is still a long plan.

| Phase | Status | Remaining |
| --- | --- | --- |
| 0 Contract lock | complete | — |
| 1 Emulator extras | complete | — |
| 2 Helium dialect | complete | — |
| 3 C register / connect | complete | — |
| 4 In-process `FB_*` | complete | — |
| 5 DDL | complete | — |
| 6 INSERT / UPDATE / DELETE | complete | — |
| 7 INSERT…SELECT / RETURNING | complete | — |
| 8 SELECT + `:NAME` binds | pending | **Moderate** |
| 9 JOIN / LATERAL | pending | **Difficult** |
| 10 Test 37 full Acuranzo | pending | **Difficult** |
| 11 Retire Cockroach names | pending | **Quick** |
| 12 SchemaTool / flush | pending | **Moderate** |
| 13 Tests 40–58 firebase matrix | pending | **Difficult** |
| 14 Docs | pending | **Quick** |
| 15 Coverage / completeness | pending | **Moderate** |
| 16 Production JWT (optional) | pending | **Quick** |

Remaining: 3 Difficult (9, 10, 13), 3 Moderate (8, 12, 15), 3 Quick
(11, 14, 16). Phase 16 may park.

**Parity:** Firebase is the fifth Hydrogen engine, not a new API. Match
PostgreSQL / SQLite / MySQL / DB2: same `QueryRequest` / `QueryResult`,
same `parse_typed_parameters` → `convert_named_to_positional` → bind,
same `data_json` array of row objects. Do not change those engines.
Firestore has no SQL server, so C interprets the SQL; that is the only
intentional difference.

## Purpose

Replace the **CockroachDB** engine slot with a real **Firebase / Cloud
Firestore** implementation. Cockroach was never a fifth Hydrogen engine:
it is a PostgreSQL alias (same `libpq` path, same
`${env.ACURANZO_DB_TYPE}` = `postgresql`, a different schema name).
YugabyteDB is the same kind of alias and **stays**. Firebase will not
alias anything. It is the **fifth** `DatabaseEngineInterface`
implementation in C and the **fifth** Helium dialect
(`database_firebase.lua`).

**There is no v1 subset and no v2 follow-up.** When Hydrogen's Firebase
engine and Helium's `database_firebase.lua` are in place, AutoMigrations
must apply the **same Acuranzo Lua files** that Test 32 applies on
PostgreSQL. QueryRefs that later run through Conduit/auth/mail must
execute too. Phases below sequence *capability* (functions before DML,
DDL before JOINs) because that is how you build an engine, not because
some tables are "out of scope."

Helium migrations stay **SQL**. Macros still expand types and function
wrappers. The C engine **interprets that SQL against Firestore** and
evaluates Brotli / Base64 / SHA-256 / `json_ingest` / timestamps
**in-process** — Firestore cannot load C UDFs the way PostgreSQL, MySQL,
SQLite, and DB2 do.

This is the only active plan for that swap.

**Session brief:** A new conversation may start with only a pointer to
this file. Do not reconstruct Cockroach vs Firebase from git history or
from memory of a prior chat. Open [Status at a glance](#status-at-a-glance),
then [Resuming Work](#resuming-work), then the next incomplete phase
only.

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

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, record lessons learned, **stop for review**. Do not begin the next
  phase in the same turn unless asked.
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
10. **Never log** Firebase service-account JSON, OAuth access tokens,
    refresh tokens, or Firestore document bodies that contain secrets
    (password hashes, JWTs, OTP codes) in normal logs or test artifacts.
11. **Do not increment `TEST_COUNTER` in blackbox scripts.** The framework
    owns the counter.
12. **Do not delete the Cockroach slot until Phase 11.** Earlier phases add
    Firebase beside it. Phase 11 is the swap so the 7-engine matrix never
    silently becomes six.
13. **Mirror the other engines.** When a Firebase choice is ambiguous,
    do what PostgreSQL / SQLite / MySQL / DB2 already do
    (`PARAMETER_BINDING.md`, `QueryResult.data_json`, named `:NAME`
    markers). Do not invent a Firebase-only execute path and do not
    change the other engines to accommodate it.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-17):** Phase 7 complete.
INSERT…SELECT / CTE / MAX+1 / RETURNING Unity-green. Two successive
LOAD-shaped inserts get `query_id` 1 then 2; `RETURNING query_id`
is `[{"query_id":1}]`. `mkt`/`mkp` green, no new `static`. Phase 7 C
(`sql_select.{c,h}` and Unity) may still be uncommitted in the tree —
do not revert it. Phase 8 discussed; locks are in the Phase 8 Working
Log (mirror SQLite bind path; no `:NAME` literal splice). **Do not
start Phase 8 C until the user says go.** No Test 37, no Cockroach
deletes. Last numbered Acuranzo file on disk: `acuranzo_1383.lua`
(Lookup 030 key 6 applied).

Keep this block current when a phase finishes (date, result, next phase
number). It is the first thing a new session reads.

### Resume here next session

1. This file is the source of truth. Do not start a second Firebase or
   "drop Cockroach" plan.
2. Read **Status at a glance**, **CURRENT PAUSE POINT**, the Phase
   Index **Status** column, and **Working Log (cross-phase memory)** —
   including JSON / Brotli / Base64 / SHA-256 locks. Parity with the
   other four engines is a standing rule.
3. Confirm the prior phase Status is actually complete (re-read its Exit
   gate; do not trust chat memory).
4. Re-read **only** the next phase: Goal + Work items + Done means +
   Exit gate. Skim a lock in
   [Proposed design locks (Phase 0)](#proposed-design-locks-phase-0)
   or [The four solvable headaches](#the-four-solvable-headaches) when
   that phase cites it. Do not re-read the whole document.
5. If the phase needs a Helium packet: re-check disk for the next
   Acuranzo migration / QueryRef
   (`ls elements/002-helium/acuranzo/migrations/acuranzo_*.lua`).
   Snapshot at this pause: last **`acuranzo_1383.lua`**, Lookup **030**
   key **6** = Firebase (applied). `DB_ENGINE_AI` is still the Unity
   mock slot. Do not trust the snapshot; re-check disk.
6. Discuss, get explicit approval, implement that phase only, verify the
   Exit gate, update Status + Working Log + this pause point, **stop**.

### Session checklist

1. CURRENT PAUSE POINT → first Status that is not complete.
2. Working Log decisions that affect this phase (especially the four
   function contracts).
3. Baseline as the phase names it: `mkq` or `mkt`; named `mku`; Test 31
   / 37 / 40 when listed. No live `firestore.googleapis.com`.
4. One phase: questions → approval → implement → verify → update this
   plan → stop for review.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new engine, after Auth Finale / quality gates |
| **Effort** | XL (SQL-on-Firestore interpreter + in-process UDF-class functions + Helium dialect + emulator CI + Cockroach retirement) |
| **Done** | ~47% — Phases 0–7 complete; 3 Difficult / 3 Moderate / 3 Quick left |
| **Why this shape** | Cockroach never earned a C implementation. Firebase cannot lean on PostgreSQL and cannot load C UDFs. The existing ~380 Lua files emit SQL; the engine must run that SQL. |
| **Do not start casually** | Touches `DatabaseEngine` enum, registry, DQM, Helium `database.lua` for four designs, Test 31/37/71, the 7-engine blackbox matrix, SchemaTool, Lookup 030, and a SQL interpreter. |

Backlog: [TODO.md item 27](/docs/H/TODO.md).

---

## Snapshot: what exists today (2026-09-16)

Do not re-implement these; they are constraints.

### Four real engines, three aliases

| Operator name | Hydrogen C | Helium dialect | Typical config `Engine` | Notes |
| --- | --- | --- | --- | --- |
| PostgreSQL | `src/database/postgresql/` | `database_postgresql.lua` | `${env.ACURANZO_DB_TYPE}` → `postgresql` | Real |
| SQLite | `src/database/sqlite/` | `database_sqlite.lua` | `sqlite` | Real |
| MySQL | `src/database/mysql/` | `database_mysql.lua` | `${env.CANVAS_DB_TYPE}` | Real |
| DB2 | `src/database/db2/` | `database_db2.lua` | `db2` | Real |
| MariaDB | **MySQL** | **mysql** | same as MySQL, schema `demomrdb` | Alias |
| YugabyteDB | **PostgreSQL** | **postgresql** | `${env.YUGABYTE_DB_TYPE}`, `YUGABYTE_DB_*` | Alias; **stays** |
| CockroachDB | **PostgreSQL** | **postgresql** | `${env.ACURANZO_DB_TYPE}`, schema `testcrdb` / `democrdb` | Alias; **this plan retires it** |

`grep` of `src/**/*.c` and `src/**/*.h` for `cockroach` is **empty**. The
C enum is:

```c
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_AI,         // Unity mock slot — do not reuse
    DB_ENGINE_MAX
} DatabaseEngine;
```

Helium `database.lua` `engines` / `query_dialects`,
`lua_load_database_module` (`lua.c` hardcoded
`{"sqlite","postgresql","mysql","db2"}`), `normalize_engine_name`,
Test 31, and Test 71 all list **four** dialects. Cockroach never had
`database_cockroach.lua`.

### How Cockroach actually runs

[`hydrogen_test_37_cockroachdb.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_37_cockroachdb.json):

- `"Engine": "${env.ACURANZO_DB_TYPE}"` (value `postgresql`)
- Host/port/user/pass from `ACURANZO_DB_*` (same Postgres host as Test 32)
- `"Schema": "testcrdb"`, bootstrap `FROM testcrdb.queries`
- Connection string is still `postgresql://…` so
  `database_queue_determine_engine_type` returns `DB_ENGINE_POSTGRESQL`

### Cockroach-named files (main tree)

Replace in Phase 11, not before:

| Path |
| --- |
| [`tests/test_37_cockroachdb_migrations.sh`](/elements/001-hydrogen/hydrogen/tests/test_37_cockroachdb_migrations.sh) |
| [`docs/H/tests/test_37_cockroachdb_migrations.md`](/docs/H/tests/test_37_cockroachdb_migrations.md) |
| `tests/configs/hydrogen_test_{37,40,43,45,46,47,58}_*cockroachdb*.json` (8 files; 43 has `_no_default`) |
| [`extras/schematool/schematool_cockroachdb.sh`](/elements/001-hydrogen/hydrogen/extras/schematool/schematool_cockroachdb.sh) |
| `tests/artifacts/oidc_idp_keys_45_cockroachdb/` |

Edit-in-place: Test 40/41/43/45/46/47/51/54/58 scripts and markdown;
`tests/lib/transaction_utils.sh`; `extras/hydrogen_flush.sh`;
SchemaTool Lua; `hydrogen_config_schema.json`; TESTING / PARAMETER_BINDING
/ SECRETS / SITEMAP / STRUCTURE / MAIL_GUIDE / SchemaTool docs / Helium
DATABASES / GUIDE / MACRO_REFERENCE.

Historical metrics JSON/TXT under `docs/H/metrics/` **stay**.

### How the other engines get Base64 / Brotli / SHA-256 / TZ

This is the analog Firebase must match — not by loading a `.so` into
Google, but by putting the same work in Hydrogen.

| Need | PostgreSQL | MySQL | SQLite | DB2 |
| --- | --- | --- | --- | --- |
| Base64 decode | native `DECODE(...,'base64')` | native `FROM_BASE64` | **sqlean `crypto.so`** loaded from `/usr/local/lib/crypto.so` in [`sqlite/connection.c`](/elements/001-hydrogen/hydrogen/src/database/sqlite/connection.c) | C UDF [`extras/base64decode_udf_db2`](/elements/001-hydrogen/hydrogen/extras/base64decode_udf_db2/README.md) |
| Base64 encode | native `ENCODE` | native `TO_BASE64` | sqlean `crypto_encode` | C UDF [`extras/base64encode_udf_db2`](/elements/001-hydrogen/hydrogen/extras/base64encode_udf_db2/README.md) |
| Brotli decompress | C extension [`extras/brotli_udf_postgresql`](/elements/001-hydrogen/hydrogen/extras/brotli_udf_postgresql/README.md) | plugin [`extras/brotli_udf_mysql`](/elements/001-hydrogen/hydrogen/extras/brotli_udf_mysql/README.md) | loadable [`extras/brotli_udf_sqlite`](/elements/001-hydrogen/hydrogen/extras/brotli_udf_sqlite/README.md) at `/usr/local/lib/brotli_decompress.so` | C UDF [`extras/brotli_udf_db2`](/elements/001-hydrogen/hydrogen/extras/brotli_udf_db2/README.md) |
| SHA-256 password hash | native `SHA256` + `ENCODE` | native `SHA2` + `TO_BASE64` | sqlean `crypto_sha256` | DB2 `HASH(..., 2)` + encode UDF |
| `json_ingest` | plpgsql in `database_postgresql.lua` (created by acuranzo_1000) | MySQL stored function in `database_mysql.lua` | passthrough `(` `)` — store as text | SQL UDF; extra `JSON_INGEST_SCHEMA` because JSON2BSON rejects `$ref` |
| `CONVERT_TZ` | native | native | [`extras/converttz_udf_sqlite`](/elements/001-hydrogen/hydrogen/extras/converttz_udf_sqlite/README.md) `/usr/local/lib/convert_tz.so` | native |

SQLite is the closest story: the **database cannot do crypto/brotli
until Hydrogen `dlopen`s extras and `sqlite3_load_extension`**. For
Firebase there is nothing to `dlopen` inside Firestore. Hydrogen **is**
the database process, so the functions live in
`src/database/firebase/` and the extras folder holds **emulator
install/start**, not a Google-side `.so`.

See [extras/README.md](/elements/001-hydrogen/hydrogen/extras/README.md)
"Database Extensions" and
[BROTLI_COMPRESSION.md](/docs/He/BROTLI_COMPRESSION.md): Lua compresses
strings >1KB in the migration payload; INSERT expressions call
`${COMPRESS_START}...${COMPRESS_END}` so the **engine** decompresses
into the `code` column at INSERT time. Firebase must evaluate that
expression in C or the stored `code` is still a wrapped base64 blob and
APPLY will not run SQL.

---

## The four solvable headaches

Cockroach failed because it was never an engine. These four will hurt
and **are in scope for this plan** — they are the same contracts Helium
already spent years making identical across PG / MySQL / SQLite / DB2.
Phase 4 exists so they are proven in Unity **before** Test 37 tries to
ingest 380 migrations.

Acuranzo README design notes already assume all four engines have
working Base64 decode and JSON_INGEST. Firebase joins that list; it
does not get a pass.

### 1. JSON ingest and extract

This is the one that took the most cross-engine work. It is **not**
"store a map in Firestore and hope."

**Ingest (`${JSON_INGEST_START}` / `_END`, plus `JIS`/`JIE`):**
migrations wrap `collection` (diagrams, lookup JSON, JSON Schema
documents) so the engine stores valid JSON even when the Lua source
contains raw newlines/tabs/CRs *inside strings*. PostgreSQL plpgsql,
MySQL `json_ingest`, and DB2 `JSON_INGEST` all: (1) fast-path if already
valid, (2) walk the string and escape controls only inside quotes, (3)
parse again. SQLite is a passthrough `(` `)` because it stores text and
does not validate.

Firebase cannot skip this. Diagram JSON and lookup `collection` blobs
go through ingest on INSERT. `FB_JSON_INGEST` must implement the same
fix-up (jansson parse; on failure, control-char pass; parse again).
Unity fixtures: already-valid JSON; JSON with `\n`/`\t`/`\r` inside a
string; invalid JSON that must still fail.

**`$ref` / `$id` / `$schema` (`JSON_INGEST_SCHEMA_*`):** DB2's
`JSON2BSON` treats nested `{ "$ref": ... }` as a BSON DBRef and
rejects it, so DB2 has a **second** function using the ISO SQL/JSON
parser (`acuranzo_1000` + `acuranzo_1154`). PostgreSQL jsonb, MySQL
`JSON_VALID`, and SQLite text have no reserved-key semantics, so their
SCHEMA macros **alias** normal ingest. Firebase aliases ingest too
(PG/SQLite/MySQL, **not** DB2 JSON2BSON). Unity must insert a JSON
Schema document with `$ref` and read it back intact.

**Extract (`${JRS}` / `${JRM}` / `${JRE}`):** used in QueryRefs
(e.g. 1114/1124 `collection` → `$.icon`). Expansions today:

| Engine | Shape |
| --- | --- |
| PostgreSQL | `col::json ->> '$.icon'` |
| MySQL | `col ->> '$.icon'` |
| SQLite | `json_extract(col, '$.icon')` |
| DB2 | `JSON_VALUE(col, '$.icon' DEFAULT NULL ON ERROR)` |

Firebase: `FB_JSON_VALUE(col, '$.icon')` via jansson. Missing path →
SQL NULL, not an error (DB2 `DEFAULT NULL ON ERROR` / sqlite null).

**Storage:** `collection` (and other `${JSON}` columns) are
**stringValue of the ingested JSON text**, same idea as SQLite. Do
**not** store Firestore `mapValue` for these columns: key order,
number-vs-string, and `$ref` keys would diverge from the other engines
and from `FB_JSON_VALUE`. Native maps are a later optimization only if
extract + round-trip fixtures still match; they are not the default.

**Round-trip:** SELECT of a `collection` field returns the ingested
text. Byte-for-byte identity with PostgreSQL jsonb is **not** required
(jsonb canonicalizes). Identity with **SQLite** (store what ingest
returned) is the target. SchemaHelper decode
([`schemahelper_qdecode.lua`](/elements/001-hydrogen/hydrogen/extras/schematool/lua/schemahelper_qdecode.lua))
already has per-engine regexes for Brotli/Base64 wrappers; Phase 12
adds `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('…'))` and
`FB_JSON_INGEST(…)`.

### 2. Base64

Used as the wire form around compressed `code` and as the outer encoding
of SHA-256 password hashes. SQLite: sqlean `CRYPTO_DECODE` /
`CRYPTO_ENCODE`. DB2: extras C UDFs (chunked CLOBs). PG/MySQL: native.

Firebase: OpenSSL (or existing Hydrogen helpers) in `fns_base64.c`.
Alphabet is standard `+/` with `=` padding (same as the other engines
and as SchemaHelper's Lua decoder). No URL-safe variant.

### 3. Brotli

Lua (`database.lua`) compresses multiline blocks >1KB at **generation**
time, then wraps with `${COMPRESS_START}` … `${COMPRESS_END}` so the
**engine** decompresses on INSERT into `queries.code`. Skip eval → APPLY
tries to run a base64 blob as SQL.

Firebase: `libbrotlidec` in `fns_brotli.c` (same library as
`extras/brotli_udf_*`). Unity: compress with lua-brotli quality 11 (or
a checked-in fixture from an existing migration), decompress in C,
original bytes match. Quality 11 on the Lua side is a lock; do not
"helpfully" recompress at a different quality.

### 4. SHA-256 password hashes

`${SHA256_HASH_START}` / `_MID` / `_END` seed `password_hash` as
**base64(SHA256(concat(account_id, password)))**. Each engine spells it
differently; the **bytes must match** or Test 40 login against a
firebase-migrated admin account fails.

| Engine | Spell |
| --- | --- |
| PostgreSQL | `ENCODE(SHA256(CONCAT(id, pass)::bytea), 'base64')` |
| MySQL | `TO_BASE64(UNHEX(SHA2(CONCAT(id, pass), 256)))` |
| SQLite | `crypto_encode(crypto_sha256(id \|\| pass), 'base64')` |
| DB2 | `CAST(BASE64ENCODEBINARY(HASH(CAST(CONCAT(id, pass) AS VARCHAR(256) FOR BIT DATA), 2)) AS CHAR(128))` |

Firebase: `FB_SHA256_B64(id, pass)` — concat as UTF-8 bytes, SHA-256,
standard base64. Phase 4 Unity fixture: `account_id=0` + a known
password compared to a hash taken from SQLite or PostgreSQL (record both
in the test). That fixture is a **login compatibility gate**, not a
nice-to-have.

These four are solvable because they are functions Hydrogen already
links libraries for (jansson, OpenSSL, libbrotli). They are headaches
because the **contracts** are subtle (control-char ingest, `$ref`,
hash concatenation order, decompress-on-INSERT). Cockroach had no
contract — it was `postgresql://` with a different schema name.

---

## Firebase installation and the UDF equivalent

Read this before Phase 0 locks. Firestore is not a SQL server you
`apt install` and then `CREATE EXTENSION`.

### What you install on the machine (CI / dev)

| Piece | Why | Notes |
| --- | --- | --- |
| Node.js 20+ | `firebase-tools` is an npm CLI | Already common on this box |
| Java 21 JRE | Firestore emulator is a JVM app | Same class of dep as DB2 client, not optional for Test 37 |
| `firebase-tools` | `firebase emulators:start --only firestore` | `npm i -g firebase-tools` or a pinned `npx` in extras |
| libcurl | Firestore REST | Hydrogen already links it (OIDC RP) |
| OpenSSL 3 | Service-account JWT in production; SHA-256 always | Already linked |
| libbrotli | In-process decompress (same library the other extras UDFs use) | `libbrotlidec`; PostgreSQL/MySQL/SQLite extras already depend on it |

There is **no** Firestore C SDK, **no** `CREATE EXTENSION`, **no**
`sqlite3_load_extension`, **no** MySQL plugin directory.

### Emulator (blackbox, like a local Postgres)

```bash
# once
npm i -g firebase-tools
# Java must be on PATH (firebase emulators:exec checks it)

# per run (Test 37 / extras script)
firebase emulators:start --only firestore --project hydrodemo
# default Firestore emulator: 127.0.0.1:8080
```

REST base (emulator, no auth):

```text
http://127.0.0.1:8080/v1/projects/hydrodemo/databases/(default)/documents/...
```

Production REST:

```text
https://firestore.googleapis.com/v1/projects/PROJECT/databases/(default)/documents/...
Authorization: Bearer <service-account access token>
```

Phase 1 lands `extras/firebase_emulator/` (README + start/stop script)
modeled on how extras documents SQLite crypto / Brotli install, **not**
modeled on a UDF `.so`. Test 37 owns lifecycle if it started the
emulator.

### What you do **not** install into Firebase

- C UDFs, Cloud Functions, Firebase Auth, Realtime Database, FCM.
- Cloud SQL / Firebase Data Connect (those are PostgreSQL again — the
  Cockroach pattern).

### UDF-class functions: in-process in Hydrogen

| Function the SQL dialect needs | Firebase implementation |
| --- | --- |
| `FB_BASE64_DECODE` / `FB_BASE64_ENCODE` | C in `firebase/fns_base64.c` (OpenSSL BIO or existing Hydrogen helpers) |
| `FB_BROTLI_DECOMPRESS` | C in `firebase/fns_brotli.c` (`libbrotlidec`, same as extras UDFs) |
| `FB_SHA256_B64(a, b)` | C in `firebase/fns_sha256.c` — `SHA256(concat(a,b))` then base64, matching the `${SHA256_HASH_*}` contract used to seed `password_hash` |
| `FB_JSON_INGEST` | C in `firebase/fns_json.c` — see [The four solvable headaches](#the-four-solvable-headaches): control-char fix-up, `$ref` accepted, store as JSON **text** |
| `FB_NOW()` | C clock → Firestore `timestampValue` / ISO text per lock |
| `FB_CONVERT_TZ` | C in `firebase/fns_tz.c` (do not copy SQLite extra's UTC-only stub; use IANA via existing OS zoneinfo or document a library). Needed wherever QueryRefs call `CONVERT_TZ` |
| `FB_JSON_VALUE` (`${JRS}`/`${JRM}`/`${JRE}`) | jansson extract |
| `LENGTH` / `COALESCE` / `MAX` / `CONCAT` / `CAST` | interpreter builtins |

`CREATE FUNCTION` / `DROP FUNCTION` / `${BROTLI_DECOMPRESS_FUNCTION}` /
`${JSON_INGEST_FUNCTION}` / `${CONVERT_TZ_FUNCTION}` on firebase are
**SQL comments** (no-ops). acuranzo_1000 already skips JSON ingest
creation for SQLite and emits DB2/MySQL-only UDF DDL. Add
`engine ~= 'firebase'` next to those skips so 1000 does not try to
`CREATE FUNCTION` against Firestore.

### extras layout (Phase 1)

```text
elements/001-hydrogen/hydrogen/extras/
  firebase_emulator/
    README.md          # install firebase-tools, Java, start/stop, ports
    start.sh           # idempotent start, wait for :8080
    stop.sh            # stop only if we started it
    firebase.json      # emulators.firestore.port = 8080
```

Do **not** put a `brotli_udf_firebase/` that uploads a Cloud Function.
The C is in `src/database/firebase/fns_*.c`. extras README "Database
Extensions" table gets a Firebase row: "in-process in Hydrogen; emulator
only."

---

## Why Helium still emits SQL

Acuranzo files contain literal SQL verbs. Macros only fill types and
function wrappers:

```lua
CREATE TABLE ${SCHEMA}${QUERIES} (
    query_id ${INTEGER} NOT NULL,
    code     ${TEXT_BIG} NOT NULL,
    ${COMMON_CREATE}
    ${PRIMARY}(query_id),
    ${UNIQUE}(query_ref, query_type_a28)
);
```

and INSERT expressions such as:

```sql
${COMPRESS_START}'<brotli+base64>'${COMPRESS_END}
${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
${JSON_INGEST_START}[==[ { ... } ]==]${JSON_INGEST_END}
```

Rewriting ~380 files into JSON ops, or applying only an "auth-minimum"
table list, is out of scope. `database_firebase.lua` is a fifth dialect
file with the **same keys** as the other four. The C engine is an
**SQL-subset interpreter** whose subset is "whatever Acuranzo actually
emits," not a new query language.

Rare `if engine == '…'` files already exist and **must grow a firebase
arm** (or a shared arm that firebase can share with sqlite):

| File | Why it branches |
| --- | --- |
| [`acuranzo_1000.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua) | UDF DDL per engine |
| [`acuranzo_1190.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1190.lua) | ALTER COLUMN nullability (sqlite table rebuild) |
| [`acuranzo_1135.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1135.lua) | per-engine DDL |
| [`acuranzo_1151.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1151.lua) | mysql vs others |

Phase 2 greps `if engine` again; do not assume this list stays complete.

---

## `database_firebase.lua` — complete macro table

Every key that appears in **any** of `database_postgresql.lua`,
`database_mysql.lua`, `database_sqlite.lua`, `database_db2.lua` must
exist on firebase so `replace_query` never leaves `${UNSUBSTITUTED}`.
Values below are the **approved** expansions (Phase 2 implements).
Phase 0 amended `${SIZE_COLLECTION}` only.

`${SCHEMA}` for firebase is a collection **prefix** with underscore
(`testfb_`), not `schema.`. Empty schema → no prefix. Tables become
collections `testfb_queries`, `testfb_lookups`, …

### Types (Firestore field kinds)

| Macro | Firebase expansion (SQL spelling the interpreter accepts) | Firestore write |
| --- | --- | --- |
| `${INTEGER}` / `${INTEGER_SMALL}` | `integer` | `integerValue` |
| `${INTEGER_BIG}` | `bigint` | `integerValue` (string if > 2^53) |
| `${FLOAT}` / `${FLOAT_BIG}` | `real` | `doubleValue` |
| `${TEXT}` / `${VARCHAR_*}` / `${CHAR_*}` | `text` | `stringValue` |
| `${TEXT_BIG}` | `text` | `stringValue` (fail closed > 900 KiB after decompress) |
| `${JSON}` | `json` | `stringValue` of canonical JSON (queryable via `FB_JSON_VALUE`; maps are a later optimization, not a second schema) |
| `${DATE}` / `${TIME}` / `${DATETIME}` / `${TIMESTAMP}` / `${TIMESTAMP_TZ}` | `timestamp` | `timestampValue` |
| `${SERIAL}` | `integer` | integer + counters doc (see keys) |
| `${PRIMARY}` | `PRIMARY KEY` | document id from PK columns |
| `${UNIQUE}` | `UNIQUE` | engine-enforced uniqueness (query-before-insert or id) |
| `${NOW}` | `FB_NOW()` | timestamp |
| `${DUMMY_TABLE}` | empty (like PG/MySQL/SQLite) | |
| `${REORG}` | `-- REORG TABLE` | no-op, like PG |

`${SIZE_INTEGER}` / `${SIZE_INTEGER_BIG}` / `${SIZE_INTEGER_SMALL}` /
`${SIZE_FLOAT}` / `${SIZE_FLOAT_BIG}` / `${SIZE_TIMESTAMP}` stay numeric
string constants (4/8/20 as in sqlite) for diagrams; they are not
Firestore types. `${SIZE_COLLECTION}` is `LENGTH(collection)` — same as
PostgreSQL, MySQL, SQLite, and DB2.

### Keys, time, JSON extract

| Macro | Proposed firebase |
| --- | --- |
| `${INSERT_KEY_START}` | SQL comment `--` plus trailing space (same as PG) |
| `${INSERT_KEY_END}` | empty |
| `${INSERT_KEY_RETURN}` | `RETURNING` plus trailing space |
| `${SESSION_SECS}` | `FB_SESSION_SECS(:SESSION_START)` |
| `${TRMS}` / `${TRME}` | `FB_TIME_ADD(${NOW}, -(` … `), 'minutes')` |
| `${TRFS}` / `${TRFE}` | `FB_TIME_ADD(${NOW}, (` … `), 'seconds')` |
| `${TRFMS}` / `${TRFME}` | `FB_TIME_ADD(${NOW}, (` … `), 'minutes')` |
| `${JRS}` / `${JRM}` / `${JRE}` | `FB_JSON_VALUE(` / comma-space / `)` |
| `${JIS}` / `${JIE}` | same as JSON_INGEST |
| `${DROP_CHECK}` | `SELECT FB_REFUSE_DROP('${SCHEMA}${TABLE}') WHERE EXISTS (SELECT 1 FROM ${SCHEMA}${TABLE})` (sqlite shape) |

DB2-only extras that must still be defined so a copy-paste of
`database.lua` keys never misses: `${BASE64ENCODE_START}` / `_END`,
`${BASE64ENCODEBINARY_START}` / `_END`, `${DATETIME_FORMAT}`,
`${TIMESTAMP_FORMAT}`, `${CONVERT_TZ_FUNCTION}` (comment).

### Encoding / hashing / ingest (the UDF surface)

| Macro | PostgreSQL today | Firebase proposed |
| --- | --- | --- |
| `${BASE64_START}` / `${BASE64_END}` | `CONVERT_FROM(DECODE(` / `'base64'), 'UTF8')` | `FB_BASE64_DECODE(` / `)` |
| `${COMPRESS_START}` / `${COMPRESS_END}` | `${SCHEMA}brotli_decompress(DECODE(` / `'base64'))` | `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE(` / `))` |
| `${SHA256_HASH_START}` / `_MID` / `_END` | `ENCODE(SHA256(CONCAT(` / comma / `)::bytea), 'base64')` | `FB_SHA256_B64(` / comma-space / `)` |
| `${JSON_INGEST_START}` / `_END` | `${SCHEMA}json_ingest (` / `)` | `FB_JSON_INGEST(` / `)` |
| `${JSON_INGEST_SCHEMA_START}` / `_END` | alias of json_ingest | same as JSON_INGEST (accept `$ref`) |
| `${BROTLI_DECOMPRESS_FUNCTION}` | `CREATE FUNCTION … LANGUAGE c` | `-- firebase: FB_BROTLI_DECOMPRESS is in-process` |
| `${JSON_INGEST_FUNCTION}` | plpgsql body | `-- firebase: FB_JSON_INGEST is in-process` |
| `${JSON_INGEST_SCHEMA_FUNCTION}` | empty on PG | empty |
| `${CONVERT_TZ_FUNCTION}` | (sqlite only) | `-- firebase: FB_CONVERT_TZ is in-process` |

Usage stays identical in migrations:

```text
${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
→ FB_SHA256_B64('0', '<password>')
```

Must match the other engines' hash bytes or Test 40 login against a
firebase-migrated admin account fails. Unity fixture: same password +
account_id `0` → same base64 as a known PostgreSQL/SQLite result.

### Dialect id

`query_dialects.firebase = 6` (Lookup 030 key 6). Do not reuse key 5
(MS SQL Server). Additive lookup seed; do not edit `acuranzo_1055.lua`
reverse-in-place.

Copy `database_firebase.lua` into Acuranzo, Gaius, GLM, and Helium
designs (same as the other four files).

---

## SQL subset the C engine must run

This is the Acuranzo/Helium surface, from the Lua files and from QueryRefs
they seed. The interpreter may reject anything outside this list with
`DB_ERR_OTHER` and a clear message (that is a bug in the interpreter or
an undocumented migration construct — extend the interpreter, do not
skip the migration).

### DDL (Test 37 APPLY)

- `CREATE TABLE` with columns, `NOT NULL`, `PRIMARY KEY (cols…)`,
  `UNIQUE (cols…)`
- `DROP TABLE` (honor `${DROP_CHECK}` first)
- `CREATE INDEX` / `CREATE UNIQUE INDEX` → Firestore composite index
  **metadata** the engine records; emulator is often auto-index; production
  Phase 16 must export `firestore.indexes.json`
- `ALTER TABLE … ADD/DROP/ALTER COLUMN`, `RENAME TO` (1190 sqlite-style
  rebuild is acceptable as the firebase arm)
- `CREATE FUNCTION` / `DROP FUNCTION` → no-op success if the name is
  one of the in-process functions; error otherwise
- `${REORG}` → no-op

### DML (Test 37 APPLY + seeds)

- `INSERT INTO t (cols) VALUES (row), (row), …`
- `INSERT INTO t (cols) WITH cte AS (SELECT COALESCE(MAX(id),0)+1 …) SELECT …`
- `UPDATE t SET … WHERE …`
- `DELETE FROM t WHERE …`
- Expression eval in those lists: literals, `NULL`, `FB_*` functions,
  `COALESCE`, `MAX`, `LENGTH`, `CONCAT` / `||`, `CAST`, `${NOW}`,
  `:NAME` binds, nested parens
- `RETURNING col` / `${INSERT_KEY_RETURN}`

### SELECT (bootstrap, QueryRefs, Test 40+)

- `SELECT` list with aliases, `FROM` one or more tables
- `WHERE` (`=`, `<>`, `<`, `>`, `<=`, `>=`, `AND`, `OR`, `IN`, `IS NULL`,
  `LIKE` if present in QueryRefs — grep in Phase 8)
- `ORDER BY`, `LIMIT`
- `LEFT JOIN` / `INNER JOIN` / `JOIN` (in-memory after per-collection
  reads). `LEFT JOIN LATERAL` (e.g. 1168) needs an explicit interpreter
  path — do not silently drop LATERAL
- CTEs (`WITH`)
- Scalar subqueries if present (grep in Phase 8)

### Constraints the interpreter must enforce

Firestore will not. On INSERT/UPDATE:

- `PRIMARY KEY` → document id. Composite PK
  `(lookup_id, key_idx)` → id `"{lookup_id}_{key_idx}"` (stable, quoted
  fields joined by `_`). `query_id` serial → counters document
  `{prefix}_counters/queries` incremented on the Lead connection
- `UNIQUE` → reject duplicate with a SQL-like error
- `NOT NULL` → reject

### Document / collection mapping

| SQL | Firestore |
| --- | --- |
| Schema `testfb` + table `queries` | collection `testfb_queries` |
| Row | document |
| Column | field |
| `NULL` | omit field or `nullValue` (lock one; prefer `nullValue` so UPDATE can distinguish omit vs null) |
| Transaction | REST `beginTransaction` / `commit` (max 500 writes; split APPLY batches with the same SQL transaction semantics as DQM already wraps per migration) |

Bootstrap config for Engine `firebase` is still a **SQL** `SELECT`
against `testfb_queries` (same shape as Test 32). The interpreter runs
it. Do not invent a JSON bootstrap.

### 1 MiB document cap

Fail closed if a field would exceed **900 KiB** after function eval.
Brotli in Helium is for **payload** size; INSERT stores decompressed
`code`. Acuranzo query text is expected under that cap; if a row
exceeds it, that is a Status variance, not silent truncate.

---

## Goals And Non-Goals

### Goals

1. Fifth C engine `src/database/firebase/` implementing the full
   `DatabaseEngineInterface`.
2. Fifth Helium dialect `database_firebase.lua` with the complete macro
   key set above, in all four designs.
3. In-process Base64, Brotli, SHA-256, json_ingest, NOW, CONVERT_TZ,
   JSON extract — equivalent to extras UDFs + sqlean.
4. SQL interpreter covering the subset above so **Test 37 applies the
   full Acuranzo design** (same bar as Test 32).
5. QueryRefs execute (JOINs in-memory) so Test 40 and the rest of the
   7-engine matrix can use firebase without a second product.
6. Local CI via Firestore emulator. Injectable HTTP seam for Unity.
7. Retire Cockroach as a named engine. Keep Yugabyte and MariaDB.
8. Test **37** stays number 37 (renamed). Docs match code.

### Non-goals (this plan)

- Firebase Realtime Database, Firebase Auth, Cloud Functions as the
  UDF host, FCM, Firebase C++ SDK, gRPC.
- Cloud SQL / Data Connect (PostgreSQL aliases).
- A second Helium language (JSON ops) or a reduced table list.
- Replacing Yugabyte or MariaDB.
- Reusing `DB_ENGINE_AI`.
- Lithium SDK work beyond Lookup 030 rendering key 6.
- Rewriting historical `docs/H/metrics/` run names.

---

## Proposed design locks (Phase 0)

These are **approved** (2026-09-16) except as amended in this section
(lock 20).

1. **Product is Cloud Firestore (Native mode).** REST via libcurl
   (OIDC RP pattern: `CURLOPT_NOSIGNAL`, per-request easy handle).
2. **Helium emits SQL.** `database_firebase.lua` is a macro table, not
   a JSON DSL. Existing Acuranzo files run unchanged except the known
   `if engine` arms plus firebase no-op UDF DDL.
3. **No v1 table subset.** Test 37's done means is full AutoMigrations
   success, same as Test 32. Phases still land interpreter features in
   order.
4. **UDF-class functions are in-process in Hydrogen**, not extras `.so`
   loaded into Google. extras/firebase_emulator is install/start only.
5. **Function names** in generated SQL are the `FB_*` set in the macro
   table (amend here if a different spelling is preferred).
6. **SHA-256 hash bytes match** the other engines for
   `account_id || password`. Phase 4 Unity compares to a live SQLite or
   PostgreSQL fixture.
7. **JSON ingest matches the existing contract:** control-char fix-up
   inside strings; `$ref`/`$id`/`$schema` accepted (PG/SQLite/MySQL, not
   DB2 JSON2BSON); `${JSON}` columns stored as **stringValue** of
   ingested text (SQLite-compatible); extract via `FB_JSON_VALUE`
   (`${JRS}`/`${JRM}`/`${JRE}`), missing path → NULL. Not Firestore
   `mapValue` by default.
8. **`${SCHEMA}`** is `testfb_`-style prefix. Lock exact test prefix
   `testfb` (analogous to `testcrdb`).
9. **Connection fields** reuse `ConnectionConfig`:

   | JSON field | Meaning |
   | --- | --- |
   | `Engine` | `firebase` |
   | `Host` | emulator host or `firestore.googleapis.com` |
   | `Port` | `8080` emulator / `443` production |
   | `Database` | `(default)` |
   | `User` | GCP project id (`hydrodemo` in tests) |
   | `Pass` | service-account JSON **path**; **empty on emulator** |
   | `Schema` | collection prefix (`testfb`) |

   Connection string:
   `firebase://PROJECT/DATABASE?host=HOST&port=PORT&emulator=1`.
   `database_queue_determine_engine_type` recognizes `firebase://`
   before the SQLite fallback. Empty `Pass` + production host → fail
   closed.
10. **Enum:** `DB_ENGINE_FIREBASE` after DB2, before AI.
11. **Lookup 030 key 6 = Firebase**, dialect id 6.
12. **Yugabyte stays. MariaDB stays.** Only Cockroach is retired.
13. **Test 37 keeps number 37.**
14. **Test 31:** add `firebase` to `ENGINES`; **skip sqruff** (FB_* is
    not postgres SQL); unsubstituted `${…}` check still runs.
15. **Bootstrap stays SQL** `SELECT … FROM ${schema}queries …`.
16. **CI uses the emulator.** Unity uses an HTTP seam. Live GCP is
    Phase 16 optional.
17. **Never log** SA JSON, Google tokens, password hashes, JWT fields.
18. **1 MiB / 900 KiB fail closed. 500-write transaction cap** split
    with documented semantics (one Helium migration = one SQL
    transaction; if it needs >500 writes, the interpreter chunks
    inside that transaction or fails — lock in Status; default: fail
    closed and split the migration, do not silently auto-chunk).
19. **Meet completeness + coverage fences** before Phase 15 Status
    complete.
20. **`${SIZE_COLLECTION}` is `LENGTH(collection)`** on firebase, matching
    the other four engines. Do not treat it as a numeric `SIZE_*`
    constant. Other `SIZE_*` keys stay numeric strings for diagrams.

---

## Architecture

```text
Helium acuranzo_NNNN.lua
        |  database_firebase.lua macros
        v
   SQL (CREATE/INSERT/SELECT… with FB_* functions)
        |
        v
Hydrogen DQM  -->  firebase_execute_query
                      |
                      +-- fns_*: base64, brotli, sha256, json, now, tz
                      +-- sql_*: parse DDL/DML/SELECT, joins in memory
                      +-- http seam --> emulator :8080  or  production :443
                      |
                      v
                 QueryResult.data_json  (same row JSON as other engines)
```

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C `firebase/fns_*`** | Brotli/base64/sha256/json/tz | Collection names, Lithium |
| **C `firebase/sql_*`** | Acuranzo SQL subset, PK→doc id | Google OAuth |
| **C `firebase/http_*`** | REST, emulator vs prod, JWT | SQL |
| **Helium `database_firebase.lua`** | macro spellings | libcurl |
| **extras/firebase_emulator** | CLI / Java / ports | Hydrogen internals |
| **Test 37** | emulator lifecycle, full Acuranzo | Google console |

---

## Completeness fences

A phase is not done if the behavior works but Hydrogen's **normal
structures** were skipped. Phase 15 re-checks the whole table.

### Engine registration

| Must exist | Notes |
| --- | --- |
| `DB_ENGINE_FIREBASE` | After DB2, before AI |
| `firebase_get_interface()` | Same shape as `postgresql_get_interface` |
| Registry lazy-loads on `type == firebase` | |
| `normalize_engine_name("firebase")` | |
| `lua.c` engines[] includes `"firebase"` | Payload contains `database_firebase.lua` |
| `database_queue_determine_engine_type` | `firebase://` |
| `database_get_counts_by_type` | Extend or new helper; lock in Phase 3 |

### C tree (`src/database/firebase/`)

Suggested split (no file > 1000 lines, no `static` functions):

`types.h`, `interface.{c,h}`, `connection.{c,h}`, `utils.{c,h}`,
`http.{c,h}`, `query.{c,h}`, `transaction.{c,h}`, `prepared.{c,h}`,
`fns_base64.{c,h}`, `fns_brotli.{c,h}`, `fns_sha256.{c,h}`,
`fns_json.{c,h}`, `fns_tz.{c,h}`, `sql_parse.{c,h}`, `sql_ddl.{c,h}`,
`sql_dml.{c,h}`, `sql_select.{c,h}`, `sql_join.{c,h}`, `sql_expr.{c,h}`.

### Helium

| Must exist | Notes |
| --- | --- |
| `database_firebase.lua` | All four designs; **complete** key set |
| `database.lua` engines + dialects + defaults | |
| Lookup 030 key 6 packet | User applies |
| `if engine` arms include firebase | 1000, 1190, 1135, 1151, re-grep |
| `test_98` luacheck | |
| Test 31 firebase generation | unsubstituted `${}`; no sqruff |

### Config / secrets / extras

| Must exist | Notes |
| --- | --- |
| `hydrogen_config_schema.json` Engine enum | `firebase` |
| [SECRETS.md](/docs/H/SECRETS.md) | `FIREBASE_PROJECT`, emulator host/port, optional `FIREBASE_SA_JSON` path |
| `extras/firebase_emulator/` | README + start/stop |
| extras README extensions table | Firebase row |

### Tests / docs

Unity under `tests/unity/src/database/firebase/`. Test 37 firebase
migrations (full design). CHANGELOG + TEST_VERSION on scripts. `jq` for
JSON. Dead-code gate clean. Docs Phase 14: MACRO_REFERENCE firebase
column, DATABASES, GUIDE, PARAMETER_BINDING, TESTING, INSTRUCTIONS,
SECRETS, STRUCTURE, SITEMAP, SchemaTool, Helium/Acuranzo READMEs,
Lithium `DATABASE-MIGRATIONS.md`.

---

## Coverage fences

| Fence | Rule |
| --- | --- |
| Unity, file **< 100** instrumented lines | **> 50%** |
| Unity, file **≥ 100** instrumented lines | **> 75%** |
| Combined Unity **or** blackbox | **85%** project target; new `src/database/firebase/` at or above per-file Unity fence before Phase 15 |
| Test 99 | no new file **> 1000** lines |
| Seams | injectable HTTP, clock, FS for SA JSON — Unity never needs emulator/Google |
| `static` | `mkt` fails on new `static` in `src/` |
| Dead functions | new public symbols must have a caller |

Each C phase Exit includes named `mku`, `mkp`, and the per-file coverage
fence. Do not wait until Phase 15.

---

## Reference Conventions

- C engine: vtable like `postgresql/`; HTTP like `oidc_rp_http_*`.
- `cancel_inflight`: abort easy handle if in flight; else log unsupported.
- Health: emulator GET of the database resource, not `SELECT 1` (unless
  the interpreter already runs `SELECT 1` as SQL — either is fine if
  locked in Phase 3).
- Helium: luacheck header + CHANGELOG; agent never applies packets.
- Blackbox Test 37: `TEST_ABBR` **FBE** (was `CDB`); ports **537x**.
- Unity: `tests/unity/src/database/firebase/` mirrors `src/`.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/`: `mkt`
  then `mkp`. After Bash: `mks`. After Lua: `test_98`. After Markdown:
  `mkl`.

---

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Locks approved (SQL-on-Firestore, full Acuranzo, in-process UDFs, FB_* names, enum, dialect 6, emulator); no C | S | complete |
| 1 | `extras/firebase_emulator/` README + start/stop; Java/tools documented; extras README table row | S | complete |
| 2 | Complete `database_firebase.lua` in four designs; Test 31 generates firebase SQL; `if engine` arms include firebase; lookup 030 key 6 packet | M | complete |
| 3 | C engine registers, `firebase://`, connect + health vs emulator or seam | M | complete |
| 4 | In-process Base64 / Brotli / SHA-256 / JSON ingest+extract Unity-green; hash and JSON contracts match other engines | M | complete |
| 5 | CREATE/DROP TABLE, INDEX, ALTER, CREATE FUNCTION no-op against emulator/seam | L | complete |
| 6 | INSERT VALUES / UPDATE / DELETE with FB_* expression eval | L | complete |
| 7 | INSERT…SELECT, WITH, COALESCE(MAX)+1, RETURNING | L | complete |
| 8 | SELECT WHERE/ORDER/LIMIT, `:NAME` binds, PARAMETER_BINDING draft | M | pending |
| 9 | JOIN / LEFT JOIN / LATERAL in-memory; QueryRef-shaped fixtures green | L | pending |
| 10 | Test 37 firebase AutoMigrations **full Acuranzo** green on emulator | L | pending |
| 11 | Cockroach names gone; 7-engine loops say Firebase | M | pending |
| 12 | SchemaTool / SchemaHelper / hydrogen_flush / transaction_utils | M | pending |
| 13 | Tests 40/43/45/46/47/58 firebase configs; each named green or `[~]` with cause | L | pending |
| 14 | Docs/SITEMAP/MACRO_REFERENCE/DATABASES/SECRETS match; `mkl` green | S | pending |
| 15 | Completeness + coverage fences; dead-code clean; `mkp` | M | pending |
| 16 | Optional production SA JWT runbook **or** `[~]` parked | S | pending |

Effort key: S = small, M = moderate, L = large. Phase 16 must not block
plan complete if Status records the park.

---

## Phase 0 — Contract lock

### Goal

Approve or amend the locks above. No `src/` or Helium edits.

### Entry gate

This document exists. Ability to read the four `database_*.lua` files,
[`acuranzo_1000.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua),
[`sqlite/connection.c`](/elements/001-hydrogen/hydrogen/src/database/sqlite/connection.c)
extension loading, and extras UDF READMEs.

### Work items

- [x] 0.1 Confirm Firestore Native + REST + emulator; no C++ SDK; no
      Cloud Functions as UDFs.
- [x] 0.2 Confirm Helium still emits SQL; no JSON-ops dialect; no table
      subset.
- [x] 0.3 Confirm in-process `FB_*` function set, SHA-256 byte
      compatibility, and the JSON contract (fix-up, `$ref`, store as
      text, `FB_JSON_VALUE`).
- [x] 0.4 Confirm `DB_ENGINE_FIREBASE` placement and Lookup 030 key 6.
- [x] 0.5 Confirm connection field mapping, `firebase://`, prefix
      `testfb`.
- [x] 0.6 Confirm Yugabyte + MariaDB stay; Test 37 keeps number 37.
- [x] 0.7 Confirm Test 31 sqruff skip; bootstrap remains SQL.
- [x] 0.8 Confirm 900 KiB / 500-write fail-closed defaults (or amend).
- [x] 0.9 Confirm extras/firebase_emulator (not a UDF `.so`).
- [x] 0.10 Confirm completeness + coverage fences for Phase 15.
- [x] 0.11 Record amendments in this document if any lock changes.

### Done means

Phase 0 Status lists every lock as approved or amended; no C/Lua/tests
changed in this phase.

### Exit gate

- Phase 0 Status = complete; user approval in Working Log.
- Next free Acuranzo migration / QueryRef re-checked on disk.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-16 |
| **Result** | Locks 1–19 approved as written. Lock 20 added: `${SIZE_COLLECTION}` is `LENGTH(collection)`. Lock 18 default kept: fail closed at 900 KiB and at 500 writes/txn; split the Helium migration rather than silently auto-chunk. No C/Lua/tests changed. |
| **Variances** | Lock 20 only. |

### Working Log

- **2026-09-16** Session opened at CURRENT PAUSE POINT (Phase 0). Entry
  gate re-read: four `database_*.lua` files, `acuranzo_1000.lua`,
  `sqlite/connection.c` extension load (`crypto.so` /
  `brotli_decompress.so` / `convert_tz.so` from `/usr/local/lib`),
  extras UDF README table.
- Disk re-check: last Acuranzo file is still
  `acuranzo_1382.lua` (383 numbered files). Next free id **1383**.
  Lookup 030 in `acuranzo_1055.lua` still ends at key **5** (MS SQL
  Server). `query_dialects` in `database.lua` is 1–4 only. C enum is
  still PG/SQLite/MySQL/DB2/`DB_ENGINE_AI`. `if engine` files remain
  1000, 1135, 1151, 1190.
- Union of dialect macro keys: **73**. DB2-only
  `BASE64ENCODE*` / `DATETIME_FORMAT` / `TIMESTAMP_FORMAT` and
  sqlite-only `CONVERT_TZ_FUNCTION` must still exist on firebase so
  `replace_query` never leaves `${UNSUBSTITUTED}`.
- User approved locks 1–19 as written plus the `SIZE_COLLECTION`
  amendment (lock 20). Explicit: do not start Phase 1 until asked.

### Lessons learned

- `${SIZE_*}` is not a uniform family. Six keys are numeric diagram
  constants; `${SIZE_COLLECTION}` is `LENGTH(collection)` on every
  current engine and is used in diagram migrations (1116, 1121–1126,
  1136, 1138, …). Treating it as `4`/`8`/`20` would have been a silent
  Test 31 unsubstituted-or-wrong-SQL bug.
- SQLite `convert_tz.so` README claims IANA `/usr/share/zoneinfo` then
  documents a UTC→America/Vancouver 8-hour offset. Phase 4 must not
  copy that stub; `FB_CONVERT_TZ` stays IANA via OS zoneinfo (or a
  named library).
- Inserting `DB_ENGINE_FIREBASE` after DB2 **shifts** `DB_ENGINE_AI`'s
  numeric value. Phase 3 must grep for a hardcoded `4` meaning AI.
- `CONVERT_TZ` is not called from Acuranzo QueryRefs today (only the
  sqlite UDF declaration in 1000). `LIKE` **is** present (1131, 1122,
  1124, …) — Phase 8 grep is not optional.
- `acuranzo_1000.lua` skips JSON ingest creation for sqlite and emits
  DB2/MySQL-only UDF DDL. Firebase must join the skip set (`engine ~=
  'sqlite' and engine ~= 'firebase'`) so 1000 does not `CREATE FUNCTION`
  against Firestore.

---

## Phase 1 — Emulator extras (install)

### Goal

A developer (or Test 37 later) can install tools and start the Firestore
emulator from extras, with the same kind of README the Brotli/SQLite
crypto extras have.

### Entry gate

Phase 0 Status complete.

### Work items

- [x] 1.1 Add `extras/firebase_emulator/README.md`: Node, Java,
      `firebase-tools`, ports, project id `hydrodemo`, no Google
      account required for emulator.
- [x] 1.2 `start.sh` / `stop.sh` / `firebase.json`. Idempotent start;
      wait for port; stop only if started.
- [x] 1.3 extras README Database Extensions table: Firebase row
      ("in-process functions in Hydrogen; emulator here").
- [x] 1.4 SECRETS.md draft names (values never committed).

### Done means

`extras/firebase_emulator/start.sh` brings up :8080 on a machine with
the documented deps; README lists the UDF situation honestly.

### Exit gate

- `mks` on new scripts.
- Manual start/stop recorded in Status (or documented skip if Java
  missing, with install steps).
- `mkl` if extras README gained links.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-16 |
| **Result** | `start.sh` brought up Firestore on `127.0.0.1:8080` (REST 200 `{}` for `hydrodemo`). Idempotent start, stop-only-if-started, restart all recorded. `mks` 172 files PASS. `mkl` 2527/2527. |
| **Variances** | Extra files beyond the four-file sketch: `.firebaserc`, `firestore.rules` (emulator-open), local `package.json` pin `firebase-tools@15.30.1` + lockfile. Emulator UI disabled (`:4000` already occupied on this box). No `npm i -g`. |

### Working Log

- **2026-09-16** Phase 0 Status re-read complete. User approved Phase 1
  implementation (local pin, UI off, SECRETS names as proposed).
- Layout:
  `extras/firebase_emulator/README.md`
  plus `start.sh`, `stop.sh`, `firebase.json`, `.firebaserc`,
  `firestore.rules`, `package.json` / `package-lock.json`.
- Manual start/stop (this machine): Node v24, OpenJDK 25 on PATH,
  Java 21 also installed. `npm install` in the extra (670 packages,
  ~15s). First `start.sh`: pid 1574504, REST
  `GET /v1/projects/hydrodemo/databases/(default)/documents/` → 200
  `{}`. Second `start.sh` no-op. `stop.sh` killed the tree; `:8080`
  closed. `stop.sh` with no pid file left the port alone. Restart pid
  1575168 then stop. Emulator left down.
- `mks` (Test 92) green: 172 scripts, 0 issues, 1116 directives
  justified. `mkl` (Test 04) green. markdownlint on changed extras /
  SECRETS / SITEMAP files clean.
- SECRETS.md section 8: `FIREBASE_PROJECT`,
  `FIREBASE_EMULATOR_HOST`, `FIREBASE_EMULATOR_PORT`,
  `FIREBASE_SA_JSON` (empty on emulator). No SA JSON committed.

### Lessons learned

- File-level `# shellcheck disable=SC2310` must sit **before** any
  command (including `set -euo pipefail`) or Test 92 still flags
  predicate helpers used in `if`/`||`.
- `nohup firebase emulators:start` + recursive `pgrep -P` SIGTERM is
  enough; the Java child dies with the CLI parent. `setsid` inside a
  subshell would have saved the wrong pid.
- Empty emulator REST is 200 `{}`, not 404 — a 2xx/4xx/5xx HTTP code
  is a sufficient ready check.
- This box already had something on `:4000`; disabling the emulator UI
  is the difference between a clean `:8080` extra and a port fight.
  Keep UI off unless a later phase needs it.

---

## Phase 2 — Helium dialect (complete macros)

### Goal

`require("database_firebase")` supplies every macro key. Test 31
generates firebase SQL for Acuranzo and Helium designs without
unsubstituted `${…}`. Per-engine Lua arms know firebase.

### Entry gate

Phase 1 Status complete.

### Work items

- [x] 2.1 Write `database_firebase.lua` (four designs) with the complete
      key set. **Verify:** Lua snippet prints every key the other four
      files have; set difference is empty.
- [x] 2.2 `database.lua`: `engines.firebase`, `query_dialects.firebase
      = 6`, `defaults.firebase`.
- [x] 2.3 Test 31: `ENGINES` includes firebase; skip sqruff; keep
      unsubstituted-macro check. **Verify:** Test 31 green.
- [x] 2.4 acuranzo_1000: skip CREATE FUNCTION for firebase like sqlite
      where appropriate; `${BROTLI_DECOMPRESS_FUNCTION}` is a comment.
- [x] 2.5 firebase arms (or shared-with-sqlite) for 1190, 1135, 1151;
      re-grep `if engine`.
- [x] 2.6 Lookup 030 key 6 packet (not applied). `test_98`.

### Done means

Test 31 generates firebase SQL for every Acuranzo migration without
`${UNSUBSTITUTED}`; luacheck clean; lookup packet handed to the user.

### Exit gate

- Test 31 run; `mks` if the script changed; `test_98`.
- Key-set diff attached to Status.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-16 |
| **Result** | Test 31 1925/1925 PASS (firebase included, sqruff skipped). `test_98` 460 files PASS. `mks` 172 files PASS. Key-set diff empty (73/73). Packet `acuranzo_1383.lua` prepared, not applied. |
| **Variances** | `database.lua` also uses underscore schema prefix for firebase (lock 8; required for generation). Lookup 030 icon uses `sql_dialect_firebase.png` copied into Lithium `public/assets/images/` (TNT source was named `sqli_dialect_firebase.png`). 1135 uses `json_object` + `FB_JSON_VALUE` (interpreter builtin, not a new lock). |

### Working Log

- **2026-09-16** Phase 1 Status re-read complete. User approved Phase 2
  with (1) 1135 `json_object` + `FB_JSON_VALUE` and (2) dialect PNG from
  `/fvl/tnt/t-philement/lithium/assets/images/` (file on disk was
  `sqli_dialect_firebase.png`; copied as
  `sql_dialect_firebase.png` to match the other dialect icons).
- `database_firebase.lua` written with the union of 73 keys and copied
  to Acuranzo, Gaius, GLM, Helium. Gaius/GLM/Helium sibling dialects
  still lag Acuranzo; firebase files are complete anyway.
- `database.lua` (four designs): `engines.firebase`,
  `query_dialects.firebase = 6` (key 5 unused MS SQL),
  `defaults.firebase`, schema prefix `name_` not `name.`.
- `if engine` re-grep: 1000 / gaius_2000 / glm_3000 / helium_4000 skip
  JSON_INGEST CREATE FUNCTION for firebase; 1000 emits CONVERT_TZ
  comment for firebase; 1190 shares sqlite rebuild; 1151
  `engine ~= 'mysql'` already covers firebase; 1135 dedicated firebase
  arm.
- Test 31: `ENGINES` + fifth schema column (`testfb` / `helium`).
  Explicit firebase sqruff skip after unsubstituted-macro check.
  1925 combos, 0 failed, 1496 cached.
- Lookup packet:
  [`acuranzo_1383.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1383.lua)
  additive key 6 only. User applies; agent did not.
- Lithium:
  [`public/assets/images/sql_dialect_firebase.png`](/elements/003-lithium/public/assets/images/sql_dialect_firebase.png)
  and `config/icons-usr.txt` row. No Test 37, no Cockroach deletes.
- **2026-09-17** AutoMigrations on existing engines failed:
  `module 'database_firebase' not found`. `database.lua` requires the
  dialect at load time; Hydrogen's payload Lua loader only preloaded
  sqlite/postgresql/mysql/db2. Hotfix: add `"firebase"` to
  `lua.c` `engines[]` (Phase 3 item 3.3 loader slice). Payload already
  copies `database_*.lua`. Rebuild Hydrogen so the new C + payload
  ship. Unity `lua_test_load_database_module` fixtures updated.

### Lessons learned

- `${SCHEMA}` for firebase cannot be implemented in the dialect file
  alone. `replace_query` always did `name.`; firebase needs `name_`.
  That branch belongs in all four `database.lua` files.
- Gaius/GLM/Helium `database_*.lua` files are not key-complete vs
  Acuranzo. Copying the **complete** firebase table into those designs
  is still correct: extra keys are unused, missing keys would fail
  `replace_query`.
- 1151 needed no new arm. `if engine ~= 'mysql'` is already the
  shared-with-sqlite path.
- QueryRef 044 (1135) hardcodes JSON constructors per engine. Sharing
  sqlite would emit `collection ->>`, not `FB_JSON_VALUE`. Dedicated
  arm; `json_object(...)` is an interpreter builtin for a later SELECT
  phase (not a Helium macro).
- TNT asset was `sqli_dialect_firebase.png` (extra `i`). Rename on copy
  so Lookup 030 matches `sql_dialect_postgres.png` and friends.
- Adding `require("database_firebase")` to Helium `database.lua` is
  not enough for AutoMigrations. Hydrogen preloads dialects via
  `lua.c` `engines[]` into `package.loaded` before executing
  `database.lua`. Test 31 uses on-disk `require` and hid this until
  a live DQM LOAD.

---

## Phase 3 — C engine skeleton

### Goal

Register, connstring, connect, health. No SQL yet.

### Entry gate

Phase 2 Status complete.

### Work items

- [x] 3.1 `DB_ENGINE_FIREBASE` after DB2, before AI. `mkt`.
- [x] 3.2 `interface`, `utils` (connstring/validate/mask), `connection`,
      `http` seam. Unity.
- [x] 3.3 Registry, `normalize_engine_name`, lazy init.
      `database_get_counts_by_type` decision implemented.
      (`lua.c` `engines[]` already includes `"firebase"` — Phase 2
      AutoMigrations hotfix 2026-09-17.)
- [x] 3.4 Optional live emulator health (Phase 1 extras).

### Done means

Unity connects via the seam and reports healthy; `mkt` + `mkp`; no new
`static`.

### Exit gate

- `mkt` then `mkp`; named `mku`; coverage fence.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-16 |
| **Result** | `mkt` green; `mkp` 2,062 files PASS; Unity connect+health via seam. Live emulator GET `documents/` HTTP 200 `{}`. |
| **Variances** | Phase 3 connect is emulator-only (production host refused; SA JWT is Phase 16). `launch_database.c` readiness counts still omit firebase (Phase 15 / Test 37 config). Query/transaction/prepared are fail-closed stubs. |

### Working Log

- **2026-09-16** Phase 2 Status re-read complete. User confirmed
  `acuranzo_1383.lua` (Lookup 030 key 6) is applied. Explicit go to
  implement Phase 3.
- Enum: `DB_ENGINE_FIREBASE` after DB2, before AI. Grep of hardcoded
  `4` meaning AI was empty; Unity mock tests use the enum name.
- `database_get_counts_by_type` extended with a fifth
  `firebase_count` out-param (launch.c + Unity). Registry lazy-loads
  firebase when a configured connection has `type == "firebase"`.
- `database_queue_determine_engine_type` recognizes `firebase://`
  **before** the SQLite fallback. Connstring:
  `firebase://PROJECT/DATABASE?host=&port=&emulator=1` (no SA path).
- Health is HTTP GET
  `/v1/projects/{project}/databases/{database}/documents/` (2xx).
  Injectable seam `firebase_http_test_set_response` (OIDC RP
  pattern: `CURLOPT_NOSIGNAL`, per-request easy handle).
- Empty Pass + `firestore.googleapis.com` fails closed. Phase 3
  does not implement production JWT.
- Unity (seam): `interface_test_firebase`, `utils_test_firebase`,
  `http_test_firebase`, `connection_test_firebase`, plus stub
  query/transaction/prepared/firebase tests. Coverage fence:
  connection 86%, http 91%, utils 86%, others 84–100%.
- 3.4: extras `start.sh` → REST 200 `{}` for hydrodemo; `stop.sh`.
- `hydrogen_config_schema.json` Engine enum gained `"firebase"`.
  No Test 37, no Cockroach deletes.

### Lessons learned

- Inserting `DB_ENGINE_FIREBASE` before AI does **not** break Unity
  mock tests that use `DB_ENGINE_AI` by name; a numeric `4` would
  have. Keep using the enum.
- `database_get_counts_by_type`'s four-out-param shape had only
  three callers. A fifth required pointer was cheaper than a new
  helper plus dual paths.
- The HTTP seam must be Firebase-owned, not a reuse of
  `oidc_rp_http_*`, so later POST/PATCH/DELETE and emulator http
  (no TLS) do not collide with OIDC fixtures.
- cppcheck `constParameterPointer` on vtable stubs is a signature
  lock, not a real const opportunity — suppress on the function
  line the same way SQLite `execute_prepared` does.
- `launch_database.c` duplicates engine counting instead of calling
  `database_get_counts_by_type`. A firebase-only Hydrogen config
  would still look like "no databases" at launch readiness until
  that file grows a fifth count (Phase 15 / Test 37).

---

## Phase 4 — In-process UDF-class functions

### Goal

`FB_BASE64_DECODE/ENCODE`, `FB_BROTLI_DECOMPRESS`, `FB_SHA256_B64`,
`FB_JSON_INGEST` (including `$ref`), `FB_NOW`, `FB_CONVERT_TZ`,
`FB_JSON_VALUE` are Unity-tested and do not need Firestore.

### Entry gate

Phase 3 Status complete.

### Work items

- [x] 4.1 Base64 round-trip Unity.
- [x] 4.2 Brotli: compress with the same library Lua uses (quality 11)
      in a fixture, decompress in C, original bytes match. Mirror extras
      UDF tests. Include a real wrapped
      `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('…'))` expression.
- [x] 4.3 SHA-256: fixture `account_id=0` + known password equals a
      hash taken from SQLite or PostgreSQL (record the fixture in the
      test). **This is a login compatibility gate.**
- [x] 4.4 JSON ingest (the expensive contract):
      already-valid JSON passthrough; `\n`/`\t`/`\r` inside a JSON
      string (the plpgsql/MySQL/DB2 fix-up); JSON Schema document with
      `$ref`/`$id`/`$schema` stored intact; garbage input still errors.
      **Verify:** Unity round-trip equals SQLite-style "store what ingest
      returned," not PG jsonb canonicalization.
- [x] 4.5 `FB_JSON_VALUE` / `${JRS}` path extract: `$.icon` on a lookup
      `collection` fixture (QueryRef 1114 shape); missing path → NULL.
- [x] 4.6 NOW / CONVERT_TZ Unity with injectable clock.

### Done means

Named `mku` for each `fns_*`; SHA-256 fixture matches another engine;
JSON ingest/`$ref`/extract fixtures green; `mkp` green.

### Exit gate

- `mkq`/`mkt` + `mkp` + named `mku` + coverage fence.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-16 |
| **Result** | `mkt`/`mkq` green; `mkp` 2,080 files PASS; named `mku` for each `fns_*` green. SHA-256 `0`+`testpass` = `CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=` matches SQLite `crypto_sha256`. Coverage: base64 100%, sha256 93%, brotli 67% (<100 lines), json 82%, tz 83%. |
| **Variances** | `FB_TIME_ADD` / `FB_SESSION_SECS` deferred to Phase 6/8 (not in this phase's work items). Nested wrapper evaluated in Unity by extract-and-compose; no `sql_expr.c`. `FB_NOW` is UTC `YYYY-MM-DD HH:MM:SS`. CONVERT_TZ is mutex + `tzset` + OS zoneinfo. |

### Working Log

- **2026-09-16** Phase 3 Status re-read complete. User said go with
  recommended contracts (IANA CONVERT_TZ, SQL-style NOW, dummy SHA-256
  fixture, no mini SQL evaluator, TIME_ADD deferred).
- Files: `fns_base64`, `fns_brotli`, `fns_sha256`, `fns_json`,
  `fns_tz` under `src/database/firebase/`. C names
  `firebase_base64_decode` etc. (SQL `FB_*` mapping is Phase 6).
- 4.1: wrap `utils_base64_*` (RFC 4648 `+/` with padding). Round-trip
  including `+/8A`.
- 4.2: `libbrotlidec`. lua-brotli quality 11 fixtures
  (`iwWASGVsbG8gV29ybGQhAw==` → `Hello World!`; payload
  `Phase4 firebase brotli quality 11 fixture`) plus extras UDF
  `jwWASGVsbG8gV29ybGQhAw==`. Wrapped
  `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('…'))` in Unity. Fail closed
  at `FIREBASE_MAX_FIELD_BYTES` (900 KiB).
- 4.3: `firebase_sha256_b64(a, b)` concatenates UTF-8 strings, SHA-256,
  standard base64. Fixture recorded in the test (SQLite + openssl).
- 4.4: jansson fast-path returns original text; fix-up only `\n`/`\t`/`\r`
  inside double-quoted strings; `$ref`/`$id`/`$schema` passthrough;
  garbage → NULL.
- 4.5: `$.icon`, `$.support prompts`, nested `$.a.b`, array `[n]`;
  missing / JSON null → NULL.
- 4.6: injectable `firebase_now_test_set_unix`. CONVERT_TZ DST-aware
  (UTC→America/Vancouver winter `04:00:00`, summer `05:00:00`). Unknown
  zone / path traversal → NULL. Same zone passthrough.
- Keep-alive: `firebase_engine_test_functions` calls every `FB_*` with
  NULL (and NOW) so `--gc-sections` does not drop them before Phase 6
  `sql_expr`. `firebase_get_interface` references that function via an
  always-false `connect == NULL` branch.
- No Test 37, no Cockroach deletes, no Firestore in this phase.

### Lessons learned

- `BrotliDecoderDecompressStream` on junk returns
  `NEEDS_MORE_INPUT`, not `ERROR`. Treating only `ERROR`/`SUCCESS`
  loops forever. Treat any non-SUCCESS except `NEEDS_MORE_OUTPUT` as
  failure.
- lua-brotli quality 11 and the extras UDF short fixture are **not**
  the same bytes for `Hello World!` (`iwWA…` vs `jwWA…`); both
  decompress. Lock quality 11 on the Lua compressor; the engine only
  decompresses.
- SHA-256 must take two **strings** (`'0'`, password), not an int.
  `utils_password_hash` is the same concat for decimal ids but is
  mocked in some Unity configs; OpenSSL `SHA256` + `utils_base64_encode`
  is the stable path.
- JSON ingest must not `json_dumps` the parse tree (that is PG jsonb
  canonicalization). Valid input returns `strdup` of the original.
- CONVERT_TZ on this box is DST-aware via `/usr/share/zoneinfo`. The
  SQLite extra's 8-hour Vancouver stub would have failed the July
  fixture (`05:00:00` vs `04:00:00`).
- `TZ` + `tzset` + `mktime` must run under a mutex; Hydrogen is
  multithreaded. QueryRefs do not call `CONVERT_TZ` today.

---

## Phase 5 — SQL DDL

### Goal

`CREATE TABLE` / `DROP TABLE` / `CREATE INDEX` / `ALTER` / no-op
`CREATE FUNCTION` persist as collections + `_schema` metadata.

### Entry gate

Phase 4 Status complete.

### Work items

- [x] 5.1 Parser for CREATE TABLE (columns, NOT NULL, PK, UNIQUE).
- [x] 5.2 DROP TABLE + DROP_CHECK.
- [x] 5.3 CREATE INDEX recorded; emulator smoke.
- [x] 5.4 ALTER ADD/DROP/RENAME; 1190-class rebuild path.
- [x] 5.5 CREATE FUNCTION no-op for known `FB_*` names.

### Done means

Unity (seam) creates `testfb_queries` metadata, drops it, rejects drop
when a row exists if DROP_CHECK is in the statement.

### Exit gate

- `mkp` + named `mku` + coverage fence.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-17 |
| **Result** | `mkt`/`mkq` green; `mkp` 2,093 files PASS; named `mku` for parse/DDL/query/http green. Coverage: sql_parse 78%, sql_ddl 76%, query 75% of 59 lines, http 91%. Emulator smoke: PATCH/GET/DELETE `_schema/testfb_queries` HTTP 200/200/200 then GET 404. |
| **Variances** | Transactions and prepared statements stay fail-closed (`prepare` fails and `database_engine_execute` already falls back to `execute_query`). 1190 `INSERT … SELECT *` stays Phase 7. DROP_CHECK is a special-case `SELECT FB_REFUSE_DROP` (not a general SELECT interpreter). |

### Working Log

- **2026-09-17** Phase 4 Status re-read complete. User said go as-is
  (catalog `_schema` with doc ids = collection names; unqualified
  `RENAME TO` gets the connection schema prefix; transactions
  fail-closed; HTTP PATCH/DELETE on the existing seam). Scope locked
  to `src/database/firebase/` plus Unity tests — no other-engine
  edits.
- Files: `sql_parse.{c,h}`, `sql_ddl.{c,h}`; `query.c` dispatches DDL;
  `http.c` gained method-aware fixtures plus PATCH/DELETE.
- 5.1: Acuranzo-shaped CREATE TABLE (types integer/bigint/real/text/
  json/timestamp, NOT NULL, DEFAULT, table-level PK/UNIQUE). 1000
  queries-shaped fixture and 1001 composite PK parse green.
- 5.2: DROP TABLE deletes data docs then the catalog doc. DROP_CHECK
  `SELECT FB_REFUSE_DROP('testfb_lookups') WHERE EXISTS (SELECT 1
  FROM testfb_lookups)` fails with `DB_ERR_OTHER` when the collection
  has documents; empty collection succeeds.
- 5.3: CREATE INDEX / CREATE UNIQUE INDEX append to catalog
  `indexes`. Live emulator: PATCH/GET `_schema/testfb_queries` 200,
  index PATCH 200, DELETE 200, GET 404. Emulator left down.
- 5.4: ALTER ADD/DROP COLUMN update catalog metadata. RENAME TO
  copies catalog (and data docs if any) and prefixes unqualified
  names (`accounts` → `testfb_accounts`). 1190 CREATE+DROP+RENAME
  pieces work; INSERT SELECT is Phase 7.
- 5.5: CREATE/DROP FUNCTION no-op for known `FB_*` names; unknown
  CREATE FUNCTION errors; `DROP FUNCTION IF EXISTS` unknown succeeds.
- No Test 37, no Cockroach deletes, no `sql_expr.c` / DML.

### Lessons learned

- APPLY always sets `use_prepared_statement=true`, but a failing
  `prepare_statement` already falls back to `execute_query`. Leaving
  prepared fail-closed does not block later APPLY once transactions
  exist.
- `_schema` as a Firestore collection id is legal (not `__.*__`).
  Emulator PATCH/GET/DELETE of `_schema/testfb_queries` worked on
  first try.
- 1190 `RENAME TO ${TABLE}` is unqualified. The interpreter must
  apply `schema_` itself or the catalog doc lands at `accounts`
  instead of `testfb_accounts`.
- DROP_CHECK is a separate APPLY statement. A successful SELECT that
  returns a row would not stop APPLY; `FB_REFUSE_DROP` must fail the
  statement (`success=false`, `DB_ERR_OTHER`).
- Method-aware HTTP fixtures are required: GET and PATCH of the same
  `_schema/…` URL would otherwise steal each other's queue slots.

---

## Phase 6 — SQL DML (VALUES / UPDATE / DELETE)

### Goal

INSERT VALUES, UPDATE, DELETE, with `FB_*` expressions evaluated into
fields.

### Entry gate

Phase 5 Status complete.

### Work items

- [x] 6.1 INSERT VALUES multi-row; PK → document id; UNIQUE/NOT NULL.
- [x] 6.2 Expression eval hooks Phase 4 functions (COMPRESS, SHA256,
      JSON_INGEST, NOW).
- [x] 6.3 UPDATE / DELETE WHERE on simple predicates.
- [x] 6.4 900 KiB fail closed.

### Done means

Unity inserts a lookups-shaped row (`030_6`) and a queries-shaped row
whose `code` was supplied as `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE(…))`
and stores plaintext.

### Exit gate

- `mkp` + named `mku` + coverage fence.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-17 |
| **Result** | `mkt`/`mkq` green; `mkp` 2,104 files PASS; named `mku` for expr/parse/DML/query/http green. Coverage: sql_expr 80%, sql_dml 79%, sql_parse 80%, query 75% of 61 lines, http 91%. Lookups INSERT `030`/`6` → document id `30_6`; queries `code` stores Brotli plaintext. |
| **Variances** | Transactions and prepared statements stay fail-closed (multi-row INSERT is sequential PATCH, not atomic). `FB_TIME_ADD` / `FB_SESSION_SECS` still Phase 8. `INSERT … SELECT` / `WITH` stay UNSUPPORTED (Phase 7). HTTP fixture queue cap raised 8 → 32. DML parse lives in `sql_expr.c` so `sql_dml.c` stays under 1000 lines. |

### Working Log

- **2026-09-17** Phase 5 Status re-read complete. User said go as-is
  (document id decimal no pad `30_6`; WHERE `IN (literals)`; timestamp
  columns as RFC3339 `timestampValue`; transactions fail-closed;
  `FB_TIME_ADD` deferred). Scope stayed `src/database/firebase/` plus
  Unity — no Test 37, no Cockroach deletes.
- Files: `sql_expr.{c,h}`, `sql_dml.{c,h}`; `sql_parse` gained INSERT /
  UPDATE / DELETE kinds; `query.c` dispatches DML vs DDL.
- 6.1: INSERT VALUES multi-row. Composite PK joined by `_` from
  evaluated integers (`030` → `30`). Duplicate PK GET-then-reject.
  UNIQUE scans the collection. NOT NULL and omitted columns (catalog
  DEFAULT or null) enforced.
- 6.2: Expression eval of literals, `NULL`, nested parens, and Phase 4
  `FB_*` (binary-safe `FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE(…))`).
  SHA-256 fixture matches Phase 4. `FB_JSON_INGEST` and `FB_NOW()`
  write JSON text and RFC3339 timestamps.
- 6.3: UPDATE GET-merge-PATCH (REST PATCH without `updateMask` would
  wipe fields). DELETE lists then deletes. WHERE: `=`, `AND`, `IS NULL`,
  `IN (literals)`. No WHERE → fail closed.
- 6.4: Field size after eval fail-closed at `FIREBASE_MAX_FIELD_BYTES`.
- HTTP seam queue cap 32 for multi-row GET+PATCH fixtures.
- No Test 37, no Cockroach deletes, no SELECT interpreter.

### Lessons learned

- SQL integer `030` is decimal 30 (`strtoll` base 10, never base 0).
  Document ids stringify the evaluated integer, so Lookup 030 key 6 is
  `30_6`, not `030_6`.
- Firestore REST PATCH without `updateMask` replaces the whole document.
  UPDATE must GET, merge SET into existing `fields`, then PATCH.
- Collection list URLs are prefixes of document URLs. Enqueue the list
  GET before per-document GET/PATCH, or a `documents/testfb_lookups`
  fixture steals `…/testfb_lookups/30_6`.
- `sql_parse.c` was already ~800 lines; DML parse had to live elsewhere
  (`sql_expr.c`) to stay under the 1000-line cap. `sql_dml.c` is execute
  only.
- Unity `TEST_ASSERT_*` plus `free(result->error_message)` on a reused
  `QueryResult*` is a cppcheck `doubleFree` false positive. A small
  `release_query_result` helper clears it.

---

## Phase 7 — INSERT…SELECT / CTE / MAX+1 / RETURNING

### Goal

The house seed pattern `WITH next_query_id AS (SELECT COALESCE(MAX…)+1)
SELECT … INSERT` works. This is most of AutoMigrations LOAD.

### Entry gate

Phase 6 Status complete.

### Work items

- [x] 7.1 WITH CTEs feeding INSERT…SELECT.
- [x] 7.2 Counters / MAX+1 on Lead; RETURNING.
- [x] 7.3 `${INSERT_KEY_*}` shape.

### Done means

Two successive LOAD-shaped inserts get `query_id` 1 then 2; RETURNING
row present in `QueryResult`.

### Exit gate

- `mkp` + named `mku`.

### Status

| | |
| --- | --- |
| **State** | complete |
| **Date** | 2026-09-17 |
| **Result** | `mkt`/`mkq` green; `mkp` 2,111 files PASS; named `mku` for select/expr/parse/DML green. Coverage: sql_select 80% of 576, sql_expr 77%, sql_parse 81%, sql_dml 79%, query 74% of 61 (under the 100-line fence). Two LOAD inserts → `query_id` 1 then 2; RETURNING `[{"query_id":1}]`. |
| **Variances** | No `{prefix}_counters` sidecar — MAX+1 lists the collection (SQL as written). Transactions stay fail-closed. `:NAME` binds and standalone SELECT stay Phase 8. |

### Working Log

- **2026-09-17** Phase 6 Status re-read complete. User said go with
  locked design: restricted INSERT-source SELECT (WITH CTE MAX+1 and
  `SELECT *`); no counters document; RETURNING as a JSON array of row
  objects; `CONCAT`/`CAST`/`||` deferred; scope `src/database/firebase/`
  plus Unity.
- Files: `sql_select.{c,h}`; `sql_expr` gained `+` / `COALESCE`;
  `sql_parse` INSERT gained WITH/SELECT/RETURNING; `sql_dml_insert`
  dispatches SELECT.
- 7.1: `INSERT INTO t (cols) WITH cte AS (SELECT COALESCE(MAX(col),0)+1
  AS alias FROM t) SELECT … FROM cte`. 1190 `INSERT INTO t_new SELECT *
  FROM t` copies documents by id.
- 7.2: Empty collection MAX is NULL → COALESCE 0 → +1 → `query_id` 1;
  a second LOAD-shaped insert with existing max 1 → `query_id` 2.
  RETURNING populates `QueryResult.data_json` / `row_count` /
  `column_names`.
- 7.3: Leading `-- col` (`INSERT_KEY_START`) is skipped by
  `firebase_sql_skip`; trailing `RETURNING col` is parsed.
- No Test 37, no Cockroach deletes, no standalone SELECT interpreter.

### Lessons learned

- Generic `firebase_expr_eval_call` cannot evaluate `MAX(col)`: the
  argument is a column ident, not a row value. CTE eval walks
  `COALESCE`/`MAX`/`+` itself and scans the listed collection.
- Collection list URLs remain prefixes of document URLs. Enqueue the
  MAX list GET before the existence GET/PATCH of the new id.
- `sql_parse.c` / `sql_expr.c` / `sql_dml.c` were already near the
  1000-line cap, so INSERT…SELECT parse and execute live in
  `sql_select.c`.
- Helium `[=[…]=]` never reaches C; LOAD `code` is already
  `FB_BASE64_DECODE` / `FB_BROTLI_DECOMPRESS`.

---

## Phase 8 — SELECT and binds

### Goal

Bootstrap-shaped SELECT and Conduit `:NAME` binds work on one
collection.

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 SELECT list, WHERE, ORDER BY, LIMIT, aliases (implicit
      `col alias` and `AS`, same bootstrap SQL the other engines run).
- [ ] 8.2 `parse_typed_parameters` → `convert_named_to_positional`
      (`?`, already the Firebase case) → bind by 1-based index in
      `firebase_execute_query`, same as `sqlite_execute_query`. Do
      **not** splice values into the SQL text.
- [ ] 8.3 Grep QueryRefs for `LIKE`, `IN`, subqueries; add or `[~]`
      with the QueryRef id. (Grep done in the pre-phase discussion;
      record the `[~]` list in Status when the phase runs.)
- [ ] 8.4 Draft PARAMETER_BINDING firebase row (`?` placeholder,
      interpreter binds by index; same JSON types).

### Done means

Bootstrap SQL from Test 32's shape returns query rows from firebase
collections in `QueryResult.data_json`.

### Exit gate

- `mkp` + named `mku` + draft PARAMETER_BINDING.

### Status

| | |
| --- | --- |
| **State** | pending |
| **Date** | |
| **Result** | |
| **Variances** | |

### Working Log

- **2026-09-17 (pre-implementation, waiting for go).** Phase 7 Status
  re-read complete. User asked for a top-of-file remaining-effort
  table (Quick / Moderate / Difficult) and a standing parity rule:
  when a Firebase choice is ambiguous, do what PostgreSQL / SQLite /
  MySQL / DB2 already do; do not change those engines.
- Bind path (replaces an earlier splice-literals idea): copy
  `sqlite_execute_query`. `parameters_json` → `parse_typed_parameters`
  → `convert_named_to_positional(..., DB_ENGINE_FIREBASE)` which
  already emits `?` → evaluate `?` against the ordered
  `TypedParameter` list. Templates keep `:NAME` on the wire from
  Helium/Conduit, same as every other engine.
- `prepare_statement` stays fail-closed (Phase 6). APPLY already
  falls back to `execute_query` in `database_engine_execute`. Direct
  path still binds, as SQLite's `execute_query` does.
- Bootstrap done-means SQL (firebase collection name):
  `SELECT query_id id, query_ref ref, query_status_a27 status,
  query_type_a28 type, query_dialect_a30 engine, query_queue_a58
  queue, query_timeout timeout, name, code query FROM testfb_queries
  WHERE (query_status_a27 = 1) ORDER BY query_type_a28 desc;`
  Implicit aliases without `AS`; parenthesized WHERE `=`; ORDER BY
  DESC. QTC needs JSON integers for `ref`/`type`/`queue`/`timeout`
  and strings for `query`/`name`.
- One collection only. No JOIN / GROUP BY / LATERAL (Phase 9).
  Optional WHERE, AND-chained predicates the DML WHERE already has
  (`=`, `IS NULL`, `IN` literals) plus comparisons (`<>`, `<`, `>`,
  `<=`, `>=`) and wrapping parens. ORDER BY one or more columns
  ASC/DESC. LIMIT a non-negative integer.
- Line caps: `sql_parse.c` ~940, `sql_expr.c` ~954, `sql_dml.c`
  ~882. Standalone SELECT parse/execute in `sql_select.c` (~764).
  Add `FIREBASE_SQL_KIND_SELECT`; dispatch from `query.c` (SELECT is
  not DDL and not `firebase_sql_kind_is_dml`).
- 8.3 grep (not yet `[x]`): `IN (literals)` already in DML WHERE.
  `LIKE`+`OR`+`UPPER`+`||` in 1123 (single-table `queries`) and in
  JOIN QueryRefs 1122 / 1124 / 1131. Scalar `(SELECT MAX…)` is the
  Phase 7 CTE, not standalone SELECT. `CONCAT()` does not appear in
  Acuranzo; concat is `||`. `CAST(:NAME AS …)` is 1151 (`FROM
  numbers`). `FB_TIME_ADD` / `FB_SESSION_SECS` are 1096 / 1112.
  Default: LIKE / `||` / CAST / time macros **[~]** unless bootstrap
  needs them; JOIN-shaped QueryRefs stay Phase 9. That is capability
  sequencing, not a smaller Firebase language.
- Gaps vs other engines that are unfinished, not a new API:
  `execute_query` ignores `parameters_json` today; prepare and
  transactions still fail-closed; no standalone SELECT yet.
- Session paused before go (token issues). No Phase 8 C in this
  turn.

### Lessons learned

- Splicing `:NAME` into SQL literals would be a Firebase-only path.
  The other engines never do that; bind by index after
  `convert_named_to_positional`.
- DROP_CHECK was already per-engine (PG terminate, MySQL CHAR(0)
  error, DB2 SIGNAL, SQLite SELECT string). `FB_REFUSE_DROP` failing
  the statement is closer to MySQL/DB2 than a new invention.
- MAX+1 with no counters sidecar matches the SQL the other engines
  already run.

---

## Phase 9 — JOINs

### Goal

`JOIN` / `LEFT JOIN` / `LEFT JOIN LATERAL` execute in memory so
QueryRefs used by auth, conduit, and mail are not firebase-exceptions.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 INNER/LEFT JOIN two collections; Unity fixtures from a real
      QueryRef (e.g. accounts + account_contacts).
- [ ] 9.2 LATERAL (1168) or documented interpreter equivalent.
- [ ] 9.3 Grep remaining JOIN forms; list in Status.

### Done means

A multi-table QueryRef fixture returns the same column names/row count
shape as SQLite for the same seed data.

### Exit gate

- `mkp` + named `mku`.

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

## Phase 10 — AutoMigrations / Test 37 (full Acuranzo)

### Goal

Hydrogen AutoMigrations against the emulator apply the **full** Acuranzo
design. Same done means as Test 32.

### Entry gate

Phase 9 Status complete. Payload includes `database_firebase.lua`
(`mkt`).

### Work items

- [ ] 10.1 `hydrogen_test_37_firebase.json` (`Engine: firebase`,
      emulator, schema `testfb`, `AutoMigration` + `TestMigration` as
      Test 32).
- [ ] 10.2 `tests/test_37_firebase_migrations.sh` **alongside**
      Cockroach 37. Emulator lifecycle via extras scripts.
- [ ] 10.3 Docs `docs/H/tests/test_37_firebase_migrations.md`.
- [ ] 10.4 Run until LOAD/APPLY/REVERSE match Test 32's expectations
      (failure detection already in the 37 script). Any failing
      migration is an interpreter or Helium-arm bug, not a skip list.

### Done means

`tests/test_37_firebase_migrations.sh` reports migration completed on
the emulator for the full design; `mks`; markdown exists.

### Exit gate

- Live Test 37 firebase log path in Status.
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

## Phase 11 — Retire Cockroach

### Goal

No Cockroach **names** in active tests/configs/wrappers/current docs
(metrics history excluded).

### Entry gate

Phase 10 Status complete.

### Work items

- [ ] 11.1 Only firebase Test 37 remains.
- [ ] 11.2 Replace Cockroach configs 40/43/45/46/47/58.
- [ ] 11.3 Engine loops, flush, SchemaTool names (behavior Phase 12).
- [ ] 11.4 Config schema enum.
- [ ] 11.5 `rg -i cockroach` on active trees: only metrics/,
      plans/complete/, and this plan's inventory.

### Done means

7-engine scripts name Firebase; Test 37 is firebase-only.

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

## Phase 12 — SchemaTool, SchemaHelper, flush

### Goal

Operator tools talk Firestore REST/emulator, not `psql` on `democrdb`.

### Entry gate

Phase 11 Status complete.

### Work items

- [ ] 12.1 `schematool_firebase.sh` (not an alias to postgresql).
- [ ] 12.2 schemahelper connect/apply/const.
- [ ] 12.3 `hydrogen_flush.sh` prefix delete / emulator reset.
- [ ] 12.4 `transaction_utils.sh` firebase path.

### Done means

No cockroach SchemaTool wrapper; firebase wrapper does not call `psql`.

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

## Phase 13 — Wider blackbox matrix

### Goal

Former Cockroach suites run Firebase. Full QueryRef SQL is in scope;
skips only for environmental reasons (emulator down), not "not
implemented."

### Entry gate

Phase 12 Status complete.

### Work items

- [ ] 13.1 Test 40 auth live on firebase.
- [ ] 13.2 Tests 43, 45, 46, 47, 58.
- [ ] 13.3 Test 41/44/51/54 docs/configs.

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

## Phase 14 — Docs sweep

### Goal

Current docs describe five Helium dialects and seven operator engines
(MariaDB + Yugabyte aliases, Firebase real). Cockroach is historical.
MACRO_REFERENCE has a Firebase column. DATABASES has install + "UDFs
are in-process."

### Entry gate

Phase 13 Status complete.

### Work items

- [ ] 14.1 Helium GUIDE, MACRO_REFERENCE, DATABASES, TESTING_GUIDE,
      BROTLI_COMPRESSION, design READMEs, `docs/He/DATABASES/database_firebase.md`.
- [ ] 14.2 Hydrogen TESTING, INSTRUCTIONS, PARAMETER_BINDING, SECRETS,
      STRUCTURE, SITEMAP, MAIL_GUIDE, SchemaTool/SchemaHelper, tests README.
- [ ] 14.3 Lithium `DATABASE-MIGRATIONS.md`.
- [ ] 14.4 Snapshot section stays dated 2026-09-16; add an "after Phase
      11" pointer.

### Done means

`mkl` green; no active doc claims Cockroach is supported.

### Exit gate

- `zsh -ic 'mkl'`; markdownlint on touched files (Test 90 / hydrogen
  ignore).

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

## Phase 15 — Completeness and coverage re-check

### Goal

Every completeness-fence row is true or `[~]`. Coverage fences hold.
Dead-code list has no stray firebase symbols.

### Entry gate

Phase 14 Status complete.

### Work items

- [ ] 15.1 Walk completeness table.
- [ ] 15.2 Walk coverage fences; `extras/add_coverage.sh` as needed.
- [ ] 15.3 `mkt` dead-code gate.
- [ ] 15.4 `mkp`, `mks`, `test_98`, Test 31, Test 37, Test 40.

### Done means

Fences green; Test 37 and Test 40 firebase green.

### Exit gate

- Commands in 15.4 actually run; output cited in Status.

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

## Phase 16 — Production runbook (optional)

### Goal

Service-account JWT, IAM, `firestore.indexes.json` export from the
indexes the interpreter recorded. Or park with `[~]`.

### Entry gate

Phase 15 Status complete.

### Work items

- [ ] 16.1 Production JWT mint Unity with a fixture key, or `[~]`.
- [ ] 16.2 Operator notes linked from SITEMAP the same phase.
- [ ] 16.3 Composite indexes file generation if production needs it.

### Done means

Documented production path or explicit park. Then move plan to
`plans/complete/FIREBASE_COMPLETE.md`; drop TODO 27; `mkl`.

### Exit gate

- Status complete or parked.

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
| Unity | Connstring, registry, every `FB_*` fn, SQL DDL/DML/SELECT/JOIN, HTTP seam, error class. Hash fixture vs other engine. |
| Blackbox | Test 37 emulator **full** AutoMigrations. Test 40+ when Phase 13 says so. Never hit `firestore.googleapis.com` in CI. |
| Coverage | See [Coverage fences](#coverage-fences). |
| Build | `mkq` ordinary C; `mkt` after add/remove `src/`; `mkp` after C; `mks` after Bash; `test_98` after Lua. |

Port scheme: Test 37 → **537x**.

---

## Threat notes

- **Secret leakage:** SA JSON, Google bearer tokens, `password_hash`,
  JWTs in tokens/sessions. Mask `firebase://`.
- **SSRF:** REST origin comes from config, never from QueryRef SQL.
- **Emulator vs prod:** empty `Pass` + `firestore.googleapis.com` fails
  closed.
- **1 MiB document cap / 900 KiB fail closed.**
- **500 writes per Firestore transaction.**
- **Hash mismatch** across engines is a login outage — Phase 4 fixture
  is mandatory.

### Risks

| Risk | Mitigation |
| --- | --- |
| Pretending Firestore is libpq | SQL interpreter in C; never alias to postgresql |
| Skipping tables "until v2" | Forbidden; Test 37 is full Acuranzo |
| UDF `.so` uploaded to Google | In-process fns; extras is emulator only |
| SHA-256 incompatibility | Cross-engine fixture in Phase 4 |
| JOIN/LATERAL missed | Phase 9 grep + Unity from real QueryRefs |
| sqruff on FB_* SQL | Test 31 skip sqruff; still flag `${…}` |
| Empty 7-engine slot | Phase 11 only after Phase 10 green |
| libcurl in DQM threads | `NOSIGNAL`, per-request easy handle |
| Helium ID drift | Re-check disk at packet time |
| Coverage late | Per-phase fence |

---

## Working Log (cross-phase memory)

### Decisions log

- **(Plan authored, 2026-09-16)** FIREBASE created to replace the
  CockroachDB operator slot with a real fifth Hydrogen engine.
- **(Plan revised, 2026-09-16)** Dropped v1 table subset and JSON-ops
  dialect. Helium still emits SQL via a complete
  `database_firebase.lua`. Hydrogen interprets that SQL against
  Firestore and evaluates Brotli/Base64/SHA-256/json_ingest/NOW/TZ
  **in-process** (the extras-UDF analog). Test 37 done means = full
  Acuranzo, same as Test 32. extras/firebase_emulator is install/start,
  not a Cloud Function. Phases renumbered 0–16. TODO item **27**.
- **(2026-09-16)** JSON ingest/extract is a first-class headache with
  Brotli, Base64, and SHA-256 — solvable, unlike Cockroach. Lock:
  control-char fix-up, `$ref` accepted (not DB2 JSON2BSON), `${JSON}`
  stored as text (SQLite-compatible, not Firestore `mapValue`),
  `FB_JSON_VALUE` for `${JRS}`/`${JRM}`/`${JRE}`. Phase 4 Unity must
  prove all four contracts before Test 37.
- **(2026-09-16, Phase 0 complete)** User approved locks 1–19 as
  written. Amendment lock 20: `${SIZE_COLLECTION}` = `LENGTH(collection)`.
  Lock 18 kept fail-closed (900 KiB field, 500 writes/txn; split the
  migration, do not auto-chunk). Next free Acuranzo **1383**. Lookup
  030 still ends at key 5. Phase 1 waits for an explicit go.
- **(2026-09-16, Phase 1 complete)** extras/firebase_emulator start/stop
  verified on `:8080` with pinned `firebase-tools@15.30.1` (local npm,
  no Google login). UI disabled. SECRETS names:
  `FIREBASE_PROJECT` / `FIREBASE_EMULATOR_HOST` /
  `FIREBASE_EMULATOR_PORT` / `FIREBASE_SA_JSON`. Next is Phase 2;
  do not start until asked.
- **(2026-09-16, Phase 3 complete)** C engine skeleton: enum after
  DB2 before AI; `firebase://` before SQLite fallback; fifth
  `firebase_count` out-param on `database_get_counts_by_type`;
  HTTP GET health + Unity seam; emulator-only connect (production
  JWT Phase 16). User applied Lookup 030 key 6 (`acuranzo_1383`).
  Next is Phase 4; do not start until asked.
- **(2026-09-16, Phase 4 complete)** In-process `FB_*` Unity-green:
  Base64, Brotli (lua-brotli q11 + extras fixture), SHA-256 login
  fixture matches SQLite, JSON ingest/`$ref`/extract, NOW + IANA
  CONVERT_TZ. `FB_TIME_ADD`/`FB_SESSION_SECS` deferred to Phase 6/8.
  Next is Phase 5 SQL DDL; do not start until asked.
- **(2026-09-17, Phase 6 complete)** INSERT VALUES / UPDATE / DELETE
  Unity-green. Document id from evaluated PK (`30_6`). Brotli
  decompress-on-INSERT stores plaintext. Transactions still
  fail-closed.
- **(2026-09-17, Phase 7 complete)** INSERT…SELECT WITH CTE MAX+1,
  1190 `SELECT *`, and RETURNING Unity-green. Two LOAD inserts get
  `query_id` 1 then 2; RETURNING is `[{"query_id":1}]`. No counters
  sidecar. Next is Phase 8 SELECT/binds; do not start until asked.
- **(2026-09-17, pre-Phase 8)** Status-at-a-glance table added at the
  top (Quick / Moderate / Difficult for remaining work). Standing
  parity rule: Firebase follows the other engines' bind and
  `QueryResult` contracts; do not splice `:NAME` into SQL literals;
  do not change PG / SQLite / MySQL / DB2. Phase 8 locks written in
  that phase's Working Log; implementation waits for go. Session
  paused (token issues). Phase 7 C may still be uncommitted.

### Surprises / deviations (historical, still true)

- Helium Test 31 / Test 71 / `lua.c` listed **four** dialects at plan
  authoring. Phase 2 added firebase (dialect 6); `lua.c` `engines[]`
  now includes it. Cockroach was never a Helium engine.
- Lookup 030 key 5 is "MS SQL Server" and unused; do not steal it.
  Key 6 (Firebase) is `acuranzo_1383.lua`, applied by the user.
- `database_get_counts_by_type` now has five out-params
  (`firebase_count` last). `launch_database.c` still counts four.
- SQLite crypto/brotli/convert_tz are loaded from `/usr/local/lib/*.so`
  in `sqlite/connection.c` — Firebase has no equivalent load hook
  inside Google.
- Brotli in migrations decompresses **at INSERT** into `queries.code`;
  skipping the function eval stores unusable wrappers.
- JSON ingest exists because migration JSON is not always spec-valid
  (raw newlines in strings). SQLite skips validation; PG/MySQL/DB2
  fix up. Firebase must fix up. DB2 alone needed `JSON_INGEST_SCHEMA`
  for `$ref` (JSON2BSON); firebase aliases ingest like PG/MySQL/SQLite.
- `${JRS}`/`${JRM}`/`${JRE}` appear in QueryRefs 1114/1124 (`$.icon`).
- SchemaHelper `schemahelper_qdecode.lua` has per-engine Brotli/Base64
  regexes and will need `FB_*` patterns in Phase 12.
- Acuranzo `if engine` files today: 1000, 1135, 1151, 1190.

### Reusable snippets / gotchas

- After C: `mkq` then `mkp`. After add/remove `src/`: `mkt` then `mkp`.
- After bash: `mks`. After Lua: `test_98`. After docs: `mkl`.
- Never apply Helium packets; hand them to the user.
- Payload rebuild (`mkt`) is required before Test 31–38 see new
  `database_firebase.lua`.
- Do not `dlopen` libpq for firebase.
- Do not wrap SQL in JSON and hope.
- Cross-check SHA-256 against SQLite `crypto_sha256` before declaring
  login green.
)
