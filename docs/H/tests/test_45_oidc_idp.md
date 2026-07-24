# Test 45: OIDC Identity Provider

## Overview

[`test_45_oidc_idp.sh`](/elements/001-hydrogen/hydrogen/tests/test_45_oidc_idp.sh)
black-box tests Hydrogen as an OpenID Connect **Identity Provider** (the inverse
of Test 42, which drives Hydrogen as a Relying Party against mock Keycloak).

Version **2.0.0** runs the IdP suite **in parallel across all 7 database
engines** (same set as Test 40), plus a sequential OIDC-disabled gate.

## Purpose

- Feature gate: `OIDC.Enabled=false` leaves discovery closed (HTTP non-200)
- Per engine: discovery, JWKS, authorize errors, login+PKCE, token, userinfo,
  id_token claims, refresh rotation/reuse, code reuse, redirect/state/nonce
  hardening

## Configuration

| File | Port | Role |
| --- | --- | --- |
| `hydrogen_test_45_oidc_idp_disabled.json` | 5457 | OIDC disabled gate |
| `hydrogen_test_45_postgres.json` | 5450 | PostgreSQL |
| `hydrogen_test_45_mysql.json` | 5451 | MySQL |
| `hydrogen_test_45_sqlite.json` | 5452 | SQLite |
| `hydrogen_test_45_db2.json` | 5453 | DB2 |
| `hydrogen_test_45_mariadb.json` | 5454 | MariaDB |
| `hydrogen_test_45_cockroachdb.json` | 5455 | CockroachDB |
| `hydrogen_test_45_yugabytedb.json` | 5456 | YugabyteDB |

Each enabled config seeds a public client via:

- `OIDC.ClientId` = `test-idp-public`
- `OIDC.RedirectUri` = `http://127.0.0.1:5459/cb`
- `OIDC.Database` = `Acuranzo`
- Per-engine keys under `tests/artifacts/oidc_idp_keys_45_<engine>`
- Issuer matches `http://localhost:<port>`

Demo credentials: `HYDROGEN_DEMO_USER_NAME` / `HYDROGEN_DEMO_USER_PASS`.

Database connection env vars match Test 40 (ACURANZO_*, CANVAS_*, HYDROTST_*,
YUGABYTE_*).

## Helpers

[`tests/lib/oidc_idp_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_idp_helpers.sh)
— PKCE, discovery/JWKS fetch, authorize login, token exchange, JWT payload decode.

Parallel pattern mirrors [`test_40_auth.sh`](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh):
per-engine background job, result markers, sequential analysis.

## Related

- Plan: [OIDC_IDP.md](/docs/H/plans/OIDC_IDP.md) Phase 14 multi-engine
- Endpoints: [oidc_endpoints.md](/docs/H/api/oidc/oidc_endpoints.md)
- Operator: [OIDC_IDP_OPERATOR.md](/docs/H/api/oidc/OIDC_IDP_OPERATOR.md)
- RP blackbox: [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md)

## Parallelism notes

- Jobs run in background subshells (test_40 pattern); result markers analyzed
  sequentially after all engines finish.
- Helpers use **`BASHPID` + `RANDOM`** for `/tmp` paths — do **not** use only
  `$$` (parent PID is shared across background jobs and races PKCE/authorize).
- Each engine config uses a distinct `Parameters.IPADDRESS` so authorize
  rate-limit windows do not collide when engines share a login identity.
