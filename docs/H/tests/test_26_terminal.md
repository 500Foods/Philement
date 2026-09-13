# Test 26 — Terminal

## Overview

`test_26_terminal.sh` validates the Hydrogen terminal subsystem end-to-end:
HTTP payload/filesystem serving, WebSocket authentication (header and query-string),
the two-key/two-protocol contract, and the `/api/system/info` authorization
gating (some-then-all). It runs two configurations in parallel (payload mode and
filesystem mode) and verifies cross-key denial, protocol mismatch rejection,
and that no secrets appear in logs or output.

## Script Information

- **Script**: `test_26_terminal.sh`
- **Purpose**: Test terminal functionality, payload serving, WebSocket connections, auth contract, and info gating
- **Version**: 2.9.0
- **Dependencies**: websocat, curl, jq, standard Unix utilities
- **Libraries**: [`terminal_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/terminal_utils.sh), [`terminal_ws_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/terminal_ws_helpers.sh)

## Contract

### Two-key / two-protocol surface

| Surface | Path | Protocol field | Key field | Environment |
| --- | --- | --- | --- | --- |
| Chat | `/wss` | `WebSocketServer.Protocol` (`hydrogen`) | `WebSocketServer.Key` | `${env.WEBSOCKET_KEY}` |
| Terminal | `{WebPath}/ws` | `Terminal.Protocol` (`terminal`) | `Terminal.Key` | `${env.WEBSOCKET_TERMINAL_KEY}` |

Cross-key denial is enforced at both the HTTP-upgrade callback
([`websocket_server.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server.c))
and the protocol-filter dispatch
([`websocket_server_dispatch.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c))
via the shared `ws_auth_accept_key` / `ws_extract_query_auth_key` helper in
[`websocket_server_auth.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_auth.c).

### Query-string auth

Browser clients send `?key=` in the WebSocket URL (the browser `WebSocket()`
API cannot set custom headers). `ws_extract_query_auth_key` reads
`WSI_TOKEN_HTTP_URI_ARGS` first (the query without `?`), then falls back to
scanning `WSI_TOKEN_GET_URI` for `?key=`. Non-browser tests may also use
`Authorization: Key <value>`.

### System-info authorization gating

`GET /api/system/info` is a some-then-all endpoint:

- **No JWT or invalid JWT** — short public payload only (`version` with
  `auth` first, `status.server_running` only). No `terminal`, no
  `system`, no `mdns`, no `services`, no FD dump.
- **Valid JWT without terminal role** — full ops dump plus `scripting`,
  `version.auth` = `jwt`. No `terminal` object.
- **Valid JWT with terminal role (role_id 32)** — full dump plus
  `scripting` plus `terminal.{enabled, url, protocol, key}`.
  `version.auth` = `jwt, terminal`. The `terminal.key` is `Terminal.Key`,
  never the chat key. The `url` is `WebSocketServer.PublicUrl` +
  `Terminal.WebPath` + `/ws` (no query string or key in the URL).

Lua `H.system.info()` always returns the full dump plus `scripting`; it
never receives `version.auth` or the `terminal` object.

## Test Configurations

The script tests two parallel configurations:

### Payload Mode

- **Configuration**: `hydrogen_test_26_terminal_payload.json`
- **Content Source**: Terminal interface served from the embedded payload
- **Expected Content**: "Hydrogen Terminal"
- **WebSocket Protocol**: `hydrogen` (chat) / `terminal` (terminal)
- **WebSocket Port**: 5261
- **Key**: chat = `${env.WEBSOCKET_KEY}` / terminal = `${env.WEBSOCKET_TERMINAL_KEY}`

### Filesystem Mode

- **Configuration**: `hydrogen_test_26_terminal_filesystem.json`
- **Content Source**: Terminal files served from filesystem (`tests/artifacts/terminal/`)
- **Expected Content**: `HYDROGEN_TERMINAL_TEST_MARKER`
- **WebSocket Protocol**: `hydrogen` (chat) / `terminal` (terminal)
- **WebSocket Port**: 5263
- **Key**: chat = `${env.WEBSOCKET_KEY}` / terminal = `${env.WEBSOCKET_TERMINAL_KEY}`

Both configs include:

- `Network.TrustedProxies`: loopback CIDRs (`127.0.0.1/32`, `::1/128`) for
  trusted-forwarded client IP handling under test.
- `WebSocketServer.PublicUrl`: the explicit public origin used to build the
  `terminal.url` in the authorized `/api/system/info` response.
- `Terminal.CORSOrigin`: exact origin allowlist for terminal assets and the
  system-info terminal block.
- `Databases`: SQLite (`Acuranzo`) with `LOGINID` from
  `${env.HYDROGEN_DEMO_USER_NAME}` and `JWTSecret` from
  `${env.HYELLOWRO DEMO_JWT_KEY}`, so the conditional valid-JWT test can run
  when demo credentials are present.

Each parallel instance copies `hydrodemo.sqlite` to a per-run work directory
(SQLite isolation) so both configs can run simultaneously without write
conflicts.

## Test Execution Flow

### Prerequisites Validation

1. **Binary Location**: Finds and validates the hydrogen executable.
2. **Configuration Files**: Validates both test configuration JSON files.
3. **WebSocket Key**: Validates `WEBSOCKET_KEY` environment variable
   (≥ 32 printable ASCII characters).
4. **Terminal Key**: Validates `WEBSOCKET_TERMINAL_KEY` is set, distinct from
   `WEBSOCKET_KEY`, and ≥ 32 characters. If unset, an ephemeral 32-char key is
   generated; if it matches the chat key, the test fails.
5. **Test Artifacts**: Validates terminal test files exist
   (`tests/artifacts/terminal/index.html`, `xterm-test.html`).

### Parallel Test Execution

Each config runs `run_terminal_test_parallel` in the background:

1. **Server Startup**: Starts hydrogen with the config file; waits for
   `STARTUP COMPLETE` in the log.
2. **Subsystem Readiness**: Polls the HTTP port until the server responds.
3. **HTTP Terminal Page**: Fetches `/terminal/` and checks for expected content.
4. **Filesystem-specific**: Fetches `/terminal/index.html` and checks for
   `HYDROGEN_TERMINAL_TEST_MARKER`.
5. **Cross-config isolation**: Fetches the other config's file and checks for
   404.
6. **System-info contract** (run before WebSocket stress, when FD load is low):
   - **No JWT**: `/api/system/info` returns 200, no `.terminal.url` in response.
   - **Invalid JWT**: same — no `.terminal.url`.
   - **CORS origin**: response does not expose a localhost terminal origin
     without the terminal role.
   - **Valid terminal JWT** (conditional on `HYDROGEN_DEMO_*` env vars):
     login → fetch sysinfo with JWT → `.terminal.url`, `.protocol`, `.key`
     present; key fingerprint matches the terminal key. Sets
     `TERMINAL_WS_URL`/`TERMINAL_WS_PROTOCOL`/`TERMINAL_WS_KEY` globals for
     subsequent WebSocket tests.
7. **WebSocket terminal connection**: Connects with the terminal key via
   `Authorization: Key` header, sends `{"type": "ping"}`.
8. **WebSocket status ping**: Sends a ping, expects protocol acceptance.
9. **WebSocket I/O**: Sends 8 shell commands, exercises `terminal_websocket.c`
   and `terminal_shell.c`.
10. **WebSocket resize**: Sends 5 resize commands with different dimensions.
11. **WebSocket long session**: 4 iterations over 8 seconds, exercises
    `pty_is_running` and the I/O bridge loop.
12. **Cross-key denial** (only if chat key and terminal key are distinct):
    - **Chat key on terminal path**: must be rejected (401 / connection refused).
    - **Terminal key on chat path**: must be rejected.
    - **Wrong protocol on terminal path**: must be rejected.
13. **Server shutdown**: SIGINT, then SIGTERM if needed; 15s timeout.

### Result Analysis

`analyze_terminal_test_results` reads the per-config result file and reports
each subtest via the framework's `print_subtest`/`print_result` pair. The
result file flags each step (e.g. `WEBSOCKET_CONNECTION_TEST_PASSED`,
`CROSS_KEY_CHAT_ON_TERMINAL_TEST_PASSED`).

## WebSocket Test Functions

All WebSocket test helpers live in
[`terminal_ws_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/terminal_ws_helpers.sh).
Each function uses `websocat` with `--protocol` and `Authorization: Key` header,
falling back to `${WEBSOCKET_KEY:-${RESOLVED_WS_KEY:-}}` so the terminal key
(from the sysinfo response or `WEBSOCKET_TERMINAL_KEY`) is preferred when
available.

### Connection Testing

- **Function**: `test_websocket_terminal_connection()`
- **Purpose**: Validates basic WebSocket connectivity with authentication
- **Protocol**: Tests protocol acceptance and connection establishment
- **Authentication**: Uses `${WEBSOCKET_KEY:-${RESOLVED_WS_KEY:-}}` for the
  terminal key
- **Retry**: 5 attempts with 50ms delay

### Status Testing

- **Function**: `test_websocket_terminal_status()`
- **Purpose**: Tests WebSocket ping/status functionality
- **Message**: Sends `{"type": "ping"}` and validates response
- **Retry**: 8 attempts with 50ms delay

### I/O Testing

- **Function**: `test_websocket_terminal_input_output()`
- **Purpose**: Tests terminal input/output processing
- **Commands**: Sends 8 shell commands (echo, pwd, date, whoami, etc.)
- **Coverage**: Exercises `terminal_websocket.c` and `terminal_shell.c`

### Resize Testing

- **Function**: `test_websocket_terminal_resize()`
- **Purpose**: Tests terminal resize functionality
- **Commands**: Sends 5 resize commands with different dimensions
- **Coverage**: Exercises `terminal_shell_ops.c` `pty_set_size()`

### Long Session Testing

- **Function**: `test_websocket_terminal_long_session()`
- **Purpose**: Tests long-running terminal session
- **Coverage**: Exercises `pty_is_running()` and the I/O bridge loop

### Cross-Key Denial Testing

- **Function**: `test_websocket_chat_key_rejected_on_terminal_path()`
- **Purpose**: Verifies chat key (`WEBSOCKET_KEY`) is rejected on the terminal
  path (`/terminal/ws`)
- **Function**: `test_websocket_terminal_key_rejected_on_chat_path()`
- **Purpose**: Verifies terminal key (`WEBSOCKET_TERMINAL_KEY`) is rejected on
  the chat path (`/wss`)
- **Function**: `test_websocket_wrong_protocol_rejected()`
- **Purpose**: Verifies a mismatched subprotocol is rejected on the terminal path

## HTTP Test Functions

All HTTP/sysinfo helpers live in
[`terminal_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/terminal_utils.sh).

### Content Validation

- **Function**: `check_terminal_response_content()`
- **Purpose**: Validates HTTP responses contain expected content
- **Retry Logic**: 25 attempts with 50ms delay for subsystem readiness
- **Timeout**: 10-second per-request timeout (`--max-time 10`)

### Endpoint Testing

- **Index Page**: Tests `/terminal/` endpoint accessibility
- **Specific Files**: Tests configuration-specific file access
  (`terminal.html` for payload mode, `index.html` for filesystem mode)
- **Cross-Config**: Validates proper file isolation between modes (expects 404
  for the other config's files)

### System-Info Contract Tests

- **Function**: `test_sysinfo_no_terminal_without_jwt()`
- **Purpose**: `/api/system/info` without JWT returns 200 with no
  `.terminal.url`
- **Function**: `test_sysinfo_no_terminal_with_invalid_jwt()`
- **Purpose**: Same with a forged JWT
- **Function**: `test_sysinfo_cors_origin_enforcement()`
- **Purpose**: CORS response does not expose terminal-specific origins without
  the terminal role
- **Function**: `test_sysinfo_terminal_with_valid_jwt()`
- **Purpose** (conditional on `HYDROGEN_DEMO_*` env vars): login → fetch sysinfo
  with JWT → `.terminal.url`/`.protocol`/`.key` present; key is the terminal
  key, not the chat key. Sets `TERMINAL_WS_URL`, `TERMINAL_WS_PROTOCOL`,
  `TERMINAL_WS_KEY` globals for WebSocket tests.

### WebSocket Config Resolution

- **Function**: `resolve_terminal_websocket_config()`
- **Purpose**: Builds the WebSocket URL, protocol, and key from the sysinfo
  response (if available) or from config/env fallbacks. Never uses the chat key
  as a fallback for terminal operations.

## Configuration Requirements

### Environment Variables

- **WEBSOCKET_KEY**: Required. Chat WebSocket authentication key (≥ 32 printable ASCII).
- **WEBSOCKET_TERMINAL_KEY**: Required (or ephemeral-generated). Terminal
  WebSocket authentication key. Must be distinct from `WEBSOCKET_KEY`.
- **HYDROGEN_DEMO_USER_NAME**: Optional. Login ID for the conditional valid-JWT
  test.
- **HYDROGEN_DEMO_USER_PASS**: Optional. Password for login.
- **HYDROGEN_DEMO_API_KEY**: Optional. API key for login.
- **HYDROGEN_DEMO_JWT_KEY**: Optional. JWT secret; checked to confirm the config
  is wired for the valid-JWT test.
- **PAYLOAD_KEY**: Required for payload mode (embedded payload decryption).

### Test Artifacts

- `tests/artifacts/terminal/index.html`: Filesystem mode test file
- `tests/artifacts/terminal/xterm-test.html`: Cross-config test file

### Port Configuration

| Test | HTTP Port | WebSocket Port |
| --- | --- | --- |
| Payload mode | 5260 | 5261 |
| Filesystem mode | 5262 | 5263 |

### SQLite Isolation

Both configs share the same source SQLite database. `prepare_sqlite_isolation`
copies `hydrodemo.sqlite` to a per-run temp directory and sets
`AutoMigration=false` on the copy, preventing write conflicts during parallel
execution. WAL files are checkpointed before copy.

## Error Handling

### Startup Failures

- Validates server startup within `STARTUP_TIMEOUT` (15 seconds)
- Checks for `STARTUP COMPLETE` in server logs
- Handles server startup failures gracefully (result file records `STARTUP_FAILED`)

### Connection Issues

- HTTP: 25 retries with 50ms delay (`wait_for_server_ready`, `check_terminal_response_content`)
- WebSocket: 5–8 retries with 50ms delay depending on test function
- Authentication: 401 / connection-refused detected and reported

### Test Timeouts

| Operation | Timeout |
| --- | --- |
| HTTP requests | 10s (`--max-time 10`) |
| Login | 15s (with retry) |
| Sysinfo fetch | 15s (with retry) |
| WebSocket connection | 5s (`timeout 5`) |
| Server shutdown | 15s (`SHUTDOWN_TIMEOUT`) |

## Redaction

All test output, logs, and artifacts must omit raw keys, JWTs, and full
`/api/system/info` response bodies:

- WebSocket `Authorization` headers in `print_command` lines are shown as
  `Authorization: Key ***`.
- JWT fingerprints use `redact_jwt_fingerprint` (first 16 hex chars of SHA-256).
- System-info bodies stored to result files use `redact_sysinfo_body`
  (replaces `terminal.key` with `REDACTED`).
- WebSocket keys from the sysinfo response are never printed; only a
  fingerprint is logged.

## Integration with Test Suite

Part of the comprehensive Hydrogen test suite:

- **Test Group**: 20s (Server functionality tests, parallel batch)
- **Dependencies**: Requires successful compilation (Test 01) and basic
  server tests
- **Parallel Execution**: Runs both payload and filesystem configs in parallel;
  rate-limited to `CORES` concurrent jobs
- **Result Tracking**: Integrates with `test_00_all.sh` aggregation

## Running

```bash
# From the Hydrogen test directory
./test_00_all.sh 26_terminal

# Or standalone
./tests/test_26_terminal.sh
```

## Related Documentation

- [Terminal Fix Plan](/docs/H/plans/TERMINAL_FIX_PLAN.md) — full architecture and contract
- [Test 23 WebSockets](/docs/H/tests/test_23_websockets.md) — general WebSocket testing
- [Test Infrastructure](/docs/H/tests/TESTING.md) — test runner and framework contract
- [Unity Unit Tests](/docs/H/tests/TESTING_UNITY.md) — C unit test framework
- [Terminal Architecture](/docs/H/core/reference/terminal_architecture.md) — terminal subsystem
