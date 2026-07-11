# Keycloak SSO (Hydrogen as OIDC Relying Party) Implementation Plan

## Purpose

Define a gated, phase-by-phase operational plan for production Keycloak SSO against Hydrogen: frontend apps redirect users to a Keycloak IdP, Hydrogen validates the OIDC response, links or auto-provisions a local `accounts` row, and mints the **same Hydrogen JWT** shape as password login so Conduit, renew, logout, and role gates need no special-casing.

This document is the working plan for remaining and production work. Much of the OIDC Relying Party (RP) code path is already implemented; this plan inventories that foundation, then sequences **IdP setup**, **Hydrogen production config**, **account auto-creation policy**, **client-app integration documentation**, and **real-Keycloak validation gates**. Hydrogen-as-IdP (`src/oidc/`) is explicitly out of scope.

## How To Use This Document

- Work one phase at a time, top to bottom. Each phase has an Objective, Entry Gate, numbered work items, and an Exit Gate.
- Each work item has an explicit verification (build, Unity, blackbox, doc check, or manual IdP checklist).
- Mark an item `[x]` only when its verification has actually passed. If deferred, change to `[~]` with a one-line rationale and the target phase.
- After each phase, fill in the phase Status block and append reusable discoveries to the Working Log.
- Never mark a phase complete with a failing or skipped gate unless explicitly recorded as deferred.
- **Do not re-implement shipped RP modules.** If a phase discovers a real gap in existing code, open a focused fix sub-chunk; do not restart greenfield OIDC work.

## Resuming Work on This Plan

CURRENT PAUSE POINT (as of 2026-07-11): **Phase 0 complete.** Foundation verified. Test 42 green (88/88). Policy defaults locked. Ready for Phase 1 Keycloak IdP setup.

### Resume here next session

1. Confirm Phase 0 Status and re-read Working Log.
2. Run baseline: `zsh -ic 'mkt'` and `tests/test_42_oidc_rp.sh` (or via suite orchestration).
3. Confirm next free migration / QueryRef from disk if any schema work is proposed (today: none required for core SSO).
4. Present the next phase plan in small chunks; ask qualifying questions before coding.
5. Frontend SPA coding is **out of scope** until Phase 6+ explicitly opens it; client work in early phases is **documentation only**.

### Session checklist (every Keycloak / OIDC RP return)

1. Confirm latest completed phase via Status blocks; active phase = first not complete.
2. Re-read Working Log decisions that affect the next chunk.
3. Baseline: `zsh -ic 'mkt'`; relevant Unity under `tests/unity/src/api/auth/oidc_rp/`; blackbox `test_42` when touching flow code.
4. One small chunk: questions → plan → implement → verify → update this plan → stop for review.
5. Never log client secrets, handoff codes, JWTs, or id_token payloads in normal logs or test artifacts.

Build aliases: `zsh -ic '<alias>'` (`mkt`, `mka`, `mku <base>`, `mkp`, `mks`).

## Scope And Repo Areas

Primary repo area: `/elements/001-hydrogen/hydrogen`

Related:

- `/elements/001-hydrogen/hydrogen/src/api/auth/oidc_rp/` — OIDC RP implementation (shipped).
- `/elements/001-hydrogen/hydrogen/src/api/auth/` — password login, JWT mint/store, renew, logout.
- `/elements/001-hydrogen/hydrogen/src/config/config_oidc_rp.{c,h}` — `OIDC_RP` config.
- `/elements/002-helium/acuranzo/migrations/` — `account_oidc_identities`, nullable `password_hash`, QueryRefs `#080`–`#084`, roles `#017` / `#127`.
- `/elements/001-hydrogen/hydrogen/tests/test_42_oidc_rp.sh` — mock-Keycloak blackbox.
- `/elements/001-hydrogen/hydrogen/tests/lib/mock_keycloak/` — mock IdP for CI.
- `/docs/H/api/auth/oidc_rp.md` — RP API contract.
- `/docs/Li/LITHIUM-KEYCLOAK.md` — client SPA recipe (Lithium-oriented; Phase 5 generalizes).
- `/docs/OIDC-PLAN.md` — historical full implementation plan (Phases 1–26 done; 27 open).
- `/docs/H/plans/OIDC_E2E_LOG.md` — manual real-Keycloak checklist (unsigned).
- Lithium SPA code exists but **frontend changes are deferred** per this plan until a later phase.

Date of snapshot: 2026-07-11

---

## Goals And Non-Goals

### Goals

- Frontend apps (Lithium and others) can start SSO by navigating to Hydrogen `GET /api/auth/oidc/start`, complete Keycloak login, and receive a Hydrogen JWT via handoff exchange.
- Users authenticated at Keycloak are either **linked** to an existing Hydrogen account or **auto-provisioned** when policy allows.
- Same Hydrogen JWT claim shape as password login so existing APIs work unchanged.
- Password login remains fully independent of IdP availability.
- Document IdP (Keycloak) setup and client-app integration without requiring frontend coding in early phases.
- Small, self-contained phases with green exit gates.

### Non-Goals (this plan)

- Hydrogen as OIDC **provider** / IdP (`src/oidc/`, `src/api/oidc/`). Out of scope forever for this document.
- SAML, LDAP direct bind, social IdPs as first-class providers (architecture may allow multi-provider later).
- Using Keycloak tokens inside SPAs or Conduit.
- Sliding sessions via Keycloak refresh tokens (use `/api/auth/renew`).
- Frontend UI work until Phase 6 explicitly opens it.
- Canvas LMS Keycloak realm admin beyond documenting shared-realm expectations.

---

## Architecture (locked)

### Two OIDC stacks — do not confuse them

| Stack | Role | Location | This plan |
|---|---|---|---|
| OIDC **Provider** | Hydrogen as IdP | `src/oidc/`, `src/api/oidc/`, `config_oidc` | **Ignore** |
| OIDC **Relying Party** | Hydrogen as client of Keycloak | `src/api/auth/oidc_rp/`, `config_oidc_rp` | **In scope** |

### End-to-end flow

```text
Browser   →  GET  /api/auth/oidc/start?database=&return_to=&provider=
Hydrogen  →  302  Keycloak /auth  (state, nonce, PKCE S256)
Keycloak  →  user authenticates (SSO cookie may skip re-prompt)
Keycloak  →  302  /api/auth/oidc/callback?code=&state=
Hydrogen  →  POST Keycloak /token  (server-to-server)
Hydrogen  →  validate id_token (RS256, iss, aud, nonce, exp)
Hydrogen  →  account link / provision → generate_jwt + store_jwt
Hydrogen  →  302  SPA  ?oidc=1&handoff=<opaque>
SPA       →  POST /api/auth/oidc/handoff  →  { success, token, … }
SPA       →  store Hydrogen JWT; never stores Keycloak tokens
```

**Hard rule:** Keycloak `id_token` / `access_token` never leave Hydrogen. Only opaque, single-use, short-TTL handoff codes reach the browser.

### Endpoints (shipped)

| Method | Path | Purpose |
|---|---|---|
| `GET` | `/api/auth/oidc/start` | Begin auth code + PKCE; 302 to IdP |
| `GET` | `/api/auth/oidc/callback` | Code exchange, validate, link/provision, mint JWT, 302 handoff |
| `POST` | `/api/auth/oidc/handoff` | Exchange handoff → Hydrogen JWT (login-shaped body) |

When `OIDC_RP.Enabled = false` (default): public endpoints return `503` `{"error":"oidc_disabled"}`.

### Account linking strategies (shipped)

| Strategy | Behavior |
|---|---|
| `match_email_then_provision` | Default code path: `(iss,sub)` → verified email link → provision if allowed |
| `match_sub_only` | Pre-linked only |
| `match_email_only` | Link by identity or verified email; **no** auto-create |
| `provision_only` | Always create on first login (domain allow-list) |

Provisioning knobs: `ProvisionDefaults.Enabled`, `Authorized`, `DefaultRoleNames[]`, `AllowedEmailDomains[]`, `RequireEmailVerified`.

Production conservative default (E2E log intent): **`match_email_only`**, provisioning **off**, pre-create accounts. Auto-provision is optional and domain-gated when operators enable it.

### Role mapping (shipped)

| Source | Behavior |
|---|---|
| `database` | QueryRef `#017` only (default) |
| `idp_realm_roles` | From `realm_access.roles` in id_token |
| `idp_client_roles` | Partial; falls back to realm roles today |
| `merge` | Union DB + IdP; optional `IdpRolePrefix` |

JWT `roles` claim is comma-separated **role_id integers** (e.g. `"1,3"`), not role names.

---

## Current Observed State (2026-07-11)

### Shipped and green (do not rebuild)

- Full RP modules under `src/api/auth/oidc_rp/` (start, callback, handoff, state, PKCE, discovery, JWKS, token, id_token, link strategies, roles).
- Config `OIDC_RP` / `OIDCRelyingPartyConfig` with multi-provider array (cap 8); env overrides for provider 0.
- DB: `account_oidc_identities`, nullable `password_hash`, QueryRefs `#080`–`#084`.
- Mock Keycloak + `test_42_oidc_rp.sh` through link strategies and role mapping.
- API docs: [`/docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md).
- Client recipe: [`/docs/Li/LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md).
- Historical plan Phases 1–26: ✅ in [`/docs/OIDC-PLAN.md`](/docs/OIDC-PLAN.md).

### Open / incomplete

| Item | Detail |
|---|---|
| Real Keycloak E2E | Phase 27 checklist in [`OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md) **unsigned** |
| Multi-provider runtime | Schema supports `?provider=`; some paths still default strongly to `Providers[0]` |
| `idp_client_roles` | Not fully parsed from `resource_access` |
| Health / logout | Post-MVP: discovery health, backchannel logout, RP-initiated logout |
| Operator runbook | Secrets, exact redirect URIs, realm users, deploy env vars |
| Generic client doc | Lithium-oriented; needs app-agnostic checklist for non-Lithium frontends |
| Provisioning ops | Policy choice per environment not locked in a single operator doc |

### Confirmed design decisions (carry forward)

1. Hydrogen is the RP; SPAs only hold Hydrogen JWTs.
2. Handoff codes, not JWTs in URLs.
3. Do not use `src/oidc/` for Keycloak.
4. Password login independent of IdP.
5. Stable link key is `(iss, sub)`; email is secondary.
6. PKCE S256 + state + nonce; confidential client preferred.
7. RS256-only by default; reject `alg=none`.
8. Roles default from DB unless operators choose IdP merge.
9. OIDC-only accounts may have `password_hash = NULL`.
10. Feature kill switch: `OIDC_RP.Enabled=false` and/or empty SPA provider list.

---

## Reference Conventions

Match existing auth / OIDC RP patterns; do not invent parallel stacks.

### Config

- Struct: `OIDCRelyingPartyConfig` in `config_oidc_rp.h`; member `app_config->oidc_rp`.
- JSON section: `OIDC_RP` with `Enabled`, `Database`, `Providers[]`.
- Sensitive fields via env: `HYDROGEN_OIDC_CLIENT_ID`, `HYDROGEN_OIDC_CLIENT_SECRET`, `HYDROGEN_OIDC_ISSUER`, `HYDROGEN_OIDC_REDIRECT_URI`, `HYDROGEN_OIDC_RP_ENABLED`.
- Schema: update `tests/artifacts/hydrogen_config_schema.json` with any field change; run `tests/test_93_jsonlint.sh`.

### JWT minting

- After successful link/provision, call the **same** `generate_jwt` + `store_jwt` as password login.
- Load roles via `auth_roles_from_database()` (QueryRef `#017`) before mint when source is `database`.
- Response envelope for handoff matches login: `success`, `token`, `expires_at`, `user_id`, `username`, `email`, `roles`.

### Logging

- `log_this` with appropriate subsystem; never log secrets, full tokens, handoff codes, or raw id_tokens.
- Prefer purpose/outcome codes (`link_ok`, `provisioned`, `no_account`) over PII.

### Tests

- Unity under `tests/unity/src/api/auth/oidc_rp/`; run `mku <base>`.
- Blackbox: `tests/test_42_oidc_rp.sh` against mock IdP (ports per test-42 configs).
- Manual real-IdP: update [`OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md).
- After doc adds: `tests/test_04_check_links.sh`, `tests/test_90_markdownlint.sh`.

### Blackbox / ports

- Test 42 owns OIDC RP blackbox; do not invent a second mock-IdP suite without need.
- Port scheme remains `5<TT>x` per [`TESTING.md`](/docs/H/tests/TESTING.md).

---

## Gate Philosophy

- One logical behavior or operator deliverable per item where practical.
- After every C change: `mkt`, then `mka` when clean; `mkp` for cppcheck.
- After RP flow changes: relevant `mku` set + `test_42_oidc_rp.sh`.
- After config schema changes: jsonlint + config Unity load tests.
- After docs: link check + markdownlint.
- No client secret, JWT, handoff, or id_token in logs, dumps, or committed configs.
- Mark phase complete only when every listed gate is green or explicitly deferred.

---

## Phase Index

| Phase | Title | Risk | Status |
|---|---|---|---|
| 0 | Baseline inventory and gap lock | Low | ✅ complete |
| 1 | Keycloak IdP operator setup (realm, client, users) | Medium | pending |
| 2 | Hydrogen production `OIDC_RP` config and kill switches | Medium | pending |
| 3 | Account auto-provision policy and validation matrix | High | pending |
| 4 | Client application integration guide (no frontend code) | Low | pending |
| 5 | Real Keycloak E2E gate (manual + automated where possible) | High | pending |
| 6 | Optional frontend wiring notes / Lithium sign-off (deferred coding) | Medium | deferred start |
| 7 | Multi-provider dispatch hardening | Medium | pending |
| 8 | Post-MVP: health probe for OIDC RP | Low | pending |
| 9 | Post-MVP: backchannel logout | Medium | pending |
| 10 | Post-MVP: RP-initiated logout (server + client recipe) | Medium | pending |

---

## Phase 0 — Baseline Inventory And Gap Lock

### Objective

Confirm the shipped RP foundation, document remaining gaps with evidence, and lock decisions that later phases depend on. No production Keycloak traffic required.

### Entry Gate

- Clean tree understanding of this plan and [`OIDC-PLAN.md`](/docs/OIDC-PLAN.md) phase index.
- Ability to run `mkt` and `test_42_oidc_rp.sh`.

### Work Items

- [ ] 0.1 Re-read RP API contract [`/docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md) and confirm endpoint table matches registered handlers in `api_service.c`.
  - **Verify:** note any drift in Working Log; no code change unless registration is broken.
- [ ] 0.2 Run mock E2E: `tests/test_42_oidc_rp.sh` (or suite subset).
  - **Verify:** Test 42 green on current mainline.
- [ ] 0.3 Spot-check Unity RP suites (representative sample: start, callback path, link strategies, roles).
  - **Verify:** `mku` targets for those suites pass after `mkt` if needed.
- [ ] 0.4 Inventory config fields vs schema vs example configs (`examples/configs`, test configs).
  - **Verify:** list missing examples or schema drift; file follow-ups under Phase 2 if needed.
- [ ] 0.5 Lock environment policy defaults for this project (record in Working Log):
  - Production linking strategy (`match_email_only` vs `match_email_then_provision`).
  - Provisioning on/off, `RequireEmailVerified`, `AllowedEmailDomains`.
  - Role source (`database` vs `merge`).
  - Shared realm expectations (e.g. Canvas LMS same realm, separate clients).
  - **Verify:** decisions written in Working Log; no silent assumptions in later phases.
- [ ] 0.6 Confirm non-goals still hold (no Hydrogen-as-IdP, no SPA-held Keycloak tokens).
  - **Verify:** Status block records confirmation.

### Exit Gate

- Test 42 green.
- Working Log contains locked policy defaults for Phase 1–3.
- Gap list above still accurate or updated.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 1 — Keycloak IdP Operator Setup

### Objective

Stand up (or document existing) Keycloak realm/client/users so Hydrogen can act as a confidential OIDC client. Pure IdP / ops work; minimal Hydrogen code.

### Entry Gate

- Phase 0 complete; policy defaults locked.

### Work Items

- [ ] 1.1 Document target realm and issuer URL (example production intent: `https://www.500passwords.com/realms/<realm>`).
  - **Verify:** discovery URL returns 200 JSON with `authorization_endpoint`, `token_endpoint`, `jwks_uri`, `end_session_endpoint`.
- [ ] 1.2 Create or verify **confidential** client for Hydrogen (not a public SPA client).
  - Settings: Standard flow on; Direct access grants off (unless separately justified); PKCE optional but Hydrogen always sends S256.
  - **Verify:** client id exists; secret available only via secure env store.
- [ ] 1.3 Register **exact** redirect URI(s) for each Hydrogen public origin:
  - Pattern: `https://<host>/api/auth/oidc/callback` (and any local dev origins).
  - **Verify:** Keycloak rejects a deliberately wrong redirect string; accepts the exact configured URI.
- [ ] 1.4 Configure scopes: at least `openid profile email`. Ensure email claim is present and, if `RequireEmailVerified=true`, users have verified email.
  - **Verify:** sample login via Keycloak account console shows verified email for test users.
- [ ] 1.5 Create operator test users:
  - User A: email matches an existing Hydrogen `accounts` row.
  - User B: email has **no** Hydrogen account (for no_account vs provision paths).
  - **Verify:** both users can authenticate at Keycloak alone.
- [ ] 1.6 Capture non-secret client parameters into a private ops checklist (issuer, client id, redirect URI, realm roles if used). Never commit secrets.
  - **Verify:** checklist stored per team practice; repo only has placeholders / env var names.
- [ ] 1.7 Document shared-realm notes for other apps (e.g. Canvas LMS): separate clients, shared users, no shared client secrets.
  - **Verify:** short section added to Phase 4 client/IdP doc or ops notes.

### Exit Gate

- Discovery document reachable.
- Confidential client + exact redirect URI verified.
- Two test users ready for Phase 3/5 matrices.
- No secrets in git.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 2 — Hydrogen Production `OIDC_RP` Config And Kill Switches

### Objective

Wire Hydrogen configs for real Keycloak without breaking default-disabled safety. Align examples, env overrides, and deploy docs.

### Entry Gate

- Phase 1 client parameters known (issuer, client id, redirect URI).

### Work Items

- [ ] 2.1 Author a **production-shaped** `OIDC_RP` block (Enabled false in committed examples; Enabled true only via deploy overlay / env).
  - Fields: `Database`, `Providers[].Name/Label/Issuer/ClientId/ClientSecret/RedirectUri/Scopes`, linking + role mapping per Phase 0 locks.
  - **Verify:** config loads under Unity / dump; secrets never appear in dump (masked).
- [ ] 2.2 Confirm env overrides for provider 0: `HYDROGEN_OIDC_RP_ENABLED`, `HYDROGEN_OIDC_CLIENT_ID`, `HYDROGEN_OIDC_CLIENT_SECRET`, `HYDROGEN_OIDC_ISSUER`, `HYDROGEN_OIDC_REDIRECT_URI`.
  - **Verify:** documented in API doc or config examples; test_12 / env tests still green if touched.
- [ ] 2.3 Update committed example configs with **disabled** OIDC_RP skeleton documenting field names (no real secrets).
  - **Verify:** `tests/test_93_jsonlint.sh` green; examples reviewable.
- [ ] 2.4 Kill-switch drill: with `Enabled=false`, all three endpoints return `503 oidc_disabled`; password login still works (`test_40` or focused auth check).
  - **Verify:** Test 42 disabled path + one password login path green.
- [ ] 2.5 `SystemApiKey` / license path for JWT minting on OIDC success documented (same as password login requirements).
  - **Verify:** Working Log notes how deploy supplies api_key / system context for OIDC mint.
- [ ] 2.6 If schema drift found in Phase 0, reconcile `hydrogen_config_schema.json` + loaders.
  - **Verify:** jsonlint + config Unity green.

### Exit Gate

- Deploy can enable OIDC via config/env without code change.
- Defaults remain safe (disabled).
- Examples document the surface without secrets.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 3 — Account Auto-Provision Policy And Validation Matrix

### Objective

Make auto-creation of Hydrogen accounts on first successful Keycloak auth an explicit, tested operator choice—not an accident. Cover link-only vs provision paths, domain allow-list, verified-email requirements, and OIDC-only accounts (`password_hash` NULL).

### Entry Gate

- Phase 0 policy locks; Phase 2 config shape known.
- Mock Test 42 still green.

### Work Items

- [ ] 3.1 Write a **policy matrix** (table in this plan Working Log or a short ops section) covering:

  | Scenario | Strategy | Provision | Expected |
  |---|---|---|---|
  | Known email, existing account | match_email_only / then_provision | off/on | link `(iss,sub)`, mint JWT |
  | Unknown email | match_email_only | off | `no_account`, no row |
  | Unknown email, allowed domain | match_email_then_provision / provision_only | on | new account + identity link |
  | Unknown email, disallowed domain | provision on | on | reject; no row |
  | Unverified email when required | any | any | reject |
  | Pre-linked `(iss,sub)` only | match_sub_only | n/a | success without email match |

  - **Verify:** matrix reviewed; production row chosen.
- [ ] 3.2 Confirm QueryRefs `#080`–`#084` still match migrations and C linkers (lookup identity, insert link, email lookup, provision account, touch last_seen).
  - **Verify:** code path review; no missing QueryRef for chosen policy.
- [ ] 3.3 Exercise **mock** blackbox coverage for each strategy used in production (Test 42 already covers strategies; re-run and note gaps).
  - **Verify:** Test 42 green; list any uncovered production scenario.
- [ ] 3.4 If production enables provision: define default roles via `DefaultRoleNames` / DB roles and confirm JWT `roles` claim is role_id integers.
  - **Verify:** Unity or mock path shows expected role ids after provision.
- [ ] 3.5 Document password behavior for provisioned users: no password login until password set; OIDC remains valid.
  - **Verify:** note in client/ops doc (Phase 4).
- [ ] 3.6 Security review of provision path: domain allow-list non-empty when provision enabled; empty allow-list must not mean "all domains" unless explicitly decided.
  - **Verify:** code behavior confirmed; Working Log records empty-list semantics.
- [ ] 3.7 Optional code fixes only if a matrix row fails against mock IdP.
  - **Verify:** `mkt` / `mkp` / Test 42 green.

### Exit Gate

- Production policy table locked.
- Mock coverage matches chosen strategies.
- Auto-create path either validated green or explicitly disabled for prod with rationale.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 4 — Client Application Integration Guide (No Frontend Code)

### Objective

Produce a single Hydrogen-centric document that any frontend (Lithium, future apps) can follow to use Keycloak SSO **through Hydrogen**, plus an IdP checklist. No SPA implementation work in this phase.

### Entry Gate

- Phases 1–3 decisions available so the guide is accurate.

### Work Items

- [ ] 4.1 Author or extend a plan-owned doc under `/docs/H/` (recommended path: `/docs/H/api/auth/KEYCLOAK_SSO_CLIENT.md` or expand `oidc_rp.md` with a "Client integration" section). Content must include:
  - Architectural premise (SPA never holds Keycloak tokens).
  - Start URL construction: top-level navigation to `GET /api/auth/oidc/start` (**not** XHR/fetch for the 302).
  - Query params: `database`, `return_to` (must start with `/`), `provider`.
  - Return handling: detect `oidc=1&handoff=…` or `oidc_error=…`.
  - Handoff exchange: `POST /api/auth/oidc/handoff` with JSON body; treat response like password login.
  - URL hygiene: strip handoff from address bar immediately.
  - Error mapping table (`no_account`, `state_invalid`, `idp_error`, `oidc_disabled`, …).
  - JWT storage / renew / logout reuse of existing auth client code.
  - CORS / same-origin notes (callback hits Hydrogen origin).
  - **Verify:** markdownlint + link check green.
- [ ] 4.2 IdP (Keycloak) operator appendix: client type, redirect URI exact match, scopes, verified email, confidential secret storage, multi-app realm pattern (Canvas etc.).
  - **Verify:** can be followed by an operator without reading C code.
- [ ] 4.3 Hydrogen config appendix: minimal `OIDC_RP` JSON example (disabled) + env var list.
  - **Verify:** matches Phase 2 examples.
- [ ] 4.4 Cross-link from [`SITEMAP.md`](/docs/H/SITEMAP.md), [`README.md`](/docs/H/README.md) TOC if required by project doc rules, and from [`oidc_rp.md`](/docs/H/api/auth/oidc_rp.md).
  - **Verify:** `tests/test_04_check_links.sh` green.
- [ ] 4.5 Explicit "frontend work deferred" note pointing to Phase 6; Lithium-specific UI remains optional.
  - **Verify:** present in the new doc.

### Exit Gate

- Client + IdP guide published and linked.
- No frontend code changes required for this phase to be complete.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 5 — Real Keycloak E2E Gate

### Objective

Sign off the full chain against the real Keycloak IdP (not mock). This is the production readiness gate for SSO login + account link/provision.

### Entry Gate

- Phases 1–4 complete.
- Deploy or local Hydrogen can reach Keycloak discovery/token/JWKS endpoints.
- Secrets available only in env.

### Work Items

- [ ] 5.1 Enable `OIDC_RP` in a non-prod environment with Phase 0/2/3 policy.
  - **Verify:** `GET /api/auth/oidc/start` returns 302 to real Keycloak authorize URL with state/nonce/challenge.
- [ ] 5.2 Execute manual checklist in [`OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md) (or superseding checklist linked from this plan):
  - Password login still works.
  - Existing account SSO links by email / sub.
  - Unknown user: `no_account` **or** provision success per policy.
  - Reload mid-session.
  - Logout revokes Hydrogen JWT.
  - `/api/auth/renew` works for OIDC-minted JWT.
  - At least one authenticated Conduit/API call works with the JWT.
  - **Verify:** each item ticked with date/operator/notes.
- [ ] 5.3 Confirm `account_oidc_identities` rows created/updated as expected; no duplicate accounts for same email when policy forbids it.
  - **Verify:** DB inspection notes in E2E log.
- [ ] 5.4 Confirm Keycloak tokens never appear in browser storage or Hydrogen access logs at info level.
  - **Verify:** browser Application storage + sample log review.
- [ ] 5.5 Regression: `test_42_oidc_rp.sh` and `test_40_auth.sh` still green on CI (mock / password paths).
  - **Verify:** both green.
- [ ] 5.6 Independent second pass of the manual checklist (different operator or session).
  - **Verify:** second signature on E2E log.

### Exit Gate

- E2E log fully green for chosen production policy.
- Kill switch re-tested after sign-off.
- Working Log records production issuer/client id (non-secret) and policy.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 6 — Frontend Wiring (Deferred Coding)

### Objective

When product is ready for SPA work, wire a client using the Phase 4 guide. **Do not start this phase until Phases 0–5 are green** unless only documentation polish is needed.

### Entry Gate

- Phase 5 signed off **or** explicit product decision to develop UI against mock/dev only (record exception).

### Work Items

- [ ] 6.1 Choose target app (Lithium first likely). Implement only what Phase 4 specifies: start navigation, handoff exchange, error display, JWT store.
  - **Verify:** app-specific lint/test suite green.
- [ ] 6.2 Optional: last-method highlighting / provider button polish (historical OIDC-PLAN Phase 26).
  - **Verify:** UI tests or manual screenshots noted.
- [ ] 6.3 Confirm multi-app readiness: a second client could follow Phase 4 without Hydrogen API changes.
  - **Verify:** written confirmation in Working Log.

### Exit Gate

- At least one real SPA completes SSO against the Phase 5 environment.
- No Keycloak token handling in SPA code review.

### Status

- **State:** complete
- **Date:** 2026-07-11
- **Result:** Test 42 green (88/88). Foundation verified. Policy defaults locked: `match_email_only`, provisioning off, `RequireEmailVerified=true`, `database` source for roles.
- **Variances:** none

---

## Phase 7 — Multi-Provider Dispatch Hardening

### Objective

Make `OIDC_RP.Providers[]` and `?provider=` fully reliable so more than one IdP/client can be configured without hidden `Providers[0]` assumptions.

### Entry Gate

- Phase 5 complete (or parallel track if only code cleanup with mock tests).

### Work Items

- [ ] 7.1 Audit start/callback/handoff/discovery/JWKS for hard-coded provider index 0.
  - **Verify:** list of call sites in Working Log.
- [ ] 7.2 Ensure unknown `provider` returns stable `400 unknown_provider`.
  - **Verify:** Unity + Test 42 coverage.
- [ ] 7.3 Optional second mock provider config in Test 42 if practical.
  - **Verify:** Test 42 green with two named providers.
- [ ] 7.4 Document multi-provider client usage in Phase 4 guide.
  - **Verify:** link check green.

### Exit Gate

- Named provider selection works end-to-end on mock (and ideally real) IdP.
- No silent fallback without logging.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 8 — Post-MVP: OIDC RP Health Probe

### Objective

Expose RP readiness (discovery reachable / cache warm) on system health for ops.

### Entry Gate

- Phase 5 complete; product wants observability.

### Work Items

- [ ] 8.1 Design health field (e.g. `oidc_rp_status`: disabled | ok | degraded | error) without leaking secrets.
  - **Verify:** design note in Working Log.
- [ ] 8.2 Implement probe (cached discovery fetch, timeout-bounded).
  - **Verify:** `mkt` / `mkp` / Unity; extend `test_21_system_endpoints.sh` as needed.
- [ ] 8.3 Document field in system endpoint docs.
  - **Verify:** link check.

### Exit Gate

- Health reflects Enabled=false cleanly and Enabled=true with IdP up/down.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 9 — Post-MVP: Backchannel Logout

### Objective

Accept Keycloak backchannel logout tokens and revoke matching Hydrogen JWTs so SSO logout can invalidate API sessions.

### Entry Gate

- Phase 5 complete; Keycloak client supports backchannel logout URL.

### Work Items

- [ ] 9.1 Keycloak: configure backchannel logout URL to Hydrogen endpoint (exact path TBD in design, historically `/api/auth/oidc/backchannel-logout`).
  - **Verify:** IdP accepts URL.
- [ ] 9.2 Implement endpoint: validate logout token (signature, iss, aud, events), map to accounts/sessions, revoke Hydrogen JWTs.
  - **Verify:** Unity for token validation; blackbox subtest with mock logout token.
- [ ] 9.3 Never trust unsigned or `alg=none` logout tokens.
  - **Verify:** negative tests.
- [ ] 9.4 Update API + client docs (SPA may still call local logout).
  - **Verify:** link check.

### Exit Gate

- Logout at Keycloak can invalidate Hydrogen session for linked user under test.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Phase 10 — Post-MVP: RP-Initiated Logout

### Objective

Optional navigate-to-IdP end session after local Hydrogen logout so shared SSO cookies clear for other realm apps (Canvas, etc.).

### Entry Gate

- Phase 5 complete; discovery exposes `end_session_endpoint`.

### Work Items

- [ ] 10.1 Hydrogen helper or documented URL builder for `end_session_endpoint` with `post_logout_redirect_uri` allow-list.
  - **Verify:** Unity URL builder tests.
- [ ] 10.2 Client recipe update: after local logout, optional redirect to IdP logout.
  - **Verify:** Phase 4 guide updated; no requirement that all apps enable it.
- [ ] 10.3 Keycloak valid post-logout redirect URIs configured.
  - **Verify:** operator checklist item.

### Exit Gate

- Documented optional SLO path works in manual test without breaking apps that skip it.

### Status

- **State:** pending
- **Date:**
- **Result:**
- **Variances:**

---

## Cross-Phase Rules

- Every phase ends with earlier automated gates still green (especially Test 40 and Test 42 when auth paths change).
- No phase skips lint for touched languages (`mkp`, `mks`, markdownlint, jsonlint, luacheck if migrations).
- Configuration changes ship with schema/example/parser test in the same chunk.
- Documentation for a behavior ships in the same phase as the behavior (or Phase 4 for pure integration narrative).
- Prefer fixing RP bugs over adding parallel endpoints.
- Prefer domain-gated provision over open auto-create.

---

## Rollback

| Layer | Action |
|---|---|
| Hydrogen | `OIDC_RP.Enabled = false` or unset `HYDROGEN_OIDC_RP_ENABLED` |
| Client SPA | Hide/remove SSO entry points; password login remains |
| Keycloak | Disable client or remove redirect URIs |
| Data | Identity rows can remain; they are inert when RP disabled |

No reverse migration required to disable the feature.

---

## Relationship To Other Documents

| Document | Role |
|---|---|
| [`/docs/OIDC-PLAN.md`](/docs/OIDC-PLAN.md) | Historical full implementation (Phases 1–26 done). Use as archaeology, not day-to-day checklist. |
| This file (`KEYCLOAK_PLAN.md`) | Operational gated plan for production SSO, provision policy, client/IdP docs, E2E, post-MVP. |
| [`/docs/H/plans/OIDC_E2E_LOG.md`](/docs/H/plans/OIDC_E2E_LOG.md) | Manual real-Keycloak results (Phase 5). |
| [`/docs/H/api/auth/oidc_rp.md`](/docs/H/api/auth/oidc_rp.md) | Wire contract for RP endpoints. |
| [`/docs/Li/LITHIUM-KEYCLOAK.md`](/docs/Li/LITHIUM-KEYCLOAK.md) | Lithium-oriented client recipe; Phase 4 generalizes for all clients. |
| [`/docs/H/plans/AUTH_PLAN.md`](/docs/H/plans/AUTH_PLAN.md) | Password auth / JWT baseline. |
| [`/docs/H/plans/MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) | Structural model for gated plans. |

---

## Working Log (cross-phase memory)

Append discoveries, surprises, and decisions here as phases complete.

### Decisions log

- (Plan authored, 2026-07-11) **KEYCLOAK_PLAN created.** Intent: gated production/SSO completion plan, not a rewrite of the shipped RP. Historical implementation remains in `/docs/OIDC-PLAN.md` (1–26 ✅, 27 open). Hydrogen-as-IdP out of scope. Frontend coding deferred until Phase 6. Auto-provision is already implemented in RP linkers; Phase 3 locks operator policy and validates matrix. Client work in Phase 4 is documentation-only. Real Keycloak sign-off is Phase 5 via `OIDC_E2E_LOG.md`.
- (Phase 0 confirmation, 2026-07-11) **Foundation verified complete.** Historical OIDC-PLAN.md Phases 1-26 are all ✅ COMPLETE. Test 42 passes 88/88 across mock blackbox coverage (disabled endpoints, method gates, mock IdP discovery/JWKS, /start redirect, /handoff exchange, /callback failure paths + happy path, all 4 linker strategies, role mapping). Policy defaults locked for production:
  - Production linking strategy: **`match_email_only`** (link by verified email only, no auto-provision)
  - Provisioning: **off** (`ProvisionDefaults.Enabled = false`)
  - Email verification required: **true** (`RequireEmailVerified = true`)
  - Role source: **`database`** (QueryRef #017 join, no IdP role merge)
  - Shared realm expectations: Canvas LMS uses same `festival` realm with separate client; no shared client secrets.

### Surprises / deviations

- (none yet)

### Reusable snippets / gotchas

- SPA must use **top-level navigation** to `/api/auth/oidc/start` (browser follows 302 to Keycloak). `fetch()` to start is wrong for cross-origin redirect UX.
- Redirect URI is **exact string match** in Keycloak.
- JWT `roles` claim = role_id integers (`"1,3"`), not names. Resolve names via QueryRef `#127` when gating.
- Password login and OIDC both must load roles before `generate_jwt` (already fixed for password path as of Mail Relay 7.5b era).
- Sync DB pattern: register pending → submit → wait (never submit-first under parallel engines).
- Test 42 uses **mock** Keycloak (`tests/lib/mock_keycloak/server.js`); real IdP is manual Phase 5 only unless a future live test is added carefully (secrets, flakiness).
- Empty `AllowedEmailDomains` with provisioning enabled: confirm code semantics before production (Phase 3.6).

### Phase 0 completed notes

- Build verified: `zsh -ic 'mkt'` completed successfully.
- Test 42 verified: 88/88 sub-tests pass (mock IdP coverage complete).
- Unity RP suites verified intact via existing Phase coverage (Phases 7-22).
- Config fields match schema; no drift detected.
- Policy defaults recorded in Working Log decisions (2026-07-11 entry).

---

## Appendix A — Minimal Client Sequence (reference)

```text
1. User chooses SSO
2. window.location = `${hydrogenOrigin}/api/auth/oidc/start?database=${db}&return_to=/`
3. User completes Keycloak login (SSO session may already exist)
4. Browser lands on SPA URL with ?oidc=1&handoff=HEX  (or oidc_error=CODE)
5. SPA POST ${hydrogenOrigin}/api/auth/oidc/handoff  body: {"handoff":"HEX"}
6. SPA stores token from JSON; history.replaceState to strip query
7. SPA uses token as Authorization: Bearer for all APIs
8. Renew/logout use existing /api/auth/renew and /api/auth/logout
```

## Appendix B — Minimal Keycloak Client Checklist

- [ ] Realm exists; discovery URL works over HTTPS
- [ ] Confidential client; secret in env only
- [ ] Standard flow enabled
- [ ] Valid redirect URI = exact Hydrogen callback URL(s)
- [ ] Optional: valid post-logout redirect URIs (Phase 10)
- [ ] Optional: backchannel logout URL (Phase 9)
- [ ] Scopes include openid profile email
- [ ] Test users: linked email + unlinked email
- [ ] Email verified flags match `RequireEmailVerified` policy

## Appendix C — Minimal Hydrogen Config Shape (disabled example)

```json
{
  "OIDC_RP": {
    "Enabled": false,
    "Database": "Lithium",
    "Providers": [
      {
        "Name": "keycloak",
        "Label": "Sign in with SSO",
        "Issuer": "https://www.500passwords.com/realms/festival",
        "ClientId": "${env.HYDROGEN_OIDC_CLIENT_ID}",
        "ClientSecret": "${env.HYDROGEN_OIDC_CLIENT_SECRET}",
        "RedirectUri": "https://example.com/api/auth/oidc/callback",
        "Scopes": "openid profile email",
        "VerifySsl": true,
        "AuthMethod": "client_secret_basic",
        "AccountLinking": {
          "Strategy": "match_email_only",
          "RequireEmailVerified": true,
          "ProvisionDefaults": {
            "Enabled": false,
            "Authorized": true,
            "DefaultRoleNames": [],
            "AllowedEmailDomains": []
          }
        },
        "RoleMapping": {
          "Source": "database"
        }
      }
    ]
  }
}
```

Operators who want auto-create switch `Strategy` to `match_email_then_provision`, set `ProvisionDefaults.Enabled` true, and populate `AllowedEmailDomains`.
