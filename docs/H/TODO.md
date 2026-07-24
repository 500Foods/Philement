# Hydrogen TODO

Actionable incomplete work only. Completed plans live in
[`/docs/H/plans/complete/`](/docs/H/plans/complete/). Active plan bodies live in
[`/docs/H/plans/`](/docs/H/plans/). Plan index:
[`/docs/H/plans/README.md`](/docs/H/plans/README.md).

## How to use this file

- Sorted by **immediate ROI vs effort** (quick wins first).
- Each item links its plan and carries rough metrics:
  - **Effort** — S / M / L / XL (session-scale estimate to finish remaining work)
  - **Done** — portion already shipped (rough %)
  - **Remaining** — what still blocks “done”
- When a plan finishes: move it to `plans/complete/` with a `_COMPLETE` suffix
  (if missing), update links, remove it from this file, run `mkl`.

---

## P0 — Close the loop (high ROI, low–medium effort)

### 1. Keycloak / OIDC RP real-IdP sign-off

| | |
|---|---|
| **Plan** | [`KEYCLOAK_PLAN.md`](/docs/H/plans/KEYCLOAK_PLAN.md) (Phase 5) · checklist [`OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md) · history [`OIDC-PLAN.md`](/docs/H/plans/OIDC-PLAN.md) |
| **Effort** | S–M (ops/manual; blocked on test-user MFA/OTP, not code) |
| **Done** | ~90% — Phases 0–4 complete; mock Test 42 88/88; production config wired |
| **Remaining** | Manual E2E against real Keycloak (password + OIDC login, provision, renew, logout, managers); optional Phase 26 last-method polish |
| **Why now** | Unblocks production SSO. Implementation is done; only verification remains. |

### 2. Database parameter support — verification & docs closeout

| | |
|---|---|
| **Plan** | [`DATABASE_UPDATE_PLAN.md`](/docs/H/plans/DATABASE_UPDATE_PLAN.md) |
| **Effort** | S |
| **Done** | ~75% — Phases 1–4 implementation complete |
| **Remaining** | Phase 5 test runs (coverage/cppcheck/shellcheck/leaks) and Phase 6 comment/docs tidy; mark complete |
| **Why now** | Code is in; finishing checkboxes shrinks the backlog with almost no design risk. |

---

## P1 — Quality / safety gates (medium effort, durable ROI)

### 3. Unity ASAN variant

| | |
|---|---|
| **Plan** | [`UNITY_ASAN_PLAN.md`](/docs/H/plans/UNITY_ASAN_PLAN.md) |
| **Effort** | M |
| **Done** | 0% — plan only |
| **Remaining** | `unity_asan` CMake tree, harness test, first-run triage (`detect_leaks=0`) |
| **Why now** | Catches UAF/double-free on unit-only paths blackbox never hits. Separate build; does not touch gcov. |

### 4. Auth register — persist email on `account_contacts`

| | |
|---|---|
| **Plan** | No dedicated plan yet (auth suite complete; this is a follow-on gap) |
| **Code** | `src/api/auth/auth_service_database.c` (`create_account_record`) · `src/api/auth/register/register.c` |
| **Effort** | S–M (needs Acuranzo QueryRef + call site) |
| **Done** | ~40% — account row via #051; password hash via #052 wired on register; email accepted by API but not stored |
| **Remaining** | New migration QueryRef to `INSERT` email into `account_contacts` (contact type email); call from `create_account_record` or register; Unity + blackbox register coverage |
| **Why now** | Register returns 201 with email in JSON but login-by-email / contact lookups stay empty unless fixtures seed contacts. Small product correctness hole. |
| **Note** | Do **not** reuse QueryRef #052 — that is password-hash storage only. |

### 5. WebSocket server heartbeat — wire scheduled PING

| | |
|---|---|
| **Plan** | No dedicated plan (WebSocket stack otherwise live) |
| **Code** | `src/websocket/websocket_server_heartbeat.c` · dispatch/timer in `websocket_server_dispatch.c` |
| **Effort** | S–M |
| **Done** | ~50% — helpers implemented (`ws_send_ping`, pong handler, health check); config `WebSocketServer.Heartbeat.*` exists |
| **Remaining** | Schedule server-side pings from writable/timer path; optional stale-connection close; blackbox in `test_23` with short `PingIntervalSeconds` |
| **Why now** | File had zero blackbox coverage because nothing calls `ws_send_ping`. Client pings alone never exercise this module. |
| **Note** | Distinct from libwebsockets client `--ping-interval` used in tests today. |

---

## P2 — Active product subsystems (larger, clear value)

### 6. Mail Relay — finish remaining phases

| | |
|---|---|
| **Plan** | [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) |
| **Effort** | L–XL |
| **Done** | ~70% — Phases 0–5, 7–8 done; Phase 6/10 partial; pause after 7B/8 |
| **Remaining** | System template seeds, Phase 9 Lithium UI, Phase 10 ops remainder, Phases 11–15 (inbound/rewrite/security/docs as scoped), auth MFA wiring via OTP |
| **Why next** | Core send/API/Lua/OTP stack works (`test_57`/`test_58`). Remaining is product surface and ops polish. |

### 7. Chat — Phase 13 advanced features (+ known gaps)

| | |
|---|---|
| **Plan** | [`CHAT_PLAN_PHASE_13.md`](/docs/H/plans/CHAT_PLAN_PHASE_13.md) · index [`CHAT_PLAN_SUMMARY.md`](/docs/H/plans/CHAT_PLAN_SUMMARY.md) |
| **Effort** | XL |
| **Done** | Phases 1–12 complete; WS streaming + media single-upload + non-stream `chat_done` blackbox live; Phase 13 feature list mostly open |
| **Remaining (Phase 13 wishlist)** | Function calling, response cache, key load-balance, fallback engines, analytics, templates, convo APIs, cost tracking, A/B, tests |
| **Remaining (concrete gaps from 2026-07 cleanup)** | See sub-bullets below — keep visible even if not started with the full P13 wishlist |
| **Why later** | Large wishlist on top of a working chat proxy. Prefer discrete bullets when product needs them. |

#### 7a. REST `/api/conduit/auth_chat` SSE streaming

| | |
|---|---|
| **Code** | `src/api/wschat/auth_stream/auth_stream.c` · `src/api/wschat/auth_chat/auth_chat.c` |
| **Effort** | L (MHD callback/SSE + reuse multi_curl proxy path) |
| **Done** | ~20% — non-stream REST works; endpoint returns intentional **501** / SSE error event; WS streaming fully works |
| **Remaining** | MHD incremental SSE response; drive `chat_proxy_*` multi-stream into SSE frames; flip `stream:true` off 501; update `test_59` (today asserts 501) and Unity stubs |
| **Note** | Interactive streaming is already on WebSocket. REST SSE is parity for HTTP-only clients. |

#### 7b. WebSocket chunked media upload

| | |
|---|---|
| **Code** | `src/websocket/websocket_server_media.c` (`handle_media_chunk_message`) |
| **Effort** | M |
| **Done** | ~70% — single-message `media_upload` path complete (hash, store #071, blackbox) |
| **Remaining** | Session buffers for `media_chunk` (upload_id / index / total); assemble → store; bounds/concurrency; cleanup on disconnect |
| **Note** | Stub returns -1 by design until multi-frame uploads are required. |

#### 7c. Dead / legacy chat stream callback cleanup

| | |
|---|---|
| **Code** | `src/websocket/websocket_server_chat_stream.c` · unused `send_chat_chunk` / `send_stream_*` helpers in `websocket_server_chat_send.c` |
| **Effort** | S |
| **Done** | Live path is multi_curl (`proxy_multi` / `proxy_mc`); legacy callback unwired |
| **Remaining** | Delete or `#if 0` legacy callback; drop dead send helpers **or** rewire if a product path needs them; adjust Unity if any |
| **Note** | Coverage noise only — not a runtime bug. |

---

## P3 — Greenfield / deferred (keep visible, do not start casually)

### 8. Hydrogen as OIDC Identity Provider

| | |
|---|---|
| **Plan** | [`OIDC_IDP.md`](/docs/H/plans/OIDC_IDP.md) |
| **Effort** | M remaining (was XL) |
| **Done** | ~90% — Phases 0–15 complete (protocol + Test 45 + hardening); Phase 16 docs |
| **Remaining** | Phase 17 optional (end-session, client credentials, consent UI); DB-backed codes/refresh for multi-process |
| **Docs** | [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md) · [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md) |
| **Note** | Separate from OIDC **RP** (Keycloak). MVP IdP is usable behind kill switch; HA needs durable stores. |

### 9. Print subsystem — job → device / Beryllium handoff

| | |
|---|---|
| **Plan** | None (print queue + Beryllium analyze exist as pieces) |
| **Code** | `src/print/print_queue_manager.c` · `src/print/beryllium.c` · upload path in webserver |
| **Effort** | L–XL (needs printer I/O product definition) |
| **Done** | ~30% — queue create/dequeue/thread; job JSON parse/log; Beryllium G-code analysis library present |
| **Remaining** | Define device interface; wire `process_print_job` → open file → optional Beryllium analyze → printer driver/status; blackbox when hardware or mock exists |
| **Note** | Launch marks print running; processing is log-only until a real sink exists. |

### 10. mDNS client runtime (browse / discover)

| | |
|---|---|
| **Plan** | None (server is complete under `src/mdns/`) |
| **Code** | `src/launch/launch_mdns_client.c` · config `mdns_client` · no `src/mdns_client/` worker |
| **Effort** | L–XL |
| **Done** | ~25% — config validation, launch/landing registry, readiness checks |
| **Remaining** | Browse PTR/SRV/TXT for configured service types; result cache/API; real init beyond “register RUNNING”; tests |
| **Note** | Launch is a scaffold that marks the subsystem running without discovery. mDNS **server** announcements are separate and working (`test_25`). |

### 11. Notify subsystem — outbound SMTP runtime

| | |
|---|---|
| **Plan** | None (or fold into Mail Relay if product chooses one path) |
| **Code** | `src/launch/launch_notify.c` · config `notify.smtp` · no `src/notify/` send path |
| **Effort** | M–L |
| **Done** | ~25% — config + launch validates SMTP notifier settings and registers subsystem |
| **Remaining** | Decide: implement notify SMTP client **or** officially route events through Mail Relay; then wire auth/system events; tests |
| **Note** | Mail Relay is the production mail stack today. Notify launch is config-only placeholder. |

### 12. Mirage distributed proxy

| | |
|---|---|
| **Plan** | [`MIRAGE_PLAN.md`](/docs/H/plans/MIRAGE_PLAN.md) |
| **Effort** | XL |
| **Done** | 0% — architecture sketch only |
| **Remaining** | Full design → phased implementation (not yet broken into gates) |
| **Note** | Deferred. Do not treat as near-term backlog. |

---

## Outside this tree (track elsewhere)

| Item | Where | Note |
|---|---|---|
| Lithium table refactor Phases 12–19 | [`/docs/Li/LITHIUM-TAB-PLAN.md`](/docs/Li/LITHIUM-TAB-PLAN.md) | Frontend; not a Hydrogen `plans/` item |
| Cap master coordination | external `CAP_PLAN.md` | Hydrogen Phase 2 query work is in [`complete/CAP_PLAN_QUERY-COMPLETE.md`](/docs/H/plans/complete/CAP_PLAN_QUERY-COMPLETE.md); deferred 2.8 cache work noted there |

---

## Recently completed (do not re-open)

Moved under [`plans/complete/`](/docs/H/plans/complete/) in this cleanup, including:

Auth suite, Conduit (+ fix/diagrams), Database subsystem, Terminal, Migrations, Chat Phases 1–12, Lua scripting, Cap query, Mail Relay blackbox, Static-function purge, Log fanout, FilterLogs, Test 40 debug archaeology, and the old code-level `plans/TODO.md`.

**2026-07 non-OIDC code cleanup** (not plan moves, but closed as code debt):

- Database façade TODOs wired (`database_execute`, shutdown/health/reload/remove/validate)
- Register password persist via QueryRef #052
- `api_create_jwt` real HS256 implementation
- Chat LRU write-through dirty-flag fix
- Stale conduit swagger 501 / config `DUMP_NOT_IMPLEMENTED` removed
- Intentional stubs documented in-code (REST SSE 501, media_chunk -1, print log-only, mDNS/Notify launch scaffold)

---

## Status snapshot

| # | Item | Effort left | Done | Priority |
|---|------|-------------|------|----------|
| 1 | Keycloak / OIDC RP E2E | S–M | ~90% | P0 |
| 2 | Database params closeout | S | ~75% | P0 |
| 3 | Unity ASAN | M | 0% | P1 |
| 4 | Register email → account_contacts | S–M | ~40% | P1 |
| 5 | WS server heartbeat wire-up | S–M | ~50% | P1 |
| 6 | Mail Relay remainder | L–XL | ~70% | P2 |
| 7 | Chat Phase 13 (+ 7a–7c gaps) | XL | ~15% of P13 | P2 |
| 7a | REST auth_chat SSE streaming | L | ~20% | P2 |
| 7b | WS chunked media upload | M | ~70% | P2 |
| 7c | Legacy chat_stream dead code | S | n/a | P2 |
| 8 | OIDC IdP (MVP + docs) | M | ~90% | P3 |
| 9 | Print job → device / Beryllium | L–XL | ~30% | P3 |
| 10 | mDNS client runtime | L–XL | ~25% | P3 |
| 11 | Notify SMTP runtime | M–L | ~25% | P3 |
| 12 | Mirage | XL | 0% | P3 |
