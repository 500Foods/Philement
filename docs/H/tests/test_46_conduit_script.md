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
- Parse/routing: invalid JSON, missing/empty `script`, slash name, `params` array, non-bool `wait`, non-int `timeout_seconds` → **400**
- `POST .../script/{job_id}` → **405**
- `GET` without JWT → **401**; unknown `job_id` → **404**; trailing slash → **400**
- SQLite `Api.Fail` → **200** with `status=failed` (Lua `error()`)

## Test Configuration

- **Test Name**: Conduit Script
- **Test Abbreviation**: CSC
- **Test Number**: 46
- **Version**: 1.3.4

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
The seed is **idempotent**: it only adds the `invokable` column if missing and
uses `INSERT OR IGNORE` for all rows, so a baseline that already has migrations
1296–1298 is not overwritten or downgraded.

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

## Suite-Load Resilience

Configs match test_40 on the shared live DBs: `LOGINMAXATTEMPTS` **100000**
(default 5 would `block_ip_address` 127.0.0.1 for 15 minutes when tests 41/44
pollute `failed_attempts`) and dedicated **Fast/Medium/Cache** workers
(QueryRef **#149** is `QTC_FAST`; with only `Slow.start=2` those invokes fall
through to the Lead DQM).

Test 46 runs in the same parallel batch (group 4, tests 40–49) as test_41
(ASAN, 500 concurrent auth requests) and test_44 (native RSS, 5000 concurrent
auth requests). These siblings create heavy CPU and database contention that
can cause transient curl failures (connection refused, 5xx, 408, 429).

The `api_request` helper retries these transient failures with linear backoff
up to 5 attempts, mirroring the fix already applied to test_40. Definitive
HTTP responses (2xx, 3xx, 4xx except 408) are never retried — expected error
codes like 401/404/400 are returned immediately.

The async GET polling uses a **45-second time-based deadline** (via `DATE +%s`)
with a **direct curl call** (5s max-time, no retry backoff) instead of
`api_request`. This is critical because `api_request`'s 5-retry linear backoff
adds up to **10 seconds of sleep per failed GET**, which would starve the
polling budget. The direct curl call enables ~0.2s polling intervals.

**POST retry on `job_not_found`**: Under heavy parallel DB load (tests 41/44),
the server may accept an async job (HTTP 202 + `job_id`) but fail to persist it.
The GET polling then receives `404 job_not_found`. The POST is retried up to 4
times. `timeout_seconds` is 60 (`ClientInvokeMaxTimeout`) for worker headroom.

## Related

- Plan: [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) Phase 9
- API: [script.md](/docs/H/api/conduit/script.md)
- Fixture migrations: Helium `acuranzo_1296`–`1298` (`Api.Echo`, `invokable`, QueryRef #149)
- Unity coverage: `script_test_*`, `scripting_invoke_test_*`
