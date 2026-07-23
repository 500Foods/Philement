# OIDC Relying Party Endpoints

Hydrogen acts as an **OpenID Connect Relying Party (RP)** that delegates user
authentication to an external Identity Provider (IdP) such as Keycloak. After
the IdP confirms the user's identity, Hydrogen mints a **Hydrogen JWT** that
is indistinguishable from a password-login JWT — every existing API endpoint
(`/api/auth/renew`, `/api/auth/logout`, `/api/conduit/*`) keeps working
unchanged.

This document describes the three RP endpoints exposed by Hydrogen, the
configuration block that controls them, and the on-the-wire contract that
client SPAs (such as Lithium) integrate against.

> **For the implementer-facing recipe** (how a client SPA wires up the
> handoff exchange, stores the JWT, and emits an auth event), see
> [LITHIUM-KEYCLOAK.md](/docs/Li/LITHIUM-KEYCLOAK.md). For project-level
> architecture and rationale, see [OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md).

---

## Overview

### Why Hydrogen is the RP, not the SPA

Lithium expects a Hydrogen-issued JWT with very specific claims (`user_id`,
`username`, `roles`, `database`, `system_id`, `app_id`, `ip`, `tz`,
`tzoffset`, `exp`, `iat`, `jti`). Keycloak's ID token does **not** contain
these. Therefore, the OIDC flow must terminate inside Hydrogen, where we
mint a Hydrogen JWT after validating the IdP's response.

### Flow Summary

```text
Browser   →  GET  /api/auth/oidc/start?provider=…&database=…&return_to=…
Hydrogen  →  302 to IdP (state store records provider_name)
IdP       →  user logs in
IdP       →  302  /api/auth/oidc/callback?code=…&state=…
Hydrogen  →  POST /token  (back to IdP for the state-bound provider)
Hydrogen  →  validate id_token (signature + claims)
Hydrogen  →  resolve account, mint Hydrogen JWT (may include idp_provider)
Hydrogen  →  302 to client SPA with one-time `?handoff=…`
Client    →  POST /api/auth/oidc/handoff  →  receives Hydrogen JWT
```

The Keycloak `id_token` and `access_token` **never leave Hydrogen**. Only
the opaque, single-use, IP-bound `handoff` code reaches the browser, and it
is wiped from the URL on arrival.

---

## Endpoints

### `GET /api/auth/oidc/start`

Initiates the OIDC sign-in flow by redirecting the browser to the IdP's
authorization endpoint with a CSRF-protected `state`, a replay-protected
`nonce`, and a PKCE `code_challenge`.

**Authentication:** none (public).

#### Start — query parameters

| Param | Required | Description |
|---|---|---|
| `database` | optional | Database name to authenticate against. Falls back to `OIDC_RP.Database` when absent. |
| `return_to` | optional | Relative path inside the client SPA to deep-link to after sign-in. **Must** start with `/`. Validated against an allow-list. |
| `provider` | optional | Provider name from `OIDC_RP.Providers[].Name` (case-sensitive exact match). When omitted or empty, Hydrogen uses `Providers[0]`. Client SPAs (Lithium) **should always send** this so multi-provider configs stay unambiguous. |

#### Start — response

- **302 Found** with `Location: <issuer>/protocol/openid-connect/auth?...`
  on success. The redirect URL contains:
  - `response_type=code`
  - `client_id=<configured>`
  - `redirect_uri=<configured>`
  - `scope=openid profile email`
  - `state=<32-byte hex>`
  - `nonce=<32-byte hex>`
  - `code_challenge=<base64url(SHA256(code_verifier))>`
  - `code_challenge_method=S256`
- **503** `{"error":"oidc_disabled"}` when `OIDC_RP.Enabled = false`.
- **503** `{"error":"oidc_no_provider"}` when the feature is enabled but
  no providers are configured.
- **400** `{"error":"unknown_provider"}` when `provider` is non-empty and
  does not match any configured `Providers[].Name` (no silent fallback to
  `Providers[0]`).
- **400** `{"error":"invalid_return_to"}` when `return_to` fails the
  allow-list check.

#### Start — side effects

A state record is inserted into the in-memory state store with TTL
`StateTtlSeconds` (default 600, or the selected provider's
`StateTtlSeconds`). The record holds `state`, `nonce`, `code_verifier`,
`database`, `return_to`, `client_ip`, **`provider_name`**, and
`created_at`. It is removed atomically by `/oidc/callback`. The stored
`provider_name` is how `/callback` resolves the same provider that
started the flow when more than one IdP is configured.

---

### `GET /api/auth/oidc/callback`

The redirect target registered in Keycloak. Exchanges the authorization
code for tokens, validates the ID token, resolves the user, mints a
Hydrogen JWT, and redirects the browser back to the client SPA with a
one-time handoff code.

**Authentication:** none (public). The endpoint is hit by the browser as
part of the IdP redirect; it does **not** carry a Hydrogen JWT.

After a successful state lookup, the provider used for token exchange,
ID-token validation, account linking, and JWT minting is the one named in
the state record (`provider_name` from `/start`), not necessarily
`Providers[0]`.

#### Callback — query parameters

| Param | Required | Description |
|---|---|---|
| `code` | on success | Authorization code from the IdP. |
| `state` | required | The state value originally minted by `/start`. Must match the in-memory store. |
| `error` | on failure | IdP-side error code (e.g. `access_denied`). |
| `error_description` | optional | Human-readable IdP error message. |

#### Callback — response

- **302 Found** to `<client_origin>/login?oidc=1&handoff=<code>` on
  success. The handoff code is 32 bytes of hex, single-use, with TTL
  `HandoffTtlSeconds` (default 60).
- **302 Found** to `<client_origin>/login?oidc_error=<code>` on failure.
  Error codes:
  - `state_invalid` — state missing or expired
  - `idp_error` — IdP returned `error` in query
  - `token_invalid_grant` — the IdP rejected the authorization code (already used, expired, or mismatched redirect URI)
  - `token_invalid_client` — the IdP rejected the client credentials
  - `token_server_error` — the IdP token endpoint returned a 5xx
  - `id_token_invalid` — signature, issuer, audience, nonce, or expiry check failed
  - `account_not_found` — user resolved to no account (`match_email_only` with no match)
  - `email_ambiguous` — the IdP email matches more than one `accounts` row
  - `account_disabled` — account exists but `enabled = false`
  - `provisioning_blocked` — provisioning required but `ProvisionDefaults.Enabled = false`
  - `provision_disallowed_email` — provisioning is enabled but the email domain is not in `AllowedEmailDomains`
  - `email_not_verified` — `RequireEmailVerified` is true but IdP says otherwise
  - `no_api_key` — the provider has no `SystemApiKey` configured or it was rejected
  - `server_error` / `internal_error` — unexpected server-side failure (logged with detail)

#### Callback — side effects

- The state record is atomically removed (single-use).
- A Hydrogen JWT is generated via `generate_jwt()` — the same path used by
  password login. The JWT is persisted via `store_jwt()` so
  `/api/auth/renew` and `/api/auth/logout` work unchanged.
- A handoff record is inserted with `{handoff → {jwt, expires_at,
  account_id, client_ip, created_at}}`.

#### Why the handoff code, not the JWT, ends up in the URL

Putting the JWT in the URL would leak it to browser history, referer
headers, and load-balancer logs. The handoff code is opaque, single-use,
short-lived, and exchanged over an HTTPS POST to `/oidc/handoff`.

---

### `POST /api/auth/oidc/handoff`

Exchanges a one-time handoff code for the Hydrogen JWT.

**Authentication:** none (public). The handoff code itself is the
authentication token.

#### Request

```http
POST /api/auth/oidc/handoff
Content-Type: application/json
Cache-Control: no-store

{ "handoff": "<32-byte hex>" }
```

#### Handoff — response

##### Success

```http
HTTP/1.1 200 OK
Content-Type: application/json

{
  "success": true,
  "token": "<hydrogen jwt>",
  "expires_at": "2026-05-10T05:30:00Z",
  "user_id": 123,
  "username": "alice",
  "roles": ["admin", "user"]
}
```

This is **the exact same response shape** as `POST /api/auth/login`, so
client SPAs can treat both paths uniformly.

##### Failure

| Status | Body | Cause |
|---|---|---|
| 401 | `{"error":"handoff_invalid"}` | Handoff missing, expired, or already used. |
| 401 | `{"error":"ip_mismatch"}` | `BindHandoffToIp = true` and the requesting IP differs from the callback-time IP. |
| 503 | `{"error":"oidc_disabled"}` | `OIDC_RP.Enabled = false`. |
| 400 | `{"error":"invalid_request"}` | Body malformed or missing `handoff` field. |

The handoff is removed atomically on lookup, so a successful exchange and
a replay attempt both consume the record at most once.

---

### `POST /api/auth/oidc/end-session`

Performs local logout and, when the current session was established via
OIDC, returns a URL that the client can navigate to in order to sign the
user out of the upstream IdP as well.

**Authentication:** required via `Authorization: Bearer <Hydrogen JWT>`
header. The endpoint validates the JWT, optionally accepts expired tokens
(so a user can sign out even after the session has expired), and deletes
the JWT from storage. If the token's OIDC context claims (`id_token` and
`idp_provider`) are present, Hydrogen resolves the provider with
`oidc_rp_find_provider(idp_provider)` (must match a configured
`Providers[].Name`). When that provider's discovery document exposes
`end_session_endpoint`, the endpoint constructs the IdP logout URL.

**Request:**

```http
POST /api/auth/oidc/end-session HTTP/1.1
Authorization: Bearer <hydrogen_jwt>
Content-Type: application/json

{}
```

The body is reserved for future use (e.g., a requested `post_logout_redirect_uri`)
and may be empty.

**Success response:**

```http
HTTP/1.1 200 OK
Content-Type: application/json

{ "redirect_url": "https://www.500passwords.com/realms/festival/protocol/openid-connect/logout?id_token_hint=...&post_logout_redirect_uri=...&client_id=..." }
```

If the session is not OIDC-backed, or the provider does not advertise an
end-session endpoint, `redirect_url` is `null` and the client should fall
back to normal local logout.

#### End Session Failure

| Status | Body | Cause |
|---|---|---|
| 401 | `{"error":"..."}` | Missing, malformed, or invalid `Authorization` header. |
| 503 | `{"error":"oidc_disabled"}` | `OIDC_RP.Enabled = false`. |

---

## Configuration

The `OIDC_RP` block lives at the top level of `hydrogen.json`. The full
schema (with sample values) is documented in
[OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md). Below is a working production-shape
example.

```jsonc
{
  "OIDC_RP": {
    "Enabled": true,
    "Database": "Lithium",
    "Providers": [
      {
        "Name": "500passwords",
        "Label": "500 Passwords",
        "Issuer": "https://www.500passwords.com/realms/festival",
        "ClientId": "${env.HYDROGEN_OIDC_CLIENT_ID}",
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
          "Strategy": "match_email_only",
          "ProvisionDefaults": {
            "Enabled": false,
            "Authorized": false,
            "DefaultRoleNames": ["user"]
          },
          "RequireEmailVerified": true
        },
        "RoleMapping": {
          "Source": "database"
        }
      }
    ]
  }
}
```

### Field reference

| Field | Type | Default | Description |
|---|---|---|---|
| `Enabled` | bool | `false` | Master kill switch. When `false`, all three endpoints return `503 oidc_disabled`. |
| `Database` | string | none | Fallback database when `/oidc/start` does not pass `?database=`. Must match a configured `Databases.Connections.<name>`. |
| `Providers[]` | array | `[]` | One entry per IdP. Capped at `OIDC_RP_MAX_PROVIDERS = 8`. |
| `Providers[].Name` | string | required | Internal name used in logs and `?provider=`. Must be unique. |
| `Providers[].Label` | string | optional | Human-readable label (informational; displayed by the client SPA via its own config). |
| `Providers[].Issuer` | string | required | The IdP's issuer URL (no trailing slash). Discovery doc is fetched from `<Issuer>/.well-known/openid-configuration`. |
| `Providers[].ClientId` | string | required | OAuth 2.0 client ID. Use `${env.X}` for env-var substitution. |
| `Providers[].ClientSecret` | string | required | OAuth 2.0 client secret. **Always** use `${env.X}` — never a literal. Treated as `PROCESS_SENSITIVE` and redacted in logs and config dumps. |
| `Providers[].RedirectUri` | string | required | Must match exactly what is registered in the IdP's client configuration. |
| `Providers[].Scopes` | string | `"openid profile email"` | Space-separated OIDC scopes. |
| `Providers[].VerifySsl` | bool | `true` | TLS verification for HTTP calls to the IdP. **Mandatory in production.** |
| `Providers[].AllowedAlgorithms` | array | `["RS256"]` | ID-token `alg` allow-list. Empty array silently falls back to `["RS256"]` to prevent the degenerate "no algorithms allowed" state. |
| `Providers[].DiscoveryCacheSeconds` | int | `3600` | TTL for the `.well-known/openid-configuration` cache. |
| `Providers[].JwksCacheSeconds` | int | `3600` | TTL for the JWKS cache. |
| `Providers[].StateTtlSeconds` | int | `600` | TTL for the state store entry between `/start` and `/callback`. |
| `Providers[].HandoffTtlSeconds` | int | `60` | TTL for the handoff store entry between `/callback` and `/handoff`. |
| `Providers[].BindHandoffToIp` | bool | `true` | When true, `/handoff` rejects requests whose source IP differs from the `/callback` IP. Disable only if a load balancer rewrites IPs between requests. |
| `Providers[].AccountLinking.Strategy` | enum | `"match_email_then_provision"` | One of: `match_sub_only`, `match_email_only`, `match_email_then_provision`, `provision_only`. |
| `Providers[].AccountLinking.ProvisionDefaults.Enabled` | bool | `false` | Auto-create `accounts` rows for first-time OIDC users. |
| `Providers[].AccountLinking.ProvisionDefaults.Authorized` | bool | `false` | Initial `authorized` flag for provisioned accounts. |
| `Providers[].AccountLinking.ProvisionDefaults.DefaultRoleNames` | array | `[]` | Initial roles for provisioned accounts. |
| `Providers[].AccountLinking.RequireEmailVerified` | bool | `true` | Reject sign-ins where the IdP's `email_verified` claim is false. |
| `Providers[].RoleMapping.Source` | enum | `"database"` | One of: `database`, `idp_realm_roles`, `idp_client_roles`, `merge`. |

### Environment variables

| Variable | Purpose |
|---|---|
| `HYDROGEN_OIDC_RP_ENABLED` | Master kill switch (overrides `Enabled` in JSON). |
| `HYDROGEN_OIDC_CLIENT_ID` | Override `Providers[0].ClientId`. |
| `HYDROGEN_OIDC_CLIENT_SECRET` | **Required.** Never written to disk. |
| `HYDROGEN_OIDC_ISSUER` | Override `Providers[0].Issuer`. |
| `HYDROGEN_OIDC_REDIRECT_URI` | Override `Providers[0].RedirectUri`. |

---

## IdP-side configuration (Keycloak example)

The Keycloak realm and client must exist before Hydrogen can authenticate
against them. Below is the Keycloak setup that pairs with the example
config above.

### Realm

| Field | Value |
|---|---|
| Realm name | `festival` |
| Issuer URL | `https://www.500passwords.com/realms/festival` |
| Discovery doc | `https://www.500passwords.com/realms/festival/.well-known/openid-configuration` |

### Client

| Setting | Value |
|---|---|
| Client ID | `lithium` |
| Client type | Confidential |
| Standard flow | Enabled (Authorization Code) |
| Implicit flow | Disabled |
| Direct access grants | Disabled |
| Service accounts | Disabled |
| Valid redirect URIs | `https://lithium.philement.com/api/auth/oidc/callback` |
| Valid post-logout redirect URIs | `https://lithium.philement.com/` (Phase 30 only) |
| PKCE Code Challenge Method | `S256` |

### Required claims in the ID token

| Claim | Use |
|---|---|
| `sub` | Stable IdP-side user ID. Anchor for `(iss, sub)` linking. |
| `email`, `email_verified` | Used by `match_email_only` to find an existing account. |
| `preferred_username` | Display only (informational). |
| `realm_access.roles` | Optional; consumed when `RoleMapping.Source` is not `database`. |

---

## Account Linking

Three pieces of data identify an OIDC user: `iss`, `sub`, and `email`. Only
`(iss, sub)` is guaranteed stable across an account's lifetime — `email`
can change.

The lookup table `account_oidc_identities` (DB migration `1189`) associates
`(issuer, subject)` with an `account_id`. The strategy on a fresh
sign-in is governed by `AccountLinking.Strategy`:

### `match_sub_only`

Find an existing row in `account_oidc_identities` for `(iss, sub)`. If
none, fail with `account_not_found`. Most secure but highest friction —
operators must pre-link every user via DB.

### `match_email_only` *(production default for the 500passwords client)*

1. Lookup by `(iss, sub)` → if found, use that `account_id`. Update
   `last_seen_at`. Done.
2. Else if `email_verified` and the email resolves to exactly one
   `accounts` row, link by inserting an `account_oidc_identities` row,
   then proceed.
3. Else fail with `account_not_found`.

### `match_email_then_provision`

Steps 1–2 as above. Step 3: if `ProvisionDefaults.Enabled = true`, create
a new `accounts` row with `password_hash = NULL`, link it, assign default
roles, set `enabled = true, authorized = ProvisionDefaults.Authorized`.

### `provision_only`

Always create on first login. Only safe for trusted IdPs and with strict
validation in the IdP itself.

### Edge cases

- Two existing `accounts` rows with the same email → reject with
  `account_not_found` (operator must merge first).
- IdP changes a user's email → `(iss, sub)` lookup still wins; the email
  on the identity row is updated for display only.
- `accounts.enabled = false` → reject with `account_disabled` (same as
  password login).

---

## Operations

### Health check

The OIDC RP exposes its discovery-probe status via `/api/system/health`'s
`oidc_rp_status` field (Phase 28). Until Phase 28 lands, operators should
monitor the IdP's discovery endpoint independently.

### Logging

All OIDC RP log lines use `log_this(SR_AUTH, …)` with the prefix
`OIDC_RP` in the message text. The following are **never** logged:

- `code` (authorization code)
- `state`, `nonce`, `code_verifier`
- `client_secret`
- `id_token`, `access_token`
- `handoff` codes

Logged values: `iss`, `sub` (truncated), `email` (only on failed-linking
events), `client_ip`, result codes.

### Rollback

Two independent kill switches:

- Hydrogen: set `OIDC_RP.Enabled = false` (or `HYDROGEN_OIDC_RP_ENABLED=false`).
  All three endpoints return 503; no DB changes needed.
- Client SPA: empty its OIDC providers array. The button is not rendered.
  Password login continues unaffected.

If a rollback happens *after* OIDC users have been auto-provisioned, those
`accounts` rows still exist with `password_hash = NULL` and cannot log in
via password. Operators need a procedure to either delete or set passwords
on those rows.

---

## Security

| Threat | Mitigation |
|---|---|
| CSRF on `/oidc/callback` | `state` parameter, single-use, 10-min TTL, mismatched state ⇒ reject. |
| Replay of authorization code | Code is single-use at the IdP. PKCE adds defence in depth. |
| Token interception in URL | `id_token`/`access_token` never appear in any URL. Only the opaque, single-use `handoff` code does, and it is wiped from the URL on arrival. |
| Open redirect via `return_to` | Allow-listed pattern: must start with `/`, no `//`, no `\\`, no scheme. |
| `alg=none` JWT | Algorithm allow-list (`RS256` only by default). |
| JWKS cache poisoning / rotation race | Cache JWKS by `kid`. On signature failure, refresh JWKS once before declaring failure. |
| Stolen handoff code | 60-second TTL, single-use (atomic remove on lookup), bound to `client_ip` of callback when `BindHandoffToIp = true`. |
| Replay of Hydrogen JWT after OIDC logout | Same as today: the `tokens` table revocation path applies; `is_token_revoked` is checked on every request. |
| IdP outage | Password login remains unaffected. |
| Logged secrets | All sensitive fields are on a deny-list; logging helpers redact them. |
| Clock skew | Allow ±60s for `iat`/`nbf`/`exp`. |

---

## Related documents

- [OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md) — Full architectural plan and phase log.
- [LITHIUM-OIDC.md](/docs/Li/LITHIUM-OIDC.md) — Lithium-side user/operator guide.
- [LITHIUM-KEYCLOAK.md](/docs/Li/LITHIUM-KEYCLOAK.md) — Implementer reference for new client SPAs.
- [LITHIUM-JWT.md](/docs/Li/LITHIUM-JWT.md) — JWT lifecycle (the same JWT is produced by OIDC).
- [AUTH_PLAN.md](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) — Hydrogen authentication architecture.
