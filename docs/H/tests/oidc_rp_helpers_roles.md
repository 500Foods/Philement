# OIDC RP Role Mapping Helpers Library

## Overview

The [`oidc_rp_helpers_roles.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_roles.sh) library provides testing utilities for Phase 22 of the OIDC RP implementation, validating role mapping from the Identity Provider to Hydrogen accounts.

## Purpose

This library tests three role mapping sources:

1. **database** - Roles from local `account_roles` table
2. **idp_realm_roles** - Roles from IdP's `realm_access.roles` claim
3. **merge** - Combined database and IdP roles with prefix

## Key Features

- Tests all three role sources end-to-end
- Verifies QueryRef #017 (Get User Roles) integration
- Validates `IdpRolePrefix` functionality
- Uses same OIDC chain infrastructure as other phases

## Library Functions

### Database Seeding

#### seed_roles_queryref

Ensures QueryRef #017 exists in the demo SQLite database.

**Signature**:

```bash
seed_roles_queryref() {
    local sqlite_db="$1"
}
```

**QueryRef #017**: Lookup role IDs for an account:

```sql
SELECT role_id FROM account_roles 
WHERE account_id = :ACCOUNTID 
AND ((valid_after IS NULL) OR (valid_after < datetime('now')))
AND ((valid_until IS NULL) OR (valid_until > datetime('now')))
```

#### seed_role_row

Seeds an account_roles row for testing database source.

**Signature**:

```bash
seed_role_row() {
    local sqlite_db="$1"
}
```

**Creates**: Row linking account_id=1 to role_id=42

#### unseed_role_row

Removes the test role row.

**Signature**:

```bash
unseed_role_row() {
    local sqlite_db="$1"
}
```

### test_oidc_roles_database_source

Tests `RoleMapping.Source = "database"`.

**Expected Flow**:

1. Pre-seed identity row (sub → account_id=1)
2. Seed role row (account_id=1 → role_id=42)
3. Drive OIDC chain
4. Verify `roles` field contains "42"

**Result**: `roles="42"` in handoff envelope

### test_oidc_roles_idp_realm_source

Tests `RoleMapping.Source = "idp_realm_roles"`.

**Configuration**: `RoleMapping.Source = "idp_realm_roles"`

**Mock IdP Response**: JWT with claims:

```json
{
  "realm_access": {
    "roles": ["test-role"]
  }
}
```

**Expected Result**: `roles="test-role"` in handoff envelope

### test_oidc_roles_merge_source

Tests `RoleMapping.Source = "merge"`.

**Configuration**:

```json
{
  "OIDC_RP": {
    "RoleMapping": {
      "Source": "merge"
    },
    "IdpRolePrefix": "kc:"
  }
}
```

**Expected Flow**:

1. Database has no role rows for account_id=1
2. IdP emits `realm_access.roles = ["test-role"]`
3. Merge combines: DB roles (empty) + IdP roles ("kc:test-role")
4. Result: `roles="kc:test-role"`

## Role Sources Comparison

| Source | Location | QueryRef | IdP Claim | Notes |
|--------|----------|----------|-----------|-------|
| database | Local SQLite | #017 | N/A | Role IDs from account_roles table |
| idp_realm_roles | Keycloak | N/A | `realm_access.roles` | Realm-level role assignments |
| idp_client_roles | Keycloak | N/A | `resource_access.{client}.roles` | Client-specific roles (planned) |
| merge | Both | #017 | Both | Comma-separated concatenation |

## Configuration Files

| Test | Config File | Source | Port |
|------|-------------|--------|------|
| database | `hydrogen_test_42_oidc_rp_default.json` | database | 5249 |
| idp_realm_roles | `hydrogen_test_42_oidc_rp_roles_idp.json` | idp_realm_roles | 5250 |
| merge | `hydrogen_test_42_oidc_rp_roles_merge.json` | merge | 5251 |

## IdpRolePrefix

When `IdpRolePrefix` is set, IdP roles are prefixed with this value:

- `IdpRolePrefix: ""` → `roles="test-role"`
- `IdpRolePrefix: "kc:"` → `roles="kc:test-role"`
- Prefix distinguishes IdP roles from database roles in merged output

## Handoff Envelope with Roles

The JWT envelope includes roles in the response:

```json
{
  "success": true,
  "token": "eyJ...<JWT>",
  "user_id": 1,
  "username": "adminuser",
  "roles": "42,kc:test-role",
  "expires_at": "2026-06-14T22:00:00Z"
}
```

The `roles` field format is implementation-dependent but typically comma-separated role identifiers.

## run_phase22_roles_tests

Manages server lifecycle for all three role source tests.

**Signature**:

```bash
run_phase22_roles_tests() {
    local default_base_url_unused="$1"  # Kept for interface symmetry
    local config_path_idp="$2"          # idp_realm_roles config
    local config_path_merge="$3"        # merge config
    local sqlite_db="$4"
    local mock_issuer="$5"
    local mock_subject="$6"
    local hydrogen_bin="$7"
    local server_log="$8"
    local results_dir="$9"
}
```

**Flow**:

1. Start default-config server (port 5249) for database source test
2. Run `test_oidc_roles_database_source`
3. Shutdown server
4. Start idp-realm-config server (port 5250) for IdP realm test
5. Run `test_oidc_roles_idp_realm_source`
6. Shutdown server
7. Start merge-config server (port 5251) for merge test
8. Run `test_oidc_roles_merge_source`
9. Shutdown server

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers_link.md](/docs/H/tests/oidc_rp_helpers_link.md) - Linker infrastructure
- [OIDC RP Implementation Plan](/docs/H/plans/AUTH_PLAN.md)