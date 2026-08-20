# Test 46: Conduit Script Invoke

## Overview

The [`test_46_conduit_script.sh`](/elements/001-hydrogen/hydrogen/tests/test_46_conduit_script.sh)
script black-box tests client-facing Lua script invoke over
`POST /api/conduit/script` and `GET /api/conduit/script/{job_id}`
(LUA_CLIENT Phase 9).

## Purpose

Validates across all seven database engines (parallel):

- JWT-required access (missing auth → **401**)
- Named script invoke with `wait:true` against fixture `Api.Echo`
- Response body: `status=completed`, echoed `params`, injected `params._hydrogen.sub`
- Unknown script and non-invokable script → **404** (no existence leak)
- Client-supplied `params._hydrogen` → **400**
- `wait:false` → **202** with `job_id`, then GET polls to terminal result

## Test Configuration

- **Test Name**: Conduit Script
- **Test Abbreviation**: CSC
- **Test Number**: 46
- **Version**: 1.1.0

## Port Assignment

Dedicated ports in the `1546x` range (outside the Linux ephemeral client range):

| Engine | Port |
|--------|------|
| PostgreSQL | 15460 |
| MySQL | 15461 |
| SQLite | 15462 |
| DB2 | 15463 |
| MariaDB | 15464 |
| CockroachDB | 15465 |
| YugabyteDB | 15466 |

## Configuration Files

- `hydrogen_test_46_conduit_script_postgres.json`
- `hydrogen_test_46_conduit_script_mysql.json`
- `hydrogen_test_46_conduit_script_sqlite.json` (isolated DB copy under diagnostics)
- `hydrogen_test_46_conduit_script_db2.json`
- `hydrogen_test_46_conduit_script_mariadb.json`
- `hydrogen_test_46_conduit_script_cockroachdb.json`
- `hydrogen_test_46_conduit_script_yugabytedb.json`

Each enables **API** (JWT) and **Scripting** (workers + DefaultDatabase `Acuranzo`).

**SQLite** uses an isolated copy of `hydrodemo.sqlite` with a blackbox seed for
`Api.Echo`, `scripts.invokable`, and QueryRef **#149** (AutoMigration is off on
this path because the shared fixture has an APPLY hole at migration 1283).

**Other engines** attempt the same cases when the live DB already has migrations
1296–1298; otherwise the engine is **skipped** (still counted pass) until
fixtures catch up. The suite **requires** the SQLite full path to pass.

## Prerequisites

- Hydrogen binary (via `find_hydrogen_binary`)
- `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_USER_PASS`, `HYDROGEN_DEMO_API_KEY`,
  `HYDROGEN_DEMO_JWT_KEY`, `PAYLOAD_KEY`
- Live engines for non-SQLite variants (same fixtures as tests 40/43)

## Test Flow (per engine)

1. Start Hydrogen; wait for `READY FOR REQUESTS`
2. `POST /api/auth/login` as demo user on database `Acuranzo`
3. Exercise auth/error/happy-path cases above
4. Graceful shutdown

## Related

- Plan: [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) Phase 9
- API: [script.md](/docs/H/api/conduit/script.md)
- Fixture migrations: Helium `acuranzo_1296`–`1298` (`Api.Echo`, `invokable`, QueryRef #149)
- Unity coverage: `script_test_*`, `scripting_invoke_test_*`
