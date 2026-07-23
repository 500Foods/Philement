# PLANS

Working and completed implementation plans for Hydrogen.

- **Active backlog:** [`/docs/H/TODO.md`](/docs/H/TODO.md) — prioritized incomplete work only
- **Completed plans:** [`/docs/H/plans/complete/`](/docs/H/plans/complete/) — finished plans (`*_COMPLETE.md`)
- **This folder:** plans still open, plus supporting logs

When a plan finishes: move it into `complete/`, add `_COMPLETE` to the filename if missing, update links, drop it from `TODO.md`, run `mkl`.

---

## Active plans

### [TODO (project backlog)](/docs/H/TODO.md)

Prioritized incomplete work with effort/done metrics. Start here.

### [KEYCLOAK PLAN](/docs/H/plans/KEYCLOAK_PLAN.md)

Production Keycloak SSO with Hydrogen as OIDC Relying Party. Phases 0–4 complete; Phase 5 real-IdP E2E in progress. Builds on historical RP work in [OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md). Manual checklist: [OIDC_E2E_LOG.md](/docs/H/plans/OIDC_E2E_LOG.md).

### [OIDC-PLAN (historical RP implementation)](/docs/H/plans/OIDC-PLAN.md)

Full phase log for Lithium + Hydrogen OIDC RP (Phases 1–25+ shipped). Day-to-day remaining work is tracked under KEYCLOAK_PLAN / OIDC_E2E_LOG, not by reopening this document from scratch.

### [OIDC IdP](/docs/H/plans/OIDC_IDP.md)

Hydrogen as OIDC **Identity Provider** (separate from RP). Plan only; not started.

### [MAIL RELAY PLAN](/docs/H/plans/MAILRELAY_PLAN.md)

SMTP mail relay subsystem. Core outbound/API/Lua/OTP delivered; later phases (UI, inbound, ops) remain.

### [DATABASE UPDATE PLAN](/docs/H/plans/DATABASE_UPDATE_PLAN.md)

Named/positional parameter and JSON binding enhancements. Implementation largely done; verification and docs closeout remain.

### [CHAT PLAN](/docs/H/plans/CHAT_PLAN_SUMMARY.md)

Chat proxy service. Phases 1–12 complete (see `complete/`). Open work: [Phase 13](/docs/H/plans/CHAT_PLAN_PHASE_13.md).

### [UNITY ASAN PLAN](/docs/H/plans/UNITY_ASAN_PLAN.md)

Separate ASAN Unity build/test variant for memory-safety gating without corrupting gcov coverage.

### [MIRAGE PLAN](/docs/H/plans/MIRAGE_PLAN.md)

Distributed proxy architecture sketch. Implementation deferred.

---

## Completed plans

Full index: [`complete/README.md`](/docs/H/plans/complete/README.md). Highlights:

| Plan | File |
|------|------|
| Auth endpoints | [AUTH_PLAN_COMPLETE.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) |
| Cap / cap_query | [CAP_PLAN_QUERY-COMPLETE.md](/docs/H/plans/complete/CAP_PLAN_QUERY-COMPLETE.md) |
| Chat Phases 1–12 | [CHAT_PLAN_PHASE_*_COMPLETE.md](/docs/H/plans/complete/) |
| Conduit | [CONDUIT_COMPLETE.md](/docs/H/plans/complete/CONDUIT_COMPLETE.md) |
| Database subsystem | [DATABASE_PLAN_COMPLETE.md](/docs/H/plans/complete/DATABASE_PLAN_COMPLETE.md) |
| Log fanout | [LOG_FANOUT_PLAN_COMPLETE.md](/docs/H/plans/complete/LOG_FANOUT_PLAN_COMPLETE.md) |
| Lua scripting | [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) |
| Mail Relay blackbox | [MAILRELAY_BLACKBOX_PLAN_COMPLETE.md](/docs/H/plans/complete/MAILRELAY_BLACKBOX_PLAN_COMPLETE.md) |
| Migrations perf | [MIGRATIONS_COMPLETE.md](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md) |
| Static-function purge | [STATIC_COMPLETE.md](/docs/H/plans/complete/STATIC_COMPLETE.md) |
| Terminal | [TERMINAL_PLAN_COMPLETE.md](/docs/H/plans/complete/TERMINAL_PLAN_COMPLETE.md) |
| VictoriaLogs | [VICTORIALOGGING_COMPLETE.md](/docs/H/plans/complete/VICTORIALOGGING_COMPLETE.md) |
