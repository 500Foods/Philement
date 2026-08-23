<!-- markdownlint-disable MD007 MD024 -->
# MCP Server Subsystem Implementation Plan

## Purpose

Define a gated, phase-by-phase plan for adding a **Model Context Protocol (MCP)**
server to Hydrogen as a first-class subsystem.

Hydrogen already embeds Lua, can invoke named DB-backed scripts over JWT REST
([LUA_CLIENT](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md)), and has a mature
launch/landing/registry/status machine. MCP is the missing **protocol adapter**:
AI clients (Claude, Cursor, custom agents) speak JSON-RPC; Hydrogen should
expose tools, resources, and prompts **entirely through Lua**. Adding or
evolving an MCP surface must not require C changes.

This document is the working plan. Edit it as work proceeds. Each phase is
numbered, focused, and gated. Do not start a phase until the previous phase's
exit gate is green. Record learnings in the Working Log.

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase has: **Goal**, **Dependencies**, **Entry gate**, **Work items**,
  **Exit gate / validation**, **Status**, **Lessons learned**.
- Mark work items `[x]` only when verification actually passed.
- Defer with `[~]` plus one-line rationale and target phase.
- After each phase: fill **Status**, append reusable discoveries to
  **Working Log**, then stop for review before the next phase.
- Build aliases: `zsh -ic 'mkt'`, `mku <base>`, `mkp`, `mka` (C); `mks` (shell).
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

### Testing policy (Unity + blackbox)

| Layer | When | What |
| --- | --- | --- |
| **Unity** | Every C phase (1–12, etc.) | Config, launch/landing, JWT/OIDC accept, PRM/`WWW-Authenticate`, JSON-RPC envelope, Origin, session reaper, MHD suspend/resume seams, allowlist, counters, inline `H.mcp.call`. No live multi-engine DB unless already mocked. See [Unity coverage inventory](#unity-coverage-inventory) |
| **Blackbox** | Phase 13 (required for plan Done) | Real hydrogen + Hydrogen JWT + MCP initialize/tools/list/call + fixture scripts + concurrent clients + cancel/timeout + PRM discovery. Docs in `/docs/H/tests/test_47_mcp.md`. Ports **1547x / 1548x** (not 547x — ephemeral clash; see Test 46) |
| **Both required** | Definition of Done | No “Unity only” ship for a client-facing protocol |

Phases must not skip Unity when adding C. Phase 13 must not be waived without
an explicit Status variance.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-22):** Plan drafted. No implementation.
Next: **Phase 0 Design Lock**.

### Resume here next session

1. Confirm this document is the source of truth; no `src/mcp/` exists yet.
2. Baseline: `zsh -ic 'mkt'`.
3. Run Phase 0 as design-only. Do not write C until the locked-decisions table
   is filled and reviewed.
4. Present the Phase 1 chunk (config only) after Phase 0 Status is complete.

### Session checklist

1. Read **CURRENT PAUSE POINT** and last **Working Log** entries.
2. Confirm previous phase **Status** is complete.
3. Re-read next phase Goal + Exit gate only.
4. Implement → verify gates → update this doc → stop.

Build aliases: `zsh -ic '<alias>'` (`mkt`, `mka`, `mku <base>`, `mkp`, `mks`).

## Priority

| | |
| --- | --- |
| **Band** | P2 — new capability, not a close-the-loop or safety gate |
| **Effort** | XL (full subsystem + transport + Lua protocol + tests + docs) |
| **Done** | 0% — plan only |
| **Why this shape** | Unblocks AI-tool access to Hydrogen without a product-named C surface. Reuses Scripting, JWT, and the `scripts.invokable` allowlist pattern |
| **Do not start casually** | Touches `MAX_SUBSYSTEMS`, launch/landing dispatch, status metrics, and blackbox configs. Phase 2 is the dangerous count-bump |

Backlog entry: [TODO.md item 24](/docs/H/TODO.md).

---

## Scope And Repo Areas

Primary repo area: `/elements/001-hydrogen/hydrogen`

Related:

- `/elements/001-hydrogen/hydrogen/src/config/` — new `config_mcp.{c,h}`,
  defaults, `AppConfig` member **T. MCP**
- `/elements/001-hydrogen/hydrogen/src/launch/` — `launch_mcp.c`, readiness
  registration, `launch.c` dispatch
- `/elements/001-hydrogen/hydrogen/src/landing/` — `landing_mcp.c`, landing
  table + dispatch
- `/elements/001-hydrogen/hydrogen/src/mcp/` — new runtime (transport, session,
  counters). Empty today
- `/elements/001-hydrogen/hydrogen/src/scripting/` — reuse
  `scripting_submit_job`, scoreboard wait, `H.set_result_json`
- `/elements/001-hydrogen/hydrogen/src/api/auth/` — JWT extract/validate
- `/elements/001-hydrogen/hydrogen/src/status/` — `ServiceMetrics` + JSON /
  Prometheus
- `/elements/001-hydrogen/hydrogen/src/globals.h` — `SR_MCP`,
  `MAX_SUBSYSTEMS`, `INITIAL_REGISTRY_CAPACITY`
- `/elements/002-helium/acuranzo/migrations/` — `scripts.mcp_access` column,
  QueryRef for MCP-only load, seed `Mcp.Server` + fixture
- `/docs/H/core/subsystems/` — new MCP subsystem doc
- `/docs/H/INSTRUCTIONS.md` — subsystem order (stale at 20 today; live launch
  list is 20 registered + room in `MAX_SUBSYSTEMS 22`)

Date of snapshot: 2026-08-22

---

## Goals And Non-Goals

### Goals

1. **New subsystem** — MCP is a peer of Scripting / WebSocket / Mail Relay, not
   a Conduit endpoint and not a Webhooks hook.
2. **Own launch / landing** — dedicated readiness, plan, launch, land, review.
   Disabled-by-default clean skip so min/max configs keep working.
3. **Own status and statistics** — live counters on the subsystem and in
   `/api/system/info` + Prometheus. Optional JWT `GET /api/mcp/status`.
4. **Own config** — operator names the **listen interface** (bind + port + path)
   and the **Lua protocol script**. No tool names in C config.
5. **Lua owns the protocol** — initialize, tools, resources, prompts, ping, and
   future MCP methods live in Lua. Adding a tool is a migration that seeds a
   script with `mcp_access = 1`.
6. **Bearer at the edge, issuers already in-tree** — require
   `Authorization: Bearer` by default. Accept tokens from the issuers Hydrogen
   already operates: first-party Hydrogen JWT, Hydrogen OIDC IdP access
   tokens, and OIDC RP / Keycloak tokens. Claims injected as
   `params._hydrogen` (same rule as LUA_CLIENT: reject client-supplied
   `_hydrogen`). Advertise those issuers via RFC 9728 so Cursor / Claude /
   Copilot / custom agents can *find* how to get a token.
7. **Separate allowlist** — `scripts.mcp_access` is independent of
   `scripts.invokable`. A script may be REST-only, MCP-only, both, or neither.
8. **Non-blocking HTTP, bounded Lua** — MHD never blocks a pool thread on
   `scripting_wait_job`. Scripting workers own Lua (fresh `lua_State` per
   job). Concurrent MCP clients share those pools; they do not each get an
   unbounded “thread per connection.”

### Non-goals (this plan)

- Implementing product tools in C (Canvas, Stripe, mail, catalog, …).
- Uploading Lua source from an MCP client.
- stdio transport (Claude Desktop local spawn). Optional later.
- Full MCP OAuth 2.1 **authorization-server** work (Dynamic Client
  Registration, browser PKCE hosted by the MCP daemon). Hydrogen IdP and
  Keycloak already do that. v1 is **resource-server** discovery + token
  accept. DCR remains Phase 16.
- Sampling / elicitation / roots / GET+SSE progress until the Lua protocol
  script needs them. v1 GET on `Path` is **405** (spec-legal).
- Changing Conduit `/api/conduit/script` or the `invokable` column semantics.
- Attaching MCP as a path on the existing WebServer in v1 (Phase 0 may reverse
  this; default is a dedicated listen).
- Per-model API keys or a new identity store. Claude, Cursor, and a custom
  GPT agent are MCP *clients*; they present a Bearer from one of the
  configured issuers. C does not learn model names.

---

## Current Observed State (2026-08-22)

### Present and reusable

| Piece | Location / notes |
| --- | --- |
| Scripting subsystem | Launch/landing, workers, scoreboard, `scripting_submit_job` |
| Client script invoke | `POST/GET /api/conduit/script`, JWT, `H.set_result_json` |
| Allowlist pattern | `scripts.invokable` + QueryRef **#149** (`acuranzo_1297` / `1298`) |
| JWT helper | Conduit / mailrelay `extract_and_validate_jwt` (Hydrogen HS256) |
| OIDC IdP | `OIDC` — discovery, JWKS, authorize/token/PKCE, Test 45 |
| OIDC RP / Keycloak | `OIDC_RP.Providers[]` — JWKS + `oidc_rp_validate_id_token`, Test 42 |
| MHD | WebServer already uses internal poll + **thread pool** + `MHD_ALLOW_SUSPEND_RESUME` (copy this, not thread-per-connection) |
| Scoreboard cancel | `H.scoreboard.cancel` / kill-on-timeout already exist |
| Status machine | `status_core.h` `ServiceMetrics` union + `status_process.c` |
| Launch/landing | 20 registered subsystems in `launch_readiness.c` |
| Config letters | A–S taken; **T** is free (`Webhooks` is S under `SR_API`) |
| Highest migration | `acuranzo_1363.lua`; next **1364** |
| Highest QueryRef | **#150**; next **#151** (re-check at Phase 8) |

### Missing

| Gap | Impact |
| --- | --- |
| `src/mcp/` | No runtime |
| `config_mcp` / `AppConfig.mcp` | No config |
| `SR_MCP` / launch / landing | Not a subsystem |
| MCP transport | No Streamable HTTP / SSE / stdio |
| `scripts.mcp_access` | No MCP allowlist |
| `scripts.mcp_schema` / `mcp_annotations` | No way to advertise tool argument shapes to agents without hardcoding per-tool logic in `Mcp.Server` |
| `Origin` validation | No DNS-rebinding mitigation for a browser-originated Streamable HTTP client |
| Unauthenticated health path | No liveness/readiness probe target that doesn't require a JWT |
| RFC 9728 Protected Resource Metadata | MCP clients that speak OAuth discovery (Claude / Cursor / Copilot) will 401 and then look for `/.well-known/oauth-protected-resource`; nothing serves it today |
| Multi-issuer Bearer accept | `extract_and_validate_jwt` only understands Hydrogen HS256. OIDC IdP and Keycloak tokens are RS256 against a JWKS and will 401 unless Phase 5 grows an issuer chain |
| Session lifecycle | No `DELETE` termination, no unknown-session 404, no idle-session reaper — session table grows unbounded |
| MHD suspend/resume on MCP | Conduit `/script` currently **blocks** the MHD thread on `scripting_wait_job`. Copying that into MCP will stall the daemon under concurrent long tools |
| Nested `H.mcp.call` deadlock | If `Mcp.Server` waits on a *queued* tool job, every Scripting worker can sit in Protocol waiting for a tool that cannot start (`WorkerCount` defaults to **2**) |
| Status counters | Nothing to export |
| Blackbox | No test slot reserved in code yet; **47** is free. Use **1547x / 1548x**, not 547x |

### Live subsystem count (do not guess)

`globals.h` comments say “primary 22” and sets `MAX_SUBSYSTEMS 22` /
`INITIAL_REGISTRY_CAPACITY 22`. The **registered launch list** is 20 calls in
`launch_readiness.c` (Registry … Reporting). `SR_AUTH` and `SR_MIRAGE` are name
macros only. `ReadinessResults.results[MAX_SUBSYSTEMS]` must stay large enough
for every `process_subsystem_readiness` call.

Adding MCP = **21st registered subsystem**. Bump `MAX_SUBSYSTEMS` and
`INITIAL_REGISTRY_CAPACITY` to **24** (21 used + headroom) rather than landing
exactly on 21.

---

## Architecture

```text
MCP client (Claude / Cursor / custom)
        |
        |  Streamable HTTP  (JSON-RPC 2.0)
        |  Authorization: Bearer <Hydrogen JWT | IdP | Keycloak>
        v
+---------------------------+
| Hydrogen MCP subsystem    |   C: bind, Bearer (multi-issuer), PRM,
| listen Interface:Port/Path|       envelope, session, suspend, counters |
+-------------+-------------+
              |
              |  submit Protocol script (Group.Name)
              |  params = { rpc, _hydrogen }
              v
+---------------------------+
| Scripting workers (Lua)   |
| Mcp.Server                |   Lua: initialize, tools/list, tools/call, …
|   H.mcp.list / H.mcp.call |   only scripts with mcp_access=1
+---------------------------+
              |
              v
     Mcp.Echo, Catalog.*, …     (seeded Lua; no C change)
```

### Responsibility split (locked unless Phase 0 revises)

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C** | Enabled, Interface, Port, Path, Protocol script name, Bearer accept (Hydrogen JWT / OIDC IdP / OIDC RP), RFC 9728 PRM, JSON-RPC envelope (`jsonrpc` / `id` / `method` string), session id, MHD suspend/resume, `mcp_access`, counters | Tool names, MCP resource URIs, prompt names, MCP method semantics, product IDs, model/vendor names |
| **Lua `Mcp.Server`** | Full MCP method set, capability negotiation, tool/resource/prompt schemas | Listen sockets, JWT crypto, MHD |
| **Lua tools** | Business logic via `H.query` / `H.http` / `H.mail` | Transport, auth parsing |

### Why a subsystem (not a Conduit path)

- Own kill switch and listen surface.
- Own thread/session lifetime independent of `/api`.
- Own counters (sessions, RPC in/out, auth failures, Lua errors).
- MCP session semantics (initialize handshake, `Mcp-Session-Id`) do not map
  onto one-shot Conduit jobs.
- Config names **one** protocol script; Conduit names **any** invokable script.

Webhooks (config letter S) is the anti-pattern to avoid: HMAC ingress that
dispatches a fixed script, **not** a subsystem. MCP is closer to WebSocket.

---

## Design Principles (proposed lock — Phase 0 confirms)

1. **Ambivalent C** — C never `strcmp`s an MCP method to decide business
   behavior. It only validates the JSON-RPC envelope and hands the object to
   Lua.
2. **Config names the surface + the protocol script** — operator provides the
   interface being served (`Interface` / `Port` / `Path`) and `Protocol`
   (`Mcp.Server`). No tool list in JSON config.
3. **`mcp_access` is not `invokable`** — two gates, two QueryRefs, two
   existence-hiding 404s.
4. **Bearer default** — `RequireJWT=true` (name kept for config continuity;
   the header is still `Authorization: Bearer`). Disabled Bearer is a
   Phase 0-documented test-only escape, default off. Accept path is
   Hydrogen JWT and, when configured, OIDC IdP / OIDC RP tokens.
5. **Disabled by default** — clean skip (`ready=true`, not a No-Go) so
   `test_17` min/max and any “count the Go lines” checks stay stable.
6. **Dedicated listen** — second MHD daemon on `MCP.Port`, not a WebServer
   prefix. Independent enablement; no `/api` CORS/prefix coupling.
7. **Launch after Scripting** — MCP submits jobs. **Land MCP before
   Scripting** so the listener stops before workers drain.
8. **No `static` functions in `src/mcp/`** — Unity must link them. Header
   declarations for every callable. See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).
9. **No secrets in logs** — never log JWT, Authorization, or tool payloads
   except behind an explicit debug flag that tests do not enable.
10. **Origin validation, not just JWT** — Streamable HTTP is a plain HTTP
    listener; a malicious page loaded in a browser can still point
    `fetch()` at `http://localhost:3100/mcp` with a stolen or ambient
    cookie/token. Validate the `Origin` header against `MCP.AllowedOrigins`
    (see config below) before JWT parsing. Non-browser agents typically send
    no `Origin` header at all — absence is not itself a reject; only a
    **present-but-mismatched** `Origin` is. This is the transport's own
    "DNS rebinding" mitigation per the MCP Streamable HTTP spec and is
    independent of, and required in addition to, the JWT gate.
11. **Content blocks are a Lua helper, not hand-rolled JSON** — provide
    `H.mcp.text` / `H.mcp.image` / `H.mcp.audio` / `H.mcp.resource_link` so
    tool authors build spec-correct MCP content blocks instead of assembling
    `{type=..., text=...}` tables by hand in every tool script.
12. **Tool failure ≠ protocol failure** — a Lua tool that fails on bad
    arguments or a downstream error returns `result.isError = true` with an
    explanatory content block. It is **not** a JSON-RPC `error` object.
    JSON-RPC `error` is reserved for envelope/dispatch failures C detects
    itself (unknown method framing, malformed JSON-RPC, C-side timeout).
    Conflating the two breaks agent-side retry/backoff logic, which treats
    tool errors as "ask the model to try different arguments" and protocol
    errors as "something is wrong with the connection."
13. **Discoverable resource server, existing authorization servers** — C
    serves RFC 9728 Protected Resource Metadata and puts
    `resource_metadata` on every 401 `WWW-Authenticate`. It does **not**
    become an authorization server. `authorization_servers` lists
    `OIDC.Issuer` (when IdP enabled) and each `OIDC_RP.Providers[].Issuer`.
    Agents from different vendors share this one document; they do not
    each need a Hydrogen patch.
14. **No thread-per-MCP-client** — Streamable HTTP is one POST per
    JSON-RPC message, not a long-lived RPC socket. Optional GET+SSE is
    out of v1 (405). MHD uses the WebServer model: internal poll, fixed
    thread pool, `MHD_ALLOW_SUSPEND_RESUME`. Lua runs on Scripting
    workers. See [Concurrency And Threading](#concurrency-and-threading).
15. **`H.mcp.call` is inline** — the Protocol job must not enqueue a
    second Scripting job and wait on the same pool (deadlock when
    `WorkerCount` concurrent `tools/call`s arrive). `H.mcp.call` runs
    the tool in a child `lua_State` on the **same** worker.
    `H.mcp.call_async` remains a queued job for explicit fan-out.

### Proposed config shape

```json
{
  "MCP": {
    "Enabled": false,
    "Interface": "127.0.0.1",
    "Port": 3100,
    "Path": "/mcp",
    "Protocol": "Mcp.Server",
    "RequireJWT": true,
    "AcceptHydrogenJWT": true,
    "AcceptOidcIdP": false,
    "AcceptOidcRp": false,
    "Resource": null,
    "RequiredScopes": [],
    "Database": null,
    "MaxBodyBytes": 1048576,
    "MaxResultBytes": 262144,
    "RequestTimeoutSeconds": 30,
    "ThreadPoolSize": 4,
    "AllowedOrigins": [],
    "SessionIdleTimeoutSeconds": 900,
    "MaxSessions": 256
  }
}
```

| Field | Role |
| --- | --- |
| `Enabled` | Kill switch. Default `false` |
| `Interface` | Bind address. Default **`127.0.0.1`** (MCP Streamable HTTP spec: local servers SHOULD NOT bind `0.0.0.0`). Remote MCP sets this explicitly |
| `Port` | Dedicated listen port. Default `3100` |
| `Path` | HTTP path. Default `/mcp` |
| `Protocol` | `Group.Name` of the Lua protocol script. Required when enabled |
| `RequireJWT` | Default `true`. Name kept; means “Bearer required” |
| `AcceptHydrogenJWT` | Validate with `extract_and_validate_jwt`. Default `true` — Test 47 path |
| `AcceptOidcIdP` | Validate RS256 against Hydrogen IdP JWKS; `iss` must match `OIDC.Issuer`. Default `false` until IdP is on |
| `AcceptOidcRp` | Validate against each `OIDC_RP.Providers[]` JWKS (Keycloak etc.). Default `false` |
| `Resource` | Canonical MCP resource URI (RFC 8707 `aud` / RFC 9728 `resource`). Default derived as `http(s)://<advertise-host>:<Port><Path>` |
| `RequiredScopes` | Optional scope strings that must appear on an OIDC access token. Empty = no scope gate. Hydrogen user JWTs have no scopes and skip this check |
| `Database` | Optional Hydrogen DB name for script load; else Scripting default |
| `MaxBodyBytes` | JSON-RPC body cap |
| `MaxResultBytes` | Cap on a single tool result before `Mcp.Server` truncates and sets `isError` / a truncation notice. Default 256 KiB. Agents die on unbounded `H.query` dumps; this is the USB-C fuse |
| `RequestTimeoutSeconds` | Ceiling for one Protocol job (MHD connection stays suspended this long). Many MCP clients time out around 30–60s on their side regardless |
| `ThreadPoolSize` | MHD worker threads for the MCP daemon. Default `4`. **Not** “one thread per client” |
| `AllowedOrigins` | Exact-match allowlist for a **present** `Origin` header. Empty = only same-origin/no-`Origin` traffic allowed; browser clients need an explicit entry |
| `SessionIdleTimeoutSeconds` | Server-side reaper interval for the session→subject binding table; prevents unbounded growth from abandoned sessions. Default `900` |
| `MaxSessions` | Hard cap on concurrent session entries; new `initialize` beyond the cap is rejected with a JSON-RPC error rather than growing unbounded |

`Interface` is the “main interface being served.” `Protocol` is the Lua
implementation of that interface. Both are required when `Enabled` is true.

A fixed, unauthenticated `GET <Path>/healthz` on the **same MHD daemon**
(no JWT, no `Origin` check, no JSON-RPC) returns `200 {"status":"ok"}` when
the listener thread is alive, purely for container/load-balancer liveness
and readiness probes. This is distinct from Phase 11's authenticated
`GET /api/mcp/status` on the main API port, which carries counters and
requires JWT. Conflating the two would force infra probes to hold a JWT.

### Proposed Lua contract

Protocol script receives:

```lua
-- params (from C)
{
  jsonrpc = "2.0",
  id = 1,                 -- or string; notify => nil
  method = "tools/list",
  params = { ... },       -- MCP params object (may be nil)
  _hydrogen = {           -- injected; client copy rejected
    sub = "...",
    iss = "...",          -- set for OIDC tokens; may be nil for Hydrogen JWT
    roles = "...",
    scopes = { ... },     -- OIDC only; empty for Hydrogen JWT
    database = "...",
    session_id = "...",
    protocol_version = "2025-06-18",  -- from MCP-Protocol-Version header
    auth_kind = "hydrogen_jwt",       -- or oidc_idp / oidc_rp
  },
}
```

Protocol script returns via `H.set_result_json`:

```lua
-- success
{ jsonrpc = "2.0", id = 1, result = { ... } }

-- protocol error (still HTTP 200 for JSON-RPC)
{ jsonrpc = "2.0", id = 1, error = { code = -32601, message = "Method not found" } }
```

C maps:

| Condition | HTTP | Body |
| --- | --- | --- |
| Bad JWT | 401 | no JSON-RPC leak of script names |
| Body too large / invalid JSON | 400 | JSON-RPC parse error `-32700` if possible |
| Protocol script missing / no `mcp_access` | 404 | existence-hiding; same as unknown |
| Lua success / protocol error object | 200 | Lua result as-is (must be JSON-RPC) |
| Lua crash / timeout | 200 | JSON-RPC `-32603` Internal error |

### Proposed `H.mcp` host API

Thin. This is the only C MCP knowledge beyond transport:

| Function | Behavior |
| --- | --- |
| `H.mcp.list(cursor?, page_size?)` | Rows with `mcp_access <> 0` (name, group, summary, schema, annotations). Not `invokable`. Returns `rows, next_cursor` for large tool sets |
| `H.mcp.call(name, args)` | Load + run that script only if `mcp_access <> 0`. Inject `_hydrogen`. **Inline:** child `lua_State` on the same Scripting worker, not a nested scoreboard job. Blocks *this* Lua job until the tool returns or the job is killed |
| `H.mcp.call_async(name, args)` | Same access check, but **queues** a scoreboard job and returns a handle for `H.wait`. Fan-out only. Never wait on more in-flight MCP jobs than `WorkerCount - 1` |
| `H.mcp.text(str)` | Build `{type="text", text=str}` |
| `H.mcp.image(data_b64, mime_type)` | Build `{type="image", data=data_b64, mimeType=mime_type}` |
| `H.mcp.audio(data_b64, mime_type)` | Build `{type="audio", data=data_b64, mimeType=mime_type}` |
| `H.mcp.resource_link(uri, name, mime_type?)` | Build a `resource_link` content block |
| `H.mcp.tool_error(content_blocks)` | Build `{isError=true, content=content_blocks}` — the correct shape for a **tool** failure (see Design Principle 12); never construct this by hand |

`Mcp.Server` implements `tools/list` by calling `H.mcp.list` and shaping MCP
tool descriptors in Lua. New tools never touch C. The content-block and
tool-error helpers exist so no tool script — including product tools seeded
later in Helium migrations — has to know the literal MCP JSON shapes;
getting a content block wrong (e.g. `text` vs `data`, missing `mimeType`) is
a client-visible protocol violation, not a Lua runtime error, so it is easy
to ship silently broken.

#### Closing the tool-schema gap

MCP's `tools/list` response requires each tool to carry a JSON Schema
`inputSchema` (and may carry `outputSchema` and `annotations` such as
`readOnlyHint` / `destructiveHint` / `idempotentHint` / `openWorldHint`) so
that MCP clients — and the LLM agents behind them — know what arguments a
tool takes and how it behaves *before* calling it. The `name, group, summary`
shape above has nowhere to source that from without hardcoding a per-tool
`if/elseif` inside `Mcp.Server`, which would silently reintroduce "adding a
tool touches a shared file" — the same anti-pattern this plan avoids for C.

Recommendation (confirm at Phase 0, land at Phase 8 alongside `mcp_access`):
add two more nullable `scripts` columns, populated by the tool's own seed
migration, not by `Mcp.Server`:

| Column | Type | Content |
| --- | --- | --- |
| `mcp_schema` | JSON text, nullable | `{ "inputSchema": {...}, "outputSchema": {...} }`. `NULL` → `Mcp.Server` falls back to a permissive `{"type":"object"}` schema so an old tool without one still lists, just without argument guidance |
| `mcp_annotations` | JSON text, nullable | `{ "title": "...", "readOnlyHint": true, ... }`. `NULL` → omit `annotations` (spec-legal; client just gets no hints) |

This keeps schema authorship where the tool's business logic already lives
(the migration that seeds the script) and keeps `Mcp.Server` a generic
shaper: `select * from mcp-allowlisted scripts -> JSON-decode mcp_schema /
mcp_annotations -> emit`. No Hydrogen release is needed to add a tool with a
rich schema, matching Goal 5.

---

## Auth Discovery And Multi-Issuer Tokens

MCP clients from different vendors (Claude, Cursor, Copilot, custom agents)
do **not** each need a Hydrogen auth scheme. They need (1) a way to
**discover** where to get a token and (2) a Bearer Hydrogen will **accept**.
Hydrogen already has three issuers. v1 is the resource-server glue; it is
not a fourth IdP.

### What v1 ships (resource server)

On the **MCP daemon** (not WebServer):

| Surface | Behavior |
| --- | --- |
| `401` + `WWW-Authenticate` | `Bearer realm="hydrogen-mcp", resource_metadata="<absolute PRM URL>"` on every failed/missing token. Spec-mandated; without it, discovery-capable clients give up or probe the wrong host |
| `GET /.well-known/oauth-protected-resource` | RFC 9728 JSON. Also serve the path-scoped variant `/.well-known/oauth-protected-resource<Path>` (clients try both). No JWT, no Origin check (same class as `/healthz`) |
| `authorization_servers` | Built at launch from live config: `OIDC.Issuer` when `OIDC.Enabled && AcceptOidcIdP`; each `OIDC_RP.Providers[].Issuer` when `AcceptOidcRp`. Hydrogen user-JWT has no AS URL — clients that cannot do OAuth still paste a login JWT as Bearer |
| `resource` | `MCP.Resource` (canonical URI). Tokens that carry `aud` must include it. Hydrogen HS256 user JWTs historically have no `aud`; accept them only when `AcceptHydrogenJWT` is true |

C still does not speak MCP methods. PRM is transport metadata, same as
`/healthz`.

### What v1 accepts (token chain)

Phase 5 validator, first match wins, all failures increment `auth_rejected`
with a **reason code** (`missing`, `malformed`, `hydrogen_jwt`, `oidc_idp`,
`oidc_rp`, `aud`, `scope`) so Test 47 and Prometheus can tell them apart
without logging the token:

1. **Hydrogen JWT** (`AcceptHydrogenJWT`) — existing
   `extract_and_validate_jwt`. This is Test 40 / Test 46 / Test 47's
   `POST /api/auth/login` path. Inject `sub`, `roles`, `database`.
2. **Hydrogen OIDC IdP** (`AcceptOidcIdP`) — RS256 against IdP JWKS,
   `iss == OIDC.Issuer`, `aud` contains `MCP.Resource`, optional
   `RequiredScopes`. Inject `sub`, `iss`, scopes as `roles` (or a
   dedicated `_hydrogen.scopes` field — pick one in Phase 0 and stick).
3. **OIDC RP / Keycloak** (`AcceptOidcRp`) — same shape against each
   configured provider JWKS. Reuse `oidc_rp` JWKS fetch, do **not**
   invent a third JWKS client. Map `sub` + `iss`; do not invent a
   Hydrogen account if none is linked. Lua tools that need an Acuranzo
   user look it up themselves.

No token passthrough to downstream APIs (MCP spec / confused-deputy).
Tools that call Stripe or a second MCP server use Hydrogen's own
credentials (`H.http` secrets, not the inbound Bearer).

### What stays Phase 16

- Dynamic Client Registration on Hydrogen IdP (`/oauth/register` is a stub).
- Hosting the authorization **code** flow on the MCP port.
- Per-MCP-client client_id provisioning UI.

Operators who want Claude/Cursor to click-through OAuth enable Hydrogen
IdP **or** point PRM at Keycloak, and register those MCP clients on that
AS the same way they register Lithium. MCP C does not grow an authorize
endpoint.

### Agent-side provisioning (no C change)

| Client | v1 recipe |
| --- | --- |
| Cursor / Claude remote MCP | URL `http://127.0.0.1:3100/mcp`. If the client implements RFC 9728 it follows `WWW-Authenticate` → PRM → IdP/Keycloak. If it only supports a static Bearer, paste a Hydrogen user JWT (or a Keycloak access token once `AcceptOidcRp` is on) |
| CI / blackbox / `mcp_probe.sh` | `POST /api/auth/login` then `Authorization: Bearer` — same as Test 46 |
| Several models at once | They are concurrent HTTP clients. Each presents its own Bearer. Session table keys on `Mcp-Session-Id` **bound to `sub`** (already a threat note). No per-model config |

Phase 14 docs include a `mcp.json` snippet and a Keycloak client checklist
(redirect `http://127.0.0.1:*` / PKCE public client / audience = `MCP.Resource`).

---

## Concurrency And Threading

Scripting already solves “run Lua without blocking the world.” MCP must
not undo that by waiting on the MHD thread or by queueing a tool job
behind the Protocol job that is waiting for it.

### What MCP connections actually are

Streamable HTTP is **not** a long-lived RPC socket:

- Each client JSON-RPC message is a **new HTTP POST**.
- A session is a header (`Mcp-Session-Id`) plus a C-side
  session→subject table, not a dedicated thread.
- Optional `GET Path` + SSE is how the spec does server-push. v1
  returns **405** (explicitly allowed). Do not implement GET+SSE until
  a Lua tool needs progress/`listChanged`.
- “One thread per MCP client connection” is the wrong model. It unbounded-
  grows under many short POSTs and under abandoned SSE clients, and it
  still deadlocks if those threads wait on the same Scripting pool.

### Recommended MHD model (copy WebServer)

```text
MCP client POST
    → MHD thread-pool worker (MCP.ThreadPoolSize)
        → Origin + Bearer + envelope (short, stays on MHD thread)
        → scripting_submit_job(Protocol)          // enqueue only
        → MHD_suspend_connection
              Scripting worker: fresh lua_State, run Mcp.Server
                  tools/call → H.mcp.call → child lua_State (inline)
              scoreboard terminal / timeout / cancel
        → MHD_resume_connection
        → write JSON-RPC body, close POST
```

- Flags: `MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SELECT_INTERNALLY |
  MHD_ALLOW_SUSPEND_RESUME` (same as
  [`web_server_core.c`](/elements/001-hydrogen/hydrogen/src/webserver/web_server_core.c)).
- Do **not** copy Conduit `/api/conduit/script`, which currently blocks
  the WebServer MHD thread on `scripting_wait_job`. That is acceptable
  for a rarely-concurrent SPA invoke; it is not acceptable for N agents
  holding 30s tools.
- Notifications (`id` absent) return **202** with no body after a
  fire-and-forget submit, or after a C-side no-op if we later special-
  case `notifications/*` — but C still must not `strcmp` method for
  *business* behavior. Envelope `id` presence is enough.
- Launch No-Go if `Scripting.WorkerCount < 2` when MCP is enabled:
  a `notifications/cancelled` POST must be able to run `Mcp.Server`
  while a long `tools/call` occupies the other worker.

### Nested-Lua deadlock (the real footgun)

`WorkerCount` defaults to **2**. If `H.mcp.call` is implemented as
“submit tool job + `scripting_wait_job`”:

1. Two concurrent `tools/call` POSTs take both workers running `Mcp.Server`.
2. Each waits for a tool job that cannot start.
3. `RequestTimeoutSeconds` fires, both fail. Looks like a flake.

**Lock:** `H.mcp.call` loads the allowlisted source (MCP QueryRef) and
runs it in a **child `lua_State` on the same worker**, then destroys
that state. The worker stays busy (the tool *is* the work) but nothing
is queued behind itself. Fresh-state-per-job is preserved: the child is
not a reused Protocol state.

`H.mcp.call_async` + `H.wait` remains the explicit fan-out path. Docs
must say: do not `H.wait` on more in-flight MCP jobs than
`WorkerCount - 1`. Unity covers the inline path; blackbox covers two
overlapping Echo calls with `WorkerCount=2` (must not deadlock).

### Long-running tools

| Concern | v1 behavior |
| --- | --- |
| HTTP thread | Suspended; pool stays available for `/healthz`, `initialize`, cancel |
| Scripting worker | Occupied until the tool (or Protocol) returns |
| Timeout | `RequestTimeoutSeconds` → scoreboard kill → JSON-RPC `-32603`; increment `dispatch_timeouts` |
| Cancel | Client POST `notifications/cancelled` (202). `Mcp.Server` maps it to `H.scoreboard.cancel` / a per-session cancel flag that the inline `H.mcp.call` polls. Needs `WorkerCount >= 2` |
| Progress | Not in v1. No GET+SSE, no POST SSE. A long tool is silent until it returns. Phase 17 if a product tool needs it |
| Landing | Stop accept → fail in-flight waits with SHUTDOWN → join MHD pool → land Scripting |

### Boundedness

Concurrent in-flight RPCs ≤ `min(MCP.ThreadPoolSize, Scripting.WorkerCount,
MCP.MaxSessions)`. Export `rpc_in_flight` next to `sessions_active`.
Reject new POST with JSON-RPC `-32000` (or HTTP 429 — Phase 0 pick one;
recommend JSON-RPC so the agent retries the RPC, not the transport)
when `rpc_in_flight` would exceed `WorkerCount`.

---

## Exposing Hydrogen To Any Model

The protocol adapter is not the product. An agent that can `initialize` and
call `Mcp.Echo` has **not** been given Hydrogen. Hydrogen's power is named
QueryRefs, seven engines, mail, HTTP, scripting, and the existing JWT/OIDC
identity. MCP is only how a model reaches those.

### What "any model" actually means

| Client class | Talks | v1 works? |
| --- | --- | --- |
| Cursor / VS Code / custom `mcp.json` | Streamable HTTP + static Bearer | **Yes** — paste Hydrogen JWT; `extras/mcp_probe.sh` path |
| Claude / Copilot remote MCP with RFC 9728 | HTTP + 401 discovery + OAuth | **Yes, if** IdP or Keycloak is on and the client is pre-registered there |
| Same clients, expecting Dynamic Client Registration | HTTP + DCR | **No** until Phase 16 / IdP `/oauth/register` |
| Claude Desktop **local** spawn | stdio | **No** — Phase 17. Remote HTTP URL is the v1 path |
| ChatGPT / other hosted connectors | often OAuth + HTTPS only | Works only behind TLS with `MCP.Resource` = public `https://…/mcp` |

Do not claim "any model" in operator docs. Claim: any MCP client that can
POST Streamable HTTP with a Bearer from a configured issuer. That is the
real USB-C port. stdio and DCR are adapters, not the port.

`MCP.Resource` must be the **external** URL clients put in `aud` (same rule
as `OIDC.Issuer`). Behind a TLS terminator it is `https://…/mcp`, not
`http://127.0.0.1:3100/mcp`.

### initialize.instructions (do this in v1)

`InitializeResult.instructions` is how every serious MCP client tells the
model what this server *is*. Without it, the model sees a bag of tool names
and guesses. `Mcp.Server` must return a short, seeded instruction string
(Lua, not C): what Hydrogen is, that tools are named `Group.Name`, that
writes are explicit, that results may be truncated, that `_hydrogen` is
server-injected. Updating the blurb is a migration, not a Hydrogen release.

Phase 15 prompts/resources can replace this later. Shipping tools without
instructions is how agents misuse `H.query`.

### Tool policy (Helium, not `src/mcp/`)

Hydrogen v1 seeds **fixtures only** (`Mcp.Echo`, `Mcp.EchoStrict`,
`Mcp.Sleep`). Product power is Helium seeds. Rules so the first real tools
do not become a security incident:

1. **No generic SQL tool.** No `query(sql)`. Every data tool names a
   QueryRef (or a small allowlisted set) and typed arguments. Same rule as
   `api_no_business_sql`.
2. **No inbound-token passthrough** to `H.http` / Stripe / another MCP
   (already a threat note).
3. **Read vs write** via `mcp_annotations` (`readOnlyHint` /
   `destructiveHint`). Destructive tools stay off the default seed list
   until an operator opts in (`mcp_access=1` on that row).
4. **Honor `_hydrogen.database`.** A JWT bound to one DB does not run
   QueryRefs on another.
5. **Cap results** at `MaxResultBytes`. Prefer pagination (`cursor`) over
   dumping tables into the model context.
6. **`H.llm` from a tool is opt-in and labeled.** Agent → Hydrogen →
   another model is a loop unless the tool's schema says so.
7. **Audit without payloads:** log `sub`, `auth_kind`, tool name, bytes,
   duration, success/isError. Never log arguments or results.

A recommended first Helium catalog (not this plan's C work):
`Query.Run` (named QueryRef + params), `System.Info` (already-public
status shape), maybe `Mail.Send` behind a destructive flag. That is what
makes "any model" useful. Echo is how we know the plug works.

---

## Subsystem Count Touchpoints

Treat this as a checklist inside Phase 2. Missing one of these is how
Reporting/Scripting additions broke tests last time.

### Must change together

| File | What |
| --- | --- |
| `src/globals.h` | `SR_MCP "MCP"`; comment “primary N”; `MAX_SUBSYSTEMS` 22 → **24**; `INITIAL_REGISTRY_CAPACITY` 22 → **24** |
| `src/hydrogen.h` | `MCPConfig mcp;` letter **T** |
| `src/config/config.c` | `LOAD_CONFIG("T", SR_MCP, load_mcp_config)` |
| `src/config/config_defaults.c` | `initialize_config_defaults_mcp()` — disabled |
| `src/launch/launch.h` | `check_mcp_launch_readiness` / `launch_mcp_subsystem` |
| `src/launch/launch_readiness.c` | 21st `process_subsystem_readiness(..., SR_MCP, ...)` after Reporting |
| `src/launch/launch.c` | `else if (strcmp(subsystem, SR_MCP) == 0)` dispatch |
| `src/landing/landing.h` | `check_mcp_landing_readiness` / `land_mcp_subsystem` |
| `src/landing/landing_readiness.c` | extern + table entry |
| `src/landing/landing.c` | `strcmp` → `land_mcp_subsystem` |
| `src/state/state.c` / `state.h` | `mcp_system_shutdown`, `ServiceThreads mcp_threads` |
| `src/threads/threads.h` | extern `mcp_threads` |
| `src/registry/registry_integration.h` | decls if that file lists subsystems |
| `src/status/status_core.h` | `ServiceMetrics mcp` + union arm |
| `src/status/status_process.c` | collect MCP counters |
| `src/logging/` defaults | optional per-subsystem log level name `MCP` |
| `docs/H/INSTRUCTIONS.md` | subsystem order 21. MCP; config letter T |

### Tests that historically hard-code counts

Search before landing Phase 2 (do not assume this list is complete):

- `tests/unity/src/registry/registry_test_core_functions.c` — comment “21 total
  / 22 deps” (may be a synthetic graph, not the live list — verify)
- Any Unity test that fills `ReadinessResults.results[MAX_SUBSYSTEMS]`
- `test_17_startup_shutdown.sh` — extra launch/land log lines when MCP is
  disabled must still pass (clean skip)
- `test_21_system_endpoints.sh` — `/api/system/info` shape if it enumerates
  services
- Logging config tests if they snapshot subsystem name lists

### Launch / landing order (target)

Launch (after dependencies ready):

1. … existing …
2. Scripting
3. Reporting
4. **MCP** (needs Scripting + Network; Database if Protocol is DB-backed)

Landing (reverse of consumers):

1. **MCP** (stop accept, drain in-flight RPC)
2. Reporting
3. Scripting
4. …

---

## Reference Conventions

Match these exactly so cppcheck and the launch machine stay green.

### Source layout

Follow `src/mdns/` / `src/mailrelay/`:

- `src/mcp/mcp.h` — public
- `src/mcp/mcp_internal.h` — shared internals
- `src/mcp/mcp.c` — lifecycle
- `src/mcp/mcp_http.c` — MHD daemon + request + suspend/resume
- `src/mcp/mcp_rpc.c` — JSON-RPC envelope only
- `src/mcp/mcp_auth.c` — Bearer chain + `WWW-Authenticate`
- `src/mcp/mcp_prm.c` — RFC 9728 document build
- `src/mcp/mcp_session.c` — session→subject + reaper
- `src/mcp/mcp_dispatch.c` — submit Protocol script + resume
- `src/mcp/mcp_stats.c` — atomics / snapshot
- `src/mcp/mcp_shutdown.c` — drain

Every `.c` starts with `#include <src/hydrogen.h>`. Includes use
`<src/folder/...>`. No `static` callables. No `goto`.

### Launch / landing

Copy `launch_scripting.c` / `landing_scripting.c` (register in readiness,
Database+Scripting+Network dependencies, worker/port validation) and
`launch_mail_relay.c` message style (`Go:` / `No-Go:` / `Decide:`).

Disabled: **clean skip** (`ready=true`) like Reporting/Mail Relay Phase 1, not
Scripting’s `ready=false` disabled No-Go.

### API / JWT

Reuse `extract_and_validate_jwt` / conduit `_hydrogen` inject for Hydrogen
user JWTs. OIDC IdP / RP accept reuses existing JWKS helpers. Do not invent
a fourth JWT parser.

### Migrations

Next files: `acuranzo_1364.lua` onward. QueryRefs start at **#151** after a
live max check. `scripts.mcp_access` is `INTEGER_SMALL NOT NULL DEFAULT 0`.
`scripts.mcp_schema` / `scripts.mcp_annotations` are nullable JSON text
(see "Closing the tool-schema gap"). Load QueryRef must require
`mcp_access <> 0` (mirror #149). Seed `Mcp.Server`, `Mcp.Echo`, and
`Mcp.EchoStrict` with `mcp_access=1`; leave `invokable=0` unless a test needs
both.

### Blackbox

- Slot: `tests/test_47_mcp.sh` (40s API band; **47 is free**).
- Ports: **15470–15476** WebServer (login + `/api/mcp/status`),
  **15480–15486** MCP daemon. Same `15<TT>x` convention as Test 43 / 46
  (below the Linux ephemeral range). Do **not** use 547x.
- Configs: `tests/configs/hydrogen_test_47_*.json`. Enable Scripting
  with `WorkerCount >= 2`, API JWT, MCP.
- Docs: `/docs/H/tests/test_47_mcp.md`.
- Create the script only in Phase 13 (project rule: no speculative test files).

### Unity coverage inventory

Mirror `src/` as `tests/unity/src/<dir>/<source>_test_<function>.c`.
Auto-discovered; `mku <base>`. Header-only includes. Do not redefine
`USE_MOCK_LOGGING` / `USE_MOCK_LIBMICROHTTPD` / `USE_MOCK_SYSTEM`.

Minimum files (add more in Phase 12 if `add_coverage.sh` still flags
safe functions):

| Source | Unity file (one function each, split if large) | Must cover |
| --- | --- | --- |
| `config_mcp.c` | `config/config_mcp_test_load_mcp_config.c` | defaults, full custom, missing section, bad port, `Interface` localhost vs `0.0.0.0`, cleanup NULL |
| `launch_mcp.c` | `launch/launch_mcp_test_check_mcp_launch_readiness.c` | disabled skip, enabled deps, missing Protocol, `WorkerCount < 2` No-Go, port-in-use |
| `landing_mcp.c` | `landing/landing_mcp_test_land_mcp_subsystem.c` | not-running, drain timeout |
| `mcp_http.c` | `mcp/mcp_http_test_handle_request.c` | path/method, `/healthz`, PRM well-known, Origin allow/deny/absent, 405 GET |
| `mcp_auth.c` | `mcp/mcp_auth_test_validate_bearer.c` | missing, bad Hydrogen JWT, reserved `_hydrogen`, IdP/RP reject, `aud`/`scope`, `WWW-Authenticate` shape |
| `mcp_rpc.c` | `mcp/mcp_rpc_test_parse_envelope.c` | parse error, notify vs request, batch reject, oversize, protocol-version header passthrough |
| `mcp_session.c` (if split) | `mcp/mcp_session_test_bind.c` | generate, bind subject, hijack other `sub`, unknown 404, DELETE, reaper, `MaxSessions` |
| `mcp_dispatch.c` | `mcp/mcp_dispatch_test_submit_protocol.c` | submit+suspend seam, timeout → `-32603`, shutdown mid-wait, `rpc_in_flight` cap |
| `mcp_stats.c` | `mcp/mcp_stats_test_collect_metrics.c` | increment/snapshot/reset, reason-coded `auth_rejected` |
| `scripting_api_mcp.c` | `scripting/scripting_api_test_mcp_call.c` (etc.) | list/pagination/schema, call denied, **inline** call ok, call_async + wait, reserved key |

Review [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) and the
[mocks README](/elements/001-hydrogen/hydrogen/tests/unity/mocks/README.md)
before deciding something “cannot be mocked.” Reset mocks in
`setUp`/`tearDown`.

### Developer ergonomics: a manual probe script

Following the `extras/add_coverage.sh` convention (a small, always-available
helper that is not itself a test), add `extras/mcp_probe.sh` in Phase 13
alongside the blackbox script: a thin `curl` + `jq` wrapper that takes a base
URL and a JWT and runs PRM GET → unauthenticated 401 (print
`WWW-Authenticate`) → `initialize` → `tools/list` → `tools/call <name>
<args json>`, pretty-printing the JSON-RPC exchange. Optional
`--login` against WebServer `/api/auth/login` so a developer does not
mint a JWT by hand. This is for iterating on a new Lua tool without
standing up `test_47_mcp.sh`'s full matrix or the Inspector. Must pass
`mks` like every other `extras/*.sh` script.

---

## Gate Philosophy

- One logical behavior per item where practical.
- After every C change: `mkt`. Then `mka` once `mkt` is clean. Then `mkp`.
- After every Bash change: `mks`.
- After every Lua change: `test_98_luacheck.sh`.
- After every new/changed markdown file: `test_04_check_links.sh` and
  `test_90_markdownlint.sh`.
- Never mark a phase complete with a failing or skipped gate unless explicitly
  deferred with rationale.

---

## Phase Groups

| Group | Phases | Theme |
| --- | --- | --- |
| A | 0 | Design lock |
| B | 1–3 | Config, subsystem skeleton, status counters (no protocol yet) |
| C | 4–6 | Listen interface, JWT, JSON-RPC envelope |
| D | 7–10 | Lua protocol dispatch, `mcp_access`, `H.mcp`, seeds |
| E | 11–14 | Status API, Unity sweep, blackbox, docs |
| F | 15+ | Optional: resources/prompts, OAuth 2.1, stdio, multi-server |

---

## Phases

### Phase 0: Design Lock

- **Goal:** Agree transport, config, auth, allowlist, and C/Lua split before
  any C.
- **Dependencies:** None.
- **Entry gate:** This document exists.

#### Work items

- [ ] **0.1 Transport** — lock Streamable HTTP on a dedicated MHD port vs
      attach-to-WebServer vs both.
- [ ] **0.2 Config schema** — lock field names, defaults, `Interface` meaning.
- [ ] **0.3 Auth** — Bearer required; Hydrogen JWT + optional OIDC IdP /
      OIDC RP accept; PRM + `WWW-Authenticate`; claim inject; 401 shape.
- [ ] **0.4 Allowlist** — `mcp_access` column + existence-hiding 404.
- [ ] **0.5 Lua contract** — params / result / `H.mcp.list` / inline
      `H.mcp.call` / `H.mcp.call_async`.
- [ ] **0.6 Disabled semantics** — clean skip vs No-Go.
- [ ] **0.7 Out of scope** — confirm non-goals (stdio, DCR, product tools,
      GET+SSE).
- [ ] **0.8 Count impact** — confirm 21st registered subsystem and the
      touchpoint table above.
- [ ] **0.9 Concurrency** — lock MHD thread-pool + suspend/resume; reject
      thread-per-connection; lock inline `H.mcp.call`; `WorkerCount >= 2`.
- [ ] **0.10 Overload code** — JSON-RPC `-32000` vs HTTP 429 when
      `rpc_in_flight` would exceed `WorkerCount`.
- [ ] **0.11 Agent surface** — lock `initialize.instructions`, no generic
      SQL, `MaxResultBytes`, `MCP.Resource` = public URL, client
      compatibility matrix (HTTP+Bearer = v1; stdio/DCR = later).

#### Exit gate / validation

Locked-decisions table below is filled. No C required. Review stop.

#### Locked decisions (Phase 0)

| Topic | Decision |
| --- | --- |
| Transport | *pending* (recommendation: dedicated Streamable HTTP) |
| Config letter | *pending* (recommendation: T) |
| Interface | *pending* (recommendation: bind address) |
| Protocol script | *pending* (recommendation: `Mcp.Server`) |
| Auth | *pending* (recommendation: Bearer always; accept Hydrogen JWT by default; optional OIDC IdP + OIDC RP / Keycloak; PRM + `WWW-Authenticate` in v1; DCR stays Phase 16) |
| Allowlist | *pending* (recommendation: `scripts.mcp_access` DEFAULT 0) |
| Disabled | *pending* (recommendation: clean skip `ready=true`) |
| Test slot | *pending* (recommendation: 47 / WebServer 1547x + MCP 1548x) |
| Tool schema storage | *pending* (recommendation: nullable `scripts.mcp_schema` / `mcp_annotations` JSON columns, seeded per tool — see "Closing the tool-schema gap") |
| Origin validation | *pending* (recommendation: `MCP.AllowedOrigins` allowlist, reject a present-but-mismatched `Origin` before JWT) |
| Health endpoint | *pending* (recommendation: unauthenticated `GET <Path>/healthz` on the MCP port, separate from Phase 11's authenticated `/api/mcp/status`) |
| Session lifecycle | *pending* (recommendation: `DELETE` termination, 404 on unknown session id, idle reaper via `SessionIdleTimeoutSeconds` / `MaxSessions`) |
| Tool-error shape | *pending* (recommendation: `result.isError=true` content block for tool failures; JSON-RPC `error` reserved for envelope/dispatch failures only) |
| Bind default | *pending* (recommendation: `127.0.0.1`, not `0.0.0.0`) |
| MHD model | *pending* (recommendation: WebServer thread pool + `MHD_ALLOW_SUSPEND_RESUME`; not thread-per-connection; GET+SSE = 405 in v1) |
| `H.mcp.call` | *pending* (recommendation: inline child `lua_State`; `call_async` for fan-out) |
| Overload | *pending* (recommendation: JSON-RPC `-32000` when `rpc_in_flight` would exceed `WorkerCount`) |
| Agent surface | *pending* (recommendation: `initialize.instructions` in v1; no generic SQL tool; `MaxResultBytes` 256 KiB; `Resource` is the public URL) |

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 1: Config Section

- **Goal:** `MCP` JSON section loads, dumps, cleans up, defaults disabled.
- **Dependencies:** Phase 0.
- **Entry gate:** Phase 0 Status complete.

#### Work items

- [ ] **1.1** `config_mcp.h` / `config_mcp.c` — `MCPConfig`, load/dump/cleanup.
      Dump must not print secrets (none expected; still mask if any appear).
- [ ] **1.2** `AppConfig.mcp` letter T; `LOAD_CONFIG("T", ...)`.
- [ ] **1.3** `initialize_config_defaults_mcp()` — `Enabled=false`,
      `Interface=127.0.0.1`, `Port=3100`, `Path=/mcp`, `Protocol=NULL`,
      `RequireJWT=true`, `AcceptHydrogenJWT=true`, `AcceptOidcIdP=false`,
      `AcceptOidcRp=false`, `ThreadPoolSize=4`.
- [ ] **1.4** Env-var overrides if other subsystems do them for Port/Interface.
- [ ] **1.5** Unity: load defaults, full custom, missing section, invalid port,
      cleanup NULL / partial, dump smoke.
- [ ] **1.6** `mkt` + `mkp` + named `mku config_mcp_test_*`.

#### Exit gate / validation

Disabled-by-default. No listen yet. Existing `test_12` / `test_15` still pass
if they iterate config sections — update those only if they break.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 2: Subsystem Skeleton (the count bump)

- **Goal:** MCP is a registered subsystem with launch/landing that does
  nothing but state + skip-when-disabled.
- **Dependencies:** Phase 1.
- **Entry gate:** Config loads in Unity.

#### Work items

- [ ] **2.1** Apply the **Subsystem Count Touchpoints** table in one change
      set. Do not land `SR_MCP` without the `MAX_SUBSYSTEMS` bump.
- [ ] **2.2** `launch_mcp.c` — register, Network+Scripting deps when enabled,
      validate Interface/Port/Path/Protocol when enabled, clean skip when not.
      Enabled + `Scripting.WorkerCount < 2` is a No-Go. Log an ALERT if
      `Interface` is `0.0.0.0` / `::` (spec prefers localhost).
- [ ] **2.3** `landing_mcp.c` — ready only if running; land sets shutdown flag.
- [ ] **2.4** `src/mcp/mcp.c` — `mcp_init_state` / `mcp_shutdown` no-op drain.
- [ ] **2.5** Unity: readiness enabled/disabled/null config/bad port/missing
      Protocol; launch success; land when not running.
- [ ] **2.6** `mkt` + `mka` + `mkp`.
- [ ] **2.7** `test_17_startup_shutdown.sh` — confirm min/max still start/stop.
      Fix any count assertions.
- [ ] **2.8** Grep Unity/blackbox for leftover `22` / “primary 22” / hardcoded
      readiness lengths and update.

#### Exit gate / validation

`test_17` green. Trial build green. Disabled MCP does not open a port.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 3: Status And Statistics Scaffold

- **Goal:** Counters exist and appear in system status even when all zeros.
- **Dependencies:** Phase 2.
- **Entry gate:** Subsystem registers.

#### Work items

- [ ] **3.1** `mcp_stat` — atomics: `sessions_active`, `sessions_total`,
      `sessions_expired`, `rpc_received`, `rpc_succeeded`, `rpc_failed`,
      `rpc_in_flight`, `auth_rejected` (plus reason counters or a small
      reason enum), `origin_rejected`, `dispatch_timeouts`, `bytes_in`,
      `bytes_out`, `last_rpc_at`.
- [ ] **3.2** `mcp_collect_metrics` → `ServiceMetrics` union arm `mcp`.
- [ ] **3.3** Wire `status_process.c` + JSON field `mcp` + Prometheus names
      `hydrogen_mcp_*`.
- [ ] **3.4** Unity: increment/snapshot/reset; JSON contains keys when enabled
      or always-present zeros (Phase 0 pick one; recommend always-present).
- [ ] **3.5** `mkt` + `mkp` + `mku` status/mcp stats tests.

#### Exit gate / validation

`/api/system/info` still parses in existing tests. New keys documented in the
phase Status.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 4: Listen Interface

- **Goal:** Dedicated MHD daemon binds `Interface:Port` and serves `Path`.
- **Dependencies:** Phase 2, 3.
- **Entry gate:** Skeleton launches.

#### Work items

- [ ] **4.1** Start MHD in `launch_mcp_subsystem` when enabled; fail launch
      (not SEGV) if port in use — [no_segv_tolerance](/docs/H/INSTRUCTIONS.md).
      Flags: internal poll + select + `MHD_ALLOW_SUSPEND_RESUME` +
      `MCP.ThreadPoolSize`. Not `MHD_USE_THREAD_PER_CONNECTION`.
- [ ] **4.2** Reject non-Path URLs with 404 (except well-known + healthz).
      GET on `Path` itself is **405** in v1 (no SSE). Other methods 405.
- [ ] **4.3** POST stub returns 501 JSON-RPC “not implemented” **or** a
      temporary hardcoded initialize — prefer 501 so Phase 7 is the first
      real dispatch.
- [ ] **4.4** Landing: stop accept, resume+fail in-flight suspended
      connections, join pool, `mcp_threads` drain with timeout (copy
      WebServer, not a single-thread join).
- [ ] **4.5** `GET <Path>/healthz` — unauthenticated, no `Origin` check, no
      JSON-RPC, fixed `200 {"status":"ok"}` while the listener is up.
- [ ] **4.5b** `GET /.well-known/oauth-protected-resource` and
      `GET /.well-known/oauth-protected-resource<Path>` — unauthenticated
      RFC 9728 document (may be a stub until Phase 5 fills issuers).
- [ ] **4.6** `Origin` header validation against `MCP.AllowedOrigins`
      (Design Principle 10) on POST/DELETE. Reject with 403 before JWT
      parsing when `Origin` is present and not allowlisted. Increment
      `origin_rejected`, not `auth_rejected`.
- [ ] **4.7** Unity with global `USE_MOCK_LIBMICROHTTPD`: bind failure,
      path mismatch, method mismatch, `/healthz`, PRM path, Origin
      allowed / mismatched / absent, thread-pool option plumbed.
- [ ] **4.8** `mkt` + `mkp`. No blackbox script yet.

#### Exit gate / validation

Enabled config logs listen address. Disabled config does not bind. Port-in-use
is a launch No-Go / launch failure, not a crash.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 5: Bearer Gate And Discovery

- **Goal:** Every POST is authenticated before Lua runs, and a failed
  auth is **discoverable** by MCP clients that speak RFC 9728.
- **Dependencies:** Phase 4, Phase 0 auth lock.
- **Entry gate:** Daemon accepts POST.

#### Work items

- [ ] **5.1** Require `Authorization: Bearer`. Missing/malformed → 401
      with `WWW-Authenticate: Bearer realm="hydrogen-mcp",
      resource_metadata="<absolute PRM URL>"`. Increment `auth_rejected`
      with reason `missing` / `malformed`.
- [ ] **5.2** Validate in order: Hydrogen JWT (`AcceptHydrogenJWT`) via
      `extract_and_validate_jwt`; then OIDC IdP JWKS (`AcceptOidcIdP`);
      then each `OIDC_RP.Providers[]` JWKS (`AcceptOidcRp`). First
      success wins. Inject claims for later dispatch. Reject request
      bodies that contain `_hydrogen`.
- [ ] **5.3** Audience / scope: OIDC tokens must include `MCP.Resource`
      in `aud` when `aud` is present; `RequiredScopes` must be a subset
      of token scopes. Hydrogen user JWTs skip scope. Failures reason-
      coded `aud` / `scope`.
- [ ] **5.4** Fill the Phase 4 PRM document: `resource`,
      `authorization_servers`, `bearer_methods_supported: ["header"]`,
      `resource_signing_alg_values_supported` matching what we accept.
- [ ] **5.5** `RequireJWT=false` path only if Phase 0 allowed it; still
      log ALERT once at launch. PRM still served.
- [ ] **5.6** Unity: no header, bad Hydrogen JWT, expired, reserved
      `_hydrogen`, happy Hydrogen JWT, IdP/RP reject + accept (mock
      JWKS), `aud` mismatch, `WWW-Authenticate` + PRM JSON shape.
- [ ] **5.7** `mkt` + `mkp`.

#### Exit gate / validation

No unauthenticated path to dispatch when `RequireJWT=true`. A client
that only knows “MCP + OAuth discovery” can find the AS list from a
bare 401 without reading Hydrogen docs.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 6: JSON-RPC Envelope

- **Goal:** C parses JSON-RPC 2.0 and rejects garbage without calling Lua.
- **Dependencies:** Phase 5.
- **Entry gate:** JWT gate green.

#### Work items

- [ ] **6.1** Parse body with jansson. Cap `MaxBodyBytes`.
- [ ] **6.2** Validate `jsonrpc == "2.0"`, `method` is a non-empty string,
      `id` is string/number/null (notify). Do **not** switch on method.
- [ ] **6.3** Batch arrays: reject in v1 with `-32600` (or lock support in
      Phase 0). Recommendation: reject. Keeps C small.
- [ ] **6.3b** Pass `MCP-Protocol-Version` header into
      `_hydrogen.protocol_version` (default `2025-03-26` if absent, per
      spec). Do not switch on it in C; Lua `initialize` negotiates.
      Store the negotiated version on the session after Phase 7.
- [ ] **6.4** Assign / echo `Mcp-Session-Id` header (generate on
      `initialize` only once Lua exists; for now generate on first POST).
      Bind session → `sub`. A later request whose Bearer `sub` does not
      match is 401, not a session steal.
- [ ] **6.5** Unknown/expired `Mcp-Session-Id` on a non-`initialize` request
      → 404 (spec-mandated), forcing the client to re-`initialize` rather
      than silently starting a new anonymous session under an old id.
- [ ] **6.6** `DELETE` with a valid `Mcp-Session-Id` → explicit session
      termination (204), freeing the session→subject binding immediately
      instead of waiting for `SessionIdleTimeoutSeconds`. Well-behaved
      clients (and the blackbox test) should call this on clean shutdown.
- [ ] **6.7** Idle-session reaper: a periodic sweep (tied to
      `SessionIdleTimeoutSeconds`) evicts session entries with no activity;
      new sessions beyond `MaxSessions` are rejected with a JSON-RPC error
      rather than growing the table unbounded. Export `sessions_expired`
      alongside the Phase 3 counters.
- [ ] **6.8** Unity: parse error, invalid request, notify vs request,
      oversize, unknown session id 404, DELETE termination, reaper eviction,
      `MaxSessions` rejection.
- [ ] **6.9** `mkt` + `mkp`.

#### Exit gate / validation

Invalid JSON never reaches Scripting. Counters distinguish parse vs auth vs
dispatch.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 7: Dispatch To Protocol Script

- **Goal:** Valid JWT + valid envelope → `scripting_submit_job` on
  `MCP.Protocol` with wait.
- **Dependencies:** Phase 6. Scripting workers.
- **Entry gate:** Envelope tests green.

#### Work items

- [ ] **7.1** Build params JSON (envelope + `_hydrogen`). Submit
      `Group.Name` from config. **Do not** `scripting_wait_job` on the
      MHD thread: increment `rpc_in_flight`, `MHD_suspend_connection`,
      wait/complete on a resume callback or MCP waiter (copy conduit
      `alt_query` suspend/resume, not conduit `/script`). Cap wait at
      `RequestTimeoutSeconds`.
- [ ] **7.1b** If `rpc_in_flight` would exceed `Scripting.WorkerCount`,
      reject with the Phase 0 overload code (recommend JSON-RPC `-32000`)
      and do not enqueue.
- [ ] **7.2** Map scoreboard COMPLETED → HTTP 200 + `result_json`. FAILED /
      timeout → JSON-RPC `-32603`. SHUTDOWN → same. Increment counters.
      Always decrement `rpc_in_flight` on resume.
- [ ] **7.3** Until Phase 8 QueryRef exists, load may use the generic script
      fetch **only in Unity with injected source**. Production path waits for
      Phase 8.
- [ ] **7.4** Unity with worker-pool / scoreboard / MHD-suspend mocks or
      existing scripting seams. Cover timeout, Lua error, shutdown,
      `rpc_in_flight` cap, and “handler returned before job finished”
      (suspend path).
- [ ] **7.5** `mkt` + `mkp`.

#### Exit gate / validation

C still does not inspect `method`. A stub Protocol source that echoes
`method` is enough for Unity.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 8: `scripts.mcp_access`

- **Goal:** MCP can load only allowlisted scripts.
- **Dependencies:** Phase 7.
- **Entry gate:** Dispatch works with injected source.

#### Work items

- [ ] **8.1** Confirm next migration number and QueryRef (expected **1364** /
      **#151**). Do not collide with in-flight Helium work.
- [ ] **8.2** `ALTER TABLE scripts ADD COLUMN mcp_access … DEFAULT 0`.
- [ ] **8.2b** `ALTER TABLE scripts ADD COLUMN mcp_schema … NULL` and
      `mcp_annotations … NULL` (see "Closing the tool-schema gap" above).
      Both are nullable JSON text set by the tool's own seed migration, not
      by `Mcp.Server`.
- [ ] **8.3** QueryRef **Get MCP Script by Group/Name** — same shape as #149
      but `mcp_access <> 0`.
- [ ] **8.4** QueryRef **List MCP Scripts** for `H.mcp.list`, returning
      `mcp_schema` / `mcp_annotations` alongside name/group/summary so
      `Mcp.Server` never has to look them up per-tool.
- [ ] **8.5** C load path uses the MCP QueryRef only. Missing / `mcp_access=0`
      → same 404. Protocol script itself must have `mcp_access=1`.
- [ ] **8.6** Unity: not found, not allowed, allowed. No existence leak.
- [ ] **8.7** luacheck + at least one engine migration path.

#### Exit gate / validation

`invokable=1` / `mcp_access=0` is not callable via MCP. `invokable=0` /
`mcp_access=1` is not callable via `/api/conduit/script`.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 9: `H.mcp` Host API

- **Goal:** Protocol Lua can list and call MCP-allowlisted scripts.
- **Dependencies:** Phase 8.
- **Entry gate:** QueryRefs exist.

#### Work items

- [ ] **9.1** `H.mcp.list` / `H.mcp.call` / `H.mcp.call_async` in
      `scripting_api_*` (new file, not bolted onto mail). `H.mcp.list`
      supports `cursor` / `page_size` and decodes `mcp_schema` /
      `mcp_annotations` from JSON text to Lua tables (or `nil` if absent)
      before returning rows.
- [ ] **9.2** `H.mcp.call` re-checks `mcp_access` in C and runs the tool
      in a **child `lua_State` on the calling worker** (Design Principle
      15). `H.mcp.call_async` re-checks and **queues**. Unity must prove
      `call` does not increment the job queue.
- [ ] **9.3** Inject `_hydrogen` into tool scripts; reject tool-supplied
      `_hydrogen`.
- [ ] **9.4** `H.mcp.text` / `H.mcp.image` / `H.mcp.audio` /
      `H.mcp.resource_link` / `H.mcp.tool_error` — pure Lua-side table
      builders (can live entirely in the seeded `Mcp.Server`/shared Lua
      helpers rather than as new host functions if that is simpler; record
      the choice here since it affects whether this is C or Lua work).
- [ ] **9.5** Unity: list empty, list two (with/without schema), list
      pagination, call denied, inline call ok (no extra scoreboard job),
      call_async + `H.wait`, reserved key, child-state destroy on tool
      `error()`.
- [ ] **9.6** Docs stub in [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md)
      including the content-block helper signatures and an example
      `tools/call` handler.
- [ ] **9.7** `mkt` + `mkp` + `mku scripting_api_test_mcp*`.

#### Exit gate / validation

Protocol script can be written without any new C.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 10: Seed `Mcp.Server` And `Mcp.Echo`

- **Goal:** A real protocol implementation and one fixture tool.
- **Dependencies:** Phase 9.
- **Entry gate:** `H.mcp` Unity green.

#### Work items

- [ ] **10.1** Seed `Mcp.Server` (`mcp_access=1`, `invokable=0`) implementing
      at least: `initialize`, `notifications/initialized`, `ping`,
      `tools/list`, `tools/call`, `notifications/cancelled` (ignore).
- [ ] **10.2** Capability object: `tools.listChanged=false` in v1.
      `initialize` returns `serverInfo` plus a seeded
      `instructions` string (see [Exposing Hydrogen To Any Model](#exposing-hydrogen-to-any-model)).
      Truncate tool results larger than `MaxResultBytes`.
- [ ] **10.3** Seed `Mcp.Echo` — returns its arguments, with a real
      `mcp_schema.inputSchema` (not the permissive fallback) and
      `mcp_annotations = { readOnlyHint = true, idempotentHint = true }` so
      it doubles as the worked example for future tool seeds, not just a
      protocol fixture.
- [ ] **10.4** `tools/call` name `Mcp.Echo` → `H.mcp.call`. Unknown tool →
      MCP tool error (`H.mcp.tool_error`), not C 404. Tool-raised failures
      (bad args) also use `H.mcp.tool_error`, never a JSON-RPC `error`
      object (Design Principle 12) — add a **second** fixture tool,
      `Mcp.EchoStrict`, that validates its own arguments against its
      `inputSchema` and returns `H.mcp.tool_error` on mismatch, so Test 47
      has a real case for tool-level failure shape.
- [ ] **10.4b** Seed `Mcp.Sleep` (`mcp_access=1`, `invokable=0`) — sleeps
      `params.seconds` (capped, e.g. 60) so Test 47 can exercise timeout
      and `notifications/cancelled` without a product tool. `readOnlyHint`
      + `destructiveHint=false`.
- [ ] **10.5** luacheck. No C change expected; if C changes, that is a
      design leak — stop and record it.

#### Exit gate / validation

A local hydrogen with MCP enabled can initialize and echo through Lua only.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 11: MCP Status Endpoint And Observability

- **Goal:** Operators can see MCP health without reading logs.
- **Dependencies:** Phase 3, Phase 4.
- **Entry gate:** Daemon + counters exist. May run in parallel with 8–10
  once 3–4 are done.

#### Work items

- [ ] **11.1** `GET /api/mcp/status` (WebServer/API, not the MCP port) —
      JWT required, no special role (mirror mailrelay status).
- [ ] **11.2** Body: enabled, listen, protocol name, accept flags
      (`AcceptHydrogenJWT` / `AcceptOidcIdP` / `AcceptOidcRp`),
      `Resource`, counters (`rpc_in_flight` included), thread count.
      No tokens, no JWKS.
- [ ] **11.3** Swagger annotations + regenerate payload.
- [ ] **11.4** Unity handler tests. `test_22_swagger.sh` if annotations
      change the spec.
- [ ] **11.5** `mkt` + `mkp`.

#### Exit gate / validation

Status works when MCP disabled (enabled=false, zeros). No listen leak of JWT.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 12: Unity Coverage Sweep

- **Goal:** Combined coverage on new `src/mcp/` and config/launch/landing
  is not a sea of zeros.
- **Dependencies:** Phases 1–11 C.
- **Entry gate:** Feature C is in.

#### Work items

- [ ] **12.1** `extras/add_coverage.sh mcp/*.c` (and config/launch/landing
      mcp files).
- [ ] **12.2** Fill gaps on safe functions. Do not chase MHD/OpenSSL
      allocation floors.
- [ ] **12.3** Confirm no new `static` callables (`mkt` static gate).
- [ ] **12.4** `mkp`.

#### Exit gate / validation

Coverage report attached to Status. Residual lines listed with rationale.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 13: Blackbox Test 47

- **Goal:** Real process, real JWT, real Lua protocol.
- **Dependencies:** Phase 10, Phase 12.
- **Entry gate:** Seeds apply on at least one engine.

#### Work items

- [ ] **13.1** Create `test_47_mcp.sh` + configs + `/docs/H/tests/test_47_mcp.md`.
      WebServer **15470–15476**, MCP **15480–15486**. `WorkerCount >= 2`.
      Update [TESTING.md](/docs/H/tests/TESTING.md),
      [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md), [SITEMAP.md](/docs/H/SITEMAP.md).
- [ ] **13.2** Cases (SQLite required; other engines skip-pass if
      migrations absent, same as Test 46). Disabled → MCP port refused.
      `/healthz` and PRM well-known 200 without JWT. 401 on `Path`
      includes `WWW-Authenticate` + `resource_metadata`. Mismatched
      `Origin` → 403. initialize → serverInfo + `Mcp-Session-Id`.
      `notifications/initialized` → 202 empty. GET `Path` → 405.
      `tools/list` has `Mcp.Echo` / `Mcp.EchoStrict` with `inputSchema`.
      Echo success (`result.content`, no `isError`). EchoStrict bad
      args → `result.isError=true`. Unknown / non-`mcp_access` hidden.
      `_hydrogen` rejected. Unknown session → 404. Session hijack
      (user B JWT + user A session) → 401. DELETE → 204 then reuse
      rejected. Two overlapping Echo calls with `WorkerCount=2` both
      200 (queued `H.mcp.call` would deadlock). `Mcp.Sleep` timeout →
      `-32603`. `notifications/cancelled` during Sleep does not hang
      landing. Shutdown clean (`test_16` / `test_17` with MCP on).
- [ ] **13.3** Prefer one engine first (SQLite), then expand if script
      load is engine-agnostic. Do not require Keycloak in Test 47; PRM
      + Hydrogen JWT is the CI path. Optional manual: IdP token (Test 45
      helper) and Keycloak token (Test 42 helper) against `AcceptOidc*`.
- [ ] **13.4** `mks`. Isolate `CHAT_CACHE_DIR` if any chat helper is reused
      (it should not be).
- [ ] **13.5** ASAN smoke: one enabled-MCP start/stop under Test 11 config
      or a dedicated ASAN run — no leaks on init/shutdown; two-client
      overlap included so the child `lua_State` path is in the leak set.
- [ ] **13.6** Protocol-compliance smoke with the reference
      `@modelcontextprotocol/inspector` CLI
      (`npx @modelcontextprotocol/inspector --cli
      http://127.0.0.1:15480/mcp --method tools/list`, JWT via header
      flag) run manually against a locally-started test instance. This is
      not part of `test_47_mcp.sh` (no network installs in blackbox CI) but
      should be run once per phase-13 pass and the result noted in Status —
      it catches spec-shape mistakes (missing `inputSchema`, wrong content
      block keys) that a hand-rolled curl assertion can miss because curl
      only checks the fields the test author remembered to check.

#### Exit gate / validation

`./tests/test_47_mcp.sh` green. `mks` green. Links/markdown tests green.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 14: Docs And Closeout

- **Goal:** Operators and AI agents can find MCP without reading this plan.
- **Dependencies:** Phase 13.
- **Entry gate:** Blackbox green.

#### Work items

- [ ] **14.1** `/docs/H/core/subsystems/mcp/mcp.md` — architecture.
- [ ] **14.2** `/docs/H/api/mcp/mcp_endpoints.md` — listen + status.
- [ ] **14.3** LUA_GUIDE / lua_api `H.mcp` section (inline `call` vs
      `call_async` deadlock note).
- [ ] **14.3b** Agent provisioning: Cursor/Claude `mcp.json` snippet,
      Keycloak public-client checklist, PRM field reference. Point at
      [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md) and
      [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) — do not duplicate IdP/RP
      docs.
- [ ] **14.4**       [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) config letter T,
      subsystem order 21, test 47 (ports 1547x / 1548x).
- [ ] **14.5** [STRUCTURE.md](/docs/H/STRUCTURE.md), [README.md](/docs/H/README.md)
      TOC if needed, [RELEASES.md](/RELEASES.md) note.
- [ ] **14.6** `test_04` + `test_90`. Move this plan to
      `plans/complete/MCP_COMPLETE.md` only when Groups A–E are done.

#### Exit gate / validation

`mkl` / Test 04 clean. Plan Status blocks 0–14 complete or deferred.

#### Status

| | |
| --- | --- |
| **State** | not started |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 15: Resources And Prompts (optional)

- **Goal:** Lua grows `resources/*` and `prompts/*` with **no C change**.
- **Dependencies:** Phase 10.
- **Entry gate:** Tools path proven.

If C must change, Phase 0 failed — record why in Working Log before adding C.

#### Work items

- [ ] **15.1** Extend `Mcp.Server` only.
- [ ] **15.2** Optional `scripts.mcp_kind` (`tool` / `resource` / `prompt`)
      **only if** list filtering in SQL is worth a column. Prefer a Lua
      naming convention (`Mcp.Tools.*`) first.
- [ ] **15.3** Blackbox cases on Test 47.

#### Status

| | |
| --- | --- |
| **State** | deferred optional |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 16: MCP OAuth 2.1 DCR (optional)

- **Goal:** Dynamic Client Registration and any remaining authorization-
  **server** work that public MCP clients cannot do with Hydrogen IdP or
  Keycloak as they exist today.
- **Dependencies:** Phase 13. OIDC IdP if we issue tokens ourselves.
- **Entry gate:** v1 PRM + multi-issuer accept is production-usable.

Not v1. v1 already advertises existing authorization servers and accepts
their tokens. This phase is only for `/oauth/register` (currently a stub)
and any MCP-specific client identity model a vendor client refuses to
configure by hand. Do not reopen Phase 5 to “add OAuth.”

#### Status

| | |
| --- | --- |
| **State** | deferred optional |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

### Phase 17: Extra Transports (optional)

- **Goal:** stdio adapter and/or multiple `Protocol` virtual servers.
- **Dependencies:** Phase 13.
- **Entry gate:** HTTP path stable.

v1 is one listen + one Protocol script. Multiple servers would be a config
array — that **is** a C change, so it is explicitly later.

#### Status

| | |
| --- | --- |
| **State** | deferred optional |
| **Date** | |
| **Result** | |
| **Variances** | |

#### Lessons learned

(fill after the phase)

---

## Supplemental Material

### Stage A — Why lock design first

MCP’s spec is moving (SSE → Streamable HTTP; optional OAuth). If C encodes
method names or tool schemas, every spec bump is a Hydrogen release. Phase 0
exists so the C surface is the **transport + trust boundary** only.

### Stage B — Why the count bump is its own phase

Hydrogen tests and `ReadinessResults` are sized to `MAX_SUBSYSTEMS`. Launch
and landing are explicit `strcmp` dispatch tables, not plugins. Phase 2 is
deliberately “plumbing only” so a broken count is not tangled with MHD.

Reporting is the newest registered subsystem and the template for a
disabled-by-default clean skip. Scripting is the template for “depends on
Database, register in readiness or the launch loop cannot find you.” MCP
needs both.

### Stage C — Why a dedicated port

WebServer already owns `/api`, uploads, swagger, and prefix rewriting
(`test_20`). MCP clients send JSON-RPC to a single URL and may use GET+SSE.
A second MHD daemon:

- Can die/restart without taking the main API down.
- Has its own port in the `5<TT>x` test scheme.
- Avoids teaching WebServer about `Mcp-Session-Id`.

If Phase 0 chooses attach-to-WebServer instead, Phase 4 shrinks to a path
handler in `api_service.c` and MCP is no longer fully independent. Record
that as a variance; do not silently mix both.

### Stage D — Why two allowlist flags

`invokable` is the SPA/Conduit gate. Orchestrator scripts, mail event
handlers, and Stripe webhooks must not become MCP tools just because they
exist in `scripts`. `mcp_access` is the same idea from the other side:
Reception must not `POST /api/conduit/script` a privileged MCP admin tool
unless that tool is also `invokable`.

### Stage E — Why discovery + existing issuers, not a new OAuth stack

Hydrogen already issues and validates JWTs (`test_40`, conduit script,
mailrelay), already *is* an OIDC IdP (`test_45`), and already consumes
Keycloak as an RP (`test_42`). MCP OAuth 2.1's missing piece on this
repo is the **resource-server** half: RFC 9728 PRM and a 401
`WWW-Authenticate` so Cursor/Claude can find those issuers. Building a
fourth token mint on the MCP port would duplicate IdP. Phase 16 is only
DCR if a vendor client refuses a pre-registered client_id.

Several models from several vendors are just several HTTP clients. They
share one MCP listen and one PRM document. C never learns model names.

### Stage F — Fixture policy

`Mcp.Echo` is the `Api.Echo` of this plan. No product seeds in the Hydrogen
plan. 500 Courses / Philement tools are Helium migrations later, with
`mcp_access=1`, and must not appear in `src/`.

### MCP methods v1 Lua should implement

| Method | v1 |
| --- | --- |
| `initialize` | required |
| `notifications/initialized` | required (no-op ok) |
| `ping` | required |
| `tools/list` | required |
| `tools/call` | required |
| `resources/list` | Phase 15 |
| `resources/read` | Phase 15 |
| `prompts/list` | Phase 15 |
| `prompts/get` | Phase 15 |
| `logging/setLevel` | optional no-op |
| `completion/complete` | out of scope |

C must not contain this table.

### Threat notes

- Unauthenticated MCP is remote script execution against the DB. Default JWT
  on. Disabled JWT is a launch ALERT.
- Existence hiding: do not return 403 “script exists but mcp_access=0”.
- Tool results may contain PII. Do not log bodies.
- Protocol script is trusted operator code (DB-seeded), same as Orchestrator.
  Clients cannot upload Lua.
- Session IDs must not be accepted from the client on the first call in a
  way that hijacks another JWT’s session. Bind session → subject; mismatch
  is 401.
- Inbound Bearer is **not** forwarded to `H.http` / Stripe / a second MCP
  server (confused deputy / token passthrough).
- **DNS rebinding**: JWT alone does not stop a browser page from POSTing to
  `localhost:3100/mcp` if a token is ambiently available (e.g. stored in a
  way the browser can reach). Validate `Origin` on every state-changing
  request in addition to JWT; do not treat JWT as sufficient because MCP is
  "just an API" — the MCP spec calls this out specifically for HTTP
  transports.
- **Unbounded session growth**: without an idle reaper and `MaxSessions`,
  a client that never sends `DELETE` (most won't) leaks a session→subject
  binding per `initialize` forever. This is a slow memory-growth bug, not a
  crash, so it will not show up in `test_16`/`test_17`/ASAN smoke — it needs
  its own long-run or count-based test case.

### Risks

| Risk | Mitigation |
| --- | --- |
| `MAX_SUBSYSTEMS` / test count drift | Phase 2 dedicated; grep + `test_17` gate |
| Lua heap corruption on reuse | Fresh `lua_State` per job (existing worker rule). Do not keep a long-lived MCP Lua state in C |
| MHD + worker deadlock | MHD suspend/resume (copy `alt_query`); never `scripting_wait_job` on an MHD thread |
| Nested `H.mcp.call` deadlock | Inline child `lua_State`; launch No-Go if `WorkerCount < 2` |
| Port clashes in parallel tests | 1547x WebServer + 1548x MCP only for Test 47 |
| Vendor clients cannot find auth | PRM + `WWW-Authenticate` in Phase 5; do not wait for Phase 16 |
| Spec churn | Method table lives in Lua |
| Helium migration number race | Re-read `acuranzo/README.md` at Phase 8 start |
| Session table growth | `SessionIdleTimeoutSeconds` reaper + `MaxSessions` cap (Phase 6) |
| Tool added without a schema | `Mcp.Server` falls back to a permissive schema on `mcp_schema IS NULL` rather than erroring, so a missing schema degrades agent UX instead of breaking `tools/list` |
| DNS rebinding via browser MCP client | `MCP.AllowedOrigins` + Origin check ahead of JWT (Phase 4) |

---

## Definition Of Done

Groups A–E complete:

- MCP is a registered subsystem with launch/landing/status.
- Disabled by default; enabled config serves Streamable HTTP on the configured
  interface.
- Bearer required; Hydrogen JWT accepted; optional OIDC IdP / Keycloak;
  PRM + `WWW-Authenticate` served; `_hydrogen` injected; `mcp_access`
  enforced in C.
- `Mcp.Server` + `Mcp.Echo` are Lua-only protocol/tool.
  `initialize.instructions` present. No generic SQL fixture.
- Adding a tool is a Helium script seed (`mcp_access=1`) with no Hydrogen C
  patch.
- Unity inventory + Test 47 (including two-client overlap, cancel,
  timeout, PRM) + lint gates green.
- Docs and INSTRUCTIONS updated.
- This plan moved to `plans/complete/MCP_COMPLETE.md`.

---

## Working Log (cross-phase memory)

Append discoveries, surprises, and decisions here. Earlier-phase learnings
that affect later phases must be recorded so they are not lost.

### Decisions log

- (2026-08-22) Plan created. Recommended locks (not yet Phase 0 official):
  dedicated Streamable HTTP; config letter T; `Interface` = bind address;
  `Protocol` = `Mcp.Server`; JWT always; `scripts.mcp_access` DEFAULT 0;
  clean skip when disabled; Test 47 / 547x; `MAX_SUBSYSTEMS` 22 → 24;
  next migration ~1364 / QueryRef ~#151 subject to live check.
- (2026-08-22) C/Lua split: C = listen + JWT + JSON-RPC envelope +
  `H.mcp.list/call`. Lua = every MCP method and every tool.
- (2026-08-22) Robustness/agent-ergonomics review pass. New recommendations
  (pending Phase 0 confirmation, added to the Locked-decisions table):
  1. **Tool schema gap** — the original `H.mcp.list()` shape (name, group,
     summary) has no way to hand a real `inputSchema` to `tools/list`
     without hardcoding tools inside `Mcp.Server`, which contradicts Goal 5.
     Added nullable `scripts.mcp_schema` / `mcp_annotations` JSON columns,
     seeded per tool, decoded generically by `Mcp.Server`.
  2. **Origin/DNS-rebinding** — JWT does not cover the MCP-spec-mandated
     `Origin` check for HTTP transports. Added `MCP.AllowedOrigins` and a
     Phase 4 validation step ahead of JWT parsing.
  3. **Unauthenticated health path** — infra liveness/readiness probes
     should not need a JWT. Added `GET <Path>/healthz` on the MCP port,
     kept distinct from Phase 11's authenticated `/api/mcp/status`.
  4. **Session lifecycle** — added `DELETE` termination, 404-on-unknown-
     session (spec-mandated reinit signal), and an idle-session reaper
     (`SessionIdleTimeoutSeconds` / `MaxSessions`) to Phase 6, since nothing
     in the original plan freed abandoned sessions.
  5. **Tool-error shape** — made explicit (Design Principle 12) that tool
     execution failures are `result.isError=true`, never a JSON-RPC `error`
     object; added `H.mcp.tool_error` plus content-block builders
     (`H.mcp.text/image/audio/resource_link`) so tool authors cannot get the
     wire shape wrong by hand.
  6. **Async tool dispatch** — added `H.mcp.call_async` mirroring the
     existing `H.query`/`H.http` async+`H.wait` convention, so `Mcp.Server`
     can fan out to multiple MCP-allowlisted scripts or avoid blocking on a
     slow tool inside the RPC's own timeout budget.
  7. **Dev ergonomics** — added `extras/mcp_probe.sh` (curl/jq, Phase 13) for
     five-second manual iteration on a new tool, and an MCP Inspector CLI
     smoke pass (manual, not blackbox CI) to catch spec-shape mistakes that
     hand-written curl assertions miss.
  8. **Seed fixtures** — `Mcp.EchoStrict` added alongside `Mcp.Echo` (Phase
     10) specifically to exercise the tool-error path and to be the worked
     example of a tool with a real `inputSchema`. `Mcp.Sleep` added for
     timeout / cancel blackbox.
  9. **Auth discovery / multi-issuer** — v1 is the MCP *resource server*:
     RFC 9728 PRM + `WWW-Authenticate` on 401, accepting Hydrogen JWT
     (default), Hydrogen OIDC IdP tokens, and OIDC RP / Keycloak tokens.
     Full DCR stays Phase 16. Several vendor models are several HTTP
     clients sharing one PRM document — no per-model C.
  10. **Concurrency lock** — not thread-per-client. MHD thread pool +
      `MHD_ALLOW_SUSPEND_RESUME` (copy WebServer / `alt_query`, not
      conduit `/script`'s blocking wait). `H.mcp.call` is an inline child
      `lua_State` so two concurrent `tools/call`s cannot deadlock a
      `WorkerCount=2` pool. Launch No-Go if MCP enabled and
      `WorkerCount < 2`. GET+SSE is 405 in v1.
  11. **Test ports** — Test 47 uses 1547x (WebServer) + 1548x (MCP),
      matching Test 43/46, not 547x (ephemeral clash).
  12. **Unity inventory** — named files per `TESTING_UNITY.md` one-
      function convention, listed before Phase 1 so coverage is planned
      rather than a Phase 12 surprise.
  13. **Agent surface** — `initialize.instructions`, no generic SQL,
      `MaxResultBytes`, public `MCP.Resource`, and an honest client
      compatibility matrix. Protocol success ≠ Hydrogen exposed.

### Surprises

(none yet)

### Follow-ups

(none yet)

---

## Lessons Learned (plan-level)

Copy phase Lessons learned here when they should outlive a single phase.

- *(none yet)*
