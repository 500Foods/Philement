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

---

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| ------- | ---------------------- | -------- | -------- |
| 0 | Contract lock; audited architecture, root causes, security boundaries, and phase dependencies | S | complete |
| 1 | Trusted-proxy JWT `ip` claim reflects the real client IP behind Traefik/DOKS | S | pending |
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

- [ ] Inspect the deployed Traefik configuration and record the trusted proxy
      boundary: `forwardedHeaders.trustedIPs`, `proxyProtocol`, or the
      current DOKS equivalent must cover only trusted load-balancer peers.
      **Verify:** Deployment config or an explicit ops variance is recorded.
- [ ] Verify DOKS preserves the original client source address or supplies a
      trustworthy forwarded chain. Use the current DOKS mechanism rather than
      assuming a specific annotation is still supported.
      **Verify:** Service/load-balancer config or a live request trace.
- [ ] Update the client-IP contract so `api_get_client_ip` honors forwarded
      values only when the immediate TCP peer is trusted; otherwise use the
      peer address and ignore client-supplied forwarding headers.
      **Verify:** Code review and unit tests for trusted and untrusted peers.
- [ ] Extend the existing `api_get_client_ip` Unity coverage with:
      - a multi-hop `X-Forwarded-For` chain from a trusted peer;
      - an untrusted peer supplying a public `X-Forwarded-For` value;
      - the existing internal/public selection cases.
      **Verify:** `mku api_utils_test_get_client_ip` passes.
- [ ] Generate or inspect a live JWT after login and confirm its `ip` claim
      matches the trusted public client address, not the load balancer or an
      attacker-supplied header.
      **Verify:** Redacted claim inspection recorded in Status.
- [ ] Run `mkq` (or `mkt` when a clean/configure build is required) and
      `mkp`.
      **Verify:** Build and cppcheck are clean.

### Done means

The trusted-proxy boundary is documented, spoofed forwarded headers are
ignored outside that boundary, and a live JWT contains the real client IP.

### Exit gate

Traefik/DOKS trust configuration verified or an explicit ops variance
recorded; trusted/untrusted client-IP unit tests pass; `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

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
      context. Missing, empty, unresolved `${env.WEBSOCKET_KEY}`, or weak
      values must prevent startup; remove `default_key`,
      `default_websocket_key`, and `ABCDEFGHIJKLMNOP` fallbacks from
      `websocket_server_context.c`, `launch_websocket.c`, and dispatch.
      **Verify:** Missing/unresolved-key startup tests fail safely.
- [ ] Ensure sensitive config logging redacts the value and never prints the
      resolved key. Search server logs for the key, env reference, and query
      string after startup and authentication attempts.
      **Verify:** Log inspection contains no secret material.
- [ ] Define the authorized system-info contract. Add an explicit terminal
      authorization check for the REST endpoint and keep the generic Lua
      `H.system.info()` response from exposing the key unless a separate
      authorized API is deliberately introduced.
      **Verify:** Unauthenticated, invalid-JWT, and non-terminal-authorized
      requests cannot obtain `terminal.key`.
- [ ] Extend the terminal object returned by authorized `/api/system/info`
      with `port` and `protocol` from the active WebSocket config. Keep the
      key server-wide and return it only after authorization succeeds.
      **Verify:** Authorized response contains the configured port/protocol;
      unauthorized response omits the terminal object or key.
- [ ] Replace hardcoded `"terminal"` routing in
      `websocket_server_message.c` and `websocket_server_terminal.c` with
      the active configured protocol. Reconcile
      `get_terminal_websocket_protocol()` and
      `terminal_websocket_requires_auth()` so legacy helpers cannot bypass or
      contradict the LWS authentication path.
      **Verify:** Configured protocol accepts terminal traffic; a mismatched
      subprotocol is rejected.
- [ ] Lock the browser authentication transport: browsers send the key in
      the WebSocket query string because the browser API cannot set custom
      headers; require TLS in production and redact the full URI from logs.
      Non-browser tests may use `Authorization: Key` where supported.
      **Verify:** Correct key succeeds, wrong/old key fails, and no key is
      logged.
- [ ] Add focused C/Unity coverage for key validation, unresolved env
      handling, configured-protocol routing, and authorized system-info
      behavior. Do not add a static helper in Hydrogen `src/`.
      **Verify:** Relevant Unity tests pass and `mkt` dead-code gate is clean.
- [ ] Run `mkq` (or `mkt` when config/payload inputs changed) and `mkp`.
      **Verify:** Build and cppcheck are clean.

### Done means

Hydrogen starts only with a resolved strong key, authorized callers receive
the active port/protocol/key contract, unauthorized callers do not, and
terminal routing follows the configured protocol with no hardcoded key or
protocol literal.

### Exit gate

Key-fail-closed, authorization, protocol, and redaction tests pass;
`mkq`/`mkp` green; no secret appears in logs or test output.

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
- [ ] Require `terminal.port` and `terminal.protocol` from the authorized
      system-info response. Remove the hardcoded `5261` port fallback and
      hardcoded `"terminal"` WebSocket subprotocol; fail visibly when either
      value is absent or invalid.
      **Verify:** Generated HTML contains no port/protocol literals used as
      connection defaults.
- [ ] Derive the API/WebSocket origin from the configured server and page
      origin. Do not assume the iframe hostname, direct port, or
      `wss://<hostname>:<port>` shape in production; use the public endpoint
      contract established in Phase 2/5.
      **Verify:** Same-origin and approved reverse-proxy configurations work.
- [ ] Replace wildcard `postMessage` behavior with an exact target origin and
      validate `event.origin` against an allowlist plus `event.source` against
      the terminal iframe. Never accept config messages from an arbitrary
      ancestor.
      **Verify:** Unit/static checks and browser test reject a wrong origin.
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

The generated terminal payload uses the authorized API contract, configured
port/protocol, exact-origin messaging, and redacted diagnostics; it fails
visibly instead of falling back to hardcoded secrets or unsafe origins.

### Exit gate

Payload rebuilt and embedded; generated HTML/static checks pass; Test 26,
`mks`, and `mkp` are green with no secret leakage.

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
      derived from the configured Hydrogen origin. Validate
      `event.origin` against the same allowlist and verify `event.source`
      is the terminal iframe before returning a JWT.
      **Verify:** Unit tests prove wrong-origin and wrong-source messages
      are ignored and no JWT is sent to `'*'`.
- [ ] Validate `terminalUrl` and the iframe origin against configured
      `server.url`/`server.terminal_path`; do not allow an arbitrary
      cross-origin iframe to request terminal credentials.
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
      `WebSocketServer.Port`, `WebSocketServer.Protocol`, and key source.
      Do not assume the local defaults or the previously observed 7001.
      **Verify:** Redacted config evidence is recorded.
- [ ] Inspect DOKS and Traefik routing for the actual public endpoint:
      service target port, TLS termination, HTTP/1.1 Upgrade handling,
      `passHostHeader`/equivalent, trusted forwarded headers, and firewall
      exposure. Prefer a path-based HTTPS route; if a direct port is used,
      document and restrict it.
      **Verify:** Route trace or deployment manifest is recorded.
- [ ] Verify CORS/origin policy for `/api/system/info` and the terminal
      iframe. No wildcard production origin; only the approved Lithium
      origin may request terminal credentials.
      **Verify:** Browser/network test from allowed and disallowed origins.
- [ ] Rotate the deployed key using a strong random value supplied through
      `WEBSOCKET_KEY` or `WebSocketServer.Key`; restart Hydrogen and verify
      the new key is returned only to an authorized caller. Record only a
      redacted fingerprint, never the raw key.
      **Verify:** New key accepted, old key rejected, no secret in logs.
- [ ] Open the production Lithium URL, log in, and open Terminal. Confirm:
      - the iframe requests config from the approved parent origin;
      - Lithium replies with an exact-origin/source-checked message;
      - `/api/system/info` returns the active port/protocol;
      - the WebSocket uses the configured protocol and accepted key;
      - `onopen` fires and a shell prompt appears.
      **Verify:** Redacted browser/network evidence and shell prompt.
- [ ] Exercise disconnect/reconnect and popup destroy/init cycles. Confirm
      there is one active terminal connection and no stale listener, timer,
      or duplicate iframe session.
      **Verify:** Browser console/network inspection has no lifecycle errors.
- [ ] If connection fails, diagnose only redacted categories: trusted-proxy
      IP, authorization, key mismatch, protocol mismatch, route/port, TLS,
      or origin policy. Never paste the key or JWT into logs/tickets.
      **Verify:** Troubleshooting result recorded without secrets.

### Done means

The deployed route and origin policy are verified, the key rotates without
code changes, and the Lithium iframe establishes an authenticated terminal
session with a shell prompt.

### Exit gate

Manual production E2E succeeds; route/TLS/origin evidence is recorded; new
key works, old key fails, and all evidence is redacted.

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
      - authenticated `/api/system/info` returns port/protocol and omits
        the terminal block when unauthorized;
      - the returned key matches the configured key by redacted comparison;
      - correct key succeeds and wrong/old keys fail;
      - configured protocol succeeds and a mismatched subprotocol fails;
      - missing/unresolved key fails startup;
      - generated payload contains no hardcoded key/port/protocol fallback
        and no full-response logging.
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
      - terminal URL construction from configured server/path.
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

A secure launcher exists, Test 26 and Lithium tests exercise authorization,
origin, protocol, key rotation, and lifecycle behavior, and all diagnostics
are redacted.

### Exit gate

Launcher help/runtime checks pass; Test 26, Lithium tests/lint, `mkp`,
`mks`, and documentation checks are green; no raw secret appears anywhere.

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
  runtime configuration; missing key values fail closed.
- Honor forwarded client IP only from trusted proxy peers.
- Use exact `postMessage` origins and validate `event.origin`/`event.source`.
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

(End of file)
