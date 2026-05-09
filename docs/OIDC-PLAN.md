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

Suggested split (all under 1000 lines each):

| New file | Approx. lines extracted from `login.js` |
|---|---|
| `login-panels.js` (panel crossfade + ESC handling) | ~250 |
| `login-password-manager.js` (1Password suppression) | ~150 |
| `login-language-panel.js` (Tabulator init) | ~250 |
| `login-logs-panel.js` (CodeMirror init) | ~150 |
| `login-help-panel.js` (version info wiring) | ~100 |

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
| 1 | Lithium login refactor — extract `login-panels` | Lithium | Low |
| 2 | Lithium login refactor — extract `login-password-manager` | Lithium | Low |
| 3 | Lithium login refactor — extract `login-language-panel` | Lithium | Low |
| 4 | Lithium login refactor — extract `login-logs-panel` and `login-help-panel` | Lithium | Low |
| 5 | Hydrogen — `OIDCRelyingPartyConfig` schema and parser | Hydrogen | Low |
| 6 | Hydrogen — disabled-stub endpoints (`/start`, `/callback`, `/handoff`) | Hydrogen | Low |
| 7 | Hydrogen — in-memory state store | Hydrogen | Medium |
| 8 | Hydrogen — PKCE helpers (verifier, challenge, state, nonce) | Hydrogen | Low |
| 9 | Hydrogen — discovery + JWKS fetch and cache | Hydrogen | Medium |
| 10 | Hydrogen — `/oidc/start` redirect builder | Hydrogen | Medium |
| 11 | Hydrogen — token endpoint client (POST to Keycloak `/token`) | Hydrogen | Medium |
| 12 | Hydrogen — ID token validation | Hydrogen | High |
| 13 | Hydrogen — handoff store and `/oidc/handoff` exchange endpoint | Hydrogen | Medium |
| 14 | Hydrogen — `/oidc/callback` end-to-end with stub linker | Hydrogen | High |
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

### Phase 1 — Lithium: extract `login-panels.js`

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
  - Created: `elements/003-lithium/src/managers/login/login-panels.js`
  - Created: `elements/003-lithium/tests/unit/managers/login/login-panels.test.js`
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - New Vitest file unit-tests panel switching with mocked DOM elements:
    happy-path crossfade, ESC returns to login panel, double-click
    suppression.
  - All existing login tests still pass.
- **Definition of Done:**
  - [ ] `npm run lint` clean.
  - [ ] `npm run build` clean.
  - [ ] `npm test` 100% green.
  - [ ] `login.js` line count went **down** by at least 200 lines.
  - [ ] No file in `src/managers/login/` exceeds 1000 lines.
  - [ ] Manual smoke (against a running backend in someone's dev env, or
        screenshot diff) shows panels still crossfade identically.

---

### Phase 2 — Lithium: extract `login-password-manager.js`

- **Goal:** Move 1Password / password-manager DOM suppression logic out
  of `login.js`, no behaviour change.
- **Prerequisites:** Phase 1.
- **In scope:**
  - Create `login-password-manager.js` exporting `attach(rootEl)` /
    `detach()` / `cleanupAfterLogin()`.
  - Replace inlined MutationObserver + suppression code in `login.js`
    with calls to the module.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-password-manager.js`
  - Created: `elements/003-lithium/tests/unit/managers/login/login-password-manager.test.js`
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - Vitest: injection of a fake password-manager element triggers
    suppression styles within one tick; `detach()` removes the
    observer; `cleanupAfterLogin()` removes injected elements.
- **Definition of Done:**
  - [ ] `npm run lint` clean.
  - [ ] `npm run build` clean.
  - [ ] `npm test` 100% green.
  - [ ] `login.js` line count down by another ≥ 100 lines.

---

### Phase 3 — Lithium: extract `login-language-panel.js`

- **Goal:** Move the Tabulator language picker out of `login.js`.
- **Prerequisites:** Phase 1.
- **In scope:**
  - `login-language-panel.js` exports an init function that takes the
    container element + a callback `(localeCode) => void`.
  - Locale detection (best-guess + saved) stays here.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-language-panel.js`
  - Created: `elements/003-lithium/tests/unit/managers/login/login-language-panel.test.js`
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:**
  - Vitest with a Tabulator mock: rows render, filtering works, keyboard
    navigation calls back with the expected locale.
- **Definition of Done:**
  - [ ] Lint, build, test all clean.
  - [ ] `login.js` line count down by another ≥ 200 lines.

---

### Phase 4 — Lithium: extract `login-logs-panel.js` and `login-help-panel.js`

- **Goal:** Final two extractions to bring `login.js` cleanly under 1000
  lines.
- **Prerequisites:** Phase 1.
- **In scope:** CodeMirror logs panel init and version/help panel
  population, each in their own module.
- **Files:**
  - Created: `elements/003-lithium/src/managers/login/login-logs-panel.js`
  - Created: `elements/003-lithium/src/managers/login/login-help-panel.js`
  - Created: matching Vitest files under `tests/unit/managers/login/`.
  - Touched: `elements/003-lithium/src/managers/login/login.js`
- **Tests required:** Vitest for each extracted module.
- **Definition of Done:**
  - [ ] Lint, build, test all clean.
  - [ ] **`login.js` is now ≤ 1000 lines.** This is the gate before
        any OIDC code lands in Lithium.
  - [ ] `LITHIUM-INS.md` rule #2 satisfied for every file under
        `src/managers/login/`.

---

### Phase 5 — Hydrogen: `OIDCRelyingPartyConfig` schema + parser

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
  - Created: `src/config/config_oidc_rp.c`, `src/config/config_oidc_rp.h`
  - Created: `tests/unity/test_config_oidc_rp.c`
  - Touched: `src/config/config.c`, `src/config/config.h`,
    `src/config/config_forward.h` (forward decl), CMakeLists.
- **Tests required:**
  - `test_config_oidc_rp.c` (Unity): missing section → defaults; full
    valid section → all fields populated; bad type → error; env-var
    substitution; `cleanup` is leak-free under ASAN.
- **Definition of Done:**
  - [ ] `mkt` and `mka` clean.
  - [ ] `mkp` (cppcheck) clean for all new files.
  - [ ] `test_10_unity` includes the new test and it is green.
  - [ ] `test_11_leaks_like_a_sieve` passes with the new test enabled.
  - [ ] No regression in `test_17_startup_shutdown` with min/max
        configs.

---

### Phase 6 — Hydrogen: disabled-stub endpoints

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
  - Created: `src/api/auth/oidc_rp/oidc_rp_start.{c,h}`,
    `oidc_rp_callback.{c,h}`, `oidc_rp_handoff.{c,h}` (skeletons only).
  - Created: `src/api/auth/oidc_rp/oidc_rp_service.{c,h}` providing
    routing helpers + `oidc_rp_is_enabled()`.
  - Touched: web server router to register the three URLs.
  - Created: `tests/test_42_oidc_rp.sh` containing only the
    "disabled" test cases for now.
- **Tests required:**
  - `test_42_oidc_rp.sh`: with `Enabled=false`, all three endpoints
    return 503 with `{"error":"oidc_disabled"}`; wrong method returns
    405; URL outside the prefix returns 404.
- **Definition of Done:**
  - [ ] `mkt`, `mka`, `mkp`, `mks` all clean.
  - [ ] `test_42_oidc_rp.sh` exists, is wired into `test_00_all.sh`,
        and passes.
  - [ ] `test_40_auth.sh` still green (no regression on the
        password flow).
  - [ ] `test_99_code_size` passes (no file > 1000 LOC).

---

### Phase 7 — Hydrogen: in-memory state store

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
  - Created: `src/api/auth/oidc_rp/oidc_rp_state.{c,h}`
  - Created: `tests/unity/test_oidc_rp_state.c`
- **Tests required:**
  - Unity `test_oidc_rp_state`: put/take round-trip; double-take
    returns NULL; expiry sweeps; concurrent put/take from two threads
    via pthreads; init/shutdown leak-free under ASAN.
- **Definition of Done:**
  - [ ] `mkt`, `mka`, `mku test_oidc_rp_state` all clean.
  - [ ] ASAN test passes (`test_11_leaks_like_a_sieve`).
  - [ ] cppcheck clean (no `-` style warnings either).

---

### Phase 8 — Hydrogen: PKCE + nonce + state generators

- **Goal:** Pure crypto helpers, no networking, no HTTP.
- **Prerequisites:** Phase 5.
- **In scope:**
  - `oidc_rp_pkce.{c,h}`: `oidc_rp_make_code_verifier()` (43–128 chars,
    URL-safe base64 of 32 random bytes); `oidc_rp_make_code_challenge()`
    (BASE64URL(SHA256(verifier))); `oidc_rp_make_random_hex(size_t)`
    used for `state` and `nonce`.
  - All randomness from existing Hydrogen entropy source (whatever
    `auth_service` already uses for `jti` generation — reuse it).
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_pkce.{c,h}`
  - Created: `tests/unity/test_oidc_rp_pkce.c`
- **Tests required:**
  - Known-answer test against an RFC 7636 example:
    verifier `dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk` →
    challenge `E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM`.
  - Random-byte size correctness.
  - 1000 invocations produce all distinct values (uniqueness sanity).
- **Definition of Done:**
  - [ ] `mkt`, `mku test_oidc_rp_pkce` clean.
  - [ ] cppcheck clean.

---

### Phase 9 — Hydrogen: discovery + JWKS fetch and cache

- **Goal:** Cache the IdP's `.well-known/openid-configuration` and JWKS,
  refresh on miss / TTL / signature-failure trigger.
- **Prerequisites:** Phases 5, 8.
- **In scope:**
  - `oidc_rp_discovery.{c,h}`: HTTP GET via existing Hydrogen HTTP
    client (libcurl). Parses the JSON. Exposes:
    `oidc_rp_discovery_get(provider) → {authorization_endpoint,
    token_endpoint, userinfo_endpoint, jwks_uri, end_session_endpoint,
    issuer}`. TTL from `DiscoveryCacheSeconds`.
  - `oidc_rp_jwks.{c,h}`: GET `jwks_uri`, parse, expose
    `oidc_rp_jwks_find(kid) → JWK`. TTL from `JwksCacheSeconds`.
  - `oidc_rp_jwks_invalidate()` for the rotation-recovery flow used
    later in Phase 12.
  - **TLS verify is mandatory** (`VerifySsl`).
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_discovery.{c,h}`,
    `oidc_rp_jwks.{c,h}`
  - Created: `tests/unity/test_oidc_rp_discovery.c` (parse + TTL
    behaviour with HTTP mocked)
  - Created: `tests/lib/mock_keycloak/` with a minimal HTTP server
    serving `/.well-known/openid-configuration` and `/jwks` (this is
    the **first** appearance of the mock — keep it tiny here, expand
    in later phases).
- **Tests required:**
  - Unity unit: parse known-good JSON; missing required field ⇒ error;
    cache-hit returns same pointer; expired entry is refetched.
  - Mock Keycloak under `tests/lib/mock_keycloak/`: starts on a fixed
    port, serves canned discovery + JWKS, exits cleanly. Tested by a
    new sub-test in `test_42_oidc_rp.sh` that just curls the mock.
- **Definition of Done:**
  - [ ] `mkt`, `mka`, `mku test_oidc_rp_discovery` clean.
  - [ ] `test_42_oidc_rp.sh` runs the mock Keycloak in a setup/teardown
        block (no leftover process on test failure).
  - [ ] cppcheck + shellcheck clean.

---

### Phase 10 — Hydrogen: real `/oidc/start` redirect

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
  - Touched: `src/api/auth/oidc_rp/oidc_rp_start.{c,h}`,
    `oidc_rp_service.{c,h}` (helpers).
  - Created: `tests/unity/test_oidc_rp_start.c`
- **Tests required:**
  - Unit: helper builds the auth URL with all required query
    parameters, in canonical order, properly URL-encoded.
  - Unit: `oidc_rp_safe_return_to` rejects `//evil.com`,
    `https://evil.com`, backslashes, schemes; accepts `/foo`,
    `/foo?x=1#y`, empty.
  - Black-box `test_42_oidc_rp.sh`: with `Enabled=true` pointing at
    mock, `GET /oidc/start` returns 302; `Location` header contains
    expected query params; `state` is registered in the state store.
- **Definition of Done:**
  - [ ] All Phase 10 tests green.
  - [ ] No regression in earlier phase tests.
  - [ ] cppcheck/shellcheck clean.

---

### Phase 11 — Hydrogen: token endpoint client

- **Goal:** Pure server-to-server POST to Keycloak's `/token`,
  parse the response, expose `{ id_token, access_token, expires_in }`
  to callers. No callback wiring yet.
- **Prerequisites:** Phase 9.
- **In scope:**
  - `oidc_rp_token.{c,h}`: `oidc_rp_exchange_code(provider,
    code, redirect_uri, code_verifier, out_tokens)`.
  - HTTP Basic auth header from `client_id:client_secret` (URL-encoded
    per RFC 6749 §2.3.1) preferred; fall back to body params if
    `auth_method = "client_secret_post"`.
  - Strict TLS, redirect = error, 4xx → typed error string returned.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_token.{c,h}`
  - Created: `tests/unity/test_oidc_rp_token.c`
  - Touched: mock Keycloak gains a `/token` endpoint that accepts a
    canned `code` and returns canned tokens.
- **Tests required:**
  - Unit (with HTTP mocked at a lower level): success → struct filled;
    400 → `invalid_grant` propagated; missing `id_token` → error.
  - Black-box: a new sub-test in `test_42_oidc_rp.sh` calls a tiny
    test-only utility that exercises `oidc_rp_exchange_code` directly
    against the mock. (Or wait until Phase 14 if calling from a CLI
    is awkward; choose one path and stick to it.)
- **Definition of Done:**
  - [ ] All token-related tests green.
  - [ ] cppcheck clean; no static functions on testable code (per
        Hydrogen instructions).

---

### Phase 12 — Hydrogen: ID token validation

- **Goal:** Validate a JWT that *Keycloak* signed (different from the
  Hydrogen-signed JWTs the auth service issues). This is the highest-
  risk phase — get it right.
- **Prerequisites:** Phases 8, 9.
- **In scope:**
  - `oidc_rp_idtoken.{c,h}`: `oidc_rp_validate_id_token(provider,
    id_token, expected_nonce, out_claims)`.
  - Header parsed; `alg` must be in `AllowedAlgorithms`. **Explicitly
    reject `none`.**
  - JWK lookup by `kid` from cached JWKS (Phase 9). On miss → invalidate
    cache → refetch once → retry. On second miss → fail.
  - Signature verify (RS256 via OpenSSL/`libssl` already linked).
  - Claims checks: `iss == provider.issuer`, `aud` contains
    `provider.client_id`, `exp > now - skew`, `iat < now + skew`,
    `nbf <= now + skew` if present, `nonce == expected_nonce`.
  - Configurable clock skew (default 60s).
  - `out_claims` is a typed struct with `sub`, `email`, `email_verified`,
    `preferred_username`, `name`, `realm_access.roles[]`.
- **Files:**
  - Created: `src/api/auth/oidc_rp/oidc_rp_idtoken.{c,h}`
  - Created: `tests/unity/test_oidc_rp_idtoken.c`
- **Tests required:**
  - Unity: good token; expired; not-yet-valid; wrong issuer; wrong
    audience; nonce mismatch; `alg=none`; unknown `kid` triggers
    refetch and succeeds; tampered signature; missing required claim
    (`sub`).
  - Each test uses a fixture token signed by a test RSA keypair whose
    JWKS the test injects into the cache directly.
- **Definition of Done:**
  - [ ] All Unity ID-token tests green.
  - [ ] ASAN clean.
  - [ ] cppcheck clean.
  - [ ] Code review specifically focused on this phase
        (`/local-review-uncommitted`) before merging.

---

### Phase 13 — Hydrogen: handoff store + `/oidc/handoff` endpoint

- **Goal:** Implement the single-use, short-lived handoff exchange that
  delivers the final Hydrogen JWT to Lithium.
- **Prerequisites:** Phase 7 (the same hash-table primitive can back
  this store; either reuse the type or make a generic version both
  stores share).
- **In scope:**
  - `oidc_rp_handoff_store.{c,h}` (or merged into `oidc_rp_state.c`
    behind a typed wrapper) with `put(handoff_code, jwt, account_id,
    client_ip, expires_at)` and atomic `take(handoff_code)`.
  - `oidc_rp_handoff.{c,h}`: `POST /api/auth/oidc/handoff` accepts
    `{handoff: "..."}`, atomically takes, optionally checks
    `client_ip` if `BindHandoffToIp = true`, and returns
    `{token, expires_at, user_id, roles}` — same shape as `/auth/login`.
  - 401 on missing/expired/IP-mismatch, single envelope.
- **Files:**
  - Created/Touched: `src/api/auth/oidc_rp/oidc_rp_handoff.{c,h}`,
    `oidc_rp_handoff_store.{c,h}` (or merged into state file).
  - Created: `tests/unity/test_oidc_rp_handoff.c`
- **Tests required:**
  - Unit: put/take/expire; double-take returns 401; IP-bind enforced
    when configured.
  - Black-box `test_42_oidc_rp.sh`: a directly-injected handoff (via
    a test-only debug helper, *only available in test builds*)
    exchanges to a JWT; replay returns 401.
- **Definition of Done:**
  - [ ] All handoff tests green.
  - [ ] No test-only debug helpers leak into release builds (compile
        guard verified by `mka`).
  - [ ] cppcheck/shellcheck clean.

---

### Phase 14 — Hydrogen: `/oidc/callback` end-to-end (stub linker)

- **Goal:** Wire `/oidc/start` → Keycloak → `/oidc/callback` →
  handoff URL into a working chain. Account linking is **stubbed** to
  always return `account_id = 1` from the existing test fixtures, so
  this phase does not depend on schema work.
- **Prerequisites:** Phases 7, 9, 10, 11, 12, 13.
- **In scope:**
  - Replace the 503 stub in `oidc_rp_callback.c`.
  - Read `code` + `state` from query string; look up state (atomic
    take); call `oidc_rp_exchange_code`; call
    `oidc_rp_validate_id_token`; call stub linker
    `oidc_rp_link_stub_returns_one()`; call `verify_api_key` for
    the configured `OIDC_RP.SystemApiKey`; call `generate_jwt` and
    `store_jwt` from the existing auth service; put a handoff entry;
    302 to Lithium origin with `?oidc=1&handoff=...&return_to=...`.
  - Errors return 302 to Lithium with `?oidc_error=<typed code>`
    (never an error body — there is no UI here).
- **Files:**
  - Touched: `src/api/auth/oidc_rp/oidc_rp_callback.{c,h}`
  - Created: stub linker `oidc_rp_link_stub.{c,h}` (compiled only
    until Phase 21).
  - Touched: `tests/test_42_oidc_rp.sh` adds full happy-path and the
    common failure paths.
- **Tests required:**
  - Black-box happy path: `start` → mock authorizes → `callback` →
    handoff exchange yields a JWT → that JWT successfully calls
    `/api/conduit/auth_query`. **This is the central regression
    test for the rest of the project.**
  - Failure: `state` unknown, nonce mismatch, expired ID token, IP
    mismatch on handoff, `oidc_error` from IdP.
- **Definition of Done:**
  - [ ] Test 42 happy path passes consistently 10 times in a row.
  - [ ] All Test 42 failure-path cases pass.
  - [ ] Test 40 (password login) still green.
  - [ ] cppcheck/shellcheck clean.
  - [ ] Code review of the full chain
        (`/local-review-uncommitted`).

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
src/api/auth/oidc_rp/oidc_rp_token.c
src/api/auth/oidc_rp/oidc_rp_token.h
src/api/auth/oidc_rp/oidc_rp_idtoken.c
src/api/auth/oidc_rp/oidc_rp_idtoken.h
src/api/auth/oidc_rp/oidc_rp_link.c
src/api/auth/oidc_rp/oidc_rp_link.h
src/config/config_oidc_rp.c
src/config/config_oidc_rp.h
tests/unity/test_oidc_rp_state.c
tests/unity/test_oidc_rp_discovery.c
tests/unity/test_oidc_rp_idtoken.c
tests/unity/test_oidc_rp_link.c
tests/unity/test_oidc_rp_pkce.c
tests/test_42_oidc_rp.sh
tests/lib/mock_keycloak/server.{js|py}
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
elements/003-lithium/src/managers/login/oidc-login.js
elements/003-lithium/src/core/oidc-client.js
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

**Last updated:** 2026-05-08
**Owner:** Philement engineering
**Status:** Plan — ready for review and phase scheduling
