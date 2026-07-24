# OIDC Identity Provider — Operator Runbook

How to enable and operate Hydrogen as an OpenID Connect **Identity Provider**.
For request/response detail see [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md).
For implementation history see [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md).

---

## IdP vs RP (pick one product path)

| Need | Use | Config |
| --- | --- | --- |
| External org SSO (Keycloak, etc.); Hydrogen apps accept IdP logins | Hydrogen as **RP** | `OIDC_RP` — [oidc_rp.md](/docs/H/api/auth/oidc_rp.md), [KEYCLOAK_PLAN.md](/docs/H/plans/KEYCLOAK_PLAN.md) |
| Hydrogen **issues** OIDC tokens for first-party / trusted clients | Hydrogen as **IdP** | `OIDC` — this runbook |

Both stacks can coexist in one process, but they are separate code and config.
Do not put IdP clients under `/api/auth/oidc/*`.

---

## Kill switch

1. Set `"OIDC": { "Enabled": false }` (default) or omit enabling IdP.
2. Restart Hydrogen.
3. Confirm `GET /.well-known/openid-configuration` is **not** 200 (typically 404).

No partial “half-open” mode: when disabled, IdP prefixes are not registered.

---

## Enable checklist

1. **Public issuer URL** — set `OIDC.Issuer` to the URL clients and JWKS consumers
   will use (e.g. `https://auth.example.com`). Must match what appears in JWT
   `iss` and discovery. Behind a reverse proxy, this is the **external** origin,
   not `http://127.0.0.1`.
2. **TLS termination** — terminate TLS at the load balancer/proxy. Hydrogen IdP
   itself may listen HTTP internally; discovery and tokens still advertise
   `OIDC.Issuer` (https). Ensure clients never mix http/https issuers.
3. **Keys directory** — set `OIDC.Keys.StoragePath` to a writable directory owned
   by the Hydrogen process. On first start, RSA signing material is created
   (`signing-active.pem`, `signing-active.kid`). Restrict permissions
   (`chmod 700` dir, `600` PEMs). Back up private keys offline.
4. **Disable key encryption** for file-backed keys unless you have configured
   encryption keys (`EncryptionEnabled: false` is the practical default).
5. **Accounts database** — set `OIDC.Database` to the connection name that holds
   Acuranzo `accounts` (password login). Resource owners are existing accounts,
   not a separate OIDC user table.
6. **Seed a client** (MVP):
   - `OIDC.ClientId` — public or confidential client id
   - `OIDC.RedirectUri` — exact callback URL (`http`/`https` only)
   - `OIDC.ClientSecret` — optional; if set, client is confidential
   - Grants seeded: `authorization_code refresh_token`
7. **Token lifetimes** — tune `OIDC.Tokens.AccessTokenLifetime` (keep short if
   relying on refresh revoke without access denylist), refresh, id_token.
8. **Signing** — `SigningAlg` must remain `RS256` for MVP.
9. Restart and verify:
   - Discovery 200 and `issuer` correct
   - JWKS has `n`/`e`/`kid`
   - Authorize → login → token with a test public client (see Test 45 patterns)

Example fragment:

```json
"OIDC": {
    "Enabled": true,
    "Issuer": "https://auth.example.com",
    "ClientId": "my-spa",
    "RedirectUri": "https://app.example.com/cb",
    "Database": "Acuranzo",
    "Keys": {
        "StoragePath": "/var/lib/hydrogen/oidc",
        "EncryptionEnabled": false,
        "RotationIntervalDays": 90
    },
    "Tokens": {
        "AccessTokenLifetime": 3600,
        "RefreshTokenLifetime": 86400,
        "IdTokenLifetime": 3600,
        "SigningAlg": "RS256"
    }
}
```

Also see
[`hydrogen_oidc_idp_example.json`](/elements/001-hydrogen/hydrogen/examples/configs/hydrogen_oidc_idp_example.json).

---

## Client integration (minimal)

1. Discover: `GET {Issuer}/.well-known/openid-configuration`
2. PKCE S256: generate `code_verifier` / `code_challenge`
3. Browser: `GET /oauth/authorize?...&state=...&nonce=...&code_challenge=...&code_challenge_method=S256`
4. User signs in on Hydrogen HTML form
5. App receives `redirect_uri?code=...&state=...` — verify `state`
6. `POST /oauth/token` with `grant_type=authorization_code`, `code`,
   `redirect_uri`, `client_id`, `code_verifier`
7. Validate `id_token` with JWKS (`iss`, `aud`/`client_id`, `nonce`, `exp`)
8. Call APIs with `Authorization: Bearer` **access_token** (or exchange for
   app-specific session if your product still uses HS256 session JWTs)

---

## Operations

| Topic | Guidance |
| --- | --- |
| Rotate signing keys | `oidc_rotate_keys` path exists; dual-key JWKS during rotation. Plan a maintenance window; keep old key until tokens expire |
| Revoke user session | Revoke **refresh** tokens (`/oauth/revoke`). Access JWTs remain valid until `exp` (no denylist) |
| Multi-process / HA | Auth codes and refresh stores are **in-memory** in current MVP. Use a single process or wire DB-backed stores before horizontal scale |
| Secrets | Never commit client secrets, PEMs, or demo passwords. Use env substitution in JSON configs |
| Logs | Codes, refresh tokens, id_tokens, and passwords must not appear in normal logs |
| Monitoring | Watch launch readiness (issuer, keys path, token lifetimes); discovery/JWKS latency |

---

## Security policy (enforced)

- Redirect: exact match; **http/https only** (no `javascript:`, no custom schemes until configured)
- `state` required; `nonce` required with `openid`
- PKCE **S256 only**
- Error redirects only to **validated** client redirect URIs
- Authorize login rate-limited like password login
- Clock skew ±60s on access-token `exp`/`nbf`

---

## Verification

- Blackbox: [test_45_oidc_idp.md](/docs/H/tests/test_45_oidc_idp.md)
- Unity: `tests/unity/src/oidc/` and `tests/unity/src/api/oidc/`
- After config change: restart, hit discovery + JWKS, one full code flow

---

## Related plans

- [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md) — IdP implementation
- [KEYCLOAK_PLAN.md](/docs/H/plans/KEYCLOAK_PLAN.md) — external IdP + Hydrogen RP
- [OIDC-PLAN.md](/docs/H/plans/OIDC-PLAN.md) — historical RP phase log
