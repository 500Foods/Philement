# Test 47: MCP Streamable HTTP

## Overview

The [`test_47_mcp.sh`](/elements/001-hydrogen/hydrogen/tests/test_47_mcp.sh)
script black-box tests the MCP subsystem over Streamable HTTP: Hydrogen JWT
login, Lua `Mcp.Server`, fixture tools, session lifecycle, and timeout.

## Purpose

Validates across all seven database engines (parallel):

- Disabled MCP does not bind the MCP port
- Unauthenticated `GET <Path>/healthz` and RFC 9728 PRM well-known → **200**
- Missing Bearer on `Path` → **401** with `WWW-Authenticate` `resource_metadata`
- Present mismatched `Origin` → **403**
- `GET Path` → **405** (no SSE in v1)
- `initialize` → `serverInfo` + `instructions` + `Mcp-Session-Id`
- `notifications/initialized` → **202**
- `tools/list` includes `Mcp.Echo` / `Mcp.EchoStrict` with `inputSchema`
- Echo success (`result.content`, no `isError`)
- EchoStrict bad args → `result.isError=true` (not JSON-RPC `error`)
- Unknown / non-`mcp_access` tools hidden as tool errors
- Client-supplied `_hydrogen` rejected
- Unknown session → **404**; session hijack (SQLite) → **401**
- Two overlapping Echo calls with `WorkerCount=2` both **200**
- `Mcp.Sleep` exceeds `RequestTimeoutSeconds` → JSON-RPC **-32603**
- `notifications/cancelled` → **202**; shutdown still clean
- `DELETE` → **204** then reuse → **404**
- JWT `GET /api/mcp/status` on the WebServer port

## Test Configuration

- **Test Name**: MCP Server
- **Test Abbreviation**: MCP
- **Test Number**: 47
- **Version**: 1.0.0

## Port Assignment

Dedicated ports in the `1547x` (WebServer) and `1548x` (MCP daemon) ranges,
below the typical Linux ephemeral client range (same `15<TT>x` convention as
Test 43 / 46). Do not use 547x.

| Engine | WebServer | MCP |
|--------|-----------|-----|
| PostgreSQL | 15470 | 15480 |
| MySQL | 15471 | 15481 |
| SQLite | 15472 | 15482 |
| DB2 | 15473 | 15483 |
| MariaDB | 15474 | 15484 |
| CockroachDB | 15475 | 15485 |
| YugabyteDB | 15476 | 15486 |
| Disabled (SQLite) | 15477 | 15487 |

## Configuration Files

- `hydrogen_test_47_mcp_postgres.json`
- `hydrogen_test_47_mcp_mysql.json`
- `hydrogen_test_47_mcp_sqlite.json` (isolated DB copy under diagnostics)
- `hydrogen_test_47_mcp_db2.json`
- `hydrogen_test_47_mcp_mariadb.json`
- `hydrogen_test_47_mcp_cockroachdb.json`
- `hydrogen_test_47_mcp_yugabytedb.json`
- `hydrogen_test_47_mcp_disabled.json`

Each enabled config turns on **API** (JWT), **Scripting** (`WorkerCount` 2),
and **MCP** (`Protocol` `Mcp.Server`, `RequestTimeoutSeconds` 4).

**SQLite** uses an isolated copy of `hydrodemo.sqlite` with AutoMigration off
so the suite does not mutate the shared fixture. The copy must already
contain MCP seeds (`Mcp.Server` / Echo / EchoStrict / Sleep, QueryRefs
**#152** / **#153**).

**Other engines** run the same cases against the live DB (migrations
1365–1372). If `initialize` cannot load `Mcp.Server`, that engine is
**skipped** (still counted pass). The suite **requires** the SQLite full
path to pass.

## Prerequisites

- Hydrogen binary (via `find_hydrogen_binary`)
- `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_USER_PASS`, `HYDROGEN_DEMO_API_KEY`,
  `HYDROGEN_DEMO_JWT_KEY`, `PAYLOAD_KEY`
- Live engines for non-SQLite variants (same fixtures as tests 40/43/46)

## Test Flow (per engine)

1. Start Hydrogen; wait for `READY FOR REQUESTS`
2. Probe healthz / PRM / 401 / Origin / GET 405
3. `POST /api/auth/login` as demo user on database `Acuranzo`
4. MCP initialize, tools/list, tools/call, session, timeout, DELETE
5. Graceful shutdown

## Manual helpers

- [`extras/mcp_probe.sh`](/elements/001-hydrogen/hydrogen/extras/mcp_probe.sh) —
  curl/jq wrapper for PRM → 401 → initialize → tools/list → tools/call
- MCP Inspector CLI is not part of this script (no network installs in
  blackbox CI)

## Related

- Plan: [MCP_COMPLETE.md](/docs/H/plans/complete/MCP_COMPLETE.md) Phase 13
- Fixture migrations: Helium `acuranzo_1365`–`1372`
