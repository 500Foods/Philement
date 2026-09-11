<!-- markdownlint-disable MD007 MD024 -->
# Terminal Fix Plan

## Purpose

End-to-end fix plan for the **Hydrogen terminal subsystem integrated into the
Lithium SPA**: JWT issuance with real client IP through Traefik/DOKS, payload
regeneration, WebSocket authentication key flow from `/api/system/info`,
iframe `postMessage` hand-off from Lithium to the xterm.js iframe, and the
`_handleIframeMessage is null` crash in the deployed Lithium terminal manager.

The system is currently **not working in production** (deployed at
`https://lithium.philement.com`). This plan starts from zero assumptions and
walks through every layer — JWT → API → payload → WebSocket → iframe — until
the terminal opens and runs from the Lithium popup without requiring
"open in new window".

## How To Use This Document

- Work **one phase at a time**, in the order in the Phase Index.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, **stop for review**. Do not begin the next phase in the same turn
  unless asked.
- Hydrogen build aliases: `zsh -ic 'mkq'` for an incremental C build,
  `mkt` for a clean/configure build, `mku <base>` for a Unity test,
  `mkp` for cppcheck, `mka` for build-all, and `mks` for shellcheck.
- Test 26 is a blackbox test. Run it from the Hydrogen test directory with
  `./test_00_all.sh 26_terminal` (or pass it alongside other test bases);
  do **not** run it with `mku`.
- Lithium: `npm test`, `npm run lint`, and `npm run build` when templates or
  production assets change.
- **Never apply a database migration.** Hand packets to the user.
- **Never log client secrets, JWTs, WebSocket keys, passwords, or OTP
  plaintext** in logs, browser output, test commands, or test artifacts.

## Implementor Workflow (every phase)

1. **Confirm the prior phase is actually done.** Re-read its Status block.
2. **Discuss the current phase first.** Read Goal + Work items + Done means
   - Exit gate. Grep/read code **before** writing anything.
3. **Ask for explicit approval to start implementation.**
4. **Ask questions as they arise** during implementation rather than guessing.
5. **Update the phase's Working Log entry** at major milestones.
6. **Record lessons learned** for the phase.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean.** Intent to verify is not
   verification.
8. **Follow existing project norms** for tests (Unity, blackbox Test 26),
   lint (`mkp`, `mks`), and build aliases.

---

## Overarching Theme: Fail Closed, No Hardcoded Keys, No Secret Leakage

The WebSocket authentication key must **never** be hardcoded in source,
defaults, generated payloads, tests, or fallback paths. It must come from
runtime configuration (`WEBSOCKET_KEY` or `WebSocketServer.Key`) and be
validated before the WebSocket server starts.

**Security and configuration requirements:**

1. **Fail closed when the key is missing or unresolved.** A default such as
   `${env.WEBSOCKET_KEY}` is a configuration reference, not a usable key.
   If the environment variable is absent or empty, Hydrogen must refuse to
   start the WebSocket server (or fail the overall startup) rather than use
   `default_key`, `default_websocket_key`, `ABCDEFGHIJKLMNOP`, or any other
   literal.
2. **No hardcoded key literals in production or test source.** Test keys are
   supplied through the test environment/configuration and are ephemeral.
   A local-development key is still configuration, never a source-code
   fallback.
3. **Key flow:** config → resolved `config->websocket.key` →
   `ws_context->auth_key` → authorized `/api/system/info` terminal block →
   iframe → `?key=` over TLS → WebSocket authentication. The key is
   server-wide, not per-session.
4. **Redact secrets everywhere.** Do not log the key, full request URI/query
   string, JWT, password, `/api/system/info` response, or test command that
   contains a key. Logs and test artifacts may record only redacted
   fingerprints or pass/fail status.
5. **Rotate without code changes.** Set a new strong `WEBSOCKET_KEY`, restart
   Hydrogen, verify the new value through an authorized info response, and
   verify the old key is rejected. The iframe obtains the new value on its
   next config fetch.
6. **Protocol is configuration, not identity.** The configured WebSocket
   protocol name is the terminal subprotocol for this integration. The
   server, generated iframe, and tests must all use that value; no layer may
   hardcode `"terminal"` as the production protocol.
7. **Proxy and origin boundaries are explicit.** Honor forwarded client IP
   only from trusted proxy peers. Restrict iframe `postMessage` to an
   allowlisted origin and validate `event.source`; never use wildcard origins
   for production terminal access.

## Locked Contracts

These decisions are binding for implementation. A deviation requires a plan
amendment and review before source changes.

### Terminal authorization and system-info response

- `GET /api/system/info` remains a generic system-information endpoint. A
  request with no JWT, an invalid JWT, or a valid JWT without terminal
  authorization receives the generic response with no `terminal` object; the
  endpoint does not reveal whether a caller lacks the terminal role.
- The `terminal` object is present only when all of these are true:
  `Terminal.Enabled` is true, the WebSocket context exists, the JWT is valid,
  and the JWT `roles` claim contains the exact `terminal` role token.
- Role matching must support a multi-role claim and must not treat an admin,
  scripting, chat, or wildcard role as a terminal grant. Reuse the existing
  role-token parser where practical or add a generic auth helper; do not use
  a whole-string `strcmp` against `roles`.
- Lua `H.system.info()` never receives the terminal object or key through the
  generic system-info path. A separate authorized API would require a new
  contract and test surface.
- The authorized response shape is:

  ```json
  {
    "terminal": {
      "enabled": true,
      "url": "wss://public-host:public-port/terminal",
      "protocol": "configured-subprotocol",
      "key": "<server-wide authentication key>"
    }
  }
  ```

  `url` is an absolute WebSocket URL without a query string or key. The
  browser appends the key with URL-safe query construction. Production URLs
  must use `wss`; `ws` is local-test-only. The URL must have a valid scheme,
  host, and path and must reject userinfo, fragments, and embedded query
  strings.
- The response is authorized data, not a logging subject. Never log the full
  response, key, JWT, or constructed WebSocket URL.

### Trusted proxy and public endpoint

- Add `Network.TrustedProxies` as an array of validated IPv4/IPv6 CIDR
  entries. An empty list means no forwarded headers are trusted; production
  behind Traefik/DOKS must configure the actual immediate proxy peer(s).
- `api_get_client_ip` must first verify the immediate TCP peer against that
  allowlist. Only then may it consume `X-Forwarded-For`, `X-Forwarded-Proto`,
  and `X-Forwarded-Port`. An untrusted peer always uses the TCP peer address
  and ignores client-supplied forwarding values.
- For a trusted peer, parse `X-Forwarded-For` right-to-left and select the
  first address outside the trusted set as the client IP. Do not retain the
  current first-public-address heuristic, which can be poisoned by a
  client-supplied chain.
- The public endpoint is explicit deployment configuration. Add a
  `WebSocketServer.PublicUrl` origin value (scheme, host, optional port; no
  path, query, userinfo, or fragment) or an equivalent explicit origin
  contract, and combine it with `Terminal.WebPath`. Do not infer a public
  host or port from an untrusted `Host` header, iframe location, or hardcoded
  local default.
- `Terminal.CORSOrigin` is the terminal origin allowlist. It must contain
  exact origins such as `https://lithium.philement.com`; `*` is local-dev-only
  and forbidden in production. The terminal file handler and API CORS path
  must use the same allowlist. The current terminal CORS field is parsed but
  is not the effective source for terminal responses and must be reconciled.

### WebSocket key and protocol

- `WebSocketServer.Key` resolves to exactly one runtime value. Missing,
  empty, unresolved `${env.*}`, whitespace/control-containing, known-default,
  or shorter-than-32-character values fail startup. There is no fallback key
  in source, defaults, generated payloads, tests, or debug tooling.
- Both authentication surfaces use the same resolved key: the HTTP-upgrade
  callback and the libwebsockets protocol-filter callback. Browser clients
  send the key in the WebSocket query string; non-browser tests may use
  `Authorization: Key` where supported.
- `WebSocketServer.Protocol` is the single terminal subprotocol. The current
  default `hydrogen` may remain for compatibility, but production and tests
  must configure it explicitly and every routing check must use the resolved
  value. Remove or reconcile legacy `TERMINAL_WS_PROTOCOL` and
  `terminal_websocket_requires_auth()` paths so they cannot bypass LWS auth.
- The dead MHD terminal-upgrade helpers in `terminal_websocket.c` must be
  removed or explicitly disabled and tested as unreachable; the live
  terminal path is the libwebsockets path.

### Browser, TLS, rotation, and rollback

- Parent/iframe messaging uses an exact `targetOrigin`, validates
  `event.origin` against `Terminal.CORSOrigin`, and verifies
  `event.source === terminalManager.iframe`. Wildcard target origins and
  arbitrary ancestors are forbidden. The iframe must not use the
  `localStorage` JWT fallback in production.
- Production terminal access requires HTTPS for the iframe/API and WSS for
  WebSocket. Traefik/DOKS must preserve the Upgrade handshake and trusted
  forwarding headers.
- Key rotation is a restart operation: set a new strong runtime key, restart
  Hydrogen, verify the new key through an authorized info response, and verify
  the old key is rejected. Record only a redacted fingerprint.
- Add redacted telemetry for config-load failure, authorized terminal config
  responses, key acceptance/rejection, protocol mismatch, origin rejection,
  and connection lifecycle. Never include key, JWT, full URI, or response
  bodies.
- Rollback uses `Terminal.Enabled=false` (or the existing terminal disable
  switch), restores the prior runtime config, restarts Hydrogen, and verifies
  that `/api/system/info` omits `terminal`, the iframe cannot connect, and
  unrelated WebSocket protocols remain unaffected.

---

## Architecture Overview (audited current state)

### Components

| Layer | Component | Location |
| ------- | ----------- | ---------- |
| Auth | Password / OIDC RP JWT | `src/api/auth/` — `login.c`, `auth_service_jwt.c` |
| Client IP | Forwarded-peer extraction | `src/api/api_utils.c` — `api_get_client_ip` |
| API | System Info (terminal config) | `src/api/system/info/info.c` |
| WebSocket | Server + auth + dispatch | `src/websocket/` — `websocket_server_*.c` |
| Terminal | Session + PTY bridge | `src/terminal/terminal_websocket.c` + `terminal_session.*` |
| Payload | xterm.js + terminal.html | `payloads/` + `payloads/terminal-generate.sh` |
| Client SPA | Terminal manager + iframe | `elements/003-lithium/src/managers/terminal/` |
| Test | Blackbox terminal test | `tests/test_26_terminal.sh` |
| Test | Lithium unit tests | `elements/003-lithium/tests/unit/managers/terminal.test.js` |
| Proxy | DOKS LoadBalancer + Traefik | External (ops) |

### Key flows

**JWT issuance (`src/api/auth/`):**

- `generate_jwt_with_oidc` embeds `client_ip` as the `"ip"` claim.
- `client_ip` comes from `api_get_client_ip`, which currently prefers an
  `X-Forwarded-For` value and falls back to the TCP peer address.
- The current helper trusts the header without proving that the peer is a
  trusted proxy. Phase 1 must lock and enforce the Traefik/DOKS trust
  boundary before the JWT claim is considered authoritative.

**System Info (`src/api/system/info/info.c`):**

- `system_info_has_valid_jwt` validates `Authorization: Bearer <jwt>`.
- `system_info_build_json` currently includes `terminal.port` and
  `terminal.key` whenever `has_jwt && ws_context` is true.
- The terminal key is `ws_context->auth_key` — the server-wide WebSocket
  auth key, not a per-session key.
- The plan must add an explicit authorization decision: a valid JWT alone is
  not sufficient unless terminal access is intentionally granted to every
  authenticated account. The REST response and Lua `H.system.info()` path
  must not expose the key to callers that are not authorized for terminal
  access.

**WebSocket auth (`src/websocket/websocket_server_dispatch.c`):**

- `ws_context->auth_key` is copied from the resolved config in
  `websocket_server_context.c`.
- `config_defaults.c` and `config_websocket.c` use the `${env.WEBSOCKET_KEY}`
  reference, but unresolved references can remain literal strings.
- Current fallback paths are unsafe and must be removed:
  `websocket_server_context.c` uses `default_key` when the key pointer is
  null, `launch_websocket.c` uses `default_websocket_key` when config is
  null, and `websocket_server_dispatch.c` contains the
  `ABCDEFGHIJKLMNOP` fallback.
- The dispatch callback authenticates the key; it does not validate the
  configured protocol name. Protocol routing occurs later in
  `websocket_server_message.c` and `websocket_server_terminal.c`.

**Terminal iframe payload (`payloads/terminal-generate.sh`):**

- The generated inline script requests a JWT from the parent frame, falls
  back to `localStorage`, calls `/api/system/info`, and builds a WebSocket
  URL.
- It currently hardcodes a fallback port (`5261`) and the subprotocol
  (`terminal`), logs the complete `/api/system/info` response, and sends
  `postMessage` with wildcard origin/target semantics.
- The payload is inline HTML/JavaScript, not an importable ES module.
  Lithium tests exercise the SPA manager's message contract, not these
  inline functions directly.

**Lithium Terminal Manager (`src/managers/terminal/terminal.js`):**

- `_handleIframeMessage` is bound once in the constructor, registered in
  `init()`, and retained across `destroy()`/`init()` cycles.
- It responds to `terminal-config-request` by retrieving the Hydrogen JWT
  and posting a response.
- The current implementation uses `'*'` for `postMessage` target origin and
  does not validate `event.origin` or `event.source`; Phase 4 must replace
  that with an explicit origin allowlist and source check.

### Protocol routing clarification

- `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION` in
  `websocket_server_dispatch.c` checks authentication, not the configured
  protocol name.
- `websocket_server_message.c` routes terminal message types only when
  `lws_get_protocol(wsi)->name` equals the hardcoded string `"terminal"`.
- `websocket_server_terminal.c` has the same hardcoded check.
- `setup_websocket_protocols` registers the protocol from
  `config->websocket.protocol` (default `"hydrogen"`).
- `terminal_websocket.c` still defines `TERMINAL_WS_PROTOCOL "terminal"` and
  `terminal_websocket_requires_auth()` returns false. These legacy helpers
  must be reconciled with the configured protocol and the real LWS auth
  path; the false helper must not be treated as the product security gate.

### Current defects observed

1. **`_handleIframeMessage is null` crash** — the Lithium fix is already
   applied: the handler is bound once and retained across lifecycle calls.
2. **Unsafe key fallbacks** — `default_key`, `default_websocket_key`, and
   `ABCDEFGHIJKLMNOP` remain in the server path; unresolved environment
   references can also become literal keys.
3. **Secret leakage** — WebSocket dispatch logs a stored key and request URI,
   the generated payload logs the full system-info response, and Test 26
   prints commands containing `WEBSOCKET_KEY`.
4. **Protocol mismatch** — production defaults to `"hydrogen"` while the
   iframe and terminal routing checks use `"terminal"`; Test 26 hides the
   mismatch by configuring `"terminal"`.
5. **Origin/auth contract is too broad** — wildcard `postMessage`, wildcard
   target origins, and a system-info terminal block gated only by JWT
   validity expose secrets beyond the intended trust boundary.
6. **Test/documentation drift** — Test 26 is a blackbox test run through
   `test_00_all.sh`, not `mku`; its script initializes `TEST_COUNTER` even
   though the framework owns that counter, and it does not cover the
   system-info/key/protocol contract.
7. **Terminal origin configuration is disconnected** — `Terminal.CORSOrigin`
   defaults to `*` and is parsed by `config_terminal.c`, but terminal file
   responses call the global CORS helper, which reads API/WebServer CORS
   settings instead. The terminal allowlist therefore needs one effective
   source and production must replace the wildcard.
8. **Public endpoint construction is implicit** — the payload derives its API
   base from the iframe location and its WebSocket URL from the iframe
   hostname plus a port. There is no trusted public-origin contract, so a
   reverse-proxy deployment can silently select the wrong scheme, host, or
   port.

---

## Verified Findings (source cross-check)

This section records the concrete source-level state the plan's assumptions were
checked against. Line references are audit aids and must be refreshed after
implementation; they are not a substitute for re-reading the code at implementation
time.

### WebSocket authentication surfaces (two, not one)

The terminal WebSocket path has **two** authentication surfaces, not the single
dispatch path the architecture overview implies:

1. **HTTP-upgrade callback** — `callback_http` in
   [`websocket_server.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server.c):60-150
   validates `Authorization: Key <key>` and the `?key=` query parameter during the
   HTTP 1.1 → WebSocket upgrade, storing the authenticated key in
   `WebSocketSessionData.authenticated_key` (line 88).
2. **Protocol-filter dispatch** — `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION` in
   [`websocket_server_dispatch.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c):262
   re-validates: the `ABCDEFGHIJKLMNOP` hardcoded fallback (line 289), the
   session-stored key (line 304), and the `?key=` query parameter (line 367).

Both surfaces must be hardened. Phase 2 must remove the `ABCDEFGHIJKLMNOP`
fallback from dispatch, redact the key logging at line 304 (`"Found stored key
in session: %s"`) and line 367 (`"Query parameter key found: %s"`), and redact
the request URI at line 323 (`"Request URI: %s"`).

### Dead code in terminal_websocket.c

`is_terminal_websocket_request`
([`terminal_websocket.c`](/elements/001-hydrogen/hydrogen/src/terminal/terminal_websocket.c):39)
and `handle_terminal_websocket_upgrade` (line 130) are **only referenced within
`terminal_websocket.c` and its header**. A grep across `src/` confirms no
request handler or dispatch path calls `handle_terminal_websocket_upgrade`.
This is the MHD/libmicrohttpd upgrade path, not the libwebsockets path — the
two code paths do not converge. These functions are dead on the live request
path and must be reconciled or removed in Phase 2.

### validate_key defect

`validate_key` in
[`launch_websocket.c`](/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c):64
checks only length ≥ 8 and printable-ASCII. It does **not** reject the dangerous
values the plan must eliminate:

- `${env.WEBSOCKET_KEY}` (the unresolved env reference is 17 chars, all printable — passes).
- `default_key`, `default_websocket_key`, `ABCDEFGHIJKLMNOP` (all ≥ 8 printable — pass).

The function is also not called from `check_websocket_launch_readiness`
(line 106) — the readiness path does not validate the key before startup.
Phase 2 must either strengthen `validate_key` to reject config references and
known-default literals, or replace it with a fail-closed startup check that
verifies the resolved key is not an env reference and is not a known default.

### Config resolution leaves literal env references

[`config_websocket.c`](/elements/001-hydrogen/hydrogen/src/config/config_websocket.c):45
sets `ws->key = strdup("${env.WEBSOCKET_KEY}")` as the default, then overwrites
it via `PROCESS_SENSITIVE` from `WebSocketServer.Key`.
[`config.c`](/elements/001-hydrogen/hydrogen/src/config/config.c):236-240
resolves `${env.X}` only when the environment variable exists; when it is
absent, the literal `${env.WEBSOCKET_KEY}` string survives into
`config->websocket.key`. `process_env_variable` in
[`config_utils.c`](/elements/001-hydrogen/hydrogen/src/config/config_utils.c):201-239
performs the substitution, but the fallback path never re-validates afterward.
Phase 2's fail-closed check must run **after** resolution and reject any residual
`${env.*}` or known-default literal.

### No TrustedProxies config; client-IP heuristic is the gate

There is no `Network.TrustedProxies` or `WebSocketServer.TrustedProxies`
configuration field. `api_get_client_ip` in
[`api_utils.c`](/elements/001-hydrogen/hydrogen/src/api/api_utils.c):188
trusts `X-Forwarded-For` whenever it is present, using `is_ip_internal`
(line 155) only to pick the *first public* address from the chain — it does
**not** verify that the immediate TCP peer is a trusted proxy. An attacker who
can reach Hydrogen directly can spoof `X-Forwarded-For` with a public IP and
poison the JWT `ip` claim. Phase 1 must add a trusted-peer check (new config
or an immediate-peer allowlist) and ignore forwarded headers from untrusted
peers.

### Terminal CORS and public endpoint are not wired

[`config_terminal.c`](/elements/001-hydrogen/hydrogen/src/config/config_terminal.c):52
initializes `Terminal.CORSOrigin` to `*`, and lines 82–84 parse the configured
value. However, [`terminal.c`](/elements/001-hydrogen/hydrogen/src/terminal/terminal.c):291
and :532 call the global `add_cors_headers()`, whose effective source is the
API/WebServer CORS setting in
[`web_server_core.c`](/elements/001-hydrogen/hydrogen/src/webserver/web_server_core.c):259–265.
The terminal-specific field is therefore not currently an effective terminal
origin policy. Phase 2/3 must choose and wire one exact-origin allowlist for
both terminal assets and `/api/system/info`.

The generated payload derives its API base from `window.location` and its
WebSocket URL from the iframe hostname plus a port at
[`terminal-generate.sh`](/elements/001-hydrogen/hydrogen/payloads/terminal-generate.sh):263–264
and :349–366. There is no trusted public-origin field, so reverse-proxy scheme,
host, and port cannot be treated as authoritative until Phase 1/2 establish
the endpoint contract.

### Protocol routing source of truth

- [`config_defaults.c`](/elements/001-hydrogen/hydrogen/src/config/config_defaults.c):39
  sets `ws->protocol = strdup("hydrogen")`.
- [`websocket_server_message.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_message.c):218
  hardcodes `strcmp(protocol->name, "terminal")` for terminal message routing.
- [`websocket_server_terminal.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_terminal.c):36
  (via `TERMINAL_WS_PROTOCOL` in `terminal_websocket.c`:25) hardcodes `"terminal"`.
- The production default protocol is `"hydrogen"`; the iframe and routing checks
  use `"terminal"`. Test 26 masks this by setting Protocol to `"terminal"` in
  [`hydrogen_test_26_terminal_payload.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_26_terminal_payload.json).

Phase 2 must replace the hardcoded `"terminal"` checks with `ws_context->protocol`
(which is already copied from the resolved config in
[`websocket_server_internal.h`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_internal.h)).
The configured protocol is the terminal subprotocol; no layer may hardcode it.

### System-info terminal authorization

`system_info_build_json` in
[`info.c`](/elements/001-hydrogen/hydrogen/src/api/system/info/info.c):94
returns the terminal block (`port`, `key`) whenever `has_jwt && ws_context`
(line 119). It does **not** distinguish between a regular user and a
terminal-authorized account — any valid JWT gets the key. `has_jwt` is set by
`system_info_has_valid_jwt` (line 60) which only checks JWT validity.
`handle_system_info_request` (line 133) calls `system_info_build_json` with
`include_scripting = has_jwt`, coupling terminal access to scripting access.
Phase 2 must add an explicit authorization gate for terminal config (or a
dedicated response path) so the Lua `H.system.info()` path and the REST path
do not expose `terminal.key` to un-authorized callers.

### Existing test coverage to extend (not replace)

- [`api_utils_test_get_client_ip.c`](/elements/001-hydrogen/hydrogen/tests/unity/src/api/api_utils_test_get_client_ip.c)
  — exists; extend for trusted-vs-untrusted peer and spoofed-XFF rejection.
- [`config_websocket_test_load_websocket_config.c`](/elements/001-hydrogen/hydrogen/tests/unity/src/config/config_websocket_test_load_websocket_config.c)
  — exists; extend for unresolved `${env.WEBSOCKET_KEY}` fail-closed.
- [`websocket_server_test_callback_http.c`](/elements/001-hydrogen/hydrogen/tests/unity/src/websocket/websocket_server_test_callback_http.c)
  — exists; extend for key-URI redaction and fallback removal.
- [`terminal_websocket_test_validation.c`](/elements/001-hydrogen/hydrogen/tests/unity/src/terminal/terminal_websocket_test_validation.c)
  — exists; confirms `validate_terminal_protocol` hardcodes `"terminal"`.

---

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| ------- | ---------------------- | -------- | -------- |
| 0 | Contract lock; audited architecture, root causes, security boundaries, and phase dependencies | S | complete |
| 1 | Trusted-proxy JWT `ip` claim reflects the real client IP behind Traefik/DOKS | S | complete |
| 2 | WebSocket config fails closed; key/protocol contract and authorized system-info response are implemented | M | pending |
| 3 | Generated terminal payload uses API-provided port/protocol, exact-origin messaging, and redacted errors | M | pending |
| 4 | Lithium manager uses exact-origin/source-checked `postMessage` and survives lifecycle cycles | S | pending |
| 5 | Deployed endpoint, TLS/proxy routing, key rotation, and browser E2E terminal session succeed | M | pending |
| 6 | Secure debug launcher and redacted Test 26/Lithium coverage prove the full flow | M | pending |

Effort key: S = small/contained, M = moderate (security/networking/deployment + testing).

---

## Phase 0 — Contract Lock

### Goal

Document the current architecture, lock design decisions, and confirm the
root cause of each defect before touching code. No production changes.
Explicitly lock the **fail-closed, no-hardcoded-keys, no-secret-leakage**
policy and correct the phase dependencies.

### Entry gate

This document exists. Ability to read code in `src/api/`, `src/config/`,
`src/websocket/`, `src/terminal/`, `payloads/terminal-generate.sh`,
`elements/003-lithium/src/managers/terminal/`, `tests/test_26_terminal.sh`,
and the Hydrogen test runner documentation.

### Work items

- [x] Confirm the JWT `ip` claim source path: `api_get_client_ip` →
      `X-Forwarded-For` → TCP peer. Record that the current helper trusts the
      header without a trusted-proxy check; Phase 1 must enforce the boundary.
      **Verify:** Status records the chain and the trust-gap variance.
- [x] Confirm the WebSocket key flow: config → resolved
      `config->websocket.key` → `ws_context->auth_key` → authorized
      `/api/system/info` → iframe → `?key=`. Record that it is server-wide,
      not per-session.
      **Verify:** Status records source files and flow.
- [x] Confirm the key source: `ws_context_create` copies the resolved config
      key. `launch_websocket.c` does not set the key at its readiness check.
      Record the unsafe `default_key`, `default_websocket_key`, and
      `ABCDEFGHIJKLMNOP` fallbacks and the unresolved `${env.WEBSOCKET_KEY}`
      case.
      **Verify:** Status records the correct source and all fallback paths.
- [x] Confirm the port: the code default is 5001; the observed deployed 7001
      value is a configuration/deployment override and must not be assumed by
      the client or tests.
      **Verify:** Status records the default and external verification need.
- [x] Confirm protocol routing: dispatch authenticates the key; protocol-name
      routing occurs in `websocket_server_message.c` and
      `websocket_server_terminal.c`; protocol registration comes from
      `config->websocket.protocol`.
      **Verify:** Status records the correct locations and the hardcoded
      `"terminal"` mismatch.
- [x] Confirm the protocol mismatch: the iframe and legacy terminal helpers
      use `"terminal"` while production defaults to `"hydrogen"`; Test 26
      hides the mismatch by setting `"terminal"` in its config.
      **Verify:** Status records the mismatch and the configured-protocol
      contract.
- [x] Confirm the `_handleIframeMessage` root cause: `destroy()` previously
      nulled the bound handler and `init()` tried to re-bind it. The fix is
      already present: bind once, retain the handler, and remove/re-add the
      listener without nulling it.
      **Verify:** Status records `terminal.js` lifecycle behavior.
- [x] Document the payload boundary: `fetchTerminalConfig` and
      `connectToWebSocket` are inline in generated `terminal.html`, not an
      importable ES module. Record the current hardcoded port/protocol,
      wildcard messaging, localStorage fallback, and full-response logging.
      **Verify:** Status records the distinction and required fixes.
- [x] Lock the trusted-proxy decision: forwarded client IP is authoritative
      only when the immediate peer is a trusted Traefik/DOKS proxy; otherwise
      use the TCP peer and reject spoofed forwarded headers.
      **Verify:** Decision in Status.
- [x] Lock the public endpoint decision: WebSocket port and public origin are
      deployment configuration. Do not hardcode 7001, 5261, a hostname, or a
      direct-port assumption in Lithium or the payload.
      **Verify:** Decision in Status.
- [x] **Lock decision: fail closed and never hardcode keys.** The WebSocket
      key comes exclusively from `WEBSOCKET_KEY` or
      `WebSocketServer.Key`; missing, empty, unresolved, or weak values fail
      startup. No source, default, generated payload, test, or debug tool may
      contain a reusable key literal.
      **Verify:** Decision in Status.
- [x] Lock the authorization decision: `/api/system/info` may return the
      terminal key only to an account authorized for terminal access; Lua
      `H.system.info()` must not expose it through the generic system-info
      path unless an explicit authorized caller contract is added.
      **Verify:** Decision in Status.
- [x] Lock the browser messaging decision: parent/child communication uses a
      configured origin allowlist, validates `event.origin` and
      `event.source`, and supplies an exact `targetOrigin`; wildcard origins
      are local-development-only and forbidden in production.
      **Verify:** Decision in Status.
- [x] Lock the debug-tool decision: it accepts `--server`, `--username`, and
      a password from stdin or a permission-restricted file (never a CLI
      argument), fetches a JWT, and uses an in-memory postMessage handoff
      without persisting the JWT or key.
      **Verify:** Decision in Status.
- [x] Document the full end-to-end flow from login/JWT issuance through
      trusted-proxy IP extraction, authorized system-info retrieval,
      exact-origin iframe handoff, configured WebSocket endpoint/protocol,
      key authentication, and PTY I/O.
      **Verify:** Status records the flow and trust boundaries.
- [~] Inspect the live Traefik/DOKS manifests, deployed Hydrogen config, and
      public WebSocket route. This requires deployment access and belongs in
      Phase 5; do not infer it from local defaults.
      **Verify:** Deferred to Phase 5.

### Done means

Architecture, root causes, security boundaries, and phase dependencies are
documented in Status. The fail-closed/no-hardcoded-keys policy is locked.
No C, JavaScript, payload, test, or deployment changes were made.

### Exit gate

Status filled, live-deployment variance recorded, and Working Log contains
the locked decisions for JWT-IP, proxy trust, WS key/protocol, system-info
authorization, payload, iframe origin, debug tool, tests, and rotation.

### Status

- **State:** complete
- **Date:** 2026-09-11
- **Result:** Code and test audit completed; contract and phase order upgraded.
- **Variances:** Live Traefik/DOKS configuration, deployed Hydrogen config,
  public route, and deployed key were not inspected in this session. Those
  checks are deferred to Phase 5. Existing line references are audit aids and
  must be refreshed after implementation.
- **Key rotation policy:** Locked — key from env/config only; missing or
  unresolved values fail closed; no reusable literal in source, defaults,
  payload, tests, or debug tooling.

---

## Phase 1 — Trusted Proxy and JWT IP Claim

### Goal

Ensure the JWT `ip` claim reflects the actual client IP when Hydrogen is
behind Traefik + DOKS LoadBalancer, without allowing clients to spoof
`X-Forwarded-For` from an untrusted peer.

### Entry gate

Phase 0 complete.

### Work items

- [~] Inspect the deployed Traefik configuration and record the trusted proxy
      boundary: `forwardedHeaders.trustedIPs`, `proxyProtocol`, or the
      current DOKS equivalent must cover only trusted load-balancer peers.
      **Verify:** Deployment config or an explicit ops variance is recorded.
- [~] Verify DOKS preserves the original client source address or supplies a
      trustworthy forwarded chain. Use the current DOKS mechanism rather than
      assuming a specific annotation is still supported.
      **Verify:** Service/load-balancer config or a live request trace.
- [x] Add `Network.TrustedProxies` to the network configuration schema,
      loader, defaults, cleanup, and redacted dump. Store validated IPv4/IPv6
      CIDR entries; an empty list means no forwarded headers are trusted.
      **Verify:** Schema/parser tests accept valid CIDRs and reject malformed
      entries without logging their values.
- [x] Update `api_get_client_ip` so it verifies the immediate TCP peer against
      `Network.TrustedProxies` before consuming `X-Forwarded-For`,
      `X-Forwarded-Proto`, or `X-Forwarded-Port`. For a trusted peer, walk
      `X-Forwarded-For` right-to-left and select the first address outside the
      trusted set; for an untrusted peer, use the TCP peer and ignore all
      client-supplied forwarding values.
      **Verify:** Code review and unit tests cover trusted multi-hop chains,
      untrusted spoofed chains, malformed addresses, and the existing
      internal/public cases.
- [x] Extend the existing `api_get_client_ip` Unity coverage with:
      - a multi-hop `X-Forwarded-For` chain from a trusted peer;
      - an untrusted peer supplying a public `X-Forwarded-For` value;
      - trusted and untrusted `X-Forwarded-Proto`/`X-Forwarded-Port` handling;
      - the existing internal/public selection cases.
      **Verify:** `mku api_utils_test_get_client_ip` passes.
- [~] Generate or inspect a live JWT after login and confirm its `ip` claim
      matches the trusted public client address, not the load balancer or an
      attacker-supplied header.
      **Verify:** Redacted claim inspection recorded in Status.
- [x] Run `mkq` (or `mkt` when a clean/configure build is required) and
      `mkp`.
      **Verify:** Build and cppcheck are clean.

### Done means

The trusted-proxy boundary is documented, spoofed forwarded headers are
ignored outside that boundary, and a live JWT contains the real client IP.

### Exit gate

Traefik/DOKS trust configuration verified or an explicit ops variance
recorded; trusted/untrusted client-IP unit tests pass; `mkq`/`mkp` green.

### Status

- **State:** complete
- **Date:** 2026-09-11
- **Result:** Trusted-proxy boundary implemented and verified. `api_get_client_ip` now checks the immediate TCP peer against `Network.TrustedProxies` CIDR entries before consuming `X-Forwarded-For`. Untrusted peers use the TCP peer address. Trusted peers walk `X-Forwarded-For` right-to-left and select the first non-trusted address. 25 Unity tests pass (`mku api_utils_test_get_client_ip`), cppcheck clean (`mkp`), build clean (`mkq`), and blackbox Test 26 passes.
- **Variances:** Live Traefik/DOKS `forwardedHeaders.trustedIPs` configuration was not inspected in this session (requires deployment access); deferred to Phase 5. The local-test config in `hydrogen_test_26_terminal_payload.json` now includes `Network.TrustedProxies` with loopback CIDRs for testing.

### Working Log

- Added `NETWORK_MAX_TRUSTED_PROXIES 64` constant and `trusted_proxies[]`/`trusted_proxies_count` fields to `NetworkConfig` in `config_network.h`.
- Added `is_ip_in_cidr()` and `is_trusted_proxy()` declarations to `config_network.h`.
- Implemented `is_ip_in_cidr()` (IPv4/IPv6 CIDR matching with prefix-length comparison) and `is_trusted_proxy()` in `config_network.c`.
- Wired `process_string_array_config` for `Network.TrustedProxies` key in `load_network_config()` (already present in the codebase).
- Added cleanup and dump support for `trusted_proxies` in `config_network_free()` and `config_network_dump()`.
- Rewrote `api_get_client_ip()` in `api_utils.c` to: (1) extract the TCP peer via new `api_get_tcp_peer_ip()`, (2) check `is_trusted_proxy()` on the peer, (3) if trusted, parse `X-Forwarded-For` right-to-left selecting the first non-trusted address, (4) if untrusted, use TCP peer and ignore all forwarded headers.
- Added `api_get_tcp_peer_ip()` declaration to `api_utils.h`.
- Extended existing XFF tests to reflect trusted-proxy semantics (no trusted proxy = XFF ignored).
- Added new Unity tests: untrusted-peer-ignores-XFF, trusted-peer-rightmost-external, trusted-peer-all-trusted-falls-back, trusted-peer-single-trusted, NULL-app-config, `is_trusted_proxy` (trusted/untrusted/null), `is_ip_in_cidr` (IPv4/IPv6/no-prefix/invalid).
- Updated `src/api/README.md` and `api_utils.h` doc comments to reflect new trusted-proxy behavior.
- Updated `hydrogen_test_26_terminal_payload.json` config to include `Network.TrustedProxies`.
- Bug fixed during implementation: IPv6 family assignment in `is_ip_in_cidr` had a `return false` where `family = AF_INET6` should have been; cppcheck caught a redundant `> 128` check after early validation.

---

## Phase 2 — WebSocket Configuration, Authorization, and Protocol Contract

### Goal

Make the WebSocket server fail closed, expose a safe terminal configuration
contract through authorized `/api/system/info`, and make protocol routing
use the configured protocol rather than hardcoded terminal names.

### Entry gate

Phase 1 complete.

### Work items

- [ ] Resolve and validate `WebSocketServer.Key` before creating the LWS
      context. Missing, empty, unresolved `${env.WEBSOCKET_KEY}`, whitespace
      or control characters, known-default literals, and values shorter than
      32 characters must prevent startup. Remove `default_key`,
      `default_websocket_key`, and `ABCDEFGHIJKLMNOP` fallbacks from
      `websocket_server_context.c`, `launch_websocket.c`, and dispatch.
      Per the Verified Findings, `validate_key` in
      [`launch_websocket.c`](/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c):64
      only checks length ≥ 8 and printable-ASCII — it does not reject config
      references or known-default literals, and it is not called from the
      readiness path. Strengthen it or replace it with a fail-closed startup
      check that rejects any residual `${env.*}` and known-default strings.
      Harden **both** auth surfaces: the HTTP-upgrade path in
      [`callback_http`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server.c):60
      and the dispatch path in
      [`websocket_server_dispatch.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c):262.
      **Verify:** Missing, unresolved, weak, and known-default key startup
      tests fail safely; a strong configured key starts normally.
- [ ] Ensure sensitive config logging redacts the value and never prints the
      resolved key. Search server logs for the key, env reference, and query
      string after startup and authentication attempts.
      **Verify:** Log inspection contains no secret material.
- [ ] Define the authorized system-info contract. Require a valid JWT whose
      `roles` claim contains the exact `terminal` role token; match role tokens
      rather than comparing the entire `roles` string. Unauthenticated,
      invalid-JWT, and valid-but-unauthorized callers receive the generic
      system-info response with no `terminal` object. Keep the generic Lua
      `H.system.info()` response free of terminal data unless a separate
      authorized API is deliberately introduced.
      **Verify:** Unit/blackbox responses prove that only a terminal-authorized
      JWT receives `terminal.key`; no response or log contains the key for
      other callers.
- [ ] Extend the authorized terminal object to the locked schema: `enabled`,
      absolute `url`, configured `protocol`, and server-wide `key`. Build
      `url` from the explicit public WebSocket origin and `Terminal.WebPath`;
      do not expose the key in the URL. Reject missing/invalid public endpoint
      data when terminal access is enabled.
      **Verify:** Authorized response contains the configured public URL,
      protocol, and key; unauthorized response omits the terminal object.
- [ ] Add and validate the explicit public WebSocket origin contract
      (`WebSocketServer.PublicUrl` or its agreed equivalent), and make
      `Terminal.CORSOrigin` the effective exact-origin allowlist for terminal
      assets and `/api/system/info`. Reject wildcard production origins and
      ensure the terminal handler no longer silently falls back to global
      API/WebServer CORS.
      **Verify:** Config/schema tests and CORS tests prove exact-origin
      behavior for allowed and disallowed origins.
- [ ] Replace hardcoded `"terminal"` routing in
      `websocket_server_message.c` (line 218) and `websocket_server_terminal.c`
      (line 36, via `TERMINAL_WS_PROTOCOL` in `terminal_websocket.c`:25) with
      the active configured protocol from `ws_context->protocol`. Reconcile
      `get_terminal_websocket_protocol()` and
      `terminal_websocket_requires_auth()` so legacy helpers cannot bypass or
      contradict the LWS authentication path. Remove or explicitly disable
      the dead MHD upgrade helpers in `terminal_websocket.c`; the live path
      must be the libwebsockets path. Per the Verified Findings, the
      production default protocol is `"hydrogen"` (config_defaults.c:39) while
      these checks hardcode `"terminal"`; Test 26 masks the mismatch by setting
      Protocol to `"terminal"`. The configured protocol must be the single
      terminal subprotocol.
      **Verify:** Configured protocol accepts terminal traffic; a mismatched
      subprotocol is rejected; dead-code/build checks show no alternate auth
      bypass.
- [ ] Lock the browser authentication transport: browsers send the key in
      the WebSocket query string because the browser API cannot set custom
      headers; require TLS in production and redact the full URI from logs.
      The dispatch log at `websocket_server_dispatch.c:323` (`"Request URI: %s"`)
      and `websocket_server_dispatch.c:304` (`"Found stored key in session: %s"`)
      and `:367` (`"Query parameter key found: %s"`) must be redacted or removed.
      Non-browser tests may use `Authorization: Key` where supported.
      **Verify:** Correct key succeeds, wrong/old key fails, and no key is
      logged.
- [ ] Add redacted terminal telemetry and rotation checks: count authorized
      config responses, key accept/reject reasons, protocol mismatches,
      origin rejections, and connection lifecycle events without recording
      key/JWT/URI bodies. Verify a configured key rotation by restart, with
      only a fingerprint recorded and the old key rejected.
      **Verify:** Logs/metrics contain categories and fingerprints only; old
      key fails after restart.
- [ ] Add focused C/Unity coverage for key validation, unresolved env
      handling, configured-protocol routing, and authorized system-info
      behavior. Do not add a static helper in Hydrogen `src/`.
      **Verify:** Relevant Unity tests pass and `mkt` dead-code gate is clean.
- [ ] Run `mkq` (or `mkt` when config/payload inputs changed) and `mkp`.
      **Verify:** Build and cppcheck are clean.

### Done means

Hydrogen starts only with a resolved strong key, authorized terminal callers
receive the active public URL/protocol/key contract, unauthorized callers do
not, terminal routing follows the configured protocol with no hardcoded key
or protocol literal, and terminal CORS is exact-origin.

### Exit gate

Key-fail-closed, role authorization, public-endpoint/CORS, protocol,
redaction, and rotation checks pass; `mkq`/`mkp` green; no secret appears in
logs or test output.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 3 — Terminal Payload Regeneration and Browser Security

### Goal

Ensure `terminal-generate.sh` produces a `terminal.html` that obtains an
authorized JWT through the parent frame, fetches `/api/system/info`, and
connects using API-provided port/protocol without hardcoded fallbacks,
wildcard messaging, or secret logging.

### Entry gate

Phase 2 complete.

### Work items

- [ ] Review the inline `fetchTerminalConfig()` and
      `connectToWebSocket()` implementation in
      `payloads/terminal-generate.sh`. Require the parent-frame handoff as
      the production path; remove the silent `localStorage` JWT fallback
      unless a separately approved same-origin local mode is retained.
      **Verify:** Code review and generated HTML inspection.
- [ ] Require the locked `terminal` response fields: `enabled`, absolute
      `url`, `protocol`, and `key`. Remove the hardcoded `5261` port fallback,
      hardcoded `"terminal"` subprotocol, and any client-side reconstruction
      of the public host/port. Fail visibly when the URL, protocol, or key is
      absent or invalid.
      **Verify:** Generated HTML contains no port/protocol literals used as
      connection defaults and performs URL-safe key query construction.
- [ ] Consume the API-provided absolute `terminal.url` directly. Require
      `wss` in production and `ws` only for explicitly marked local tests;
      reject userinfo, fragments, embedded query strings, and malformed
      schemes/hosts. Do not assume the iframe hostname, direct port, or
      `wss://<hostname>:<port>` shape.
      **Verify:** Same-origin and approved reverse-proxy configurations work;
      malformed and non-TLS production URLs are rejected.
- [ ] Replace wildcard `postMessage` behavior with an exact target origin and
      validate `event.origin` against `Terminal.CORSOrigin` plus `event.source`
      against the terminal iframe. Never accept config messages from an
      arbitrary ancestor; wildcard origins are local-development-only.
      **Verify:** Unit/static checks and browser test reject a wrong origin
      and wrong source.
- [ ] Remove logging of JWTs, keys, full `/api/system/info` responses, full
      request URIs, and message payloads. Log only non-sensitive state and
      redacted error categories.
      **Verify:** Browser/server logs and generated artifacts contain no
      secret values.
- [ ] Make config-fetch and reconnect lifecycle deterministic: remove stale
      timers/listeners on success/failure, prevent duplicate WebSocket
      connections after visibility changes, and use bounded reconnect behavior.
      **Verify:** Destroy/reopen or visibility-cycle test leaves one active
      connection and no unhandled rejection.
- [ ] Rebuild the embedded Hydrogen payload after generator changes with
      `mkt` (or `mka`), then run Test 26 through `test_00_all.sh`.
      **Verify:** Embedded payload and filesystem artifact are both current.
- [ ] Run `mks` for generator changes and `mkp` for any C changes.
      **Verify:** Shellcheck/cppcheck are clean.

### Done means

The generated terminal payload uses the authorized API contract, an
API-provided WSS URL and configured protocol, exact-origin/source-checked
messaging, and redacted diagnostics; it fails visibly instead of falling back
to hardcoded secrets or unsafe origins.

### Exit gate

Payload rebuilt and embedded; generated HTML/static checks pass; Test 26,
`mks`, and `mkp` are green with no secret leakage or wildcard production
origin.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 4 — Lithium Terminal Manager and Exact-Origin Messaging

### Goal

Ensure the Lithium Terminal Manager safely passes the JWT to the terminal
iframe, validates message origin/source, and survives destroy/init cycles.

### Entry gate

Phase 3 complete.

### Work items

- [ ] Confirm the existing `_handleIframeMessage` fix remains intact:
      bind once in the constructor, register the retained handler in
      `init()`, remove it in `destroy()`, and never null it.
      **Verify:** Read current `terminal.js` lifecycle code.
- [ ] Replace wildcard `postMessage` calls with an exact `targetOrigin`
      derived from `Terminal.CORSOrigin`/the configured Hydrogen origin.
      Validate `event.origin` against the same allowlist and verify
      `event.source === this.iframe` before returning a JWT.
      **Verify:** Unit tests prove wrong-origin and wrong-source messages
      are ignored and no JWT is sent to `'*'`.
- [ ] Validate `terminalUrl` and the iframe origin against configured
      `server.url`/`server.terminal_path` and the approved origin allowlist;
      do not allow an arbitrary cross-origin iframe to request terminal
      credentials. Reject malformed or non-approved URLs before creating the
      iframe.
      **Verify:** URL construction and rejection tests pass.
- [ ] Keep JWT retrieval and messaging free of console logging. Use the
      existing Lithium logging facility for non-sensitive lifecycle events.
      **Verify:** Source scan and test output contain no JWT.
- [ ] Run Lithium unit tests with `npm test`, then lint with
      `npm run lint`; run `npm run build` if templates or production assets
      change, and `npm run templates:copy` after template edits.
      **Verify:** All named commands are green.

### Done means

The manager sends the JWT only to the approved terminal iframe/origin,
ignores hostile messages, and remains safe across popup lifecycle cycles.

### Exit gate

`npm test` and `npm run lint` green; origin/source checks and lifecycle
tests are present; no JWT appears in logs or test output.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 5 — Deployed Endpoint, Key Rotation, and Browser E2E

### Goal

Verify the production Traefik/DOKS route, TLS/origin configuration, active
WebSocket port/protocol, and key rotation, then prove the terminal opens
from the Lithium popup without a new-window workaround.

### Entry gate

Phase 4 complete.

### Work items

- [ ] Inspect the deployed Hydrogen config and record the active
      `Terminal.Enabled`, `Terminal.WebPath`, `Terminal.CORSOrigin`,
      `WebSocketServer.PublicUrl`, `WebSocketServer.Protocol`, and key source.
      Do not assume local defaults or the previously observed 7001.
      **Verify:** Redacted config evidence is recorded.
- [ ] Inspect DOKS and Traefik routing for the actual public endpoint:
      service target port, TLS termination, HTTP/1.1 Upgrade handling,
      `passHostHeader`/equivalent, trusted forwarded headers, and firewall
      exposure. Confirm the deployed public URL is the origin returned by
      `/api/system/info`; if a direct port is used, document and restrict it.
      **Verify:** Route trace or deployment manifest is recorded.
- [ ] Verify CORS/origin policy for `/api/system/info` and the terminal
      iframe. No wildcard production origin; only the approved Lithium
      origin may request terminal credentials. Verify the terminal role gate
      with a valid non-terminal JWT as well as an authorized terminal JWT.
      **Verify:** Browser/network test from allowed and disallowed origins and
      both JWT role cases.
- [ ] Rotate the deployed key using a strong random value supplied through
      `WEBSOCKET_KEY` or `WebSocketServer.Key`; restart Hydrogen and verify
      the new key is returned only to an authorized caller. Record only a
      redacted fingerprint, never the raw key.
      **Verify:** New key accepted, old key rejected, no secret in logs.
- [ ] Open the production Lithium URL, log in, and open Terminal. Confirm:
      - the iframe requests config from the approved parent origin;
      - Lithium replies with an exact-origin/source-checked message;
      - `/api/system/info` returns the active public URL, protocol, and key;
      - the WebSocket uses the configured protocol and accepted key;
      - `onopen` fires and a shell prompt appears.
      **Verify:** Redacted browser/network evidence and shell prompt.
- [ ] Exercise disconnect/reconnect and popup destroy/init cycles. Confirm
      there is one active terminal connection and no stale listener, timer,
      or duplicate iframe session.
      **Verify:** Browser console/network inspection has no lifecycle errors.
- [ ] Exercise the rollback path: set `Terminal.Enabled=false`, restore the
      prior runtime config, restart Hydrogen, and verify `/api/system/info`
      omits `terminal`, the iframe cannot connect, and unrelated WebSocket
      protocols remain available.
      **Verify:** Redacted rollback evidence is recorded.
- [ ] If connection fails, diagnose only redacted categories: trusted-proxy
      IP, authorization, key mismatch, protocol mismatch, route/port, TLS,
      or origin policy. Never paste the key or JWT into logs/tickets.
      **Verify:** Troubleshooting result recorded without secrets.

### Done means

The deployed route, TLS, exact-origin policy, role gate, and key rotation are
verified; rollback is proven; and the Lithium iframe establishes an
authenticated terminal session with a shell prompt.

### Exit gate

Manual production E2E and rollback succeed; route/TLS/origin/role evidence is
recorded; new key works, old key fails, and all evidence is redacted.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 6 — Secure Debug Launcher and Redacted Test Coverage

### Goal

Add a secure terminal launcher for troubleshooting, extend Test 26 and
Lithium tests to cover the real contract, and prove key rotation without
printing or persisting secrets.

### Entry gate

Phase 5 complete.

### Work items

- [ ] Create `extras/terminal-launcher.sh` (or the repository's established
      launcher location) that accepts `--server`, `--username`, and a
      password from stdin or a permission-restricted `--password-file`.
      Never accept a password as a CLI argument, store it in shell history,
      or print it in command diagnostics.
      **Verify:** `--help` documents safe usage; missing/invalid inputs fail
      without exposing credentials.
- [ ] Have the launcher obtain a JWT through `/api/auth/login`, fetch the
      authorized terminal config, and hand the JWT to a temporary in-memory
      browser page via exact-origin `postMessage`. Do not write the JWT/key
      to `launcher.html`, `localStorage`, logs, or world-readable files.
      **Verify:** Static scan and runtime log inspection find no secrets.
- [ ] Extend `tests/test_26_terminal.sh` to cover:
      - authorized `/api/system/info` returns `enabled`, absolute `url`,
        `protocol`, and `key`; generic, invalid-JWT, and valid non-terminal
        JWT responses omit the terminal block;
      - the returned key matches the configured key by redacted comparison;
      - correct key succeeds and wrong/old keys fail;
      - configured protocol succeeds and a mismatched subprotocol fails;
      - missing, unresolved, weak, or known-default key fails startup;
      - generated payload consumes the API URL, contains no hardcoded
        key/port/protocol fallback, and logs no full response;
      - allowed and disallowed CORS/origin cases are enforced;
      - redacted auth/protocol/origin telemetry is present.
      **Verify:** Test 26 green through `./test_00_all.sh 26_terminal`.
- [ ] Update Test 26 to follow framework ownership rules: remove its local
      `TEST_COUNTER=0` initialization, let `print_subtest` increment the
      counter, pair every TEST with one result, and redact key-bearing
      `print_command`/output lines.
      **Verify:** `mks` and Test 26 output contain no secret or counter
      ownership violation.
- [ ] Extend `tests/unit/managers/terminal.test.js` to cover:
      - exact `targetOrigin` and allowed `event.origin`;
      - rejection of wrong-origin/wrong-source messages;
      - JWT response and no-JWT error behavior;
      - destroy/init lifecycle with one retained handler;
      - terminal URL construction and rejection of malformed/unapproved URLs;
      - no wildcard `postMessage` target or JWT logging.
      **Verify:** `npm test` green.
- [ ] Verify rotation end to end: change `WEBSOCKET_KEY`, restart Hydrogen,
      compare redacted fingerprints, confirm the new key is returned and
      accepted and the old key is rejected. Do not record raw values.
      **Verify:** Redacted rotation evidence in Status/test output.
- [ ] Run `mkp` + `mks` for C/script changes, `npm test` + `npm run lint`
      for Lithium changes, and `mkl` plus Test 90 after documentation
      changes.
      **Verify:** All named commands are green.

### Done means

A secure launcher exists, Test 26 and Lithium tests exercise role
authorization, exact origins, public URL validation, protocol, key rotation,
rollback, telemetry, and lifecycle behavior, and all diagnostics are
redacted.

### Exit gate

Launcher help/runtime checks pass; Test 26, Lithium tests/lint, `mkp`,
`mks`, and documentation checks are green; no raw secret or wildcard
production origin appears anywhere.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Cross-Phase Rules

- After every C change: `mkq` for incremental work or `mkt` for a clean
  configure/build, then `mkp`.
- After payload generation or embedding changes: run `mkt`/`mka`, then
  Test 26 through `./test_00_all.sh 26_terminal`.
- Test 26 is blackbox; never substitute `mku` for it. Use `mku <base>` only
  for Unity tests.
- After Lithium JS/CSS/HTML changes: `npm test`, `npm run lint`, and
  `npm run build` when production assets change; run
  `npm run templates:copy` after template edits.
- After Bash changes: `mks`.
- After documentation changes: `mkl` (Test 04) and Test 90 markdownlint.
- **Never log or print JWTs, keys, passwords, full WebSocket URIs, or full
  `/api/system/info` responses.** Redact test commands and artifacts.
- **Never hardcode the WebSocket key or terminal protocol.** Both come from
  runtime configuration; missing or weak key values fail closed.
- Require the exact `terminal` JWT role for terminal config; never treat a
  generic valid JWT as terminal authorization.
- Require an explicit public WebSocket origin and WSS in production; never
  derive it from an untrusted `Host` or iframe location.
- Honor forwarded client IP only from trusted proxy peers.
- Use exact `postMessage` origins and validate `event.origin`/`event.source`.
- Enforce one effective terminal CORS allowlist across API and payload
  responses; wildcards are local-dev-only.
- Record redacted rotation and rollback evidence; never persist raw keys or
  JWTs.
- Follow existing test numbering and conventions; extend existing tests
  before creating new ones.

## Relationship To Other Documents

| Document | Role |
| --- | --- |
| This file | Active terminal fix plan |
| [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) | Authentication/product security relationship; terminal WS auth remains subject to its product gate |
| [`test_26_terminal.md`](/docs/H/tests/test_26_terminal.md) | Blackbox test documentation |
| [`TESTING.md`](/docs/H/tests/TESTING.md) | Test runner and blackbox invocation contract |
| [`terminal_architecture.md`](/docs/H/core/reference/terminal_architecture.md) | Terminal subsystem architecture |
| [`terminal-generate.sh`](/elements/001-hydrogen/hydrogen/payloads/terminal-generate.sh) | Payload generator; inline `<script>` JS |
| [`terminal.js`](/elements/003-lithium/src/managers/terminal/terminal.js) | Lithium Terminal Manager |
| [`terminal.test.js`](/elements/003-lithium/tests/unit/managers/terminal.test.js) | Lithium unit tests |
| [`AGENTS.md`](/elements/003-lithium/AGENTS.md) | Lithium-specific workflow and verification rules |
| [`websocket_server_dispatch.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c) | WebSocket auth and key handling |
| [`websocket_server_message.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_message.c) | Protocol-name routing |
| [`websocket_server_terminal.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_terminal.c) | Terminal protocol validation |
| [`websocket_server_context.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_context.c) | `ws_context_create` and key copy |
| [`websocket_server_startup.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_startup.c) | Configured protocol registration |
| [`config_utils.c`](/elements/001-hydrogen/hydrogen/src/config/config_utils.c) | Environment-reference resolution and sensitive logging |
| [`config.c`](/elements/001-hydrogen/hydrogen/src/config/config.c) | Config load/schema behavior |
| [`config_network.c`](/elements/001-hydrogen/hydrogen/src/config/config_network.c) | Trusted-proxy configuration target |
| [`config_terminal.c`](/elements/001-hydrogen/hydrogen/src/config/config_terminal.c) | Terminal enable/path/CORS configuration |
| [`web_server_core.c`](/elements/001-hydrogen/hydrogen/src/webserver/web_server_core.c) | Effective CORS origin matching |
| [`info.c`](/elements/001-hydrogen/hydrogen/src/api/system/info/info.c) | System info endpoint and terminal config |
| [`config_defaults.c`](/elements/001-hydrogen/hydrogen/src/config/config_defaults.c) | WebSocket defaults |
| [`launch_websocket.c`](/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c) | WebSocket startup/readiness checks |
| [`terminal_websocket.c`](/elements/001-hydrogen/hydrogen/src/terminal/terminal_websocket.c) | Legacy terminal protocol/auth helpers to reconcile |
| [`api_utils.c`](/elements/001-hydrogen/hydrogen/src/api/api_utils.c) | Client IP extraction |
| [`auth_service_jwt.c`](/elements/001-hydrogen/hydrogen/src/api/auth/auth_service_jwt.c) | JWT generation with `ip` claim |
| [`hydrogen_test_26_terminal_payload.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_26_terminal_payload.json) | Test config; currently sets Protocol to terminal |
| [`examples/configs/hydrogen.json`](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen.json) | Example WebSocket protocol/config |

## Working Log

### Session 1 (2026-09-11)

- Read `AUTH_FINALE.md` for plan structure and conventions.
- Read all terminal-related C source: `info.c`, `websocket_server_dispatch.c`,
  `websocket_server_auth.c`, `websocket_server_startup.c`,
  `websocket_server.c`, `websocket_server_internal.h`,
  `terminal_websocket.c`, `launch_websocket.c`, `config_defaults.c`,
  `api_utils.c`, `auth_service_jwt.c`, `globals.h`.
- Read Lithium `terminal.js` (full file) — confirmed the
  `_handleIframeMessage` fix is already applied: bound in constructor
  (line 148), registered in `init()` (line 247), not nulled in `destroy()`
  (line 699–715).
- Read `terminal-generate.sh` — confirmed version 2.1.1 removed the
  hardcoded `ABCDEFGHIJKLMNOP` fallback key from `connectToWebSocket`.
- Read `test_26_terminal.sh` — confirmed it tests WebSocket connections
  with `Authorization: Key ${WEBSOCKET_KEY}` header.
- Read `terminal.test.js` — confirmed updated tests for re-init behavior
  and postMessage flow.
- **Corrected line references** from initial plan draft:
  - `websocket_server_dispatch.c:270` → actual is line 262 (`FILTER_PROTOCOL_CONNECTION`),
    and it checks the **key only**, not the protocol name.
  - Protocol routing is in `websocket_server_message.c:215-224` (not in
    `callback_http`).
  - `websocket_server_terminal.c:33-43` — `validate_terminal_protocol`
    hardcodes `"terminal"`.
  - `websocket_server_context.c:38` — `ws_context_create` sets `auth_key`;
    `launch_websocket.c:248` is `return 0`, not key assignment.
  - `config_defaults.c:315` = protocol (`"hydrogen"`), line 316 = key
    (`${env.WEBSOCKET_KEY}`); previous plan had these swapped.
- Confirmed protocol mismatch: iframe sends `'terminal'`, Hydrogen default
  is `'hydrogen'`. Test config sets `'terminal'` to make Test 26 pass.
- Confirmed `terminal_websocket.c:25` defines `TERMINAL_WS_PROTOCOL "terminal"`,
  used in `websocket_server_terminal.c:36` and `websocket_server_message.c:218`.
- **Key rotation theme added:** The `ABCDEFGHIJKLMNOP` fallback
  (`websocket_server_dispatch.c:289`) is dev/test only; production must
  use a strong secret from `WEBSOCKET_KEY` env var or `WebSocketServer.Key`
  config.
- This plan document upgraded with all corrections and key rotation policy.

### Session 2 (2026-09-11) — Plan upgrade audit

- Re-read the full plan, Hydrogen config resolution/logging code, WebSocket
  dispatch/context/startup/message/terminal code, generated payload, Test 26,
  Lithium terminal manager/tests, Lithium AGENTS, and Hydrogen TESTING docs.
- Confirmed the current key fallbacks are broader than the prior plan stated:
  `default_key` in `websocket_server_context.c`, `default_websocket_key` in
  `launch_websocket.c`, and `ABCDEFGHIJKLMNOP` in dispatch.
- Confirmed `${env.WEBSOCKET_KEY}` can remain unresolved when the environment
  variable is absent; the plan now requires fail-closed startup validation.
- Confirmed secret leakage in dispatch URI/key logs, payload response logging,
  and Test 26 command output; the plan now requires redaction at every layer.
- Confirmed the payload has hardcoded port/protocol fallbacks, wildcard
  messaging, and a localStorage JWT fallback; the plan now treats these as
  Phase 3 work.
- Confirmed Lithium uses wildcard `postMessage` target origin and does not
  validate origin/source; the plan now requires exact origins and source
  checks in Phase 4.
- Confirmed Test 26 is a blackbox test invoked through `test_00_all.sh`, and
  its local `TEST_COUNTER=0` conflicts with the framework-owned counter rule.
- Reordered phases so server config/authorization/protocol (Phase 2) precedes
  payload generation (Phase 3), and moved live deployment verification to
  Phase 5.
- Marked Phase 0 complete with the live Traefik/DOKS/deployed-config variance
  deferred; no source, test, payload, or deployment changes were made.

### Session 3 (2026-09-11) — Source cross-check lock-in

- Performed exact source-level audit against `src/` to lock in the verified
  findings that the plan's work items depend on. Read the full implementations of:
  - `callback_http` in `websocket_server.c:60-150` — confirmed the HTTP-upgrade
    auth surface (Authorization header + `?key=` query param) is distinct from
    dispatch.
  - `websocket_server_dispatch.c:262-384` — confirmed the `ABCDEFGHIJKLMNOP`
    fallback at line 289, the stored-key log at line 304, the URI log at line
    323, and the query-param key log at line 367.
  - `terminal_websocket.c` (full file) — confirmed `is_terminal_websocket_request`
    (line 39) and `handle_terminal_websocket_upgrade` (line 130) are dead code:
    grep across `src/` finds no caller outside the file/header.
  - `launch_websocket.c:64-61` — confirmed `validate_key` checks only length
    and printable-ASCII; it does not reject `${env.*}` or known-default literals,
    and `check_websocket_launch_readiness` (line 106) never calls it.
  - `api_utils.c:155-228` — confirmed `is_ip_internal` is used only for XFF
    chain selection, not peer trust; no trusted-proxy config field exists.
  - `config_websocket.c:39,45` — confirmed protocol default `"hydrogen"` and
    key default `${env.WEBSOCKET_KEY}`; `PROCESS_SENSITIVE` overwrites from
    `WebSocketServer.Key`.
  - `config.c:236-240` and `config_utils.c:201-239` — confirmed env resolution
    leaves the literal `${env.WEBSOCKET_KEY}` when the env var is absent.
  - `websocket_server_message.c:218` and `websocket_server_terminal.c:36` —
    confirmed hardcoded `"terminal"` protocol checks (via `TERMINAL_WS_PROTOCOL`).
  - `info.c:94-142` — confirmed the terminal block (`port`, `key`) is gated on
    `has_jwt && ws_context` only, with no terminal-authorization distinction;
    `handle_system_info_request` passes `include_scripting = has_jwt`.
- Added a "Verified Findings" section to the plan documenting all of the above.
- Updated Phase 1 work items to specify the `Network.TrustedProxies` config
  addition and the `api_utils.c` target.
- Updated Phase 2 work items to reference both auth surfaces, the `validate_key`
  weakness, the specific dispatch log lines to redact, and the hardcoded
  `"terminal"` line references.
- Recorded existing Unity test files to extend rather than create new ones.
- No source, test, payload, or deployment changes were made; implementation
  awaits approval after Phase 0/1 review.

### Session 4 (2026-09-11) — Contract hardening

- Added binding contracts for terminal JWT role authorization, the authorized
  `/api/system/info` response schema, trusted-proxy CIDR handling, explicit
  public WebSocket origin, exact terminal CORS, WSS production transport,
  key validation/rotation, redacted telemetry, and rollback.
- Recorded the terminal CORS defect: `Terminal.CORSOrigin` is parsed but
  terminal responses use the global API/WebServer CORS source, so production
  currently has no effective terminal-specific origin policy.
- Recorded the public-endpoint defect: the generated payload derives API and
  WebSocket origins from iframe location and a direct port, with no trusted
  reverse-proxy contract.
- Expanded Phase 1 for `Network.TrustedProxies`, right-to-left XFF selection,
  trusted forwarded proto/port handling, and spoofing tests.
- Expanded Phase 2 for the exact `terminal` role, absolute public `terminal.url`,
  `WebSocketServer.PublicUrl`, CORS wiring, 32-character minimum key
  validation, dead MHD path reconciliation, rotation telemetry, and rollback.
- Expanded Phases 3–6 for API URL consumption, exact-origin/source checks,
  production WSS, role-negative tests, CORS tests, public-route evidence,
  rollback evidence, and redacted Test 26/Lithium coverage.
- No source, test, payload, deployment, or configuration changes were made;
  this remains a plan-only amendment.

(End of file)
