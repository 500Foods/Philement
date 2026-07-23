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

---

## P2 — Active product subsystems (larger, clear value)

### 4. Mail Relay — finish remaining phases

| | |
|---|---|
| **Plan** | [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) |
| **Effort** | L–XL |
| **Done** | ~70% — Phases 0–5, 7–8 done; Phase 6/10 partial; pause after 7B/8 |
| **Remaining** | System template seeds, Phase 9 Lithium UI, Phase 10 ops remainder, Phases 11–15 (inbound/rewrite/security/docs as scoped), auth MFA wiring via OTP |
| **Why next** | Core send/API/Lua/OTP stack works (`test_57`/`test_58`). Remaining is product surface and ops polish. |

### 5. Chat — Phase 13 advanced features

| | |
|---|---|
| **Plan** | [`CHAT_PLAN_PHASE_13.md`](/docs/H/plans/CHAT_PLAN_PHASE_13.md) · index [`CHAT_PLAN_SUMMARY.md`](/docs/H/plans/CHAT_PLAN_SUMMARY.md) |
| **Effort** | XL |
| **Done** | Phases 1–12 complete; Phase 13 streaming-architecture note done, feature list mostly open |
| **Remaining** | Function calling, response cache, key load-balance, fallback engines, analytics, templates, convo APIs, cost tracking, A/B, tests |
| **Why later** | Large wishlist on top of a working chat proxy. Pick individual bullets only when a product need appears. |

---

## P3 — Greenfield / deferred (keep visible, do not start casually)

### 6. Hydrogen as OIDC Identity Provider

| | |
|---|---|
| **Plan** | [`OIDC_IDP.md`](/docs/H/plans/OIDC_IDP.md) |
| **Effort** | XL |
| **Done** | 0% — scaffold/stubs only; plan authored |
| **Remaining** | Phases 0–17 (crypto, JWKS, clients, codes, tokens, UserInfo, refresh, Test 45, docs) |
| **Note** | Separate from completed OIDC **RP** (Keycloak). Start only if first-party IdP is a real goal. |

### 7. Mirage distributed proxy

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

Auth suite, Conduit (+ fix/diagrams), Database subsystem, Terminal, Migrations, Chat Phases 1–12, Lua scripting, Cap query, Mail Relay blackbox, Static-function purge, Log fanout, VictoriaLogs, Test 40 debug archaeology, and the old code-level `plans/TODO.md`.

---

## Status snapshot

| # | Plan | Effort left | Done | Priority |
|---|------|-------------|------|----------|
| 1 | Keycloak / OIDC RP E2E | S–M | ~90% | P0 |
| 2 | Database params closeout | S | ~75% | P0 |
| 3 | Unity ASAN | M | 0% | P1 |
| 4 | Mail Relay remainder | L–XL | ~70% | P2 |
| 5 | Chat Phase 13 | XL | ~15% of P13 | P2 |
| 6 | OIDC IdP | XL | 0% | P3 |
| 7 | Mirage | XL | 0% | P3 |
