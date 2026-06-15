# OIDC RP Helpers Library

## Overview

The [`oidc_rp_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers.sh) library provides core testing utilities for the OIDC Relying Party test suite (`test_42_oidc_rp.sh`). It contains helpers for HTTP request validation, mock IdP interaction, and endpoint testing across Phases 6-14 of the OIDC RP implementation.

## Purpose

This library handles:

- HTTP request validation with JSON response parsing
- Feature gate testing (disabled configuration)
- Method discrimination validation
- Mock Keycloak lifecycle management
- OIDC discovery document and JWKS validation
- Authorization code flow testing
- Handoff exchange testing

## Key Features

### Request Validation

The library provides `validate_oidc_request()` which issues HTTP requests and validates:

1. HTTP status code matches expected value
2. Response body is valid JSON
3. `error` field contains expected value (for error responses)

This function is used throughout the OIDC RP tests for consistent validation.

## Library Functions

### validate_oidc_request

Validates an HTTP request against expected status and error value.

**Signature**:

```bash
validate_oidc_request() {
    local url="$1"                # Full endpoint URL
    local method="$2"             # HTTP method (GET, POST)
    local expected_status="$3"    # Expected HTTP status code
    local expected_error="$4"     # Expected .error value in JSON response
    local response_file="$5"      # File to write response body
    local body="${6:-}"           # Optional request body for POST
}
```

**Returns**: 0 on match, 1 otherwise

### Endpoint Test Functions

These functions test the three OIDC RP endpoints under various conditions:

#### test_oidc_endpoint_disabled

Tests that an endpoint returns 503 `oidc_disabled` when OIDC is disabled.

**Signature**:

```bash
test_oidc_endpoint_disabled() {
    local base_url="$1"       # Server base URL
    local endpoint="$2"       # Endpoint name (start, callback, handoff)
    local method="$3"         # Correct HTTP method
    local description="$4"    # Description for output
}
```

#### test_oidc_endpoint_method_not_allowed

Tests that wrong HTTP method returns 405 `method_not_allowed`.

**Signature**:

```bash
test_oidc_endpoint_method_not_allowed() {
    local base_url="$1"
    local endpoint="$2"
    local wrong_method="$3"   # Method that should NOT be accepted
    local description="$4"
}
```

#### test_oidc_unknown_path_404

Tests that unknown paths return 404.

**Signature**:

```bash
test_oidc_unknown_path_404() {
    local base_url="$1"
    local path="$2"           # Path to test (e.g., auth/oidc/nope)
    local description="$3"
}
```

## Mock Keycloak Management

### start_mock_keycloak

Starts the mock IdP server in the background.

**Signature**:

```bash
start_mock_keycloak() {
    # Uses global MOCK_KC_PORT and MOCK_KC_SCRIPT
    # Sets MOCK_KC_PID on success
}
```

**Behavior**:

- Requires `node` to be available
- Runs `MOCK_KC_SCRIPT` (mock Keycloak) with `MOCK_KC_PORT`
- Waits up to 5 seconds for `READY <port>` in log output
- Sets `MOCK_KC_PID` for later cleanup

### stop_mock_keycloak

Stops the mock IdP server.

**Signature**:

```bash
stop_mock_keycloak() {
    # Uses global MOCK_KC_PID
}
```

**Behavior**:

- Sends TERM signal, then KILL after 2 seconds if needed
- Waits for process exit

## Mock Keycloak Sub-Tests

These functions validate the mock IdP behavior:

### test_mock_keycloak_reachable

Verifies mock `/health` endpoint returns 200 OK.

### test_mock_keycloak_discovery_doc

Validates OIDC discovery document has required fields:

- `issuer`
- `authorization_endpoint`
- `token_endpoint`
- `jwks_uri`

### test_mock_keycloak_jwks_doc

Validates JWKS document:

- At least one key entry
- Key has `kid` (key ID) field
- Key has `n` (modulus) field with RSA-length base64url value

### test_mock_keycloak_token_happy_path

Tests `/token` endpoint:

- Returns HTTP 200
- Returns `id_token` (RS256 signed JWT)
- Returns `access_token`
- Returns `token_type: "Bearer"`
- id_token has 3-segment JWS structure

### test_mock_keycloak_token_invalid_grant

Tests `/token` rejects unknown authorization codes with HTTP 400 `invalid_grant`.

### test_mock_keycloak_token_unsupported_grant_type

Tests `/token` rejects wrong grant types with HTTP 400 `unsupported_grant_type`.

### test_mock_keycloak_id_token_header_and_claims

Phase 12 sub-test validating JWT structure:

- Header: `alg=RS256`, `kid=test-key-1`
- Claims: `iss`, `sub`, `aud=lithium-web`, `exp`, `iat`, `nonce`

## Phase 10 /oidc/start Tests

### test_oidc_start_redirects_to_idp

Tests `/start` redirect behavior:

- Returns HTTP 302
- Location header points to mock IdP authorization endpoint
- Includes PKCE parameters: `response_type=code`, `client_id`, `redirect_uri`, `scope`, `state`, `nonce`, `code_challenge`, `code_challenge_method=S256`
- Includes `Cache-Control: no-store` header

### test_oidc_start_invalid_return_to_rejected

Tests that unsafe `return_to` values are rejected with HTTP 400 `invalid_return_to`.

### test_oidc_start_method_check_still_works_when_enabled

Confirms method discrimination fires before feature gate check.

## Phase 13 /oidc/handoff Tests

### inject_handoff

Injects a handoff record via debug endpoint.

**Signature**:

```bash
inject_handoff() {
    local base_url="$1"
    local jwt="$2"
    local account_id="$3"
    local username="${4:-}"
    local roles="${5:-}"
    local response_file="$6"
}
```

**Returns**: Handoff code on stdout, 0 on success

### test_oidc_handoff_happy_path

Tests successful handoff exchange:

1. Inject handoff record
2. Exchange code at `/handoff`
3. Validate envelope: `token`, `user_id`, `username`, `roles`, `success=true`

### test_oidc_handoff_replay_returns_401

Tests that consumed handoff codes cannot be replayed:

- First exchange returns 200
- Second exchange returns 401 `handoff_invalid`

### test_oidc_handoff_unknown_code_returns_401

Tests that unknown handoff codes return 401 `handoff_invalid`.

### test_oidc_handoff_missing_field_returns_401

Tests that missing `handoff` field returns 401 `handoff_invalid`.

### test_oidc_handoff_method_check_still_works_when_enabled

Confirms GET on `/handoff` returns 405 even when OIDC is enabled.

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers_callback.md](/docs/H/tests/oidc_rp_helpers_callback.md) - Phase 14 callback tests
- [oidc_rp_helpers_link.md](/docs/H/tests/oidc_rp_helpers_link.md) - Phases 18-20 linker tests
- [oidc_rp_helpers_default.md](/docs/H/tests/oidc_rp_helpers_default.md) - Phase 21 default strategy
- [oidc_rp_helpers_roles.md](/docs/H/tests/oidc_rp_helpers_roles.md) - Phase 22 role mapping
- [OIDC RP Implementation Plan](/docs/H/plans/AUTH_PLAN.md)