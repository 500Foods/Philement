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

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.**
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, **stop for review**. Do not begin the next phase in the same turn
  unless asked.
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure),
  `mku <base>`, `mkp`, `mka`, `mks`. Lithium: `npm test`, `npm run lint`.
- **Never apply a database migration.** Hand packets to the user.
- **Never log client secrets, JWTs, or OTP plaintext** in logs or test artifacts.

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
8. **Follow existing project norms** for tests (Unity, blackbox `Test 26`),
   lint (`mkp`, `mks`), and build aliases.

---

## Architecture Overview (current state)

### Components

| Layer | Component | Location |
| ------- | ----------- | ---------- |
| Auth | Password / OIDC RP JWT | `src/api/auth/` — `login.c`, `auth_service_jwt.c` |
| API | System Info (terminal config) | `src/api/system/info/info.c` |
| WebSocket | Server + auth + dispatch | `src/websocket/` — `websocket_server_*.c` |
| Terminal | Session + PTY bridge | `src/terminal/terminal_websocket.c` + `terminal_session.*` |
| Payload | xterm.js + terminal.html | `payloads/` + `payloads/terminal-generate.sh` |
| Client SPA | Terminal manager + iframe | `elements/003-lithium/src/managers/terminal/` |
| Test | Blackbox terminal test | `tests/test_26_terminal.sh` |
| Test | Lithium unit tests | `tests/unit/managers/terminal.test.js` |
| Proxy | DOKS LoadBalancer + Traefik | External (ops) |

### Key flows

**JWT issuance (`src/api/auth/`):**

- `generate_jwt_with_oidc` in `auth_service_jwt.c:96` embeds `client_ip` as
  the `"ip"` claim.
- `client_ip` comes from `api_get_client_ip` (`api_utils.c:188`) which checks
  `X-Forwarded-For` header, falling back to the TCP peer address.
- Behind Traefik/DOKS, `X-Forwarded-For` must be passed through for the
  real client IP to appear in the JWT.

**System Info (`src/api/system/info/info.c`):**

- `system_info_has_valid_jwt` validates the `Authorization: Bearer <jwt>` header.
- `system_info_build_json` includes a `"terminal"` object with `port` and
  `key` only when `has_jwt && ws_context`.
- The terminal key is `ws_context->auth_key` — the server-wide WebSocket auth
  key, **not** a per-session key.

**WebSocket auth (`src/websocket/websocket_server_dispatch.c`):**

- `ws_context->auth_key` is set from `config->websocket.key` at startup
  (`launch_websocket.c:248`, default `"default_websocket_key"`).
- Config default: `config->websocket.key = strdup("${env.WEBSOCKET_KEY}")`
  (`config_defaults.c:316`).
- Three auth paths during `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION`:
  1. **Fallback key** `ABCDEFGHIJKLMNOP` — hardcoded in `websocket_server_dispatch.c:289`,
     accepted only if `ws_context->auth_key` equals that fallback. This is a
     dev/test convenience, **not** the production flow.
  2. **Stored key** from HTTP upgrade — set during `LWS_CALLBACK_HTTP` when the
     `Authorization: Key <value>` header or `?key=<value>` query param is
     presented and matches `ws_context->auth_key`.
  3. **Query param key** (re-parsed during protocol filtering).

**Terminal iframe payload (`payloads/terminal-generate.sh`):**

- Generates `terminal.html` with `fetchTerminalConfig()` which:
  1. Tries `parent.postMessage({ type: 'terminal-config-request' })` to Lithium.
  2. Falls back to `localStorage.getItem('lithium_jwt')` after 250ms.
  3. Uses the JWT to call `/api/system/info` with `Authorization: Bearer <jwt>`.
  4. Extracts `data.terminal.port` and `data.terminal.key` from the response.
- `connectToWebSocket(config)` builds `wss://<hostname>:<port>?key=<key>`.
- Version 2.1.1 removed the hardcoded `ABCDEFGHIJKLMNOP` fallback from
  `connectToWebSocket` — the iframe now fails loudly if no key is received.

**Lithium Terminal Manager (`src/managers/terminal/terminal.js`):**

- `terminalUrl` getter builds the iframe `src` from `server.url` +
  `server.terminal_path` config (default `/terminal`).
- `init()` creates the popup, iframe, and registers
  `window.addEventListener('message', this._handleIframeMessage)`.
- `_handleIframeMessage` responds to `terminal-config-request` by calling
  `retrieveJWT()` and `postMessage({ type: 'terminal-config', config: { jwt } })`.
- `destroy()` removes listeners and nulls DOM references, but must **not**
  null `this._handleIframeMessage` (the bound handler) so it can be re-registered
  on `init()`.

### Current defects observed

1. **`_handleIframeMessage is null` crash** in deployed Lithium:
   `destroy()` nulled `this._handleIframeMessage`, then `init()` tried to
   re-bind it via `this._handleIframeMessage.bind(this)` which fails on null.
   **Fix already applied in `terminal.js:139-148,247,697-715`** — the bound
   handler is now created once in the constructor and `destroy()` no longer
   nulls it. Unit test updated and passing (947 tests).

2. **WebSocket connects to port 7001 with key `ABCDEFGHIJKLMNOP`:**
   The console log shows the iframe receiving config from `/api/system/info`
   successfully (JWT valid, terminal object present with port 7001, key
   `ABCDEFGHIJKLMNOP`). But the WebSocket connection to `wss://lithium.philement.com:7001/?key=ABCDEFGHIJKLMNOP`
   fails. Port 7001 is likely not the configured WebSocket port and/or not
   exposed through the DOKS LoadBalancer + Traefik.

3. **Protocol mismatch:** The iframe opens WebSocket with protocol `'terminal'`
   but Hydrogen's config default protocol is `'hydrogen'`. The `callback_http`
   handler in `websocket_server.c` does not validate the protocol
   string match — it checks the key only. But `websocket_server_dispatch.c`
   `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION` does check `lws_get_protocol(wsi)`
   against the configured protocol name.

---

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| ------- | ---------------------- | -------- | -------- |
| 0 | Contract lock; architecture documented; root cause confirmed; JWT-IP, payload, WS-key, iframe-flow, debug tool, tests | S | pending |
| 1 | JWT `ip` claim reflects real client IP behind Traefik/DOKS | S | pending |
| 2 | `terminal-generate.sh` payload produces a `terminal.html` that fetches `/api/system/info` and passes JWT via postMessage from parent | S | pending |
| 3 | WebSocket server key exposed via `/api/system/info` when authenticated; port is configurable and matches Hydrogen config | S | pending |
| 4 | Lithium `_handleIframeMessage` bound once in constructor; destroy/init cycle safe; iframe receives JWT and fetches terminal config | S | pending |
| 5 | WebSocket connection from iframe works end-to-end (port exposed, key matches, protocol accepted) | M | pending |
| 6 | Test 26 terminal blackbox + Lithium unit tests cover the full flow; new debug/launcher tool in extras/ | S | pending |

Effort key: S = small/contained, M = moderate (networking/deployment + testing).

---

## Phase 0 — Contract Lock

### Goal

Document the current architecture, lock design decisions, and confirm the
root cause of each defect before touching code. No production changes.

### Entry gate

This document exists. Ability to read code in `src/api/`, `src/websocket/`,
`src/terminal/`, `payloads/terminal-generate.sh`,
`elements/003-lithium/src/managers/terminal/`, and `tests/test_26_terminal.sh`.

### Work items

- [ ] Confirm the JWT `ip` claim source path: `api_get_client_ip` →
      `X-Forwarded-For` → TCP peer. Document Traefik/DOKS requirement.
      **Verify:** Status records the chain.
- [ ] Confirm the WebSocket key flow: config → `ws_context->auth_key` →
      `/api/system/info` → iframe → `?key=`. Document that it is server-wide,
      not per-session.
      **Verify:** Status records source files and flow.
- [ ] Confirm the port: config default is 5001
      (`config_defaults.c:304`); the console log shows 7001 on the deployed
      instance — this is a **config override**, not the code default.
      **Verify:** Status records that 7001 comes from `hydrogen.json`
      `WebSocketServer.Port` and needs checking.
- [ ] Confirm the protocol mismatch: iframe sends `'terminal'`, Hydrogen
      default protocol is `'hydrogen'`. Document that `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION`
      in `websocket_server_dispatch.c:270` checks the protocol name.
      **Verify:** Status records the mismatch and that both the config
      `WebSocketServer.Protocol` and the iframe must agree.
- [ ] Confirm the `_handleIframeMessage is null` root cause: `destroy()`
      nulled the bound handler, `init()` tried to re-bind it.
      **Verify:** Status records files `terminal.js:139-148,247,697-715`.
      (Fix already applied — document it here.)
- [ ] Lock decision: JWT `ip` claim should reflect the **real** client IP
      behind the proxy. Traefik must be configured with
      `forwardedHeaders.trustedIPs` or `proxyProtocol` and the
      `X-Forwarded-For` header passed through.
      **Verify:** Decision in Status.
- [ ] Lock decision: WebSocket port must be **configurable** and exposed
      through DOKS + Traefik. Port 5001 default; the deployed 7001 override
      must be validated.
      **Verify:** Decision in Status.
- [ ] Lock decision: The debug/launcher tool (Phase 6) will accept
      `--server`, `--username`, `--password` as parameters, fetch a JWT
      via `/api/auth/login`, then open a browser window pointing at the
      terminal iframe URL. No hardcoded server.
      **Verify:** Decision in Status.
- [ ] Document the full end-to-end flow diagram from JWT issuance through
      iframe WebSocket connection.
      **Verify:** Status.

### Done means

Architecture, root causes, and design decisions are documented in Status.
No C/JS changes.

### Exit gate

Status filled. Working Log has locked decisions for JWT-IP, payload, WS-key,
iframe-flow, debug tool, tests.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 1 — JWT IP Claim Behind Traefik/DOKS

### Goal

Ensure the JWT `ip` claim reflects the actual client IP when Hydrogen is
behind Traefik + DOKS LoadBalancer.

### Entry gate

Phase 0 complete.

### Work items

- [ ] Verify Traefik `forwardedHeaders.trustedIPs` includes the DOKS
      LoadBalancer subnet, or that `proxyProtocol` is enabled end-to-end.
      **Verify:** Check `traefik.yaml` / Helm values in the deployment.
- [ ] Verify Hydrogen's `api_get_client_ip` (`api_utils.c:188`) correctly
      parses the first non-internal IP from `X-Forwarded-For`.
      **Verify:** `test_system_endpoint` test already checks client_ip at
      `src/api/system/test/test.c:71-80` — confirm it works behind a proxy.
- [ ] If the Traefik config cannot be changed, ensure DOKS passes the
      original client IP. The DOKS LoadBalancer must forward the real
      source IP, not the load balancer's.
      **Verify:** Check DOKS service annotation
      `service.beta.kubernetes.io/do-loadbalancer-preserve-client-ip: "true"`
      or equivalent.
- [ ] Add or extend a unit test for `api_get_client_ip` with a multi-hop
      `X-Forwarded-For` header to confirm the public-IP selection logic.
      **Verify:** Unity test passes.
- [ ] `mkq` + `mkp`.
      **Verify:** Build and lint clean.

### Done means

The JWT `ip` claim contains the real client IP when deployed behind
Traefik + DOKS, as confirmed by inspecting a live JWT's `ip` field.

### Exit gate

Traefik/DOKS config verified or documented. `mkq`/`mkp` green. Unit test
for `api_get_client_ip` updated/added.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 2 — Terminal Payload Regeneration

### Goal

Ensure `terminal-generate.sh` produces a `terminal.html` that correctly
fetches `/api/system/info` using a JWT obtained from the parent frame (Lithium)
via `postMessage`, and passes the terminal port + key to the WebSocket
connection.

### Entry gate

Phase 1 complete.

### Work items

- [ ] Review the current `terminal.html` in the payload
      (`payloads/terminal-generate.sh` lines 147–425). Confirm the
      `fetchTerminalConfig()` logic:
      - Sends `terminal-config-request` via `postMessage` to parent.
      - Falls back to `localStorage.lithium_jwt` after 250ms.
      - Calls `/api/system/info` with `Authorization: Bearer <jwt>`.
      - Extracts `data.terminal.port` and `data.terminal.key`.
      **Verify:** Code review; no hardcoded key fallback.
- [ ] Confirm the WebSocket URL construction uses `wss://` when the page
      is loaded over `https://`, and `ws://` for `http://`.
      **Verify:** `connectToWebSocket` at line 350.
- [ ] Confirm the WebSocket protocol sent by the iframe matches Hydrogen's
      configured protocol. Currently the iframe sends `'terminal'` but
      Hydrogen's default is `'hydrogen'`.
      **Decision from Phase 0:** Either change the iframe to send the
      configured protocol, or change Hydrogen's default to `'terminal'`.
      **Verify:** Documented in Status.
- [ ] Rebuild the Hydrogen payload after any `terminal-generate.sh` changes.
      **Verify:** `mkt` (or `mka`).
- [ ] `mkp` + `mks`.
      **Verify:** Lint clean.

### Done means

`terminal.html` in the payload correctly obtains JWT from parent,
fetches `/api/system/info`, and constructs the WebSocket URL with the
correct port and key. Protocol mismatch resolved.

### Exit gate

Payload rebuilt. `mkq`/`mkp`/`mks` green. Protocol mismatch fixed.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 3 — WebSocket Server Key + Port Exposure

### Goal

Ensure the WebSocket server's auth key and port are correctly configured,
exposed via `/api/system/info`, and reachable from the client (Lithium iframe).

### Entry gate

Phase 2 complete.

### Work items

- [ ] Confirm the WebSocket port configuration path: `config_defaults.c:304`
      sets default `5001`; the deployed instance shows `7001` in the
      `/api/system/info` terminal block. Verify `hydrogen.json` on the
      deployed server has `WebSocketServer.Port: 7001`.
      **Verify:** Check deployed config or deployment manifest.
- [ ] Confirm the WebSocket key: `config_defaults.c:316` sets default
      `${env.WEBSOCKET_KEY}`. The `/api/system/info` response shows key
      `ABCDEFGHIJKLMNOP` — check whether this is the fallback key in
      `websocket_server_dispatch.c:289` or a real configured key.
      **Verify:** If the key in `/api/system/info` matches
      `ws_context->auth_key`, the fallback path is not being used. If it
      is `ABCDEFGHIJKLMNOP`, the config may have that as the actual key.
- [ ] Verify DOKS LoadBalancer exposes port 7001 for WebSocket traffic.
      The DOKS LB must forward port 7001 to the Hydrogen pod(s), and
      Traefik must route WebSocket traffic on that port.
      **Verify:** Check DOKS service definition and Traefik ingress.
- [ ] Verify Traefik `wsRoute` or `PassHostHeader` is configured for
      WebSocket on port 7001. Traefik requires explicit WebSocket support
      configuration.
      **Verify:** Check Traefik `traefik.yaml` or Kubernetes IngressRoute
      annotations.
- [ ] Confirm `/api/system/info` returns the terminal block with the
      correct port and key when authenticated. The console log shows this
      is already working (status 200, terminal object present).
      **Verify:** `system_info_build_json` in `info.c:119-128`.
- [ ] `mkq` + `mkp`.
      **Verify:** Build and lint clean.

### Done means

`/api/system/info` returns the correct WebSocket port and key, the key
matches `ws_context->auth_key`, and port 7001 is exposed through DOKS +
Traefik for WebSocket traffic.

### Exit gate

WebSocket port/key verified in deployed config. DOKS + Traefik WebSocket
exposure verified. `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 4 — Lithium Terminal Manager (Fix + Verify)

### Goal

Ensure the Lithium Terminal Manager correctly handles the iframe lifecycle,
passes JWT via `postMessage`, and survives destroy/init cycles.

### Entry gate

Phase 3 complete.

### Work items

- [ ] Confirm the `_handleIframeMessage is null` fix in `terminal.js`:
      - `_handleIframeMessage` is bound in the constructor (`terminal.js:148`).
      - `init()` registers `window.addEventListener('message', this._handleIframeMessage)`
        (`terminal.js:247`) — no re-binding.
      - `destroy()` removes the listener but does **not** null
        `this._handleIframeMessage` (`terminal.js:699`).
      **Verify:** Read current `terminal.js` lines 139–148, 247, 697–715.
- [ ] Confirm `_handleIframeMessage` correctly responds to
      `terminal-config-request` by calling `retrieveJWT()` and posting
      `{ type: 'terminal-config', config: { jwt } }` back to the iframe
      (`terminal.js:675-692`).
      **Verify:** Code review.
- [ ] Verify the iframe `src` is set to `this.terminalUrl` which combines
      `server.url` + `server.terminal_path` (`terminal.js:163-174,229`).
      **Verify:** Config check — does `lithium.json` have the right
      `server.url` and `server.terminal_path`?
- [ ] Run Lithium unit tests: `npm test` from `elements/003-lithum/`.
      **Verify:** 947 tests pass (updated terminal test included).
- [ ] Run Lithium lint: `npm run lint`.
      **Verify:** Lint clean.

### Done means

Lithium Terminal Manager binds `_handleIframeMessage` once in the
constructor, survives destroy/init cycles, correctly passes JWT to the
iframe via `postMessage`, and all unit tests + lint pass.

### Exit gate

`npm test` green. `npm run lint` green. `terminal.js` fix confirmed in code.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 5 — End-to-End WebSocket From Iframe

### Goal

The terminal iframe opens a WebSocket connection to the Hydrogen WebSocket
server and the terminal session works, all from within the Lithium popup
(no "open in new window" workaround).

### Entry gate

Phase 4 complete.

### Work items

- [ ] Open `https://lithium.philement.com` in a browser.
- [ ] Log in and open the Terminal popup.
- [ ] Open browser dev tools → Console → Network → WS.
- [ ] Observe the iframe sends `terminal-config-request` via `postMessage`.
- [ ] Observe Lithium responds with
      `{ type: 'terminal-config', config: { jwt } }`.
- [ ] Observe the iframe calls `/api/system/info` with
      `Authorization: Bearer <jwt>` and receives
      `{ terminal: { port: 7001, key: "..." } }`.
- [ ] Observe the iframe opens `wss://lithium.philement.com:7001/?key=<key>`
      with protocol `'terminal'`.
- [ ] Observe the WebSocket `onopen` event fires (connection accepted).
- [ ] If connection fails, check server logs for:
      - Authentication failure (key mismatch).
      - Protocol mismatch (iframe sends `'terminal'`, server expects `'hydrogen'`).
      - Port not reachable (DOKS/Traefik not exposing 7001).
- [ ] If the key in `/api/system/info` is `ABCDEFGHIJKLMNOP` and
      `ws_context->auth_key` is also `ABCDEFGHIJKLMNOP`, the fallback path
      in `websocket_server_dispatch.c:289` is matching. This is insecure
      for production — the key should be set via `WEBSOCKET_KEY` env var.
      **Verify:** Check deployed config for `WebSocketServer.Key`.
- [ ] Fix any remaining issues (protocol mismatch, port exposure, key).
- [ ] Confirm a shell prompt appears in the xterm.js terminal.

### Done means

The terminal opens from the Lithium popup iframe, the WebSocket connection
is accepted, and a shell prompt appears. No need to "open in new window."

### Exit gate

Manual E2E test against `https://lithium.philement.com` succeeds.
All console log steps observable with no errors.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 6 — Debug Tool + Test Coverage

### Goal

Add a debug/launcher tool in `extras/` that can fetch a JWT and open a
terminal iframe directly (for troubleshooting), and write/extend tests to
cover the terminal flow.

### Entry gate

Phase 5 complete.

### Work items

- [ ] Create `extras/terminal-launcher.sh` (or `.py`) — a script that:
      - Accepts `--server <url>`, `--username <user>`, `--password <pass>`
        as parameters (no hardcoded server).
      - POSTs to `/api/auth/login` to obtain a JWT.
      - Writes a minimal `launcher.html` that loads the terminal iframe
        with the JWT injected via `postMessage`, or opens a browser window
        pointing at `<server>/terminal/` with the JWT in `localStorage`.
      **Verify:** Script runs without `--server` failing; documents usage.
- [ ] Extend `tests/test_26_terminal.sh` to also verify:
      - `/api/system/info` returns terminal config when authenticated.
      - The WebSocket key in the response matches the configured key.
      **Verify:** Test 26 green.
- [ ] Extend `tests/unit/managers/terminal.test.js` to cover:
      - `fetchTerminalConfig` logic (JWT retrieval, postMessage flow).
      - `connectToWebSocket` URL construction with port and key.
      **Verify:** `npm test` green.
- [ ] Run `mkp` + `mks` for any script changes.
      **Verify:** Lint clean.

### Done means

A debug tool exists in `extras/` for launching a terminal with credentials.
Test 26 and Lithium unit tests cover the terminal config + WebSocket flow.

### Exit gate

`extras/terminal-launcher.sh` exists and runs. Test 26 green.
`npm test` green. `mkp`/`mks` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Cross-Phase Rules

- After every C change: `mkq` (or `mkt`), then `mkp`.
- After WebSocket/server changes: Test 26 (`zsh -ic 'mku test_26_terminal'`).
- After Lithium JS/CSS: `npm test` + `npm run lint`.
- After payload changes: rebuild payload (`mkt`/`mka`), then Test 26.
- After bash script changes: `mks` (shellcheck).
- After docs: `mkl` (Test 04) + markdownlint (Test 90).
- Never log JWTs, keys, or secrets in logs or test artifacts.
- Follow existing test numbering and conventions; prefer extending existing
  tests.

## Relationship To Other Documents

| Document | Role |
| --- | --- |
| This file | Active terminal fix plan |
| [`AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md) | Phase 7 owns terminal WS auth gate (product lock) |
| [`test_26_terminal.md`](/docs/H/tests/test_26_terminal.md) | Blackbox test documentation |
| [`terminal_architecture.md`](/docs/H/core/reference/terminal_architecture.md) | Terminal subsystem architecture |
| [`terminal-generate.sh`](/elements/001-hydrogen/hydrogen/payloads/terminal-generate.sh) | Payload generator |
| [`terminal.js`](/elements/003-lithium/src/managers/terminal/terminal.js) | Lithium Terminal Manager |
| [`terminal.test.js`](/elements/003-lithium/tests/unit/managers/terminal.test.js) | Lithium unit tests |
| [`websocket_server_dispatch.c`](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c) | WebSocket auth + dispatch |
| [`info.c`](/elements/001-hydrogen/hydrogen/src/api/system/info/info.c) | System info endpoint (terminal config) |
| [`config_defaults.c`](/elements/001-hydrogen/hydrogen/src/config/config_defaults.c) | WebSocket config defaults |
| [`launch_websocket.c`](/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c) | WebSocket server startup |
| [`api_utils.c`](/elements/001-hydrogen/hydrogen/src/api/api_utils.c) | Client IP extraction |

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
  (line 699).
- Read `terminal-generate.sh` — confirmed version 2.1.1 removed the
  hardcoded `ABCDEFGHIJKLMNOP` fallback key.
- Read `test_26_terminal.sh` — confirmed it tests WebSocket connections
  with `Authorization: Key ${WEBSOCKET_KEY}` header.
- Read `terminal.test.js` — confirmed the updated test for re-init behavior
  is present (line 228–240).
- Confirmed the root cause of the production issue: WebSocket key
  `ABCDEFGHIJKLMNOP` in `/api/system/info` response suggests the config
  `WEBSOCKET_KEY` environment variable is either unset or set to that
  literal string; the WebSocket port 7001 must be exposed through DOKS +
  Traefik.
- Confirmed protocol mismatch: iframe sends `'terminal'`, Hydrogen default
  is `'hydrogen'`.
- This plan document authored.
