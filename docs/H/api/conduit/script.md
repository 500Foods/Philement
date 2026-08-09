# Conduit Script Invoke (`/api/conduit/script`)

## Overview

`/api/conduit/script` runs a **named, DB-backed Lua script** on a scripting
worker and returns a job status envelope plus optional JSON result body.
C does not interpret business logic: the client chooses an allowlisted script
name and JSON params; the script talks to Canvas, SQL, HTTP, and so on via the
usual `H.*` host API.

Same mental model as `/api/conduit/auth_query`: package request in, package
result out. Scripts must exist in the `scripts` table with `invokable = 1`
(default is **not** invokable).

Primary SPA consumer pattern: Reception free-enroll and similar paths map a
config path to a script name (for example `Enroll.FreeCourse`) — no
product-specific C routes in Hydrogen.

## Base URL

```text
http://your-server:port/api/conduit/script
http://your-server:port/api/conduit/script/{job_id}
```

## Prerequisites

- `Scripting.Enabled` = `true` (else **503** `scripting_disabled`)
- Workers and `DefaultDatabase` (or a single configured DB) so DB script load works
- JWT from `/api/auth/login` (or equivalent OIDC path)
- Target row in `scripts` with `invokable = 1` (migrations / ops seed)

## Configuration

Optional knobs under `Scripting` (defaults match Phase 0 design lock):

| Key | Default | Description |
|-----|---------|-------------|
| `ClientInvokeDefaultTimeout` | `15` | Default `timeout_seconds` when omitted |
| `ClientInvokeMaxTimeout` | `60` | Clamp ceiling for `timeout_seconds` |
| `ClientInvokeMaxParamsBytes` | `262144` (256 KiB) | Max serialized params JSON |
| `ClientInvokeMaxResultBytes` | `1048576` (1 MiB) | Max `H.set_result_json` body |

## Authentication

JWT **required** on POST and GET.

```text
Authorization: Bearer <access_token>
Content-Type: application/json
```

Identity is injected server-side as `params._hydrogen` (filtered claims bag).
Clients **must not** send `params._hydrogen` (**400** `reserved_params`).
`id_token` is omitted from the bag.

GET `{job_id}` is allowed only for the submitting `sub` (stored on the
scoreboard at submit). Other callers get **403**.

## POST — run script

### Request

```bash
curl -sS -X POST "http://127.0.0.1:5000/api/conduit/script" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{
    "script": "Api.Echo",
    "params": { "hello": "world" },
    "wait": true,
    "timeout_seconds": 15
  }'
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `script` | string | Yes | Canonical `Group.Name` (dot only; slash rejected) |
| `params` | object | No | JSON object passed to the worker as global `params` |
| `wait` | boolean | No | Default **true**. If false, return immediately with `job_id` |
| `timeout_seconds` | integer | No | Wait budget when `wait` is true; default 15, max 60 |

### Response — job outcomes (`wait: true`)

Transport success for terminal job results uses **HTTP 200**, including
`status: failed`, `killed`, and `timeout`. Auth, validation, and routing
errors use 4xx/5xx as listed below.

```json
{
  "status": "completed",
  "job_id": "ABCDE",
  "script": "Api.Echo",
  "result": {
    "hello": "world",
    "_hydrogen": { "sub": "..." }
  },
  "result_type": "json",
  "result_location": null,
  "error": null,
  "elapsed_ms": 123
}
```

| `status` | Meaning |
|----------|---------|
| `completed` | Worker finished; `result` is JSON body (or `{}` if script set none) |
| `failed` | Lua runtime failure; `error` is message only (no traceback) |
| `killed` | Job killed (limits or request) |
| `timeout` | Wait budget exhausted; kill requested; GET may still see a later terminal |

Business failures should prefer **COMPLETED** with
`result: { "ok": false, "code": "...", "message": "..." }` rather than crashing
the script.

### Response — async submit (`wait: false`)

**HTTP 202**:

```json
{
  "status": "pending",
  "job_id": "ABCDE",
  "script": "Api.Echo",
  "result": {},
  "result_type": null,
  "result_location": null,
  "error": null,
  "elapsed_ms": 0
}
```

Poll with GET until terminal.

## GET — job status

```bash
curl -sS "http://127.0.0.1:5000/api/conduit/script/${JOB_ID}" \
  -H "Authorization: Bearer ${TOKEN}"
```

Same envelope as POST when terminal. Non-terminal jobs return `pending` or
`running`. Scoreboard entries are in-memory and may disappear after retention
or restart — clients must not assume durable history.

## HTTP error codes

| HTTP | `error` (typical) | Trigger |
|------|-------------------|---------|
| 400 | `missing_script`, `invalid_params`, `invalid_wait`, `invalid_timeout`, `reserved_params` | Bad body |
| 401 | (JWT helper) | Missing/invalid token |
| 403 | (message) | GET job owned by another `sub` |
| 404 | `script_not_found` | Unknown name **or** not invokable (existence-hiding) |
| 404 | (job) | Unknown `job_id` |
| 405 | | Method other than POST on base / GET on `{job_id}` |
| 413 | `params_too_large` | Params over configured max |
| 503 | `scripting_disabled` | Scripting off |
| 503 | `scripting_shutdown` | Wait aborted because subsystem is shutting down |

## Lua author contract

Worker scripts receive a global `params` table (merged client params +
`_hydrogen`). To return a body for SPA `fetch`:

```lua
H.set_result_json({
  ok = true,
  data = { ... }
})
```

- Host encodes the table with jansson (arrays vs objects: pure 1..n integer
  keys → JSON array; empty table → object).
- Successful `set_result_json` also sets `result_type` to `"json"`.
- Oversize results are rejected at the scoreboard (config max).
- `H.set_result(type, location)` remains metadata-only (type + location
  strings); prefer `set_result_json` for client invoke responses.

Only scripts with DB column `scripts.invokable = 1` are callable. Fixture
seed: `Api.Echo` (Helium migrations `acuranzo_1296`–`1298`, QueryRef **#149**).

## Example: echo fixture

```bash
# TOKEN from /api/auth/login (database claim Acuranzo or your DefaultDatabase)
curl -sS -X POST "http://127.0.0.1:${PORT}/api/conduit/script" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"script":"Api.Echo","params":{"hello":"world"},"wait":true,"timeout_seconds":15}'
# Expect HTTP 200, status=completed, result.hello=world, result._hydrogen.sub=...
```

Optional probe (script may call `H.http.get_sync` when `probe_health` is true):

```json
{"script":"Api.Echo","params":{"probe_health":true},"wait":true}
```

## Reception / SPA handoff

Hydrogen provides only the generic surface. Product work (for example
`enroll.freeEnrollPath` → script `Enroll.FreeCourse`, FL-49b) lives in the SPA
and Helium seed data after this endpoint exists — not in Hydrogen C routes.

## Testing

- Blackbox: [`test_46_conduit_script.md`](/docs/H/tests/test_46_conduit_script.md)
- Unity: `script_test_*`, `scripting_invoke_test_*`, scoreboard result_json suites

## Implementation files

```directory
src/api/conduit/script/
├── script.c
└── script.h
src/scripting/
├── scripting_invoke.c    # submit_from_db + wait_job
├── scoreboard.c          # result_json
└── scripting_api_system.c  # H.set_result_json
```

## Related documentation

- [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) — `H.*` including `set_result_json`
- [scripting README](/docs/H/core/subsystems/scripting/README.md) — config and job execution
- [Conduit API](/docs/H/core/subsystems/conduit/conduit_api.md) — query endpoints
- [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) — implementation plan
- [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) — author guide
