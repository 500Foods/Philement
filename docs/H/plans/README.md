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

Hydrogen as OIDC **Identity Provider** (separate from RP). Phases 0–15 complete
(crypto through security hardening + Test 45). Docs/operator:
[oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md),
[OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md). Remaining: Phase 16
closeout items if any, Phase 17 optional post-MVP.

### [MAIL RELAY PLAN](/docs/H/plans/MAILRELAY_PLAN.md)

SMTP mail relay subsystem. Core outbound/API/Lua/OTP delivered; later phases (UI, inbound, ops) remain.

### [CHAT FINALE](/docs/H/plans/CHAT_FINALE.md)

Active chat plan (P0). Dual REST+WS, provider knobs, MCP `System.Info`, Grok user-JWT MCP, dead-code gate. History: [CHAT_PLAN_SUMMARY_COMPLETE.md](/docs/H/plans/complete/CHAT_PLAN_SUMMARY_COMPLETE.md).

### [UNITY ASAN PLAN](/docs/H/plans/UNITY_ASAN_PLAN.md)

Separate ASAN Unity build/test variant for memory-safety gating without corrupting gcov coverage.

### [SCHEMAHELPER PLAN](/docs/H/plans/SCHEMAHELPER.md)

Interactive Lua TUI front-end to SchemaTool. Phases 0–4 shipped (review,
skip, accept, packets). Phase 5 first slice: one-field apply. Operator guide:
[`/docs/H/tools/SCHEMAHELPER.md`](/docs/H/tools/SCHEMAHELPER.md).

### [MDNS UPGRADE](/docs/H/plans/MDNS_UPGRADE.md)

RFC 6762/6763 server (codec, probe/claim, selective answers, NSEC) plus a
real browse/resolve client. Test 25 log-contract. Phases 0–8 not started.

### [MIRAGE PLAN](/docs/H/plans/MIRAGE_PLAN.md)

Distributed proxy architecture sketch. Implementation deferred.

---

## Completed plans

Full index: [`complete/README.md`](/docs/H/plans/complete/README.md). Highlights:

| Plan | File |
| ------ | ------ |
| Auth endpoints | [AUTH_PLAN_COMPLETE.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) |
| Cap / cap_query | [CAP_PLAN_QUERY-COMPLETE.md](/docs/H/plans/complete/CAP_PLAN_QUERY-COMPLETE.md) |
| Chat Phases 1–12 | [CHAT_PLAN_PHASE_*_COMPLETE.md](/docs/H/plans/complete/) · [summary](/docs/H/plans/complete/CHAT_PLAN_SUMMARY_COMPLETE.md) |
| Conduit | [CONDUIT_COMPLETE.md](/docs/H/plans/complete/CONDUIT_COMPLETE.md) |
| Image / Reporting | [IMAGE_PLAN_COMPLETE.md](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md) |
| Database subsystem | [DATABASE_PLAN_COMPLETE.md](/docs/H/plans/complete/DATABASE_PLAN_COMPLETE.md) |
| Database parameters | [DATABASE_UPDATE_PLAN_COMPLETE.md](/docs/H/plans/complete/DATABASE_UPDATE_PLAN_COMPLETE.md) |
| Forties (tests 40–47) | [FORTIES_COMPLETE.md](/docs/H/plans/complete/FORTIES_COMPLETE.md) |
| Log fanout | [LOG_FANOUT_PLAN_COMPLETE.md](/docs/H/plans/complete/LOG_FANOUT_PLAN_COMPLETE.md) |
| Lua scripting | [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) |
| Lua client script invoke | [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) |
| Lua 5.5 embed upgrade | [LUA_55_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_55_PLAN_COMPLETE.md) |
| Mail Relay blackbox | [MAILRELAY_BLACKBOX_PLAN_COMPLETE.md](/docs/H/plans/complete/MAILRELAY_BLACKBOX_PLAN_COMPLETE.md) |
| MCP server | [MCP_COMPLETE.md](/docs/H/plans/complete/MCP_COMPLETE.md) |
| Migrations perf | [MIGRATIONS_COMPLETE.md](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md) |
| SchemaTool | [SCHEMATOOL_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md) |
| Static-function purge | [STATIC_COMPLETE.md](/docs/H/plans/complete/STATIC_COMPLETE.md) |
| Terminal | [TERMINAL_PLAN_COMPLETE.md](/docs/H/plans/complete/TERMINAL_PLAN_COMPLETE.md) |
| Unity disabled-test cleanup | [UNITY_CLEANUP_COMPLETE.md](/docs/H/plans/complete/UNITY_CLEANUP_COMPLETE.md) |
| VictoriaLogs | [VICTORIALOGGING_COMPLETE.md](/docs/H/plans/complete/VICTORIALOGGING_COMPLETE.md) |
