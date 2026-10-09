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

### [MariaDB / MySQL Split](/docs/H/plans/MARIADB_SPLIT_PLAN.md)

Support for MariaDB/MySQL was originally developed as one thing - no distinction was made. This was probably true a long time ago
but isn't true today, so despite the optimism, these have to be cleanly split into entirely separate database paths in the code.

### [AUTH FINALE](/docs/H/plans/AUTH_FINALE.md)

Active auth / OIDC / Keycloak plan (P0). Register email, DefaultRoles, client
roles, RP health/backchannel, login MFA, IdP durability and
post-MVP, real-Keycloak E2E (Phase 11). Terminal WS E2E is
[TERMINAL_FIX_PLAN_COMPLETE.md](/docs/H/plans/complete/TERMINAL_FIX_PLAN_COMPLETE.md).
History:
[OIDC-PLAN_COMPLETE.md](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md),
[KEYCLOAK_PLAN_COMPLETE.md](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md),
[OIDC_IDP_COMPLETE.md](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md),
[OIDC_E2E_LOG_COMPLETE.md](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md),
[AUTH_PLAN_COMPLETE.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md).

### [PERSIST PLAN](/docs/H/plans/complete/PERSIST_PLAN_COMPLETE.md)

**Complete (2026-09-04).** TODO 12d closed: live `test_58` mysql+mariadb
plaintext+STARTTLS green with Persist on (full 7-engine matrix 14/14 in 27.8s).
Two parts landed: `mysql_process_prepared_result` honours `mysql_stmt_store_result` rc (no `mysql_stmt_fetch` on NULL `fetch_row_func`); `repo_add_datetime` translates ISO 8601 → MySQL DATETIME for `DB_ENGINE_MYSQL` only. Archive only.

Archive only.

### [UNITY ASAN PLAN](/docs/H/plans/UNITY_ASAN_PLAN.md)

### [NOTIFICATIONS / SUBSCRIBERS PLAN](/docs/H/plans/NOTIFICATIONS_PLAN.md)

Web Push backend (RFC 8030/8291/8292): Subscribers API + dispatch queue.
Letter **W** (V is NATS, U is Chat), launch 23, Test 63. Exhaustive plan
(Phases 0–15) with completeness and coverage fences. Phase 0 not approved.
Lithium UI deferred. Does not reuse `Notify` SMTP scaffold or `H.notify`.

### [FIREBIRD PLAN](/docs/H/plans/FIREBIRD.md)

Replace the CockroachDB operator slot with a real Firebird engine
(`libfbclient`, Fedora 43 package 4.0.7) and Helium dialect
`database_firebird.lua`. Helium still emits SQL; Hydrogen does not
interpret it. Lookup 030 key 6 relabelled Firebase → Firebird.
Firebase C/Helium/extras teardown is Phases 3–4. Test 37 stays 37.
Phase 0 not approved.

Firestore attempt (historical):
[`FIREBASE_SUPERSEDED.md`](/docs/H/plans/complete/FIREBASE_SUPERSEDED.md).
Stub: [`FIREBASE.md`](/docs/H/plans/FIREBASE.md).

### [MIRAGE PLAN](/docs/H/plans/MIRAGE_PLAN.md)

Distributed proxy architecture sketch. Implementation deferred.

### [ARGENT PLAN](/docs/H/plans/ARGENT_PLAN.md)

Double-entry bookkeeping as an optional pack on the Acuranzo database
(`elements/002-helium/argent/`, migrations `argent_2xxx`, first file
`argent_2000`). Never applied alone. A future Gaius pack is `3xxx`.
Plus-list loader is in source. Tests 32–40 use
`PAYLOAD:acuranzo+argent`. Restructured 2026-10-06 into Phases 0–16.
Phase 0 approved the same day. Phases 1–3 applied 2026-10-07.
Phase 9 closed on the 2029 apply. Phase 10 closed on Test 71 3.2.0 (1490 passed, 0 failed). Phase 11 closed 2026-10-08 on the forward load plus Test 73 `test_73_20261008_085126` (178/178 on all eight engines). The reverse half of tests 32–39 was not run. Phase 12 closed 2026-10-08 on Test 73 1.0.4 `test_73_20261008_145836` (276/276 on all eight engines). Phase 13 closed 2026-10-08. Andrew reported that all Unity tests pass and `mkp` passes. He did not quote a count. Phase 14 closed 2026-10-08 on Test 73 1.0.5 `test_73_20261008_173729` (300/300 on all eight engines, harness 22/22, 314.196s). Phase 15 closed 2026-10-08 on Test 73 1.0.7 diagnostics `test_73_20261008_221958` (321/321 on all eight engines, harness 22 pass, 0 fail, 328.966s). Every engine log shows argent AVAIL = LOAD = APPLY = 2069. Work item 15.4 is checked. The full Test 31 harness was not run. Tests 32–39 were not in this report. Phase 16 opened 2026-10-08 and is waiting on Andrew. No new migration. The agent does not apply.
`H.http.request` allowlist is `GET`, `POST`, `PUT`, `DELETE`, `PROPFIND`, `REPORT`, `MKCALENDAR`, and `PROPPATCH`. `H.http.get` and `H.http.post` call that path.

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
| Chat Finale (beachhead 0–9 + Phase 10) | [CHAT_FINALE_COMPLETE.md](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) · [beachhead](/docs/H/plans/complete/CHAT_FINALE_BEACHHEAD_COMPLETE.md) |
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
| NATS client | [NATS_PLAN_COMPLETE.md](/docs/H/plans/complete/NATS_PLAN_COMPLETE.md) |
| mDNS upgrade | [MDNS_UPGRADE_COMPLETE.md](/docs/H/plans/complete/MDNS_UPGRADE_COMPLETE.md) |
| Migrations perf | [MIGRATIONS_COMPLETE.md](/docs/H/plans/complete/MIGRATIONS_COMPLETE.md) |
| MS SQL Server | [MSSQL_COMPLETE.md](/docs/H/plans/complete/MSSQL_COMPLETE.md) |
| SchemaHelper v1 | [SCHEMAHELPER_COMPLETE.md](/docs/H/plans/complete/SCHEMAHELPER_COMPLETE.md) |
| SchemaHelper v2 | [SCHEMAHELPER_V2_COMPLETE.md](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md) |
| SchemaTool | [SCHEMATOOL_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMATOOL_PLAN_COMPLETE.md) |
| Schema v2 | [SCHEMA_V2_PLAN_COMPLETE.md](/docs/H/plans/complete/SCHEMA_V2_PLAN_COMPLETE.md) |
| Static-function purge | [STATIC_COMPLETE.md](/docs/H/plans/complete/STATIC_COMPLETE.md) |
| Terminal | [TERMINAL_PLAN_COMPLETE.md](/docs/H/plans/complete/TERMINAL_PLAN_COMPLETE.md) |
| Unity disabled-test cleanup | [UNITY_CLEANUP_COMPLETE.md](/docs/H/plans/complete/UNITY_CLEANUP_COMPLETE.md) |
| VictoriaLogs | [VICTORIALOGGING_COMPLETE.md](/docs/H/plans/complete/VICTORIALOGGING_COMPLETE.md) |
