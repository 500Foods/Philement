# OIDC RP Callback Helpers Library

## Overview

The [`oidc_rp_helpers_callback.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_callback.sh) library provides testing utilities for Phase 14 of the OIDC RP implementation, focusing on `/callback` endpoint behavior and error handling.

## Purpose

This library tests the callback endpoint failure paths:

- Missing required parameters
- Unknown state values (CSRF protection)
- IdP error passthrough
- State replay protection
- Full end-to-end happy path

## Key Features

### Error Parameter Extraction

The library provides utilities for extracting OAuth error parameters from redirect responses:

```bash
extract_oidc_error_param() {
    local headers_file="$1"
    # Extracts oidc_error value from Location header
}
```

## Library Functions

### extract_oidc_error_param

Extracts the `oidc_error` parameter from a redirect response.

**Signature**:

```bash
extract_oidc_error_param() {
    local headers_file="$1"   # File containing curl -i output
}
```

**Returns**: The literal error string (e.g., `state_invalid`, `no_account`)

### test_oidc_callback_missing_params_redirects_state_invalid

Tests `/callback` with no query parameters.

**Expected Behavior**:

- Returns HTTP 302 redirect
- Location header contains `oidc_error=state_invalid`
- No JWT envelope is returned

This validates that the state parameter is required and enforced.

### test_oidc_callback_unknown_state_redirects_state_invalid

Tests `/callback` with unknown state value.

**Expected Behavior**:

- Returns HTTP 302 redirect
- Location header contains `oidc_error=state_invalid`

This validates CSRF protection: consumed states cannot be reused.

### test_oidc_callback_idp_error_redirects_idp_error

Tests `/callback` with IdP-supplied error parameters.

**Test Request**: `?error=access_denied&error_description=User+canceled`

**Expected Behavior**:

- Returns HTTP 302 redirect
- Location header contains `oidc_error=idp_error`
- `error_description` does NOT leak into redirect URL (security)

### test_oidc_callback_method_check_still_works_when_enabled

Confirms method discrimination for `/callback`:

- POST on a GET-only endpoint returns 405 `method_not_allowed`
- Test runs against enabled configuration

### test_oidc_callback_happy_path_end_to_end

Drives the complete `/start → /auth → /callback → /handoff` chain:

**Steps**:

1. GET `/api/auth/oidc/start` (follows redirect to mock IdP's `/auth` endpoint)
2. Mock IdP redirects to `/callback?code=test-code-ok&state=<state>`
3. Callback exchanges code with mock `/token`, validates JWT
4. Callback creates handoff record and redirects to `/login?handoff=<code>`
5. Exchange handoff code for JWT envelope

**Expected Result**:

- Envelope contains: `success=true`, `token` (JWS format), `user_id=1` (for seeded identity)
- JWT has proper three-segment structure

### test_oidc_callback_replay_returns_state_invalid

Tests state replay protection:

1. First `/start` generates state A
2. First `/callback` with state A succeeds
3. Second `/callback` with same state A returns `oidc_error=state_invalid`

This validates that states are atomically consumed and cannot be replayed.

## Shared Helper: _drive_oidc_chain

Drives the OIDC redirect chain and captures results.

**Signature**:

```bash
_drive_oidc_chain() {
    local base_url="$1"
    local headers_file="$2"
}
```

**Side Effects**:

- `OIDC_CHAIN_STATUS` - HTTP status code
- `OIDC_CHAIN_HANDOFF` - Handoff code from final redirect
- `OIDC_CHAIN_ERROR` - oidc_error parameter if present

This function is called by link strategy helpers (Phases 18-21) to drive the authentication flow.

## Test Prerequisites

### Configuration

- Uses `hydrogen_test_42_oidc_rp_full.json` for happy path
- Requires SQLite-backed demo database
- Requires seeded identity row (issuer + subject for account_id=1)

### Environment Variables

- `HYDROGEN_DEMO_API_KEY` - For any authenticated operations

### External Tools

- `jq` - Required for JWT envelope validation
- `sqlite3` - For seeding/clearing identity rows

## Error Vocabulary

The `/callback` endpoint uses these error codes (defined in `oidc_rp_callback.h`):

| Error | Condition |
|-------|-----------|
| `state_invalid` | Missing or unknown state parameter |
| `idp_error` | IdP returned error in callback parameters |
| `no_account` | Linker found no matching identity (Phase 18/19/20/21) |
| `email_ambiguous` | Multiple accounts share the email (Phase 19) |
| `provision_disallowed_email` | Email domain not in allow-list (Phase 20) |

## Integration

The main test script sources this library after `framework.sh`:

```bash
source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
source "$(dirname "${BASH_SOURCE[0]}")/lib/oidc_rp_helpers_callback.sh"
```

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers.md](/docs/H/tests/oidc_rp_helpers.md) - Core helpers and Phase 13
- [oidc_rp_helpers_link.md](/docs/H/tests/oidc_rp_helpers_link.md) - Linker strategy tests
- [OIDC RP Implementation Plan](/docs/H/plans/AUTH_PLAN.md)