# OIDC RP Link Helpers Library

## Overview

The [`oidc_rp_helpers_link.sh`](/elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_link.sh) library provides testing utilities for the account linker strategies in Phases 18-20 of the OIDC RP implementation. It handles database seeding, SQLite interaction, and black-box testing of the linking flows.

## Purpose

This library supports testing three linker strategies:

- **Phase 18: match_sub_only** - Direct subject-to-account matching
- **Phase 19: match_email_only** - Email-based account linking
- **Phase 20: provision_only** - New account provisioning

## Key Features

### SQLite Seeding

The library provides functions to seed the demo SQLite database with test data:

- Identity rows in `account_oidc_identities`
- Email contacts in `account_contacts`
- QueryRefs for linker operations (#080, #081, #082, #083, #084)

All seeding is idempotent using `INSERT OR IGNORE` and `INSERT OR REPLACE`.

## Library Functions

### Database Seeding Functions

#### seed_oidc_identity

Inserts an identity row linking issuer+subject to account_id.

**Signature**:

```bash
seed_oidc_identity() {
    local sqlite_db="$1"        # Path to SQLite database
    local account_id="$2"       # Target account ID
    local issuer="$3"           # OIDC issuer URL
    local subject="$4"          # IdP subject claim
}
```

**Creates**:

- `account_oidc_identities` table if not exists
- QueryRef #080 (lookup by issuer+subject)
- QueryRef #084 (touch/update identity)
- Identity row with validity window

#### unseed_oidc_identity

Removes seeded identity row.

**Signature**:

```bash
unseed_oidc_identity() {
    local sqlite_db="$1"
    local issuer="$2"
    local subject="$3"
}
```

#### seed_email_contact

Seeds an email address for an account.

**Signature**:

```bash
seed_email_contact() {
    local sqlite_db="$1"
    local account_id="$2"
    local email="$3"
}
```

**Creates**: Row in `account_contacts` with `contact_type_a18=0` (email type)

#### seed_ambiguous_email_contact

Seeds the same email for a second account to trigger `email_ambiguous`.

Uses account_id=2 (`demouser`) as the secondary account.

### QueryRef Seeding Functions

#### seed_oidc_queryref_seed_only

Ensures Phase 18 QueryRefs (#080, #084) and table exist.

Used by Phase 20 to set up prerequisites without seeding an identity row.

#### seed_email_queryrefs

Ensures QueryRefs #081 and #082 exist:

- #081: Link OIDC identity to account
- #082: Lookup account by email

#### seed_provision_queryrefs

Ensures QueryRef #083 exists:

- Insert new account row for provisioning

**Note**: Also relaxes `NOT NULL` constraint on `accounts.password_hash` if needed.

### Account Management Functions

#### get_max_account_id

Captures current maximum account_id for delta detection.

```bash
get_max_account_id() {
    local sqlite_db="$1"
}
```

#### delete_account

Removes a provisioned account and its identity link.

```bash
delete_account() {
    local sqlite_db="$1"
    local account_id="$2"
}
```

## Phase 18: match_sub_only Tests

### test_oidc_link_match_sub_only_hit

Tests successful subject-based linking.

**Prerequisites**:

- Identity row seeded with (issuer, subject) pointing to account_id=1
- QueryRefs #080 and #084 present

**Expected Flow**:

1. `/start` redirects to mock IdP
2. `/callback` exchanges code, finds identity via #080
3. Identity touched via #084
4. Handoff record created
5. JWT envelope returned with `user_id=1`

### test_oidc_link_match_sub_only_miss

Tests no-match scenario.

**Prerequisites**:

- No identity row exists for (issuer, subject)

**Expected Flow**:

1. `/start` → `/auth` → `/callback` chain
2. Linker fails fast on #080
3. Redirect with `oidc_error=no_account`

## Phase 19: match_email_only Tests

### Shared Helper: _drive_oidc_chain

Drives the redirect chain and captures outcomes.

**Sets**:

- `OIDC_CHAIN_HANDOFF` - Handoff code if success
- `OIDC_CHAIN_ERROR` - Error code if failure
- `OIDC_CHAIN_STATUS` - HTTP status

### test_oidc_link_match_email_only_hit

Tests email-based linking with seeded contact.

**Prerequisites**:

- No prior identity link (so #080 misses and #082/#081 fire)
- Email contact seeded for account_id=1
- QueryRefs #081, #082, #084 present

**Expected Flow**:

1. #080 lookup fails (no prior link)
2. #082 finds account by email
3. #081 creates identity link
4. JWT envelope with `user_id=1`

### test_oidc_link_match_email_only_miss

Tests email not found.

**Prerequisites**:

- No identity row
- No email contact

**Expected**: `oidc_error=no_account`

### test_oidc_link_match_email_only_ambiguous

Tests email shared by multiple accounts.

**Prerequisites**:

- Email seeded for both account_id=1 and account_id=2
- QueryRefs present

**Expected**: `oidc_error=email_ambiguous`

## Test Configuration

### Mock IdP Values

The mock Keycloak emits these values:

| Claim | Value |
|-------|-------|
| `sub` | `mock-sub-12345` |
| `email` | `mockuser@example.com` |
| `preferred_username` | `mockuser` |

### Configuration Files

| Strategy | Config File | Port |
|----------|-------------|------|
| match_sub_only | `hydrogen_test_42_oidc_rp_sub.json` | 5245 |
| match_email_only | `hydrogen_test_42_oidc_rp_email.json` | 5246 |

## Database Schema

### account_oidc_identities

```sql
identity_id   INTEGER PRIMARY KEY
account_id    INTEGER NOT NULL    -- Links to accounts.account_id
issuer        TEXT NOT NULL       -- Matches config OIDC_RP.Issuer
subject       TEXT NOT NULL       -- The IdP sub claim
email         TEXT              -- Optional email from IdP
email_verified INTEGER           -- Boolean flag
last_seen_at  TEXT NOT NULL
created_at    TEXT NOT NULL
updated_at    TEXT NOT NULL
valid_after   TEXT NOT NULL
valid_until   TEXT NOT NULL
```

### account_contacts

```sql
contact_id    INTEGER PRIMARY KEY
account_id    INTEGER NOT NULL
contact_seq   INTEGER
contact_type_a18 INTEGER      -- 0 = email
authenticate_a19 INTEGER    -- 1 = can authenticate
status_a20  INTEGER           -- 1 = active
contact     TEXT              -- The email address
```

## Related Documentation

- [test_42_oidc_rp.md](/docs/H/tests/test_42_oidc_rp.md) - Main OIDC RP test
- [oidc_rp_helpers_provision.md](/docs/H/tests/oidc_rp_helpers_provision.md) - Phase 20 provisioning
- [oidc_rp_helpers_default.md](/docs/H/tests/oidc_rp_helpers_default.md) - Phase 21 combined strategy
- [oidc_rp_helpers_roles.md](/docs/H/tests/oidc_rp_helpers_roles.md) - Phase 22 role mapping