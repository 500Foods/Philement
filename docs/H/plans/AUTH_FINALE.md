<!-- markdownlint-disable MD007 MD024 -->
# Auth Finale

## Purpose

One gated plan for **remaining Hydrogen authentication work**: password
register gaps, OIDC Relying Party production sign-off, Hydrogen-as-IdP
post-MVP, terminal WebSocket auth, and login MFA via Mail Relay OTP.

Password login/renew/logout, OIDC RP Phases 1–26 + multi-provider
dispatch, and OIDC IdP Phases 0–16 are **shipped**. This document does
not rewrite them. It is the **only** active auth / OIDC / Keycloak plan.

History (do not start work from these):

- Password auth suite:
  [`AUTH_PLAN_COMPLETE.md`](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md)
- OIDC RP implementation:
  [`OIDC-PLAN_COMPLETE.md`](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md)
- Keycloak production ops:
  [`KEYCLOAK_PLAN_COMPLETE.md`](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md)
- Hydrogen as IdP:
  [`OIDC_IDP_COMPLETE.md`](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md)
- Real-Keycloak E2E pre-flight (unsigned happy path):
  [`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md)

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- **Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.** Exception: Phase 11 (real Keycloak E2E) may
  run in parallel as soon as a non-OTP test user exists — it is an ops
  gate, not a code dependency. Record that exception in Phase 0.
- Each phase has one **Done means** line — that is the testable state.
- Mark work items `[x]` only when that item's verification actually passed.
- Defer with `[~]` plus one-line rationale and the phase it moves to.
- After each phase: fill Status (date, result, variances), append Working
  Log, **stop for review**. Do not begin the next phase in the same turn
  unless asked.
- Build aliases: `zsh -ic 'mkq'` (ordinary C), `mkt` (clean/configure),
  `mku <base>`, `mkp`, `mka`, `mks`. See
  [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

## Implementor Workflow (every phase)

Each phase is worked in its **own conversation**. Follow this sequence:

1. **Confirm the prior phase is actually done.** Re-read its Status block
   and Exit gate before touching anything; do not trust memory of a prior
   session.
2. **Discuss the current phase first.** Re-read only that phase's Goal +
   Work items + Done means + Exit gate. Ask clarifying questions and do
   any research needed (grep the code, read the relevant `.md`, check
   Helium/QueryRef IDs on disk) **before** writing any code.
3. **Ask for explicit approval to start implementation.** Do not begin
   editing source files until the user says go.
4. **Ask questions as they come up** during implementation rather than
   guessing at ambiguous requirements.
5. **Update the phase's Working Log entry when major pieces land** (not
   only at the very end).
6. **Record lessons learned** for the phase, even small ones.
7. **Mark work items `[x]` and the phase Status "complete" only after the
   phase's actual verification commands ran clean** (`mkq`/`mkt`/`mkp`/`mks`
   as applicable, the named `mku` tests, the named blackbox test). Intent
   to verify is not verification.
8. **Never apply a database migration.** If a phase needs a Helium seed,
   schema change, or migration packet, prepare/generate it and hand it to
   the user to apply; do not run `schematool`/`schemahelper` apply steps
   yourself.
9. **Follow existing project norms** for unit tests (Unity, one file per
   function, no `static` in `src/`, see
   [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md)), blackbox tests
   (`tests/test_NN_*.sh`, see
   [TESTING.md](/docs/H/tests/TESTING.md)), and the existing aliases
   (`mkq`, `mkt`, `mkp`, `mks`, `mka`, `mku`) rather than inventing new
   scripts or ad hoc build commands.
10. **Never log client secrets, handoff codes, JWTs, id_tokens, OTP
    plaintext, or passwords** in normal logs or test artifacts.

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-09-02):** Plan authored. No AUTH_FINALE
phase has started. Next: **Phase 0 (contract lock, no C)**.

### Resume here next session

1. Confirm the latest completed phase via Status blocks.
2. Re-read only the next phase Goal + Done means + Exit gate.
3. `zsh -ic 'mkq'` (or `mkt` if `build/` is missing); relevant `mku`;
   Test 40 / 42 / 45 as named in that phase.
4. Implement that phase only → verify Exit gate → update this doc → stop.

### Session checklist (every auth return)

1. Confirm latest completed phase via Status blocks; active phase = first
   not complete (Phase 11 may be in parallel — see exception above).
2. Re-read Working Log decisions that affect the next chunk.
3. Baseline: `zsh -ic 'mkq'`; Unity under `tests/unity/src/api/auth/` and
   `tests/unity/src/oidc/` as named; blackbox Test 40 / 42 / 45 when the
   phase touches those flows.
4. One small chunk: questions → plan → implement → verify → update this
   plan → stop for review.

---

## Priority

| | |
| --- | --- |
| **Band** | P0 — [TODO.md item 1](/docs/H/TODO.md) |
| **Effort** | XL (many small gates; IdP post-MVP is the bulk) |
| **First deploy** | Production SSO already wired; remaining is register honesty, provision roles, real-IdP sign-off, then optional IdP HA |

TODO lists this as **one P0 item** (same pattern as
[`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md)). Inside the plan, phases
are ordered so **code that does not need a live Keycloak user ships
first**. Real-IdP E2E is Phase 11.

Urgency inside the plan (not phase order):

| Original TODO | Urgency | AUTH_FINALE phase |
| --- | --- | --- |
| Keycloak / OIDC RP E2E | P0 (ops; OTP-blocked) | 11 |
| Provision DefaultRoles | P0 | 2 |
| Register email → `account_contacts` | P1 | 1 |
| Terminal WS auth | P2 | 7 |
| OIDC RP client-role parse | P2 | 3 |
| Login MFA via Mail Relay OTP | P2 (was Mail Relay leftover) | 8 |
| OIDC IdP post-MVP + durability | P3 | 9–10 |

---

## Goals

1. Register stores the email the API already accepts, so login-by-email
   and OIDC email-link work for self-registered users.
2. When operators enable OIDC auto-provision with `DefaultRoles`, those
   role_ids are written to `account_roles` (not only logged).
3. `RoleMapping.Source = idp_client_roles` (and MERGE) actually reads
   `resource_access.<client>.roles`.
4. Real Keycloak E2E checklist is signed (password + SSO, provision
   policy, renew, logout, managers) and RP-initiated logout is verified
   live.
5. RP ops extras that were post-MVP: discovery health on system
   endpoints, backchannel logout.
6. Terminal WebSocket has a real auth decision and matching gate.
7. Login MFA / email verify can use the **existing** Mail Relay OTP
   primitives (`mailrelay_otp_generate_and_send` / `mailrelay_otp_verify`).
8. Hydrogen-as-IdP can run multi-process: auth codes and refresh tokens
   on QueryRefs `#136`–`#142`, not only in-memory stores; userinfo can
   merge accounts-DB profile; `oidc_users.c` dummy retired or documented
   as dead.
9. IdP optional post-MVP (end-session, client credentials, consent, DCR,
   key rotation, access denylist, native redirect schemes) is either
   implemented or explicitly parked with 501s unchanged.
10. Docs and operator runbooks match code; no second active auth plan.
11. Self-service password reset uses the **same** Mail Relay OTP
    primitives as login MFA, without email-enumeration leaks.
12. A logged-in user can see and revoke their own active Hydrogen
    sessions (not just "logout this browser"), reusing the JWT storage
    that already backs Phase 6/11 revocation.

## Non-Goals

- **Chat JWT mint / MCP `aud` gate.** Owned by
  [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) Phase 8. Do not
  duplicate a minting primitive here.
- **Rewriting shipped RP, IdP MVP, or password login.** Prefer focused
  fixes. Do not reopen
  [`OIDC-PLAN_COMPLETE.md`](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md)
  or [`AUTH_PLAN_COMPLETE.md`](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md)
  as working plans.
- **Mail Relay transport, templates, Lithium mail UI, inbound SMTP.**
  Stay on [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md). This
  plan only **consumes** OTP send/verify for login MFA.
- **SPA-held Keycloak tokens.** Hard rule remains: SPAs hold Hydrogen
  JWTs only.
- **Hydrogen session JWT as OIDC access token.** Separate token surfaces.
- **Implicit / hybrid / device / CIBA / PAR/JAR** unless Phase 10 pulls
  a single item with an explicit variance.
- **Lithium last-method polish.** OIDC-PLAN Phase 26 is done; do not
  reopen unless Phase 0 records a product gap.
- **Using Keycloak refresh tokens for sliding Hydrogen sessions.** Use
  `/api/auth/renew`.
- **CAPTCHA, GDPR erasure product, concurrent-session *product* limits**
  from the original AUTH_PLAN security wishlist. Those documents are
  complete history, not a backlog to resurrect. (Per-account per-hour
  reset-request throttling in Phase 8b is anti-abuse plumbing, not the
  concurrent-session product feature this excludes.)

---

## Security & Safety

Auth is already a production surface. This finale closes honesty gaps
and leftover protocol. Review risks here so later phases do not
"discover" them.

| Risk | Current reality | Handled by |
| --- | --- | --- |
| Register email dropped | `create_account_record` `(void)email`; QueryRef `#051` does not write `account_contacts`. Login-by-email and OIDC `#082` miss self-registered users | Phase 1 |
| Provision DefaultRoles is a log lie | Config parsed; no `account_roles` INSERT. DATABASE role source → empty JWT roles for new OIDC users | Phase 2 |
| `idp_client_roles` falls back to realm | `oidc_rp_idtoken` only fills `realm_access.roles`; `resource_access` unused | Phase 3 |
| Real SSO unsigned | Mock Test 42 101/101; live checklist blocked on Keycloak MFA/OTP for `andrew@500foods.com` | Phase 11 |
| RP-initiated logout unverified live | Code shipped; Keycloak post-logout URI + fresh `id_token` claim still operator checklist | Phase 11 |
| No RP discovery health | Ops cannot see IdP down vs RP disabled | Phase 5 |
| No backchannel logout | Keycloak SSO logout does not revoke Hydrogen JWTs | Phase 6 |
| Terminal WS open | `terminal_websocket_requires_auth` always returns false | Phase 7 (product lock in Phase 0) |
| Login MFA unwired | Mail Relay OTP generate/verify exist; `/api/auth/login` is password-only | Phase 8 |
| No self-service password reset | Only fix today is an operator/DB edit; users with a forgotten password have no recovery path | Phase 8b |
| No self-service session visibility/revoke | `logout` only ends the caller's own JWT; a user who suspects compromise cannot see or kill other active sessions | Phase 10b |
| IdP codes/refresh in-memory | QueryRefs `#136`–`#142` exist; runtime store is still in-memory. Multi-process / HA will drop codes | Phase 9 |
| IdP access revoke is TTL-only | `/oauth/revoke` on access is no-op success; no denylist | Phase 10 (optional) or documented |
| IdP `oidc_users.c` dummy | Hard-coded `test_user` path unused by authorize (accounts DB). Dead-code risk | Phase 9 |
| Empty `AllowedEmailDomains` + provision on | Must not mean "all domains" unless Phase 0 says so | Phase 0 lock |
| Secrets in logs | Existing RP/IdP/auth rules; keep them | Every phase |

---

## Architecture (locked — do not confuse the stacks)

| Stack | Role | Location | This plan |
| --- | --- | --- | --- |
| Password / session JWT | HS256 login, renew, logout, register | `src/api/auth/` | Phases 1, 8 |
| OIDC **Relying Party** | Hydrogen is client of Keycloak | `src/api/auth/oidc_rp/`, `config_oidc_rp` | Phases 2–6, 11 |
| OIDC **Identity Provider** | Hydrogen issues tokens | `src/oidc/`, `src/api/oidc/`, `config_oidc` | Phases 9–10 |
| Terminal WS | Browser terminal | `src/terminal/terminal_websocket.c` | Phase 7 |
| Mail Relay OTP | Generate / verify codes | `src/mailrelay/mailrelay_otp.c` | Phase 8 consumes; does not own |

### RP flow (shipped)

```text
Browser   →  GET  /api/auth/oidc/start?database=&return_to=&provider=
Hydrogen  →  302  Keycloak /auth  (state, nonce, PKCE S256)
Keycloak  →  302  /api/auth/oidc/callback?code=&state=
Hydrogen  →  POST Keycloak /token; validate id_token; link/provision; mint JWT
Hydrogen  →  302  SPA  ?oidc=1&handoff=<opaque>
SPA       →  POST /api/auth/oidc/handoff  →  { success, token, … }
```

Hard rule: Keycloak tokens never leave Hydrogen. Only opaque handoff
codes reach the browser.

### IdP flow (MVP shipped; HA not)

```text
Client  →  GET/POST /oauth/authorize  (PKCE S256, embedded login form)
Hydrogen IdP →  302 redirect_uri?code=&state=
Client  →  POST /oauth/token
Client  →  GET  /oauth/userinfo  +  /.well-known/openid-configuration  +  /oauth/jwks
```

Kill switches: `OIDC_RP.Enabled=false` (RP 503 `oidc_disabled`);
`OIDC.Enabled=false` (IdP endpoints not registered). Password login is
independent of both.

---

## Current Observed State (2026-09-02)

### Shipped — do not rebuild

| Surface | Evidence |
| --- | --- |
| Password login / renew / logout | Test 40; QueryRefs `#008`–`#017`; HS256 `generate_jwt` / `store_jwt` |
| Register account + password hash | QueryRef `#051` + `#052`; 201 JSON includes email **that is not stored** |
| OIDC RP start / callback / handoff / end-session | `src/api/auth/oidc_rp/`; Test 42 **101/101** (2026-07-23) |
| Multi-provider dispatch | `oidc_rp_find_provider`; `?provider=`; state `provider_name` |
| Link strategies | `match_sub_only`, `match_email_only`, `match_email_then_provision`, `provision_only` |
| Role mapping DATABASE / realm / MERGE | MERGE uses realm; client roles fall back |
| Production Keycloak config | Issuer `https://www.500passwords.com/realms/festival`; client `lithium`; kill switch safe |
| Lithium SSO client | `oidc-client.js`; Vitest oidc-client 26/26; last-method Phase 26 done |
| IdP MVP | Discovery, JWKS, authorize+login, token, userinfo, refresh, introspect, revoke; Test 45 **7/7 engines** |
| Mail Relay OTP primitives | generate + send + verify; **not** wired to `/api/auth/login` |

### Open / incomplete (this plan)

| Item | Detail |
| --- | --- |
| Register email | `auth_service_database.c` `create_account_record` ignores email |
| DefaultRoles | Logged when non-empty; no `account_roles` rows |
| `resource_access` | Not parsed |
| Dual-provider Test 42 | Deferred; Unity covers named lookup |
| Real Keycloak E2E | Pre-flight passed; happy path unsigned (OTP blocker). Ticks are Phase 11 |
| RP logout live | Code done; Keycloak post-logout URI + fresh JWT unverified |
| RP health | Not on system endpoints |
| Backchannel logout | Not implemented |
| Terminal WS auth | Always false |
| Login MFA | OTP C seam exists; auth does not call it |
| IdP DB codes/refresh | Schema + QueryRefs exist; runtime in-memory |
| IdP userinfo profile | `{sub}` + token `user_data` only; no accounts-DB merge |
| IdP `oidc_users.c` | Dummy; authorize uses accounts DB |
| IdP end-session / DCR | 501 `not_implemented` |
| IdP config JSON Schema | Example JSON only; `hydrogen_config_schema.json` has no IdP block |

### Confirmed design decisions (carry forward)

1. Hydrogen is the RP; SPAs only hold Hydrogen JWTs.
2. Handoff codes, not JWTs in URLs.
3. Do not use `src/oidc/` for Keycloak.
4. Password login independent of IdP availability.
5. Stable RP link key is `(iss, sub)`; email is secondary.
6. PKCE S256 + state + nonce; confidential client preferred for RP.
7. RS256-only IdP tokens; reject `alg=none`.
8. JWT `roles` = comma-separated **role_id integers** (e.g. `"1,3"`).
9. Production RP policy: `match_email_only`, provision **off**,
   `RequireEmailVerified=true`, role source `database`.
10. Feature kill switches remain.
11. Chat MCP token minting is **not** this plan.

---

## Reference Conventions

Match existing auth / OIDC patterns; do not invent parallel stacks.

### Tests

| Suite | Owns |
| --- | --- |
| `tests/test_40_auth.sh` | Password login / register / renew / logout (7 engines) |
| `tests/test_42_oidc_rp.sh` | RP mock Keycloak (`tests/lib/mock_keycloak/`) |
| `tests/test_45_oidc_idp.sh` | IdP code+PKCE (ports 5450–5456) |
| `tests/test_21_system_endpoints.sh` | System health fields (Phase 5) |
| `tests/test_26_terminal.sh` | Terminal (Phase 7) |
| `tests/test_40_auth.sh` | Also owns password reset (Phase 8b) and session list/revoke (Phase 10b) cases |
| Unity `tests/unity/src/api/auth/` | Password + RP helpers |
| Unity `tests/unity/src/oidc/` + `api/oidc/` | IdP |

Port scheme remains `5<TT>x` per [TESTING.md](/docs/H/tests/TESTING.md).
Do not invent a second mock-IdP suite.

### Config / JWT / logging

- RP: `OIDCRelyingPartyConfig` / `OIDC_RP`; env
  `HYDROGEN_OIDC_CLIENT_ID`, `HYDROGEN_OIDC_CLIENT_SECRET`,
  `HYDROGEN_OIDC_ISSUER`, `HYDROGEN_OIDC_REDIRECT_URI`,
  `HYDROGEN_OIDC_RP_ENABLED`.
- IdP: `OIDCConfig` / `OIDC`; default `Enabled=false`.
- After successful link/provision, call the **same** `generate_jwt` +
  `store_jwt` as password login.
- `log_this` with `SR_AUTH` / `SR_OIDC`; never log secrets.

### Migrations

Re-check highest Acuranzo migration and QueryRef on disk before
assigning numbers. Hand packets to the user. Do **not** reuse QueryRef
`#052` for email (password hash only).

---

## Gate Philosophy

- One logical behavior per phase where practical.
- After every C change: `mkq` (or `mkt`), then `mkp`.
- After RP flow changes: relevant `mku` + `test_42_oidc_rp.sh`.
- After password/register changes: relevant `mku` + `test_40_auth.sh`.
- After IdP changes: relevant `mku` + `test_45_oidc_idp.sh`.
- After config schema changes: jsonlint + config Unity.
- After docs: `mkl` (Test 04) + markdownlint (Test 90).
- After bash: `mks`.
- Mark phase complete only when every listed gate is green or explicitly
  deferred.

---

## Phase Index

| Phase | Done means (one line) | Effort | Status |
| --- | --- | --- | --- |
| 0 | Decisions written in Status (policy, MFA, reset, sessions, failure modes, terminal, E2E parallel, empty-domain); no C | S | pending |
| 1 | Register persists email on `account_contacts`; Test 40 covers it | S | pending |
| 2 | Provision writes `DefaultRoles` into `account_roles`; Test 42 provision cases | S | pending |
| 3 | `resource_access.<client>.roles` parsed; IDP_CLIENT_ROLES / MERGE use it | M | pending |
| 4 | Optional second mock provider in Test 42 **or** `[~]` with rationale | S | pending |
| 5 | System health exposes `oidc_rp_status` without secrets | M | pending |
| 6 | Backchannel logout validates logout token and revokes Hydrogen JWTs | M | pending |
| 7 | Terminal WS auth matches Phase 0 decision; Test 26 | S | pending |
| 8 | Login MFA (and email verify if locked) uses Mail Relay OTP; Test 40 | M | pending |
| 8b | Password reset via Mail Relay OTP; no enumeration leak; Test 40 | M | pending |
| 9 | IdP codes/refresh on QueryRefs; userinfo can merge accounts; `oidc_users` resolved | L | pending |
| 10 | Each pulled IdP post-MVP item has its own exit; unpulled stay 501 | S–L (per item) | pending |
| 10b | User can list and revoke own active JWT sessions; Test 40 | M | pending |
| 11 | Seven SSO checks + RP logout live + second reviewer signed | — (ops, not code) | **pending — OTP-blocked, ops gate, may run in parallel with 1–10b** |
| 12 | Docs/SITEMAP/operator notes match code | S | pending |

Effort key: S = small/contained, M = moderate (new endpoint + Unity +
blackbox), L = large (schema-touching or multi-file). Phase 11 has no
code effort of its own; it is an operator/Keycloak-admin gate tracked
here for visibility, not an engineering estimate.

---

## Phase 0 — Contract Lock

### Goal

Write the decisions later phases implement. No C.

### Entry gate

This document exists. Ability to read historical plans in
`plans/complete/` and run Test 40 / 42 / 45 as a baseline (read-only).

### Work items

- [ ] Confirm two-stack split (RP vs IdP vs password) still holds.
      **Verify:** Status records confirmation; no silent merge of
      `src/oidc/` into Keycloak work.
- [ ] Re-lock production RP policy or record a change:
      `match_email_only`, `ProvisionDefaults.Enabled=false`,
      `RequireEmailVerified=true`, role source `database`.
      **Verify:** written in Status.
- [ ] Lock empty `AllowedEmailDomains` semantics when provision is
      later enabled: empty list must **not** mean all domains.
      **Verify:** Status + later Phase 2 must not contradict.
- [ ] Lock **terminal WS auth** product: JWT in first message vs session
      cookie vs stay-open on trusted nets. Default recommendation:
      require Hydrogen JWT (same shape as other WS), fail closed when
      Terminal is enabled on a public origin.
      **Verify:** Decision column in Status.
- [ ] Lock **login MFA** product: default off; enable via config;
      purpose `login_mfa` (lookup 066); email verify on register in or
      out of Phase 8.
      **Verify:** Status.
- [ ] Lock **password reset** as **in scope, Phase 8b**, using a
      dedicated OTP purpose distinct from `login_mfa` (do not reuse
      login_mfa's purpose id — a leaked login OTP must not double as a
      password-reset token). Lock: reset-request response is identical
      whether or not the email exists (no account-enumeration leak);
      successful reset invalidates all existing JWTs for the account
      (force re-login everywhere, consistent with the Phase 6/11
      "logout revokes JWT" model).
      **Verify:** Status.
- [ ] Lock **self-service session management** as **in scope, Phase
      10b**: list own active JWTs (created/last-used/device label if
      captured; no token material returned) and revoke one or all
      except current. Decide whether device/user-agent metadata needs a
      new column on the JWT storage table or can be read from what is
      already stored.
      **Verify:** Status; migration need (or not) recorded.
- [ ] Lock **Phase 1 failure discipline**: if the new `account_contacts`
      email insert fails after `#051`/`#052` succeed, decide fail-closed
      (roll back / delete the just-created account and return an error)
      vs fail-open (keep the account, log at ALERT, register still
      returns 201). Record the choice — Phase 1 must not improvise this
      mid-implementation.
      **Verify:** Status.
- [ ] Lock **Phase 2 failure discipline**: if a `DefaultRoleNames` entry
      fails to resolve to a role_id or the `account_roles` insert fails,
      decide fail-closed (provisioning fails, no account/identity link
      created) vs fail-open (account provisioned with whatever roles did
      insert, logged at ALERT). Record the choice.
      **Verify:** Status.
- [ ] Lock **Phase 3 role-merge scope**: when `RoleMapping.Source =
      MERGE`, decide the exact source set — `database ∪ client`,
      `database ∪ realm ∪ client`, or operator-configurable — so Phase 3
      does not have to invent product policy mid-implementation.
      **Verify:** Status.
- [ ] Confirm Phase 11 (real Keycloak) is allowed in parallel once a
      non-OTP user or TOTP secret exists.
      **Verify:** Status records the exception.
- [ ] Confirm chat MCP minting stays on CHAT_FINALE.
      **Verify:** Non-Goals still accurate.
- [ ] Baseline (no code): `zsh -ic 'mkq'`; note latest Test 40 / 42 / 45
      results if recently run, or record "not re-run, trust last green."
      **Verify:** Working Log.

### Done means

Decisions written in Status. No C.

### Exit gate

Status filled. Working Log has locked policy, MFA, terminal, E2E
parallel, empty-domain.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 1 — Register Email → `account_contacts`

### Goal

`POST /api/auth/register` persists the email it already accepts.

### Entry gate

Phase 0 complete.

### Work items

- [ ] New Acuranzo QueryRef (re-check next free id on disk) to `INSERT`
      email into `account_contacts` with `contact_type_a18 = 1` (E-Mail).
      Do **not** reuse `#052`.
      **Verify:** migration packet handed to user; luacheck green.
- [ ] Call it from `create_account_record` after `#051` succeeds (or
      from `register.c` with the new `account_id`). Failure must not
      leave a half-created account without a logged error; prefer
      transactional discipline consistent with existing register.
      **Verify:** code review; Unity for the helper.
- [ ] Test 40 register path asserts contact row / login-by-email after
      register (or equivalent QueryRef read).
      **Verify:** `test_40_auth.sh` green.
- [ ] `mkq` + `mkp`. Docs: one sentence in auth API doc if the contract
      changes from "email in JSON only."

### Done means

Self-registered users have an email contact; login-by-email and OIDC
`#082` can find them.

### Exit gate

Test 40 green. `mkq`/`mkp` green. Migration applied by human.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 2 — Provision DefaultRoles → `account_roles`

### Goal

OIDC auto-provision writes configured default role_ids into
`account_roles`.

### Entry gate

Phase 1 complete.

### Work items

- [ ] QueryRef (or reuse if one already inserts `account_roles` — none
      found in migrations except seed `acuranzo_1258.lua`) to insert
      `(account_id, role_id)` per DefaultRoles entry.
      **Verify:** packet handed to user; luacheck.
- [ ] Resolve `DefaultRoleNames[]` to role_id (QueryRef `#127` Get Role
      By Name already exists) and INSERT after provision in both
      `oidc_rp_link_provision.c` and `oidc_rp_link_default.c`.
      **Verify:** Unity; failure discipline matches existing
      non-fatal-vs-fatal pattern (lock in implementation discussion:
      empty roles vs fail provision).
- [ ] Test 42 provision cases with DefaultRoles set; JWT `roles` claim
      contains those ids when source is `database`.
      **Verify:** Test 42 green.
- [ ] `mkq` + `mkp`. Comments in linkers no longer say "not wired."

### Done means

Enabling provision + DefaultRoles yields non-empty DATABASE JWT roles
for new users.

### Exit gate

Test 42 green. `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 3 — Parse IdP Client Roles (`resource_access`)

### Goal

`IDP_CLIENT_ROLES` and MERGE can use Keycloak client-scoped roles.

### Entry gate

Phase 2 complete.

### Work items

- [ ] Parse `resource_access.<client_id>.roles` in
      `oidc_rp_idtoken.c` into claims (client id = RP `ClientId` unless
      config names a different resource).
      **Verify:** Unity on token fixtures.
- [ ] `oidc_rp_roles.c`: IDP_CLIENT_ROLES uses client roles (not realm
      fallback). MERGE unions DB + client (and/or realm — lock in
      discussion: realm, client, or both).
      **Verify:** Unity + Test 42 role-mapping cases.
- [ ] Mock Keycloak id_token includes `resource_access` for the new
      cases.
      **Verify:** Test 42 green.
- [ ] `mkq` + `mkp`. Docs in `oidc_rp.md`.

### Done means

Operators who set `idp_client_roles` get client roles in the Hydrogen
JWT, not a silent realm fallback.

### Exit gate

Test 42 green. `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 4 — Dual-Provider Mock Blackbox

### Goal

Test 42 proves named `?provider=` against two mock IdPs **or** the item
is `[~]` because Unity + state `provider_name` is enough.

### Entry gate

Phase 3 complete.

### Work items

- [ ] Decide: second mock provider in Test 42 vs defer.
      **Verify:** if deferred, `[~]` with rationale; phase can complete
      as documentation-only.
- [ ] If implemented: second mock issuer, `/start?provider=`, callback
      uses state provider, unknown provider still `400 unknown_provider`.
      **Verify:** Test 42 green.

### Done means

Either dual-provider blackbox exists, or Working Log records why Unity
is sufficient.

### Exit gate

Test 42 still green. Decision recorded.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 5 — OIDC RP Health Probe

### Goal

System health shows RP readiness without leaking secrets.

### Entry gate

Phase 4 complete.

### Work items

- [ ] Design field (e.g. `oidc_rp_status`: `disabled` | `ok` |
      `degraded` | `error`). No issuer secrets, no tokens.
      **Verify:** design note in Working Log.
- [ ] Implement timeout-bounded cached discovery fetch.
      **Verify:** Unity; extend `test_21_system_endpoints.sh`.
- [ ] Document in system endpoint docs.
      **Verify:** `mkl`.
- [ ] `mkq` + `mkp`.

### Done means

Health reflects Enabled=false cleanly and Enabled=true with IdP up/down.

### Exit gate

Test 21 green (or named extension). `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 6 — Backchannel Logout

### Goal

Keycloak backchannel logout revokes matching Hydrogen JWTs.

### Entry gate

Phase 5 complete.

### Work items

- [ ] Design path (historical: `/api/auth/oidc/backchannel-logout`).
      Validate logout token (signature, iss, aud, events). Reject
      unsigned / `alg=none`.
      **Verify:** Unity negative tests.
- [ ] Map to accounts/sessions; delete Hydrogen JWTs from storage.
      **Verify:** Unity + Test 42 mock logout token.
- [ ] Operator note: Keycloak client backchannel URL.
      **Verify:** `oidc_rp.md` + E2E log appendix.
- [ ] `mkq` + `mkp`. SPA may still call local logout.

### Done means

A valid mock (and later real) backchannel logout invalidates the
Hydrogen session for the linked user.

### Exit gate

Test 42 green. `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 7 — Terminal WebSocket Authentication

### Goal

Terminal WS matches the Phase 0 product decision.

### Entry gate

Phase 6 complete.

### Work items

- [ ] Implement `terminal_websocket_requires_auth` (or delete the hook
      if Phase 0 chose stay-open — then document the risk).
      **Verify:** Unity.
- [ ] Blackbox: Test 26 rejects unauthenticated upgrade when required;
      accepts valid JWT/cookie per decision.
      **Verify:** Test 26 green.
- [ ] `mkq` + `mkp`. Docs: terminal operator note.

### Done means

No silent "auth hook always false" on exposed deployments unless Phase 0
explicitly accepted that.

### Exit gate

Test 26 green. `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 8 — Login MFA via Mail Relay OTP

### Goal

`/api/auth/login` can require a second factor using existing Mail Relay
OTP send/verify. Email verify on register only if Phase 0 pulled it in.
Password reset stays out unless Phase 0 pulled it in.

### Entry gate

Phase 7 complete. Mail Relay OTP primitives still green (Unity
`mailrelay_otp_*`).

### Work items

- [ ] Config knobs (default off): e.g. `Auth.Mfa.Enabled`, digits /
      expiry reuse of `MailRelay.Otp` or dedicated auth section. Lock
      JSON shape in discussion before coding.
      **Verify:** schema + config Unity + jsonlint.
- [ ] Login flow: password success → if MFA enabled and user enrolled,
      send OTP (`mailrelay_otp_generate_and_send`, purpose login_mfa) →
      return a **pending** envelope (no JWT yet) → second request
      verifies (`mailrelay_otp_verify`) → then `generate_jwt` /
      `store_jwt`.
      **Verify:** Unity; Test 40 MFA cases when enabled in test config.
- [ ] Enrollment: how a user gets MFA (contact email required — Phase 1
      makes register honest). Lock in discussion (always-on for all
      vs per-account flag).
      **Verify:** Status + code.
- [ ] If Phase 0 included email verify: register sends OTP; account
      unauthorized until verify. Else `[~]`.
- [ ] Never log OTP plaintext. `mkq` + `mkp` + `mks` if Test 40 grows.
- [ ] Docs: auth API + MAIL_GUIDE pointer (Mail Relay remains the mail
      engine).

### Done means

With MFA config on, password alone does not mint a JWT; OTP verify does.
With MFA off, Test 40 unchanged.

### Exit gate

Test 40 green (MFA off default + MFA on cases). `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 8b — Password Reset Via Mail Relay OTP

### Goal

A user who forgot their password can recover their account through
Hydrogen alone, using the **same** Mail Relay OTP primitives Phase 8
wires for login MFA, without leaking whether an email is registered.

### Entry gate

Phase 8 complete (OTP config/wiring pattern and purpose-lookup
convention already established; reuse them rather than inventing a
second OTP call pattern).

### Work items

- [ ] New OTP purpose (distinct lookup id from `login_mfa` — locked in
      Phase 0) so a captured login OTP cannot be replayed as a password
      reset token and vice versa.
      **Verify:** lookup value in Status; migration packet if a new
      lookup row is needed.
- [ ] `POST /api/auth/password/reset-request { email }`: always returns
      the same generic `202`-style body whether or not the email
      resolves to an account (Phase 1 makes `account_contacts` reliable
      for this lookup). On a hit, call
      `mailrelay_otp_generate_and_send` with the new purpose; on a miss,
      do nothing but still return the generic response and take
      roughly the same time (avoid a timing side-channel).
      **Verify:** Unity; Test 40 checks response is identical for known
      vs unknown email.
- [ ] `POST /api/auth/password/reset-confirm { email, otp, new_password
      }`: `mailrelay_otp_verify` the code, then reuse the **same**
      password-hashing path register/login already use to set the new
      hash. On success, invalidate every stored JWT for that account
      (same revocation primitive Phase 6/11 rely on) so a stolen session
      cannot survive a reset the legitimate owner initiated.
      **Verify:** Unity; Test 40 end-to-end reset flow; old JWT rejected
      after reset.
- [ ] Per-account and per-IP throttling on `reset-request` (reuse an
      existing rate-limit/backoff utility if one exists in `mailrelay`
      or `webserver`; otherwise a minimal counter keyed by account +
      window). This is anti-abuse plumbing for this endpoint, not the
      excluded "concurrent-session product limits" feature.
      **Verify:** Unity throttle test; Test 40 asserts 4xx after the
      configured burst.
- [ ] Config knob (default **on**, since this closes a real recovery
      gap) under the same `Auth.*`/OTP config section Phase 8 defines.
      **Verify:** schema + config Unity + jsonlint.
- [ ] Never log the OTP or the new password. `mkq` + `mkp` + `mks` if
      Test 40 grows.
- [ ] Docs: auth API doc gets the two new endpoints; note the
      no-enumeration contract explicitly so a future change doesn't
      "fix" it into a leak.

### Done means

A user with a registered email can reset their password without any
operator/DB intervention, without revealing account existence, and the
reset revokes prior sessions.

### Exit gate

Test 40 green (reset happy path + unknown-email + throttle cases).
`mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 9 — IdP Durability And Userinfo Honesty

### Goal

Auth codes and refresh tokens survive process restart / multi-worker via
QueryRefs `#136`–`#142`. Userinfo can merge accounts-DB profile.
`oidc_users.c` dummy is retired or documented as unused and kept off
the dead-code lie list.

### Entry gate

Phase 8 (and 8b) complete. IdP MVP (Test 45) still green.

### Work items

- [ ] Run SchemaTool against QueryRefs `#136`–`#142` before writing any
      wiring code — confirm the live schema still matches what these
      QueryRefs assume. Do not hand-edit the DB; if SchemaTool reports
      drift, generate a migration packet and hand it to the user first.
      **Verify:** SchemaTool report attached/summarized in Working Log.
- [ ] Wire authorization-code store to `#136` insert / `#137` get /
      `#138` consume on the Test 45 / production path. Unity may keep
      in-memory.
      **Verify:** Test 45 still 7/7; restart-mid-code optional subtest
      if practical.
- [ ] Wire refresh to `#139`–`#142` (insert / get / revoke / family).
      Rotation reuse still burns family.
      **Verify:** Test 45 refresh cases.
- [ ] Userinfo: lookup account by id (new QueryRef if needed) for
      email/profile when scope allows; do not invent a second user
      table.
      **Verify:** Unity + Test 45 userinfo claims.
- [ ] `oidc_users.c`: delete unused dummy **or** stop calling it from
      `init_oidc_service` and document. Do not leave `test_user` as a
      secret backdoor.
      **Verify:** `mkt` dead-code gate; `mkp`.
- [ ] Operator runbook: HA now requires shared DB for codes/refresh.
      **Verify:** `OIDC_IDP_OPERATOR.md`.

### Done means

IdP MVP behavior unchanged for single-process; codes/refresh are
DB-backed on the live path. No dummy user auth.

### Exit gate

Test 45 green. Test 42 still green (no RP regression). `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 10 — IdP Optional Post-MVP

### Goal

Pull only items product wants. Unpulled stay 501 with comments pointing
here, not at a dead `OIDC_IDP.md` phase number.

### Entry gate

Phase 9 complete. Product owner names which of 10.1–10.8 to pull.
Unpulled items may all be `[~]` — that is a valid complete phase.

### Work items (pull individually)

- [ ] 10.1 RP-initiated logout `/oauth/end-session` + optional
      `id_token_hint`. Today: `end_session.c` 501.
- [ ] 10.2 Client credentials grant for confidential service clients.
- [ ] 10.3 Consent screen + `oauth_consents` when third-party clients
      need user-approved scopes.
- [ ] 10.4 Dynamic client registration (auth-gated). Security review
      required before enable. Today: `registration.c` 501.
- [ ] 10.5 Key rotation job / admin trigger with dual-key JWKS overlap
      (`oidc_rotate_keys` exists; full policy was deferred).
- [ ] 10.6 Device code / native app enhancements — separate mini-plan
      if pursued.
- [ ] 10.7 Access-token denylist (`jti` + table) so `/oauth/revoke` on
      access is real. Default remains TTL-only if unpulled.
- [ ] 10.8 Native redirect URI schemes (`myapp://`) via explicit
      allow-list. Default remains http/https only.
- [ ] 10.9 `hydrogen_config_schema.json` OIDC IdP block if test_93
      needs it.

Each pulled item: Unity + Test 45 subtest + `mkq`/`mkp` + docs.

### Done means

Working Log lists pulled vs parked. No 501 claims "Phase 17" anymore —
they point at this phase.

### Exit gate

Test 45 green. Comments/docs updated.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 10b — Self-Service Session Visibility And Revocation

### Goal

A logged-in user can see their own active Hydrogen sessions (password
and OIDC-minted alike) and revoke one or all of them, not only the
current one via `logout`.

### Entry gate

Phase 6 complete (backchannel logout already establishes the pattern
of deleting a stored JWT to revoke it live — this phase reuses that
same revocation semantics for a user-triggered instead of IdP-triggered
revoke).

### Work items

- [ ] Decide (Phase 0 already locked this) whether existing JWT storage
      carries enough metadata (created time, last-used time, IdP vs
      password origin) to list usefully, or whether a device/user-agent
      label needs a new column. Prefer using what is already stored;
      only generate a migration packet if genuinely missing.
      **Verify:** Status note; migration packet handed to user if
      needed.
- [ ] `GET /api/auth/sessions`: list the caller's own non-expired stored
      JWTs by id/created/last-used/origin (password vs `idp_provider`).
      Never return token material, only metadata, and only for the
      caller's own `account_id` (row-level check, not just JWT claim
      trust).
      **Verify:** Unity; Test 40.
- [ ] `DELETE /api/auth/sessions/{id}`: revoke one session (must belong
      to the caller). `POST /api/auth/sessions/revoke-all-others`:
      revoke every stored JWT for the account except the one presenting
      the request.
      **Verify:** Unity; Test 40 — revoked session's JWT is rejected on
      the next authenticated call, current session survives
      `revoke-all-others`.
- [ ] Cross-check with Phase 10.7 (access-token denylist, if pulled):
      if that item is *not* pulled for the IdP, note here that IdP
      access tokens remain TTL-only even after this phase — this phase
      only guarantees revocation for Hydrogen's own stored-JWT sessions
      (password + OIDC-minted Hydrogen JWTs), not raw unexpired IdP
      access tokens issued to third-party clients.
      **Verify:** Working Log note; no over-claiming in docs.
- [ ] `mkq` + `mkp`. Docs: auth API doc gets the three new endpoints.

### Done means

A user who suspects a compromised device can list and kill that
session (or everything else) from their own account without operator
help.

### Exit gate

Test 40 green (list, single revoke, revoke-all-others, cross-account
isolation). `mkq`/`mkp` green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 11 — Real Keycloak E2E And RP-Initiated Logout Live

### Goal

Sign the production SSO chain against real Keycloak (not mock). Verify
RP-initiated logout live.

**Parallel exception (Phase 0):** may start as soon as a Keycloak user
without OTP (or a current TOTP secret) exists, even if Phases 1–10 are
unfinished. Do not block code phases on this.

### Entry gate

Phase 0 complete **and** a usable Keycloak test user. Deploy or
port-forward can reach discovery/token/JWKS. Secrets in env only.

### Work items

Environment (locked; details in
[`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md)):
issuer `https://www.500passwords.com/realms/festival`, client `lithium`,
origin/redirect `https://lithium.philement.com/api/auth/oidc/callback`,
`match_email_only`, provision off, `RequireEmailVerified=true`.
Pre-flight (discovery, start 302, handoff invalid, callback errors)
already passed 2026-07-11.

Need a Keycloak user **without** OTP (or a current TOTP secret). The
known user `andrew@500foods.com` hits an MFA challenge — not a Hydrogen
defect.

**SSO happy path (tick with date / operator / notes):**

- [ ] 11.1 Password login still works (user has `password_hash`).
      **Verify:** lands in app; JWT stored; `AUTH_LOGIN` emitted.
- [ ] 11.2 Same user SSO via "Sign in with 500 Passwords"; same
      `account_id`; `account_oidc_identities` row for `(iss, sub)`.
      **Verify:** DB row; no duplicate accounts.
- [ ] 11.3 Unknown Keycloak email → `oidc_error=no_account`; no
      `accounts` row (provisioning off).
      **Verify:** error mapping; DB unchanged.
- [ ] 11.4 Hard reload mid-session: JWT still valid; no second handoff.
- [ ] 11.5 Logout revokes Hydrogen JWT; subsequent API calls fail.
- [ ] 11.6 `/api/auth/renew` succeeds for an OIDC-minted JWT (idle past
      half-life).
- [ ] 11.7 Managers (Queries, Lookups, Style, Crimson, Tour, Version,
      Session Log, User Profile) behave like password login; Conduit OK.
- [ ] 11.8 Keycloak tokens never in browser storage or info-level
      Hydrogen logs.

**RP-initiated logout (code shipped; live verify open):**

- [ ] 11.9 Hydrogen rebuilt with `POST /api/auth/oidc/end-session`;
      Keycloak client has Valid post-logout redirect URI
      `https://lithium.philement.com/*`; fresh OIDC JWT contains
      `id_token` + `idp_provider`.
- [ ] 11.10 Global signout navigates to Keycloak logout
      (`id_token_hint`, `post_logout_redirect_uri`, `client_id`),
      returns to Lithium login; Keycloak session cookies gone; clicking
      500 Passwords again does **not** auto-login.
- [ ] 11.11 Password-only Global signout is local only (no Keycloak
      redirect).

**Sign-off:**

- [ ] 11.12 Independent second pass of 11.1–11.11.
      **Verify:** second operator/date in Status.
- [ ] 11.13 Test 42 and Test 40 still green; kill switch re-tested.

If 11.10 still auto-logs in: confirm binary rebuilt, non-empty
`id_token_hint`, `post_logout_redirect_uri` origin of RedirectUri, pod
logs for `/end-session`. Historical troubleshooting:
[`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md).

### Done means

Status records dates/operators for 11.1–11.13. Production policy
unchanged unless Phase 0 recorded a change.

### Exit gate

Phase Status complete with two signatures (or explicit variance).
Automated suite still green.

### Status

- **State:** pending (historically in progress 2026-07-11; OTP blocker)
- **Date:**
- **Result:** Pre-flight against real Keycloak already passed (start 302,
  handoff invalid, callback errors). Happy path unsigned.
- **Variances:** Blocker is Keycloak MFA on `andrew@500foods.com`, not
  Hydrogen code.

---

## Phase 12 — Docs And Closeout

### Goal

Indexes and operator docs point only at AUTH_FINALE for remaining work.
No second active auth plan.

### Entry gate

Phases 1–10, 8b, 10b complete (Phase 11 may still be unsigned if still
OTP-blocked; record variance).

### Work items

- [ ] `oidc_rp.md`, `OIDC_IDP_OPERATOR.md`, `oidc_endpoints.md`,
      Lithium OIDC/Keycloak docs, SITEMAP, plans README, TODO snapshot
      all agree.
      **Verify:** `mkl` + Test 90.
- [ ] Comments in `oidc_rp_link_*.c` / `end_session.c` / `registration.c`
      / `oidc_users.c` / `terminal_websocket.c` match reality.
      **Verify:** grep for stale "Phase 17" / "TODO.md" / "not wired."
- [ ] Auth API doc lists `password/reset-request`, `password/reset-confirm`,
      `GET /api/auth/sessions`, `DELETE /api/auth/sessions/{id}`,
      `POST /api/auth/sessions/revoke-all-others` alongside existing
      login/register/renew/logout.
      **Verify:** `mkl`.
- [ ] Working Log lessons for a future auth sequel.

### Done means

A new session can start from this file + Status blocks only.

### Exit gate

`mkl` green. Test 90 green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Cross-Phase Rules

- Every phase ends with earlier automated gates still green (especially
  Test 40 and Test 42 when auth paths change; Test 45 when IdP changes).
- No phase skips lint for touched languages.
- Configuration changes ship with schema/example/parser test in the same
  chunk.
- Documentation for a behavior ships in the same phase as the behavior
  (Phase 12 is leftover narrative only).
- Prefer fixing RP/auth bugs over adding parallel endpoints.
- Prefer domain-gated provision over open auto-create.

---

## Rollback

| Layer | Action |
| --- | --- |
| RP | `OIDC_RP.Enabled = false` or unset `HYDROGEN_OIDC_RP_ENABLED` |
| IdP | `OIDC.Enabled = false` |
| MFA | Auth MFA config off; password login remains |
| Password reset | Config knob off; recovery reverts to operator/DB, as today |
| Session self-service | Endpoints can be disabled/unrouted; `logout` still works |
| Client SPA | Hide SSO entry points; password login remains |
| Keycloak | Disable client or remove redirect URIs |
| Data | Identity rows and contacts can remain; inert when RP disabled |

No reverse migration required to disable features.

---

## Relationship To Other Documents

| Document | Role |
| --- | --- |
| This file | **Only** active auth / OIDC / Keycloak plan (Phase 11 owns E2E ticks) |
| [`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md) | Historical real-Keycloak pre-flight / defects |
| [`OIDC-PLAN_COMPLETE.md`](/docs/H/plans/complete/OIDC-PLAN_COMPLETE.md) | RP Phases 1–26+31 archaeology |
| [`KEYCLOAK_PLAN_COMPLETE.md`](/docs/H/plans/complete/KEYCLOAK_PLAN_COMPLETE.md) | Production SSO ops archaeology |
| [`OIDC_IDP_COMPLETE.md`](/docs/H/plans/complete/OIDC_IDP_COMPLETE.md) | IdP Phases 0–16 archaeology |
| [`AUTH_PLAN_COMPLETE.md`](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) | Password auth suite |
| [`oidc_rp.md`](/docs/H/api/auth/oidc_rp.md) | RP wire contract |
| [`oidc_endpoints.md`](/docs/H/api/oidc/oidc_endpoints.md) | IdP wire contract |
| [`OIDC_IDP_OPERATOR.md`](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md) | IdP operator runbook |
| [`LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md) | Client recipe |
| [`MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) | OTP engine; MFA wiring moved here |
| [`CHAT_FINALE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) | Chat/MCP JWT mint — not this plan |
| [`TESTING.md`](/docs/H/tests/TESTING.md) | Blackbox conventions |
| [`TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md) | Unity conventions |
| [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) | Build aliases |

---

## Working Log (cross-phase memory)

### Decisions log

- **(Plan revised, 2026-09-03)** Added Phase 8b (password reset via the
  same Mail Relay OTP primitives as Phase 8, dedicated OTP purpose, no
  email-enumeration leak, revokes existing sessions on success) and
  Phase 10b (self-service list/revoke of own active JWT sessions,
  reusing Phase 6's revocation semantics). Used lettered phase numbers
  to avoid renumbering Phase 9–12 and breaking existing cross-references
  to specific phase numbers in `MAILRELAY_PLAN.md` and
  `plans/README.md`. Pulled three previously-deferred product decisions
  (Phase 1 contact-insert failure mode, Phase 2 role-insert failure
  mode, Phase 3 realm/client/merge scope) into Phase 0 so those phases
  do not have to invent policy mid-implementation. Added an effort
  column and an explicit ops-gate annotation to the Phase Index so
  Phase 11's "pending" does not read as unstarted engineering work.
  Added a SchemaTool-audit work item at the start of Phase 9 given the
  project's known migration-drift risk.
- **(Plan authored, 2026-09-02)** AUTH_FINALE created to gather remaining
  OIDC / Keycloak / auth work. Moved KEYCLOAK_PLAN, OIDC-PLAN, OIDC_IDP
  to `plans/complete/` with `_COMPLETE` suffix. TODO items 1, 3, 6, 15,
  17, 18 collapsed into one P0 pointer. Mail Relay keeps OTP primitives;
  login MFA wiring lives here (Phase 8). Chat MCP mint stays CHAT_FINALE.
  Phase 11 E2E may run in parallel when a non-OTP Keycloak user exists.
- **(2026-09-03)** `OIDC_E2E_LOG` moved to
  [`OIDC_E2E_LOG_COMPLETE.md`](/docs/H/plans/complete/OIDC_E2E_LOG_COMPLETE.md).
  Remaining unsigned SSO + RP-logout ticks live in Phase 11 of this file.
- **Carry-forward from KEYCLOAK_PLAN (2026-07-11):** Production RP policy
  `match_email_only`, provision off, `RequireEmailVerified=true`, roles
  from `database`. Issuer `festival` / client `lithium` / redirect
  `https://lithium.philement.com/api/auth/oidc/callback`.
- **Carry-forward from OIDC_IDP (2026-07-23):** Access revoke = no
  denylist (TTL-only). Codes/refresh QueryRefs exist but runtime is
  in-memory. Native redirect schemes rejected until allow-list. RP
  leftover fields on `OIDCConfig` kept for JSON compat.

### Surprises / deviations (historical, still true)

- Keycloak admin credentials in tenant README are outdated.
- Test user `andrew@500foods.com` accepts password then hits OTP —
  **not** a Hydrogen defect.
- QueryRef `#082` originally filtered `contact_type_a18 = 0` (Username);
  production email rows are `1`. Fixed in source + production; E2E log
  records it.
- Duplicate email contacts caused `email_ambiguous`; cleaned in Lithium
  DB during E2E.
- `SpaFallback` was added so `/login?oidc=1&…` hits the SPA.

### Reusable snippets / gotchas

- SPA must use **top-level navigation** to `/api/auth/oidc/start`.
- Redirect URI is exact string match in Keycloak.
- JWT `roles` = role_id integers, not names.
- Sync DB: register pending → submit → wait.
- Test 42 uses mock Keycloak; real IdP is Phase 11 only.
- RP logout needs `id_token` claim on the Hydrogen JWT; tokens minted
  before that deploy cannot drive IdP logout.
- `post_logout_redirect_uri` is the **origin** of RP `RedirectUri`.
- Port-forward + `Host: lithium.philement.com` works for pre-flight.
- After C: `mkq` then `mkp`. After bash: `mks`. After docs: `mkl`.
)
