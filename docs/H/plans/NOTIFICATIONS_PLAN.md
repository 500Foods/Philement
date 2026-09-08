<!-- markdownlint-disable MD007 MD024 -->
# Notifications / Subscribers Plan

## Purpose

Define a gated, phase-by-phase plan for Hydrogen's **Web Push** backend: a
**Subscribers** API service plus a **Subscribers** subsystem that stores
browser push subscriptions and dispatches RFC 8291-encrypted payloads to
vendor push services so Lithium (or any webapp) can show a desktop/OS
notification even when the browser is closed.

This is **not** a second mail stack. Mail stays Mail Relay. Existing
`Notify` / `H.notify` stay as they are (SMTP config scaffold + permanent
Lua shim). This plan does not implement native iOS/Android APNs/FCM SDKs,
SMS, or the Oxygen element.

The analog is [MAILRELAY_PLAN_COMPLETE.md](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md)
(outbound delivery: config/launch, transport seam, queue/workers, Helium,
REST, Lua, local sink, blackbox). The newest full-subsystem template is
[MCP_COMPLETE.md](/docs/H/plans/complete/MCP_COMPLETE.md) (letter, launch
count, status metrics, Swagger, Test 17 clean skip).

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
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure or
  after adding/removing `src/` files), `mku <base>`, `mkp`, `mka`, `mks`.
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**. Follow this sequence:

1. **Confirm the prior phase is actually done.** Re-read its Status block
   and Exit gate before touching anything; do not trust memory of a prior
   session.
2. **Discuss the current phase first.** Re-read only that phase's Goal +
   Work items + Done means + Exit gate. Ask clarifying questions and do
   any research needed **before** writing any code.
3. **Ask for explicit approval to start implementation.** Do not begin
   editing source files until the user says go.
4. **Ask questions as they come up** during implementation rather than
   guessing at ambiguous requirements.
5. **Update the phase's Working Log entry when major pieces land** (not
   only at the very end).
6. **Record lessons learned** for the phase, even small ones.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean.** Intent to verify is
   not verification.
8. **Never apply a database migration.** Prepare/generate Helium packets
   and hand them to the user; do not run `schematool`/`schemahelper` apply.
9. **Follow existing project norms:** no `static` functions in `src/`;
   Unity one file per function ([TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md));
   blackbox `tests/test_NN_*.sh` ([TESTING.md](/docs/H/tests/TESTING.md));
   `jq` for JSON in Bash; absolute Markdown links; `mkq`/`mkt` then `mkp`
   after C; `mks` after scripts. Meet the [Coverage fences](#coverage-fences)
   and [Completeness fences](#completeness-fences).
10. **Never log VAPID private keys, subscription `auth` secrets, JWTs, or
    push payload bodies** in normal logs or test artifacts.
11. **Do not increment `TEST_COUNTER` in blackbox scripts.** The framework
    owns the counter.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-08):** Plan rewritten to Mail
Relay / MCP depth. Phase 0 design locks are **proposed, not approved**.
No C, Lua, or tests have been written. Do not start Phase 1 until Phase 0
Status is complete.

### Resume here next session

1. Confirm this document is the source of truth.
2. Re-read Phase 0 locks. If Status is still draft, discuss and get
   explicit approval before any `src/` edits.
3. Re-check disk for next Acuranzo migration / QueryRef before any Helium
   packet (`ls elements/002-helium/acuranzo/migrations/acuranzo_*.lua`).
   Last seen: **`acuranzo_1377.lua`** (QueryRef **#154**). Next free:
   migration **1378**, QueryRef **#155** (re-check; do not trust this
   snapshot).
4. Confirm config letter **V** is still free (`config.h` last letter is
   **U. Chat**). If someone shipped W, amend the lock.

### Session checklist

1. Read **CURRENT PAUSE POINT** and last **Working Log** entries.
2. Confirm previous phase **Status** is complete.
3. Re-read next phase Goal + Exit gate only.
4. Get approval → implement → verify gates → update this doc → stop.

## Priority

| | |
| --- | --- |
| **Band** | P2 — new product surface, after Auth Finale / quality gates |
| **Effort** | L–XL (crypto + HTTP + queue + Helium + REST + Lua + blackbox) |
| **Done** | 0% — plan only |
| **Why this shape** | Lithium's service worker already handles `push` / `notificationclick`. Hydrogen has no subscribe or dispatch path. Mail Relay is the analog for outbound delivery. |
| **Do not start casually** | Touches config letters, launch/landing dispatch, `AppConfig`, Swagger generation, status metrics, Helium, and a new blackbox slot. Phase 1 is plumbing-only so a missed table is not tangled with crypto. |

Backlog: [TODO.md item 26](/docs/H/TODO.md).

---

## Scope And Repo Areas

Primary: `/elements/001-hydrogen/hydrogen`

Related:

- `/elements/001-hydrogen/hydrogen/src/config/` — `config_subscribers.{c,h}`,
  defaults, `AppConfig` member **V. Subscribers**
- `/elements/001-hydrogen/hydrogen/src/launch/` — `launch_subscribers.c`,
  readiness, `launch.c` dispatch
- `/elements/001-hydrogen/hydrogen/src/landing/` — `landing_subscribers.c`,
  landing table + dispatch
- `/elements/001-hydrogen/hydrogen/src/subscribers/` — runtime (empty today)
- `/elements/001-hydrogen/hydrogen/src/api/subscribers/` — REST (empty today)
- `/elements/001-hydrogen/hydrogen/src/scripting/` — new `H.subscribers`;
  **do not** change `H.notify`
- `/elements/001-hydrogen/hydrogen/src/status/` — counters + JSON /
  Prometheus
- `/elements/001-hydrogen/hydrogen/src/globals.h` — `SR_SUBSCRIBERS`
- `/elements/001-hydrogen/hydrogen/extras/pushval/` — local Web Push sink
  (own CMake, like `extras/mailval/`)
- `/elements/001-hydrogen/hydrogen/extras/vapidgen/` — VAPID key mint
- `/elements/002-helium/acuranzo/migrations/` — `push_subscriptions` +
  QueryRefs + `push_send` role seed
- `/elements/003-lithium/` — subscribe client **deferred** (SW display
  path already exists)
- `/docs/H/` — operator guide, API pages, indexes (Phase 10 only)

Date of snapshot: 2026-09-08

---

## Goals And Non-Goals

### Goals

1. **New subsystem `Subscribers`** — peer of Mail Relay / MCP, not a
   Conduit query, not a Webhook, not Notify SMTP.
2. **Own launch / landing** — dedicated readiness, plan, launch, land,
   review. Disabled-by-default **clean skip** (`ready=true`, not a No-Go)
   so [test_17](/docs/H/tests/TESTING.md) min/max stay stable.
3. **Own config letter V** — operator supplies VAPID subject + keys,
   workers, queue, TTL/urgency caps, database name, test seams.
4. **Own status** — queue/worker counters on `GET /api/subscribers/status`
   and in `/api/system/info` + Prometheus.
5. **Web Push only** — RFC 8030 (HTTP), RFC 8291 (`aes128gcm`), RFC 8292
   (VAPID ES256). Browser `PushSubscription.endpoint` already is the
   vendor URL.
6. **Persistent subscriptions** — restart must not drop browser endpoints.
   Dispatch queue may start in-memory (Mail Relay Phase 3).
7. **One delivery path** — REST, Lua, and later events enqueue through
   `subscribers_dispatch`. Scripts must not POST to vendor hosts.
8. **Local sink in CI** — `extras/pushval` + injectable transport. No live
   Google / Apple / Mozilla calls.

### Non-goals (this plan)

- Folding Web Push into `Notify` / `Notify.SMTP` / `H.notify`.
- Proprietary FCM/APNs client libraries or native mobile tokens.
- Legacy Safari website-push IDs / `apple-app-site-association`.
- SMS, email (Mail Relay), in-app toasts (Lithium already has those).
- Lithium Notification Manager / `pushManager.subscribe` client (Phase 11,
  permanently deferred unless pulled in).
- Oxygen element productization.
- Persist dispatch queue / HA claim (Phase 14 optional).
- System-event → push in C (Lua rules later, not a v1 gate).
- Silent push (`userVisibleOnly: false`). Chrome forbids it.

---

## Current observed state (2026-09-08)

Do not re-implement these; they are constraints.

| Area | Status | Where |
| --- | --- | --- |
| Config letters | **A–U taken.** Last is **U. Chat**. **V is free.** | `src/config/config.h`, `hydrogen.h` `AppConfig` |
| Launch list | 21 `process_subsystem_readiness` calls (Registry … MCP). Chat is **config-only**, not a launch subsystem. | `launch_readiness.c` |
| `MAX_SUBSYSTEMS` | **24** / `INITIAL_REGISTRY_CAPACITY` **24**. Adding Subscribers = **22nd** registered. **No bump required.** | `globals.h` |
| `Notify` config/launch | SMTP **scaffold only**. No send runtime. Enabled defaults false. | `config_notify.*`, `launch_notify.c`, `landing_notify.c` |
| `H.notify` | **Permanent** deferred-error shim `"notify: deferred to mailrelay rules"` | `scripting_api_mail_notify.c`; [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md) |
| Mail Relay | Production outbound mail (REST, Lua `H.mail`, events, LogNotify) | [MAILRELAY_PLAN_COMPLETE.md](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md) |
| Webhooks | **Ingress** HMAC → allowlisted Lua. Not outbound push. | `config_webhooks.*`, `src/api/conduit/webhook/` |
| Chat | Config letter U, rate-limit only. Not a launch subsystem. | `config_chat.*` |
| Lithium PWA SW | `push` shows notification; `notificationclick` focuses a window. **No** `pushManager.subscribe`, no VAPID key fetch, no POST to Hydrogen. | `elements/003-lithium/public/service-worker.js`; [LITHIUM-PWA.md](/docs/Li/LITHIUM-PWA.md) |
| Lithium Notification Manager | Placeholder (menu ID 19) | [LITHIUM-MGR.md](/docs/Li/LITHIUM-MGR.md) |
| Oxygen (008) | Idea-stage brainstorm | `elements/008-oxygen/README.md` |
| Crypto on hand | Base64url, SHA-256, HMAC, RSA/RS256 via OpenSSL 3 `EVP_PKEY_*`. **No** ECDSA P-256 sign, **no** ECDH, **no** HKDF, **no** AES-128-GCM helper. OIDC lists `KEY_ALG_ES256` but keys are RSA. | `src/utils/utils_crypto.*`, `src/oidc/oidc_keys.c` |
| Outbound HTTP | libcurl via OIDC RP helpers; scripting wraps them | `oidc_rp_http_*`, `src/scripting/http_client.*` |
| API registration | JWT `protected_endpoints` + `json_endpoints` in `api_service.c`; Mail Relay does JWT **inside handlers** (not in the middleware list). Swagger `//@ swagger:` on handler headers; `payloads/swagger-generate.sh`. | `src/api/api_service.c`, `src/api/mailrelay/` |
| Role check | JWT `roles` claim = comma-separated **role_id integers**. Mail resolves `mail_send` via QueryRef **#127** Get Role By Name. | `mailrelay_api_auth.*`, `acuranzo_1260.lua` |
| Lua handles | `H_HK_QUERY=1` … `H_HK_MCP=6`. Next free: **`H_HK_SUBSCRIBERS = 7`**. `H.wait` must wire **both** single- and multi-handle paths. | `scripting_handle.h` |
| Status | `ServiceMetrics` has logging/webserver/websocket/mdns/print/database/scripting/**mcp**. Mail uses `QueueMetrics mail_relay_queue`. | `status_core.h`, `status_process.c`, `status_formatters.c` |
| Test 17 min | Almost empty JSON (`Server` + WebServer IPv4/IPv6 false). New subsystem **must** clean-skip when absent. | `tests/configs/hydrogen_test_17_startup_min.json` |
| Blackbox slots | **62 is free.** 57/58/61 = Mail Relay; 59 = auth chat; 60 = performance; 47 = MCP. | `tests/test_*.sh` |
| Helium | Last `acuranzo_1377.lua`, QueryRef **#154**. Next **1378** / **#155**. | `elements/002-helium/acuranzo/migrations/` |
| INSTRUCTIONS.md | Stale: letters end at T. MCP; launch order ends at 21 MCP; **U. Chat is missing**. This plan adds **V** / **22** and should also write U. Chat into the letter list so the doc matches `config.h`. | [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) |
| `landing_plan.c` | `expected_order[]` is stale (missing Scripting/Reporting/MCP). Do **not** rewrite the whole list. Add Subscribers adjacent to MCP in every **live** dispatch table (`launch_readiness.c`, `launch.c`, `landing.c`, `landing_readiness.c`). | `landing_plan.c` |

### Live subsystem count (do not guess)

`launch_readiness.c` registers **21** subsystems. `MAX_SUBSYSTEMS` is **24**.
Adding Subscribers uses slot 22. Do not bump 24 unless a later subsystem
lands in the same change.

---

## Protocol: what the backend actually talks to

Short answer to "does the backend talk to Apple/Google/Mozilla?": **yes,
but through the standard Web Push protocol, not through proprietary
FCM/APNs SDKs.**

Browsers implement the [Push API](https://www.w3.org/TR/push-api/). On
`pushManager.subscribe({ userVisibleOnly: true, applicationServerKey })`
the **browser** talks to **its** vendor push service and returns a
`PushSubscription`:

| Field | Meaning |
| --- | --- |
| `endpoint` | HTTPS URL on the vendor push service |
| `keys.p256dh` | Client ECDH public key (encrypt to this) |
| `keys.auth` | 16-byte auth secret |
| `expirationTime` | Optional; Chrome often `null` |

Typical `endpoint` hosts (the browser chooses these; Hydrogen does not):

| Browser | Push service host |
| --- | --- |
| Chrome / Edge / Android WebView | `fcm.googleapis.com` |
| Firefox | `updates.push.mozilla.org` |
| Safari 16+ (macOS); iOS 16.4+ **PWA added to Home Screen** | `web.push.apple.com` |

Hydrogen then:

1. Encrypts the JSON payload per RFC 8291 (`aes128gcm`: ECDH P-256 +
   HKDF-SHA256 + AES-128-GCM).
2. Signs a short-lived VAPID JWT (RFC 8292, ES256) with the
   **application server** ECDSA P-256 key. `aud` is the origin of that
   subscription's endpoint; `sub` is a `mailto:` or `https:` contact.
3. `POST`s the ciphertext to `subscription.endpoint` with headers
   `Authorization: vapid t=<jwt>, k=<applicationServerKey>`, `TTL`,
   optional `Urgency`, `Content-Encoding: aes128gcm`,
   `Content-Type: application/octet-stream`.

The vendor delivers to the device OS even if the browser is closed. The
service worker's `push` event then calls `showNotification`.

### Browser / ops constraints (document, do not fight)

- Push API requires **HTTPS** (localhost is exempt).
- Chrome requires `userVisibleOnly: true` (no silent push).
- Payloads are small (treat **4096 bytes** plaintext JSON as the hard cap
  before encrypt; ciphertext is larger).
- iOS: installed PWA only, 16.4+.
- `410 Gone` / some `404`s mean the subscription is dead — disable it.
  Do not retry gone.
- Urgency tokens (RFC 8030): `very-low`, `low`, `normal`, `high`.
- TTL: 0–2419200 seconds (28 days). Default 86400.

### RFC 8291 record (implementor cheat-sheet)

- Coding name: `aes128gcm`.
- Salt: 16 random bytes.
- Local ECDH P-256 keypair per message; public key uncompressed 65 bytes
  (`0x04 \|\| x \|\| y`).
- IKM via ECDH(local private, subscription `p256dh`).
- HKDF-SHA256 with info `WebPush: info\0 \|\| client_public \|\| server_public`
  and subscription `auth` as salt (RFC 8291 §3.3).
- Content encryption key + nonce from HKDF with `Content-Encoding: aes128gcm`
  / `Content-Encoding: nonce`.
- Plaintext padded with delimiter `0x02` then zero padding.
- AES-128-GCM; 16-byte tag appended.
- Body prefix: `salt (16) \|\| rs (4, typically 4096) \|\| idlen (1=65) \|\|
  local public (65) \|\| ciphertext+tag`.

Unity must lock a fixture pair (encrypt → decrypt round-trip + one RFC
vector if we have it). Injectable RNG/ECDH for determinism.

### RFC 8292 VAPID (implementor cheat-sheet)

- JWT header `{"typ":"JWT","alg":"ES256"}`.
- Claims: `aud` = origin of endpoint (`https://fcm.googleapis.com`),
  `exp` = now + a few hours (not days), `sub` = config Subject.
- Signature: ECDSA P-256 over `base64url(header).base64url(payload)`,
  IEEE P1363 (r\|\|s, 64 bytes) not DER.
- Header: `Authorization: vapid t=<jwt>, k=<urlsafe-b64 uncompressed public>`.
- `Crypto-Key: p256ecdsa=…` is the old draft; **do not send it** (RFC 8292
  replaced it).

VAPID keys in env: urlsafe-base64 **raw** keys (web-push library shape),
not PEM. `extras/vapidgen` prints `VAPID_PUBLIC_KEY` / `VAPID_PRIVATE_KEY`
ready for `${env.…}` substitution.

```diagram
Browser                    Hydrogen                     Vendor push service
  |                            |                                |
  |  GET /api/subscribers/vapid |                                |
  |<---- applicationServerKey --|                                |
  |                            |                                |
  |  pushManager.subscribe() ---------------------------------->|
  |<---- PushSubscription --------------------------------------|
  |                            |                                |
  |  POST /api/subscribers     |                                |
  |  (JWT + endpoint/keys)     |                                |
  |--------------------------->|  persist row                   |
  |                            |                                |
  |                            |  event / H.subscribers.send    |
  |                            |  encrypt RFC 8291              |
  |                            |  VAPID JWT RFC 8292            |
  |                            |  POST endpoint --------------->|
  |                            |                                |
  |  SW push + showNotification|<-------------------------------|
```

---

## Architecture

```text
Lithium / any webapp (later)          Hydrogen
  GET /api/subscribers/vapid   -->    subscribers_vapid (public)
  pushManager.subscribe()      -->    (browser <-> vendor; not us)
  POST /api/subscribers        -->    REST register --> repository
  POST /api/subscribers/dispatch -->  producer --> in-memory queue
                                         |
                                         v
                                      workers (N)
                                         |
                          RFC 8291 encrypt + RFC 8292 JWT
                                         |
                                         v
                              subscribers_http_post (seam)
                                         |
                          production: vendor endpoint
                          tests: extras/pushval or injected fn
```

### Responsibility split

| Layer | Knows | Must not know |
| --- | --- | --- |
| **C Subscribers** | VAPID keys, RFC 8291/8292, queue, HTTP POST, 410 disable, SSRF allowlist | Lithium UI, printer names, mail templates |
| **C API** | JWT, `push_send` role, JSON envelopes | Vendor hostnames beyond the allowlist helper |
| **Lua `H.subscribers`** | title/body/ttl/urgency/user_id | libcurl, OpenSSL, raw endpoint POST |
| **Helium** | `push_subscriptions` rows + QueryRefs | Crypto |
| **`pushval`** | Record method/path/headers/body; reply 201/410/429 | Hydrogen internals |

### Why a subsystem (not Notify, not Conduit)

- Own kill switch and VAPID secrets.
- Own worker threads and shutdown drain (`test_16` / `test_17`).
- Own counters.
- Notify is an unused SMTP scaffold; Mail Relay already owns mail.
- Conduit is inbound query/script, not outbound fan-out.

---

## Proposed design locks (Phase 0)

These are **proposed**. Phase 0 is complete only when they are approved
or explicitly amended in this section.

1. **Web Push only (RFC 8030 + RFC 8291 + RFC 8292).** No proprietary
   FCM/APNs client libraries. Safari 16+ is the same protocol via
   `web.push.apple.com`.
2. **New subsystem `Subscribers`**, not a reuse of `Notify`.
   - Config letter **V. Subscribers** (after **U. Chat**). **Not U.**
   - Launch order **22** (after MCP, last registered).
   - `SR_SUBSCRIBERS` `"Subscribers"`.
   - Source: `src/subscribers/`.
   - API: `src/api/subscribers/`, Swagger tag **Subscribers**, prefix
     `/api/subscribers`.
3. **Do not touch `H.notify`.** New Lua surface: `H.subscribers.send` /
   `send_sync`. Handle kind `H_HK_SUBSCRIBERS = 7`.
4. **Leave `NotifyConfig` SMTP scaffold and `launch_notify` alone.** No
   parallel SMTP, no folding Web Push into `Notify.SMTP`.
5. **Subscriptions persist in the database** (restart must not drop
   browser endpoints). Dispatch **queue** starts in-memory (Mail Relay
   Phase 3 pattern). Persist-queue is Phase 14 optional.
6. **`Subscribers.Database`** names the Hydrogen database connection that
   owns `push_subscriptions` (Mail Relay analog). Default to the sole
   configured connection when omitted.
7. **JWT** on register / unregister / list / status / dispatch, checked
   **inside handlers** (Mail Relay pattern), not via
   `protected_endpoints` middleware — because GET vapid must stay public
   on the same prefix. **GET vapid is public** (the key is not a secret).
   Dispatch REST requires role **`push_send`**, resolved at request time
   via existing QueryRef **#127** Get Role By Name (do **not** invent a
   second resolver). Seed the role in Helium (Mail `mail_send` analog:
   `acuranzo_1257`). Any authenticated user may register/list/delete
   **their own** subscription.
8. **SSRF fence (v1, not later).** Register and POST reject non-HTTPS
   endpoints unless `Test.PushEndpointOverride` is set. Default host
   allowlist: `fcm.googleapis.com`, `updates.push.mozilla.org`,
   `web.push.apple.com`, plus the override host in test. Reject
   RFC1918 / link-local / localhost unless test override. An
   authenticated attacker must not be able to make Hydrogen POST to
   `169.254.169.254`.
9. **Lithium Notification Manager and `pushManager.subscribe` client
   are deferred** (same as Mail Relay Phase 9). This plan ships
   Hydrogen + Helium + blackbox sink. Lithium SW already displays
   pushes; wiring subscribe is a later Lithium phase.
10. **Oxygen stays idea-stage.** Hydrogen owns this backend.
11. **No live vendor calls in tests.** `extras/pushval` local HTTP sink
    - injectable POST transport (Mail Relay `mailval` / smtp seam).
    `pushval` has **its own CMakeLists** and is **not** in Hydrogen's
    recursive `src/` glob.
12. **Blackbox Test 62**, ports **5620–5626** (`5<TT>x` per
    [TESTING.md](/docs/H/tests/TESTING.md)). If 562x collides in
    practice (MCP lesson: Test 47 moved to 1547x), switch to **15620–
    15626** and record the variance. Do not create the script until
    Phase 9.
13. **Helium packets only; never apply.** Next IDs re-checked at packet
    time. Snapshot: migration **1378**, QueryRef **#155**. Reuse #127
    for role lookup. New QueryRefs start at #155.
14. **One delivery path.** REST, Lua, and (later) system events enqueue
    through `subscribers_dispatch`. Scripts must not POST to vendor
    endpoints themselves.
15. **No v1 idempotency key.** Duplicate dispatch is acceptable (two
    notifications). Do not copy Mail Relay 11.4 unless a later phase
    asks.
16. **No `Topic` header in v1.** TTL + Urgency only.
17. **OpenSSL 3 EVP APIs** for P-256 / ECDH / HKDF / AES-GCM, matching
    `utils_crypto.c`. Prefer `src/subscribers/` helpers first; promote
    to `utils_crypto` only if a second caller appears.
18. **Disabled by default.** Missing `Subscribers` section = clean skip.
    Enabled without VAPID subject + both keys = No-Go.
19. **Error envelope** matches Mail/Conduit: `{ success, error, message }`
    with `PUSH_*` codes (table below).
20. **Meet completeness + coverage fences** before Phase 15 Status
    complete. A feature that works but is missing from
    `INSTRUCTIONS.md` / `STRUCTURE.md` / Swagger / status / Test 17 is
    not done.

### Locked endpoint sketch

| Method | Path | Auth | Body / result |
| --- | --- | --- | --- |
| GET | `/api/subscribers/vapid` | none | `{ "applicationServerKey": "<urlsafe-b64 uncompressed P-256>" }` |
| POST | `/api/subscribers` | JWT | PushSubscription JSON (`endpoint`, `keys.p256dh`, `keys.auth`) |
| DELETE | `/api/subscribers` | JWT | `{ "endpoint": "https://..." }` — owner only |
| GET | `/api/subscribers` | JWT | List current user's subscriptions (no `auth` secret in response) |
| GET | `/api/subscribers/status` | JWT | Queue/worker counters (no `push_send` required) |
| POST | `/api/subscribers/dispatch` | JWT + `push_send` | `{ "user_id" \| "endpoint", "title", "body", "url"?, "ttl"?, "urgency"? }` → `{ "success", "message_id", "status": "queued" }` |

Prefix honours `Api.Prefix` (`test_20`).

### Locked `PUSH_*` error codes

| Code | HTTP | When |
| --- | --- | --- |
| `PUSH_DISABLED` | 503 | Subsystem disabled or not launched |
| `PUSH_AUTH_REQUIRED` | 401 | Missing/invalid JWT or missing database claim |
| `PUSH_FORBIDDEN` | 403 | Missing `push_send`, or mutating another user's endpoint |
| `PUSH_INVALID_SUBSCRIPTION` | 400 | Missing/malformed endpoint, p256dh, or auth |
| `PUSH_ENDPOINT_INSECURE` | 400 | SSRF / allowlist / non-HTTPS rejection |
| `PUSH_PAYLOAD_TOO_LARGE` | 413 | Over `MaxPayloadBytes` |
| `PUSH_VAPID_UNAVAILABLE` | 503 | Keys missing at dispatch time |
| `PUSH_QUEUE_FULL` | 503 | Bounded queue overflow |
| `PUSH_NOT_FOUND` | 404 | Unknown endpoint for this user |
| `PUSH_GONE` | 410 | Already disabled (optional on delete) |
| `PUSH_INVALID_TTL` | 400 | TTL out of range |
| `PUSH_INVALID_URGENCY` | 400 | Not one of the four tokens |
| `PUSH_RATE_LIMITED` | 429 | Phase 12 |

### Locked config sketch

```json
"Subscribers": {
  "Enabled": false,
  "Database": "",
  "Vapid": {
    "Subject": "mailto:${env.HYDROGEN_DEV_EMAIL}",
    "PublicKey": "${env.VAPID_PUBLIC_KEY}",
    "PrivateKey": "${env.VAPID_PRIVATE_KEY}"
  },
  "Workers": 2,
  "QueueCapacity": 1000,
  "DefaultTTL": 86400,
  "MaxTTL": 2419200,
  "DefaultUrgency": "normal",
  "MaxPayloadBytes": 4096,
  "AllowedHosts": [
    "fcm.googleapis.com",
    "updates.push.mozilla.org",
    "web.push.apple.com"
  ],
  "Test": {
    "FailNextSendOnLaunch": false,
    "PushEndpointOverride": ""
  }
}
```

Ranges (No-Go when Enabled): Workers 1–16; QueueCapacity 1–100000;
DefaultTTL 0–MaxTTL; MaxPayloadBytes 1–4096.

VAPID keys are operator-generated (`extras/vapidgen` in Phase 2). Do not
auto-write secrets into `hydrogen.json`. Dump redacts the private key as
`*****`.

Env substitution uses the existing `${env.NAME}` loader (Test 12 pattern).

### Locked data sketch (Helium, Phase 5)

Table working name `push_subscriptions`:

- `subscription_id` integer PK (`MAX+1` + confirm/retry per TODO 12e)
- `user_id` (JWT subject / existing users FK pattern)
- `endpoint` unique
- `p256dh`, `auth` (secrets at rest; never log)
- `expiration_time` nullable
- `user_agent` optional
- `created_at`, `last_seen_at`, `disabled_at` nullable (410 Gone)
- `${COMMON_CREATE}` audit columns

QueryRefs (assign at packet time; snapshot start **#155**):

| Working name | Use |
| --- | --- |
| Insert Subscription | register upsert |
| Get By Endpoint | owner check / dispatch one |
| List By User | GET list (exclude auth column in C, not in SQL if we must select it — prefer SQL that does **not** return `auth` for list) |
| List Active For User | dispatch fan-out (`disabled_at` IS NULL) |
| Disable By Endpoint | 410 / DELETE |
| Touch Last Seen | successful delivery |

Plus Helium **seed** `push_send` role (new migration, not a QueryRef).
Reuse QueryRef **#127** in C. Do not duplicate Get Role By Name.

Internal QueryRefs: `internal_sql` / `system_sql`, never `public` /
`protected`.

### Locked source layout

Follow `src/mdns/` / `src/mailrelay/` (service with threads):

```text
src/subscribers/
  subscribers.h              public producer / lifecycle
  subscribers_internal.h     shared internals, no static helpers
  subscribers.c              init / shutdown / metrics snapshot
  subscribers_vapid.c        RFC 8292
  subscribers_crypto.c       RFC 8291
  subscribers_http.c         POST seam
  subscribers_queue.c
  subscribers_workers.c
  subscribers_retry.c
  subscribers_producer.c
  subscribers_repository.c
  subscribers_shutdown.c     drain + join

src/api/subscribers/
  subscribers_service.h      //@ swagger:service Subscribers
  subscribers_api_auth.c/.h  push_send via #127
  vapid/vapid.c/.h
  register/register.c/.h
  unregister/unregister.c/.h
  list/list.c/.h
  status/status.c/.h
  dispatch/dispatch.c/.h
```

Each `.c` begins with `#include <src/hydrogen.h>`. Includes use
`<src/folder/...>`. Every function has a header prototype. **No `static`
functions.** File-scope state only.

`extras/pushval/` and `extras/vapidgen/` are **standalone** CMake
targets, not picked up by `../src/*.c`.

---

## Completeness fences

A phase is not done if the behavior works but Hydrogen's **normal
structures** were skipped. Phase 1 lands the plumbing rows. Phase 15
re-checks the whole table.

### Config / AppConfig

| Must exist | Notes |
| --- | --- |
| Letter **V** in `config.h` comment block | After U. Chat |
| `config_forward.h` `SubscribersConfig` | |
| `config_subscribers.h/.c` | load / dump (redact private key) / cleanup / apply_defaults |
| `initialize_config_defaults_subscribers` | `config_defaults.c` + header; called from master init |
| `LOAD_CONFIG("V", SR_SUBSCRIBERS, …)` | `config.c` |
| `DUMP_CONFIG_SECTION("V", …)` | `config.c` — Chat dump is currently missing; do not "fix" Chat as a drive-by except the INSTRUCTIONS letter list |
| `cleanup_subscribers_config` in `cleanup_application_config` | `config.c` |
| `AppConfig.subscribers` | `hydrogen.h` |
| Example `hydrogen.json` section | `Enabled: false` |
| `tests/artifacts/hydrogen_config_schema.json` | If Mail Relay / MCP updated it, we do too |
| Test 12 env | If VAPID keys are `${env.*}` in a test config |

### Launch / landing / registry / threads

| Must exist | Notes |
| --- | --- |
| `SR_SUBSCRIBERS` | `globals.h` |
| `launch_subscribers.c` | readiness clean-skip when disabled; No-Go when enabled+invalid |
| `launch.h` `check_*` + `launch_*` decls | |
| `launch_readiness.c` after MCP | 22nd `process_subsystem_readiness` |
| `launch.c` `strcmp` dispatch | |
| `landing_subscribers.c` | drain workers, join, registry shutdown |
| `landing.h` decls | |
| `landing.c` dispatch | |
| `landing_readiness.c` table | Land Subscribers **before** Database (workers may still query). Place **immediately before MCP** in the table unless a better reverse-order slot is obvious. |
| `volatile sig_atomic_t subscribers_system_shutdown` | `state.c` + externs matching Notify/Mail Relay |
| `ServiceThreads subscribers_threads` | define `state.c`, extern `threads.h`, count in thread status if Mail/MCP do |
| `register_subsystem_from_launch` | same helper MCP uses |
| Dependencies | Registry + Network; Database when Enabled |

`MAX_SUBSYSTEMS` stays 24 unless the 22nd slot somehow does not fit
`ReadinessResults.results[]` (it will).

### API / Swagger / prefix

| Must exist | Notes |
| --- | --- |
| Handler files + `subscribers_service.h` tag | `payloads/swagger-generate.sh` reads `//@ swagger:` |
| `api_service.c` includes, startup log lines, `handle_api_request` branches | |
| JWT inside handlers | vapid **not** protected |
| `json_endpoints` for POST/DELETE bodies | |
| Regenerated Swagger in payload | `mkt` embeds payload; `test_22` |
| Honour `Api.Prefix` | `test_20` must not 404 `/<prefix>/subscribers/vapid` |

### Status / observability

| Must exist | Notes |
| --- | --- |
| Counters struct | queued, sending, sent, failed, retrying, gone, last_success, last_failure, worker_count, queue_depth |
| `GET /api/subscribers/status` | JWT, no `push_send` |
| `status_core.h` + `status_process.c` + `status_formatters.c` | JSON + Prometheus, MCP analog |
| Log with `log_this(SR_SUBSCRIBERS, …)` | `num_args` matches `%` count |
| Never log secrets or bodies | grep fence in Phase 12 |

### Scripting

| Must exist | Notes |
| --- | --- |
| `H.subscribers.send` / `send_sync` | producer only |
| `H_HK_SUBSCRIBERS` | `scripting_handle.h` |
| `H.wait` single **and** multi-handle | Mail Relay lesson |
| `H.notify` regression | still the mail shim string |
| `lua_api.md` | Phase 10 |

### Helium

| Must exist | Notes |
| --- | --- |
| Packet on disk | table + indexes + QueryRefs + `push_send` role |
| Dialect macros | `${INTEGER}`, `${TIMESTAMP_TZ}`, `${COMMON_CREATE}`, … |
| `test_98` luacheck | |
| User-applied before live blackbox | agent never applies |
| `test_34` (SQLite) after apply if user asks | not an agent apply |

### Tests / extras

| Must exist | Notes |
| --- | --- |
| Unity under `tests/unity/src/subscribers/` and `…/api/subscribers/` and `…/config/` `…/launch/` `…/landing/` | unique filenames; search before create |
| `extras/pushval/` standalone CMake | not in `src/` glob |
| `extras/vapidgen/` | |
| `tests/test_62_subscribers.sh` + `.md` + configs | Phase 9 only |
| CHANGELOG + TEST_VERSION | every script change |
| `jq` only for JSON | no grep-JSON |
| No `TEST_COUNTER=$((TEST_COUNTER + 1))` | |
| `test_00` discovery | glob `test_NN_*.sh` — no manual register if that is still true |
| `test_17` min/max | disabled skip; max may enable with test keys |
| `test_91` / `test_92` / `test_93` / `test_98` / `test_99` / Test 04 | as touched |
| Dead-code gate | `build/deadcode/dead_functions.txt` empty for new symbols after `mkt` |

### Docs (Phase 10, then Phase 15 re-check)

| Must exist | Notes |
| --- | --- |
| `docs/H/PUSH_GUIDE.md` | **Do not create or link before Phase 10** (Test 04 orphans) |
| `docs/H/api/subscribers/` | |
| [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) | letters A–V including **U. Chat** (existing omission) + launch **22** |
| [STRUCTURE.md](/docs/H/STRUCTURE.md) | every new file |
| [SITEMAP.md](/docs/H/SITEMAP.md) | |
| [README.md](/docs/H/README.md) | |
| [API_OVERVIEW.md](/docs/H/core/API_OVERVIEW.md) | |
| [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) | |
| [TESTING.md](/docs/H/tests/TESTING.md) | Test 62 row |
| [SECRETS.md](/docs/H/SECRETS.md) | `VAPID_*` env names, not values |
| [TODO.md](/docs/H/TODO.md) item 26 | progress |
| Test 04 / `mkl` | absolute links, no `:line` refs |

---

## Coverage fences

From [CURIOSITIES.md](/docs/H/CURIOSITIES.md), [PROMPTS.md](/docs/H/PROMPTS.md),
[README.md](/docs/H/README.md):

| Fence | Rule |
| --- | --- |
| Unity, file **< 100** instrumented lines | **> 50%** |
| Unity, file **≥ 100** instrumented lines | **> 75%** |
| Combined Unity **or** blackbox | **85%** project target; new `src/subscribers/` and `src/api/subscribers/` files should be at or above the per-file Unity fence before Phase 15 |
| `hydrogen.c` | excluded (architecture) |
| Test 99 | no new file **> 1000** lines — split instead |
| `extras/add_coverage.sh` | use when a file is under fence |
| Seams | injectable transport, RNG, repository, producer, role lookup — so Unity never needs live curl/DB/vendor |
| `static` | build gate fails `mkq`/`mkt` on new `static` functions in `src/` |
| Dead functions | new public symbols must have a caller; do not baseline |

Each C phase Exit gate includes: named `mku` green, `mkp` green, and
**no new `src/subscribers/` or `src/api/subscribers/` file ≥100 lines
below 75%** (run the coverage helper or Test 89 slice). Do not wait until
Phase 13 to discover a 200-line file at 20%.

---

## Reference Conventions

Match these so cppcheck and launch/landing/registry/thread machinery pass.

### Launch / landing handlers

Model `launch_mcp.c` / `launch_mail_relay.c` / `landing_websocket.c`:

- `LaunchReadiness check_subscribers_launch_readiness(void)` builds
  messages with `add_launch_message`, first line `SR_SUBSCRIBERS`, then
  `Go:` / `No-Go:`, final `Decide:  Go For Launch of …`,
  `finalize_launch_messages`.
- Disabled → `ready=true` clean skip (Mail Relay / MCP).
- `launch_subscribers_subsystem()` returns `1`/`0`. Spawn workers with
  `pthread_create` + `add_service_thread`. `update_subsystem_on_startup`.
- Landing: set shutdown flag, poll `update_service_thread_metrics` until
  `thread_count == 0` or timeout, then reset tracking.

### Thread tracking

`ServiceThreads subscribers_threads`.
`init_service_threads(&subscribers_threads, SR_SUBSCRIBERS)`.
`MAX_SERVICE_THREADS` is 1024.

### API handlers

Signature: `enum MHD_Result handle_<x>_request(...)`.
POST buffering: `api_buffer_post_data` / `api_free_post_buffer`.
JSON: `api_parse_json_body` (jansson).
JWT: `extract_and_validate_jwt` after Bearer prefix.
Send: `api_send_json_response` (takes ownership).
Errors: `{ success:false, error, message }` +
`api_send_error_and_cleanup`.

GET status must **not** use `handle_method_validation()` if that helper
only allows POST (Mail Relay lesson).

### libcurl (thread-safe)

Per-request easy handle. Always `CURLOPT_NOSIGNAL = 1L`. Connect/timeout
set. `curl_slist` for headers. Read `CURLINFO_RESPONSE_CODE`. Cleanup
slist + easy. Model `oidc_rp_http` `apply_common_curl_opts`.

Status map: 201 success; 410/404 gone; 413 too large; 429 retry; 5xx
retry; else fail. Do not retry gone.

### Logging

`log_this(SR_SUBSCRIBERS, fmt, level, num_args, ...)`. `num_args` equals
`%` count. Never log VAPID private, `auth`, JWT, or payload bodies.

### Helium

`acuranzo_NNNN.lua` after current max. Cross-engine macros. luacheck
header + CHANGELOG. QueryRefs `internal_sql`. Agent hands packet to
user.

### Blackbox

`test_62_subscribers.sh`: shebang, title, FUNCTIONS, CHANGELOG (newest
first), `set -euo pipefail`, `TEST_NAME` / `TEST_ABBR` / `TEST_NUMBER` /
`TEST_VERSION`, `TEST_COUNTER=0` once, `source lib/framework.sh`. Configs
`tests/configs/hydrogen_test_62_*.json`. Lifecycle helpers. `jq` for
JSON. Matching `docs/H/tests/test_62_subscribers.md`. Run Test 04 after
adding that doc.

### Unity

`tests/unity/src/` mirrors `src/`. Name `<source>_test_<function>.c`.
Search before creating. `#include <src/hydrogen.h>` then `<unity.h>`.
No `.c` includes. `mku <base>`. Prefer one function per file; if the
Mail Relay analog for that layer is a suite file (`mailrelay_otp_test_generate`),
match that analog.

### Gate philosophy

- One logical behavior per work item where practical.
- After ordinary C: `mkq` then `mkp`. After add/remove `src/` files: `mkt`
  then `mkp`.
- After Bash: `mks`.
- After Lua: `test_98`.
- After Markdown: Test 04 / `mkl`.
- Do not mark complete on skipped gates.

---

## Phase 0 — Design lock

### Goal

Approve or amend the locks above. No `src/` edits.

### Entry gate

This plan exists and has been read.

### Work items

- [ ] 0.1 Confirm Web Push-only (no FCM/APNs SDKs, no legacy Safari).
- [ ] 0.2 Confirm new `Subscribers` subsystem vs folding into `Notify`.
- [ ] 0.3 Confirm letter **V** (U is Chat) and launch **22**.
- [ ] 0.4 Confirm `H.subscribers` + `H_HK_SUBSCRIBERS=7` (not `H.notify`,
      not `H.push`).
- [ ] 0.5 Confirm JWT-in-handlers / public-vapid / `push_send` via #127.
- [ ] 0.6 Confirm SSRF allowlist is v1 (not Phase 12-only).
- [ ] 0.7 Confirm Lithium UI deferred; Hydrogen+Helium+`pushval` only.
- [ ] 0.8 Confirm Test 62 / ports 562x / Helium re-check / never-apply.
- [ ] 0.9 Confirm completeness + coverage fences as Exit criteria for
      Phase 15.
- [ ] 0.10 Record amendments in this document if any lock changes.

### Done means

Locks in this file match what the user approved; Status complete.

### Exit gate

User explicit approval of Phase 0. No C compiled.

### Status

**draft — awaiting approval** (2026-09-08)

### Lessons learned

- 2026-09-08: First draft used letter **U**. Live `config.h` already has
  **U. Chat**. Corrected to **V** in this rewrite. Chat is not a launch
  subsystem.

---

## Phase 1 — Config, launch, landing

### Goal

`Subscribers` config loads, dumps (redacted), cleans up, and the
subsystem registers, launches, and lands with Enabled true/false. **No
network, no crypto, no API yet.**

### Dependencies

Phase 0 complete.

### Entry gate

Phase 0 Status complete. User said go.

### Work items

- [ ] 1.1 `config_subscribers.h/.c` + defaults + `AppConfig` field +
      load/dump/cleanup + letter V in `config.h` / `config_forward.h` /
      `config.c` load/dump/cleanup. Env overrides for VAPID
      keys/subject. AllowedHosts array. Test substruct.
      Verification: `mku config_subscribers_test_load_subscribers_config`.
- [ ] 1.2 `SR_SUBSCRIBERS`, launch order 22, `launch_subscribers.c`,
      `landing_subscribers.c`, wire `launch.h` / `launch.c` /
      `launch_readiness.c` / `landing.h` / `landing.c` /
      `landing_readiness.c`. Shutdown flag + `subscribers_threads`.
      Verification: `mku` launch readiness + landing tests (Notify/MCP
      analogs).
- [ ] 1.3 Readiness: disabled or missing section → Go clean skip;
      enabled → require VAPID subject + both keys + worker/queue
      ranges + Database resolvable when set. Do not open sockets.
- [ ] 1.4 Example `hydrogen.json` section `Enabled: false`. Schema
      artifact if that file is maintained.
- [ ] 1.5 `mkt` (new `src/` files), then `mkp`. `test_17` min still
      starts (disabled skip). Dead-code: unused launch symbols must
      still be referenced from dispatch tables.

### Done means

Trial build green; Unity config/launch/landing pass; dump redacts
private key; disabled launch is a no-op success; `test_17` min not
regressed.

### Exit gate

`zsh -ic 'mkt'`; named `mku`; `zsh -ic 'mkp'`; `test_17` min if run.

### Status

not started

---

## Phase 2 — VAPID keys and JWT (RFC 8292)

### Goal

Load ECDSA P-256 VAPID keys; emit `applicationServerKey`; sign/verify
VAPID JWTs. No HTTP yet.

### Work items

- [ ] 2.1 P-256 load from urlsafe-b64 raw private (32) + public (65).
      Reject wrong curve / wrong length. Uncompressed public → urlsafe
      b64 (no padding) for `applicationServerKey`.
- [ ] 2.2 VAPID JWT ES256 (P1363 r\|\|s). `aud` = origin of endpoint
      URL. `exp` hours, not days. Header format locked above.
- [ ] 2.3 `extras/vapidgen` (small C + own CMake, or documented
      `openssl` recipe that prints env-ready values). Do not invent a
      blackbox `test_NN`.
- [ ] 2.4 Unity: known-vector sign/verify; reject wrong curve; tests do
      not print private keys.
- [ ] 2.5 `mkt`/`mkq`, `mku`, `mkp`. Coverage fence on new files.

### Done means

Given a private key + endpoint URL, C produces a JWT that verifies with
the public key; `applicationServerKey` matches the public key bytes.

### Exit gate

Named `mku` green; `mkp` green; coverage fence.

### Status

not started

---

## Phase 3 — Payload encryption (RFC 8291)

### Goal

Encrypt/decrypt `aes128gcm` bodies with subscription `p256dh` + `auth`.

### Work items

- [ ] 3.1 ECDH P-256, HKDF-SHA256, AES-128-GCM padding/record as RFC 8291.
- [ ] 3.2 Enforce `MaxPayloadBytes` **before** encrypt.
- [ ] 3.3 Unity against a locked fixture pair + injectable RNG/ECDH.
- [ ] 3.4 `mkq`, `mku`, `mkp`. No `static` helpers. Coverage fence.

### Done means

Plaintext JSON in → ciphertext + salt/record that decrypt recovers.
Oversized payload fails closed.

### Exit gate

Named `mku` green; `mkp` green; coverage fence.

### Status

not started

---

## Phase 4 — HTTP POST seam + `pushval`

### Goal

POST ciphertext to an endpoint URL with Web Push headers. Transport is
swappable. Local sink records requests. No real vendor hosts.

### Work items

- [ ] 4.1 `subscribers_http_post` wrapping OIDC RP / libcurl POST.
      Headers locked in Protocol section. Status map locked. SSRF
      allowlist enforced here as well as at register.
      `CURLOPT_NOSIGNAL=1`.
- [ ] 4.2 Injectable transport function pointer (Mail Relay SMTP
      pattern).
- [ ] 4.3 `extras/pushval`: tiny HTTP listener, own CMakeLists, captures
      method/path/headers/body; replies 201/410/429 on command. Not a
      `test_NN` yet.
- [ ] 4.4 Unity with injected transport; `mks` if any shell wrappers.
- [ ] 4.5 `Test.PushEndpointOverride` accepted in config (used in Phase 9).
- [ ] 4.6 `mkq`/`mkt`, `mkp`, coverage fence.

### Done means

Injected 201 → success; injected 410 → gone code; `pushval` can be
started and records a POST.

### Exit gate

Named `mku`; `mkp`; `mks` if scripts added.

### Status

not started

---

## Phase 5 — Helium persistence + repository

### Goal

`push_subscriptions` table + QueryRefs + `push_send` role seed + C
repository. Packet handed to the user; not applied by the agent.

### Work items

- [ ] 5.1 Re-check next migration/QueryRef on disk.
- [ ] 5.2 Helium packet: table + unique endpoint index + QueryRefs
      (insert, get-by-endpoint, list-by-user without `auth`,
      list-active-for-user, disable, touch) + `push_send` role seed.
      Dialect-safe; no business SQL in C. Reuse #127.
- [ ] 5.3 Hand packet to the user. Do not apply.
- [ ] 5.4 `subscribers_repository.c` QueryRef calls; MAX+1 confirm/retry
      (TODO 12e). Never log `auth` / p256dh.
- [ ] 5.5 Unity repository with mocks; `test_98` luacheck on new Lua.
      After user applies: `test_34` only if the user asks in this phase.

### Done means

Packet exists; C compiles against QueryRef numbers; Unity repository
pass. Live apply is a **user** gate.

### Exit gate

`mkt`/`mkq`, `mku`, `mkp`, luacheck. User confirms apply before Phase 6
relies on a live table (Unity may mock until then).

### Status

not started

---

## Phase 6 — REST Subscribers service

### Goal

Thin API surface registered like Mail Service. Swagger annotations
complete. Dispatch handler may 503 until Phase 7.

### Work items

- [ ] 6.1 `subscribers_service.h` (`//@ swagger:service` / tag
      **Subscribers**). Handlers: vapid, register, unregister, list,
      status (zeros until Phase 7), dispatch stub 503
      `PUSH_DISABLED` or `PUSH_QUEUE_FULL` — **not** a silent success.
- [ ] 6.2 Wire `api_service.c` routes + `json_endpoints`. GET vapid is
      **not** JWT-gated.
- [ ] 6.3 Register: validate HTTPS + allowlist + keys; upsert by
      endpoint bound to JWT user; reject stealing another user's
      endpoint (`PUSH_FORBIDDEN`).
- [ ] 6.4 Unregister/list: owner only; list omits `auth`.
- [ ] 6.5 `subscribers_api_auth` for `push_send` via #127 (copy
      `mailrelay_api_auth` shape, do not share its types).
- [ ] 6.6 Unity per handler; `mkq`/`mkt`, `mkp`. Run
      `payloads/swagger-generate.sh` path as existing builds do;
      `test_22` if payload regenerated.
- [ ] 6.7 Coverage fence.

### Done means

Swagger lists Subscribers; JWT register/list/delete work against mocks;
vapid is public; dispatch stub is an honest 503.

### Exit gate

Named `mku`; `mkp`. Live blackbox is Phase 9.

### Status

not started

---

## Phase 7 — Dispatch queue, workers, observability

### Goal

In-memory queue + worker threads encrypt and POST. 410 disables the
row. Launch/landing start/stop workers. Status counters live.

### Work items

- [ ] 7.1 Queue (bounded) + workers + retry/backoff (429/5xx). Copy Mail
      Relay structure, do not share its types.
- [ ] 7.2 `subscribers_dispatch` / producer used by REST + Lua later.
- [ ] 7.3 410/gone → repository disable. Do not retry gone.
- [ ] 7.4 `POST /api/subscribers/dispatch` with `push_send`.
- [ ] 7.5 Status counters + `status_core.h` / `status_process.c` /
      `status_formatters.c` JSON + Prometheus. `FailNextSendOnLaunch`
      test seam.
- [ ] 7.6 Unity queue/workers/retry/producer; `mkt`/`mkq`, `mkp`.
      `test_17` still clean (new subsystem must shut down). Coverage
      fence.

### Done means

Dispatch enqueues; worker delivers to injected transport; gone prunes;
shutdown joins workers without hang; system info shows counters.

### Exit gate

Named `mku`; `mkp`; `test_17` if run.

### Status

not started

---

## Phase 8 — Lua `H.subscribers`

### Goal

Scripts can send a push. `H.notify` unchanged.

### Work items

- [ ] 8.1 `H.subscribers.send` / `send_sync` → producer. Wait helper
      wired in **both** single- and multi-handle `H.wait` paths.
      `H_HK_SUBSCRIBERS = 7`.
- [ ] 8.2 Reject missing title/body; honour ttl/urgency caps;
      `PUSH_*` mapped to Lua errors.
- [ ] 8.3 Unity scripting tests including **notify-shim regression**.
- [ ] 8.4 `mkq`, `mku`, `mkp`. Coverage fence. Full operator docs are
      Phase 10.

### Done means

Lua enqueue returns `{ message_id, status = "queued" }`; `H.notify`
still returns the mail shim string.

### Exit gate

Named `mku` including notify-shim regression; `mkp`.

### Status

not started

---

## Phase 9 — Blackbox Test 62 + `pushval`

### Goal

End-to-end: start Hydrogen with Subscribers enabled, VAPID keys, local
`pushval`, JWT register, dispatch, sink sees ciphertext/headers, 410
path disables subscription. No public Internet.

### Work items

- [ ] 9.1 `tests/test_62_subscribers.sh` + `docs/H/tests/test_62_subscribers.md`
      + configs on ports **562x**. `jq` only. CHANGELOG + TEST_VERSION.
      Do not increment `TEST_COUNTER`.
- [ ] 9.2 Orchestrator discovery (glob). Do not collide with 561x
      (test 61 inbound mail). If 562x is unusable, variance to 1562x.
- [ ] 9.3 Cases: vapid public; unauth register 401; register+list (no
      `auth` in JSON); dispatch 403 without role; dispatch 200 queued;
      pushval 201; override 410 then list omits/disabled; prefix still
      works if cheap; clean shutdown.
- [ ] 9.4 `mks`; run the new test; Test 04 for the new md; fix until
      green. Prefer SQLite first; expand engines if repository SQL is
      engine-sensitive.

### Done means

`test_62` PASS on the engines this phase locks.

### Exit gate

Live `test_62` green; `mks` green; Test 04 green for new doc.

### Status

not started

---

## Phase 10 — Operator docs and indexes

### Goal

Someone can enable Subscribers without reading the plan.

### Work items

- [ ] 10.1 Create `docs/H/PUSH_GUIDE.md` — config, VAPID, subscribe
      flow, dispatch, 410, Lua, testing with pushval, SSRF allowlist,
      what we do **not** speak (FCM/APNs SDKs). **First time this file
      may be linked.**
- [ ] 10.2 API pages under `docs/H/api/subscribers/`.
- [ ] 10.3 Link from [README.md](/docs/H/README.md),
      [SITEMAP.md](/docs/H/SITEMAP.md),
      [STRUCTURE.md](/docs/H/STRUCTURE.md) (every new source file),
      [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) (U. Chat + V.
      Subscribers; launch 22),
      [API_OVERVIEW.md](/docs/H/core/API_OVERVIEW.md),
      [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md),
      [TESTING.md](/docs/H/tests/TESTING.md),
      [SECRETS.md](/docs/H/SECRETS.md) (`VAPID_PUBLIC_KEY` /
      `VAPID_PRIVATE_KEY` names only).
- [ ] 10.4 Test 04 / `mkl` green. No relative links, no `:line` refs.

### Done means

Guide + API docs + indexes match shipped behavior.

### Exit gate

`zsh -ic 'mkl'` (Test 04) green; markdownlint (Test 90) if docs-only
session.

### Status

not started

---

## Phase 11 — Lithium subscribe client (deferred)

### Goal

Lithium calls `pushManager.subscribe` and POSTs to `/api/subscribers`.

### Status

**Permanently deferred from this Hydrogen plan** unless the user pulls
it in. Placeholder Notification Manager stays. Service worker display
path already exists.

---

## Phase 12 — Security hardening

### Goal

Close remaining abuse paths that v1 locks did not already implement.
SSRF allowlist is **already a Phase 0/4/6 lock** — this phase is the
rest, not a place to postpone SSRF.

### Work items

- [ ] 12.1 Confirm SSRF allowlist + HTTPS-only still enforced at
      register **and** HTTP POST (regression Unity).
- [ ] 12.2 Rate limit dispatch REST (per-user / global), Mail Relay 14.3
      analog. Config knobs, not hardcoded magic only.
- [ ] 12.3 Reject CR/LF in title/body/url (`PUSH_INVALID_SUBSCRIPTION`
      or a dedicated code). Cap URL length.
- [ ] 12.4 Grep fence: no private key / `auth` / JWT / body in
      `log_this` format strings on the subscribers path.
- [ ] 12.5 Minimum TLS on outbound libcurl (Mail Relay 14.4 analog).
- [ ] 12.6 `mkq`, `mku`, `mkp`, `test_62` regression if behavior changed.

### Done means

Abuse tests exist; secrets stay out of logs; outbound TLS is not
optional in production.

### Exit gate

Named `mku`; `mkp`; `test_62` if touched.

### Status

not started

---

## Phase 13 — Coverage fence close

### Goal

Every new `src/subscribers/` and `src/api/subscribers/` file meets the
[Coverage fences](#coverage-fences). No file > 1000 lines.

### Work items

- [ ] 13.1 Run Test 89 / coverage helper on the new trees.
- [ ] 13.2 `extras/add_coverage.sh` or split files until fences pass.
- [ ] 13.3 Confirm Test 99 does not flag new files over 1000 lines.
- [ ] 13.4 Confirm `mkt` dead-code list has no new subscribers symbols.

### Done means

Per-file Unity fences green; combined target not regressed for these
files; dead-code clean.

### Exit gate

Test 89 slice + Test 99 + `mkt` dead-code.

### Status

not started

---

## Phase 14 — Optional later

Not scheduled. Record here so they do not sneak into Phase 7.

- Persist dispatch queue / HA claim (only if multi-instance push
  becomes real).
- System events → push (startup, printer fault) via Lua rules, not C.
- Native APNs/FCM for mobile apps.
- Oxygen element productization.
- `Topic` header / collapsing.
- Idempotency keys.

---

## Phase 15 — Release gate

### Goal

The subsystem is a normal Hydrogen citizen: build, lint, launch, docs,
Swagger, coverage, dead code.

### Work items

- [ ] 15.1 Re-check [Completeness fences](#completeness-fences) table;
      every row true or explicitly deferred with rationale.
- [ ] 15.2 `zsh -ic 'mkt'` then `mkp`; `mka` once trial is clean.
- [ ] 15.3 `test_17` min + max; `test_16` shutdown; `test_22` Swagger;
      `test_20` if prefix configs include the new routes; `test_62`.
- [ ] 15.4 Test 04, 90, 91, 92, 93, 98, 99 as touched.
- [ ] 15.5 Update TODO item 26 Done %; Working Log verdict.

### Done means

Fences green; plan Status blocks honest; Lithium still deferred.

### Exit gate

Named verification commands actually ran. Then this plan may move to
`docs/H/plans/complete/NOTIFICATIONS_PLAN_COMPLETE.md` when the user
asks.

### Status

not started

---

## Unity coverage inventory (expected)

Names are **targets** — search before creating; adjust if a name exists.
`mku` base = filename without `.c`.

| Area | Example bases |
| --- | --- |
| Config | `config_subscribers_test_load_subscribers_config` |
| Launch | `launch_subscribers_test_check_subscribers_launch_readiness` |
| Landing | `landing_subscribers_test_land`, `landing_subscribers_test_readiness` |
| VAPID | `subscribers_vapid_test_sign`, `subscribers_vapid_test_application_server_key` |
| RFC 8291 | `subscribers_crypto_test_encrypt`, `subscribers_crypto_test_decrypt`, `subscribers_crypto_test_too_large` |
| HTTP | `subscribers_http_test_post`, `subscribers_http_test_ssrf` |
| Queue | `subscribers_queue_test_enqueue`, `subscribers_workers_test_gone` |
| Producer | `subscribers_producer_test_dispatch` |
| Repository | `subscribers_repository_test_insert` (mocked) |
| API | `vapid_test_handle`, `register_test_handle`, `dispatch_test_forbidden` |
| Auth | `subscribers_api_auth_test_push_send` |
| Lua | `scripting_api_test_subscribers` + existing `scripting_api_test_mail` regression |

---

## Testing notes

| Layer | What |
| --- | --- |
| Unity | Config, VAPID, RFC 8291, HTTP seam, SSRF, repository, handlers, queue/workers, Lua. No `static` in `src/`. |
| Blackbox | Test 62 + `pushval`. Never hit `fcm.googleapis.com` / `web.push.apple.com` / `updates.push.mozilla.org`. |
| Coverage | See [Coverage fences](#coverage-fences). |
| Build | `mkq` ordinary C; `mkt` after add/remove `src/`; `mkp` after C; `mks` after Bash. |

Port scheme: Test 62 → **562x** ([TESTING.md](/docs/H/tests/TESTING.md)).
Fallback **1562x** if needed (Test 47 lesson).

---

## Threat notes

- **SSRF:** JWT holders can POST an `endpoint`. Allowlist + HTTPS-only
  is a v1 lock, not a polish item.
- **Secret leakage:** `auth`, VAPID private, JWT, and notification bodies
  must not appear in logs, dumps, or test artifacts.
- **410 handling:** failing to disable gone subscriptions wastes workers
  and may look like an outage.
- **Queue growth:** bounded queue + 503 `PUSH_QUEUE_FULL`. No unbounded
  enqueue.
- **Shutdown hang:** workers must honour `subscribers_system_shutdown`
  (`test_16` / `test_17`).
- **Confused deputy:** do not forward inbound JWTs to vendor POST.
- **Public vapid key:** not a secret; still do not log the private half.

### Risks

| Risk | Mitigation |
| --- | --- |
| Letter collision | V confirmed free as of 2026-09-08; re-check Phase 1 |
| OpenSSL API maze | Stay on EVP 3.x like `utils_crypto.c`; Unity vectors |
| libcurl in workers | `NOSIGNAL`, per-request easy handle, timeouts |
| Helium ID drift | Re-check disk at packet time |
| Test 62 port clash | 562x first, 1562x variance |
| Coverage discovered late | Per-phase fence, not only Phase 13 |
| Scope creep into Lithium | Phase 11 permanently deferred |

---

## Working Log

### 2026-09-08 — Plan drafted, then rewritten exhaustive

- Reviewed [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md),
  [TESTING.md](/docs/H/tests/TESTING.md),
  [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md),
  [MAILRELAY_PLAN_COMPLETE.md](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md),
  [MCP_COMPLETE.md](/docs/H/plans/complete/MCP_COMPLETE.md).
- Existing Notify is SMTP scaffold; `H.notify` is a locked mail shim.
- Lithium SW already shows pushes; no subscribe client.
- Crypto gap: need P-256 ECDSA, ECDH, HKDF, AES-128-GCM.
- HTTP: reuse OIDC RP libcurl helpers with an injectable seam.
- Analog: Mail Relay (queue/workers/REST/Lua/mailval) — copy structure,
  not types. MCP for letter/launch/status/Swagger/Test 17 skip.
- **Letter U is Chat.** Subscribers is **V**. Launch **22**.
  `MAX_SUBSYSTEMS` already 24 — no bump.
- Next Helium snapshot: `acuranzo_1377` / QueryRef #154. Reuse #127 for
  `push_send`.
- Completeness fences + coverage fences added as first-class Exit
  criteria.
- SSRF allowlist locked for v1 (register + POST).
- Phase 0 awaiting user approval of design locks.
