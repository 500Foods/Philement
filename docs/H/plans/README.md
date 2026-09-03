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

### [AUTH FINALE](/docs/H/plans/AUTH_FINALE.md)

Active auth / OIDC / Keycloak plan (P0). Register email, DefaultRoles, client
roles, RP health/backchannel, terminal WS auth, login MFA, IdP durability and
post-MVP, real-Keycloak E2E (Phase 11). History:
[OIDC-PLAN_COMPLETE.md](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md),
[KEYCLOAK_PLAN_COMPLETE.md](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md),
[OIDC_IDP_COMPLETE.md](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md),
[OIDC_E2E_LOG_COMPLETE.md](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md),
[AUTH_PLAN_COMPLETE.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md).

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

### [MIRAGE PLAN](/docs/H/plans/MIRAGE_PLAN.md)

Distributed proxy architecture sketch. Implementation deferred.

---

## Completed plans

Full index: [`complete/README.md`](/docs/H/plans/complete/README.md). Highlights:

| Plan | File |
| ------ | ------ |
| Auth endpoints | [AUTH_PLAN_COMPLETE.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) |
| OIDC RP (historical) | [OIDC-PLAN_COMPLETE.md](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md) |
| Keycloak SSO ops | [KEYCLOAK_PLAN_COMPLETE.md](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md) |
| OIDC IdP MVP | [OIDC_IDP_COMPLETE.md](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md) |
| OIDC real-IdP E2E log | [OIDC_E2E_LOG_COMPLETE.md](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md) |
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
| mDNS upgrade | [MDNS_UPGRADE_COMPLETE.md](/docs/H/plans/complete/MDNS_UPGRADE_COMPLETE.md) |
| Migrations perf | [MIGRATIONS_COMPLETE.md](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md) |
| SchemaTool | [SCHEMATOOL_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md) |
| Static-function purge | [STATIC_COMPLETE.md](/docs/H/plans/complete/STATIC_COMPLETE.md) |
| Terminal | [TERMINAL_PLAN_COMPLETE.md](/docs/H/plans/complete/TERMINAL_PLAN_COMPLETE.md) |
| Unity disabled-test cleanup | [UNITY_CLEANUP_COMPLETE.md](/docs/H/plans/complete/UNITY_CLEANUP_COMPLETE.md) |
| VictoriaLogs | [VICTORIALOGGING_COMPLETE.md](/docs/H/plans/complete/VICTORIALOGGING_COMPLETE.md) |
