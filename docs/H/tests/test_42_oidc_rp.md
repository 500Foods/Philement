# Test 42: OIDC Relying Party — End-to-End Coverage

## Overview

The [`test_42_oidc_rp.sh`](/elements/001-hydrogen/hydrogen/tests/test_42_oidc_rp.sh) script provides comprehensive black-box testing for the OIDC Relying Party endpoints in the Hydrogen Server. This test drives the OIDC RP endpoints through every gate defined in the project plan (Phases 6-22) to validate the complete authentication flow against a mock Keycloak Identity Provider.

## Purpose

This script validates the complete OIDC authentication lifecycle including:

- Feature gate behavior (disabled vs enabled)
- Method discrimination for HTTP endpoints
- Mock IdP reachability and OIDC discovery document structure
- JWKS (JSON Web Key Set) with real RSA keys for signature verification
- Authorization code flow with PKCE (Proof Key for Code Exchange)
- State nonce protection against CSRF attacks
- Token exchange and JWT validation
- Account linking strategies for resolving OIDC identities to local accounts
- Role mapping from IdP to Hydrogen roles

## Test Configuration

- **Test Name**: OIDC Relying Party
- **Test Abbreviation**: ORP
- **Test Number**: 42
- **Version**: 2.2.1

## Test Architecture

### Phases Covered

| Phase | Description | Test Focus |
|-------|-------------|------------|
| Phase 6 | Disabled feature gate | All three endpoints return 503 `oidc_disabled` when `OIDC_RP.Enabled = false` |
| Phase 6 | Method discrimination | Wrong HTTP methods return 405 `method_not_allowed` |
| Phase 9 | Mock IdP discovery/JWKS | Mock Keycloak reachability and OIDC document shape |
| Phase 10 | /oidc/start redirect | Redirect to IdP with PKCE parameters (state, nonce, code_challenge) |
| Phase 11 | Mock /token endpoint | Token exchange with authorization code |
| Phase 12 | Signed JWT id_token | RS256-signed JWT with required OIDC claims |
| Phase 13 | /oidc/handoff exchange | Handoff code exchange returns JWT envelope |
| Phase 14 | /oidc/callback handling | Error paths: missing params, unknown state, IdP errors |
| Phase 18 | match_sub_only linker | Subject-based account linking (hit and miss paths) |
| Phase 19 | match_email_only linker | Email-based account linking (hit, miss, ambiguous paths) |
| Phase 20 | provision_only linker | Account provisioning with domain allow-list (happy and blocked paths) |
| Phase 21 | match_email_then_provision | Default strategy: sub fast-path + provision path |
| Phase 22 | Role mapping | Database, idp_realm_roles, and merge role sources |

### Configuration Files

The test uses multiple configuration files to cover different strategies:

- **`hydrogen_test_42_oidc_rp.json`**: Base disabled configuration (`OIDC_RP.Enabled = false`)
- **`hydrogen_test_42_oidc_rp_enabled.json`**: Enabled configuration for endpoint testing
- **`hydrogen_test_42_oidc_rp_sub.json`**: `match_sub_only` strategy configuration
- **`hydrogen_test_42_oidc_rp_email.json`**: `match_email_only` strategy configuration
- **`hydrogen_test_42_oidc_rp_provision.json`**: `provision_only` with `AllowedEmailDomains=["example.com"]`
- **`hydrogen_test_42_oidc_rp_provision_blocked.json`**: `provision_only` with `AllowedEmailDomains=["philement.com"]`
- **`hydrogen_test_42_oidc_rp_default.json`**: Default `match_email_then_provision` strategy
- **`hydrogen_test_42_oidc_rp_roles_idp.json`**: Role mapping with `idp_realm_roles` source
- **`hydrogen_test_42_oidc_rp_roles_merge.json`**: Role mapping with `merge` source + `IdpRolePrefix`

### Mock Keycloak

The test includes a mock Keycloak server (`tests/lib/mock_keycloak/server.js`) that:

- Serves a canned OIDC discovery document at `/realms/test/.well-known/openid-configuration`
- Provides JWKS with real RSA keys at `/realms/test/protocol/openid-connect/certs`
- Signs real RS256 JWTs for the id_token at the `/token` endpoint
- Accepts only the `authorization_code` grant type
- Returns signed JWTs with claims: `iss`, `sub`, `aud`, `exp`, `iat`, `nonce`, and `preferred_username`

## Library Dependencies

The test logic is split across multiple helper libraries to stay under the 1,000-line cap:

- **`oidc_rp_helpers.sh`**: Core utilities, mock IdP helpers, Phase 10/13 tests
- **`oidc_rp_helpers_callback.sh`**: Phase 14 /callback failure path tests
- **`oidc_rp_helpers_link.sh`**: Phase 18/19/20 account linker tests
- **`oidc_rp_helpers_provision.sh`**: Phase 20 provisioning-specific helpers
- **`oidc_rp_helpers_default.sh`**: Phase 21 default strategy lifecycle
- **`oidc_rp_helpers_roles.sh`**: Phase 22 role mapping tests

## Account Linking Strategies

### match_sub_only

Attempts to match the IdP's `sub` claim directly to an existing identity in `account_oidc_identities`. Uses QueryRef #080 for lookup and #084 for updating last-seen timestamp.

### match_email_only

Attempts to match via email address in `account_contacts`. Uses QueryRef #082 for lookup and #081 to create the identity link.

### provision_only

Creates a new account when the email domain is on the `AllowedEmailDomains` allow-list. Uses QueryRef #083 for provisioning.

### match_email_then_provision (default)

Tries sub match first, then email match, then provisioning. This is the most flexible strategy for production use.

### Role Mapping Sources

- **database**: Queries `account_roles` table via QueryRef #017
- **idp_realm_roles**: Extracts roles from IdP's `realm_access.roles` claim
- **merge**: Combines database and IdP roles with optional prefix

## Required Prerequisites

- **Node.js**: For running the mock Keycloak server
- **sqlite3**: For seeding test data into the demo database
- **jq**: For JSON parsing and assertions
- **Demo SQLite database**: `tests/artifacts/database/sqlite/hydrodemo.sqlite`
- **Environment variables**:
  - `HYDROGEN_DEMO_API_KEY`: API key for Hydrogen authentication
  - `HYDROGEN_DEMO_USER_NAME`: Demo user login name (for seeded identity)
  - `HYDROGEN_DEMO_USER_PASS`: Demo user password

## Test Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/auth/oidc/start` | GET | Initiates OIDC flow, redirects to IdP with PKCE params |
| `/api/auth/oidc/callback` | GET | Receives IdP callback, exchanges code, creates handoff |
| `/api/auth/oidc/handoff` | POST | Exchanges handoff code for JWT token (SPA integration) |
| `/api/auth/oidc/_inject_handoff` | POST | Debug-only endpoint for injecting test handoff records |

## Expected Results

### Successful Test Criteria

- All Phase 6 disabled/envelope tests pass
- Phase 9 mock Keycloak discovery and JWKS validate correctly
- Phase 10 redirect includes all required PKCE parameters
- Phase 11 token endpoint returns signed JWT
- Phase 13 handoff exchange returns correct envelope
- Phase 14 error paths return appropriate `oidc_error` values
- Phase 18: seeded identity → JWT; missing identity → `no_account`
- Phase 19: email match → JWT; email miss → `no_account`; email ambiguous → `email_ambiguous`
- Phase 20: allowed domain → provisioned account; blocked domain → `provision_disallowed_email`
- Phase 21: sub fast-path and provision path both succeed
- Phase 22: role mapping returns correct roles for all sources

### Response Envelope Structure

On success, the `/handoff` endpoint returns:

```json
{
  "success": true,
  "token": "eyJ...<JWT with Hydrogen claims>",
  "user_id": 1,
  "username": "adminuser",
  "roles": "42,kc:test-role",
  "expires_at": "2026-06-14T22:00:00Z"
}
```

### Error Response Structure

On failure, endpoints return appropriate HTTP status codes with:

```json
{
  "error": "oidc_disabled | method_not_allowed | invalid_return_to | no_account | email_ambiguous | provision_disallowed_email | handoff_invalid | state_invalid | idp_error"
}
```

## Diagnostic Output

- **Server logs**: Located in `build/tests/logs/` with timestamps
- **Metrics log**: Memory and request metrics when running under ASAN
- **Response files**: JSON responses saved for each endpoint test
- **Mock Keycloak log**: Output from the mock IdP server

## Integration

This test is part of the Hydrogen test suite and runs automatically with:

```bash
# Run this specific test
./tests/test_42_oidc_rp.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [OIDC RP Implementation Plan](/docs/H/plans/complete/AUTH_PLAN_COMPLETE.md) - Detailed OIDC RP specification
- [Conduit Service](/docs/H/plans/complete/CONDUIT_COMPLETE.md) - Conduit endpoints used by identity queries
- [Database Architecture](/docs/H/core/subsystems/database/database.md) - QueryRef system
- [Test Framework Overview](/docs/H/tests/TESTING.md)
- [Authentication Tests](/docs/H/tests/test_40_auth.md) - Related auth endpoint testing