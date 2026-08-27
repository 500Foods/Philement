# MCP Endpoints

MCP has two HTTP surfaces. Architecture: [mcp.md](/docs/H/core/subsystems/mcp/mcp.md).

| Surface | Port | Auth | Purpose |
| --- | --- | --- | --- |
| MCP daemon | `MCP.Port` (default 3100) | Bearer on POST/DELETE | Streamable HTTP JSON-RPC |
| WebServer API | `WebServer.Port` | JWT | Operator status only |

## MCP daemon (`MCP.Interface`:`MCP.Port`)

Default bind `127.0.0.1:3100`. Path default `/mcp`.

### Unauthenticated

| Method | Path | Response |
| --- | --- | --- |
| `GET` | `<Path>/healthz` | **200** `{"status":"ok"}` while the listener is up. No JWT, no Origin check, no JSON-RPC |
| `GET` | `/.well-known/oauth-protected-resource` | **200** RFC 9728 PRM JSON |
| `GET` | `/.well-known/oauth-protected-resource<Path>` | Same document (clients try both) |

Healthz is for container/load-balancer probes. Do not use it as a substitute for `/api/mcp/status`.

### PRM (RFC 9728)

```json
{
  "resource": "http://127.0.0.1:3100/mcp",
  "authorization_servers": [],
  "bearer_methods_supported": ["header"],
  "resource_signing_alg_values_supported": ["HS256"]
}
```

`authorization_servers` is filled at launch from `OIDC.Issuer` when `AcceptOidcIdP` and from each `OIDC_RP.Providers[].Issuer` when `AcceptOidcRp`. Hydrogen user-JWT has no AS URL — paste a login JWT as Bearer.

`resource` is `MCP.Resource` when set; otherwise derived from bind host/port/path. Behind a TLS terminator set `MCP.Resource` to the public `https://…/mcp` URL (same rule as `OIDC.Issuer`).

### Authenticated JSON-RPC

```text
POST <Path>
Authorization: Bearer <token>
Content-Type: application/json
Mcp-Session-Id: <id after initialize>
MCP-Protocol-Version: 2025-03-26
```

```bash
curl -sS -X POST "http://127.0.0.1:3100/mcp" \
  -H "Authorization: Bearer ${TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"curl","version":"1"}}}'
```

| Condition | HTTP | Body |
| --- | --- | --- |
| Missing/bad Bearer | 401 | `WWW-Authenticate: Bearer realm="hydrogen-mcp", resource_metadata="<PRM URL>"` |
| Present mismatched `Origin` | 403 | before JWT |
| Body too large / invalid JSON | 400 | JSON-RPC `-32700` if possible |
| Batch array | 400 | JSON-RPC `-32600` |
| Unknown / expired session (non-initialize) | 404 | spec reinit signal |
| Session `sub` hijack | 401 | |
| Protocol script missing / no `mcp_access` | 404 | existence-hiding |
| Lua success or protocol `error` object | 200 | Lua JSON-RPC as-is |
| Lua crash / timeout | 200 | JSON-RPC `-32603` |
| Notification (`id` absent) | 202 | empty |
| `rpc_in_flight` would exceed `WorkerCount` | 200 | JSON-RPC `-32000` |
| `MaxSessions` exceeded on initialize | 200 | JSON-RPC `-32001` |
| GET on `Path` | 405 | no SSE in v1 |
| Other methods on `Path` except DELETE | 405 | |
| Non-Path URL (except healthz / PRM) | 404 | |

Tool execution failures are **HTTP 200** with `result.isError = true`. JSON-RPC `error` is reserved for envelope/dispatch failures.

### Session

- `initialize` creates `Mcp-Session-Id` and binds it to Bearer `sub`.
- Later POSTs must send that header. Unknown id → **404**.
- A different `sub` with the same session id → **401**.
- `DELETE <Path>` with a valid `Mcp-Session-Id` → **204**, then reuse → **404**.
- Idle entries expire after `SessionIdleTimeoutSeconds`.

### Origin

A **present** `Origin` must match `MCP.AllowedOrigins` exactly. No `Origin` (typical non-browser agent) is allowed. Empty allowlist means only no-`Origin` / same-origin traffic.

### Lua contract

C does not switch on MCP method. It validates the JSON-RPC envelope and submits `MCP.Protocol` with:

```lua
{
  jsonrpc = "2.0",
  id = 1,
  method = "tools/list",
  params = { ... },
  _hydrogen = {
    sub = "...",
    iss = "...",
    roles = "...",
    scopes = { ... },
    database = "...",
    session_id = "...",
    protocol_version = "2025-03-26",
    auth_kind = "hydrogen_jwt",
  },
}
```

`MCP-Protocol-Version` defaults to `2025-03-26` if absent. Lua `initialize` negotiates. Client-supplied `_hydrogen` is rejected.

## WebServer status

### `GET /api/mcp/status`

JWT required (same as mailrelay status). No special role. Lives on the **API** port, not `MCP.Port`. Works when MCP is disabled (`enabled=false`, zeros).

```bash
curl -sS "http://127.0.0.1:5000/api/mcp/status" \
  -H "Authorization: Bearer ${TOKEN}"
```

```json
{
  "success": true,
  "enabled": true,
  "initialized": true,
  "listen": { "interface": "127.0.0.1", "port": 3100, "path": "/mcp" },
  "protocol": "Mcp.Server",
  "accept_hydrogen_jwt": true,
  "accept_oidc_idp": false,
  "accept_oidc_rp": false,
  "resource": "http://127.0.0.1:3100/mcp",
  "thread_count": 0,
  "thread_pool_size": 4,
  "counters": {
    "sessions_active": 0,
    "sessions_total": 0,
    "sessions_expired": 0,
    "rpc_received": 0,
    "rpc_succeeded": 0,
    "rpc_failed": 0,
    "rpc_in_flight": 0,
    "auth_rejected": 0,
    "auth_rejected_reasons": {
      "missing": 0,
      "malformed": 0,
      "hydrogen_jwt": 0,
      "oidc_idp": 0,
      "oidc_rp": 0,
      "aud": 0,
      "scope": 0
    },
    "origin_rejected": 0,
    "dispatch_timeouts": 0,
    "bytes_in": 0,
    "bytes_out": 0,
    "last_rpc_at": 0
  }
}
```

No tokens, no JWKS. Swagger tag **MCP Service**. Counters are also always-present on `/api/system/info` as `services.mcp` and as Prometheus `hydrogen_mcp_*`.

## Agent provisioning

v1 is Streamable HTTP plus a Bearer from a configured issuer. Do not claim Claude Desktop stdio or Dynamic Client Registration.

### Static Bearer (`mcp.json`)

Paste a Hydrogen user JWT (or a Keycloak access token once `AcceptOidcRp` is on):

```json
{
  "mcpServers": {
    "hydrogen": {
      "url": "http://127.0.0.1:3100/mcp",
      "headers": {
        "Authorization": "Bearer <paste-jwt>"
      }
    }
  }
}
```

Mint a JWT the same way as Test 46 / Test 47: `POST /api/auth/login`. Or run [`extras/mcp_probe.sh`](/elements/001-hydrogen/hydrogen/extras/mcp_probe.sh) with `--login`.

Clients that implement RFC 9728 follow `401` → `WWW-Authenticate` → PRM → `authorization_servers`. Hydrogen user-JWT has no AS URL in PRM.

### Keycloak public-client checklist

Do not duplicate IdP/RP docs. Configure the AS as usual, then:

1. Public client, PKCE.
2. Redirect `http://127.0.0.1:*` (or the MCP client's documented loopback).
3. Audience / client mapper so access tokens include `MCP.Resource`.
4. Enable `MCP.AcceptOidcRp` and list the issuer on `OIDC_RP.Providers[]`.
5. Confirm PRM `authorization_servers` contains that issuer.

IdP operator path: [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md), [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md). RP path: [oidc_rp.md](/docs/H/api/auth/oidc_rp.md).

### Compatibility

| Client class | v1 |
| --- | --- |
| Cursor / VS Code / custom `mcp.json` + static Bearer | Yes |
| Claude / Copilot remote MCP with RFC 9728 | Yes if IdP or Keycloak is on and the client is pre-registered |
| Dynamic Client Registration | No until later |
| Claude Desktop local spawn (stdio) | No — use the remote HTTP URL |
| Hosted connectors that require public HTTPS | Only behind TLS with `MCP.Resource` = public URL |

## Probe

```bash
extras/mcp_probe.sh --url http://127.0.0.1:3100/mcp --login http://127.0.0.1:5000
extras/mcp_probe.sh --url http://127.0.0.1:3100/mcp --jwt "${TOKEN}" --call Mcp.Echo '{"message":"hi"}'
```

Prints PRM GET, unauthenticated 401 `WWW-Authenticate`, `initialize`, `tools/list`, and `tools/call`.

Blackbox: [test_47_mcp.md](/docs/H/tests/test_47_mcp.md) (WebServer **15470–15476**, MCP **15480–15486**).
