# OIDC RP Provision Helpers Library

## Overview

The [`oidc_rp_helpers_provision.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_provision.sh) library provides testing utilities for the `provision_only` linker strategy (Phase 20) of the OIDC RP implementation.

## Purpose

This library tests:

- Account provisioning with allowed email domain
- Provisioning rejection when domain is blocked
- Database seeding for provisioning flow

## Key Features

- Idempotent database seeding using `INSERT OR IGNORE`
- Cleanup verification to prevent test pollution
- Domain allow-list validation testing

## Library Functions

### test_oidc_link_provision_only_happy

Tests successful account provisioning.

**Prerequisites**:

- Configuration with `AllowedEmailDomains=["example.com"]`
- Mock IdP emits `mockuser@example.com`
- QueryRefs #080, #083, #084 present

**Expected Flow**:

1. #080 (sub lookup) fails - no prior identity
2. Domain `example.com` is on allow-list
3. #083 provisions new `accounts` row
4. #081 (link) creates identity row
5. JWT envelope with new `user_id` (max+1)

**Verification**:

- Captures pre/post `MAX(account_id)`
- Asserts exactly one new account row created
- Cleanup deletes provisioned account

### test_oidc_link_provision_only_blocked

Tests domain rejection.

**Prerequisites**:

- Configuration with `AllowedEmailDomains=["philement.com"]`
- Mock IdP emits `mockuser@example.com` (blocked domain)
- QueryRefs present

**Expected Flow**:

1. #080 fails
2. Domain `example.com` not on allow-list
3. Redirect with `oidc_error=provision_disallowed_email`
4. NO account row created

**Verification**:

- Pre/post `MAX(account_id)` must match
- Error code must be `provision_disallowed_email`

### seed_oidc_queryref_seed_only

Ensures Phase 18 QueryRefs (#080 lookup, #084 touch) and the `account_oidc_identities` table exist, without seeding an identity row.

**Implementation**:

- Calls `seed_oidc_identity` with sentinel values
- Immediately deletes the sentinel row
- Side effects: table + queries created, sentinel removed

This provides infrastructure for Phase 20 tests without polluting identity space.

## Configuration

### Provision-Only Config

```json
{
  "OIDC_RP": {
    "Enabled": true,
    "Issuer": "http://localhost:7042/realms/test",
    "ClientId": "lithium-web",
    "Strategy": "provision_only",
    "AllowedEmailDomains": ["example.com"]
  }
}
```

### QueryRef #083 - Provision Account

SQL to insert new account row:

```sql
INSERT INTO accounts (account_id, name, first_name, last_name, password_hash, 
                     status_a16, iana_timezone_a17, ...)
WITH next_account_id AS (
    SELECT COALESCE(MAX(account_id), 0) + 1
)
SELECT next_account_id, :USERNAME, :FIRST_NAME, :LAST_NAME, NULL,
       1, 1, ...
RETURNING account_id
```

Note: `password_hash` is NULL (social login doesn't have password).

## Test Values

| Field | Value |
|-------|-------|
| Mock email | `mockuser@example.com` |
| Mock issuer | `http://localhost:7042/realms/test` |
| Mock subject | `mock-sub-12345` |
| Blocked domain config | `AllowedEmailDomains=["philement.com"]` |

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers_link.md](/docs/H/tests/oidc_rp_helpers_link.md) - Linker infrastructure
- [oidc_rp_helpers_default.md](/docs/H/tests/oidc_rp_helpers_default.md) - Phase 21 combined strategy