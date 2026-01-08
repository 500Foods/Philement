# AUTH ENDPOINTS - ACURANZO DATABASE INTEGRATION

## Overview

This document details the Acuranzo database integration requirements for the Hydrogen authentication endpoints. It identifies existing database structures, required additional queries/migrations, and integration patterns.

## Existing Acuranzo Database Structure

### Relevant Tables

Based on the Acuranzo migrations, the following tables are relevant for authentication:

- **accounts** (Migration 1005) - User account storage
- **account_access** (Migration 1002) - Account access control
- **account_roles** (Migration 1004) - User role assignments
- **licenses** (Migration 1012) - API key and system licensing
- **lists** (Migration 1013) - IP whitelist/blacklist storage
- **tokens** (Migration 1021) - JWT token storage
- **queries** (Migration 1000) - Query reference system

### Existing QueryRefs for Authentication

The following QueryRefs are already implemented in Acuranzo (from migrations 1092-1147):

| QueryRef | Migration | Description | Status |
| ---------- | ----------- | ------------- | -------- |
| #001 | 1092 | Get System Information | ✅ Available |
| #002 | 1093 | Get IP Whitelist | ✅ Available |
| #003 | 1094 | Get IP Blacklist | ✅ Available |
| #004 | 1095 | Log Login Attempt | ✅ Available |
| #005 | 1096 | Get Login Attempt Count | ✅ Available |
| #006 | 1097 | Mark Repeated Login Failures | ✅ Available |
| #007 | 1098 | Block IP Address Temporarily | ✅ Available |
| #008 | 1099 | Get Account ID | ✅ Available |
| #009 | 1100 | Check Login Authorization | ✅ Available |
| #010 | 1101 | Check Account Authorization | ✅ Available |
| #011 | 1102 | Get Login E-Mail | ✅ Available |
| #012 | 1103 | Check Password | ✅ Available |
| #013 | 1104 | Store JWT | ✅ Available |
| #014 | 1105 | Log the Login | ✅ Available |
| #015 | 1106 | Cleanup Login Records | ✅ Available |
| #016 | 1107 | Log Endpoint Access | ✅ Available |
| #017 | 1108 | Get User Roles | ✅ Available |
| #018 | 1109 | Validate JWT | ✅ Available |
| #019 | 1110 | Delete JWT | ✅ Available |
| #020 | 1111 | Delete Account JWTs | ✅ Available |

## Required Additional Queries

### New QueryRefs Needed for Registration

The authentication system requires 3 additional QueryRefs for user registration:

| QueryRef | Purpose | Required Tables | Status |
| ---------- | --------- | ----------------- | -------- |
| #050 | Check username/email availability | accounts | ❌ Needs Implementation |
| #051 | Create new account | accounts | ❌ Needs Implementation |
| #052 | Hash and store password | accounts | ❌ Needs Implementation |

### QueryRef #050 - Check Username/Email Availability

**Purpose**: Verify if a username or email is already registered

**SQL Template**:

```sql
SELECT
    COUNT(*) as username_count,
    (SELECT COUNT(*) FROM accounts WHERE email = ?) as email_count
FROM accounts
WHERE username = ?
```

**Parameters**:

- `username` (string) - Username to check
- `email` (string) - Email to check

**Expected Result**:

```json
{
  "username_count": 0,
  "email_count": 0
}
```

### QueryRef #051 - Create New Account

**Purpose**: Insert a new user account into the database

**SQL Template**:

```sql
INSERT INTO accounts
    (username, email, password_hash, full_name, created_at, updated_at, enabled, authorized)
VALUES
    (?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, 1, 1)
RETURNING account_id
```

**Parameters**:

- `username` (string) - New username
- `email` (string) - User email address
- `password_hash` (string) - Hashed password
- `full_name` (string, optional) - User's full name

**Expected Result**:

```json
{
  "account_id": 123
}
```

### QueryRef #052 - Hash and Store Password

**Purpose**: Update password hash for an existing account

**SQL Template**:

```sql
UPDATE accounts
SET password_hash = ?, updated_at = CURRENT_TIMESTAMP
WHERE account_id = ?
```

**Parameters**:

- `password_hash` (string) - New password hash
- `account_id` (integer) - Account ID

**Expected Result**:

```json
{
  "rows_affected": 1
}
```

## Database Schema Requirements

### Accounts Table Structure

The `accounts` table (Migration 1005) should have the following structure:

```sql
CREATE TABLE accounts (
    account_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    full_name VARCHAR(255),
    created_at TIMESTAMP NOT NULL,
    updated_at TIMESTAMP NOT NULL,
    enabled BOOLEAN DEFAULT 1,
    authorized BOOLEAN DEFAULT 1,
    last_login TIMESTAMP,
    failed_attempts INTEGER DEFAULT 0,
    account_locked BOOLEAN DEFAULT 0,
    lock_reason VARCHAR(255),
    timezone VARCHAR(50)
);
```

### Required Indexes

```sql
-- For authentication performance
CREATE INDEX idx_accounts_username ON accounts(username);
CREATE INDEX idx_accounts_email ON accounts(email);
CREATE INDEX idx_accounts_enabled ON accounts(enabled);
CREATE INDEX idx_accounts_authorized ON accounts(authorized);

-- For JWT token management
CREATE INDEX idx_tokens_account ON tokens(account_id);
CREATE INDEX idx_tokens_hash ON tokens(token_hash);
CREATE INDEX idx_tokens_expires ON tokens(expires_at);
```

## Migration Implementation Plan

### Step 1: Create New QueryRef Migrations

Create Lua migration files for the 3 new QueryRefs:

1. **acuranzo_1141.lua** - QueryRef #050 - Check username/email availability
2. **acuranzo_1142.lua** - QueryRef #051 - Create new account
3. **acuranzo_1143.lua** - QueryRef #052 - Hash and store password

### Step 2: Update Migration Index

Add the new migrations to the README.md migration table and update the totals.

### Step 3: Database Schema Validation

Ensure the accounts table has all required columns for authentication:

- Verify password_hash column exists and is large enough (255 chars)
- Confirm email column has UNIQUE constraint
- Check username column has UNIQUE constraint
- Validate all required indexes are present

### Step 4: Test Data Setup

Create test data migrations for development/testing:

```lua
-- Migration 1151: Create test accounts
INSERT INTO accounts (username, email, password_hash, full_name, enabled, authorized) 
VALUES 
    ('testuser', 'test@example.com', 'hashed_password_1', 'Test User', 1, 1),
    ('adminuser', 'admin@example.com', 'hashed_password_2', 'Admin User', 1, 1);

-- Migration 1152: Create test API keys
INSERT INTO licenses (api_key, system_id, app_id, license_expiry, features) 
VALUES 
    ('test_api_key_1', 1, 1, '2026-12-31', '{"auth": true}'),
    ('test_api_key_2', 1, 1, '2026-12-31', '{"auth": true}');
```

## Integration with Hydrogen Query System

### Query Execution Pattern

```c
// Example: Using QueryRef #008 to get account info
query_result_t result = execute_query(8, json_object({
    "login_id": "testuser"
}));

if (result.row_count > 0) {
    account_info_t* account = parse_account_result(result.rows[0]);
    // Process account data
}
```

### Error Handling

```c
// Handle database query errors
query_result_t result = execute_query(query_ref, params);
if (result.error) {
    log_this("AUTH", "Database error in QueryRef #%d: %s", query_ref, result.error_message);
    return create_error_response(HTTP_500_INTERNAL_SERVER_ERROR, "DATABASE_ERROR");
}
```

## Database Performance Considerations

### Query Optimization

1. **Login Flow Optimization**:
   - Use prepared statements for all authentication queries
   - Cache frequently accessed data (API keys, whitelists)
   - Implement connection pooling via DQM

2. **Registration Flow Optimization**:
   - Combine username/email availability check into single query
   - Use transactions for atomic account creation
   - Batch insert default roles and permissions

### Caching Strategy

```c
// Cache API key validation results
typedef struct {
    char* api_key;
    system_info_t system_info;
    time_t cached_at;
    bool valid;
} api_key_cache_entry_t;

// Cache whitelist/blacklist results
typedef struct {
    char* ip_address;
    bool is_whitelisted;
    bool is_blacklisted;
    time_t cached_until;
} ip_cache_entry_t;
```

## Security Considerations

### SQL Injection Protection

All queries use parameterized statements through the DQM system:

```sql
-- Safe parameterized query example
SELECT * FROM accounts WHERE username = ? AND password_hash = ?
```

### Data Sanitization

```c
// Input validation before database operations
bool is_valid_username(const char* username) {
    if (!username || strlen(username) < 3 || strlen(username) > 50) 
        return false;
    return is_alphanumeric_underscore_hyphen(username);
}
```

## Testing Database Integration

### Unit Test Requirements

1. **QueryRef Validation Tests**:
   - Test each QueryRef with valid/invalid parameters
   - Verify error handling for malformed queries
   - Test parameter binding and type safety

2. **Transaction Tests**:
   - Test atomic operations (registration flow)
   - Verify rollback on failure
   - Test concurrent access scenarios

3. **Performance Tests**:
   - Benchmark query execution times
   - Test under load (100+ concurrent logins)
   - Measure cache hit/miss ratios

### Integration Test Requirements

```bash
# Test database connectivity
./test_40_auth.sh --database-test

# Test query execution
./test_40_auth.sh --query-test

# Test transaction handling
./test_40_auth.sh --transaction-test
```

## Deployment Considerations

### Database Migration Order

1. Apply base Acuranzo migrations (1000-1140)
2. Apply new auth-specific migrations (1141-1143)
3. Apply test data migrations (1144-1145) for development
4. Update Hydrogen payload with new queries
5. Restart Hydrogen services

### Rollback Plan

```bash
# Rollback procedure for database issues
./helium_update.sh --rollback-to 1140
./payload_update.sh --reload
systemctl restart hydrogen
```

## Monitoring and Maintenance

### Database Health Metrics

```sql
-- Monitor authentication query performance
SELECT 
    query_ref,
    AVG(execution_time) as avg_time,
    COUNT(*) as executions,
    MAX(execution_time) as max_time
FROM query_stats 
WHERE query_ref BETWEEN 1 AND 23
GROUP BY query_ref
ORDER BY avg_time DESC;
```

### Maintenance Tasks

1. **Regular Index Optimization**:

   ```sql
   ANALYZE accounts;
   REINDEX idx_accounts_username;
   REINDEX idx_accounts_email;
   ```

2. **Query Performance Monitoring**:
   - Track slow queries (>100ms)
   - Monitor cache hit ratios
   - Alert on failed query executions

## Conclusion

The Acuranzo database provides a solid foundation for the authentication system with 20 existing QueryRefs already implemented. The system requires 3 additional QueryRefs for user registration functionality. The integration follows established Hydrogen patterns using the DQM system for safe, parameterized query execution.

**Next Steps**:

1. Implement the 3 new QueryRef migrations (1141-1143)
2. Create test data migrations for development
3. Update migration documentation
4. Integrate with Hydrogen query system
5. Develop comprehensive database tests