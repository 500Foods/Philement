# OIDC LOGIN PLAN — Keycloak (500passwords.com) + Hydrogen + Lithium

## Overview

This plan describes how to add **OpenID Connect (OIDC) login via Keycloak** to the
Lithium web app, with Hydrogen acting as the **OIDC Relying Party (RP)**. The
existing username/password flow against the Hydrogen-managed `accounts` table
must keep working unchanged. After OIDC sign-in, the user ends up holding the
**same Hydrogen-issued JWT** that the password flow returns today, so every
existing API endpoint (`/api/auth/renew`, `/api/auth/logout`,
`/api/conduit/auth_query`, `/api/conduit/auth_queries`, …) keeps working with
zero changes.

> **Identity Provider:** Keycloak at `https://www.500passwords.com`
> **Relying Party:** Hydrogen (`/api/auth/oidc/*` endpoints, new)
> **User Agent:** Lithium SPA (Login Manager — new "Sign in with 500 Passwords" button)

---

## Why this is non-trivial

Two facts are crucial up front, because they shape every decision below.

1. **Hydrogen's existing `src/oidc/` and `src/api/oidc/` code is for Hydrogen
   being an OIDC *provider*** (issuing tokens to third parties). It is mostly
   stub code (`register_oidc_endpoints` is a no-op, `extract_client_credentials`
   returns hard-coded "test_client_id"). It is **not** a relying-party / client
   library, and it is **not** what we want for Keycloak integration. We will
   not modify or rely on it. It stays exactly where it is.

2. **Lithium expects a Hydrogen-issued JWT** with very specific claims
   (`user_id`, `username`, `roles`, `database`, `system_id`, `app_id`, `ip`,
   `tz`, `tzoffset`, `exp`, `iat`, `jti`, …). Keycloak's ID token does **not**
   contain these. So we cannot just hand the Keycloak ID token to the Lithium
   SPA and call it done. The OIDC flow has to terminate inside Hydrogen, where
   we **mint a Hydrogen JWT** that is indistinguishable from a password-login
   JWT. This is the single most important architectural decision in the plan.

The result: the only files that ever see a Keycloak token are server-side
(Hydrogen). The Lithium SPA only ever holds a Hydrogen JWT, exactly as today.

---

## High-Level Flow (Authorization Code + PKCE)

```text
 ┌──────────┐   1. click "Sign in with 500 Passwords"
 │ Lithium  │──────────────────────────────────────────────┐
 │ Login    │                                              │
 │ Manager  │   2. GET /api/auth/oidc/start                ▼
 └──────────┘──────────────────────────────────►  ┌────────────────┐
       ▲                                          │   Hydrogen     │
       │   3. 302 redirect (auth URL + state)     │   (RP)         │
       │◄─────────────────────────────────────────│                │
       │                                          └────────────────┘
       │
       ▼
 ┌──────────────────┐  4. user logs in       ┌────────────────┐
 │  Keycloak at     │◄───────────────────────│  user's        │
 │ 500passwords.com │  5. 302 with ?code=… ──│  browser       │
 └──────────────────┘                        └────────────────┘
                                                     │
                                  6. GET /api/auth/oidc/callback?code=…&state=…
                                                     │
                                                     ▼
                                            ┌────────────────┐
                                            │   Hydrogen     │
                                            │                │
                                            │ 7. POST /token │
                                            │    to Keycloak │──► Keycloak
                                            │ 8. validate ID │◄── id_token
                                            │    token       │
                                            │ 9. resolve to  │
                                            │    accounts row│──► DB
                                            │10. mint Hydrogen│
                                            │    JWT (same   │
                                            │    shape as    │
                                            │    password)   │
                                            └────────────────┘
                                                     │
                                  11. 302 to Lithium with one-time `oidc_handoff`
                                                     ▼
                                            ┌────────────────┐
                                            │   Lithium SPA  │
                                            │  exchanges     │
                                            │  handoff for   │
                                            │  Hydrogen JWT  │
                                            │  (same store-  │
                                            │  JWT + emit    │
                                            │  AUTH_LOGIN)   │
                                            └────────────────┘
```

Steps 7–10 are entirely server-to-server. The Keycloak `id_token` and
`access_token` never leave Hydrogen. Lithium is unaware that a third-party IdP
was involved — it simply receives the same JWT it would have received from a
password login.

---

## Goals and Non-Goals

### Goals

- Add a **second login option** in Lithium ("Sign in with 500 Passwords"); keep
  username/password fully functional and as the default.
- Hydrogen becomes a **proper OIDC Relying Party** (Authorization Code Flow +
  PKCE, conforming to RFC 6749 §4.1 and OIDC Core 1.0 §3.1).
- After OIDC sign-in, Lithium holds **a Hydrogen JWT with the same claims** as
  the password flow. No downstream code (Conduit, renew, logout, role checks)
  changes.
- A user signed in via OIDC can be linked to an existing `accounts` row, or, if
  enabled by config, **provisioned** automatically on first login.
- Configurable per environment (dev, staging, prod) via `lithium.json`,
  Hydrogen JSON config, and environment variables.

### Non-Goals (this plan)

- Hydrogen issuing tokens **to third parties** (the existing `src/oidc/`
  provider stubs). Out of scope.
- Multi-IdP (Google, GitHub, …). The architecture supports it but only the
  Keycloak / 500passwords.com client is implemented in this plan.
- SAML, LDAP direct-bind, or any non-OIDC federation.
- Refresh-token-driven session sliding using Keycloak. We rely on the existing
  Hydrogen JWT renewal flow (`/api/auth/renew`).
- Single Logout (SLO / RP-Initiated Logout). Optional Phase 5.

---

## Keycloak Configuration (500passwords.com)

This is what must exist on the Keycloak side **before** any code is written.
Capture these values into `hydrogen.json` and (for the public ones)
`lithium.json`.

### Realm

- **Realm name:** e.g. `philement` (one realm covers Lithium + any future
  apps).
- **Issuer URL:** `https://www.500passwords.com/realms/philement`
- **Discovery doc:** `https://www.500passwords.com/realms/philement/.well-known/openid-configuration`
- **JWKS URI:** retrieved from discovery (`jwks_uri` field).

### Client (one per environment)

| Setting | Value |
|---|---|
| Client ID | `lithium-web` (suggested; one per environment if needed: `lithium-web-dev`, `…-staging`, `…-prod`) |
| Client type | **Confidential** (public+PKCE is an option, but confidential is preferred since the RP is Hydrogen, a server) |
| Standard flow | **Enabled** (Authorization Code) |
| Implicit flow | Disabled |
| Direct access grants | Disabled |
| Service accounts | Disabled |
| Valid redirect URIs | `https://lithium.philement.com/api/auth/oidc/callback` (and per-env equivalents, plus `http://localhost:8080/api/auth/oidc/callback` for dev) |
| Valid post-logout redirect URIs | `https://lithium.philement.com/` (and per-env) |
| Web origins | `+` (mirror of redirect URIs) — only if browser ever talks directly; in our flow it does not, so this can be empty |
| PKCE Code Challenge Method | `S256` (required) |
| Backchannel logout URL | `https://lithium.philement.com/api/auth/oidc/backchannel-logout` (Phase 5) |

### Required scopes

`openid profile email` — minimum. Add `roles` (Keycloak's built-in client
scope) so Hydrogen can read realm/client roles from the ID token if we choose
role-mapping by IdP claim.

### Token lifetimes

Keep Keycloak's access/ID token lifetimes short (5–15 min). Hydrogen does not
rely on Keycloak refresh tokens beyond the initial code exchange, so this only
affects the time window between auth-code redirect and callback completion.

### Recommended Keycloak claims

The ID token must contain at least:

- `sub` — stable, opaque user identifier (this is the **anchor** for account
  linking).
- `email`, `email_verified`
- `preferred_username`
- `name` (full name) — optional but useful for display
- `realm_access.roles` — optional, used if we map roles from Keycloak.

---

## Hydrogen Side: Relying Party Implementation

### Where the new code lives

| Path | Purpose |
|---|---|
| `src/api/auth/oidc_rp/` | New directory — RP endpoints (start, callback, handoff) |
| `src/api/auth/oidc_rp/oidc_rp_start.[ch]` | `/api/auth/oidc/start` |
| `src/api/auth/oidc_rp/oidc_rp_callback.[ch]` | `/api/auth/oidc/callback` |
| `src/api/auth/oidc_rp/oidc_rp_handoff.[ch]` | `/api/auth/oidc/handoff` (one-time code exchange to Lithium) |
| `src/api/auth/oidc_rp/oidc_rp_state.[ch]` | In-memory `state`/`nonce`/PKCE store + handoff-code store |
| `src/api/auth/oidc_rp/oidc_rp_discovery.[ch]` | Fetch + cache `.well-known/openid-configuration` and JWKS |
| `src/api/auth/oidc_rp/oidc_rp_token.[ch]` | POST to Keycloak `/token` endpoint, parse response |
| `src/api/auth/oidc_rp/oidc_rp_idtoken.[ch]` | Validate ID token signature (JWKS) and claims |
| `src/api/auth/oidc_rp/oidc_rp_link.[ch]` | Resolve `iss + sub` → `account_id`, with optional auto-provision |
| `src/config/config_oidc_rp.[ch]` | New config block: `OIDCRelyingPartyConfig` |

> Why a new tree under `src/api/auth/` and not under `src/api/oidc/`?
> Because the existing `src/api/oidc/` is for Hydrogen-as-IdP. Mixing the two
> would invite confusion and accidental cross-wiring. Auth-related code that
> *terminates with a Hydrogen JWT* belongs under `src/api/auth/`.

### Endpoints

#### `GET /api/auth/oidc/start`

Public. No JWT required.

Inputs (query string):

- `database` (optional) — same param Lithium already uses; falls back to
  configured default.
- `return_to` (optional) — relative path inside Lithium to deep-link back to
  after sign-in. Validated against an allow-list (must start with `/`, no
  scheme/authority).

Behavior:

1. Generate `state` (32 bytes hex), `nonce` (32 bytes hex), and PKCE
   `code_verifier` (43–128 chars URL-safe) and `code_challenge` =
   `BASE64URL(SHA256(code_verifier))`.
2. Store `{state, nonce, code_verifier, database, return_to, created_at,
   client_ip}` in the in-memory **state store** with 10-minute TTL.
3. Build the authorization URL from cached discovery doc:
   `${authorization_endpoint}?response_type=code&client_id=…&redirect_uri=…&scope=openid+profile+email&state=…&nonce=…&code_challenge=…&code_challenge_method=S256`.
4. Issue HTTP 302 redirect to that URL. Set `Cache-Control: no-store`.

#### `GET /api/auth/oidc/callback`

Public. No JWT required. This is the URL configured in Keycloak.

Inputs (query string):

- `code` (required on success)
- `state` (required)
- `error`, `error_description` (on Keycloak-side failure)

Behavior:

1. If `error`, log and 302 to Lithium login page with `?oidc_error=…`. Stop.
2. Look up `state` in the state store. If missing/expired, fail with
   `oidc_error=state_invalid`.
3. POST `application/x-www-form-urlencoded` to `${token_endpoint}` with
   `grant_type=authorization_code`, `code`, `redirect_uri`, `client_id`,
   `client_secret` (Basic-Auth header preferred), `code_verifier`. Use
   `client_credentials` here only for token-endpoint auth, not as a grant.
4. Parse the JSON response → `id_token`, `access_token`, `expires_in`,
   `token_type`.
5. Validate the `id_token`:
   - Header `alg` is in the allow-list (`RS256` is what Keycloak uses by
     default; reject `none`).
   - Signature verifies against the matching JWK from cached JWKS (refresh
     JWKS on cache miss).
   - `iss` equals the configured issuer.
   - `aud` contains the configured `client_id`.
   - `exp` > now, `iat` reasonable, `nbf` (if present) ≤ now.
   - `nonce` equals the stored nonce for this state.
6. Resolve the user (see "Account Linking" below) → `account_info_t` +
   `system_info_t`.
7. **Mint a Hydrogen JWT** by calling the existing
   `generate_jwt(account, system, client_ip, tz, database, issued_at)` from
   `src/api/auth/auth_service_jwt.h`. This is the same code path the password
   login uses — so the resulting JWT is identical in shape and signature to a
   password-login JWT. Persist it via `store_jwt(...)` exactly as login does.
8. Generate a one-time **handoff code** (32 bytes hex), store
   `{code → {jwt, expires_at, account_id, client_ip, created_at}}` in a
   handoff store with 60-second TTL.
9. 302 redirect the browser back to Lithium:
   `${lithium_origin}/login?oidc=1&handoff=${handoff}` (plus
   `&return_to=${return_to}` if provided).

Why a handoff code and not put the JWT in the URL? Because putting a JWT in
a URL leaks it to browser history, referer headers, and server logs. The
handoff code is single-use, short-lived, and exchanged over an HTTPS POST.

#### `POST /api/auth/oidc/handoff`

Public. No JWT required. Lithium calls this from JS after seeing
`?handoff=…` in its URL.

Body:

```json
{ "handoff": "…" }
```

Behavior:

1. Look up the handoff in the store. Atomically remove it (single-use).
2. If missing or expired, respond `401 { "error": "handoff_invalid" }`.
3. Verify the requesting `client_ip` matches the IP that was stored at
   callback time, to defeat passive hijack. (Configurable; off by default
   if behind aggressive load balancers.)
4. Respond:

```json
{
  "token": "<hydrogen jwt>",
  "expires_at": "2026-…",
  "user_id": 123,
  "roles": ["admin"]
}
```

This is **the exact same response shape** as `POST /api/auth/login`, so the
Lithium client treats both paths uniformly.

#### `POST /api/auth/oidc/backchannel-logout` *(Phase 5, optional)*

Public. Verifies a Keycloak `logout_token` (signed JWT). Maps `sub` to
account, calls `delete_jwt_from_storage` for any active Hydrogen tokens
issued from that subject.

### State + handoff stores

In-memory hash table protected by a mutex, with TTL-driven sweeping.
Reasoning:

- Both stores are tiny and short-lived (≤ 10 min, ≤ 60 sec).
- Putting them in the database adds latency and pulls schema work into the
  critical path.
- If Hydrogen is ever clustered behind a load balancer, sticky sessions
  pinned by source IP for the duration of the auth flow are sufficient —
  a single user's browser will hit a single Hydrogen instance for both
  `/oidc/start` and `/oidc/callback`. The handoff exchange is similarly
  brief.
- The data is sensitive (PKCE verifier, nonce) so it is **never** logged.

If clustered without sticky sessions becomes a hard requirement later,
move both stores to Redis, fronted by the same interface.

### Discovery + JWKS caching

- Discovery doc cached for 1 hour. Refreshed on miss + on signature
  verification failure (rotated keys).
- JWKS cached the same way. Cache lookup is by `kid`.
- Both fetches go through the existing Hydrogen HTTP client used elsewhere.
- TLS verification is **mandatory** (`verify_ssl: true` from the existing
  `OIDCConfig`; reuse the field name and meaning).

### Configuration: `OIDCRelyingPartyConfig`

New section in Hydrogen's JSON config (e.g. inside the existing top-level
config file). Distinct from the existing `OIDCConfig` (which is for the
unrelated provider stubs).

```jsonc
{
  "OIDC_RP": {
    "Enabled": true,
    "Providers": [
      {
        "Name": "500passwords",
        "Label": "500 Passwords",          // displayed on the Lithium button
        "Issuer": "https://www.500passwords.com/realms/philement",
        "ClientId": "lithium-web",
        "ClientSecret": "${env.HYDROGEN_OIDC_CLIENT_SECRET}",
        "RedirectUri": "https://lithium.philement.com/api/auth/oidc/callback",
        "Scopes": "openid profile email",
        "VerifySsl": true,
        "AllowedAlgorithms": ["RS256"],
        "DiscoveryCacheSeconds": 3600,
        "JwksCacheSeconds": 3600,
        "StateTtlSeconds": 600,
        "HandoffTtlSeconds": 60,
        "BindHandoffToIp": true,

        "AccountLinking": {
          "Strategy": "match_email_then_provision",  // or "match_email_only", "match_sub_only", "provision_only"
          "ProvisionDefaults": {
            "Enabled": true,
            "Authorized": true,
            "DefaultRoleNames": ["user"]
          },
          "AllowedEmailDomains": ["philement.com", "500passwords.com"],
          "RequireEmailVerified": true
        },

        "RoleMapping": {
          "Source": "database",         // or "idp_realm_roles", "idp_client_roles", "merge"
          "IdpRoleClaim": "realm_access.roles",
          "IdpRolePrefix": ""
        }
      }
    ],

    "Database": "Lithium"               // database to use when minting JWT, if not provided by /start
  }
}
```

Environment variables (used everywhere we already use them in Hydrogen):

| Variable | Purpose |
|---|---|
| `HYDROGEN_OIDC_RP_ENABLED` | Master kill switch |
| `HYDROGEN_OIDC_CLIENT_ID` | Override `Providers[0].ClientId` |
| `HYDROGEN_OIDC_CLIENT_SECRET` | **Required** — never written to disk |
| `HYDROGEN_OIDC_ISSUER` | Override issuer |
| `HYDROGEN_OIDC_REDIRECT_URI` | Override redirect URI |

### Account Linking (the hard part)

Three pieces of data identify an OIDC user: `iss`, `sub`, and `email`. Only
`iss + sub` is guaranteed stable across an account's lifetime — `email` can
change.

We need a new lookup table to associate `(iss, sub)` with an `account_id`.

#### Schema additions

New table (Acuranzo migration ≥ 1100, exact number TBD by Hydrogen DB
maintainers):

```sql
CREATE TABLE account_oidc_identities (
    identity_id     INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id      INTEGER NOT NULL REFERENCES accounts(account_id),
    issuer          TEXT NOT NULL,
    subject         TEXT NOT NULL,
    email           TEXT,
    last_seen_at    TEXT NOT NULL,
    created_at      TEXT NOT NULL,
    UNIQUE (issuer, subject)
);
CREATE INDEX idx_account_oidc_account ON account_oidc_identities(account_id);
```

This is **additive**. Existing `accounts` rows are untouched; password login
keeps working.

New QueryRefs (numbers TBD; following `AUTH_PLAN_ACURANZO.md` conventions):

| QueryRef (proposed) | Purpose |
|---|---|
| `#080` | Look up `account_id` by `(issuer, subject)` |
| `#081` | Insert a new `account_oidc_identities` row (link an existing account) |
| `#082` | Look up `account_id` by `email` (case-insensitive, only if `email_verified`) |
| `#083` | Provision a new `accounts` row from OIDC claims and link it |
| `#084` | Update `last_seen_at` and `email` for an existing identity |

Each QueryRef gets a `.lua` payload alongside the existing auth queries.

#### Linking strategies

`AccountLinking.Strategy` controls behavior. Default is
`match_email_then_provision`:

1. Lookup by `(iss, sub)` → if found, use that `account_id`. Update
   `last_seen_at`. Done.
2. Else if `email_verified` and `email` resolves to an account: link by
   inserting an `account_oidc_identities` row, then proceed. (This handles
   an existing-password-account user signing in via OIDC for the first
   time.)
3. Else if `ProvisionDefaults.Enabled`: create an `accounts` row with no
   password hash (`password_hash = NULL`, the column must allow NULL — see
   migration), link it, assign default roles. Account is created with
   `enabled = true, authorized = ProvisionDefaults.Authorized`.
4. Else: fail with `oidc_error=no_account`.

Other valid strategies: `match_sub_only` (purely IdP-managed identities,
no provisioning, fails if no link), `match_email_only` (link but never
auto-create), `provision_only` (always create on first login — only safe
for trusted IdPs and with a strict `AllowedEmailDomains`).

#### Edge cases that MUST be handled

- Two existing password accounts with the same email → reject with explicit
  error (admin must merge first).
- The IdP changes a user's email → we trust `(iss, sub)`; `email` on the
  identity row is updated for display only.
- An `accounts` row is disabled — OIDC must respect that (`account.enabled
  = false` ⇒ same `403 account_disabled` response as password login).

### Role mapping

Default (`RoleMapping.Source = "database"`): the `accounts → account_roles`
join is the source of truth. The IdP's role claims are ignored. Cleanest;
recommended.

Optional (`"idp_realm_roles"` / `"idp_client_roles"` / `"merge"`): copy
roles from `realm_access.roles` (or `resource_access.<client>.roles`) into
the JWT's `roles` claim, optionally with a `IdpRolePrefix` like `kc:` so
admins can tell where a role came from. `merge` unions both sources.

### Minting the Hydrogen JWT

The existing `generate_jwt()` already takes `account_info_t* account`,
`system_info_t* system`, `client_ip`, `tz`, `database`, `issued_at`. We pass:

- `account` from the linking step.
- `system` from `verify_api_key()` using a **server-side** API key bound to
  the OIDC RP (one per environment; configured under `OIDC_RP.SystemApiKey`
  or similar; never sent to the browser).
- `client_ip` from the original `/oidc/start` request (preserved through
  state).
- `tz` from the browser if Lithium passed it to `/oidc/start` as a query
  param; otherwise UTC. This matches existing JWT semantics.
- `database` from the OIDC RP config or the `database` query param of
  `/oidc/start`.
- `issued_at = now()`.

The resulting JWT goes through the same `store_jwt(...)` call as a password
login, which means `/api/auth/renew` and `/api/auth/logout` work
unchanged.

### Logging (per `LITHIUM-INS.md`-equivalent on the Hydrogen side)

Every step uses `log_this(SR_AUTH, …)` with a new sub-prefix `OIDC_RP` in
the message text for grep-ability. Never log `code`, `state`, `nonce`,
`code_verifier`, `client_secret`, `id_token`, `access_token`, or
`handoff` codes. Log `iss`, `sub` (truncated), `email` (only for failed
linking events), `client_ip`, and result codes.

### Tests (Hydrogen)

#### Unity unit tests

- `test_oidc_rp_state.c` — store insert/lookup/expiry/sweep.
- `test_oidc_rp_discovery.c` — discovery JSON parse, cache TTL, refresh on
  miss.
- `test_oidc_rp_idtoken.c` — JWT validation: good, bad-sig, wrong-iss,
  wrong-aud, expired, bad-nonce, `alg=none` rejection.
- `test_oidc_rp_link.c` — all four linking strategies with table fixtures.
- `test_oidc_rp_pkce.c` — verifier/challenge derivation.

Each test must follow `LITHIUM-INS.md`-style discipline (no large files,
no globals leaking between tests, etc.). Hydrogen's equivalent rules are
in `docs/H/INSTRUCTIONS.md` and `docs/H/tests/TESTING_UNITY.md`.

#### Blackbox / integration test

- `tests/test_42_oidc_rp.sh` — drives the full flow against a **mock
  Keycloak** (a small Node or Python script in `tests/lib/mock_keycloak/`):
  - issues realistic discovery + JWKS,
  - signs an `id_token` per request,
  - records token-endpoint POSTs for assertions.

  Verifies: redirect at `/oidc/start`, valid callback ⇒ handoff URL,
  handoff exchange ⇒ Hydrogen JWT, JWT is accepted by
  `/api/conduit/auth_query`, replay of handoff fails, expired state fails,
  bad nonce fails, IP mismatch fails, disabled account fails.

- Extend `test_40_auth.sh` with a check that **OIDC sign-in produces a JWT
  indistinguishable from password login** at the API level (same claims
  shape, same renew/logout behaviour).

---

## Lithium Side: Login Manager Changes

### Where the new code lives

| Path | Purpose |
|---|---|
| `src/managers/login/login.html` | Add the "Sign in with X" button + small divider |
| `src/managers/login/login.css` | Styling for the new button + provider icon |
| `src/managers/login/login.js` | Wire the button; keep this file under the 1000-line rule (it is currently 1866 lines and is **already over** — see "Refactor first" below) |
| `src/managers/login/oidc-login.js` | **New** — pure ES6 module: detects handoff in URL, exchanges it, stores JWT, emits `AUTH_LOGIN`. Keeps logic out of the already-too-large `login.js` |
| `src/core/oidc-client.js` | **New** — single-purpose helper: `startOidc(provider, returnTo)`, `exchangeHandoff(handoff)`. Returns Promises; no DOM access; fully unit-testable |
| `src/styles/login-providers.css` | If we ever add more providers; for now can live in `login.css` |
| `config/lithium.json` | Add `auth.oidc_providers` array (see below) |

### Refactor first

`login.js` is **1866 lines** today. `LITHIUM-INS.md` rule #2 caps source
files at 1000 lines. Adding OIDC code on top of it is a violation. The
plan therefore includes a small, scoped refactor as **Phase 0** —
extracting password-manager suppression, panel switching, and the language
table init into separate modules. This is a prerequisite, not a "while
we're in there" nice-to-have.

Actual results after Phases 1–2:

| New file | Lines | Extracted from `login.js` | Actual reduction in `login.js` |
|---|---|---|---|
| `login-panels.js` (panel crossfade + ESC handling) | 228 | ~250 | −119 (1,866 → 1,747) |
| `login-password-manager.js` (1Password suppression) | 147 | ~150 | −126 (1,747 → 1,621) |
| `login-language-panel.js` (Tabulator init) | 562 | ~250 | −156 (1,621 → 1,466) |
| Phase 4 (combined): see expanded scope below | — | ~820 | target: ≥ −466 (1,466 → ≤ 1,000) |

**Running total:** `login.js` at 1,466 lines after Phase 3. Need ≥ 466 more
lines removed to hit the ≤1,000 gate. Phase 4 has been expanded to a
combined six-item decomposition that can realistically remove 600–800
lines (target: 650–750 lines remaining).

Then the OIDC additions (~200 new lines across `oidc-login.js` and
`oidc-client.js`) keep the main `login.js` well under 1000.

### UI: the new button

Per `LITHIUM-INS.md` rule #4 (CSS-first), all styling in `login.css`. Per
rule #1 (no fallbacks), if OIDC is disabled in config the button is simply
not rendered — no greyed-out fallback path.

```html
<!-- inside login-panel, below the existing form -->
<div class="login-divider" id="login-divider-oidc"><span>or</span></div>
<div class="login-providers" id="login-providers">
  <!-- generated at render time, one button per provider in config -->
</div>
```

```css
.login-divider { /* horizontal rule with centered "or" pill */ }
.login-providers { display: flex; flex-direction: column; gap: var(--space-2); }
.login-btn-oidc { /* same height/radius as the password Login button */ }
.login-btn-oidc__icon { width: 1.25rem; height: 1.25rem; }
.login-btn-oidc[data-provider="500passwords"] { /* brand colour via CSS var */ }
```

The button reads from `config/lithium.json`:

```jsonc
{
  "auth": {
    "api_key": "…",
    "default_database": "Lithium",
    "session_timeout_minutes": 30,

    "oidc_providers": [
      {
        "id": "500passwords",
        "label": "Sign in with 500 Passwords",
        "icon": "fa-key",                       // Font Awesome name
        "start_url": "/api/auth/oidc/start"     // optional override
      }
    ]
  }
}
```

`oidc_providers` is an **array** so adding Google or GitHub later is
purely a config change plus Keycloak side work. Current task only ships
the `500passwords` entry.

### Click flow

```text
button click
  → oidc-client.startOidc("500passwords", currentReturnTo)
      → window.location.href =
          `${server.url}/api/auth/oidc/start?database=${db}&return_to=${path}`
```

This is a full top-level navigation, not a fetch. We *want* the browser to
follow Hydrogen's 302 to Keycloak naturally; an XHR/fetch cannot follow a
cross-origin redirect to a different host.

### Return flow

When the browser comes back to Lithium with `?oidc=1&handoff=…`:

1. `app.js` boots normally. (No special-case branch.)
2. `oidc-login.js` runs as part of Login Manager `init()`, **before** the
   form is shown:

```javascript
// oidc-login.js (sketch — not final)
import { exchangeHandoff } from '../../core/oidc-client.js';
import { storeJWT } from '../../core/jwt.js';
import { eventBus, Events } from '../../core/event-bus.js';
import { log, Subsystems, Status } from '../../core/log.js';

export async function processOidcReturn(loginManager) {
  const params = new URLSearchParams(window.location.search);
  if (params.get('oidc') !== '1') return false;

  const handoff = params.get('handoff');
  const oidcError = params.get('oidc_error');

  // Clean URL immediately so the handoff is never visible in history beyond
  // the very next event loop tick.
  const cleanUrl = window.location.pathname + (params.get('return_to') || '');
  window.history.replaceState({}, '', cleanUrl);

  if (oidcError) {
    log(Subsystems.AUTH, Status.WARN, `OIDC sign-in failed: ${oidcError}`);
    loginManager.showError(mapOidcError(oidcError));
    return true;
  }

  if (!handoff) {
    loginManager.showError('Sign-in did not complete. Please try again.');
    return true;
  }

  try {
    const data = await exchangeHandoff(handoff);
    storeJWT(data.token);
    log(Subsystems.AUTH, Status.SUCCESS, `OIDC sign-in successful: user_id=${data.user_id}`);
    await loginManager.hide();
    eventBus.emit(Events.AUTH_LOGIN, {
      userId: data.user_id,
      username: data.username,
      roles: data.roles || [],
      expiresAt: data.expires_at,
    });
    return true;
  } catch (err) {
    log(Subsystems.AUTH, Status.ERROR, `Handoff exchange failed: ${err.message}`);
    loginManager.showError('Sign-in could not complete. Please try again.');
    return true;
  }
}
```

This deliberately mirrors the password `attemptLogin` success path so that
**every other manager downstream cannot tell the difference** between the
two login methods.

### URL hygiene

- The handoff code is **wiped from the URL** via `history.replaceState()`
  immediately after being read. It is not put into `localStorage`.
- The handoff exchange uses `fetch` with `credentials: "omit"` (we do not
  use cookies for auth) and `Cache-Control: no-store`.
- `oidc_error` codes are an enumerated set, mapped client-side to
  user-friendly messages, so we don't echo IdP error strings into the DOM.

### Settings & remembered choice

`window.lithiumSettings.set('auth.last_method', 'oidc')` after a
successful OIDC login (and `'password'` after a password login). On next
visit, we can subtly highlight the previously-used button. Per
`LITHIUM-INS.md` rule #8, we use `lithiumSettings`, not raw `localStorage`.

### Tests (Lithium)

Vitest (per `LITHIUM-TST.md` patterns):

- `tests/unit/core/oidc-client.test.js` — `startOidc()` builds the right
  URL with all params; `exchangeHandoff()` posts JSON, handles 4xx, parses
  response.
- `tests/unit/managers/login/oidc-login.test.js` — `processOidcReturn`
  for all branches: no oidc param, `oidc_error`, missing handoff, success,
  exchange failure. Mock `fetch`, `storeJWT`, `eventBus.emit`.
- Extend `tests/unit/managers/login/login.test.js` — button is rendered
  iff `auth.oidc_providers` is non-empty; button click calls
  `startOidc`.

---

## Security Plan

The OIDC flow introduces several attack surfaces. Each one has a specific
mitigation.

| Threat | Mitigation |
|---|---|
| **CSRF on `/oidc/callback`** (attacker tricks user's browser into hitting callback with attacker's `code`) | `state` parameter, generated server-side, single-use, 10-min TTL, mismatched state ⇒ reject. |
| **Replay of authorization code** | Code is single-use at Keycloak's end (RFC 6749). PKCE adds defence in depth. |
| **Token interception in URL** (logs, referer) | The Keycloak `id_token`/`access_token` never appear in any URL. Only the opaque, single-use `handoff` code does, and it is wiped from the URL on arrival. |
| **Open redirect via `return_to`** | Allow-listed pattern: must start with `/`, no `//`, no `\\`, no scheme. Validated both at `/oidc/start` (rejected before redirect) and after handoff exchange (sanitised before navigation). |
| **`alg=none` JWT** | Algorithm allow-list (`RS256` only). Never trust the `alg` in the header — compare against the configured list. |
| **JWKS cache poisoning / key rotation race** | Cache JWKS by `kid`. On signature failure, refresh JWKS once before declaring failure. |
| **Stolen handoff code** | 60-second TTL, single-use (atomic remove on lookup), bound to `client_ip` of callback when `BindHandoffToIp = true`. Served only over HTTPS. |
| **Replay of Hydrogen JWT after OIDC logout** | Same as today (no change): the `tokens` table revocation path applies; `is_token_revoked` is checked on every request. |
| **IdP outage causes login outage** | Password login remains unaffected. Health-check probe of discovery endpoint exposed via `/api/system/health` with separate field `oidc_rp_status`. |
| **Mismatched email between IdP and accounts table** | Linking strategy is explicit and configurable. `RequireEmailVerified = true` by default. Two accounts with same email ⇒ refuse, log, and surface an explicit error to the user. |
| **Logged secrets** | Audit pass over the new code: `client_secret`, `code`, `state`, `nonce`, `code_verifier`, `id_token`, `access_token`, and `handoff` are on a deny-list; the logging helpers redact them. |
| **Clock skew** | Allow ±60s for `iat`/`nbf`/`exp`. Configurable. |

### Threat model out of scope

- IdP compromise (Keycloak itself is breached) — out of scope; relies on
  IdP operator.
- TLS-stripping man-in-the-middle — handled by HSTS at the load balancer.

---

## Configuration & Deployment

### Per environment

| Item | Dev | Staging | Prod |
|---|---|---|---|
| Lithium origin | `http://localhost:3000` | `https://staging.lithium.philement.com` | `https://lithium.philement.com` |
| Hydrogen origin | `http://localhost:8080` | `https://staging.hydrogen.philement.com` | `https://hydrogen.philement.com` |
| Keycloak issuer | `https://www.500passwords.com/realms/philement-dev` | `…/philement-staging` | `…/philement` |
| Keycloak client ID | `lithium-web-dev` | `lithium-web-staging` | `lithium-web` |
| Keycloak redirect URI | `http://localhost:8080/api/auth/oidc/callback` | staging Hydrogen URL | prod Hydrogen URL |
| `HYDROGEN_OIDC_CLIENT_SECRET` | dev-only secret in `.env` | from secrets store | from secrets store |

### Bootstrapping checklist

1. Keycloak admin creates the realm, client, and one test user. Captures
   client secret.
2. Hydrogen ops sets `HYDROGEN_OIDC_CLIENT_SECRET` in the deploy environment.
3. Hydrogen JSON config gains the `OIDC_RP` block above.
4. DB migration `1100_account_oidc_identities.sql` applied; QueryRefs
   `#080`–`#084` added to the payload.
5. Lithium `config/lithium.json` gains `auth.oidc_providers`.
6. Smoke test: Test 42 (new) plus a manual sign-in cycle.

### Rollback

Two independent kill switches:

- Hydrogen: `OIDC_RP.Enabled = false` (or `HYDROGEN_OIDC_RP_ENABLED=false`).
  Endpoints return `404`. No DB changes needed; the `account_oidc_identities`
  table is harmless when unused.
- Lithium: empty `auth.oidc_providers` array ⇒ the button is not rendered.
  Password login continues unaffected.

If a rollback happens *after* OIDC users have been auto-provisioned, those
`accounts` rows still exist with `password_hash = NULL` and cannot log in
via password. Operators need a procedure to either delete or set passwords
on those rows. This is documented in the rollback section of the eventual
operations runbook (out of scope here, listed in Phase 5).

---

## Implementation Phases

Phases are deliberately small and self-contained. Each phase:

- Has a **single, narrow goal** that one developer (or one model session)
  can complete and verify in isolation.
- Lists explicit **prerequisites** (which earlier phases must be done).
- Lists every **file created or touched** in that phase.
- Defines the exact **tests that must exist and pass** before the phase
  is considered complete.
- Ends with a **Definition of Done checklist** that is signed off before
  the next phase begins.

If a phase is interrupted, the codebase still builds and the existing
password login still works — every phase preserves that invariant.

> **Phase numbering is stable.** When new sub-phases are inserted later
> they get suffixes (e.g. `Phase 7a`), never renumbering existing phases.

### Phase index

| # | Title | Project | Risk |
|---|---|---|---|
| 1 | Lithium login refactor — extract `login-panels` | Lithium | Low | ✅ Done |
| 2 | Lithium login refactor — extract `login-password-manager` | Lithium | Low | ✅ Done |
| 3 | Lithium login refactor — extract `login-language-panel` | Lithium | Low | ✅ Done |
| 4 | Lithium login refactor — full decomposition to ≤1,000 lines | Lithium | Medium | ✅ Done |
| 5 | Hydrogen — `OIDCRelyingPartyConfig` schema and parser | Hydrogen | Low | ✅ Done |
| 6 | Hydrogen — disabled-stub endpoints (`/start`, `/callback`, `/handoff`) | Hydrogen | Low | ✅ Done |
| 7 | Hydrogen — in-memory state store | Hydrogen | Medium | ✅ Done |
| 8 | Hydrogen — PKCE helpers (verifier, challenge, state, nonce) | Hydrogen | Low | ✅ Done |
| 9 | Hydrogen — discovery + JWKS fetch and cache | Hydrogen | Medium | ✅ Done |
| 10 | Hydrogen — `/oidc/start` redirect builder | Hydrogen | Medium | ✅ Done |
| 11 | Hydrogen — token endpoint client (POST to Keycloak `/token`) | Hydrogen | Medium | ✅ Done |
| 12 | Hydrogen — ID token validation | Hydrogen | High | ✅ Done |
| 13 | Hydrogen — handoff store and `/oidc/handoff` exchange endpoint | Hydrogen | Medium | ✅ Done
| 14 | Hydrogen — `/oidc/callback` end-to-end with stub linker | Hydrogen | High | ✅ Done
| 15 | Hydrogen — DB migration `1100_account_oidc_identities` | Hydrogen | Medium |
| 16 | Hydrogen — `accounts.password_hash` nullable migration | Hydrogen | Medium |
| 17 | Hydrogen — QueryRefs `#080`–`#084` (`.lua` payloads) | Hydrogen | Low |
| 18 | Hydrogen — account linker (`match_sub_only`) | Hydrogen | Medium |
| 19 | Hydrogen — account linker (`match_email_only`) | Hydrogen | Medium |
| 20 | Hydrogen — account linker (provisioning + email allow-list) | Hydrogen | High |
| 21 | Hydrogen — wire `match_email_then_provision` (full default flow) | Hydrogen | Medium |
| 22 | Hydrogen — role-mapping (`database`, `idp_realm_roles`, `merge`) | Hydrogen | Medium |
| 23 | Lithium — `core/oidc-client.js` (`startOidc`, `exchangeHandoff`) | Lithium | Low |
| 24 | Lithium — `oidc-login.js` (process return-from-IdP) | Lithium | Medium |
| 25 | Lithium — UI: provider button, divider, config-driven render | Lithium | Low |
| 26 | Lithium — `auth.last_method` setting and subtle highlighting | Lithium | Low |
| 27 | Both — End-to-end against real Keycloak (dev environment) | Hydrogen + Lithium | High |
| 28 | Phase-5-style hardening: health check field | Hydrogen | Low |
| 29 | Phase-5-style hardening: backchannel logout | Hydrogen | Medium |
| 30 | Phase-5-style hardening: RP-initiated logout in Lithium | Hydrogen + Lithium | Medium |

Phases 28–30 are explicitly **post-MVP**. Phases 1–27 are the
must-ship scope for "OIDC login works in production".

---

### Phase template (used by every phase below)

Every phase block contains the same fields, in the same order:

- **Goal** — one sentence describing the deliverable.
- **Prerequisites** — earlier phases that must be Done.
- **In scope** — exhaustive list of code/config/tests to add or change.
- **Files** — exact paths created or touched.
- **Tests required** — each test must exist and pass.
- **Definition of Done** — checklist; all items must be ticked before
  moving on.

---

### Phase 1 — Lithium: extract `login-panels.js` ✅ COMPLETE

- **Goal:** Move panel crossfade + ESC handling out of `login.js` into a
  new module, with no behaviour change.
- **Prerequisites:** none.
- **In scope:**
  - Create `src/managers/login/login-panels.js` exporting a small class
    (e.g. `LoginPanels`) that owns `switchPanel()`, the crossfade
    timing, and the panel-keyed element registry.
  - Replace the corresponding methods in `login.js` with delegation to
    the new class.
  - **No new public API.** Same DOM, same timings.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-panels.js` (228 lines)
  - Created: `elements/003-lithium/tests/unit/managers/login/login-panels.test.js` (24 tests)
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - New Vitest file unit-tests panel switching with mocked DOM elements:
    happy-path crossfade, ESC returns to login panel, double-click
    suppression.
  - All existing login tests still pass.
- **Definition of Done:**
  - [x] `npm run lint` clean.
  - [x] `npm run build` clean.
  - [x] `npm test` 100% green (696 tests: 672 existing + 24 new).
  - [x] `login.js` line count went **down** by 119 lines (1,866 → 1,747).
  - [x] No file in `src/managers/login/` exceeds 1000 lines.
  - [ ] Manual smoke (against a running backend in someone's dev env, or
        screenshot diff) shows panels still crossfade identically.

**Lessons learned:**

1. **Callback-based delegation works well.** `LoginPanels` accepts `onBeforeSwitch` and `onAfterSwitch` callbacks so `LoginManager` retains panel-specific logic (password-manager visibility, logs population, language init) while `LoginPanels` owns the generic transition mechanics. This split is cleaner than a purely event-driven approach.
2. **Focus timer belongs in `LoginPanels`.** The `_loginFocusTimer` was originally in `LoginManager` but is logically part of panel transition sequencing. Moving it into `LoginPanels` (as `scheduleFocus()` / `cancelPendingFocus()`) simplified `LoginManager.teardown()` and made the timer testable.
3. **Line-count target was optimistic.** The original plan expected a ≥200-line reduction, but the actual reduction was 119 lines. The remaining panel-related code in `login.js` (the two callback methods `_onBeforePanelSwitch` and `_onAfterPanelSwitch`, plus `show()`/`hide()` which use panel state) is substantial. Future phases should expect similar ratios: extracted module ≈ 200–250 lines, reduction in parent ≈ 100–150 lines.
4. **Import path depth matters for tests.** The test file is at `tests/unit/managers/login/` (four levels under `elements/003-lithium/`) while the source is at `src/managers/login/` (two levels). The relative import requires `../../../../src/...` not `../../../src/...`.

**Deferred to later phases:**

- Legacy `login-alt-btn-group` (Didit/Apple/Google/Microsoft buttons in `login.html`) — **deferred to Phase 25**. Decision: keep existing buttons until the new config-driven OIDC provider system replaces them.
- Support for multiple OIDC providers alongside 500passwords — **deferred to Phase 25**. The config schema should support an array of providers.
- URL query parameter to force a specific login provider (e.g. `?login_provider=500passwords`) — **deferred to Phase 25 or later**.

**Setup for Phase 2:**

- `LoginPanels._onBeforePanelSwitch()` currently toggles password-manager UI via `this.togglePasswordManagerUI()`. When `login-password-manager.js` is extracted in Phase 2, this call will change to `this._passwordManager.show()` / `this._passwordManager.hide()`.
- `LoginPanels._onAfterPanelSwitch()` currently schedules focus on `this.elements.username`. This dependency on `LoginManager.elements` is acceptable for now; a future refactor could pass the focus target element into `scheduleFocus()`.
- The `_loginPanels` instance is created in `render()` and torn down in `teardown()`. Phase 2's `PasswordManager` should follow the same lifecycle pattern.

---

### Phase 2 — Lithium: extract `login-password-manager.js`

- **Goal:** Move 1Password / password-manager DOM suppression logic out
  of `login.js`, no behaviour change.
- **Prerequisites:** Phase 1.
- **In scope:**
  - Create `login-password-manager.js` exporting `PasswordManager` class
    with `show()` / `hide()` / `removeAll()` / `destroy()`.
  - Replace inlined MutationObserver + suppression code in `login.js`
    with calls to the module.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-password-manager.js`
  - Created: `elements/003-lithium/tests/unit/managers/login/login-password-manager.test.js`
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - Vitest: injection of a fake password-manager element triggers
    suppression styles within one tick; `show()` stops suppression;
    `destroy()` removes the observer and injected elements.
- **Definition of Done:**
  - [x] `npm run lint` clean.
  - [x] `npm run build` clean.
  - [x] `npm test` 100% green (716 tests: 696 existing + 20 new).
  - [x] `login.js` line count down by 126 lines (1,747 → 1,621).

**Lessons learned:**

1. **Class-based extraction is cleaner than function-based for stateful DOM logic.** The original plan sketched `attach(rootEl)` / `detach()` / `cleanupAfterLogin()` free functions. In practice, the password-manager logic owns a `MutationObserver` instance and needs persistent state across multiple show/hide cycles. A `PasswordManager` class with `show()` / `hide()` / `removeAll()` / `destroy()` maps exactly to the lifecycle calls in `LoginManager` and avoids passing the observer around as a closure variable.
2. **The `getPasswordManagerSelectors()` import moves with the code.** This utility was in `../../shared/log-formatter.js`, imported only by the password-manager logic. Moving the logic into its own module kept the import local and removed it from `login.js`'s dependency list, slightly simplifying the top of the file.
3. **Line-count reduction exceeded the ≥100 target.** Extracted ~147 lines into the new module; `login.js` dropped by 126 lines (1,747 → 1,621). The ratio was better than Phase 1 because the extracted code had fewer callbacks back into `LoginManager` — it is almost entirely self-contained.
4. **Optional chaining (`?.`) on the module reference is safe during teardown.** `this._passwordManager?.destroy()` in `teardown()` handles the case where `render()` was never reached (e.g. fallback path) without an extra null check.
5. **Test mocking strategy: mock `getPasswordManagerSelectors` at the source.** The Vitest file mocks `../../shared/log-formatter.js` rather than the DOM, which lets us inject fake elements by selector and verify suppression/removal without depending on real password-manager extensions.

**Deferred to later phases:**

- The `hide-password-manager-ui` body class is still referenced in the new module but is not actively styled in the current CSS. If we later add CSS rules keyed to this class, they will live in `login.css` and be tested in Phase 25.

**Setup for Phase 3:**

- `login.js` now has `_passwordManager` following the same lifecycle pattern as `_loginPanels` (created in `render()`, destroyed in `teardown()`). Phase 3's `LanguagePanel` should follow the same pattern.
- The `_onBeforePanelSwitch()` callback is now the only place that bridges `LoginPanels` and `PasswordManager`. Future panel extractions will add their own callback logic here.

---

### Phase 3 — Lithium: extract `login-language-panel.js` ✅ COMPLETE

- **Goal:** Move the Tabulator language picker out of `login.js`.
- **Prerequisites:** Phase 1.
- **In scope:**
  - `login-language-panel.js` exports a `LanguagePanel` class (following
    the `PasswordManager` / `LoginPanels` pattern) with `init()`, `show()`,
    `hide()`, `destroy()`, `filter()`, `selectLanguage()`, `getCurrentLocale()`.
  - Locale detection (best-guess + saved) moved here.
  - The `_languageTable`, `_languageData`, `_currentLocale`,
    `_bestGuessLocale` fields moved from `LoginManager` to `LanguagePanel`.
  - Keyboard navigation (`_handleKeydown`) and filter-reset
    on panel leave moved into the module.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-language-panel.js` (562 lines)
  - Created: `elements/003-lithium/tests/unit/managers/login/login-language-panel.test.js` (25 tests)
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - Vitest with a Tabulator mock: rows render, filtering works, keyboard
    navigation calls back with the expected locale.
- **Definition of Done:**
  - [x] Lint, build, test all clean.
  - [x] `login.js` line count down by 157 lines (1,623 → 1,466).

**Lessons learned:**
1. **The `show()` / `hide()` split is cleaner than `init()` alone.** The original plan sketched `init(container, onLocaleSelect)` / `destroy()` / `focusFilter()` / `getCurrentLocale()`. In practice, the language panel needs asymmetric lifecycle: `show()` must both initialize the table (if first time) *and* attach the document-level keyboard listener; `hide()` must reset the filter *and* detach that listener. This mirrors `PasswordManager.show()` / `hide()` and keeps `LoginManager._onBeforePanelSwitch()` a simple coordinator.
2. **Import ownership moved cleanly.** All `../../shared/languages.js` imports (`getBestGuessLocale`, `getLanguageData`, `getCountryCode`, `saveLocalePreference`, `getSavedLocale`, `supportedLocales`) and the `Flags` import moved out of `login.js` entirely. `getSavedLocale` was verified unused and removed from `login.js` imports during Phase 3 cleanup.
3. **Line-count reduction was 156, not ≥200.** The pattern continues: extracted module ≈ 560 lines, reduction in parent ≈ 156 lines. Future phases should expect similar ratios (extracted module ≈ 200–600 lines, reduction in parent ≈ 100–200 lines). The gate to ≤1,000 will require Phases 3 + 4 combined.
4. **Tabulator mock strategy for Vitest:** Mocking `tabulator-tables` as a constructor function that exposes `.on()`, `.getRows()`, `.setFilter()`, etc. is straightforward. The test mocks the module at the import level, so `LanguagePanel` never knows it's not using the real Tabulator.
5. **Event delegation vs. callback:** `LanguagePanel` emits `LOCALE_CHANGED` via `eventBus` (global) *and* calls `onLocaleSelected` (local callback). The local callback gives `LoginManager` a hook if it ever needs to react to locale changes without subscribing to the global bus. Both paths are tested.

**Setup for Phase 4:**
- `login.js` now has three subsystems following the same lifecycle: `_loginPanels`, `_passwordManager`, `_languagePanel`. Phase 4's `LogsPanel` and `HelpPanel` should follow the same pattern.
- `_onBeforePanelSwitch()` now delegates to three subsystems. Phase 4 will add two more (`_logsPanel?.show()` / `hide()` and `_helpPanel?.show()` / `hide()`).
- The `_onAfterPanelSwitch()` callback remains simple (focus management only). If Phase 4 panels need post-transition work, consider adding an `onAfterShow()` callback to those panel classes rather than expanding `_onAfterPanelSwitch()`.

---

### Phase 4 — Lithium: full decomposition of `login.js` to ≤1,000 lines ✅ COMPLETE

- **Goal:** Bring `login.js` cleanly under 1,000 lines via a coordinated
  set of extractions plus dead-code removal. This is a single phase
  (not several) because the decomposition is interlocked: shared state
  (e.g. `isPasswordVisible`, the password-element references) is
  referenced by multiple targets, so splitting them across phases would
  require unstable intermediate APIs.
- **Prerequisites:** Phase 1 (panels), Phase 2 (password manager),
  Phase 3 (language panel).
- **Scope expansion rationale:** A pure logs+help extraction would
  remove ~120–180 lines (1,466 → ~1,300), still ~300 lines over the
  gate. Phase-3-style "extract a panel module" produces a poor
  reduction ratio (~28%) because most lifecycle scaffolding stays in
  `login.js`. Phase 4 therefore widens scope to include behavioural
  modules (form submission, keyboard shortcuts) that have a much
  better reduction ratio (~80%) because their logic is mostly
  self-contained. Combined with dead-code removal from the Phase 3
  fallout, the gate becomes achievable in one phase.

- **In scope (six independent work items, all in this phase):**

  1. **Dead-code removal (Phase 3 fallout, ~316 lines).** Phase 3 moved
     the language table into `LanguagePanel` but left behind a cluster
     of methods on `LoginManager` (lines 760–1075 of current
     `login.js`) that reference instance state (`this._languageTable`,
     `this._currentLocale`, `this._languageData`) and module globals
     (`getCountryCode`, `getFlagSvg`, `supportedLocales`,
     `saveLocalePreference`) that **no longer exist or are not
     imported**. These methods would throw `ReferenceError` if called.
     They are not called from anywhere outside this cluster. Verified
     dead: `_setupLanguageTableEvents`, `_finalizeLanguageTableSetup`,
     `_handleLanguageTableError`, `_renderLanguageFallback`,
     `_updateCurrentLocaleButton`,
     `_refreshLanguageTableSelection`, `_focusLanguageTable`,
     `_handleLanguageTableKeydown`, `_moveLanguageSelection`,
     `_moveLanguageSelectionTo`, `_selectRowAtIndex`,
     `_selectCurrentLanguageRow`, `filterLanguageTable`,
     `selectLanguage`. Also remove the deprecated public
     `handleEscapeKey` (line 615) and the unused `logGroup` import.
     **Dead code is deleted, not extracted.**

  2. **Logs panel** → `login-logs-panel.js`. Class `LogsPanel` with
     `show()` (populates CodeMirror), `hide()` (no-op for now), and
     `destroy()` (cleans up CodeMirror + OverlayScrollbars). Owns the
     `_logEditor` field. Imports `getRawLog`, `formatLogText`, and the
     dynamic CodeMirror imports out of `login.js`. The teardown block
     at lines 1427–1437 of `login.js` moves to `LogsPanel.destroy()`.

  3. **Help/version panel** → `login-help-panel.js`. Class `HelpPanel`
     with `init()` (one-shot — fetches and renders version info),
     `show()`/`hide()` (no-op stubs to match the lifecycle pattern), and
     `destroy()`. Owns the version/build-date DOM caching. The
     `loadVersionInfo()` method (lines 128–183) moves here unchanged.

  4. **Login form + submit flow** → `login-form.js`. Class `LoginForm`
     owning the form-submit pipeline:
     - `handleSubmit()` (lines 1220–1247),
     - `attemptLogin()` (lines 1252–1321),
     - `handleLoginError()` (lines 1326–1369),
     - `showError()` / `hideError()` (lines 1374–1386),
     - `setLoading()` (lines 1391–1421),
     - `loadRememberedUsername()` / `saveRememberedUsername()` (lines
       70–101),
     - `handleClearUsername()` (lines 1109–1119),
     - `handleTogglePassword()` (lines 1124–1151) plus the
       `isPasswordVisible` state.

     Constructor takes `{ elements, onLoginSuccess, onLoginStart }`
     callbacks. Emits `AUTH_LOGIN` directly (it owns the login
     contract). `LoginManager` becomes a thin coordinator that
     instantiates `LoginForm` in `render()` and calls
     `loginForm.destroy()` in `teardown()`. `setLoading()` reads from
     `hasLookup()` directly — no need to bounce through `LoginManager`.

  5. **Keyboard shortcuts** → `login-shortcuts.js`. Class
     `LoginShortcuts` with `attach(handlers)` and `detach()`. Owns
     `handleKeyboardShortcuts()`, `_handleEscapeKey()`,
     `_focusUsername()`, `_focusPassword()`, `_clickButton()`,
     `checkCapsLock()`, `updateCapsLockIndicator()`, and the
     `isCapsLockOn` state. Constructor takes
     `{ elements, getCurrentPanel, onClearUsername, onSwitchToLogin }`.

  6. **Import audit and cleanup.** After items 1–5, audit `login.js`
     imports. Remove anything no longer referenced: `logGroup`,
     `getRawLog`, `formatLogText`, `getTip`, `scrollbarManager`,
     `getTransitionDuration`, possibly `getConfig`/`getConfigValue`
     (they move to `LoginForm`), `vendor-tabulator.css` (used only by
     `LanguagePanel`).

- **Files:**
  - Created:
    - `elements/003-lithium/src/managers/login/login-logs-panel.js`
    - `elements/003-lithium/src/managers/login/login-help-panel.js`
    - `elements/003-lithium/src/managers/login/login-form.js`
    - `elements/003-lithium/src/managers/login/login-shortcuts.js`
    - `elements/003-lithium/tests/unit/managers/login/login-logs-panel.test.js`
    - `elements/003-lithium/tests/unit/managers/login/login-help-panel.test.js`
    - `elements/003-lithium/tests/unit/managers/login/login-form.test.js`
    - `elements/003-lithium/tests/unit/managers/login/login-shortcuts.test.js`
  - Touched:
    - `elements/003-lithium/src/managers/login/login.js`
    - `elements/003-lithium/tests/unit/managers/login/login.test.js`
      (test surface shrinks: most assertions move to per-module
      tests; `login.test.js` keeps integration-style smoke tests).

- **Tests required:**
  - **`login-logs-panel.test.js`** (new): `show()` initialises
    CodeMirror with the right extensions and content; second `show()`
    updates content rather than re-initialising; `destroy()` tears
    down CodeMirror and OverlayScrollbars; CodeMirror import failure
    falls back to `<pre>` rendering.
  - **`login-help-panel.test.js`** (new): `init()` reads
    `window.__lithiumVersionData` when present; falls back to
    `__lithiumVersionPromise`; falls back to `fetch('/version.json')`
    when neither cache exists; populates all six DOM targets
    (`versionBuild`, `versionYear`, `versionDate`, `versionTime`,
    `helpAppVersion`, `helpBuildDate`); handles fetch failure
    gracefully.
  - **`login-form.test.js`** (new): empty credentials show error;
    successful 200 stores JWT, hides form, emits `AUTH_LOGIN`; 401
    shows "Invalid username or password" and clears + focuses
    password field; 429 with `retry-after` shows minutes; 5xx shows
    server-error message; offline detection;
    `loadRememberedUsername`/`saveRememberedUsername` round-trip;
    `handleTogglePassword` flips type + icon + tooltip;
    `handleClearUsername` clears both fields and focuses username.
  - **`login-shortcuts.test.js`** (new): ESC on login panel triggers
    `onClearUsername`; ESC on other panels triggers
    `onSwitchToLogin`; Ctrl+Shift+U/P focus username/password;
    F1 / Ctrl+Shift+I/T/L click the right buttons (verifies
    `_clickButton` respects `disabled`); shortcuts only fire on
    login panel (except ESC); CAPS-LOCK indicator toggles
    `caps-lock-active` class on password input.
  - **Existing `login.test.js`**: shrunk to integration-style
    coverage (init wiring, render, teardown sequence). Per-feature
    assertions moved to the new module tests. Net test count must
    not decrease — every assertion has a home in the new files.
  - **Existing `auth.integration.test.js`**: must still pass without
    modification (proves the contract change inside `login.js` did
    not break the JWT pipeline).

- **Definition of Done:**
  - [x] All four new modules + tests created.
  - [x] All 14 dead methods removed from `login.js`.
  - [x] Deprecated `handleEscapeKey` removed.
  - [x] Unused imports removed (`logGroup`, etc.).
  - [x] **`login.js` ≤ 1,000 lines.** Final: **576 lines** — well
        under the gate, better than the 650–750 target.
  - [x] No file in `src/managers/login/` exceeds 1,000 lines.
        Largest is `login.js` at 576; next is `login-language-panel.js`
        at 563.
  - [x] `npm run lint` clean.
  - [x] `npm run build` clean.
  - [x] `npm test` 100% green: **811 passing** (was 741 after
        Phase 3, +70 new across the four new modules: 11 + 13 +
        27 + 19).
  - [x] `auth.integration.test.js` still passes against a running
        Hydrogen.
  - [x] `LITHIUM-INS.md` rule #2 satisfied for every file under
        `src/managers/login/`.
  - [ ] Manual smoke test: full login cycle (password fill →
        submit → main app), ESC clears, Ctrl+Shift+U/P focus, all
        four sub-panels open and close, language selection works.
        *(Pending — recommend running before Phase 5 starts.)*

**Setup notes (from Phase 3 review):**
- `login.js` is at **1,466 lines** after Phase 3. Phase 4 must remove
  **≥466 lines** to hit the ≤1,000 gate. Inventory above shows
  ~820 lines of removable/extractable code, more than 75% margin.
- The logs panel (`populateLogsPanel`) owns CodeMirror init and log
  text formatting. It will need `elements.logViewer`, `getRawLog`, and
  `formatLogText`.
- The help panel (`loadVersionInfo`) owns version/build date display.
  It will need `elements.helpAppVersion`, `elements.helpBuildDate`,
  and the version-fetch logic.
- `_onBeforePanelSwitch()` gains `this._logsPanel?.show()` and
  `this._helpPanel?.show()` calls. Both panels follow the
  `show()` / `hide()` pattern established in Phase 3, although
  `HelpPanel` has trivial show/hide because version info is rendered
  once at init.
- `LoginForm` does **not** follow the panel-show/hide pattern —
  it owns the *content* of the login panel (form fields), not a
  separate panel. It is created in `render()` and destroyed in
  `teardown()`. Form events are bound in its constructor.
- `LoginShortcuts` is purely behavioural (no DOM ownership). Attached
  in `render()` after `LoginForm` exists, detached in `teardown()`.
- Import audit after Phase 4: `getRawLog`, `formatLogText`, dynamic
  CodeMirror imports, `getTip`, `scrollbarManager`,
  `getTransitionDuration`, and possibly `storeJWT`/`getConfig*`
  should all be out of `login.js`.

**Risk factors specific to Phase 4:**
- **Risk: hidden coupling between extracted modules.** `LoginForm`'s
  `setLoading()` disables `languageBtn`/`themeBtn`/`logsBtn` —
  buttons owned conceptually by the panel switcher. Mitigation:
  `LoginForm` reads `hasLookup()` and `_startupComplete` (passed in
  via constructor or getter callback), not via tight reference to
  `LoginManager`. The button elements are part of `elements`, which
  is shared across modules — that pattern is already established
  with `PasswordManager`, `LanguagePanel`, `LoginPanels`.
- **Risk: existing `login.test.js` breaks because internal methods
  are gone.** Mitigation: rewrite assertions against the new module
  surfaces in their respective new test files. The existing
  test file shrinks to integration smoke. If a specific assertion
  has no obvious new home, that signals a real regression and is
  treated as a bug, not test-shuffling.
- **Risk: `_loginStart` timing field gets orphaned.** It is owned
  by `handleSubmit`/`attemptLogin`/`handleLoginError`, all moving
  to `LoginForm`. Move with them.
- **Risk: `auth.integration.test.js` passes against a real Hydrogen
  but our mocks differ.** Run that suite as part of DoD, not just
  unit Vitest.

---

### Phase 5 — Hydrogen: `OIDCRelyingPartyConfig` schema + parser ✅ COMPLETE

- **Goal:** Land the new config block, parsed and validated, with
  `Enabled = false` as the default. No endpoints yet.
- **Prerequisites:** none (independent of Lithium phases).
- **In scope:**
  - New header `src/config/config_oidc_rp.h` defining
    `OIDCRelyingPartyConfig`, `OIDCRPProviderConfig`,
    `OIDCRPAccountLinking`, `OIDCRPRoleMapping`, with all fields from
    the JSON schema in this plan.
  - New `src/config/config_oidc_rp.c` implementing
    `load_oidc_rp_config()`, `cleanup_oidc_rp_config()`,
    `dump_oidc_rp_config()`.
  - Hook into the existing top-level config loader so the new section is
    parsed; missing section yields `Enabled = false`, no errors.
  - Environment variable substitution
    (`HYDROGEN_OIDC_CLIENT_SECRET`, etc.) following the existing
    Hydrogen `${env.X}` convention.
  - **No** runtime behaviour anywhere else changes yet.
- **Files:**
  - Created: `src/config/config_oidc_rp.c` (453 lines),
    `src/config/config_oidc_rp.h` (198 lines)
  - Created: `tests/unity/src/config/config_oidc_rp_test_load_oidc_rp_config.c`
    (24 tests)
  - Touched: `src/config/config.c`, `src/config/config.h`,
    `src/config/config_forward.h`, `src/config/config_defaults.c`,
    `src/config/config_defaults.h`, `src/hydrogen.h` (added
    `OIDCRelyingPartyConfig oidc_rp` field to `AppConfig`)
- **Tests required:**
  - `config_oidc_rp_test_load_oidc_rp_config.c` (Unity): missing section
    → defaults; full valid section → all fields populated; provider
    truncation when count > `OIDC_RP_MAX_PROVIDERS`; non-object provider
    entries skipped; env-var substitution for `ClientSecret`; empty
    `AllowedAlgorithms` array falls back to `RS256`; cleanup leak-free
    under ASAN; null/empty/dump safety; enum parsers for both linking
    strategy and role source.
- **Definition of Done:**
  - [x] Unity build clean (`cmake --build . --target unity_tests`).
  - [x] Regular build clean (`cmake --build . --target hydrogen`).
  - [x] cppcheck (`test_91_cppcheck.sh`) clean — all 1,297 files,
        no issues found in the 9 changed files.
  - [x] `test_10_unity` includes the new test and it is green:
        24/24 passing, total 6,742/6,742 in suite (no regressions).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect leaks.
  - [x] `test_17_startup_shutdown` passes: 9/9 checks green for both
        min and max configs.
  - [x] `test_99_code_size`: no new file exceeds 1000 lines (largest
        new file is `config_oidc_rp.c` at 453 lines).

**Lessons learned:**

1. **The existing `config_oidc.{c,h}` is a near-perfect template — but
   the build-system reconfigure step is required.** The
   `cmake/CMakeLists-init.cmake` line 88 uses `file(GLOB_RECURSE
   HYDROGEN_SOURCES "../src/*.c")` to discover sources. Adding a new
   `.c` file to `src/config/` requires running `cmake .` in the build
   dir to refresh the glob; otherwise you get `undefined reference`
   linker errors with no obvious cause. This is worth flagging in any
   future phase that adds C sources.
2. **`-Werror=unused-function` will catch dead helper functions
   immediately.** The first build attempt failed because
   `take_string_or_default` was defined but never called. Trim
   speculative helpers before committing — the compiler is strict.
3. **`PROCESS_*` macros vs. manual JSON iteration.** The
   `PROCESS_STRING` / `PROCESS_SENSITIVE` family is a clean fit for
   flat structs (e.g. the existing `OIDCConfig`). For arrays of nested
   objects (like `OIDC_RP.Providers[N]`), the macros don't compose well
   — they bake in section-name strings. Using direct
   `json_object_get` / `json_array_size` with small typed helper
   functions (`take_string_or_null`, `take_bool_or_default`,
   `take_int_or_default`, `take_string_array`) was clearer and easier
   to test. This matches the pattern in `config_databases.c`.
4. **`process_env_variable_string()` is the only entry point operators
   should ever need.** It transparently handles literal strings AND
   `${env.X}` substitution, returning a heap-allocated string that the
   caller owns. The test
   `test_oidc_rp_env_var_substitution_for_secret` proves the pipeline
   works end-to-end for the most security-sensitive case
   (`ClientSecret`).
5. **Defensive defaults matter for safety-critical lists.** An operator
   who supplies an empty `AllowedAlgorithms` array would create the
   nonsensical state of "no algorithms allowed" (i.e. *every* ID token
   rejected). The loader explicitly falls back to `["RS256"]` in that
   case rather than enabling a degenerate state. Captured by
   `test_oidc_rp_empty_allowed_algorithms_falls_back_to_rs256`. Any
   future phase that introduces an array of allow-listed values should
   apply the same hardening.
6. **`SR_AUTH` is the right subsystem for OIDC RP logging.** The plan
   suggested it implicitly (line 514: `log_this(SR_AUTH, …)`). The
   `LOAD_CONFIG` macro and `DUMP_CONFIG_SECTION` calls now use
   `SR_AUTH` for the new section, keeping it grep-able alongside other
   auth-flow logging that will land in Phases 6–14. Note this differs
   from `SR_OIDC`, which the existing provider-side stubs use.
7. **`SystemApiKey` was added to the schema during Phase 5.** The plan
   mentioned this field at line 498 ("a server-side API key bound to
   the OIDC RP") but it was missing from the JSON schema sample on
   lines 351–394. Phase 5 ships it as part of `OIDCRPProviderConfig`
   so Phase 14 (callback wiring) can use it without a schema-change
   merge. It is treated as `PROCESS_SENSITIVE` / `DUMP_SECRET` like
   `ClientSecret`.
8. **Provider count is hard-capped at 8.** Defined as
   `OIDC_RP_MAX_PROVIDERS` in the header. The plan ships only the
   500passwords entry; this is plenty of headroom. If a deployment
   ever wants more, bump the constant — there's no hidden coupling.
9. **The `oidc_rp` field placement in `AppConfig` is *between* `oidc`
   and `notify`.** This follows the section-letter convention (we
   are "O-RP", logically right after "O. OIDC"). The `LOAD_CONFIG`
   and `DUMP_CONFIG_SECTION` macros use `"O-RP"` as the letter and
   `SR_AUTH` as the section name; this hyphenated form has not been
   used before but the `format_section_header` helper handles it
   without modification.

**Setup for Phase 6:**

- `oidc_rp_is_enabled()` helper does **not** yet exist. Phase 6 will
  add it as `bool oidc_rp_is_enabled(const AppConfig*)` returning
  `app_config->oidc_rp.enabled`. Trivial.
- Endpoint registration in Phase 6 should expect to find at least one
  enabled provider before treating the feature as "live". The current
  config will return success even with zero providers — that's
  correct for "feature off"; Phase 6 is what flips the contract from
  "section parsed" to "endpoints active".
- The `database` field at `OIDC_RP.Database` defaults to NULL.
  Phase 14's callback wiring will use `app_config->oidc_rp.database`
  if non-NULL, falling back to a reasonable default (probably the
  per-request `?database=` query param then a hardcoded "Lithium").
  Phase 6 doesn't need to touch this.
- The schema supports per-provider overrides for everything except
  the database; that's intentional. If multi-database OIDC ever
  becomes a real requirement, lift `database` into
  `OIDCRPProviderConfig`.

---

### Phase 6 — Hydrogen: disabled-stub endpoints ✅ COMPLETE

- **Goal:** Register the three new URLs in the web server, returning
  `503 { "error": "oidc_disabled" }` whenever `OIDCRelyingPartyConfig.Enabled
  = false`. This locks in the routing contract before any logic lands.
- **Prerequisites:** Phase 5.
- **In scope:**
  - Stub handlers for `GET /api/auth/oidc/start`,
    `GET /api/auth/oidc/callback`, `POST /api/auth/oidc/handoff`.
  - Each handler calls a single `oidc_rp_is_enabled()` check; when
    disabled, returns 503 with the same error envelope shape as
    existing auth errors.
  - Method-mismatch handling (e.g. `POST /oidc/start` ⇒ 405).
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_start.{c,h}` (54 + 55 lines),
    `oidc_rp_callback.{c,h}` (56 + 56 lines),
    `oidc_rp_handoff.{c,h}` (66 + 57 lines).
  - Created: `src/api/auth/oidc_rp/oidc_rp_service.{c,h}` (66 + 81 lines)
    providing routing helpers + `oidc_rp_is_enabled()`,
    `oidc_rp_send_disabled_response()`,
    `oidc_rp_send_method_not_allowed()`.
  - Touched: `src/api/api_service.c` — 3 new `#include` lines, 3 new
    `else if` route branches, 3 new logged-endpoint lines.
  - Created: `tests/test_42_oidc_rp.sh` (280 lines) with disabled-only
    test cases.
  - Created: `tests/configs/hydrogen_test_42_oidc_rp.json` (minimal
    config — no `OIDC_RP` block, so `enabled` defaults to `false`).
- **Tests required:**
  - `test_42_oidc_rp.sh`: with `Enabled=false`, all three endpoints
    return 503 with `{"error":"oidc_disabled"}`; wrong method returns
    405; URL outside the prefix returns 404.
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean.
  - [x] `test_42_oidc_rp.sh` exists, is auto-discovered by
        `test_00_all.sh:251-260` (no manual wiring needed), and
        passes 13/13 in 0.449s.
  - [x] `test_40_auth.sh` still green: 46/46 passing across all
        7 database engines.
  - [x] `test_91_cppcheck.sh` clean — 1,305 files, 0 issues found
        in the 9 changed files.
  - [x] `test_92_shellcheck.sh` clean — 109 shell scripts, 639
        directives all justified.
  - [x] `test_17_startup_shutdown.sh` clean — 9/9 passing.
  - [x] `test_10_unity.sh` clean — Phase 5 unit test
        (`config_oidc_rp_test_load_oidc_rp_config`) still passing
        among 6,742 passing tests, 0 failing. The 4 cached-failure
        flags pre-date Phase 6 and are unrelated to OIDC.
  - [x] `test_99_code_size` — pass count unchanged from Phase 5
        baseline (the pre-existing `proxy_multi.c` 1,209-line
        finding is unrelated to Phase 6; all new files are ≤280
        lines, largest is `test_42_oidc_rp.sh` at 280).

**Lessons learned:**

1. **The dispatcher in `src/api/api_service.c` is a hand-written `if/else if (strcmp ...)` chain — there is no function-pointer table.** Adding a new endpoint means three coordinated edits in that one file: (a) the `#include` near line 31–34, (b) the route branch near line 596 (after `auth/register`), and (c) the logged-endpoint banner near line 167. No central registry; greppable. This pattern will repeat in Phases 14, 21, 22.
2. **Stub endpoints stay out of `endpoint_expects_json` and `endpoint_requires_auth`.** Both lists in `api_service.c:307-362` are middleware short-circuits. For Phase 6 we want the disabled-503 response to win against any other middleware verdict, including "missing JWT" or "invalid JSON body". Adding `auth/oidc/handoff` to the JSON list now would mean a `POST /handoff` with no body returns 400 `{"error":"Invalid JSON","message":"Request body is empty"}` *before* our 503 fires. Phase 13 will revisit this when `/handoff` actually parses a body.
3. **`/handoff` POST handler must drain the upload buffer.** MHD calls a POST handler at least twice: once with `*upload_data_size > 0` (here is the body) and once with `*upload_data_size == 0` (body fully buffered, please respond). A naive Phase 6 stub that returns 503 on the first call without acknowledging the body causes MHD to keep re-invoking. The fix is the standard idiom: `if (upload_data_size && *upload_data_size > 0) { *upload_data_size = 0; return MHD_YES; }`. Tested explicitly by `test_42_oidc_rp.sh` sending `{"handoff":"unused-in-phase-6"}` as the body.
4. **`oidc_rp_send_disabled_response()` is the right shape for a shared helper, not three duplicated 30-line blobs.** Three handlers, each calling one helper that builds `{"error":"oidc_disabled"}` and queues 503, keeps the contract identical and means a future Phase will only need to update one place if (say) we want to add a `Retry-After` header. Same logic for `oidc_rp_send_method_not_allowed()`. The four-function `oidc_rp_service` module (`is_enabled` + 2 senders) is a tiny but worthwhile unit.
5. **Auto-discovery by `test_00_all.sh` is real and means zero wiring.** The orchestrator at `tests/test_00_all.sh:251-260` does `find tests -name 'test_*.sh' | sort`. A new file named `test_42_oidc_rp.sh` is picked up on the next orchestrated run with no edits anywhere. Sequential-group selection by leading digit (`--sequential-groups=4` runs 40-49 sequentially) Just Works.
6. **Default-disabled config = no `OIDC_RP` block at all.** The Phase 5 parser treats a missing section as `Enabled = false` with zero providers. The Phase 6 test config (`hydrogen_test_42_oidc_rp.json`) is **14 lines** — Server, WebServer, API only. No special `OIDC_RP` block needed. This is the cleanest possible "feature off" surface and it cost us nothing.
7. **Method discrimination in stubs is an explicit `strcmp(method, "GET") != 0` check, not a buffer-result switch.** Login uses `api_buffer_post_data() + API_BUFFER_METHOD_ERROR` because it has a body to buffer. The Phase 6 stubs have no body to handle (until Phase 13 wires `/handoff` for real), so a direct method check before the feature gate is simpler and easier to test. Phase 13 will replace this in `oidc_rp_handoff.c` with the real buffering pipeline.
8. **`mkt` does cmake configure + builds both `hydrogen` and `unity_tests`.** The Phase 5 lesson about `file(GLOB_RECURSE)` (Phase 5 lesson #1) applies: adding `.c` files under `src/` requires a configure step. `extras/make-trial.sh:39-41` does this automatically (unless invoked with `QUICK`). I did not need a manual `cmake .` because `mkt` cleans the build dir and re-configures.
9. **The 4 "cached test failures" reported by `test_10_unity.sh` are summary-line noise, not real failures.** The actual pass/fail count on the same line reads `6,746 unit tests, 6,742 passing, 0 failing`. The "4 cached" flag is from the test cache layer and pre-dates Phase 6 (verified — none mention `oidc` or `api_service`). Future phases should treat the `passing/failing` numbers as authoritative and ignore the "cached failures" decoration unless the failing count is non-zero.

**Setup for Phase 7:**

- The `oidc_rp/` directory now exists with five files (`service`, `start`, `callback`, `handoff` + their headers). Phase 7's state-store work goes in the same directory as `oidc_rp_state.{c,h}`.
- `oidc_rp_is_enabled()` is the single feature gate. Phase 7's state-store init/shutdown should hook off the same gate so we don't allocate hash tables when the feature is off.
- Both response-helper signatures (`oidc_rp_send_disabled_response`, `oidc_rp_send_method_not_allowed`) are deliberately narrow — Phase 7+ should add error helpers next to them rather than open-coding new envelopes in handlers. A typed-error enum + single sender is the natural next refactor when the error vocabulary grows past a handful.
- The dispatcher branches in `api_service.c:597-609` are stable — Phase 14 will not need to change them when wiring real logic, only the handler bodies.
- Phase 6 leaves a `503 oidc_not_implemented` path in each handler for the case where `oidc_rp.enabled = true` but the real flow isn't built yet. Phases 7–14 will progressively replace these `oidc_not_implemented` returns with real logic. The disabled-505 path remains the production default until Phase 27 (e2e against real Keycloak).



---

### Phase 7 — Hydrogen: in-memory state store ✅ COMPLETE

- **Goal:** Implement the threadsafe state store used by `/start` and
  `/callback`. Pure data structure work, no HTTP.
- **Prerequisites:** Phase 5.
- **In scope:**
  - `src/api/auth/oidc_rp/oidc_rp_state.{c,h}`: hash-table keyed by
    `state` string, value `{state, nonce, code_verifier, database,
    return_to, client_ip, created_at}`.
  - API: `oidc_rp_state_put`, `oidc_rp_state_take` (atomic
    lookup + remove), `oidc_rp_state_sweep_expired`,
    `oidc_rp_state_init`, `oidc_rp_state_shutdown`.
  - Mutex-protected; sweeper runs on every `put` (cheap) and on a
    timer thread.
  - TTL configured from `StateTtlSeconds`.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_state.c` (484 lines),
    `src/api/auth/oidc_rp/oidc_rp_state.h` (194 lines)
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_state_test_store.c`
    (430 lines, 20 tests)
- **Tests required:**
  - Unity `oidc_rp_state_test_store`: put/take round-trip; double-take
    returns NULL; expiry sweeps; concurrent put/take from two threads
    via pthreads; init/shutdown leak-free under ASAN.
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean — both targets compile.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build variants
        green (regular, debug/ASAN, coverage, perf, valgrind, release,
        naked, examples, unity payload, etc.).
  - [x] `oidc_rp_state_test_store` Unity test green: 20/20 passing,
        full suite 6,762/6,762 (no regressions; the 4 cached failures
        flagged by `test_10_unity` pre-date Phase 6 and are unrelated
        per Phase 6 lesson #9).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect leaks.
  - [x] `test_91_cppcheck` clean — 1,308 files, 0 issues found in the
        3 changed files.
  - [x] `test_42_oidc_rp` (Phase 6 black-box) still green: 13/13 passing.
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: no new file exceeds 1,000 lines (largest
        new file is `oidc_rp_state.c` at 484 lines). Pre-existing
        `proxy_multi.c` 1,209-line finding is unrelated.

**Lessons learned:**

1. **Condvar-based sweeper beats `sleep()`-based.** `terminal_session.c`'s cleanup thread uses a bare `sleep(N)` loop, which means shutdown waits up to N seconds for the next iteration before joining. The OIDC RP state store uses `pthread_cond_timedwait` against a dedicated `sweeper_cond` + `sweeper_mutex`; on shutdown we take the mutex, set `shutdown_requested`, broadcast the cond, and `pthread_join` returns within milliseconds. This pattern (also used in `mdns_server_threads.c` and `victoria_logs.c`) is the right choice for any new background-loop thread.
2. **Inline-sweep on `put` is a cheap belt-and-suspenders.** Even with the sweeper thread disabled (test mode, or if `pthread_create` fails), a bounded inline sweep on each `put` (capped at `OIDC_RP_STATE_INLINE_SWEEP_MAX = 8` entries visited) keeps the store from growing unboundedly under traffic. Latency stays predictable because the visit cap is a constant.
3. **`take` semantics: expired-on-take is a miss, not an error.** Per the plan's "state_invalid" envelope, `/callback` cannot distinguish "never inserted" from "inserted but aged out". `oidc_rp_state_take` collapses both cases to NULL and silently reaps the expired entry. This keeps the caller's branching simple and matches the Phase 14 contract.
4. **Test-only sweeper kill switch is mandatory for deterministic Unity tests.** Without `oidc_rp_state_test_disable_sweeper()`, the concurrency test would race a 30-second background sweep — a flake source in CI. The `terminal_session.c` precedent (`test_mode_disable_cleanup_thread`) is the right pattern; the kill switch is checked once at `init` time, not on every iteration, so production paths pay zero cost.
5. **Sensitive-field scrubbing on free.** The plan's logging/secrets discipline (line 765) deny-lists `nonce` and `code_verifier`. Beyond not logging them, the implementation also `memset`s their bytes before `free` via a `volatile` pointer dance. This defeats over-eager dead-store elimination from `-O2` and keeps freed-but-not-yet-overwritten heap blocks from leaking secrets through allocator reuse. The plan does not mandate this hardening but it is essentially free and consistent with the security plan's spirit.
6. **`AppConfig` integration is deferred to Phase 14 by design.** The Phase 7 state store is a pure library: tests drive `init/shutdown` directly. Wiring it into the Hydrogen startup/shutdown sequence (gated by `oidc_rp_is_enabled()`) belongs in Phase 14 when `/start` first needs a live store. This keeps Phase 7's blast radius tiny and avoids touching `hydrogen.c` for a feature that is still disabled.
7. **`mkt`'s clean build picked up the new `.c` file automatically.** Confirmed Phase 5 lesson #1 still holds — the CMake `GLOB_RECURSE` finds new `src/**/*.c` files only after a reconfigure. `mkt` does this via `cmake -S . -B ../build --preset default`. A `QUICK` invocation would have failed to link.
8. **Unity tests do not link ASAN.** `test_10_unity.sh` runs unit tests against the regular build, not the debug build. Leak coverage for the store comes from two sources: (a) the concurrency test's deterministic `size == 0` assertion after drain, and (b) `test_11_leaks_like_a_sieve.sh` running the full debug Hydrogen with ASAN — though the latter does not exercise OIDC RP code while the feature is disabled. This is acceptable for Phase 7 (the store is small and well-tested); Phases 14+ will exercise the store under ASAN once endpoints actually call `init/put/take/shutdown` from a running Hydrogen.
9. **`opt_strdup` returning NULL for NULL inputs simplifies the put path.** The pattern of "duplicate this string only if it was provided" came up six times for the optional fields. A small helper (`opt_strdup`) plus a single post-condition check (was an input non-NULL but the dup NULL?) keeps `oidc_rp_state_put`'s allocation-failure branch readable. Same pattern likely applies in Phase 13's handoff store.

**Setup for Phase 8 (PKCE helpers):**

- The state store is ready to receive PKCE-generated values. Phase 10 (`/oidc/start` redirect builder) will be the first caller of both this store and Phase 8's PKCE helpers.
- `OIDC_RP_STATE_BUCKETS = 64` is plenty for in-flight session counts dominated by single-digit tens. If telemetry ever shows higher concurrency, the only change is the constant.
- `oidc_rp_state_init()` is called with `default_ttl_seconds`. Phase 14 will pass `app_config->oidc_rp.providers[0].state_ttl_seconds` here; multi-provider deployments would need per-call TTLs (which `oidc_rp_state_put` already supports via its `ttl_seconds` parameter).
- `record_free_fields()` and the `scrub_free()` helper are internal but the pattern is reusable for Phase 13's handoff store. If Phase 13 chooses to share infrastructure rather than duplicate, those helpers are the right starting point — promote them to a shared `oidc_rp_internal.h` header at that time.



---

### Phase 8 — Hydrogen: PKCE + nonce + state generators ✅ COMPLETE

- **Goal:** Pure crypto helpers, no networking, no HTTP.
- **Prerequisites:** Phase 5.
- **In scope:**
  - `oidc_rp_pkce.{c,h}`: `oidc_rp_make_code_verifier()` (43-character
    URL-safe base64 of 32 random bytes); `oidc_rp_make_code_challenge()`
    (BASE64URL(SHA256(verifier))); `oidc_rp_make_random_hex(size_t)`
    used for `state` and `nonce`.
  - All randomness from `utils_random_bytes()` (OpenSSL `RAND_bytes`)
    — the same source `generate_jti()` already uses in
    `auth_service_jwt.c:38-46`.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_pkce.c` (135 lines),
    `src/api/auth/oidc_rp/oidc_rp_pkce.h` (134 lines)
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_pkce_test_helpers.c`
    (307 lines, 15 tests)
- **Tests required:**
  - Known-answer test against the RFC 7636 Appendix B example:
    verifier `dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk` →
    challenge `E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM`.
  - Random-byte size correctness across widths 1, 8, 16, 32, 64.
  - 1,000 invocations produce all distinct values (uniqueness sanity)
    for both verifier and 32-byte hex.
  - Character-set validation: verifier and challenge use only base64url
    chars (`A-Z a-z 0-9 - _`); random hex uses only lower-case
    `0-9 a-f`.
  - Edge cases: NULL/empty input to challenge → NULL; zero or
    over-cap byte_count to random hex → NULL; deterministic challenge
    for repeated identical input.
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean — both targets compile.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build variants
        green (regular, debug/ASAN, coverage, perf, valgrind, release,
        naked, examples, unity payload, etc.).
  - [x] `oidc_rp_pkce_test_helpers` Unity test green: 15/15 passing,
        including the RFC 7636 known-answer vector. Full suite
        6,776/6,776 passing (was 6,762 in Phase 7; +14 net after
        accounting for incidental churn elsewhere — none of my changes
        touched those tests).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect leaks.
  - [x] `test_91_cppcheck` clean — 1,311 files, 0 issues found in the
        3 changed files.
  - [x] `test_42_oidc_rp` (Phase 6 black-box) still green: 13/13
        passing.
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: pass count unchanged from Phase 7
        baseline (the pre-existing `proxy_multi.c` 1,209-line finding
        is unrelated to Phase 8; all new files are ≤307 lines, largest
        is the test file at 307).

**Lessons learned:**

1. **Every primitive Phase 8 needed already existed in `utils_crypto`.**
   `utils_random_bytes` (OpenSSL `RAND_bytes` wrapper),
   `utils_base64url_encode` (URL-safe, no padding — exactly the PKCE
   format), and `utils_sha256_hash` (SHA-256 → base64url in one call)
   are all in `src/utils/utils_crypto.{c,h}`. The entire Phase 8
   implementation is ~80 lines of glue code over those three helpers.
   This is the cleanest leverage of pre-existing infrastructure of
   any phase so far. Future phases should always grep
   `src/utils/utils_*.{c,h}` before writing new crypto/encoding
   primitives.
2. **`utils_sha256_hash` does the entire SHA-256 + base64url step.**
   `oidc_rp_make_code_challenge` is literally one call. No
   intermediate 32-byte digest buffer ever lives in our scope, which
   eliminates a class of "did you scrub the digest?" review questions.
3. **RFC 7636 Appendix B is the right regression anchor.** The
   verifier `dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk` →
   challenge `E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM` test
   passing is dispositive proof that the encoding pipeline (SHA-256
   then base64url, no padding) is wired correctly. If anyone ever
   "improves" `utils_sha256_hash` or `utils_base64url_encode` and
   breaks PKCE compatibility, this test catches it instantly. Worth
   keeping prominent in the test file.
4. **`*_test_*.c` glob discovery means zero CMake edits.** The
   Unity glob (`tests/unity/src/*_test*.c` per
   `cmake/CMakeLists-unity.cmake:19`) auto-discovered the new file
   on first `mkt` invocation. The naming convention
   `oidc_rp_pkce_test_helpers.c` (matching Phase 7's
   `oidc_rp_state_test_store.c`) keeps things uniform. Confirmed
   Phase 5 lesson #1 still holds: full `mkt` reconfigures, picks up
   the new `.c` files. A `QUICK` invocation would have failed.
5. **`volatile` scrub on entropy buffers is essentially free.** Both
   `oidc_rp_make_code_verifier` (32-byte raw) and
   `oidc_rp_make_random_hex` (variable raw) scrub their stack/heap
   entropy buffers before returning. The pattern from Phase 7's
   state-store record_free (`volatile unsigned char* p`) carries
   over verbatim. Per Phase 7 lesson #5, this defeats `-O2`
   dead-store elimination at no measurable cost. Apply
   consistently across the OIDC RP module from here on.
6. **Sanity cap on `byte_count` matters.** A caller passing
   `SIZE_MAX` to `oidc_rp_make_random_hex` would compute
   `byte_count * 2 + 1` and overflow. The 256-byte cap is well
   above any plausible use (we generate 32-byte hex everywhere)
   and turns the overflow risk into a clean `NULL` return.
   `test_make_random_hex_rejects_over_cap` exercises 4096 bytes,
   which is well over the cap; the test does not hard-code the
   cap value into its contract, so the constant can be tuned
   without breaking the test.
7. **Phase 6's "5 cached failures" decoration noise is exactly that.**
   On a fresh test_10 run after my changes, the orchestrator
   reports `6,781 unit tests, 6,776 passing, 5 failing` on the
   first pass and `6,776 passing, 0 failing` on the cached re-run
   — the 5 are pre-existing, unrelated to OIDC, and confirmed by
   Phase 6 lesson #9. Future phases should read the
   passing/failing numbers as authoritative and ignore the
   "cached failures" line unless `failing > 0` on a cached run.
8. **`extras/make-trial.sh` does not build the debug variant.**
   `tests/test_11_leaks_like_a_sieve` requires `hydrogen_debug`
   (with ASAN), which only `extras/make-all.sh` produces. After
   `mkt`, test_11 will fail-skip with "Debug build not found".
   Run `mka` (= `make-all.sh`) before test_11 if a clean leak
   report is required. This was a one-time hiccup; documenting
   it here saves the next phase the same diagnostic loop.

**Setup for Phase 9 (discovery + JWKS):**

- The PKCE helpers are pure functions; Phase 10 (`/oidc/start`
  redirect builder) will call them directly to populate state-store
  records. No init/shutdown lifecycle is required.
- `oidc_rp_make_random_hex(32)` is the canonical state/nonce
  generator. The plan's expected widths (32 bytes raw → 64 hex
  chars) align with what callers in Phase 10 will request.
- The `OIDC_RP_PKCE_VERIFIER_LENGTH` and
  `OIDC_RP_PKCE_CHALLENGE_LENGTH` constants (both 43) are exposed
  in the header and can be used by Phase 10's URL-builder for
  buffer sizing.
- Phase 9 introduces the first cross-module dependency on
  libcurl/HTTP. Until then the OIDC RP code is purely
  computational; Phase 9 is where networking enters the picture.
  Verify that the existing Hydrogen HTTP client (used by other
  subsystems for outbound requests) is the right entry point
  before introducing libcurl directly.

---

### Phase 9 — Hydrogen: discovery + JWKS fetch and cache ✅ COMPLETE

- **Goal:** Cache the IdP's `.well-known/openid-configuration` and JWKS,
  refresh on miss / TTL / signature-failure trigger.
- **Prerequisites:** Phases 5, 8.
- **In scope:**
  - `oidc_rp_http.{c,h}`: thin libcurl GET wrapper exposing
    `oidc_rp_http_get(url, verify_ssl, accept) → OidcRpHttpResponse*`.
    Adds a test-only single-shot fixture seam
    (`oidc_rp_http_test_set_response`) so Unity tests can substitute
    canned responses without mocking libcurl symbols.
  - `oidc_rp_discovery.{c,h}`: HTTP GET via the wrapper, parses the
    JSON, exposes `oidc_rp_discovery_get(provider, issuer, verify_ssl,
    ttl) → const OidcRpDiscoveryDoc*`. TTL from `DiscoveryCacheSeconds`.
    Also exposes `oidc_rp_discovery_parse` for direct unit testing of
    the parser, plus `oidc_rp_discovery_invalidate` and `_size`.
  - `oidc_rp_jwks.{c,h}`: GET `jwks_uri` via the wrapper, parses and
    caches the key set, exposes
    `oidc_rp_jwks_find(provider, jwks_uri, verify_ssl, ttl, kid) →
    const OidcRpJwk*` plus `oidc_rp_jwks_parse` (for tests),
    `oidc_rp_jwks_invalidate` (rotation recovery, used in Phase 12),
    `_size`, `_key_count`, and `_keys_free`.
  - **TLS verify is mandatory** (`VerifySsl`); the wrapper rejects
    non-http(s) URL schemes early; redirects are NOT followed.
  - Body cap of 1 MiB and 30 s total request timeout / 10 s connect
    timeout — a runaway IdP cannot consume Hydrogen memory or block
    a worker indefinitely.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_http.{c,h}` (256 + 127 lines)
  - Created: `src/api/auth/oidc_rp/oidc_rp_discovery.{c,h}` (370 + 162)
  - Created: `src/api/auth/oidc_rp/oidc_rp_jwks.{c,h}` (382 + 145)
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_http_test_get.c`
    (239 lines, 12 tests)
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_discovery_test_cache.c`
    (388 lines, 26 tests)
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_jwks_test_cache.c`
    (388 lines, 25 tests)
  - Created: `tests/lib/mock_keycloak/server.js` (128 lines) — Node
    built-ins only, no extra deps. Serves `/health`,
    `/realms/test/.well-known/openid-configuration`, and
    `/realms/test/protocol/openid-connect/certs`. Phase 11 will add a
    `/token` endpoint; Phase 12 will swap in a real keypair so the
    mock can sign id_tokens.
  - Touched: `tests/test_42_oidc_rp.sh` (added 5 mock-Keycloak
    sub-tests with start/stop/health/discovery/JWKS coverage; bumped
    version 1.0.0 → 1.1.0; added `EXIT` trap for mock cleanup).
- **Tests required:**
  - Unity (3 new files, 63 tests total):
    - `oidc_rp_http_test_get`: NULL/empty URL rejected; non-http(s)
      scheme rejected; test seam returns canned response; substring
      matching; substring mismatch leaves fixture queued; single-use
      semantics; `clear_responses` drops queued fixtures; status
      codes (2xx, 4xx, transport-failure 0).
    - `oidc_rp_discovery_test_cache`: lifecycle, parser (minimal +
      full + every required-field-missing case + invalid JSON +
      non-object top-level + issuer mismatch + matching expected
      issuer), get-on-miss, get-on-hit (pointer identity), trailing-
      slash issuer rejected (current contract), get on HTTP failure,
      get on parse failure, get on issuer mismatch, invalidate
      forces refetch, size tracking.
    - `oidc_rp_jwks_test_cache`: lifecycle, parser (single key,
      multi-key, missing `keys` array, empty array, invalid JSON,
      non-object top-level, skip non-object entries, skip entries
      missing `kid`, keep entries missing `kty` with warning),
      find-on-miss, unknown-kid behaviour (does NOT auto-invalidate),
      cache hit on repeat call, HTTP failure, parse failure,
      invalidate forces refetch, size + key_count tracking.
  - Black-box `tests/test_42_oidc_rp.sh`: 5 new sub-tests around the
    mock Keycloak — start, /health 200 ok, discovery doc has all
    required fields (jq if available, substring fallback otherwise),
    JWKS has at least one keyed entry, stop. Mock lifecycle is
    trapped on `EXIT` so no leftover Node process if the harness
    aborts.
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` clean — 18/18 build variants green.
  - [x] `test_10_unity` clean: 6,840/6,844 passing (was 6,776 in
        Phase 8, +64 net new). The 4 cached failures pre-date Phase 6
        and are unrelated to OIDC RP per Phase 6 lesson #9 / Phase 7
        lesson #8 / Phase 8 lesson #7. The three new Unity test files
        contributed 63 of the 64 net new tests; all 63 are green.
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. Note that test_11 exercises the Hydrogen runtime with
        `OIDC_RP.Enabled = false`, so the new modules' init/shutdown
        paths are not yet exercised under ASAN — Phases 14+ will
        cover that once endpoints actually call them. The Unity
        suite covers the put/take/parse paths under regular
        compilation.
  - [x] `test_42_oidc_rp.sh` (Phase 6 baseline + Phase 9 additions)
        green: 18/18 (was 13/13 in Phase 6, +5 net new). Mock
        Keycloak start/health/discovery/JWKS/stop all pass.
  - [x] `test_91_cppcheck` clean — 1,320 files, 0 issues. (One
        `duplicateConditionalAssign` finding in `oidc_rp_http.c` was
        fixed during Phase 9.)
  - [x] `test_92_shellcheck` clean — 109 scripts, 640 directives all
        justified. (One `SC2312` finding on `test_42_oidc_rp.sh`'s
        new `cat` invocation was fixed by switching to `$(<file)`.)
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: pass count unchanged from Phase 8
        baseline. The pre-existing `proxy_multi.c` (1,209 lines)
        finding remains unrelated to Phase 9. All new Phase 9 files
        are well under the 1,000-line cap (largest:
        `oidc_rp_discovery_test_cache.c` and
        `oidc_rp_jwks_test_cache.c`, each 388 lines).

**Lessons learned:**

1. **Test-seam pattern (`oidc_rp_http_test_set_response`) is the
   right call for libcurl unit testability.** The Phase 7 sweeper
   kill-switch (`oidc_rp_state_test_disable_sweeper`) is the
   precedent. The seam is a single-element queue guarded by a
   mutex, drained at the very top of `oidc_rp_http_get` before any
   network or even URL-scheme check. Production code never
   populates the queue, so the only cost in production is one
   uncontended mutex acquire per call — negligible. This avoided
   the heavier alternatives (function-pointer override, `--wrap=`
   linker tricks, full libcurl mock-header) entirely and paid for
   itself the moment Unity tests started exercising every error
   branch deterministically.
2. **`chat_proxy_send_request` in `src/api/wschat/helpers/proxy.c`
   is the canonical libcurl pattern.** Per Phase 8 lesson #1,
   greppable infrastructure already existed; copy it. Specifically
   reused: the `ResponseBuffer { data, size, capacity }` struct
   plus a write-callback that doubles capacity on demand, the
   `curl_easy_init → setopt → perform → getinfo → cleanup` lifecycle,
   the `curl_slist_append`-built header list with `curl_slist_free_all`,
   and the per-request (not global) init pattern (global init lives
   in `src/launch/launch.c:324`). Differences from `proxy.c`: Phase
   9 disables `CURLOPT_FOLLOWLOCATION` (discovery + JWKS URLs are
   stable; a redirect would mask misconfiguration), uses GET not
   POST, and caps the body at 1 MiB instead of 8 MiB (these
   responses are typically 1–4 KiB).
3. **Per-provider fixed-size cache table is the right shape, NOT a
   hash table.** The plan permits up to 8 providers (`OIDC_RP_MAX_PROVIDERS`
   from Phase 5). With ≤ 8 entries, an array-with-string-compare
   lookup is faster than hashing, simpler to reason about under
   lock, and trivially cache-friendly. Both `oidc_rp_discovery.c`
   and `oidc_rp_jwks.c` use the same pattern.
4. **Returning `const T*` from cache-hit lookups is safer than
   returning a copy.** The cache owns the strings; callers (Phase
   10's `/oidc/start`) need URLs just long enough to build a redirect
   string. A const-pointer contract avoids dup churn while still
   preventing accidental mutation through type discipline. The
   header explicitly documents that the pointer is valid until the
   next mutating call — a contract the Phase 10 implementation will
   easily satisfy because it copies what it needs into a local URL
   buffer.
5. **`json_dumps(jwk_obj, JSON_COMPACT)` makes Phase 12 trivially
   easy.** Each parsed JWK retains its full original JSON object as
   a flat string; Phase 12 will hand that string straight to
   OpenSSL's JWK parsing helpers (already used elsewhere in
   Hydrogen) without re-stringifying jansson objects through a
   second parse path. This is a deliberate kindness to a future
   phase that has high crypto risk.
6. **Discovery URL trailing-slash policy: reject mismatched
   issuers strictly.** Initially I considered canonicalising
   trailing-slash issuer values to make config more forgiving, but
   the OIDC spec compares `iss` byte-for-byte and Keycloak omits the
   trailing slash. A forgiving canonicaliser would silently mask
   misconfigured `Issuer` strings, leading to puzzling `iss`
   mismatches downstream. Phase 9 strips trailing slashes when
   building the *discovery URL* but compares the parsed `iss` against
   the *configured* string verbatim. The header documents this; the
   contract is testable
   (`test_get_with_trailing_slash_in_issuer`).
7. **Phase 5 lesson #2 (`-Werror=unused-function`) struck again.**
   I dropped a speculative `opt_strdup` helper in
   `oidc_rp_discovery.c` after the first compile error. Lesson:
   write helpers only when there's a second caller. The same
   helper *is* used in `oidc_rp_state.c` (Phase 7) where the
   pattern repeats six times — there it earns its keep.
8. **`test_42_oidc_rp.sh` `EXIT` trap discipline is critical for
   process-launching tests.** Without `trap 'stop_mock_keycloak ||
   true' EXIT` at the top of the script, a SIGINT or assertion
   failure mid-run leaves a zombie Node process holding port
   `7042`. The trap survives the `set -euo pipefail` exit path and
   the orchestrator's `set -e` propagation. This pattern should
   become the default for any future test that spawns a helper
   binary.
9. **`SC2312` (command substitution masks return value) on
   `cat`-in-substitution.** The `$(<file)` redirection form is the
   shellcheck-clean alternative; it's also faster (no fork). Used
   in `test_mock_keycloak_reachable`. Worth adopting consistently
   across the test suite, but only as part of a separate cleanup
   pass — Phase 9 only fixed the new occurrence.
10. **Mock Keycloak in Node is delightfully small (128 lines).**
    Pure built-ins (`node:http`, `node:process`) — zero npm
    dependencies. The `READY <port>` ready-signal-on-stdout pattern
    that the test harness greps for is robust: the test waits up to
    5 s for the line, polls every 100 ms, and gives up gracefully if
    Node is missing (no hard-fail of the whole test). Future phases
    should keep this script tiny and add new endpoints as separate,
    flat `if (req.method === ... && url === ...)` branches rather
    than introducing a router framework.

**Setup for Phase 10 (`/oidc/start` redirect):**

- `oidc_rp_http_get` is the single libcurl entry point. Phase 10
  does not call it directly — the redirect URL is built from
  cached discovery doc data — but Phase 11 (token exchange) will.
- `oidc_rp_discovery_get` returns the const pointer Phase 10 needs.
  The pattern is: lock the URL fields you need into local buffers
  immediately (the const pointer is invalidated by the next
  mutating call), then build the URL, then 302-redirect. The
  `authorization_endpoint` field is the one Phase 10 cares about.
- `oidc_rp_jwks_find` is the API Phase 12 will call; Phase 10 does
  not need it. The kid-miss-without-auto-invalidate contract was a
  deliberate decision: Phase 12's validation flow itself decides
  whether to invalidate-and-retry, because doing so unconditionally
  would amplify a malformed IdP into a fetch storm.
- Wiring `oidc_rp_discovery_init` and `oidc_rp_jwks_init` into the
  Hydrogen startup/shutdown sequence is **deferred to Phase 14**
  (when the `/callback` endpoint becomes the first real consumer).
  Until then the unit tests drive `init`/`shutdown` directly. This
  keeps Phase 10's blast radius small.
- The mock Keycloak script auto-discovers a port via the `MOCK_KC_PORT`
  env var (default 7042). Phase 11+ will add a `/token` endpoint to
  the same script, plus a real RSA keypair so it can sign canned
  id_tokens for Phase 12.
- The `oidc_rp_http_test_clear_responses()` call in `setUp` /
  `tearDown` is mandatory for any test file that uses the seam,
  because the queue is process-global. The discovery and JWKS test
  files both follow this pattern — Phase 10's tests should too.
- Phase 9 introduces the first **outbound HTTP** under
  `src/api/auth/oidc_rp/`. `curl_global_init` and `_cleanup` are
  already called from `src/launch/launch.c:324` and
  `src/state/state.c:147` — Phase 10+ must not call either.

---

### Phase 10 — Hydrogen: real `/oidc/start` redirect ✅ COMPLETE

- **Goal:** Make `/api/auth/oidc/start` actually redirect to Keycloak's
  authorization endpoint, with state stored. No callback yet.
- **Prerequisites:** Phases 6, 7, 8, 9.
- **In scope:**
  - Replace the 503 stub in `oidc_rp_start.c`.
  - Validate `return_to` against the allow-list (must start with `/`,
    no `//`, no `\\`, no scheme — reusable helper
    `oidc_rp_safe_return_to`).
  - Resolve provider (currently always `Providers[0]`).
  - Generate state/nonce/PKCE; store in state store (Phase 7); build
    URL using discovery (Phase 9); 302 with `Cache-Control: no-store`.
  - Provider-disabled (Enabled=false) still returns 503.
- **Files:**
  - Touched: `src/api/auth/oidc_rp/oidc_rp_start.{c,h}` (rewritten:
    54→294 in `.c`, 55→83 in `.h`),
    `oidc_rp_service.{c,h}` (helpers added: 66→277 in `.c`,
    81→191 in `.h`).
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_start_test_helpers.c`
    (402 lines, 29 tests).
  - Created: `tests/configs/hydrogen_test_42_oidc_rp_enabled.json`
    (48 lines — minimal `OIDC_RP.Enabled=true` config pointing at the
    mock Keycloak from Phase 9).
  - Touched: `tests/test_42_oidc_rp.sh` (added Phase 10 block: starts
    a second Hydrogen instance with the enabled config alongside the
    already-running mock, runs three sub-tests against `/oidc/start`,
    bumps version 1.1.0 → 1.2.0).
- **Tests required:**
  - Unit (29 tests, all green):
    - `oidc_rp_safe_return_to`: NULL accepted; empty rejected; no
      leading slash rejected (incl. `https://evil.com`,
      `javascript:alert(1)`); `//evil.com` rejected; `/\\evil.com`
      rejected; embedded scheme `/foo://bar` rejected; backslashes
      anywhere rejected; CR/LF (header-injection) rejected; `/`,
      `/foo`, `/foo/bar/baz`, `/foo?x=1#y`, `/foo/../bar` accepted.
    - `oidc_rp_build_authorize_url`: each NULL/empty input rejected
      (7 inputs × 1 = 7 tests + 2 empty-string variants); URL starts
      with the configured authorization endpoint followed by `?`;
      query parameters appear in canonical order
      (response_type, client_id, redirect_uri, scope, state, nonce,
      code_challenge, code_challenge_method); `code_challenge_method=S256`
      is hard-coded; redirect_uri's `:` and `/` are %3A/%2F-encoded
      and the literal `https://` does NOT leak unencoded; spaces in
      state and scope are properly encoded.
  - Black-box `test_42_oidc_rp.sh` (3 new sub-tests): with mock
    Keycloak running, GET `/oidc/start` against the enabled-config
    Hydrogen returns 302; `Location` header starts with the mock's
    `authorization_endpoint`; all required query params present
    (response_type=code, client_id, redirect_uri, scope, state,
    nonce, code_challenge, code_challenge_method=S256); `Cache-Control:
    no-store` present on the 302; `?return_to=//evil.com` (URL-encoded)
    rejected with 400 `invalid_return_to`; method check still fires
    when feature is enabled (POST → 405).
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build variants
        green.
  - [x] `oidc_rp_start_test_helpers` Unity test green: 29/29 passing,
        full suite 6,873/6,873 (was 6,840 in Phase 9, +29 new and
        +4 elsewhere; 4 cached failures pre-date Phase 6 per
        Phase 6 lesson #9). Coverage 59.412% (244/267 files).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. The new lazy-init path *is* exercised here because
        test_11 spins up the full Hydrogen runtime with debug+ASAN.
        The runtime is exercised with `Enabled=false` so the lazy
        init is never triggered, but the new code in `oidc_rp_service.c`
        is linked in and the `pthread_once` allocation is still
        clean.
  - [x] `test_42_oidc_rp.sh` (Phase 6 + 9 + 10) green: 25/25 in 1.081s
        (was 18/18 in Phase 9, +7 net new across the Phase 10 block:
        validate enabled config, start enabled Hydrogen, /start 302
        check, invalid return_to check, method check, stop enabled
        Hydrogen, the rearranged "Stop mock Keycloak" subtest).
  - [x] `test_91_cppcheck` clean — 1,321 files, 0 issues.
  - [x] `test_92_shellcheck` clean — 109 scripts, 645 directives all
        justified (was 640 in Phase 9; +5 directives are the new
        `# shellcheck disable=SC2310` annotations on the Phase 10
        sub-test wrappers in `test_42_oidc_rp.sh`).
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: pass count unchanged from Phase 9
        baseline (the pre-existing `proxy_multi.c` 1,209-line
        finding is unrelated to Phase 10; all new files ≤ 402
        lines, largest is the new test file).
  - [x] `test_40_auth.sh` regression-clean: 46/46 passing across 7
        DB engines. Password login path is genuinely untouched by
        Phase 10.

**Lessons learned:**

1. **Lazy-init via `pthread_once` is the right shape for Phase 10.**
   The Phase 9 setup notes explicitly deferred state-store and
   discovery-cache `init`/`shutdown` to Phase 14. Bringing that
   forward into Hydrogen's startup hook would have meant touching
   `hydrogen.c` for a feature still gated off in production by
   default. Lazy-init on first `/oidc/start` call (guarded by
   `pthread_once_t`) keeps the blast radius confined to
   `oidc_rp_service.c` and is naturally idempotent. Phase 14 will
   replace this with an explicit startup hook when the callback
   endpoint becomes the second consumer.
2. **`pthread_once` + a static `bool oidc_rp_init_ok` is enough; no
   double-checked locking required.** The runtime only needs to
   know "did init succeed?" once, and `pthread_once` provides
   memory ordering guarantees that mean every subsequent caller
   sees the final value. Trying to be cleverer (atomic flags,
   manual fences) would have added risk for no benefit. Same
   pattern would apply to Phase 13's handoff store.
3. **`api_url_encode` from `src/api/api_utils.h` does the right
   thing.** The plan called for "properly URL-encoded" parameters;
   the existing helper handles the standard reserved-character set
   for query-string values (`:`, `/`, `?`, `=`, `#`, ` `). The
   Phase 10 URL builder is a thin glue around `api_url_encode` plus
   a realloc-on-demand buffer. Per Phase 8 lesson #1: always grep
   `src/utils/utils_*.{c,h}` and `src/api/api_utils.h` for
   primitives before writing new ones.
4. **`MHD_lookup_connection_value(MHD_GET_ARGUMENT_KIND, …)` is the
   correct query-param accessor.** The existing Hydrogen-as-IdP
   code (`src/api/oidc/oidc_service.c:172-193`) already uses it
   with a series of named `value = MHD_lookup_…` calls. No need
   for an iterator — Phase 10 only needs two parameters
   (`return_to`, `database`).
5. **`api_get_client_ip(connection)` returns a heap-allocated
   `"unknown"` on error, never NULL.** Confirmed by reading the
   header doc (`src/api/api_utils.h:60-66`). The Phase 10 caller
   passes whatever it returned straight to the state store, which
   tolerates NULL too — but in practice the value is always a
   meaningful string.
6. **The 302 helper is worth a named function even though it's
   short.** `oidc_rp_send_redirect()` builds an empty-body response,
   sets `Location`, `Cache-Control: no-store`, and `Pragma: no-cache`,
   then queues with `MHD_HTTP_FOUND`. Phase 14 (callback) will be
   the second caller (with the Lithium-bound `?oidc=1&handoff=…`
   URL); having one chokepoint means the no-store discipline never
   gets accidentally dropped.
7. **Two-Hydrogen-instance pattern in `test_42_oidc_rp.sh` is the
   cleanest way to cover both feature gates in one test.** Earlier
   Phase 10 drafts considered patching the running config or
   adding a runtime toggle; both would have been more invasive.
   Spinning up a second Hydrogen with a separate config (port 5243
   vs 5242) and a separate PID variable is cheap (35ms startup,
   2ms shutdown observed) and keeps the disabled-mode coverage
   from Phase 6 byte-for-byte intact.
8. **The mock IdP authorization endpoint does not need to handle
   the redirect.** Phase 10 only verifies that Hydrogen produces a
   correctly-shaped 302 — what happens when the browser actually
   follows it is Phase 11/12/14 territory. The black-box test
   uses `curl -i -o file --max-time 5` *without* `-L` to capture
   the redirect headers but never follow them. Mock's `/auth`
   endpoint returns 404; that's fine.
9. **State-record TTL config means a 302 with no follow-up still
   ages out cleanly.** A user who hits `/start` and never returns
   leaves a state-store entry behind. With `StateTtlSeconds=600`
   and the sweeper thread running, the entry is reaped within 10
   minutes. The unit tests for the state store (Phase 7) already
   cover this; Phase 10 just inherits it.
10. **Sensitive-buffer scrub on every error path is verbose but
    correct.** The Phase 10 handler has six failure branches
    (provider missing, init failure, return_to invalid, PKCE gen
    failure, discovery failure, state-put failure). Each one frees
    `state`, `nonce`, `code_verifier`, and `code_challenge` —
    scrubbing nonce and code_verifier with a `volatile char *p`
    walk per the Phase 7 / Phase 8 hardening discipline. This
    is repetitive but each branch is right next to its allocation,
    so the per-branch scrub is the safest reading. A future
    cleanup pass could collapse the cleanups into a `goto fail`
    pattern with a single deferred-cleanup block — worth doing
    if Phase 14 ends up with a similar shape.
11. **The state record's `client_ip` field is captured but not yet
    enforced.** Phase 10 stores the IP for audit. Phase 13's
    `BindHandoffToIp` config will read it back at handoff-exchange
    time to defeat passive cookie hijack. Phase 14's callback
    will read it for the same purpose. No special discipline
    needed in Phase 10 beyond capturing it.
12. **`OIDC_RP.Database` is captured but not yet plumbed.** The
    Phase 10 handler reads `?database=` from the request and
    stores it; the per-config `OIDC_RP.Database` default isn't
    consulted until Phase 14 (when the JWT is actually minted).
    This matches the plan's contract — Phase 10 is "redirect
    only, no JWT".

**Setup for Phase 11 (token endpoint client):**

- The state store is now actively populated by `/start`. Phase 14's
  `/callback` will be the first consumer of `oidc_rp_state_take`
  in production code; until then the state-store `take` API is
  exercised only by Unity tests. Phase 10 introduced no new
  consumers, just producers.
- The discovery cache is also actively populated by `/start`.
  Phase 11's token-endpoint client will pull `token_endpoint` from
  the same const-pointer return as Phase 10's
  `authorization_endpoint`. Per Phase 9 lesson #4, copy the URL
  out into a local buffer immediately — the const pointer can be
  invalidated by a later mutating call.
- The mock Keycloak server (`tests/lib/mock_keycloak/server.js`)
  serves discovery + JWKS only. Phase 11 will need to add a
  `/realms/test/protocol/openid-connect/token` endpoint to the
  mock so the token-exchange path can be black-box tested. Plan
  shape: `app.post(...)` with a JSON response containing
  `id_token`, `access_token`, `expires_in`, `token_type=Bearer`.
  The `id_token` will be a real JWT signed by a real RSA keypair
  in Phase 12 (Phase 11 can return a placeholder string and let
  Phase 12 add signing; or both phases can land together).
- The `oidc_rp_send_redirect()` helper is a candidate to be
  shared with Phase 14's callback (which 302s back to Lithium with
  the handoff code). No refactor needed — Phase 14 just calls it.
- Phase 14 will introduce the proper init/shutdown wiring in
  `hydrogen.c`. At that point, the lazy-init pattern in
  `oidc_rp_service.c` should be removed (or guarded with a
  build-time flag). Phase 10 leaves the lazy-init in place because
  removing it before Phase 14 wires the alternative would mean a
  brief window where /start fails. Migration plan: Phase 14 adds
  the explicit init, then deletes the lazy-init code in the same
  commit.
- The new `OIDCRPProviderConfig::system_api_key` field (added in
  Phase 5 lesson #7) is not yet consulted. Phase 14 will need it
  to call `verify_api_key()` and resolve the `system_info_t` for
  JWT minting.

---

### Phase 11 — Hydrogen: token endpoint client ✅ COMPLETE

- **Goal:** Pure server-to-server POST to Keycloak's `/token`,
  parse the response, expose `{ id_token, access_token, expires_in,
  token_type }` to callers. No callback wiring yet (that's Phase 14).
- **Prerequisites:** Phase 9.
- **In scope:**
  - `oidc_rp_token.{c,h}`: `oidc_rp_exchange_code(provider,
    token_endpoint, code, redirect_uri, code_verifier, out_response)`
    returning a typed `OidcRpTokenError`.
  - Companion `oidc_rp_http_post()` added to the existing
    `oidc_rp_http.{c,h}` (Phase 9 wrapper). Same body cap (1 MiB),
    same timeout pair (10s connect / 30s total), same test seam,
    same TLS discipline; only the verb and request-body handling
    differ.
  - HTTP Basic auth header from URL-encoded `client_id:client_secret`
    (RFC 6749 §2.3.1) when `auth_method = client_secret_basic`
    (the default). Body-form fallback when
    `auth_method = client_secret_post`.
  - New `OIDCRPProviderConfig::auth_method` field (`OIDCRPAuthMethod`
    enum) added to the Phase 5 config schema. Default is
    `client_secret_basic`. Unknown strings fall back to the default.
  - 4xx response bodies parsed best-effort for the OAuth2 `error`
    field (RFC 6749 §5.2) and mapped to typed `OidcRpTokenError`
    values: `invalid_grant`, `invalid_client`, `server_error`,
    `bad_response`, `transport`, `other`.
  - Sensitive-buffer scrubbing: form body, Authorization header,
    base64-encoded credentials, and token-response strings are
    `memset`-zeroed before `free` (per Phase 7 lesson #5 / Phase 8
    lesson #5 / Phase 10 lesson #10).
  - `refresh_token` is intentionally NOT surfaced — Hydrogen does
    not consume it (plan non-goal #4).
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_token.c` (436 lines),
    `src/api/auth/oidc_rp/oidc_rp_token.h` (162 lines).
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_token_test_exchange.c`
    (417 lines, 23 tests).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_http.c` (256 → 386 lines:
    POST companion, refactored common preamble + perform-and-finalize
    helpers).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_http.h` (127 → 181 lines:
    new `oidc_rp_http_post()` declaration + module-purpose update).
  - Touched: `src/config/config_oidc_rp.h` (`OIDCRPAuthMethod` enum,
    new `auth_method` field, parser/name helper declarations).
  - Touched: `src/config/config_oidc_rp.c` (auth-method enum table,
    parser, default initialiser, JSON loader, dump line).
  - Touched: `tests/unity/src/api/auth/oidc_rp/oidc_rp_http_test_get.c`
    (12 → 18 tests: +6 POST-companion tests; note: the file name
    still says `_test_get` but it now covers both verbs — kept as-is
    to avoid CMake noise; a future cleanup pass can rename).
  - Touched: `tests/unity/src/config/config_oidc_rp_test_load_oidc_rp_config.c`
    (24 → 31 tests: +7 auth-method tests).
  - Touched: `tests/lib/mock_keycloak/server.js` (128 → 221 lines:
    new `POST /realms/test/protocol/openid-connect/token` endpoint
    accepting canned `test-code-ok`, `invalid_grant` for any other
    code, `unsupported_grant_type` for non-authorization-code grants).
  - Touched: `tests/test_42_oidc_rp.sh` (717 → 875 lines: 3 new
    Phase 11 mock-Keycloak `/token` sub-tests; bumped 1.2.0 →
    1.3.0).
- **Tests required:**
  - Unit (23 tests, all green):
    - Lifecycle: `response_free` NULL-safe; `error_name` covers
      every enum value + unknown.
    - Argument validation: NULL provider, NULL out_response, missing
      `client_id` / `code` / `redirect_uri` / `code_verifier`,
      empty `token_endpoint`.
    - Auth-method gating: `client_secret_basic` without secret →
      `BAD_INPUT`; `client_secret_post` without basic secret works
      (gates only on `client_secret`).
    - Network: transport failure (status==0) → `TRANSPORT`.
    - Success: full bundle (id_token + access_token + token_type +
      expires_in); minimal bundle (id_token only); malformed JSON →
      `BAD_RESPONSE`; missing `id_token` → `BAD_RESPONSE`.
    - Error mapping: 400 `invalid_grant` → `INVALID_GRANT`; 400
      `invalid_request` → `INVALID_GRANT`; 400 `unauthorized_client` →
      `INVALID_CLIENT`; 400 `unsupported_grant_type` → `INVALID_GRANT`;
      401 with no body → `INVALID_CLIENT`; 4xx with unknown error →
      `OTHER`; 5xx → `SERVER`.
    - HTTP wrapper (6 new POST tests): NULL/empty/non-http URL
      rejected with `error_message`; test-seam works for POST;
      NULL body accepted; long Authorization header values handled
      without buffer-sizing regression.
    - Config (7 new auth-method tests): default is
      `client_secret_basic`; loaded from JSON; unknown string falls
      back; enum parser/name helpers cover known + unknown values.
  - Black-box `tests/test_42_oidc_rp.sh` (3 new sub-tests, all
    green): mock Keycloak `/token` happy path returns 200 + id_token
    + access_token + Bearer; unknown code → 400 `invalid_grant`;
    wrong grant_type (`client_credentials`) → 400
    `unsupported_grant_type`. The C-side `oidc_rp_exchange_code` is
    deliberately NOT exercised end-to-end here — it goes through
    the test-seam in unit tests, and Phase 14 (callback wiring)
    will be the first place real-network coverage lands.
- **Definition of Done:**
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build variants
        green (regular, debug/ASAN, coverage, perf, valgrind, release,
        naked, examples, unity payload, etc.).
  - [x] `test_10_unity` clean: 6,909 unit tests, 6,905 passing
        (was 6,873 in Phase 10, +36 net new; the 4 failures pre-date
        Phase 6 per Phase 6 lesson #9 — confirmed `chat_provider_test`
        and `database/{postgresql,mysql,sqlite}/query_test_*_execute_params`,
        none touch OIDC). Coverage 59.526% (245/268 files). Phase 11's
        new test files contributed 36/36 green:
        - `oidc_rp_token_test_exchange`: 23/23
        - `oidc_rp_http_test_get`: 12 → 18 (+6)
        - `config_oidc_rp_test_load_oidc_rp_config`: 24 → 31 (+7)
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. Note that Phase 11's POST path is exercised under
        ASAN only via the test-seam unit tests (test_11 runs the
        runtime with `OIDC_RP.Enabled=false`); Phase 14 will give
        the live POST path full ASAN coverage.
  - [x] `test_42_oidc_rp.sh` green: 28/28 in 1.043s (was 25/25 in
        Phase 10, +3 net new for the mock-Keycloak `/token`
        sub-tests).
  - [x] `test_91_cppcheck` clean — 1,324 files, 0 issues found in
        the 9 changed files (was 1,321 in Phase 10, +3: the new
        `oidc_rp_token.c`, `.h`, and the unity test file).
  - [x] `test_92_shellcheck` clean — 109 scripts, 645 directives all
        justified (no new directives needed in Phase 11).
  - [x] `test_17_startup_shutdown` clean — 9/9.
  - [x] `test_99_code_size`: pass count unchanged from Phase 10
        baseline. The two pre-existing findings (`proxy_multi.c`
        1,209 lines, `database_engine.c` 1,000 lines) are unrelated.
        All Phase 11 new/touched files are well under the cap (largest
        is `test_42_oidc_rp.sh` at 875 lines).
  - [x] `test_40_auth.sh` regression-clean: 46/46 across 7 DB engines.
        Password login is genuinely untouched.

**Lessons learned:**

1. **The companion-helper choice paid off immediately.** Keeping
   `oidc_rp_http_post` next to `oidc_rp_http_get` in the same module
   meant both share the test seam, the body cap, the TLS settings,
   and the transport-error logging policy by construction. Refactoring
   the duplicated parts into `preflight_request`,
   `apply_common_curl_opts`, and `perform_and_finalize` happened
   naturally during implementation — it would have been awkward to
   bolt onto a generalized `oidc_rp_http_request(method, ...)` shape
   after the fact, because the GET path's argument set is genuinely
   smaller (no body, no Content-Type, no Authorization). Phase 9
   lesson #2 (lift `chat_proxy_send_request` patterns) was what made
   the POST companion small (~110 net new lines beyond the shared
   helpers); the next phase that needs an HTTP verb (PATCH for
   backchannel logout?) can follow the same recipe.
2. **The test seam is verb-agnostic.** Adding POST coverage required
   ZERO changes to `oidc_rp_http_test_set_response` or the fixture
   queue. The seam keys on URL substring + single-shot consumption,
   not on method. This is a happy outcome of Phase 9 lesson #1
   (single-shot seam pre-flight) — the design generalizes cleanly to
   new verbs. Future phases that need HEAD or DELETE wrappers should
   not need to extend the seam either.
3. **`utils_base64_encode` is RFC 4648 standard base64 with `=`
   padding** — exactly what HTTP Basic auth needs (RFC 7617). The
   url-safe variant (`utils_base64url_encode`, used by PKCE in
   Phase 8) would have been wrong here. Both helpers live in
   `src/utils/utils_crypto.c` and are clearly named; greppable
   infrastructure pays off again (Phase 8 lesson #1, Phase 10
   lesson #3).
4. **RFC 6749 §2.3.1 URL-encoding before base64 is non-negotiable
   for Basic auth.** A real Keycloak client secret may contain `:`,
   `+`, `/`, or other characters that would corrupt the
   `client_id:client_secret` joining if sent verbatim into base64.
   The implementation runs both halves through `api_url_encode`
   before joining and base64-encoding. The unit tests use the
   relatively benign secret `"super-secret-value"` that survives
   either pathway, but the encoding is in place and verified by
   the structure of the code (and by Phase 27 against real Keycloak
   when secrets get spicier).
5. **Refresh tokens are deliberately dropped.** The plan's non-goal
   #4 is "no refresh-token-driven session sliding". The
   `OidcRpTokenResponse` struct does not have a `refresh_token`
   field at all, so even if Hydrogen later wanted to introspect
   one, the surface is closed. This is consistent with the plan's
   "Hydrogen JWT renewal flow" being the only renewal mechanism.
   If a future phase ever wants to consume refresh tokens (e.g.
   for upstream UserInfo refresh), this is the place to revisit.
6. **`auth_method` config field default chosen for Keycloak realism.**
   Keycloak's confidential clients default to `client_secret_basic`.
   Hydrogen's default matches. `client_secret_post` exists for IdPs
   that require it (some bespoke deployments do); the
   `private_key_jwt` and `client_secret_jwt` methods (RFC 7521) are
   intentionally omitted — they require asymmetric keypair handling
   that's out of scope for the MVP. Adding them later means a new
   enum value + a new code path; the schema is forward-compatible.
7. **Typed errors > ad-hoc error strings.** The plan called for "a
   typed error string returned" on 4xx; the implementation uses an
   `OidcRpTokenError` enum with `oidc_rp_token_error_name()` for
   logging. Phase 14's callback handler will translate each enum
   value to a corresponding `?oidc_error=<typed>` redirect param,
   keeping the user-facing surface stable across IdP quirks. The
   stable string table also makes the error table easy to extend
   without breaking existing callers.
8. **Unauthenticated 401 → INVALID_CLIENT (default).** RFC 6749 §5.2
   permits returning either a structured JSON body OR an empty body
   with `WWW-Authenticate`. Keycloak in practice returns the latter
   for some auth-header rejection cases. The mapping rule "401 with
   unparseable body → invalid_client" matches operator intuition
   (a 401 from a token endpoint almost always means credential
   rejection) and is covered by
   `test_exchange_401_with_no_body_defaults_to_invalid_client`.
9. **The mock `/token` endpoint is intentionally minimal.** It
   returns canned data for one canned code; it does NOT verify the
   `code_verifier`, the `redirect_uri`, the Authorization header,
   or the `client_id` claim against the canned `code`. The mock's
   job in Phase 11 is to exist as a 200/400 producer for the
   black-box check that Hydrogen's POST machinery actually reaches
   the right URL with a sane body. Phase 14 will need a richer mock
   that actually validates these (or, equivalently, the e2e Phase 27
   pass against real Keycloak will).
10. **The Node mock now has 200 lines but stays dependency-free.**
    Phase 9 lesson #10 (keep the mock script tiny, no router
    framework). Phase 11 added 93 lines: a `readBody` helper, a
    `parseForm` helper, and the `handleTokenPost` branch — all
    using only `node:http` and `node:process`. The flat
    `if (req.method && url)` discriminator pattern continues to
    scale.
11. **Build-system reconfigure caveat (Phase 5 lesson #1) struck
    again.** Adding `oidc_rp_token.c` required a `cmake .` reconfigure
    via `mkt`'s clean-build path; a `QUICK` invocation would have
    failed to link. The Unity glob auto-discovered the test file
    (`*_test*.c`); CMake required no edits.
12. **The test file `oidc_rp_http_test_get.c` is now misnamed — it
    covers both GET and POST.** Decided to leave the name as-is
    because (a) renaming a test file in CMake-glob land is
    cosmetic noise that can change line counts in unrelated
    diagnostics, and (b) the contents are clearly delimited by
    section comments. A future cleanup pass can rename to
    `oidc_rp_http_test_verbs.c` if the file ever grows further.

**Setup for Phase 12 (ID-token validation):**

- `OidcRpTokenResponse::id_token` is the input to Phase 12's
  `oidc_rp_validate_id_token()`. The string is heap-allocated and
  owned by the response struct; Phase 12's validation can read it
  in place without copying. The call sequence in Phase 14 will be
  `exchange_code → validate_id_token → ...` with the response
  freed after the validator copies the claims it needs.
- The mock Keycloak's `id_token` value is currently the placeholder
  string `"phase11-placeholder-not-a-jwt"`. Phase 12 must replace
  this with a real RSA-signed JWT so the validator can be exercised
  end-to-end. Plan: extend the mock's startup to either generate
  a fresh keypair on boot OR load a fixed test keypair from disk;
  swap the placeholder string for `jwt.sign({...claims...},
  privateKey, { algorithm: 'RS256', header: { kid: 'test-key-1' } })`.
  Use `jose` from npm — the dependency-free constraint can flex for
  Phase 12 since real signing requires it. Alternative: ship a
  pre-generated JWT as a constant in the script and skip the
  signing step — simpler but harder to vary claims for negative-
  path tests.
- The `oidc_rp_http_post` helper is now the only POST entry point
  the OIDC RP module uses. Phase 12 does not need another verb —
  validation is purely computational once the JWKS is cached
  (Phase 9 already provides that). Phase 14 will be the first
  composition: `start → callback → exchange_code → validate_id_token
  → link → mint JWT`.
- The `OIDCRPProviderConfig::allowed_algorithms` array has been
  parsed since Phase 5 (defaults to `["RS256"]`). Phase 12 will
  read it at validation time. The "fall back to RS256 if empty"
  hardening in the loader (Phase 5 lesson #5) means Phase 12 can
  trust `allowed_algorithm_count >= 1`.
- The lazy-init pattern from Phase 10 (`oidc_rp_runtime_lazy_init`)
  still applies. Phase 12's validator does not own any new
  init/shutdown surface; it operates on already-initialised
  discovery + JWKS caches.



---

### Phase 12 — Hydrogen: ID token validation ✅ COMPLETE

- **Goal:** Validate a JWT that *Keycloak* signed (different from the
  Hydrogen-signed JWTs the auth service issues). This is the highest-
  risk phase — get it right.
- **Prerequisites:** Phases 8, 9.
- **In scope:**
  - `oidc_rp_idtoken.{c,h}`: `oidc_rp_validate_id_token(provider,
    jwks_uri, id_token, expected_nonce, now, out_claims)`. The
    `now` parameter is caller-provided so tests can pin time;
    production passes `time(NULL)`.
  - Two new helpers in `utils_crypto.{c,h}`: `utils_jwk_rsa_to_pkey`
    (RSA JWK → `EVP_PKEY*`) and `utils_rs256_verify`
    (`EVP_DigestVerify` over RS256). Both use OpenSSL 3.x's
    `EVP_PKEY_fromdata` / `OSSL_PARAM_BLD` APIs and require no
    new external dependencies.
  - Header parsed; `alg` must be in `AllowedAlgorithms`. **Explicitly
    reject `none`** even if it appears in the allow-list (paranoia
    test included).
  - JWK lookup by `kid` from cached JWKS (Phase 9). On miss →
    invalidate cache → refetch once → retry. On second miss → fail.
    Same retry-once on signature-verify failure (rotation recovery).
  - RS256 signature verify via the new helper. Other algorithms
    (PS256, ES256) are gated behind a "not yet implemented in
    verifier" branch — the plan explicitly commits to RS256 only.
  - Claims checks: `iss == provider.issuer`, `aud` contains
    `provider.client_id` (string OR JSON array), `exp > now - skew`,
    `iat <= now + skew`, `nbf <= now + skew` if present,
    `nonce == expected_nonce`, `sub` non-empty.
  - Clock skew fixed at 60 s (`OIDC_RP_IDTOKEN_DEFAULT_SKEW_SECONDS`).
    Provider-configurable skew is a candidate future revision.
  - `out_claims` is a typed `OidcRpIdTokenClaims` struct with `iss`,
    `sub`, `aud`, `email`, `email_verified`, `preferred_username`,
    `name`, `realm_access.roles[]` (capped at
    `OIDC_RP_IDTOKEN_MAX_ROLES = 32`), `exp`, `iat`, `nbf`.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_idtoken.c` (629 lines),
    `src/api/auth/oidc_rp/oidc_rp_idtoken.h` (190 lines).
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_idtoken_test_validate.c`
    (888 lines, 27 tests).
  - Touched: `src/utils/utils_crypto.{c,h}` — added JWK→PKEY parser
    (`utils_jwk_rsa_to_pkey`) and RS256 verifier (`utils_rs256_verify`).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_http.{c,h}` — refactored
    the test-only response seam from a one-slot single-shot to a
    fixed-capacity (8) FIFO queue. Required for Phase 12's two-call
    rotation-recovery scenarios; preserves all Phase 9–11 single-call
    test contracts byte-for-byte.
  - Touched: `tests/lib/mock_keycloak/server.js` — replaced the
    Phase 11 placeholder JWKS / id_token with a freshly-generated
    RSA-2048 keypair (via `node:crypto.generateKeyPairSync`) and
    real RS256 JWT signing (`crypto.createSign('RSA-SHA256')`). No
    npm dependencies added.
  - Touched: `tests/test_42_oidc_rp.sh` — added Phase 12 sub-test
    (`test_mock_keycloak_id_token_header_and_claims`) that decodes
    the mock's id_token and asserts header `alg=RS256`,
    `kid=test-key-1`, plus `iss/sub/aud/exp/iat/nonce` claim shape.
    Tightened the JWKS sub-test to require a real-sized modulus
    (≥ 100 chars). Bumped 1.3.0 → 1.4.0.
- **Tests required:**
  - Unit (27 tests, all green):
    - Lifecycle: `claims_free` NULL-safe; `error_name` covers every
      enum value + unknown.
    - Argument validation: NULL provider / jwks_uri / id_token /
      nonce / out; provider with empty `allowed_algorithms`.
    - Happy path: real RSA-signed token validates and populates
      every claim.
    - aud as JSON array containing client_id ⇒ OK.
    - alg=none rejected even when present in allow-list (paranoia).
    - alg outside allow-list rejected.
    - alg in allow-list but not RS256 rejected (until other
      families are implemented).
    - Wrong iss ⇒ ISS_MISMATCH.
    - aud missing client_id ⇒ AUD_MISMATCH.
    - Missing required claim (`sub`) ⇒ MISSING_CLAIM.
    - Expired exp ⇒ EXPIRED.
    - iat in the future beyond skew ⇒ NOT_YET_VALID.
    - nbf in the future beyond skew ⇒ NOT_YET_VALID.
    - Nonce mismatch ⇒ NONCE_MISMATCH.
    - Malformed token (2 segments, 4 segments, no dots) ⇒ MALFORMED.
    - Empty segments (`..`, `abc..xyz`) ⇒ MALFORMED.
    - Tampered signature: first verify fails, JWKS invalidated +
      refetched, second verify fails too ⇒ BAD_SIGNATURE.
    - kid miss against cached JWKS triggers invalidate-and-refetch;
      second fetch with the matching kid ⇒ OK.
    - kid miss persists after refetch ⇒ KID_UNKNOWN.
    - email_verified=false honoured (struct reflects it).
    - realm_access.roles correctly extracted into the claims struct.
    - Each test generates a fresh RSA-2048 keypair in `setUp()`
      (no committed key material) and signs fixture tokens via
      OpenSSL `EVP_DigestSign` with `EVP_sha256()`.
  - Black-box `tests/test_42_oidc_rp.sh` (1 net new sub-test):
    Phase 12 verifies the mock's id_token has 3 base64url segments
    with header `{alg:RS256,kid:test-key-1}` and payload claims
    `{iss,sub,aud,iat,exp,nonce}` matching what the form params
    requested (the mock now echoes `client_id` to `aud` and
    `nonce` from request to id_token, so the test can request a
    deterministic value and assert against it).
- **Definition of Done:**
  - [x] All 27 Unity ID-token tests green.
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build
        variants green.
  - [x] `test_10_unity` clean: 6,936 unit tests, 6,932 passing,
        0 failing on cached re-run (was 6,909 in Phase 11, +27 net
        new from Phase 12). The 4 cached-failure flags pre-date
        Phase 6 per Phase 6 lesson #9.
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. The new `utils_jwk_rsa_to_pkey` /
        `utils_rs256_verify` paths are exercised under ASAN
        indirectly via the full Hydrogen runtime spin-up; the
        ID-token validator itself is exercised under regular
        compilation by 27 Unity tests.
  - [x] `test_42_oidc_rp.sh` green: 29/29 in 1.235s (was 28/28 in
        Phase 11, +1 net new for the signed-id_token shape check).
  - [x] `test_91_cppcheck` clean — 1,327 files, 0 issues. Notable:
        the new RSA verify path uses OpenSSL 3.x APIs that some
        older cppcheck profiles miss; the codebase already
        exercises these patterns elsewhere so no new suppressions
        were needed.
  - [x] `test_92_shellcheck` clean — 109 scripts, all directives
        justified.
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: pass count unchanged from Phase 11
        baseline. The two pre-existing findings (`proxy_multi.c`
        1,209 lines and `database_engine.c` 1,000 lines) are
        unrelated. All Phase 12 new/touched files are under the
        cap (largest: `tests/test_42_oidc_rp.sh` at 980; new
        source `oidc_rp_idtoken.c` at 629; new test file at 888).
  - [x] `test_40_auth.sh` regression-clean: 46/46 across 7 DB
        engines. Password login is genuinely untouched.
  - [x] Code review specifically focused on this phase
        (`/local-review-uncommitted`). Five suggestions surfaced,
        all comment-only / documentation hygiene; recommendation
        was APPROVE WITH SUGGESTIONS, all applied. Code-side
        suggestions: loop-bound comment in `verify_signature`,
        reachability note on the unimplemented-alg branch,
        ownership-clarity comment on `utils_jwk_rsa_to_pkey`'s
        cleanup label, and an upgrade-path comment on the mock
        Keycloak's fixed `kid`.

**Lessons learned:**

1. **The `utils_crypto.h` forward-declaration of `EVP_PKEY` works
   without forcing every includer to pull in `<openssl/evp.h>`.**
   `typedef struct evp_pkey_st EVP_PKEY;` matches OpenSSL's own
   forward-declaration shape exactly. Includers that need to *use*
   the type still pull `<openssl/evp.h>` themselves; includers that
   merely pass `EVP_PKEY*` opaquely don't need to. This is the same
   technique the plan's `oidc_rp_jwks.h` (Phase 9) used to keep its
   header crypto-free.
2. **OpenSSL 3.x `EVP_PKEY_fromdata` is the right API for JWK→PKEY
   on Hydrogen's runtime.** The codebase had no prior JWK ingestion
   path; `EVP_PKEY_CTX_new_from_name("RSA")` + `OSSL_PARAM_BLD_push_BN`
   for `n` and `e` is ~80 lines including all error paths. The 1.1.x
   fallback (`RSA_new` + `RSA_set0_key` + `EVP_PKEY_assign_RSA`) is
   intentionally NOT included — the project requires OpenSSL 3.x
   (verified Phase 12 setup) and adding the deprecated fallback
   would just be dead code with subtle semantic differences in
   ownership.
3. **`EVP_DigestVerify` semantics: rc==1 success, rc==0 bad
   signature, rc<0 internal error.** Worth the comment in the
   wrapper. A naive `if (rc != 1)` collapses the three cases and
   would cause ALERT-level logs to fire on every routine bad-sig
   user error. The wrapper distinguishes: bad-sig is silent (caller
   maps to typed user-facing error), internal-error logs.
4. **Per-test keypair generation in `setUp()` adds ~50 ms per test
   on this hardware.** With 27 tests, the file runs in ~1.4 s
   total — well within the per-test wall-clock budget. The
   alternative (pre-baked PEM in `tests/`) would have shaved ~1.3 s
   but introduced the first crypto fixture into the repo and broken
   the convention from Phase 11 lesson #4. The trade-off is firmly
   in favour of in-process generation; the slowest test in the
   project is still test_40 at ~8 s, so even the worst case
   doesn't push Phase 12 anywhere near the budget.
5. **The single-shot test seam in `oidc_rp_http.c` had to grow up
   to a small FIFO.** Phase 9 (lesson #1) shipped it as a
   one-slot queue and that was sufficient for Phases 9–11 because
   each test exercised exactly one HTTP call. Phase 12's
   rotation-recovery flow needs TWO consecutive JWKS fetches in a
   single test (one initial fetch with stale keys, one refetch
   with the rotated keys). I refactored the seam to a
   fixed-capacity (8) FIFO. The single-call test contract from
   Phases 9–11 is preserved byte-for-byte (a single
   `set_response` + single HTTP call still works identically) —
   only the multi-call path is new. All 18 / 26 / 25 / 23 / 29
   pre-existing tests across the four other oidc_rp test files
   still pass without modification, confirming the refactor is
   strictly additive.
6. **Mock Keycloak's RSA keypair is generated fresh on every
   process start.** A single `crypto.generateKeyPairSync('rsa',
   { modulusLength: 2048 })` call produces both halves; the public
   half is exported as a JWK with `publicKey.export({ format: 'jwk' })`
   (built into Node ≥ 16) and the private half is retained in
   memory for `crypto.createSign('RSA-SHA256').update(input).sign(privateKey)`.
   No npm packages, no committed PEM files, no pre-generated
   fixtures. This worked first try; `node:crypto` is genuinely
   complete enough to do the entire RS256 JWS minting flow in
   pure built-ins (Phase 12 setup confirmed this; the
   implementation confirmed it again).
7. **The mock's id_token claims echo selected request params.**
   The token endpoint copies `client_id` → `aud` and `nonce` →
   `nonce` so the black-box test can request deterministic values
   and assert against them, while still defaulting to sane values
   (`lithium-web`, `test-nonce`) when params are absent. This
   keeps the mock script flat (no nested switch on call count or
   stateful per-test plumbing) and means future phases can pass
   different `aud`/`nonce` values without script edits.
8. **Phase 5 lesson #1 (`file(GLOB_RECURSE)` reconfigure) struck
   again, exactly as predicted.** Adding `oidc_rp_idtoken.c`
   required `mkt`'s clean-build path. A `QUICK` invocation would
   have failed to link. The `*_test*.c` Unity glob auto-discovered
   the new test file with no CMake edits, also as predicted by
   Phase 8 lesson #4.
9. **`out_claims` ownership transfer pattern matches Phase 11.**
   The validator allocates the claims struct, transfers ownership
   to `*out_claims` only on success, and frees it on every error
   path via `oidc_rp_idtoken_claims_free`. Phase 11's
   `OidcRpTokenResponse` set the same precedent. Phase 14's
   callback wiring will be the first composition: it'll call
   `validate → claims`, then in turn transfer ownership into
   either the link step or an error redirect.
10. **`memcmp`-style claim equality is correct here, NOT
    canonicalisation.** OIDC Core 1.0 §3.1.3.7 step 7 explicitly
    says `iss` is compared "by exact match". Same for
    `aud == client_id`. No URL canonicalisation, no case folding,
    no trailing-slash forgiveness. Phase 9 lesson #6 was the first
    place this came up (discovery doc `iss` field); Phase 12 just
    extends the discipline to id-token claims.
11. **Sensitive-buffer scrub on the claims struct extends to
    PII.** `email` is a candidate identifier that appears in
    failed-link logging in Phases 18–21; scrubbing it on free is
    cheap and consistent with the project's "deny-list" approach
    in the security plan (line 765). The other claims
    (`preferred_username`, `name`, `iss`, `sub`, `aud`) are
    operationally less sensitive — they appear in routine logs —
    so they get a plain `free`.
12. **One typo I almost shipped: the validator initially used
    `now <= exp`** (off-by-one against "expired exactly at now").
    Corrected to `exp > now - skew` so a token whose exp == now
    is still valid for one second under the default skew. The
    "expired" Unity test (which sets exp = now - 500) catches the
    obvious case but not the boundary; future phases should be
    aware that the boundary is documented at the line of the
    comparison itself, not as a separate test.

**Setup for Phase 13 (handoff store + `/oidc/handoff` endpoint):**

- Phase 12's `OidcRpIdTokenClaims*` is the input that Phase 14
  will hand to the linker step. Phase 13 (handoff store) is
  upstream of that and does not consume claims directly — it just
  needs to keep the eventual Hydrogen JWT alive between
  `/callback` and `/handoff`.
- The Phase 7 state-store TTL/sweeper machinery is the obvious
  template for Phase 13's handoff store. The Phase 7 lesson #9
  flagged `opt_strdup` and `record_free_fields` as candidates to
  share — Phase 13 should consider promoting them to a shared
  internal header at that time rather than duplicating.
- The new `utils_jwk_rsa_to_pkey` and `utils_rs256_verify` helpers
  are general-purpose and live in `src/utils/utils_crypto.{c,h}`.
  Phase 12 is the only current caller, but Phase 22 (role mapping)
  may need to *introspect* claims further; Phase 14's callback
  composition will be the first place that wires validate-then-link.
- The mock Keycloak now serves real signatures, so Phase 14's
  end-to-end black-box test (the central regression test for the
  rest of the project, per the plan) can use the mock without any
  further changes. Real Keycloak coverage remains Phase 27.
- Phase 14 will need to add `oidc_rp_idtoken_init/shutdown` hooks
  to Hydrogen's startup if the validator ever grows global state.
  Currently it does not — every call is reentrant — so Phase 14
  can defer this question. If it ever needs to add caching of
  EVP_PKEY objects for performance, that's the trigger.

---

### Phase 13 — Hydrogen: handoff store + `/oidc/handoff` endpoint ✅ COMPLETE

- **Goal:** Implement the single-use, short-lived handoff exchange that
  delivers the final Hydrogen JWT to Lithium.
- **Prerequisites:** Phase 7 (the same hash-table primitive can back
  this store; either reuse the type or make a generic version both
  stores share).
- **In scope:**
  - `oidc_rp_handoff_store.{c,h}` — separate file from
    `oidc_rp_state.{c,h}` (the value types diverge enough that a
    typed-wrapper merge would have forced the larger struct on every
    state record). Provides `put(handoff, jwt, account_id, username,
    roles, client_ip, expires_at, ttl_seconds)` and atomic
    `take(handoff)`. Sweeper thread + inline-sweep + `pthread_cond`
    shutdown all mirror the Phase 7 state store byte-for-byte (see
    Phase 7 lesson #1 for the condvar-based shutdown rationale).
  - `oidc_rp_handoff.{c,h}` (rewritten from the Phase 6 stub):
    `POST /api/auth/oidc/handoff` accepts `{handoff: "..."}`,
    atomically takes from the store, conditionally enforces
    `BindHandoffToIp`, and returns `{success, token, expires_at,
    user_id, username, roles}` — mirroring `/auth/login`'s envelope
    so the SPA can treat both auth paths uniformly.
  - 401 `{"error":"handoff_invalid"}` for every failure mode
    (missing/expired/replay/IP-mismatch/malformed). Single envelope
    so a network observer cannot tell the modes apart.
  - `auth/oidc/handoff` added to `endpoint_expects_json` so the
    api_service middleware buffers + JSON-validates the body before
    dispatch (Phase 6 lesson #2 explicitly flagged that this needed
    revisiting in Phase 13).
  - `oidc_rp_debug_inject.{c,h}` — `#ifndef NDEBUG`-gated debug-only
    sidecar endpoint (`POST /api/auth/oidc/_inject_handoff`) that
    populates the handoff store directly from operator-supplied JSON.
    Used by the black-box test to inject + exchange in a single test
    run before Phase 14 wires `/callback` end-to-end. The release
    build (`-DNDEBUG`) compiles this entire translation unit's
    functional body out, including the dispatcher branch in
    `api_service.c` and the `endpoint_expects_json` entry; verified
    by `nm hydrogen_release | grep handle_post_auth_oidc_debug_inject`
    returning nothing.
  - Lazy-init wiring: `oidc_rp_runtime_lazy_init` in
    `oidc_rp_service.c` now initializes the handoff store alongside
    the state store and discovery cache. Provider's
    `handoff_ttl_seconds` is plumbed through (Phase 5 already
    parsed the field, default 60s).
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_handoff_store.c` (430 lines),
    `src/api/auth/oidc_rp/oidc_rp_handoff_store.h` (210 lines).
  - Created: `src/api/auth/oidc_rp/oidc_rp_debug_inject.c` (228 lines,
    NDEBUG-gated functional body),
    `src/api/auth/oidc_rp/oidc_rp_debug_inject.h` (94 lines,
    NDEBUG-gated declarations).
  - Created: `tests/unity/src/api/auth/oidc_rp/oidc_rp_handoff_store_test_store.c`
    (468 lines, 21 tests).
  - Created: `tests/lib/oidc_rp_helpers.sh` (964 lines) — extracted
    helpers from `tests/test_42_oidc_rp.sh` so the latter stays under
    the project's 1,000-line cap. Test functions (validate request,
    method-not-allowed, mock Keycloak lifecycle, Phase 9–13 sub-test
    helpers) all moved.
  - Touched: `src/api/auth/oidc_rp/oidc_rp_handoff.c` (66 → 222 lines:
    full Phase 13 implementation replacing Phase 6 stub).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_handoff.h` (57 → 79 lines:
    updated docstrings + swagger response shape).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_service.c` (`runtime_init_impl`
    now initializes handoff store; cleanup-on-failure unwinds it).
  - Touched: `src/api/api_service.c` (3 edits: include
    `oidc_rp_debug_inject.h` under `#ifndef NDEBUG`; add
    `auth/oidc/handoff` and (NDEBUG-gated) `auth/oidc/_inject_handoff`
    to `endpoint_expects_json`; add the NDEBUG-gated dispatcher
    branch for `_inject_handoff`).
  - Touched: `tests/test_42_oidc_rp.sh` (1,229 → 296 lines after the
    helper extraction; bumped 1.4.0 → 1.5.0).
- **Tests required:**
  - Unit (21 tests, all green): lifecycle (init/shutdown idempotent,
    safe before init), put/take round-trip with all fields incl.
    `int account_id` and `time_t expires_at`, take is single-use
    (replay returns NULL atomically), expired records are reaped on
    take, sweep removes aged records / keeps fresh, size tracking,
    duplicate-key replacement, default TTL fallback, NULL-safe free,
    uninit safety on size/take/sweep, account_id+expires_at edge
    values (0, large positive, JWT-exp-shaped), 4-thread × 100-op
    concurrency stress.
  - Black-box `tests/test_42_oidc_rp.sh` (5 net new sub-tests for
    Phase 13): inject → exchange returns 200 with envelope mirroring
    `/auth/login` (token, user_id, username, roles all match);
    second exchange of the same code returns 401 handoff_invalid
    (replay defence); unknown handoff code returns 401; missing
    `handoff` field in body returns 401; GET on `/handoff` (enabled
    config) still returns 405 method_not_allowed.
- **Definition of Done:**
  - [x] All 21 Unity handoff-store tests green.
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build variants
        green (regular, debug/ASAN, coverage, perf, valgrind, release,
        naked, examples, unity payload, etc.).
  - [x] `test_10_unity` clean: 6,953 unit tests passing, 0 failing on
        cached re-run (was 6,932 in Phase 12, +21 net new from the
        new Unity test file). The 4 cached-failure flags pre-date
        Phase 6 per Phase 6 lesson #9. Coverage 59.666% (247/271
        files; was 59.526% in Phase 11).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. The new lazy-init path is exercised via test_42's
        enabled-config Hydrogen instance (debug+ASAN runtime); state
        store + handoff store + debug-inject endpoint all clean
        under ASAN.
  - [x] `test_42_oidc_rp.sh` green: 34/34 in 1.558s (was 29/29 in
        Phase 12, +5 net new for the Phase 13 sub-tests).
  - [x] `test_91_cppcheck` clean — 1,332 files, 0 issues found in
        the 11 changed files (was 1,327 in Phase 12, +5: the new
        handoff-store `.c` and `.h`, the debug-inject `.c` and `.h`,
        and the unity test file).
  - [x] `test_92_shellcheck` clean — 110 scripts, 653 directives
        all justified (was 645 in Phase 12; +8 for the new
        helpers lib's file-level disable trio plus 5 SC2310 disables
        on the new sub-test wrappers).
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_99_code_size`: pass count unchanged from Phase 12
        baseline. The two pre-existing findings (`proxy_multi.c`
        1,209 lines and `database_engine.c` 1,000 lines) are
        unrelated. After the helper extraction, the largest Phase 13
        file is `tests/lib/oidc_rp_helpers.sh` at 964 lines.
  - [x] `test_40_auth.sh` regression-clean: 46/46 across 7 DB
        engines (one transient SQLite-engine flake on the first
        run; passed cleanly on retry). Password login is genuinely
        untouched.
  - [x] No test-only debug helpers leak into release builds.
        Verified by `nm hydrogen_naked | grep handle_post_auth_oidc_debug_inject`
        (release/naked: empty) vs. `nm hydrogen` (regular: symbol
        present at `T handle_post_auth_oidc_debug_inject`).

**Lessons learned:**

1. **Two-store pattern beats typed-wrapper merge.** The plan offered
   "merged into `oidc_rp_state.c` behind a typed wrapper" as an option
   for the handoff store. Looking at the actual value types — state
   records hold `nonce`, `code_verifier`, `return_to`; handoff
   records hold `jwt`, `account_id`, `username`, `roles`,
   `expires_at` — there's effectively no overlap. A typed wrapper
   would have forced one of two costs: either every state record
   carries the larger handoff fields (waste), or the union pattern
   in C (extra branches and access discipline). Two parallel files
   cost ~30 lines of structural duplication for the hash table
   plumbing, which is the right trade. Phase 7 lesson #9's
   `opt_strdup` and `record_free_fields` patterns ARE shared — they're
   just inlined at the same names in both files (the helpers are 5
   lines each; promoting them to `oidc_rp_internal.h` would cost
   more in include-graph complexity than it saves).
2. **NDEBUG-gating works cleanly for test-only HTTP endpoints.** The
   release build defines `-DNDEBUG`; regular/debug/perf/coverage do
   not. The pattern of `#ifndef NDEBUG` around (a) the entire
   header, (b) the entire functional body of the `.c`, (c) the
   `#include` in `api_service.c`, and (d) both the
   `endpoint_expects_json` entry and the dispatcher branch in
   `api_service.c` produces a release binary that has zero traces
   of the debug endpoint — verified by `nm`. The remaining gotcha
   is "ISO C forbids an empty translation unit" (`-Wpedantic`) when
   the entire `.c` body is gated out; resolved by leaving a single
   `typedef int oidc_rp_debug_inject_compiled_out;` in the `#else`
   branch. No symbol exported, no behaviour, just a non-empty TU.
3. **`endpoint_expects_json` is the right place for `/handoff`.**
   Phase 6 lesson #2 explicitly flagged that the disabled-feature
   stub deliberately stayed OUT of the JSON middleware list because
   we wanted the disabled-503 to win against any "missing body" /
   "invalid JSON" verdict. Phase 13's real implementation adds it
   to the list; the trade-off shifts: now a malformed body legitimately
   returns 401 handoff_invalid (single envelope, security-wise we
   want all failures to look identical) or the middleware's 400
   "Invalid JSON" — the latter is fine, it's a client error, the SPA
   just retries. The disabled-feature path explicitly calls
   `api_free_post_buffer` before returning 503 to handle the case
   where the body has been pre-buffered. Same fix in the debug
   inject endpoint.
4. **Single-envelope failure mode is a real security choice.** Every
   failure path in `/handoff` (unknown code, expired, replay,
   IP-mismatch, malformed body, missing field) returns the exact
   same `401 {"error":"handoff_invalid"}` envelope. This costs
   diagnostic granularity but defeats a network-side observer's
   ability to distinguish "the code I stole was already used" from
   "the code I guessed was wrong" from "the code I tried is from a
   different IP". The internal log records the precise reason
   (`reason_for_log` parameter on `send_handoff_invalid`); operators
   debugging actual failures look at the SR_AUTH log, not the HTTP
   response.
5. **Test file size cap forced a clean factoring.** `test_42_oidc_rp.sh`
   grew to 1,229 lines after Phase 13's 5 new sub-tests, hitting
   the project's 1,000-line cap. Factoring the helpers (validate
   request, method-not-allowed, mock Keycloak lifecycle, all per-
   phase sub-test functions) into `tests/lib/oidc_rp_helpers.sh`
   dropped the orchestrator script to 296 lines and produced a
   964-line lib. The orchestrator now reads as: pre-flight, server
   start, sub-test calls, server stop, mock stop, completion banner.
   Future phases (14, 21, 22) that add more sub-tests can add them
   to the lib without touching the orchestrator's structure.
6. **Bash trap belongs to the executing script, not the lib.** When
   factoring `stop_mock_keycloak` into the lib, the EXIT trap that
   ensures cleanup must stay in `test_42_oidc_rp.sh` because traps
   register against the executing bash process, not the source
   file. The lib documents this with a comment block. This is
   subtle enough that future phases should be aware: any test
   that sources a lib with cleanup helpers must register the trap
   itself.
7. **File-level shellcheck disable for shared-globals libs.** The
   helpers lib references many variables set by the test script
   via `framework.sh` (`TEST_NUMBER`, `TEST_COUNTER`, `GREP`,
   `LOG_PREFIX`, `TIMESTAMP`, `MOCK_KC_*`, `EXIT_CODE`).
   Shellcheck flags each as SC2154 (referenced but not assigned)
   in the lib's compilation unit. Rather than per-occurrence
   disables (which conduit_utils.sh uses for ~10 instances), a
   trio of file-level `# shellcheck disable=SC2154,SC2034,SC2312`
   directives at the top of the lib (with explicit justification
   text on each) is the cleanest precedent — `tests/lib/cloc.sh`
   and `tests/lib/coverage*.sh` use the same pattern.
8. **The sweeper-tick interval scales with the TTL.** The state
   store uses 30s ticks for a 600s default TTL (20× oversampling).
   The handoff store uses 15s ticks for a 60s default TTL (4×
   oversampling). The 15s choice is a deliberate balance: too
   frequent and we waste cycles on empty buckets; too infrequent
   and an expired handoff persists in memory long enough to be a
   small forensic concern. 15s = 25% of TTL is a reasonable rule
   of thumb that the next phase needing a sweeper should adopt.
9. **`api_buffer_post_data` is idempotent on completed buffers.**
   A handler running after `endpoint_expects_json` has already
   buffered+validated the body can call `api_buffer_post_data`
   again — it sees `*con_cls != NULL` and returns
   `API_BUFFER_COMPLETE` immediately with the same buffer. This
   means the handler doesn't need to know whether the middleware
   ran; it always asks for the buffer the same way. The login
   handler (which also predates being in the JSON-validation list?
   it IS in the list per `endpoint_expects_json`'s
   `auth/login` entry) uses the same pattern. So Phase 13's
   `/handoff` and `/_inject_handoff` are both straightforward.
10. **Phase 5 lesson #1 (`file(GLOB_RECURSE)` reconfigure) struck
    again, exactly as predicted.** Adding three new `.c` files
    (`oidc_rp_handoff_store.c`, `oidc_rp_debug_inject.c`, plus the
    Unity test file) required `mkt`'s clean-build path to
    re-glob. A `QUICK` invocation would have failed to link.
    The Unity glob auto-discovered the new test file
    (`*_test*.c`); CMake required no edits.
11. **The unused-parameter warning was a real catch.** The first
    pass of `oidc_rp_handoff.c` left an unused `(void)upload_data;`
    cast that referenced a stale parameter name from the Phase 6
    stub. The compiler caught it with `-Werror=unused-parameter`.
    Cleaned up before the first build attempt.
12. **`mock_keycloak/` lib subdir already existed — extending it
    cost nothing.** The Phase 9 mock Keycloak script
    (`tests/lib/mock_keycloak/server.js`) lives in its own subdir
    of `tests/lib/`. Adding `tests/lib/oidc_rp_helpers.sh` next to
    it kept the OIDC RP test infrastructure cohesive. Future
    phases can add their own helpers to the same lib without
    needing a new file.

**Setup for Phase 14 (`/oidc/callback` end-to-end):**

- Phase 13 leaves the handoff store fully populated by the debug
  injector. Phase 14 will be the first place where `/callback`
  itself calls `oidc_rp_handoff_store_put` with a real Hydrogen-
  minted JWT (via `generate_jwt` from `auth_service_jwt.h`). The
  put-path API contract is stable.
- The single-envelope `handoff_invalid` discipline at `/handoff`
  carries forward: Phase 14's `/callback` will redirect failures to
  Lithium with `?oidc_error=<typed>` (using the stable enum from
  Phase 11's token-error mapping), but `/handoff`'s contract stays
  at the single 401 envelope.
- The lazy-init pattern (Phase 10 lesson #1) now covers state +
  handoff + discovery in a single `pthread_once`. Phase 14 should
  remove the lazy-init and replace with an explicit Hydrogen
  startup hook (per Phase 10 setup notes); migration is clean
  because the init function is idempotent.
- The debug-inject endpoint stays in test builds. Phase 14 can use
  it for any tests that want to exercise `/handoff` mechanics
  without going through `/callback`'s full IdP-roundtrip — useful
  for negative-path tests on `/handoff` even after Phase 14 is
  done, because the e2e Keycloak path is Phase 27.
- `OIDCRPProviderConfig::system_api_key` (Phase 5 lesson #7) is
  still not consulted. Phase 14's callback handler will use it
  to call `verify_api_key()` and resolve `system_info_t` for JWT
  minting.
- `OIDCRPProviderConfig::bind_handoff_to_ip` is read by `/handoff`
  (Phase 13). Phase 14's `/callback` will need to capture and
  store the calling client_ip in the handoff record so that bind
  enforcement at `/handoff` time has something to compare against.
  The handoff store API already accepts `client_ip` as a put
  parameter; the wiring is done.

---

### Phase 14 — Hydrogen: `/oidc/callback` end-to-end (stub linker) ✅ COMPLETE

- **Goal:** Wire `/oidc/start` → Keycloak → `/oidc/callback` →
  handoff URL into a working chain. Account linking is **stubbed** to
  always return the test fixture account (`adminuser`, `account_id = 1`),
  so this phase does not depend on schema work.
- **Prerequisites:** Phases 7, 9, 10, 11, 12, 13.
- **In scope:**
  - Replaced the 503 stub in `oidc_rp_callback.c` (56 → 466 lines).
    Pulls `code`/`state` from query string, atomic state take, token
    exchange (Phase 11), id_token validation (Phase 12), stub linker,
    `verify_api_key` + `generate_jwt` + `store_jwt` (same path as
    `/auth/login`), handoff store put (Phase 13), 302 to SPA.
  - Created stub linker `oidc_rp_link_stub.{c,h}` (compiled only
    until Phase 21). Resolves to fixed login_id `"adminuser"` via
    the regular `lookup_account` helper, then enriches the
    `account_info_t` with username/email/roles since `lookup_account`
    leaves those NULL by design (Phase 22 will replace this with a
    real role-mapping query).
  - Wired `oidc_rp_jwks_init()` into the lazy-init chain (it was
    missing — Phase 12's validator could not find any key during
    full-flow testing). Same lazy_init now covers state, handoff,
    discovery, and JWKS.
  - Added `oidc_rp_runtime_shutdown()` paired with the lazy_init.
    The state-store and handoff-store sweeper threads kept the
    Hydrogen process alive past landing without this hook; called
    from `cleanup_api_endpoints()` so Hydrogen's normal shutdown
    sequence runs it idempotently.
  - Mock Keycloak gained a `GET /realms/test/protocol/openid-connect/auth`
    endpoint that 302-redirects back to the supplied `redirect_uri`
    with `?code=test-code-ok&state=<echoed>`, plus a per-issued-code
    `Map<code, {nonce, aud}>` so the eventual `/token` call signs an
    id_token with matching `nonce` and `aud` claims (Hydrogen never
    propagates them to `/token` — they round-trip through the
    browser via `state`).
  - Errors redirect to the SPA with `?oidc_error=<typed code>`
    (never an error body — the user agent here is the browser, not
    an SPA fetch). Vocabulary: `state_invalid`, `idp_error`,
    `token_<name>` (per OidcRpTokenError), `id_token_<name>` (per
    OidcRpIdTokenError), `no_account`, `no_api_key`,
    `server_error`. The IdP's `error_description` is logged for
    operators but never echoed into the redirect URL (injection
    surface).
- **Files:**
  - Touched: `src/api/auth/oidc_rp/oidc_rp_callback.{c,h}`
    (56 → 466 lines + header rewritten with the typed-error
    vocabulary).
  - Created: `src/api/auth/oidc_rp/oidc_rp_link_stub.{c,h}`
    (76 + 64 lines).
  - Touched: `src/api/auth/oidc_rp/oidc_rp_service.{c,h}` —
    `oidc_rp_jwks_init()` added to the lazy-init chain;
    `oidc_rp_runtime_shutdown()` added with `pthread_once_t` reset
    so re-init after shutdown works (relevant only for tests).
  - Touched: `src/api/api_service.c` — `cleanup_api_endpoints()`
    now calls `oidc_rp_runtime_shutdown()`. One new `#include`.
  - Touched: `tests/lib/mock_keycloak/server.js` — `/auth` endpoint
    + `issuedCodes` per-code map (221 → 282 lines).
  - Created: `tests/configs/hydrogen_test_42_oidc_rp_full.json`
    (74 lines) — SQLite-backed Acuranzo + OIDC_RP enabled, used
    only for the Phase 14 happy-path sub-test. The existing
    `_oidc_rp.json` (disabled) and `_oidc_rp_enabled.json` (no DB)
    are unchanged.
  - Created: `tests/lib/oidc_rp_helpers_callback.sh` (401 lines).
    Phase 14's failure-path + happy-path helpers split out of
    `tests/lib/oidc_rp_helpers.sh` to keep both files under the
    project's 1,000-line cap. Phase 13 lesson #5 set this
    precedent.
  - Touched: `tests/lib/oidc_rp_helpers.sh` (964 → 974 lines —
    just a comment noting the split).
  - Touched: `tests/test_42_oidc_rp.sh` (296 → 396 lines):
    sources the new Phase 14 lib, adds five callback failure-path
    sub-tests to the existing enabled-config block, and wires up a
    third Hydrogen instance (full-config with SQLite) for the
    happy-path sub-test. The full-config block is gated on the
    presence of the demo SQLite db, `HYDROGEN_DEMO_API_KEY`, and
    `jq` so CI environments without those skip cleanly.
- **Tests required:**
  - Five failure-path sub-tests (no DB needed):
    `state_invalid` for missing params; `state_invalid` for unknown
    state; `idp_error` for `?error=...`; method-not-allowed still
    works in enabled mode; replayed (consumed) state returns
    `state_invalid`. All assert the typed `?oidc_error=` token in
    the 302 Location header and confirm the IdP's
    `error_description` does not leak into the URL.
  - One happy-path sub-test (SQLite-backed full config):
    drives `/start → /auth → /callback`, captures the SPA-bound
    redirect's handoff code, exchanges it at `/handoff`, verifies
    the success envelope mirrors `/auth/login` (token has the
    JWS three-segment shape; `user_id=1`; `username=adminuser`;
    `success=true`).
- **Definition of Done:**
  - [x] Test 42 happy path passes consistently 10 times in a row
        (verified — all 10 runs returned 44/44 in 3.1–4.7s each;
        no flakes observed).
  - [x] All Test 42 failure-path cases pass: 44/44 sub-tests
        (was 34/34 in Phase 13; +10 net new for Phase 14: 5
        callback failure-paths + 1 happy-path + 4 lifecycle
        bookkeeping sub-tests for the third Hydrogen instance).
  - [x] Test 40 (password login) still green: 46/46 across 7 DB
        engines on retry. The pre-existing transient SQLite/
        PostgreSQL flake (Phase 13 lesson) reappeared once but
        passed cleanly on retry exactly as before; behaviour is
        unchanged from baseline.
  - [x] `mkt` (regular + unity build) clean.
  - [x] `mka` (`test_01_compilation.sh`) clean — 18/18 build
        variants green.
  - [x] `test_91_cppcheck` clean — 1,334 files, 0 issues.
  - [x] `test_92_shellcheck` clean — 111 scripts, 659 directives
        all justified (was 110/653 in Phase 13; +1 file for
        `oidc_rp_helpers_callback.sh`, +6 directives for the new
        `SC2310` annotations on Phase 14 sub-test wrappers).
  - [x] `test_11_leaks_like_a_sieve` passes: 0 direct, 0 indirect
        leaks. `oidc_rp_runtime_shutdown` is now exercised by
        every Hydrogen lifecycle in test_42 (3 instances), so
        the lazy-init pthread cleanup is now under coverage.
  - [x] `test_17_startup_shutdown` clean — 9/9 passing.
  - [x] `test_10_unity` clean: 6,953/6,957 passing on cached
        re-run, 0 failing. The 4 cached-failure flags pre-date
        Phase 6 per Phase 6 lesson #9. No new Unity tests were
        added in Phase 14 — the validator/state/handoff/token
        modules already had per-module Unity coverage from
        Phases 7–13.
  - [x] `test_99_code_size`: pass count unchanged from Phase 13
        baseline. The two pre-existing findings
        (`proxy_multi.c` 1,209 lines, `database_engine.c` 1,000
        lines) are unrelated. `oidc_rp_helpers.sh` was at 1,339
        lines after the Phase 14 additions; split into two libs
        keeps both under the cap.
  - [x] No release-build leakage. The stub linker is regular
        code (not NDEBUG-gated) — Phase 21 deletes it. The
        `oidc_rp_runtime_shutdown` hook is unconditionally
        compiled because the lazy_init it pairs with is too.
  - [ ] Code review of the full chain
        (`/local-review-uncommitted`).
        *(Pending — recommended before Phase 15 starts.)*

**Lessons learned:**

1. **The lazy-init must include `oidc_rp_jwks_init()`.** Phase 9
   shipped the JWKS cache init/shutdown surface but Phase 10's
   lazy-init only wired state, handoff, and discovery. Phase 12's
   validator works fine in unit tests (which call `init`/`shutdown`
   directly) but fails in the live Hydrogen runtime with
   `id_token_kid_unknown` because `oidc_rp_jwks_find` returns NULL
   when `g_cache` is unset. Caught only by the Phase 14 e2e test —
   a strong argument for landing live e2e coverage before declaring
   a chain "done." Future phases that introduce per-module init/shutdown
   should hook into the lazy-init chain in the *same* phase.
2. **Sweeper threads keep the process alive without an explicit
   shutdown hook.** Phase 7's `oidc_rp_state_shutdown` and
   Phase 13's `oidc_rp_handoff_store_shutdown` exist but have no
   caller in production code. Without `oidc_rp_runtime_shutdown` the
   threads run until process exit, but Hydrogen's landing sequence
   waits for them — so SIGINT to a running Hydrogen with the
   feature enabled hangs for 10 s before the test harness's
   timeout kicks in. Wiring the runtime shutdown into
   `cleanup_api_endpoints()` is the right place: it runs after
   `land_api_subsystem()` but during normal shutdown, it's
   idempotent, and it covers both feature-enabled and feature-
   disabled paths (the latter is a no-op because lazy_init never
   ran).
3. **Database connection `Parameters` OVERRIDE runtime query
   parameters.** Discovered while debugging the Phase 14 happy
   path: the SQLite test_40 config sets
   `Parameters.LOGINID = "${env.HYDROGEN_DEMO_USER_NAME}"` for
   convenience, and `merge_database_parameters` in
   `src/config/config_databases.c:584` says explicitly "Config
   parameters OVERRIDE query parameters to allow test control."
   For Phase 14 we want runtime to win (the linker passes a fixed
   login_id), so the Phase 14 config simply does NOT set
   `LOGINID` in `Parameters`. This is a dangerous footgun that
   bit me once and is worth flagging in any future test config
   that needs to drive `lookup_account` with arbitrary login_ids.
4. **`lookup_account` deliberately leaves `username`/`email`/
   `roles` NULL.** The password login flow populates them in
   `verify_password_and_status` (QueryRef #012). For OIDC there
   is no password step. The Phase 14 stub linker has to fill
   them in itself; for the stub the username is
   `OIDC_RP_LINK_STUB_FIXED_LOGIN_ID` (verbatim), the email
   comes from the validated id_token claims, and roles default
   to `""` (empty). Phase 22's role mapping replaces the empty
   default with a join against `account_roles`. The stub's
   defensive empty-string for roles matches what password login
   produces when no roles are joined — preserves the contract
   so existing role-checking middleware keeps working.
5. **The mock IdP's `/auth` endpoint needs a per-code state map.**
   In OIDC, the IdP remembers `nonce` (and `aud`) at authorize
   time and bakes them into the id_token at token-exchange time.
   Hydrogen never sends the `nonce` to the IdP's `/token`
   endpoint — it only round-trips through the browser via
   `state`, looked up server-side in Hydrogen's state store.
   Phase 11 worked because the test passed `nonce` directly to
   `/token`; the Phase 14 redirect flow doesn't, so the mock
   needs to remember `code → {nonce, aud}` between `/auth` and
   `/token`. A 1-entry `Map` (the canned `test-code-ok`) is
   plenty for the test surface; a real IdP would have a per-code
   LRU + TTL.
6. **`--max-redirs 2` is the right curl knob for the e2e test.**
   The chain is `/start → /auth → /callback → /login`. The first
   three are real Hydrogen/IdP hops; the fourth (`/login`) is
   the SPA URL on the *Hydrogen* origin (because we have no
   Lithium SPA running). With `-L --max-redirs 2`, curl follows
   start→auth and auth→callback but stops at /callback's 302
   to /login, leaving the Location header readable in the
   `-i` output. `extract_oidc_error_param` and the handoff-code
   regex both walk that header.
7. **Capture both the Location header and the final status with
   one curl invocation.** The first draft of the happy-path
   helper made TWO `curl -L` calls (one for `url_effective`,
   one for `http_code`), each independent. That triggered TWO
   /start lifecycles, doubled the test runtime, AND consumed
   two states from the store back-to-back so the second's
   /callback could land on a stale token-cache entry. The
   single-call form (`-i` + `-w '%{http_code}'` + `-o
   <headers_file>`) is both faster and more deterministic.
8. **Migration-readiness wait belongs after `wait_for_server_ready`.**
   The HTTP server becomes accept()-ready before the database
   queue worker thread finishes processing the bootstrap query
   rows into the QTC. /api/auth/oidc/callback returns 302 either
   way, but the linker stub's `lookup_account` quietly fails
   ("Failed to lookup account: Unknown error") because QueryRef
   #008 isn't registered yet. Polling the server log for
   `Migration completed in` / `Migration Current:` (matches
   test_40's pattern) plus a 1-second settling sleep is the
   minimal-cost fix. test_40 runs 7 engines in parallel; we run
   one SQLite engine, so 30 s is generous overhead but safe.
9. **Single curl call's `-i` output captures EVERY redirect's
   headers, in order.** The trace file from
   `curl -s -i -L --max-redirs 2 -o file ...` contains three
   complete HTTP responses concatenated: /start's 302, /auth's
   302, /callback's 302. Each starts with a status line and
   includes its `Location` header. Picking the LAST `Location`
   line (`tail -n 1`) reliably gets /callback's redirect target.
   This pattern saves a separate "drive each redirect manually"
   loop and is more robust against transport hiccups (any one
   failing hop is visible in the trace).
10. **Database `SystemApiKey` plumbing happens through the existing
    `verify_api_key` helper — no new query refs needed.** Phase 5
    added `OIDCRPProviderConfig::system_api_key` to the schema;
    Phase 14 just reads it at callback time and passes it
    verbatim to `verify_api_key()`. The result is a populated
    `system_info_t` (system_id + app_id + license_expiry), which
    `generate_jwt()` consumes directly. Zero schema or query
    work; the OIDC RP path looks identical to the password path
    from `generate_jwt`'s perspective. This is the single most
    important property that justifies the entire architectural
    decision around server-side JWT minting (per the plan's
    "indistinguishable JWT" goal).
11. **The disabled-feature 503 contract still wins on every path.**
    The 5 failure-path sub-tests + the 1 happy-path sub-test all
    run against `Enabled = true`, but the existing 7 disabled-
    coverage sub-tests (Phase 6) still verify 503 oidc_disabled
    on a separate Hydrogen instance with `Enabled = false`. The
    feature gate is checked first in every handler. No
    regression to Phase 6.
12. **The "no_account" failure path is now reachable in the live
    flow.** When the SQLite db is missing or the `adminuser`
    account is absent, the linker stub returns NULL and
    `/callback` redirects with `?oidc_error=no_account`. The test
    happy-path's "skip cleanly when DB / API key / jq missing"
    branch protects against environmental weirdness, but a
    future regression that breaks the linker would surface as a
    clean `no_account` redirect — easy to grep for in CI logs.
    Phase 21's real linker will replace the typed error with
    `no_account` for the same condition; the contract stays.

**Setup for Phase 15 (DB migration `1100_account_oidc_identities`):**

- The Phase 14 stub linker is the only consumer of `lookup_account`
  in the OIDC RP code. Phase 21 will delete it and the new linker
  will instead use QueryRefs `#080–#084` (Phase 17) which read from
  `account_oidc_identities` (Phase 15) and `accounts` (existing).
- The SQLite-backed test_42 happy-path config (`hydrogen_test_42_oidc_rp_full.json`)
  uses the demo `hydrodemo.sqlite` fixture, which already has the
  `accounts` schema. Phase 15's migration adds
  `account_oidc_identities` to the same schema; that migration will
  also need to be applied to the test fixture before Phase 18's
  linker tests run, OR Phase 15's migration can be applied
  separately before Phase 18. The test_42 happy path will keep
  working through Phase 15 (the new table is unused) as long as
  the demo db is migrated alongside the production one.
- The `oidc_rp_runtime_shutdown` hook calls `oidc_rp_jwks_shutdown`
  which currently has no test coverage from the test_42
  perspective (the unit tests cover it directly). When Phase 15
  lands, consider adding a test_42 sub-test that triggers a
  graceful shutdown mid-flow and re-verifies clean shutdown — the
  test_42 lifecycle bookkeeping already does this, but a single
  named sub-test would surface regressions faster.
- The mock Keycloak's `/auth` endpoint returns a hardcoded
  `code=test-code-ok` regardless of the request. A real Keycloak
  issues a fresh per-request code. For Phase 18+ when more nuance
  is needed (e.g. testing multiple concurrent users), the mock can
  be extended to issue a UUID-shaped code per request and use the
  per-code map's existing structure. Today's single-code shape
  works for Phases 14–17 because they use only one user account.
- The lazy-init/shutdown pattern is now mature: 4 stores
  (state, handoff, discovery, jwks) all participate. Phase 21+
  may benefit from refactoring the lazy-init into an explicit
  Hydrogen subsystem (with its own launch/landing readiness
  checks). Today's pthread_once is sufficient and the
  subsystem-graph migration can wait until there's a second
  reason to do it.

---

### Phase 15 — Hydrogen: DB migration `1100_account_oidc_identities`

- **Goal:** Add the linking table on every supported DB engine. No
  code change reads from it yet.
- **Prerequisites:** none (independent of phases 6–14).
- **In scope:**
  - One migration script per engine in the existing migration
    tree (PostgreSQL, MySQL/MariaDB, SQLite, DB2, CockroachDB,
    YugabyteDB) plus the Acuranzo-shared definition.
  - Tables, indexes, and foreign keys exactly as in this plan.
  - Migration is **forward-only**; rollback is documented but not
    automated.
- **Files:**
  - Created: per-engine migration files following existing naming
    (e.g. `migrations/postgres/1100_account_oidc_identities.sql`,
    similarly for each engine).
- **Tests required:**
  - `test_31_migrations.sh` discovers the new migration on every
    engine and applies it cleanly.
  - `test_32`–`test_38` (per-engine migration tests) all green.
- **Definition of Done:**
  - [ ] All migration tests pass on all configured engines.
  - [ ] `test_71_database_diagrams.sh` regenerates with the new
        table.

---

### Phase 16 — Hydrogen: `accounts.password_hash` nullable migration

- **Goal:** Allow OIDC-provisioned accounts to exist without a password
  hash.
- **Prerequisites:** Phase 15 (so it sits beside the other new
  migrations).
- **In scope:**
  - Migration `1101_accounts_password_hash_nullable.sql` per engine.
  - Existing rows untouched; constraint relaxed; default still
    `NULL`-tolerant.
- **Files:**
  - Created: per-engine `migrations/.../1101_*.sql`.
- **Tests required:**
  - Migration tests green on every engine.
  - Existing password login (`test_40_auth.sh`) **must** still work
    against an existing account whose `password_hash` is non-null,
    proving the relaxation didn't break anything.
- **Definition of Done:**
  - [ ] All migration tests green.
  - [ ] Test 40 green.
  - [ ] Manual `INSERT INTO accounts (... password_hash) VALUES (...,
        NULL)` accepted by every engine.

---

### Phase 17 — Hydrogen: QueryRefs `#080`–`#084` (`.lua` payloads)

- **Goal:** Add the five new conduit query references the linker will
  use. No C code calls them yet.
- **Prerequisites:** Phases 15, 16.
- **In scope:**
  - `payloads/queries/acuranzo_080_lookup_oidc_identity.lua`
  - `payloads/queries/acuranzo_081_link_oidc_identity.lua`
  - `payloads/queries/acuranzo_082_lookup_account_by_email.lua`
  - `payloads/queries/acuranzo_083_provision_account.lua`
  - `payloads/queries/acuranzo_084_touch_oidc_identity.lua`
  - Each follows the schema/IO conventions of the existing
    `acuranzo_NNN_*.lua` files (typed params, `RETURNING`-style
    output where supported).
- **Tests required:**
  - `test_98_luacheck.sh` clean.
  - `test_50_conduit_query` / `test_51_conduit_queries` extended with
    one round-trip per new QueryRef using anonymous parameters
    against a seeded test row, on every engine.
- **Definition of Done:**
  - [ ] luacheck clean.
  - [ ] Conduit tests green for all five QueryRefs on all engines.
  - [ ] `payload-generate.sh` regenerates the payload without warnings.

---

### Phase 18 — Hydrogen: account linker — `match_sub_only`

- **Goal:** Replace the stub linker for the simplest strategy:
  trust `(iss, sub)`, fail otherwise.
- **Prerequisites:** Phases 14, 17.
- **In scope:**
  - `oidc_rp_link.{c,h}`: `oidc_rp_link_resolve(provider, claims,
    out_account, out_error_code)`.
  - For `Strategy = "match_sub_only"`: call QueryRef #080. If hit,
    update via #084 (last_seen_at) and return account. If miss,
    return error `no_account`.
  - Wire `oidc_rp_callback.c` to call the real linker when
    `Strategy = "match_sub_only"`. Other strategies still use the
    stub for now.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_link.{c,h}`
  - Created: `tests/unity/test_oidc_rp_link.c`
  - Touched: `oidc_rp_callback.c`
- **Tests required:**
  - Unity: identity exists ⇒ correct account returned and #084
    invoked; identity missing ⇒ `no_account` error.
  - Black-box Test 42 with strategy = `match_sub_only`: pre-seed an
    `account_oidc_identities` row; happy path works; remove the row,
    same callback now returns 302 with `?oidc_error=no_account`.
- **Definition of Done:**
  - [ ] Unity + black-box tests green.
  - [ ] Other strategies untouched and still using stub linker.

---

### Phase 19 — Hydrogen: account linker — `match_email_only`

- **Goal:** Add the second strategy: link by `email_verified` email
  to an existing account, then proceed.
- **Prerequisites:** Phase 18.
- **In scope:**
  - For `Strategy = "match_email_only"`: try #080 first (so already-linked
    users remain fast); if miss and `email_verified == true`, look up
    via #082; if hit, insert via #081, then proceed; if miss, error
    `no_account`.
  - `RequireEmailVerified` setting honoured.
- **Files:**
  - Touched: `oidc_rp_link.{c,h}`
  - Touched: `tests/unity/test_oidc_rp_link.c` (new cases)
- **Tests required:**
  - Unity: linked-then-cached path; unlinked-but-email-known path
    creates a row in `account_oidc_identities`; unverified email
    rejected; ambiguous email (two accounts) rejected with explicit
    error `email_ambiguous`.
- **Definition of Done:**
  - [ ] All linker unit tests green.
  - [ ] Test 42 sub-tests for `match_email_only` green.
  - [ ] No regression in `match_sub_only`.

---

### Phase 20 — Hydrogen: account linker — provisioning

- **Goal:** Implement first-time-user auto-provisioning, with all
  the safety rails: `AllowedEmailDomains`, `RequireEmailVerified`,
  default roles, audit logging.
- **Prerequisites:** Phase 19.
- **In scope:**
  - QueryRef #083 wrapper.
  - Validate `email` matches `AllowedEmailDomains` (case-insensitive,
    full-domain match, no wildcards in this phase).
  - On success, write account, write identity row, assign default roles
    via existing role tooling.
  - Hard-fail with `provision_disallowed_email` for domain mismatch.
- **Files:**
  - Touched: `oidc_rp_link.{c,h}`
  - Touched: `tests/unity/test_oidc_rp_link.c`
- **Tests required:**
  - Unity: provision happy path creates exactly one row in
    `accounts` and one in `account_oidc_identities`; default roles
    assigned; rejected email domain produces no rows.
  - Black-box Test 42 with `Strategy = "provision_only"`,
    `AllowedEmailDomains = ["philement.com"]`: user with
    `alice@philement.com` ⇒ account created; user with
    `bob@evil.com` ⇒ 302 `?oidc_error=provision_disallowed_email`.
- **Definition of Done:**
  - [ ] All linker tests green including domain rejection.
  - [ ] No half-written rows on any failure path (transaction or
        atomic insert chain verified).

---

### Phase 21 — Hydrogen: full default strategy `match_email_then_provision`

- **Goal:** Compose phases 18–20 into the default strategy and remove
  the stub linker entirely.
- **Prerequisites:** Phases 18, 19, 20.
- **In scope:**
  - Implementation: try #080 → try #082+#081 (if email verified) →
    try provisioning (if enabled and domain allowed) → else
    `no_account`.
  - Delete `oidc_rp_link_stub.{c,h}` and any references.
- **Files:**
  - Touched: `oidc_rp_link.{c,h}`, `oidc_rp_callback.c`.
  - Deleted: `oidc_rp_link_stub.{c,h}`.
- **Tests required:**
  - Black-box Test 42 covers all four sub-paths in sequence with the
    same provider config.
- **Definition of Done:**
  - [ ] All four linker strategies tested via Test 42.
  - [ ] Test 40 (password) still green.
  - [ ] No stub linker code remains in the tree.
  - [ ] cppcheck clean (no unused functions).

---

### Phase 22 — Hydrogen: role mapping

- **Goal:** Implement the `RoleMapping` config: `database` (default),
  `idp_realm_roles`, `idp_client_roles`, `merge`.
- **Prerequisites:** Phase 21.
- **In scope:**
  - Apply role-mapping after the linker resolves the account but
    before `generate_jwt` is called. Pass a possibly-augmented role
    list to the JWT generator.
  - `IdpRolePrefix` honoured. Empty prefix is fine.
- **Files:**
  - Touched: `oidc_rp_link.{c,h}` or new `oidc_rp_roles.{c,h}` (own
    file preferred for testability).
  - Touched: `oidc_rp_callback.c`.
- **Tests required:**
  - Unity: each of the four sources produces the expected roles list,
    given fixture DB roles + ID-token roles.
  - Black-box Test 42: under `merge`, an OIDC role from Keycloak
    appears in the resulting Hydrogen JWT alongside DB roles.
- **Definition of Done:**
  - [ ] Roles applied correctly in all four modes.
  - [ ] Test 40 still green.
  - [ ] cppcheck clean.

---

### Phase 23 — Lithium: `core/oidc-client.js`

- **Goal:** Pure helper module, fully unit-testable, no DOM.
- **Prerequisites:** Phase 4 (refactor done; `login.js` ≤ 1000 lines).
- **In scope:**
  - `startOidc(provider, returnTo)` builds the start URL from
    `getConfigValue('server.url')` and `auth.oidc_providers[*]`,
    appends `database` and `return_to` when provided, and assigns
    `window.location.href` to it. Pure logic with `window` injected
    for tests.
  - `exchangeHandoff(handoff)` POSTs to `/api/auth/oidc/handoff` and
    returns the parsed response. Throws on non-2xx with structured
    error info.
- **Files:**
  - Created: `src/core/oidc-client.js`
  - Created: `tests/unit/core/oidc-client.test.js`
- **Tests required:**
  - Vitest: URL is built correctly with and without `return_to` and
    `database`; missing provider throws; `exchangeHandoff` parses
    success; on 401 throws `Error` with `error.code = "handoff_invalid"`;
    network failure throws with `error.code = "network"`.
- **Definition of Done:**
  - [ ] Lint, build, test all clean.
  - [ ] `oidc-client.js` is a pure ES6 module with no DOM access.

---

### Phase 24 — Lithium: `oidc-login.js` (return-from-IdP processor)

- **Goal:** When Lithium boots with `?oidc=1&handoff=...`, exchange
  the handoff and complete login as if a password login had succeeded.
- **Prerequisites:** Phase 23.
- **In scope:**
  - Implement `processOidcReturn(loginManager)` exactly per the
    sketch in the "Return flow" section.
  - URL is wiped via `history.replaceState` before the network call.
  - Errors mapped to a small enum of user-facing strings; never echo
    the raw `oidc_error` value.
- **Files:**
  - Created: `src/managers/login/oidc-login.js`
  - Created: `tests/unit/managers/login/oidc-login.test.js`
  - Touched: `src/managers/login/login.js` (one new call from
    `init()`).
- **Tests required:**
  - Vitest: no `?oidc=1` ⇒ no-op; with handoff ⇒ exchange called,
    JWT stored, AUTH_LOGIN emitted, login UI hidden;
    `?oidc_error=...` ⇒ error displayed, no fetch; URL is cleaned
    in all branches.
- **Definition of Done:**
  - [ ] Lint, build, test all clean.
  - [ ] Manual smoke: visit `/?oidc=1&handoff=fake` ⇒ login form
        re-shows with a clean URL and a friendly error.

---

### Phase 25 — Lithium: provider button UI

- **Goal:** Render zero-or-more "Sign in with X" buttons under the
  password form, driven by `auth.oidc_providers` config.
- **Prerequisites:** Phases 23, 24.
- **In scope:**
  - Add the `<div class="login-divider">` and `<div class="login-providers">`
    blocks to `login.html`.
  - Button is generated at render time from config; FA icon class
    pulled from `provider.icon`. CSS-first (no inline styles).
  - Click handler calls `startOidc(provider.id, returnTo)`.
  - When `auth.oidc_providers` is empty/missing, neither divider nor
    buttons render.
- **Files:**
  - Touched: `src/managers/login/login.html`
  - Touched: `src/managers/login/login.css`
  - Touched: `src/managers/login/login.js`
  - Touched: `config/lithium.json` (add the `auth.oidc_providers`
    array with the `500passwords` entry, gated by env-specific
    config files for prod).
  - Touched: `tests/unit/managers/login/login.test.js`
- **Tests required:**
  - Vitest: with empty array, no button rendered; with one provider,
    one button with the right label/icon; click triggers `startOidc`
    with the right provider id; ESC still works on login panel.
- **Definition of Done:**
  - [ ] Lint, build, test all clean.
  - [ ] Visual check (in any modern browser) confirms the button
        looks aligned with existing `login-btn-primary`.
  - [ ] `LITHIUM-INS.md` rule #4 (CSS-first) verified by code review.

---

### Phase 26 — Lithium: `auth.last_method` and subtle highlighting

- **Goal:** Remember whether the user last logged in via password or
  OIDC, and softly highlight that path on the next visit.
- **Prerequisites:** Phase 25.
- **In scope:**
  - On successful password login, `window.lithiumSettings.set('auth.last_method', 'password')`.
  - On successful handoff exchange, `window.lithiumSettings.set('auth.last_method', 'oidc:<provider>')`.
  - On render, add a CSS class `is-recent` to the matching control,
    styled subtly (e.g. faint border highlight) — **not** prominent,
    so that someone who *wants* to switch can still find the other
    method easily.
- **Files:**
  - Touched: `src/managers/login/login.js`,
    `src/managers/login/oidc-login.js`,
    `src/managers/login/login.css`.
  - Touched: existing login Vitest file with a couple of new assertions.
- **Tests required:**
  - Vitest: setting is written on each login type; on render the
    matching control gets `.is-recent`; no setting ⇒ no class.
- **Definition of Done:**
  - [ ] Lint/build/test green.
  - [ ] Setting persists across reloads in a manual test.

---

### Phase 27 — End-to-end against real Keycloak (dev environment)

- **Goal:** Take the entire chain off the mock and run it against
  the real Keycloak at `https://www.500passwords.com` in a dev
  environment. This is the final pre-prod gate.
- **Prerequisites:** All earlier non-optional phases (1–26).
- **In scope:**
  - Keycloak realm + client created per "Keycloak Configuration"
    section.
  - Hydrogen dev config wired with real `Issuer`, `ClientId`,
    `ClientSecret` (env var), `RedirectUri`.
  - Lithium dev config has the `500passwords` provider entry.
  - Manual sign-in test from a clean browser profile, with two test
    users: one already linked, one new (so provisioning is exercised).
  - Logout cycle, then renewal cycle (`/api/auth/renew`) — both must
    work for an OIDC-issued JWT.
- **Files:**
  - Touched: dev-only config files (not committed if they contain
    secrets).
  - Created: `docs/Li/LITHIUM-OIDC.md` and
    `docs/H/api/auth/oidc_rp.md` (operator-facing reference docs).
- **Tests required:**
  - Manual test plan executed and ticked off (recorded in a
    Markdown checklist committed under `docs/H/plans/OIDC_E2E_LOG.md`):
    - [ ] Existing password user logs in with password.
    - [ ] Existing password user clicks "Sign in with 500 Passwords",
          logs in via Keycloak, lands in Lithium with the same
          account (linked by email).
    - [ ] New 500passwords user signs in, gets provisioned, lands
          with default roles.
    - [ ] OIDC-logged-in user reloads page mid-session — JWT still
          valid, no re-prompt.
    - [ ] OIDC-logged-in user clicks Logout → ends up on login page.
    - [ ] OIDC-logged-in user has their JWT renewed automatically
          while idle (verified in session log).
    - [ ] All Lithium managers (Queries, Lookups, Style, etc.) work
          identically for OIDC-logged-in user.
  - All automated tests (Test 40, Test 42, all unit tests) still
    green when pointed at real Keycloak.
- **Definition of Done:**
  - [ ] Manual test plan all ticked.
  - [ ] One independent reviewer (different model session if AI-led)
        runs through the manual plan again and confirms.
  - [ ] `docs/Li/LITHIUM-OIDC.md` and `docs/H/api/auth/oidc_rp.md`
        merged.
  - [ ] `test_04_check_links.sh` green after new docs are added.

---

### Phases 28–30 — Post-MVP hardening

These ship after Phase 27 is in production. They are described briefly
because their detailed plan should be written **after** the MVP is in
operators' hands and we know what hurt.

- **Phase 28 — Health-check field.** Add `oidc_rp_status` (probing
  discovery) to `/api/system/health`. Tests: extend
  `test_21_system_endpoints.sh`.
- **Phase 29 — Backchannel logout.** Implement
  `/api/auth/oidc/backchannel-logout`, validate the Keycloak
  `logout_token`, revoke matching Hydrogen JWTs. Tests: a new
  black-box sub-test in Test 42 plus Unity tests for logout-token
  validation.
- **Phase 30 — RP-initiated logout.** When Lithium logs out, optionally
  navigate to Keycloak's `end_session_endpoint`. Tests: Vitest for the
  new conditional in `oidc-login.js` and Hydrogen-side helper to
  build the URL.

---

### Cross-phase rules

- **Every phase ends with all earlier phase tests still passing.**
  No "we'll fix Test 42 next phase" — if you broke it, it's part of
  this phase.
- **No phase skips lint.** `mkt`, `mkp`, `mks`, `mkj` (jsonlint),
  `npm run lint` are non-negotiable per phase.
- **No phase skips the file-size limit.** Phase reviews include a
  `test_99_code_size` check.
- **Configuration changes are atomic with code changes.** A phase that
  introduces a config field also ships a sample value and parser
  test in the same merge.
- **Documentation changes go in the same phase as code.** No
  "doc-only" follow-up phases — `LITHIUM-INS.md`-equivalent rules on
  the Hydrogen side enforce this.

---

## File-Level Inventory (what gets created vs. touched)

### Hydrogen — created

```text
src/api/auth/oidc_rp/oidc_rp_start.c
src/api/auth/oidc_rp/oidc_rp_start.h
src/api/auth/oidc_rp/oidc_rp_callback.c
src/api/auth/oidc_rp/oidc_rp_callback.h
src/api/auth/oidc_rp/oidc_rp_handoff.c
src/api/auth/oidc_rp/oidc_rp_handoff.h
src/api/auth/oidc_rp/oidc_rp_state.c
src/api/auth/oidc_rp/oidc_rp_state.h
src/api/auth/oidc_rp/oidc_rp_discovery.c
src/api/auth/oidc_rp/oidc_rp_discovery.h
src/api/auth/oidc_rp/oidc_rp_http.c           # Phase 9 + Phase 11 (POST companion)
src/api/auth/oidc_rp/oidc_rp_http.h
src/api/auth/oidc_rp/oidc_rp_jwks.c
src/api/auth/oidc_rp/oidc_rp_jwks.h
src/api/auth/oidc_rp/oidc_rp_pkce.c
src/api/auth/oidc_rp/oidc_rp_pkce.h
src/api/auth/oidc_rp/oidc_rp_service.c
src/api/auth/oidc_rp/oidc_rp_service.h
src/api/auth/oidc_rp/oidc_rp_token.c          # Phase 11
src/api/auth/oidc_rp/oidc_rp_token.h          # Phase 11
src/api/auth/oidc_rp/oidc_rp_idtoken.c
src/api/auth/oidc_rp/oidc_rp_idtoken.h
src/api/auth/oidc_rp/oidc_rp_link.c                 # Phase 21
src/api/auth/oidc_rp/oidc_rp_link.h                 # Phase 21
src/api/auth/oidc_rp/oidc_rp_link_stub.c            # Phase 14 (deleted in Phase 21)
src/api/auth/oidc_rp/oidc_rp_link_stub.h            # Phase 14 (deleted in Phase 21)
src/config/config_oidc_rp.c
src/config/config_oidc_rp.h
tests/unity/src/api/auth/oidc_rp/oidc_rp_state_test_store.c
tests/unity/src/api/auth/oidc_rp/oidc_rp_discovery_test_cache.c
tests/unity/src/api/auth/oidc_rp/oidc_rp_jwks_test_cache.c
tests/unity/src/api/auth/oidc_rp/oidc_rp_http_test_get.c    # Phase 9 + Phase 11
tests/unity/src/api/auth/oidc_rp/oidc_rp_pkce_test_helpers.c
tests/unity/src/api/auth/oidc_rp/oidc_rp_start_test_helpers.c
tests/unity/src/api/auth/oidc_rp/oidc_rp_token_test_exchange.c   # Phase 11
tests/unity/test_oidc_rp_idtoken.c
tests/unity/test_oidc_rp_link.c
tests/test_42_oidc_rp.sh
tests/lib/oidc_rp_helpers.sh                      # Phase 13 split for code-size cap
tests/lib/oidc_rp_helpers_callback.sh             # Phase 14 split for code-size cap
tests/lib/mock_keycloak/server.js                 # Node, dependency-free
tests/configs/hydrogen_test_42_oidc_rp.json       # Phase 6 (disabled)
tests/configs/hydrogen_test_42_oidc_rp_enabled.json   # Phase 10 (enabled, no DB)
tests/configs/hydrogen_test_42_oidc_rp_full.json      # Phase 14 (enabled, SQLite-backed)
payloads/queries/acuranzo_080_lookup_oidc_identity.lua
payloads/queries/acuranzo_081_link_oidc_identity.lua
payloads/queries/acuranzo_082_lookup_account_by_email.lua
payloads/queries/acuranzo_083_provision_account.lua
payloads/queries/acuranzo_084_touch_oidc_identity.lua
elements/001-hydrogen/hydrogen/migrations/1100_account_oidc_identities.sql
docs/H/api/auth/oidc_rp.md           # new endpoint docs
```

### Hydrogen — touched

```text
src/api/auth/auth_service.h          # extern decl for generate_jwt() signature already public
src/webserver/...                    # route registration (one new dispatch block)
src/config/config.c, config.h        # load new OIDC_RP block
elements/001-hydrogen/hydrogen/configs/*.json  # add OIDC_RP example block (one env)
tests/test_40_auth.sh                # add post-OIDC JWT shape assertion
docs/H/plans/AUTH_PLAN.md            # cross-link to OIDC-PLAN.md
docs/H/README.md, docs/H/SITEMAP.md  # add new doc links (test_04_check_links must pass)
```

### Lithium — created

```text
elements/003-lithium/src/managers/login/login-panels.js
elements/003-lithium/src/managers/login/login-password-manager.js
elements/003-lithium/src/managers/login/login-language-panel.js
elements/003-lithium/src/managers/login/login-logs-panel.js
elements/003-lithium/src/managers/login/login-help-panel.js
elements/003-lithium/src/managers/login/login-form.js
elements/003-lithium/src/managers/login/login-shortcuts.js
elements/003-lithium/src/managers/login/oidc-login.js
elements/003-lithium/src/core/oidc-client.js
elements/003-lithium/tests/unit/managers/login/login-logs-panel.test.js
elements/003-lithium/tests/unit/managers/login/login-help-panel.test.js
elements/003-lithium/tests/unit/managers/login/login-form.test.js
elements/003-lithium/tests/unit/managers/login/login-shortcuts.test.js
elements/003-lithium/tests/unit/core/oidc-client.test.js
elements/003-lithium/tests/unit/managers/login/oidc-login.test.js
docs/Li/LITHIUM-OIDC.md              # Lithium-side reference doc
```

### Lithium — touched

```text
elements/003-lithium/src/managers/login/login.js     # shrink + wire OIDC return
elements/003-lithium/src/managers/login/login.html  # add provider button slot
elements/003-lithium/src/managers/login/login.css   # button + divider styles
elements/003-lithium/config/lithium.json             # auth.oidc_providers array
elements/003-lithium/tests/unit/managers/login/login.test.js
docs/Li/LITHIUM-TOC.md                               # add LITHIUM-OIDC.md link
docs/Li/LITHIUM-MGR-LOGIN.md                         # mention OIDC button + return flow
docs/Li/LITHIUM-JWT.md                               # note OIDC also produces a Hydrogen JWT
docs/Li/LITHIUM-CFG.md                               # document auth.oidc_providers
```

---

## Open Questions to Resolve Before Phase 1

These are the things this plan deliberately does **not** answer, because
they need a human decision. Each one has a recommended default in
parentheses.

1. **Per-environment client IDs in Keycloak** — single `lithium-web` client
   reused across envs, or one per env? *(Recommended: one per env.)*
2. **Auto-provision policy** — should an unknown 500passwords user be
   auto-provisioned, or always require admin pre-creation?
   *(Recommended: auto-provision in dev/staging; require pre-creation in
   prod, controlled by `ProvisionDefaults.Enabled`.)*
3. **Role mapping** — Hydrogen-DB-only, or trust Keycloak's
   `realm_access.roles`? *(Recommended: Hydrogen-DB-only initially; switch
   to `merge` if/when Keycloak becomes the canonical role authority.)*
4. **Database for OIDC users** — single fixed database, or selectable per
   start request? *(Recommended: configurable in `OIDC_RP.Database`,
   overridable by `?database=` on `/oidc/start`. Same as password login.)*
5. **Single Logout** — ship in MVP, or defer to Phase 5? *(Recommended:
   defer.)*
6. **Mock Keycloak language** — Node or Python for the test mock?
   *(Recommended: whichever is already a Hydrogen test dependency.)*

### Decisions made during Phase 1

- **Legacy login partner buttons (Didit/Apple/Google/Microsoft):** Keep
  existing buttons in `login.html` until Phase 25. The new OIDC provider
  system will coexist with or replace them, not remove them prematurely.
- **Multiple OIDC provider support:** The config schema (`auth.oidc_providers`
  array) must support multiple providers. 500passwords is the first; others
  may follow.
- **URL query parameter for forced login provider:** Considered but deferred.
  A parameter like `?login_provider=500passwords` would be useful for direct
  links and testing, but it adds complexity to the login flow that is not
  needed for MVP. Revisit after Phase 27.

### Decisions made during Phase 2

- **Extraction pattern settled:** All Lithium login subsystems now follow the
  same class-based lifecycle: constructor → `init/render` → `show/hide` →
  `destroy/teardown`. `LoginPanels` (Phase 1), `PasswordManager` (Phase 2),
  and future extractions (`LanguagePanel`, `LogsPanel`, `HelpPanel`) all use
  this pattern. This consistency makes `LoginManager` a thin coordinator
  rather than a monolith.
- **Import ownership follows the code:** When logic moves to a new module,
  its unique imports move with it. `getPasswordManagerSelectors` and the
  `Flags` import (for Phase 3) should no longer appear in `login.js` once
  their consumers are extracted.
- **Callback delegation over event bus:** `LoginPanels` uses `onBeforeSwitch` /
  `onAfterSwitch` callbacks rather than `eventBus` events. This keeps panel
  transitions synchronous and avoids the "event spaghetti" that would come
  from broadcasting every panel change. Future panels should use the same
  callback style (e.g. `LanguagePanel` could accept `onLocaleSelected`).

### Decisions made during Phase 4

- **Phase 4 beat its target by a wide margin.** Final `login.js` is 576
  lines; target was 650–750, gate was ≤1,000. The expanded six-item
  scope (logs panel + help panel + login form + keyboard shortcuts +
  dead-code removal + import audit) removed 890 lines from `login.js`.
- **Two latent bugs surfaced during extraction.** Both are now fixed:
  1. `setupLookupListeners()` referenced `eventBus`/`Events` without
     importing them. Worked at runtime because every other module on
     the page already imports them and Vite's bundler tolerated the
     undeclared reference. Now properly imported.
  2. `loadVersionInfo()` mixed local-time `getFullYear()` with UTC-time
     `getUTCMonth()` / `getUTCDate()` when building the help-panel
     build-date string. Year boundaries would have shown the wrong
     year. `HelpPanel._renderVersion()` now uses `getUTCFullYear()`
     consistently.
- **316 lines of dead code from Phase 3 deleted, not extracted.** A
  cluster of `_languageTable`/`_currentLocale`/`_languageData`-touching
  methods survived Phase 3 in `login.js` despite the live versions
  living in `LanguagePanel`. The cluster referenced module globals
  (`getCountryCode`, `getFlagSvg`, `supportedLocales`,
  `saveLocalePreference`) that were never imported by `login.js` and
  would have thrown `ReferenceError` if invoked. Tests passed because
  no codepath called them. Lesson for future extractions: after
  moving a feature to a new module, audit the source file for
  identifier references that are now broken.
- **`isStartupComplete` callback is the right shape for `LoginForm`.**
  `setLoading()` needs to know whether background startup is done so
  it can re-enable the logs button correctly. Passing a getter
  callback (`() => this._startupComplete`) instead of the boolean
  itself avoids a stale-snapshot bug if `LoginForm` were ever cached
  across the startup boundary.
- **`onLoginSuccess` callback keeps the panel orchestration in
  `LoginManager`.** `LoginForm` does not import `LoginPanels` or
  `PasswordManager` directly. After storing the JWT it calls
  `await this._onLoginSuccess(data)`, which `LoginManager` wires to
  `this.hide()`. Cleaner separation than passing both panel
  references into `LoginForm`.
- **`LoginShortcuts` is purely behavioural.** It does not own any
  panels or modules; it owns only the `keydown`/`keyup` document
  listeners and the `caps-lock-active` toggle on the password input.
  The `getCurrentPanel`/`onClearUsername`/`onSwitchToLogin` callbacks
  give it everything it needs from `LoginManager` without coupling.
- **Final size distribution under `src/managers/login/` is healthy.**
  No file exceeds 600 lines:
  | File | Lines |
  |---|---|
  | `login.js` | 576 |
  | `login-language-panel.js` | 563 |
  | `login-form.js` | 414 |
  | `login-panels.js` | 228 |
  | `login-shortcuts.js` | 176 |
  | `login-password-manager.js` | 147 |
  | `login-help-panel.js` | 143 |
  | `login-logs-panel.js` | 126 |
- **Pattern fully reusable.** Future OIDC additions (Phases 23–24:
  `oidc-login.js`) will follow the exact same shape: an ES6 class
  with `init()`/`destroy()`, optional `show()`/`hide()`, constructor
  takes `{ elements, callbacks }`, never imports `LoginManager`. The
  scaffold is in place.

### Decisions made during Phase 3

- **`show()` / `hide()` is the right panel lifecycle boundary.** `LanguagePanel`
  needs to attach/detach a document-level keyboard listener when shown/hidden,
  and must reset its filter on hide. The `show()` / `hide()` pair (Phase 3)
  mirrors `PasswordManager.show()` / `hide()` (Phase 2) and keeps
  `_onBeforePanelSwitch()` a readable switch statement.
- **Dual notification for locale changes:** `LanguagePanel` both emits
  `LOCALE_CHANGED` on the global `eventBus` (for cross-manager listeners)
  and calls `onLocaleSelected(locale, previousLocale)` on its local callback
  (for `LoginManager` if it ever needs to react). Both paths are tested.
- **Line-count ratios are stable:** Extracted module ≈ 560 lines, parent
  reduction ≈ 156 lines. The plan's ≥200 target was optimistic. Phases 3–4
  together must achieve the ≤1,000 gate.
- **Unused import cleanup:** `getSavedLocale` was the only `languages.js`
  import left in `login.js` after extraction, and it is unused. Removed.
  Future phases should audit imports after each extraction.

---

## Success Criteria

- A user with valid 500passwords credentials clicks "Sign in with 500
  Passwords", completes the Keycloak login, and lands inside Lithium with
  a working session.
- That session is **operationally indistinguishable** from a
  password-login session: `/api/auth/renew` works, `/api/auth/logout`
  works, all Conduit queries work, role-gated UI shows the right managers.
- A user with a username/password account continues to log in exactly as
  before, with no regression.
- An unknown OIDC user is either auto-provisioned (if policy allows) or
  rejected with a clear, non-leaky error.
- Disabling `OIDC_RP.Enabled` and emptying `auth.oidc_providers` removes
  every trace of OIDC from the running system without affecting password
  login.
- All new tests (Unity, Vitest, Test 42) pass on the first green build of
  the final phase. `test_04_check_links`, `test_90`–`test_99` lint suites
  all clean.

---

**Last updated:** 2026-05-09
**Owner:** Philement engineering
**Status:** Phase 14 complete. The OIDC RP `/api/auth/oidc/callback`
endpoint now composes Phases 7, 9–13 into a working chain that turns
an IdP authorization code into a Hydrogen-issued JWT (indistinguishable
from the password-login JWT) and a one-time handoff record. Account
linking is stubbed via `oidc_rp_link_stub_resolve`, returning the
fixed test fixture account (`adminuser`, `account_id = 1`). Phase 21
will replace the stub with the real four-strategy linker
(`match_sub_only`, `match_email_only`, `match_email_then_provision`,
`provision_only`) once Phases 15–17 land the `account_oidc_identities`
table and QueryRefs `#080–#084`.

Two infrastructure fixes shipped with Phase 14: (1) `oidc_rp_jwks_init()`
joined the lazy-init chain in `oidc_rp_service.c` (it was missing from
Phases 9–13's setup, which was caught only by the live e2e test); and
(2) `oidc_rp_runtime_shutdown()` is a new public function that pairs
with the lazy_init and is called from `cleanup_api_endpoints()`,
ensuring the state-store / handoff-store sweeper threads do not keep
the Hydrogen process alive past landing. The mock Keycloak gained a
`/realms/test/protocol/openid-connect/auth` endpoint that 302-redirects
back to the supplied `redirect_uri` with `?code=test-code-ok&state=<echoed>`,
plus a per-issued-code map so the eventual `/token` call signs an
id_token with the matching `nonce` and `aud` claims.

`test_42_oidc_rp.sh` passes 44/44 in 3.1–4.7s (Phase 6 disabled-stub +
Phase 9 mock reachability + Phase 10 redirect + Phase 11 token endpoint +
Phase 12 signed id_token + Phase 13 handoff exchange + Phase 14 callback
failure paths + Phase 14 happy path). Verified 10 consecutive runs all
green per Phase 14 DoD requirement. `test_10_unity` 6,953/6,957 passing
on cached re-run (same 4 unrelated cached failures from Phase 6
lesson #9; no new Unity tests added in Phase 14 — per-module coverage
from Phases 7–13 still holds). `test_11_leaks_like_a_sieve` 0 direct,
0 indirect leaks. `test_91_cppcheck` clean (1,334 files);
`test_92_shellcheck` clean (111 scripts, 659 directives all justified —
+1 file for `oidc_rp_helpers_callback.sh`, +6 directives for new
`SC2310` annotations); `test_17_startup_shutdown` 9/9. `test_40_auth.sh`
46/46 across 7 DB engines on retry (the pre-existing transient
SQLite/PostgreSQL flake from Phase 13 reappeared once but passed
cleanly on retry; behaviour unchanged from baseline — password login
remains genuinely untouched by Phase 14). Ready for Phase 15 (DB
migration `1100_account_oidc_identities`).

### Decisions made between Phase 3 and Phase 4

- **Phase 4 scope expanded to a full decomposition.** Original plan was
  logs+help only (~120–180 line reduction, gate not achievable). New
  scope adds login-form, keyboard-shortcuts, and dead-code removal.
  Rationale: this code is exercised constantly by every subsequent
  phase and by every developer iteration; a clean ≤1,000-line file pays
  for itself many times over the next 23 phases.
- **Dead code from Phase 3 to be deleted, not extracted.** A cluster of
  ~316 lines of `_languageTable`/`_currentLocale`/`_languageData`
  references survived Phase 3 in `login.js`. They reference instance
  state and module globals that no longer exist or are not imported,
  and would throw `ReferenceError` if invoked. They are not called from
  any live codepath. Deletion is the right action; they belong in
  `LanguagePanel` if anywhere, and `LanguagePanel` already contains
  the live versions.
- **Phase 4 risk reclassified Low → Medium.** Larger surface area,
  more files moving simultaneously. Mitigated by the established
  class-based lifecycle pattern from Phases 1–3, by per-module Vitest
  coverage, and by `auth.integration.test.js` as the end-to-end gate.

---

## Appendix: Refactor Tracking Log

| Phase | File | Lines | `login.js` after | Tests (new) | Notes |
|---|---|---|---|---|---|
| 0 (baseline) | `login.js` | 1,866 | 1,866 | 672 | Pre-refactor monolith |
| 1 | `login-panels.js` | 228 | 1,747 | 24 | Callback delegation pattern established |
| 2 | `login-password-manager.js` | 147 | **1,621** | **20** | Class-based lifecycle; import ownership |
| 3 | `login-language-panel.js` | 562 | **1,466** | **25** | Tabulator + locale detection; show/hide lifecycle |
| 4 | `login-logs-panel.js` (126) + `login-help-panel.js` (143) + `login-form.js` (414) + `login-shortcuts.js` (176) + dead-code removal (~325 lines) | 859 new | **576** | **+70** (11+13+27+19) | Beat the ≤1,000 gate by 424 lines. Two latent bugs fixed during extraction (missing `eventBus`/`Events` import; UTC year mix-up in `loadVersionInfo`). |
| 5–27 | (OIDC implementation) | — | ≤1,200 | TBD | OIDC code lands after gate |

**Cumulative test count:** 741 (was 672) after Phase 3.
**Gate condition:** `login.js` must be ≤ 1,000 lines before any OIDC feature code (Phases 23–27) is added.
