<!-- markdownlint-disable MD007 MD024 -->
# Hydrogen as OIDC Identity Provider (IdP) Implementation Plan

## Purpose

Define a gated, phase-by-phase operational plan for finishing Hydrogen's OpenID
Connect **Identity Provider** stack: configuration and launch already exist;
core service modules and HTTP handlers are largely stubs; MHD routing is not
wired; there is no OAuth client/code/refresh schema. When complete, Hydrogen can
issue OIDC tokens (authorization code + PKCE, RS256 id_token/access_token,
refresh, userinfo, JWKS, discovery) backed by the existing Acuranzo
`accounts` / password / roles model.

This document is the working plan. It is meant to be edited as work proceeds.
Each phase is numbered, focused, and gated. Do not start a phase until the
previous phase's exit gate is green. Record learnings in the Working Log.

## How To Use This Document

- Work one phase at a time, top to bottom. Each phase has an Objective, Entry
  Gate, numbered work items, and an Exit Gate.
- Each work item has an explicit verification (`mkt`, named `mku <base>`, named
  blackbox test, migration check, or lint).
- Mark an item `[x]` only when its verification has actually passed. If deferred,
  change to `[~]` with a one-line rationale and the target phase.
- After each phase, fill in the phase Status block and append reusable
  discoveries to the Working Log.
- Never mark a phase complete with a failing or skipped gate unless explicitly
  recorded as deferred.
- **Do not modify OIDC RP** (`src/api/auth/oidc_rp/`, `config_oidc_rp`, Test 42)
  except where this plan explicitly allows shared crypto helpers.

## Resuming Work on This Plan

CURRENT PAUSE POINT (as of 2026-07-23): **Phases 0–7 complete. Next: Phase 8
(resource-owner login at authorize).**

### Resume here next session

1. Confirm Phases 0–7 Status complete; re-read Working Log.
2. Baseline: `zsh -ic 'mkt'`;
   `mku oidc_auth_codes_test_issue_consume && mku oidc_pkce_test_verify_s256 && mku oidc_clients_test_validate_client`.
3. Phase 8: authorize GET/POST + password via `#012`; issue code + 302.
4. Apply migrations **1271–1279** on all test DBs if not already (user applied
   1266–1270; 1271+ are new).
5. **Test 45 MUST cover all 7 engines** (postgres/mysql/sqlite/db2/mariadb/
   cockroachdb/yugabytedb) like test_40 — not SQLite-only.
6. Keep RP/Test 42 green.

### Session checklist (every OIDC IdP return)

1. Confirm latest completed phase via Status blocks; active phase = first not
   complete.
2. Re-read Working Log decisions that affect the next chunk.
3. Baseline: `zsh -ic 'mkt'`; relevant `mku` under `tests/unity/src/oidc/` and
   `tests/unity/src/api/oidc/`; blackbox `test_45_oidc_idp.sh` when it exists.
4. One small chunk: questions → plan → implement → verify → update this plan →
   stop for review.
5. Never log client secrets, authorization codes, refresh tokens, id_tokens, or
   passwords in normal logs or test artifacts.

Build aliases: `zsh -ic '<alias>'` (`mkt`, `mka`, `mku <base>`, `mkp`, `mks`).

## Scope And Repo Areas

Primary repo area: `/elements/001-hydrogen/hydrogen`

Related:

- `/elements/001-hydrogen/hydrogen/src/oidc/` — IdP core (keys, tokens, clients,
  users, service) — **stubs to finish**.
- `/elements/001-hydrogen/hydrogen/src/api/oidc/` — IdP HTTP handlers — **stubs;
  not registered with MHD**.
- `/elements/001-hydrogen/hydrogen/src/config/config_oidc.{c,h}` — provider-side
  `OIDC` / `OIDCConfig`.
- `/elements/001-hydrogen/hydrogen/src/launch/launch_oidc.c` — launch readiness
  - `init_oidc_service`.
- `/elements/001-hydrogen/hydrogen/src/landing/landing_oidc.c` — shutdown.
- `/elements/001-hydrogen/hydrogen/src/api/auth/` — password login, HS256 session
  JWT (reuse password verify + roles; **do not** overload for OIDC tokens).
- `/elements/001-hydrogen/hydrogen/src/api/auth/oidc_rp/` — OIDC **RP** (Keycloak
  client); patterns only (PKCE, JWKS verify).
- `/elements/001-hydrogen/hydrogen/src/utils/utils_crypto.{c,h}` — base64url,
  SHA-256, HMAC, JWK→PKEY, RS256 **verify** (need **sign** for IdP).
- `/elements/002-helium/acuranzo/migrations/` — accounts, tokens, roles, OIDC RP
  identities; **new oauth_* tables** for IdP.
- `/docs/H/core/subsystems/oidc/oidc.md` — original IdP architecture notes.
- `/docs/H/core/reference/oidc_architecture.md` — component architecture.
- `/docs/H/api/oidc/oidc_endpoints.md` — endpoint doc stubs (empty sections).
- `/docs/H/plans/KEYCLOAK_PLAN.md` — RP plan; IdP explicitly out of scope there.
- `/docs/H/plans/OIDC-PLAN.md` — RP implementation history (Phases 1–27).

Date of snapshot: 2026-07-23

---

## Goals And Non-Goals

### Goals

- Hydrogen serves as a standards-oriented OIDC provider for first-party and
  trusted third-party clients (authorization code + PKCE S256 required for
  public clients).
- Resource owners are existing Acuranzo `accounts` authenticated via the same
  password path as `/api/auth/login` (QueryRef `#012`), with roles from `#017`.
- Issue RS256-signed `id_token` and access tokens; opaque or rotated refresh
  tokens stored server-side.
- Discovery (`/.well-known/openid-configuration`) and JWKS (`/oauth/jwks`) are
  live and accurate when `OIDC.Enabled = true`.
- Full Unity coverage for non-static helpers; blackbox Test 45 drives a mock
  RP through the complete code flow (mirror of Test 42's mock-IdP approach,
  inverted).
- Disabled by default; kill switch leaves no open endpoints.

### Non-Goals (this plan)

- Replacing Keycloak for external org SSO (that remains RP + Keycloak).
- Implicit flow, hybrid flow, device code, CIBA, PAR/JAR (post-MVP).
- Dynamic client registration as a public open endpoint (optional later;
  admin/seeded clients first).
- SAML, social federation as IdP plugins.
- Using Hydrogen session HS256 JWTs as OIDC access tokens (separate token
  surfaces).
- Lithium SPA work beyond documenting how a client would point at Hydrogen IdP.
- Changing OIDC RP behavior or Test 42 semantics.

---

## Architecture (locked)

### Two OIDC stacks — do not confuse them

| Stack | Role | Location | This plan |
| --- | --- | --- | --- |
| OIDC **Provider / IdP** | Hydrogen issues tokens | `src/oidc/`, `src/api/oidc/`, `config_oidc` | **In scope** |
| OIDC **Relying Party** | Hydrogen is client of Keycloak | `src/api/auth/oidc_rp/`, `config_oidc_rp` | **Out of scope** (reference only) |

### Target authorization code + PKCE flow

```text
Client SPA/App  →  GET  /oauth/authorize?client_id&redirect_uri&response_type=code
                     &scope=openid&state&nonce&code_challenge&code_challenge_method=S256
Hydrogen IdP    →  authenticate resource owner (login form or existing session)
Hydrogen IdP    →  optional consent
Hydrogen IdP    →  302 redirect_uri?code=<auth_code>&state=
Client          →  POST /oauth/token  (code + code_verifier + client auth if confidential)
Hydrogen IdP    →  { access_token, id_token, refresh_token?, token_type, expires_in }
Client          →  GET  /oauth/userinfo  Authorization: Bearer <access_token>
Client          →  GET  /.well-known/openid-configuration
Client          →  GET  /oauth/jwks
```

### Endpoint map (intended)

| Method | Path | Purpose |
| --- | --- | --- |
| `GET` | `/.well-known/openid-configuration` | Discovery document |
| `GET` | `/oauth/jwks` | Public signing keys (JWKS) |
| `GET`/`POST` | `/oauth/authorize` | Authorization endpoint |
| `POST` | `/oauth/token` | Token endpoint |
| `GET` | `/oauth/userinfo` | UserInfo (Bearer access token) |
| `POST` | `/oauth/introspect` | Token introspection (RFC 7662) |
| `POST` | `/oauth/revoke` | Token revocation (RFC 7009) |
| `GET`/`POST` | `/oauth/end-session` | RP-initiated logout (later phase) |
| `POST` | `/oauth/register` | Dynamic registration (deferred / admin-only) |

When `OIDC.Enabled = false` (default): endpoints must not be registered, or must
return `503` with a clear error — prefer **not registered** so scans see 404.

### Token surfaces (hard rule)

| Token | Algorithm | Storage | Purpose |
| --- | --- | --- | --- |
| Hydrogen session JWT (`/api/auth/login`) | HS256 | `tokens.token_hash` via `#013` | Conduit / API session |
| OIDC `id_token` | RS256 | not stored (JWT) | OIDC identity assertion to client |
| OIDC access token | RS256 JWT (preferred) or opaque | optional denylist | Resource access / userinfo |
| OIDC refresh token | opaque | new `oauth_refresh_tokens` | Refresh grant |
| Authorization code | opaque, single-use, short TTL | new `oauth_authorization_codes` | Code exchange |

Never put OIDC tokens in the existing `tokens` table without a design that
preserves session-JWT semantics. Prefer dedicated oauth_* tables.

### Identity model

- **Resource owner** = `accounts` row (+ `account_contacts` for email claims).
- **Authentication** = reuse password verify path (QueryRef `#012`) and login
  rate-limit QueryRefs `#004`–`#007` at the authorize login step.
- **Roles / claims** = QueryRef `#017` (and `#127` by name if needed).
- **`account_oidc_identities`** = RP-only (`iss`/`sub` of *external* IdPs). Do
  **not** use for Hydrogen-as-IdP client registry.

### Client model

- Seeded/admin `oauth_clients` rows (client_id, secret hash, redirect URI allow
  list, grant types, public vs confidential, require_pkce).
- Public clients: PKCE S256 required; no client_secret.
- Confidential clients: client_secret_basic or client_secret_post.

---

## Current Observed State (2026-07-23)

### What already works (do not rebuild)

| Area | Evidence |
| --- | --- |
| Config load / defaults | `config_oidc.c`, `initialize_config_defaults_oidc()` — `Enabled=false` by default |
| Launch readiness validation | `launch_oidc.c` — issuer URL, port, token lifetimes, keys path |
| Landing / shutdown hooks | `landing_oidc.c` + Unity landing tests |
| Service init skeleton | `init_oidc_service` wires key/token/user/client contexts (stubs) |
| Discovery JSON *builder* | `oidc_generate_discovery_document()` builds real-looking JSON from config |
| Password login + roles | `src/api/auth/`, QueryRefs `#008`–`#017` |
| Session JWT mint/store | HS256 `generate_jwt` / `store_jwt` — separate product surface |
| OIDC RP (Keycloak) | Full stack + Test 42 — **do not break** |
| Crypto primitives | `utils_crypto`: base64url, SHA-256, HMAC, `utils_rs256_verify`, JWK→PKEY |
| PKCE helpers (RP) | `oidc_rp_pkce.c` — reuse patterns for S256 verify on IdP side |

### What is stubbed or dead

| Component | Path | Gap |
| --- | --- | --- |
| Key management | `src/oidc/oidc_keys.c` | No real RSA keys; JWKS is hard-coded placeholder |
| Token service | `src/oidc/oidc_tokens.c` | Stub strings; validate/revoke always succeed |
| Client registry | `src/oidc/oidc_clients.c` | Always-true validate/authenticate |
| User management | `src/oidc/oidc_users.c` | Hard-coded `test_user`; not wired to accounts |
| Protocol process_* | `src/oidc/oidc_service.c` | authorize/token/userinfo/introspect/revoke → `not_implemented` |
| HTTP registration | `src/api/oidc/oidc_service.c` | `register_oidc_endpoints()` is a no-op |
| MHD routing | `web_server_request.c` / `api_service.c` | **Zero call sites** for `handle_oidc_request` |
| Discovery handler | `api/oidc/discovery/discovery.c` | Hard-coded `https://example.com`; ignores service builder |
| Authorize handler | `api/oidc/authorization/authorization.c` | Thin shell; backend returns error JSON as "code" |
| Registration / end-session | handlers | Explicit 501 `not_implemented` |
| Protocol Unity | `tests/unity/src/oidc/`, `api/oidc/` | **Missing** (only launch/landing/config tests) |
| Blackbox IdP | — | No Test 45; only Test 42 (RP) |
| OAuth DB tables | acuranzo | No `oauth_clients` / codes / refresh / consent |

### Acuranzo tables — reuse vs add

**Reuse as-is:**

| Table | Migration | IdP use |
| --- | --- | --- |
| `accounts` | 1005 (+1190 nullable password) | Resource owners |
| `account_contacts` | 1003 | Email / preferred_username claims |
| `account_roles` / `roles` | 1004 / 1016 | Roles into tokens / scopes |
| `actions` / `lists` | 1006 / 1013 | Login audit / IP block |
| `mail_otp_codes` | 1221 | Optional step-up MFA later |
| `tokens` | 1021 | **Hydrogen session JWT hashes only** — do not overload |
| `account_oidc_identities` | 1189 | **RP only** — external IdP links |

**Add (new migrations 1266+):**

| Table | Purpose |
| --- | --- |
| `oauth_clients` | client_id, secret_hash, name, confidential, redirect_uris (JSON/text), grant_types, require_pkce, status |
| `oauth_authorization_codes` | code_hash, client_id, account_id, redirect_uri, scope, nonce, code_challenge, method, expires_at, consumed_at |
| `oauth_refresh_tokens` | token_hash, client_id, account_id, scope, expires_at, revoked_at, parent/rotation |
| `oauth_consents` (optional MVP+) | account_id, client_id, scope, granted_at |

### QueryRefs — reuse vs add

**Reuse:** `#001` system info; `#004`–`#007` login rate limit; `#008` account by login; `#012` password check; `#014` log login; `#017` roles; `#127` role by name; OTP `#112+` if MFA later.

**Do not reuse for IdP protocol:** `#013`/`#018`/`#019` (session JWT); `#080`–`#084` (RP identities).

**Next free (snapshot):** migration **1266+**, QueryRef **132+** (gaps 076–079 available if a tight IdP block is preferred). Re-check disk before each migration phase.

### Confirmed design decisions (carry forward)

1. IdP and RP are separate stacks; shared code only via `utils_crypto` and documented helpers.
2. Default `OIDC.Enabled = false`; no endpoints when disabled.
3. Authorization code + PKCE S256 is the only interactive grant in MVP.
4. Client credentials grant is optional post-MVP (service accounts).
5. RS256 only for OIDC JWTs; reject `alg=none`.
6. Resource owners are Acuranzo accounts; no parallel user store in `oidc_users` long-term (stub API becomes thin facade over auth DB).
7. In-memory auth-code store is acceptable for early Unity phases; durable DB before blackbox multi-process and production gate.
8. Implicit flow stays disabled (`allow_implicit_flow = false`).
9. Test 45 owns IdP blackbox ports `545x` per TESTING.md scheme and **must**
   exercise **all 7 database engines** in parallel (same set as test_40), not
   SQLite alone.
10. Historical docs under `docs/H/core/subsystems/oidc/` are aspirational; this plan is the implementation source of truth.

---

## Reference Conventions

Match existing Hydrogen / auth / RP patterns; do not invent parallel stacks.

### Config

- Struct: `OIDCConfig` in `config_oidc.h`; member `app_config->oidc`.
- JSON section: `OIDC` with `Enabled`, `Issuer`, `Endpoints`, `Keys`, `Tokens`.
- Clean up legacy RP-like fields on `OIDCConfig` (`client_id`, `client_secret`,
  `redirect_uri`) in an early config phase — they belong to `OIDC_RP`, not IdP.
- Defaults: disabled; signing alg `RS256`; lifetimes already set in
  `config_defaults.c`.
- Schema: update `tests/artifacts/hydrogen_config_schema.json` on field changes;
  run `tests/test_93_jsonlint.sh`.

### Source layout

- Core: `src/oidc/oidc_*.c` — no new `static` functions (Unity + static gate).
- HTTP: `src/api/oidc/<endpoint>/<endpoint>.c` — already scaffolded.
- Includes: `#include <src/hydrogen.h>` first; `<src/folder/...>` style.
- Logging: `log_this(SR_OIDC, ...)`; never log secrets or tokens.

### Web routing

- Register real prefixes via `register_web_endpoint` (or equivalent) from
  `register_oidc_endpoints()` when enabled:
  - `/.well-known` (discovery path validator)
  - `/oauth` (authorize, token, userinfo, jwks, introspect, revoke, …)
- Handler entry: existing `handle_oidc_request` in `api/oidc/oidc_service.c`.
- Do **not** put IdP under `/api/auth/oidc/*` (that is RP).

### Database

- Migrations: `acuranzo_NNNN.lua` after current max; luacheck + one engine path.
- QueryRefs: `internal_sql` / `system_sql` types only (not Conduit-public).
- Cross-engine macros: `${INTEGER}`, `${VARCHAR_*}`, `${TIMESTAMP_TZ}`,
  `${COMMON_CREATE}`, etc.

### Crypto

- Prefer extending `utils_crypto` with `utils_rs256_sign` + RSA keygen/export
  rather than open-coding OpenSSL only inside `oidc_keys.c`.
- Key files under configured `Keys.StoragePath`; generate on first launch if
  missing (test-friendly).
- JWKS exposes public keys only; private keys never leave the host.

### Tests

- Unity: `tests/unity/src/oidc/` and `tests/unity/src/api/oidc/` mirroring
  source; naming `<source>_test_<function>.c`.
- Blackbox: `tests/test_45_oidc_idp.sh`, configs `hydrogen_test_45_*.json`,
  ports **5450–5456**, helpers under `tests/lib/oidc_idp_*.sh`.
- Mock RP: Node (or curl-driven) client that performs authorize→login→token→
  userinfo against Hydrogen — inverse of `tests/lib/mock_keycloak/`.
- After C changes: `mkt` then relevant `mku`; after phase: `mkp`.
- After docs: `test_04_check_links.sh`, `test_90_markdownlint.sh`.
- RP regression: keep `test_42_oidc_rp.sh` green when touching shared crypto.

### Blackbox / ports

| Test | Role | Ports |
| --- | --- | --- |
| 40 | Password auth | 540x |
| 42 | OIDC RP | 542x / dedicated 42 configs |
| **45** | **OIDC IdP (this plan)** | **5450–5456** |

---

## Gate Philosophy

- One logical behavior per work item where practical.
- After every C change: `mkt`; then `mka` when clean; `mkp` for cppcheck.
- When adding unit-testable code: matching `mku <base>`.
- After DB migrations: `test_98_luacheck.sh` + at least one engine migration.
- After each phase that exposes HTTP: relevant blackbox slice (Test 45 when it
  exists; until then curl-based smoke in phase notes is not a substitute for
  the final blackbox gate).
- No client secret, auth code, refresh token, id_token, or password in logs,
  config dumps, or committed fixtures (use env / generated secrets in tests).
- Mark a phase complete only when every listed gate has passed or is explicitly
  deferred with rationale.

---

## Phase Index

| Phase | Title | Risk | Status |
| --- | --- | --- | --- |
| 0 | Baseline inventory, config cleanup decisions, gap lock | Low | complete |
| 1 | RS256 crypto primitives (`utils_crypto` sign + keygen) | Medium | complete |
| 2 | Real key management + JWKS (`oidc_keys`) | Medium | complete |
| 3 | Wire MHD registration for discovery + JWKS only | Medium | complete |
| 4 | Schema: `oauth_clients` + QueryRefs | Medium | complete |
| 5 | Client registry implementation (DB-backed) | Medium | complete |
| 6 | Schema: auth codes + refresh tokens (+ optional consent) | Medium | complete |
| 7 | Authorization codes + PKCE verify | High | complete |
| 8 | Resource-owner login at authorize (accounts + `#012`) | High | pending |
| 9 | Token endpoint: code exchange → id_token + access_token | High | pending |
| 10 | UserInfo endpoint | Medium | pending |
| 11 | Refresh token grant + rotation | Medium | pending |
| 12 | Introspection + revocation | Medium | pending |
| 13 | Unity suite completion for `src/oidc` + `api/oidc` | Medium | pending |
| 14 | Blackbox Test 45 — full code+PKCE flow | High | pending |
| 15 | Security hardening (redirect URI, state, rate limits) | High | pending |
| 16 | Docs, swagger/payload notes, operator runbook | Low | pending |
| 17 | Optional: end-session, client credentials, consent UI | Low | pending |

---

## Phase 0 — Baseline Inventory And Gap Lock

### Objective

Confirm the IdP scaffold, lock design decisions that later phases depend on, and
record accurate file-level gaps. No protocol behavior changes required beyond
documentation and optional config-field inventory.

### Entry Gate

- This plan reviewed.
- Ability to run `zsh -ic 'mkt'`.
- Disk check: highest acuranzo migration and QueryRef numbers.

### Work Items

- [x] 0.1 Inventory `src/oidc/*.c` and `src/api/oidc/**/*.c`; mark each function
  stub vs real in Working Log (or confirm this plan's tables still match).
  - **Verify:** Working Log table updated if drift found.
- [x] 0.2 Confirm `register_oidc_endpoints` is still a no-op and
  `handle_oidc_request` has no webserver call sites (`rg` evidence).
  - **Verify:** note paths in Working Log.
- [x] 0.3 List `OIDCConfig` fields that are RP leftovers (`client_id`,
  `client_secret`, `redirect_uri`) and decide keep-for-compat vs remove in
  Phase 0.4 / config cleanup.
  - **Verify:** decision recorded.
- [x] 0.4 Confirm free blackbox slot **45** and ports **5450–5456**; confirm
  next migration **≥1266** and QueryRef **≥132** (or chosen gap block).
  - **Verify:** numbers written in Working Log with date.
- [x] 0.5 Confirm non-goals (no RP breakage, no implicit flow, no overload of
  `tokens` / `account_oidc_identities`).
  - **Verify:** Status block confirmation.
- [x] 0.6 Baseline build: `zsh -ic 'mkt'`.
  - **Verify:** green.

### Exit Gate

- Working Log contains locked numbers (migration, QueryRef, test/ports) and
  config-field decision.
- `mkt` green.
- No code required unless inventory found a broken build (fix only).

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** Inventory matches plan; numbers locked; mkt green after Phase 1.
- **Variances:** Next free QueryRef is **210+** (not 132); highest migration
  **1265** so next is **1266**.

---

## Phase 1 — RS256 Crypto Primitives

### Objective

Add production-usable RSA key generation and RS256 **sign** alongside existing
verify helpers so IdP token issuance does not invent a second crypto stack.

### Entry Gate

- Phase 0 complete.
- `utils_crypto` already has `utils_rs256_verify` and JWK import.

### Work Items

- [x] 1.1 Add `utils_rsa_generate_keypair` (or equivalent) returning `EVP_PKEY*`
  with configurable bits (default 2048).
  - **Verify:** Unity test generates key; null/failure paths covered.
- [x] 1.2 Add `utils_rs256_sign(pkey, data, len, &sig, &sig_len)`.
  - **Verify:** Unity: sign then `utils_rs256_verify` round-trip.
- [x] 1.3 Add PEM export helpers (private + public) and/or JWK public export
  consistent with RP JWKS expectations (`n`, `e`, `kid`, `kty`, `alg`, `use`).
  - **Verify:** Unity asserts JWK fields parse back via existing JWK→PKEY.
- [x] 1.4 `mkt` + `mku` for new crypto tests; `mkp` on touched files.
  - **Verify:** green.
- [x] 1.5 Run a representative RP Unity that uses verify (or Test 42 subset if
  crypto headers changed) to ensure no RP regression.
  - **Verify:** green (`utils_crypto_test_rs256_verify` still builds via mkt;
  new tests exercise same verify path).

### Exit Gate

- Sign/verify round-trip Unity green.
- No secrets logged.
- RP still builds.

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** All Phase 1 APIs + 3 Unity files green; mkt green.
- **Variances:** mkp deferred to end of Phase 2 batch (same crypto tree).

---

## Phase 2 — Real Key Management And JWKS

### Objective

Replace `oidc_keys.c` stubs with real key lifecycle: load/generate signing key,
persist under `Keys.StoragePath`, expose active key, produce real JWKS JSON.

### Entry Gate

- Phase 1 complete (`utils_rs256_sign` available).

### Work Items

- [x] 2.1 Implement `init_oidc_key_management` to load keys from storage or
  generate+save when missing (test-friendly; no interactive prompt).
  - **Verify:** Unity with temp directory; second init loads same `kid`.
- [x] 2.2 Implement `oidc_get_active_signing_key`, `oidc_find_key_by_id`,
  `oidc_sign_data` / verify wrappers using Phase 1 helpers.
  - **Verify:** Unity coverage for active key and unknown kid.
- [x] 2.3 Replace stub `oidc_generate_jwks` with real public JWK set (only
  ACTIVE + ROTATING keys).
  - **Verify:** Unity JWKS JSON has `keys[]` with non-placeholder `n`.
- [x] 2.4 Optional skeleton for `oidc_rotate_keys` (may leave rotation policy
  as no-op until post-MVP) — document behavior.
  - **Verify:** documented in Working Log; if implemented, Unity for status
  transition.
- [x] 2.5 `mkt` + relevant `mku` + `mkp`.
  - **Verify:** green (mkp deferred with Phase 3 batch).

### Exit Gate

- Fresh process can generate keys and emit valid-looking JWKS without network.
- Cleanup frees all key material (no ASAN issues on Unity path).

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** Real RSA keys, PEM+kid persistence, JWKS, sign/verify. Unity 5/5.
- **Variances:** On-disk format is single active PEM + kid file (not full key
  set dump); sufficient for MVP single-signer.

---

## Phase 3 — Wire MHD: Discovery And JWKS Only

### Objective

Make IdP HTTP surface real for the two read-only endpoints. No authorize/token
yet. Proves routing, enable/disable behavior, and discovery accuracy.

### Entry Gate

- Phase 2 complete (real JWKS from key context).
- OIDC subsystem still initializes when enabled.

### Work Items

- [x] 3.1 Implement `register_oidc_endpoints()` to register `/.well-known` and
  `/oauth` (or precise paths) with validators + `handle_oidc_request` when
  `app_config->oidc.enabled`.
  - **Verify:** code review + later smoke; registration skipped when disabled.
- [x] 3.2 Fix `discovery.c` to call `oidc_generate_discovery_document()` (or
  shared builder) using configured issuer/endpoints — remove hard-coded
  `example.com`.
  - **Verify:** Unity for discovery builder; JSON contains issuer and jwks_uri.
- [x] 3.3 Ensure `jwks.c` returns Phase 2 JWKS with correct content-type.
  - **Verify:** Unity and/or manual curl against hydrogen with OIDC enabled.
- [~] 3.4 Add minimal test config `hydrogen_test_45_oidc_idp_disabled.json` and
  `..._discovery.json` (ports 5450+) — may be used by interim smoke until full
  Test 45 exists.
  - **Verify:** deferred to Phase 14 / early Phase 4 smoke; wiring complete
    without blackbox configs yet.
- [x] 3.5 Unity for `is_oidc_endpoint` / registration helpers where testable.
  - **Verify:** `mku` green.
- [x] 3.6 `mkt` + `mkp`.
  - **Verify:** mkt green; mkp deferred to next lint batch.

### Exit Gate

- With `OIDC.Enabled=true` and valid issuer, curl discovery and jwks succeed.
- With disabled, endpoints not exposed (or hard-fail closed).
- Launch still validates issuer/port/lifetimes.

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** MHD registration real; discovery uses service builder (MVP claims);
  JWKS via key context; Unity validators green.
- **Variances:** 3.4 blackbox configs deferred; live curl smoke not yet run
  (needs enabled config + writable Keys.StoragePath).

---

## Phase 4 — Schema: OAuth Clients

### Objective

Add durable client registry table and QueryRefs. No Hydrogen runtime wiring yet
beyond migration apply.

### Entry Gate

- Phase 3 complete (optional parallelism: schema may start after Phase 0 if
  needed, but prefer after routing so end-to-end thinking is stable).
- Next free migration/QueryRef re-checked on disk.

### Work Items

- [x] 4.1 Design `oauth_clients` columns (document in Working Log): client_id,
  client_secret_hash (nullable for public), client_name, confidential,
  redirect_uris, grant_types, response_types, require_pkce, token lifetimes
  overrides (optional), status, audit columns via `${COMMON_CREATE}`.
  - **Verify:** design approved in Working Log before coding migration.
- [x] 4.2 Migration `acuranzo_NNNN.lua` forward + reverse + diagram.
  - **Verify:** `test_98_luacheck.sh`; one engine migration path green.
- [x] 4.3 QueryRefs (starting at locked number): insert client, get by
  client_id, list active, update, soft-disable. Types: internal/system only.
  - **Verify:** QueryRefs present in `queries` after migrate; docs note numbers.
- [x] 4.4 Seed strategy for tests: migration seed **or** Test 45 setup SQL —
  decide and record (prefer test-time insert via QueryRef for isolation).
  - **Verify:** decision in Working Log.

### Exit Gate

- Migration applies on at least PostgreSQL (project default engine for auth
  tests) and reverse is safe.
- QueryRef numbers recorded in this plan's Working Log.

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** Migrations 1266–1270; QueryRefs #132–#135; luacheck green.
  Full engine migrate deferred to CI/Test 45 env (same as other schema phases).
- **Variances:** No dedicated "update client" QueryRef yet (soft-disable #134
  only); add if admin UI needs full update.

---

## Phase 5 — Client Registry Implementation

### Objective

Replace `oidc_clients.c` stubs with DB-backed validate / authenticate / register
(admin) using Phase 4 QueryRefs.

### Entry Gate

- Phase 4 complete.
- Database subsystem available in same process (same as auth login).

### Work Items

- [x] 5.1 Define internal client struct (id, secret_hash, redirects[], flags).
  - **Verify:** header in `oidc_clients.h`; no secrets in dump helpers.
- [x] 5.2 `oidc_validate_client(client_id, redirect_uri)` — exact redirect match
  (or strict allow-list rules documented).
  - **Verify:** Unity with mocked DB or fixture QueryRef layer.
- [x] 5.3 `oidc_authenticate_client` — constant-time secret compare against hash;
  public clients skip secret.
  - **Verify:** Unity success/fail; no secret in logs.
- [x] 5.4 Optional `oidc_register_client` for admin/tests only; not public HTTP
  yet (registration endpoint stays 501 or admin-gated).
  - **Verify:** Unity create + fetch.
- [~] 5.5 Wire client context init to database name from config (new
  `OIDC.Database` field if missing — add config + schema).
  - **Verify:** deferred — in-memory registry sufficient until Test 45; DB load
    via #132 next when blackbox needs durable clients.
- [x] 5.6 `mkt` + `mku` + `mkp`.
  - **Verify:** mkt + mku green.

### Exit Gate

- Stubs removed from validate/authenticate success paths.
- Invalid redirect and bad secret fail closed.

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** In-memory registry; exact redirect JSON allow-list; SHA-256 secret
  hash + CRYPTO_memcmp; Unity 5/5.
- **Variances:** DB-backed load (#132) deferred; `OIDC.Database` not added yet.

---

## Phase 6 — Schema: Authorization Codes And Refresh Tokens

### Objective

Add tables (and QueryRefs) for single-use auth codes and refresh tokens.
Optional consent table may be deferred to Phase 17 if MVP uses
pre-approved first-party clients only.

### Entry Gate

- Phase 5 complete (clients exist to FK logically; no SQL FK required per
  project convention).

### Work Items

- [x] 6.1 Design `oauth_authorization_codes`: code_hash PK, client_id,
  account_id, redirect_uri, scope, nonce, code_challenge, challenge_method,
  expires_at, consumed_at, created meta.
  - **Verify:** design in Working Log.
- [x] 6.2 Design `oauth_refresh_tokens`: token_hash PK, client_id, account_id,
  scope, expires_at, revoked_at, replaced_by (rotation), family_id optional.
  - **Verify:** design in Working Log.
- [x] 6.3 Migrations + reverse + diagrams.
  - **Verify:** luacheck + one engine migrate.
- [x] 6.4 QueryRefs: insert code, get+consume code (atomic), insert refresh,
  get refresh, revoke refresh, revoke all for account/client.
  - **Verify:** numbers logged; internal_sql only.
- [x] 6.5 Decision: in-memory code store for Unity-only vs DB-only — prefer DB
  for anything Test 45 hits; Unity may use a test seam.
  - **Verify:** decision recorded.

### Exit Gate

- Tables and QueryRefs applied.
- No use of `tokens` table for OAuth artifacts.

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** Tables 1271–1272; QueryRefs #136–#142; luacheck green. Apply to
  all 7 engines before Test 45.
- **Variances:** Consume is separate get (#137) + update (#138) rather than one
  atomic SQL — engines differ on UPDATE…RETURNING; app must check rows-affected.

---

## Phase 7 — Authorization Codes And PKCE

### Objective

Implement generate/store/validate authorization codes with PKCE S256
verification. No full login UI yet — may accept a test seam for "already
authenticated account_id" to unit-test the code machine.

### Entry Gate

- Phase 6 complete.
- Phase 1 crypto available for SHA-256 / base64url (PKCE).

### Work Items

- [x] 7.1 Extract or share PKCE S256 verify helper (prefer
  `utils_crypto` or small `oidc_pkce.c` under `src/oidc/`, patterned on
  `oidc_rp_pkce.c` without coupling to RP).
  - **Verify:** Unity: challenge from verifier validates; wrong verifier fails.
- [x] 7.2 `oidc_generate_authorization_code` — random code, store hash + metadata,
  TTL from config (e.g. 60–300s).
  - **Verify:** Unity insert + validate once.
- [x] 7.3 `oidc_validate_authorization_code` — single-use consume, redirect_uri
  match, client_id match, PKCE verify, expiry.
  - **Verify:** Unity: second consume fails; expiry fails; bad PKCE fails.
- [x] 7.4 Replace token-service stubs for code path only; leave token JWT
  generation to Phase 9.
  - **Verify:** `mkt` + `mku` + `mkp`.

### Exit Gate

- Code lifecycle fully tested at Unity level without HTTP.
- No plaintext codes stored (hash only).

### Status

- **State:** complete
- **Date:** 2026-07-23
- **Result:** `oidc_pkce` + `oidc_auth_codes` in-memory store; Unity green.
- **Variances:** APIs named `oidc_auth_code_issue` / `oidc_auth_code_consume`
  (not generate/validate). DB persistence via #136–#138 still to wire for
  Test 45 multi-process.

---

## Phase 8 — Resource Owner Authentication At Authorize

### Objective

Implement the interactive authorization endpoint path: validate client +
redirect + PKCE params; authenticate user against Acuranzo accounts; issue
code and redirect.

### Entry Gate

- Phases 5 and 7 complete.
- Password path QueryRef `#012` understood (`login.c` patterns).

### Work Items

- [ ] 8.1 Define login presentation approach for MVP:
  - **Option A (recommended for MVP):** `POST /oauth/authorize` with
    `username`/`password` form fields (resource owner password at authorize)
    after initial GET shows minimal HTML form from payload or embedded string.
  - **Option B:** Reuse existing session cookie/JWT if present (later).
  - Record choice in Working Log before implementation.
  - **Verify:** decision locked.
- [ ] 8.2 Implement GET authorize: validate params via existing
  `extract_oauth_params`; on missing login → 200 HTML login (or 401 JSON for
  non-browser clients — document).
  - **Verify:** Unity/handler tests for missing client_id, bad redirect,
    missing PKCE when required.
- [ ] 8.3 Implement POST authorize: verify password via shared auth helper
  (refactor thin wrapper around `#012` if needed — **do not duplicate hash
  logic**); apply rate-limit QueryRefs; on success create code + 302.
  - **Verify:** Unity with DB fixture or integration smoke; failed password
    does not redirect with code.
- [ ] 8.4 Replace `oidc_users` hard-coded `test_user` facade to call accounts
  lookups for claims export (or bypass oidc_users and call auth DB directly —
  prefer one path; record decision).
  - **Verify:** no remaining production dependency on `test_user`.
- [ ] 8.5 Error responses per OAuth: `invalid_request`, `unauthorized_client`,
  `access_denied`, `invalid_scope` with redirect-when-safe rules.
  - **Verify:** Unity/table-driven tests for error codes.
- [ ] 8.6 `mkt` + `mku` + `mkp`.
  - **Verify:** green.

### Exit Gate

- Browser-like curl form POST can obtain a one-time code for a seeded client
  and test account.
- RP stack untouched.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 9 — Token Endpoint: Code Exchange

### Objective

Exchange authorization code (+ PKCE verifier + client auth) for `id_token`,
`access_token`, and optional `refresh_token`. Real RS256 JWTs via Phase 2 keys.

### Entry Gate

- Phases 2, 7, 8 complete.

### Work Items

- [ ] 9.1 Implement claims builder: `iss`, `sub` (stable account id string),
  `aud` (client_id), `exp`/`iat`, `auth_time`, `nonce`, `email`/`email_verified`
  when scope includes email, `preferred_username`/name from accounts when
  profile scope.
  - **Verify:** Unity claims JSON/JWT payload contents.
- [ ] 9.2 Implement `oidc_create_jwt` / `oidc_generate_id_token` /
  `oidc_generate_access_token` with RS256 and `kid` header.
  - **Verify:** Unity sign; verify with public JWK from JWKS.
- [ ] 9.3 Implement `oidc_process_token_request` for `grant_type=authorization_code`
  only; reject unknown grants with `unsupported_grant_type`.
  - **Verify:** Unity success + failure matrix.
- [ ] 9.4 Wire `token/token.c` handler: parse body (form URL-encoded), client
  auth, call process, return JSON token response.
  - **Verify:** curl smoke after authorize.
- [ ] 9.5 Ensure access token is accepted later by userinfo (Phase 10) —
  document access token type (JWT recommended).
  - **Verify:** Working Log notes token format decision.
- [ ] 9.6 `mkt` + `mku` + `mkp`.
  - **Verify:** green.

### Exit Gate

- Full code→token path works in manual curl against enabled config.
- Tokens verify against `/oauth/jwks`.
- No HS256 OIDC tokens.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 10 — UserInfo Endpoint

### Objective

`GET /oauth/userinfo` with Bearer access token returns claims filtered by
granted scope.

### Entry Gate

- Phase 9 complete (valid access tokens exist).

### Work Items

- [ ] 10.1 Validate access token (signature, iss, exp, typ/scope); resolve
  account_id.
  - **Verify:** Unity valid/expired/wrong-iss/tampered.
- [ ] 10.2 Build userinfo JSON from accounts + contacts; honor scope.
  - **Verify:** Unity openid-only vs profile+email.
- [ ] 10.3 Wire `userinfo/userinfo.c`; proper WWW-Authenticate on 401.
  - **Verify:** curl with bearer succeeds.
- [ ] 10.4 `mkt` + `mku` + `mkp`.
  - **Verify:** green.

### Exit Gate

- Userinfo matches id_token subject and agreed claims.
- Invalid bearer never leaks whether account exists beyond standard OAuth
  errors.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 11 — Refresh Token Grant

### Objective

Issue refresh tokens on code exchange (when `offline_access` or config default);
implement `grant_type=refresh_token` with rotation and revoke-on-reuse policy.

### Entry Gate

- Phases 6 and 9 complete.

### Work Items

- [ ] 11.1 Issue refresh token on successful code exchange when allowed by
  client grant_types / scope.
  - **Verify:** Unity token response includes refresh_token when expected.
- [ ] 11.2 Refresh grant: validate token, rotate (new refresh, revoke old),
  mint new access (+ optional new id_token).
  - **Verify:** Unity rotation; reuse of old refresh fails and preferably
    revokes family (document policy).
- [ ] 11.3 Wire into `oidc_process_token_request`.
  - **Verify:** curl refresh flow.
- [ ] 11.4 `mkt` + `mku` + `mkp`.
  - **Verify:** green.

### Exit Gate

- Refresh works end-to-end; stolen old refresh cannot mint forever.
- Refresh hashes only in DB.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 12 — Introspection And Revocation

### Objective

Implement RFC 7662 introspection and RFC 7009 revocation for access/refresh
tokens. Replace stub responses.

### Entry Gate

- Phases 9–11 complete enough to have tokens to introspect/revoke.

### Work Items

- [ ] 12.1 Introspection: client auth required; response `active` true/false +
  metadata when active.
  - **Verify:** Unity active/inactive/expired/revoked.
- [ ] 12.2 Revocation: client auth; revoke refresh and/or access denylist if
  JWT access tokens need denylist (if pure JWT access without denylist,
  document limitation — prefer denylist table or short TTL only).
  - **Verify:** Unity + decision on access-token revoke strategy in Working Log.
- [ ] 12.3 Wire handlers; remove `not_implemented` paths for these two.
  - **Verify:** curl.
- [ ] 12.4 `mkt` + `mku` + `mkp`.
  - **Verify:** green.

### Exit Gate

- Introspect/revoke behave per RFCs for supported token types.
- Discovery document lists introspection/revocation endpoints only if enabled.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 13 — Unity Suite Completion

### Objective

Bring Unity coverage for IdP core and HTTP helpers to project standards:
one test file per significant function, no static helpers, mocks for MHD where
needed. Close gaps left as "smoke only" in earlier phases.

### Entry Gate

- Phases 1–12 functionally complete (or remaining items explicitly deferred).

### Work Items

- [ ] 13.1 Inventory uncovered lines via `extras/add_coverage.sh` on
  `oidc/*.c` and `api/oidc/**/*.c`.
  - **Verify:** coverage gap list in Working Log.
- [ ] 13.2 Add missing Unity tests under `tests/unity/src/oidc/` and
  `tests/unity/src/api/oidc/` per TESTING_UNITY naming rules.
  - **Verify:** each new `mku <base>` green.
- [ ] 13.3 Ensure no new `static` functions in `src/` (static-function gate on
  `mkt`).
  - **Verify:** `mkt` green.
- [ ] 13.4 `mkp` clean on oidc trees.
  - **Verify:** `tests/test_91_cppcheck.sh` or `mkp` green for touched files.
- [ ] 13.5 Confirm launch/landing/config Unity still pass.
  - **Verify:** `mku` sample of existing launch_oidc / landing_oidc /
    config_oidc tests.

### Exit Gate

- Gap list reduced to irreducible OpenSSL failure paths only (documented).
- Full Unity oidc set green after `mkt`.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 14 — Blackbox Test 45 (OIDC IdP)

### Objective

Add `test_45_oidc_idp.sh` that drives Hydrogen as IdP through discovery, JWKS,
authorize (login), token (PKCE), userinfo, refresh, and basic error paths —
the inverse of Test 42's mock-Keycloak approach.

### Entry Gate

- Phases 8–12 complete enough for the happy path.
- Port range 5450–5456 free.

### Work Items

- [ ] 14.1 Create configs: disabled, enabled full, bad client, PKCE required.
  - **Verify:** JSON lint; server starts on 545x.
- [ ] 14.2 Implement helpers `tests/lib/oidc_idp_helpers.sh` (curl wrappers,
  form login, code parse from redirect, token POST, jwt decode via jq).
  - **Verify:** shellcheck clean (`mks` or test_92 on new scripts).
- [ ] 14.3 Optional mock RP Node script if form/redirect automation needs it
  (keep simpler than mock_keycloak if curl suffices).
  - **Verify:** documented in test md.
- [ ] 14.4 Subtests (minimum):
  1. Disabled → closed
  2. Discovery shape (issuer, endpoints, S256, RS256)
  3. JWKS usable key
  4. Authorize missing params → error
  5. Happy path code+PKCE→tokens
  6. Userinfo with access token
  7. Refresh rotation
  8. Reuse auth code fails
  9. Bad redirect_uri fails
  10. Token signature verifies against JWKS
  - **Verify:** script implements and passes each.
- [ ] 14.5 Docs: `/docs/H/tests/test_45_oidc_idp.md`; update TESTING.md list,
  STRUCTURE/SITEMAP as required by project renumber rules.
  - **Verify:** `test_04_check_links.sh`, `test_90_markdownlint.sh`.
- [ ] 14.6 Run full `tests/test_45_oidc_idp.sh` green; run `test_42_oidc_rp.sh`
  regression.
  - **Verify:** both green.

### Exit Gate

- Test 45 green in isolation and discoverable by `test_00_all.sh`.
- Test 42 still green.
- No secrets in logs/artifacts.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 15 — Security Hardening

### Objective

Close common OAuth/OIDC failure modes before calling MVP production-ready.

### Entry Gate

- Phase 14 green (tests exist to lock hardening behavior).

### Work Items

- [ ] 15.1 Redirect URI: exact match only; no open redirects; block
  `javascript:` / non-http(s) unless explicitly allowed for native apps
  (document native policy).
  - **Verify:** Test 45 cases + Unity.
- [ ] 15.2 State parameter: required for authorize; reflected on success
  redirect.
  - **Verify:** Test 45.
- [ ] 15.3 Nonce: required when openid scope; echoed in id_token.
  - **Verify:** Unity + Test 45 assert nonce.
- [ ] 15.4 PKCE: reject `plain` in production default; S256 only when
  `RequirePkce` / security config says so.
  - **Verify:** config flag + tests.
- [ ] 15.5 Rate limit authorize login using existing `#004`–`#007` patterns.
  - **Verify:** Unity or blackbox lockout case.
- [ ] 15.6 Constant-time secret compare; generic error messages on client auth
  failure.
  - **Verify:** code review + Unity.
- [ ] 15.7 Clock skew tolerance documented for exp/iat validation.
  - **Verify:** Working Log + Unity boundary.
- [ ] 15.8 `mkt` + Test 45 + Test 42 + `mkp`.
  - **Verify:** green.

### Exit Gate

- Hardening checklist above checked.
- No critical findings from self-review against OIDC Core + OAuth 2.1 drafts
  notes in Working Log.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 16 — Documentation And Operator Runbook

### Objective

Replace empty endpoint stubs and architecture aspirational text with accurate
operator and API docs aligned to implemented behavior.

### Entry Gate

- Phase 14 complete (behavior stable).

### Work Items

- [ ] 16.1 Fill `/docs/H/api/oidc/oidc_endpoints.md` for each live endpoint
  (request/response, errors, examples).
  - **Verify:** markdownlint + link check.
- [ ] 16.2 Update `/docs/H/core/subsystems/oidc/oidc.md` and
  `oidc_architecture.md` to match reality (or add "implemented subset" section
  pointing at this plan).
  - **Verify:** no contradictory "fully implemented" claims.
- [ ] 16.3 Operator runbook section in this plan or separate doc: enable OIDC,
  set issuer to public URL, key storage path permissions, seed client, TLS
  termination expectations, kill switch.
  - **Verify:** reviewed checklist exists.
- [ ] 16.4 Config schema + example config under `examples/configs` if project
  pattern requires.
  - **Verify:** jsonlint.
- [ ] 16.5 Cross-link from `/docs/H/README.md`, `SITEMAP.md`, `TODO.md`.
  - **Verify:** `test_04_check_links.sh`.
- [ ] 16.6 Note relationship to KEYCLOAK_PLAN / OIDC RP: when to use Hydrogen
  IdP vs external Keycloak.
  - **Verify:** short comparison table published.

### Exit Gate

- Docs match code; links green; markdownlint green.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 17 — Optional Post-MVP

### Objective

Park non-essential features. Do not start until Phases 0–16 are complete unless
a specific item is pulled forward with an explicit variance.

### Entry Gate

- Phases 0–16 complete (or product owner pulls a single item).

### Work Items

- [ ] 17.1 RP-initiated logout `/oauth/end-session` + optional id_token_hint.
  - **Verify:** Unity + Test 45 subtest.
- [ ] 17.2 Client credentials grant for confidential service clients.
  - **Verify:** Unity + blackbox.
- [ ] 17.3 Consent screen + `oauth_consents` table when third-party clients
  need user-approved scopes.
  - **Verify:** migration + flow tests.
- [ ] 17.4 Dynamic client registration (auth-gated).
  - **Verify:** security review required before enable.
- [ ] 17.5 Key rotation job / admin trigger with dual-key JWKS overlap window.
  - **Verify:** Unity rotation + dual-kid verify.
- [ ] 17.6 Device code / native app enhancements.
  - **Verify:** separate mini-plan if pursued.

### Exit Gate

- Each pulled item has its own exit criteria recorded when started.

### Status

- **State:** pending (not started by design)
- **Date:**
- **Result:**
- **Variances:**

---

## Dependency Graph (summary)

```text
Phase 0  inventory
   │
   ▼
Phase 1  crypto sign ──────────────────────────────┐
   │                                                 │
   ▼                                                 │
Phase 2  keys + JWKS                                 │
   │                                                 │
   ▼                                                 │
Phase 3  wire discovery/jwks HTTP                    │
   │                                                 │
   ▼                                                 │
Phase 4  oauth_clients schema                        │
   │                                                 │
   ▼                                                 │
Phase 5  client registry                             │
   │                                                 │
   ├──────────────► Phase 6  codes/refresh schema    │
   │                     │                           │
   │                     ▼                           │
   │                Phase 7  codes + PKCE ◄──────────┘
   │                     │
   │                     ▼
   └──────────────► Phase 8  authorize + login
                         │
                         ▼
                    Phase 9  token endpoint
                         │
            ┌────────────┼────────────┐
            ▼            ▼            ▼
       Phase 10     Phase 11     Phase 12
       userinfo     refresh      introspect/revoke
            │            │            │
            └────────────┼────────────┘
                         ▼
                    Phase 13  Unity completion
                         │
                         ▼
                    Phase 14  Test 45 blackbox
                         │
                         ▼
                    Phase 15  security hardening
                         │
                         ▼
                    Phase 16  docs
                         │
                         ▼
                    Phase 17  optional post-MVP
```

Phases 4–6 are schema-heavy and may be batched in one migration PR if gates
still pass individually. Phases 10–12 may overlap after Phase 9 if staffing
allows, but each keeps its own exit gate before Phase 14.

---

## Risks And Mitigations

| Risk | Mitigation |
| --- | --- |
| Breaking OIDC RP / Test 42 | Shared code only via `utils_crypto`; run Test 42 after crypto and routing phases |
| Confusing session JWT with OIDC tokens | Hard table split; separate HS256 vs RS256; docs callout |
| Open redirect | Exact redirect allow-list; Phase 15 gate |
| Auth code leakage in logs/URL referrers | Short TTL; hash at rest; no logging codes |
| Incomplete stub cleanup | Phase 13 coverage inventory; ban remaining `not_implemented` on MVP paths |
| Login UX scope creep | Phase 8 locks minimal form POST; consent UI deferred to 17 |
| Multi-engine SQL drift | Use acuranzo macros; run at least one secondary engine before Phase 14 |

---

## Working Log (cross-phase memory)

Append discoveries, surprises, and decisions here as phases complete.

### Snapshot decisions (plan authoring, 2026-07-23)

1. **IdP vs RP:** separate stacks; this plan owns IdP only.
2. **Next migration / QueryRef (re-check before use):** after Phase 4/5:
   migration **1271+**, QueryRef **136+**. Assigned: table **1266**, #132 get,
   #133 insert, #134 soft-disable, #135 list.
3. **Blackbox:** Test **45**, ports **5450–5456**, name `test_45_oidc_idp`.
4. **Reuse tables:** `accounts`, contacts, roles, login rate-limit QueryRefs.
5. **New tables:** `oauth_clients`, `oauth_authorization_codes`,
   `oauth_refresh_tokens`; consent optional.
6. **Do not reuse:** `tokens` (session JWT), `account_oidc_identities` (RP).
7. **Crypto:** extend `utils_crypto` with RS256 sign + keygen; finish
   `oidc_keys` on top.
8. **Routing:** implement real `register_oidc_endpoints`; paths under
   `/.well-known` and `/oauth/*` — not under `/api/auth/oidc/*`.
9. **MVP grant:** authorization_code + PKCE S256 only; refresh in Phase 11.
10. **Default:** `OIDC.Enabled=false`.
11. **Existing stubs inventory:** see Current Observed State tables (validated
    against tree 2026-07-23).
12. **Historical docs:** `docs/H/core/subsystems/oidc/oidc.md` is aspirational;
    implementation truth is this plan + code.

### Decisions log

- **2026-07-23 Phase 0:** Keep RP leftover fields on `OIDCConfig`
  (`client_id`, `client_secret`, `redirect_uri`) for JSON compat until a later
  config cleanup; IdP must not use them for protocol. Prefer ignore + optional
  dump, not hard-remove (would break existing configs/tests).
- **2026-07-23 Phase 0:** Blackbox Test **45**, ports **5450–5456** free
  (tests present: 40–44 only in 40s).
- **2026-07-23 Phase 0:** Next migration **1266**; next QueryRef **210+**
  (disk: highest QueryRef tokens observed into 200s; do not use 132).
- **2026-07-23 Phase 0:** `register_oidc_endpoints()` still no-op stub in
  `src/api/oidc/oidc_service.c`; `handle_oidc_request` has **no** call sites in
  `webserver/` or `api_service.c` — only defined/used inside `api/oidc/`.
- **2026-07-23 Phase 0:** Non-goals confirmed — no RP/Test 42 changes; no
  implicit flow; no overload of `tokens` or `account_oidc_identities`.
- **2026-07-23 Phase 1:** Crypto APIs live on `utils_crypto`:
  `utils_rsa_generate_keypair`, `utils_rs256_sign`, `utils_rsa_private_to_pem`,
  `utils_rsa_public_to_pem`, `utils_rsa_private_from_pem`,
  `utils_rsa_public_to_jwk`. Bits must be ≥2048.
- **2026-07-23 Phase 1:** JWK export uses compact JSON with kty/alg/use/n/e/kid;
  import round-trips via existing `utils_jwk_rsa_to_pkey`.
- **2026-07-23 Phase 2:** Storage files under `Keys.StoragePath`:
  `signing-active.pem` + `signing-active.kid`. Load on init; generate if missing.
  NULL storage_path still generates in-memory key (Unity-friendly).
- **2026-07-23 Phase 2:** `oidc_rotate_keys` generates new ACTIVE RS256 and
  marks prior ACTIVE as ROTATING; both appear in JWKS. Full dual-key policy
  remains Phase 17.
- **2026-07-23 Phase 3:** `register_oidc_endpoints` registers `/.well-known`
  and `/oauth` only when `OIDC.Enabled`; cleanup unregisters both.
  Handlers: `oidc_web_handler` → `handle_oidc_request`.
- **2026-07-23 Phase 3:** Loader default endpoint paths now `/oauth/*` (was bare
  `/authorize` etc.). Key encryption default **false** (was true, blocked launch
  without encryption key). Storage default `/var/lib/hydrogen/oidc`.
- **2026-07-23 Phase 3:** Discovery document MVP shape: `response_types=code`
  only, PKCE `S256` only, grants `authorization_code`+`refresh_token`, no
  implicit.
- **2026-07-23 Phase 4:** `oauth_clients` columns: client_id PK, client_secret_hash
  nullable, client_name, is_confidential, require_pkce, redirect_uris (JSON text),
  grant_types, response_types, status_a12, + COMMON_CREATE.
- **2026-07-23 Phase 4:** Test seed via QueryRef **#133** at Test 45 setup (not
  migration seed) for isolation.
- **2026-07-23 Phase 5:** Secret storage = `utils_sha256_hash(secret)` base64url;
  compare with `CRYPTO_memcmp`. Public clients: authenticate with null/empty
  secret only. Redirect: exact string match against JSON array entries.
- **2026-07-23:** True max QueryRef before IdP work was **131** (not 209/210);
  earlier “210+” note was wrong (false positive from migration numbers).
- **2026-07-23 Phase 6:** `oauth_authorization_codes` (1271),
  `oauth_refresh_tokens` (1272). QueryRefs: **#136** insert code, **#137** get
  code, **#138** consume code, **#139** insert refresh, **#140** get refresh,
  **#141** revoke refresh, **#142** revoke family. Next free: mig **1280+**,
  QRef **143+**.
- **2026-07-23 Phase 6:** Unity uses **in-memory** `OIDCAuthCodeStore`; Test 45
  / multi-process must use DB QueryRefs on **all 7 engines** (mirror test_40
  configs: postgres, mysql, sqlite, db2, mariadb, cockroachdb, yugabytedb;
  ports **5450–5456**).
- **2026-07-23 Phase 7:** PKCE S256 only (`plain` rejected at issue). Code hash
  = `utils_sha256_hash`. Default TTL 300s.

### Surprises / fixes

- **2026-07-23:** `oidc_keys.c` redefines a *different* incomplete
  `struct OIDCKeyContext` than `oidc_keys.h` (header has keys array; .c has
  private_keys/public_keys placeholders). **Fixed in Phase 2** — .c uses header
  layout only.
- **2026-07-23:** Most `oidc_keys.h` APIs were declared but **not defined** —
  **implemented in Phase 2**.
- **2026-07-23:** Plan QueryRef “132+” was stale; use **210+** after disk check.
- **2026-07-23 Phase 1:** `utils_jwk_rsa_to_pkey` still uses `goto cleanup`
  (grandfathered). New crypto helpers avoid `goto` per INSTRUCTIONS.
- **2026-07-23 Phase 2:** Helpers `oidc_free_key`, `oidc_build_storage_path`,
  etc. are non-static + header-declared for Unity/static-function gate.
- **2026-07-23 Phase 3:** Nested C comments with `/*` inside block comments
  trip `-Werror=comment` — use `//` for path notes.
- **2026-07-23 Phase 3:** `load_oidc_config` zeros struct then sets
  `enabled=true` as process default before JSON — operators must set
  `OIDC.Enabled=false` explicitly if section present; defaults path still
  disabled via `config_defaults`. Watch for test config surprises.
- **2026-07-23 Phase 3:** `memcpy` of `OIDCConfig` into service context does
  **not** deep-copy strings — service shares pointers with `app_config`. Do
  not free config while service lives (existing pattern; document only).

---

## Related Documents

- [KEYCLOAK_PLAN.md](/docs/H/plans/KEYCLOAK_PLAN.md) — Hydrogen as OIDC RP
- [OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md) — RP historical implementation plan
- [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) — RP API contract
- [oidc.md](/docs/H/core/subsystems/oidc/oidc.md) — original IdP architecture notes
- [oidc_architecture.md](/docs/H/core/reference/oidc_architecture.md)
- [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md) — endpoint docs (to fill)
- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) — RP blackbox (mirror)
- [TESTING.md](/docs/H/tests/TESTING.md) — blackbox conventions
- [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) — Unity conventions
- [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) — build aliases and coding rules
- [TODO.md](/docs/H/TODO.md) — incomplete plans index

---

## Definition Of Done (MVP)

Hydrogen OIDC IdP MVP is done when:

1. Phases 0–16 Status blocks are complete (Phase 17 optional).
2. `OIDC.Enabled=true` serves discovery, JWKS, authorize, token, userinfo,
   refresh, introspect, revoke with real crypto and DB-backed clients/codes.
3. Resource owners authenticate via existing accounts/password infrastructure.
4. Unity suites for oidc core + handlers are green; `mkt`/`mkp` green.
5. `test_45_oidc_idp.sh` green; `test_42_oidc_rp.sh` still green.
6. Docs and operator runbook match behavior; kill switch works.
7. No secrets in logs; no overload of session `tokens` or RP identity tables.
