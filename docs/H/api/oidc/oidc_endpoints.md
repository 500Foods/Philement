# OIDC Identity Provider Endpoints

Hydrogen as an **OpenID Connect Identity Provider (IdP)** issues tokens for
first-party and trusted clients. Paths live under `/.well-known` and `/oauth/*`
— **not** under `/api/auth/oidc/*` (that tree is the OIDC **Relying Party**).

| Role | Config | Docs |
| --- | --- | --- |
| Hydrogen **IdP** (this document) | `OIDC` | Plan: [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md) |
| Hydrogen **RP** (Keycloak client) | `OIDC_RP` | [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) |

**Kill switch:** `OIDC.Enabled` defaults to `false`. When disabled, IdP routes
are not registered (clients typically see HTTP 404).

**Operator checklist:** [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md)

---

## Endpoint summary

| Method | Path | Status |
| --- | --- | --- |
| `GET` | `/.well-known/openid-configuration` | Live |
| `GET` | `/oauth/jwks` | Live |
| `GET` / `POST` | `/oauth/authorize` | Live (code + PKCE S256) |
| `POST` | `/oauth/token` | Live (`authorization_code`, `refresh_token`) |
| `GET` | `/oauth/userinfo` | Live (Bearer access JWT) |
| `POST` | `/oauth/introspect` | Live (RFC 7662) |
| `POST` | `/oauth/revoke` | Live (RFC 7009) |
| `GET` / `POST` | `/oauth/end-session` | Stub / not MVP |
| `POST` | `/oauth/register` | Stub / not MVP |

Discovery advertises only the live protocol endpoints above (plus standard
OIDC metadata). Issuer and absolute endpoint URLs are built from `OIDC.Issuer`.

---

## Discovery

### `GET /.well-known/openid-configuration`

Returns the OpenID Provider Metadata document.

**Response:** `200` `application/json`

Notable fields (MVP):

- `issuer` — must match `OIDC.Issuer` (public URL behind TLS terminator)
- `authorization_endpoint`, `token_endpoint`, `userinfo_endpoint`, `jwks_uri`
- `introspection_endpoint`, `revocation_endpoint`
- `response_types_supported`: `code`
- `grant_types_supported`: `authorization_code`, `refresh_token`
- `code_challenge_methods_supported`: `S256` only
- `id_token_signing_alg_values_supported`: `RS256`
- `subject_types_supported`: `public`

**Errors:** `404` when OIDC disabled (route not registered).

```bash
curl -sS "${ISSUER}/.well-known/openid-configuration" | jq .
```

---

## JWKS

### `GET /oauth/jwks`

Public RSA keys for verifying IdP-signed JWTs.

**Response:** `200` `application/json`

```json
{
  "keys": [
    {
      "kty": "RSA",
      "use": "sig",
      "alg": "RS256",
      "kid": "<key-id>",
      "n": "<base64url>",
      "e": "AQAB"
    }
  ]
}
```

Keys are loaded from `OIDC.Keys.StoragePath` (`signing-active.pem` +
`signing-active.kid`), or generated on first start if missing.

**Never** expose private PEMs via this endpoint.

---

## Authorization

### `GET /oauth/authorize`

Validates OAuth/OIDC parameters and returns an HTML login form when valid.

### `POST /oauth/authorize`

Authenticates the resource owner (Acuranzo account password) and redirects with
an authorization code.

#### Required query/form parameters

| Parameter | Notes |
| --- | --- |
| `client_id` | Registered / config-seeded client |
| `redirect_uri` | **Exact** match to allow-list; scheme `http` or `https` only |
| `response_type` | Must be `code` |
| `state` | **Required** (CSRF); echoed on success redirect |
| `nonce` | **Required** when scope includes `openid` (default scope is `openid`) |
| `code_challenge` | PKCE challenge |
| `code_challenge_method` | Must be `S256` (`plain` rejected) |
| `scope` | Optional; default `openid`. Use `openid offline_access` with refresh-capable clients |
| `username` / `password` | POST body only (login) |

#### Success

`302 Found` with:

```text
Location: {redirect_uri}?code={auth_code}&state={state}
```

#### Errors

- Invalid / missing params: OAuth error JSON or redirect **only if**
  `client_id` + `redirect_uri` are already validated (no open redirect).
- Bad credentials: HTML form again with generic “Invalid credentials.”
- Rate-limited IP/login: HTML “Too many attempts…” (same helpers as
  `/api/auth/login`, QueryRefs `#004`–`#007`).
- Disallowed scheme (`javascript:`, custom native schemes, etc.): rejected.

Login reuses password verify (`#012`) against `OIDC.Database` (or first DB /
`demo`).

---

## Token

### `POST /oauth/token`

`Content-Type: application/x-www-form-urlencoded`

### Grant: `authorization_code`

| Field | Notes |
| --- | --- |
| `grant_type` | `authorization_code` |
| `code` | Single-use authorization code |
| `redirect_uri` | Must match the authorize request |
| `client_id` | Required |
| `client_secret` | Confidential clients only (Basic or body) |
| `code_verifier` | PKCE verifier for S256 |

**Success `200`:**

```json
{
  "access_token": "<RS256 JWT>",
  "token_type": "Bearer",
  "expires_in": 3600,
  "id_token": "<RS256 JWT>",
  "scope": "openid offline_access",
  "refresh_token": "<opaque>"
}
```

`refresh_token` is issued when the client’s `grant_types` include
`refresh_token`.

Access JWT claims include `token_use=access`. ID token includes `nonce` when
present on the code; does **not** use `token_use=access`.

**Errors:** `400` with `{"error":"..."}`; `401` for `invalid_client`.

### Grant: `refresh_token`

| Field | Notes |
| --- | --- |
| `grant_type` | `refresh_token` |
| `refresh_token` | Opaque token |
| `client_id` | Required (+ secret if confidential) |

Rotation: new refresh returned; **reuse of an old refresh burns the family**.

---

## UserInfo

### `GET /oauth/userinfo`

```http
Authorization: Bearer <access_token>
```

**Success `200`:** at least `{"sub":"<account_id>"}`. Additional claims only if
present in the access token’s optional user payload and allowed by scope.

**Errors:** `401` with

```http
WWW-Authenticate: Bearer error="invalid_token", ...
```

Access token must verify as RS256, valid `exp`/`nbf` (60s skew),
`token_use=access`, and matching issuer.

---

## Introspection (RFC 7662)

### `POST /oauth/introspect`

Client authentication required (same as token endpoint).

| Field | Notes |
| --- | --- |
| `token` | Access JWT or refresh opaque |
| `token_type_hint` | Optional `access_token` / `refresh_token` (advisory) |

**Success `200`:** `{"active":true, ...}` or `{"active":false}` for
unknown/expired/wrong-client/revoked (no leak).

---

## Revocation (RFC 7009)

### `POST /oauth/revoke`

Client authentication required.

| Field | Notes |
| --- | --- |
| `token` | Prefer refresh tokens to end a session |
| `token_type_hint` | Optional |

**Success:** `200` empty body (including unknown tokens after successful client
auth).

**Access tokens:** no denylist — revoke is a no-op success until JWT `exp`.
Kill sessions by revoking refresh (family burn on reuse still applies).

**Errors:** `401` `invalid_client` on bad client auth.

---

## Registration / end-session

### `POST /oauth/register`

Not implemented for open registration (MVP). Prefer config seed
(`OIDC.ClientId` + `OIDC.RedirectUri` [+ optional `OIDC.ClientSecret`]) or
admin/DB client rows.

### `GET` / `POST /oauth/end-session`

RP-initiated logout — deferred (Phase 17).

---

## Security notes (MVP)

- Authorization code + **PKCE S256** only; no implicit/hybrid.
- Redirect URIs: exact string match; schemes **http** / **https** only.
- `state` required; `nonce` required with `openid`.
- OIDC JWTs are **RS256** only (`alg=none` rejected).
- Session JWTs from `/api/auth/login` (HS256) are a **separate** surface —
  do not mix with OIDC access tokens.
- Clock skew: 60 seconds (`OIDC_JWT_CLOCK_SKEW_SECONDS`).
- Do not log codes, secrets, refresh tokens, or id_tokens.

---

## Related

- [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md) — enable, keys, TLS
- [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md) — implementation plan
- [test_45_oidc_idp.md](/docs/H/tests/test_45_oidc_idp.md) — blackbox coverage
- [oidc.md](/docs/H/core/subsystems/oidc/oidc.md) — subsystem overview
- [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) — when Hydrogen is the client of Keycloak
