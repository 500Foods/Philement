# OIDC RP Default Strategy Helpers Library

## Overview

The [`oidc_rp_helpers_default.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_default.sh) library provides testing utilities for the `match_email_then_provision` linker strategy (Phase 21) of the OIDC RP implementation.

## Purpose

This library tests the default strategy which combines all three linker approaches:

1. **Sub fast-path**: Direct subject match (Phase 18 behavior)
2. **Email link**: Match via email, then create identity (Phase 19 behavior)
3. **Provision**: Create new account if email domain allowed (Phase 20 behavior)

## Key Features

- Tests all three sub-paths with single configuration
- Verifies linker priority order (sub → email → provision)
- Tracks account provisioning for cleanup

## Library Functions

### test_oidc_link_default_sub_hit

Tests sub fast-path in default strategy.

**Prerequisites**:

- Identity row seeded with (issuer, subject) → account_id=1
- QueryRefs #080, #084 present

**Expected Flow**:

1. `/start` → `/auth` → `/callback`
2. #080 finds existing identity immediately
3. No email lookup (#082) needed
4. No provisioning (#083) needed
5. JWT envelope with `user_id=1`

**Verification**:

- Sub fast-path fires without side effects
- Cleanup removes seeded identity row

### test_oidc_link_default_provision

Tests provision path in default strategy.

**Prerequisites**:

- No prior identity row
- No email contact
- QueryRefs #080, #081, #082, #083, #084 present
- `AllowedEmailDomains` includes `example.com`

**Expected Flow**:

1. #080 (sub lookup) fails
2. #082 (email lookup) fails (no contact seeded)
3. Domain validation passes
4. #083 provisions new account
5. #081 links to new account
6. JWT envelope with `user_id = max+1`

**Verification**:

- New account_id is exactly max+1
- Cleanup removes both account and identity

### run_phase21_default_tests

Manages full server lifecycle for default strategy tests.

**Signature**:

```bash
run_phase21_default_tests() {
    local config_path="$1"         # Default config path
    local sqlite_db="$2"           # Demo SQLite path
    local mock_issuer="$3"         # Mock IdP issuer URL
    local mock_subject="$4"        # Mock IdP subject
    local mock_email="$5"          # Mock IdP email
    local hydrogen_bin="$6"        # Binary path
    local server_log="$7"          # Log file path
    local results_dir="$8"         # Results directory
}
```

## Strategy Priority

The linker attempts strategies in order:

```strategy
1. QueryRef #080: Lookup by (issuer, subject)
   ├─ Hit → JWT (fast-path, no DB writes)
   └─ Miss → Try next strategy

2. QueryRef #082: Lookup account by email
   ├─ Hit → QueryRef #081: Link identity
   │            └─ JWT
   ├─ Multiple hits → email_ambiguous error
   └─ Miss → Try next strategy

3. Provision (if domain allowed)
   ├─ Domain check → QueryRef #083: Create account
   ├─ QueryRef #081: Link identity
   └─ JWT
   └─ Domain blocked → provision_disallowed_email error
```

## Configuration

### Default Strategy Config

```json
{
  "OIDC_RP": {
    "Enabled": true,
    "Issuer": "http://localhost:7042/realms/test",
    "ClientId": "lithium-web",
    "Strategy": "match_email_then_provision",
    "AllowedEmailDomains": ["example.com"],
    "RequireEmailVerified": true
  }
}
```

## Test Flow

### Sub-Test Order

1. **Sub Hit**: Pre-seed identity → verify fast-path
2. **Provision**: No seed → verify account creation

Both tests run against the same server instance:

1. Validate configuration file
2. Seed QueryRefs
3. Start Hydrogen server
4. Wait for QTC bootstrap (`Migration completed` or `Migration Current:`)
5. Run sub-tests
6. Shutdown server

## Cleanup

Both sub-tests clean up after execution:

| Sub-test | Cleanup Actions |
|----------|-----------------|
| sub_hit | Delete seeded identity row |
| provision | Delete provisioned account + identity row |

Cleanup runs regardless of test outcome to ensure clean state for subsequent runs.

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers_link.md](/docs/H/tests/oidc_rp_helpers_link.md) - Linker infrastructure
- [oidc_rp_helpers_provision.md](/docs/H/tests/oidc_rp_helpers_provision.md) - Provisioning logic
- [oidc_rp_helpers_roles.md](/docs/H/tests/oidc_rp_helpers_roles.md) - Role mapping