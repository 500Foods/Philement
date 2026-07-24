# Test 45: OIDC Identity Provider

## Overview

[`test_45_oidc_idp.sh`](/elements/001-hydrogen/hydrogen/tests/test_45_oidc_idp.sh)
black-box tests Hydrogen as an OpenID Connect **Identity Provider** (the inverse
of Test 42, which drives Hydrogen as a Relying Party against mock Keycloak).

## Purpose

- Feature gate: `OIDC.Enabled=false` leaves discovery closed (HTTP non-200)
- Discovery document shape (issuer, endpoints, S256, RS256, introspect/revoke)
- JWKS with a real RSA public key
- Authorize missing params rejected
- Happy path: login form POST → authorization code → PKCE token exchange
- Userinfo with access token
- id_token claims (`iss`, `sub`, `nonce`)
- Refresh rotation and reuse rejection
- Authorization code reuse rejection
- Bad `redirect_uri` rejection
- Disallowed redirect schemes (`javascript:`)
- Missing `state` / missing `nonce` (with `openid`) rejected
- Success redirect includes matching `state`

## Configuration

| File | Port | Role |
| --- | --- | --- |
| `hydrogen_test_45_oidc_idp_disabled.json` | 5450 | OIDC disabled |
| `hydrogen_test_45_oidc_idp.json` | 5451 | OIDC enabled + SQLite demo DB |

Enabled config seeds a public client via:

- `OIDC.ClientId` = `test-idp-public`
- `OIDC.RedirectUri` = `http://127.0.0.1:5459/cb`
- `OIDC.Database` = `Acuranzo` (resource-owner login)
- Keys under `tests/artifacts/oidc_idp_keys_45`

Demo credentials: `HYDROGEN_DEMO_USER_NAME` / `HYDROGEN_DEMO_USER_PASS`.

## Helpers

[`tests/lib/oidc_idp_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_idp_helpers.sh)
— PKCE, discovery/JWKS fetch, authorize login, token exchange, JWT payload decode.

No separate mock RP process: curl drives the browser-like form POST and follows
the 302 `Location` for the code.

## Related

- Plan: [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md) Phases 14–15
- Endpoints: [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md)
- Operator: [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md)
- RP blackbox: [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md)
