# AUTH ENDPOINTS - IMPLEMENTATION DETAILS

## Detailed Function Implementations

### Login Functions with Pseudo-code

#### validate_login_input()

```c
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz) {
    // Check minimum lengths
    if (strlen(login_id) < 1 || strlen(login_id) > 255) return false;
    if (strlen(password) < 8 || strlen(password) > 128) return false;
    if (strlen(api_key) < 1 || strlen(api_key) > 255) return false;
    if (strlen(tz) < 1 || strlen(tz) > 50) return false;

    // Validate timezone format (basic check)
    if (!is_valid_timezone(tz)) return false;

    return true;
}
```

#### verify_api_key()

```c
bool verify_api_key(const char* api_key, system_info_t* sys_info) {
    // Execute QueryRef #001: Check API key exists in APP.Licenses
    // Parameters: { "api_key": api_key }
    // Expected result: system_id, app_id, license_expiry, features

    query_result_t result = execute_query(1, json_object({
        "api_key": api_key
    }));

    if (result.row_count == 0) {
        log_this("AUTH", "Invalid API key attempted: %s", api_key);
        return false;
    }

    // Extract system information
    sys_info->system_id = result.rows[0]["system_id"];
    sys_info->app_id = result.rows[0]["app_id"];
    sys_info->license_expiry = result.rows[0]["license_expiry"];

    return true;
}
```

#### check_ip_whitelist()

```c
bool check_ip_whitelist(const char* client_ip) {
    // Execute QueryRef #002: Get IP Whitelist (APP.Lists #1)
    // Parameters: { "ip_address": client_ip }
    // Expected result: boolean exists

    query_result_t result = execute_query(2, json_object({
        "ip_address": client_ip
    }));

    return (result.row_count > 0);
}
```

#### check_ip_blacklist()

```c
bool check_ip_blacklist(const char* client_ip) {
    // Execute QueryRef #003: Get IP Blacklist (APP.Lists #0)
    // Parameters: { "ip_address": client_ip }
    // Expected result: boolean exists

    query_result_t result = execute_query(3, json_object({
        "ip_address": client_ip
    }));

    return (result.row_count > 0);
}
```

#### log_login_attempt()

```c
void log_login_attempt(const char* login_id, const char* client_ip,
                      const char* user_agent, time_t timestamp) {
    // Execute QueryRef #004: Log Login Attempt
    // Parameters: { "login_id": login_id, "ip": client_ip, "timestamp": timestamp }
    // Expected result: attempt_id

    execute_query(4, json_object({
        "login_id": login_id,
        "ip": client_ip,
        "user_agent": user_agent,
        "timestamp": timestamp
    }));
}
```

#### check_failed_attempts()

```c
int check_failed_attempts(const char* login_id, const char* client_ip,
                         time_t window_start) {
    // Execute QueryRef #005: Get Login Attempt Count
    // Parameters: { "login_id": login_id, "ip": client_ip, "since": window_start }
    // Expected result: count of failed attempts

    query_result_t result = execute_query(5, json_object({
        "login_id": login_id,
        "ip": client_ip,
        "since": window_start
    }));

    return result.rows[0]["attempt_count"];
}
```

#### handle_rate_limiting()

```c
bool handle_rate_limiting(const char* client_ip, int failed_count,
                         bool is_whitelisted) {
    const int MAX_ATTEMPTS = 5;
    const int BLOCK_DURATION_MINUTES = 15;

    if (failed_count >= MAX_ATTEMPTS && !is_whitelisted) {
        // Execute QueryRef #007: Block IP Address Temporarily
        // Parameters: { "ip": client_ip, "duration_minutes": BLOCK_DURATION_MINUTES }

        execute_query(7, json_object({
            "ip": client_ip,
            "duration_minutes": BLOCK_DURATION_MINUTES
        }));

        return true; // IP blocked
    }

    return false; // No blocking needed
}
```

#### lookup_account()

```c
account_info_t* lookup_account(const char* login_id) {
    // Execute QueryRef #008: Get Account ID
    // Parameters: { "login_id": login_id }
    // Expected result: account_id, username, email, enabled, authorized

    query_result_t result = execute_query(8, json_object({
        "login_id": login_id
    }));

    if (result.row_count == 0) {
        return NULL; // Account not found
    }

    account_info_t* account = calloc(1, sizeof(account_info_t));
    account->id = result.rows[0]["account_id"];
    account->username = strdup(result.rows[0]["username"]);
    account->email = strdup(result.rows[0]["email"]);
    account->enabled = result.rows[0]["enabled"];
    account->authorized = result.rows[0]["authorized"];

    return account;
}
```

#### verify_password()

```c
bool verify_password(const char* password, const char* stored_hash) {
    // Password hash format: account_id + password (SHA256)
    // Compare computed hash with stored hash

    char* computed_hash = compute_password_hash(password, account_id);
    bool matches = (strcmp(computed_hash, stored_hash) == 0);

    free(computed_hash);
    return matches;
}
```

#### generate_jwt()

```c
char* generate_jwt(account_info_t* account, system_info_t* system,
                  const char* client_ip, time_t issued_at) {
    // JWT Claims structure
    jwt_claims_t claims = {
        .iss = "hydrogen-auth",           // Issuer
        .sub = account->id,               // Subject (user ID)
        .aud = system->app_id,            // Audience (app ID)
        .exp = issued_at + JWT_LIFETIME,  // Expiration
        .iat = issued_at,                 // Issued at
        .nbf = issued_at,                 // Not before
        .jti = generate_jti(),            // JWT ID
        .user_id = account->id,
        .system_id = system->system_id,
        .app_id = system->app_id,
        .username = account->username,
        .email = account->email,
        .roles = account->roles,
        .ip = client_ip,
        .tz = client_tz
    };

    // Sign with HMAC-SHA256
    return jwt_encode(&claims, jwt_secret, JWT_ALG_HMAC_SHA256);
}
```

#### store_jwt()

```c
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at) {
    // Execute QueryRef #013: Store JWT
    // Parameters: { "account_id": account_id, "jwt_hash": jwt_hash, "expires": expires_at }

    execute_query(13, json_object({
        "account_id": account_id,
        "jwt_hash": jwt_hash,
        "expires": expires_at
    }));
}
```

### JWT Handling Architecture

#### JWT Library Requirements

Hydrogen currently uses a custom JWT implementation. For the auth endpoints, we need to ensure:

1. **HMAC-SHA256 signing** - Already supported
2. **RSA signing** - May need to add for enhanced security
3. **Key rotation** - Implement via existing OIDC key management
4. **Token validation** - Extend current validation functions

#### JWT Secret Management

##### Key Types and Purposes

Hydrogen supports two JWT signing methods:

1. **HMAC-SHA256 (Symmetric)**: Uses a shared secret for both signing and verification
   - **Purpose**: Fast signing/verification, suitable for single-tenant deployments
   - **Security**: Secret must be shared across all server instances
   - **Rotation**: Requires coordinated rollout across all instances

2. **RSA-SHA256 (Asymmetric)**: Uses private key for signing, public key for verification
   - **Purpose**: Better for multi-tenant or distributed deployments
   - **Security**: Private key stays on server, public key can be shared
   - **Rotation**: Can rotate keys independently, public key can be cached by clients

##### Key Generation Commands

**HMAC Secret Generation:**

```bash
# Generate a 256-bit (32-byte) random secret
openssl rand -hex 32

# Or using /dev/urandom
head -c 32 /dev/urandom | xxd -p -c 32

# Example output: a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890
```

**RSA Key Pair Generation:**

```bash
# Generate 2048-bit RSA private key
openssl genrsa -out jwt_private.pem 2048

# Extract public key from private key
openssl rsa -in jwt_private.pem -pubout -out jwt_public.pem

# Convert to single-line format for environment variables
awk 'NF {sub(/\r/, ""); printf "%s\\n",$0;}' jwt_private.pem
awk 'NF {sub(/\r/, ""); printf "%s\\n",$0;}' jwt_public.pem
```

##### Key Storage and Access

**Environment Variables:**

```bash
# HMAC Secret (hex-encoded)
export HYDROGEN_JWT_HMAC_SECRET="a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890"

# RSA Keys (PEM format, single line)
export HYDROGEN_JWT_RSA_PRIVATE_KEY="-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEA...-----END RSA PRIVATE KEY-----"
export HYDROGEN_JWT_RSA_PUBLIC_KEY="-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...-----END PUBLIC KEY-----"

# Rotation settings
export HYDROGEN_JWT_ROTATION_DAYS="90"
export HYDROGEN_JWT_USE_RSA="true"
```

**Configuration Structure:**

```c
typedef struct {
    char* hmac_secret;           // Hex-encoded HMAC secret
    char* rsa_private_key;       // PEM-encoded RSA private key
    char* rsa_public_key;        // PEM-encoded RSA public key
    int rotation_interval_days;  // Auto-rotation interval
    bool use_rsa;               // Prefer RSA over HMAC
    time_t last_rotation;       // Timestamp of last rotation
    char rotation_salt[32];     // Salt for key derivation
} jwt_config_t;
```

##### Key Lifecycle Management

**Initial Setup:**

1. Generate keys using above commands
2. Store in secure environment (Kubernetes secrets, HashiCorp Vault, etc.)
3. Deploy to all server instances simultaneously
4. Verify all instances can validate tokens from each other

**Rotation Process:**

```bash
# 1. Generate new keys
NEW_HMAC_SECRET=$(openssl rand -hex 32)
openssl genrsa -out new_private.pem 2048
openssl rsa -in new_private.pem -pubout -out new_public.pem

# 2. Update environment variables
export HYDROGEN_JWT_HMAC_SECRET="$NEW_HMAC_SECRET"
# ... update RSA keys ...

# 3. Rolling deployment (DOKS)
kubectl set env deployment/hydrogen HYDROGEN_JWT_HMAC_SECRET="$NEW_HMAC_SECRET"
kubectl rollout restart deployment/hydrogen

# 4. Monitor for validation errors
tail -f /var/log/hydrogen.log | grep "JWT"
```

**Key Rotation Strategy:**

- **Overlap Period**: Keep old keys valid during transition (24-48 hours)
- **Graceful Degradation**: Accept both old and new keys during rotation
- **Forced Logout**: Optional - invalidate all sessions on rotation
- **Audit Trail**: Log all key rotations with timestamps

##### Multi-Instance Deployment (DOKS) Considerations

**Key Synchronization:**

- All instances must have identical keys
- Use Kubernetes secrets or config maps for consistency
- Implement key version checking to detect mismatches

**Token Validation Across Instances:**

```c
// During key rotation, accept multiple key versions
jwt_validation_result_t validate_jwt_multi_version(const char* token) {
    // Try current key first
    jwt_validation_result_t result = validate_jwt_with_key(token, current_key);
    if (result.valid) return result;

    // Try previous key (during rotation window)
    if (time(NULL) - last_rotation < ROTATION_GRACE_PERIOD) {
        result = validate_jwt_with_key(token, previous_key);
        if (result.valid) {
            log_this("AUTH", "Token validated with previous key version");
            return result;
        }
    }

    return (jwt_validation_result_t){ .valid = false, .error = JWT_ERROR_INVALID };
}
```

**Failure Scenarios:**

- **Key Mismatch**: Instances with different keys can't validate each other's tokens
- **Rotation Timing**: Tokens issued just before rotation may become invalid
- **Cache Invalidation**: Public keys cached by CDNs may need purging

**Monitoring Key Health:**

- Alert on key rotation failures
- Monitor token validation success rates
- Track key age and schedule rotations
- Log key synchronization status

##### Security Considerations

**Key Protection:**

- Never log keys in plaintext
- Use secure key storage (KMS, Vault, etc.)
- Implement access controls for key management
- Regular security audits of key handling code

**Compromise Response:**

1. **Immediate**: Generate new keys, invalidate all tokens
2. **Investigation**: Analyze access logs for suspicious activity
3. **Communication**: Notify affected users if necessary
4. **Recovery**: Implement additional security measures

#### JWT Validation Function

```c
jwt_validation_result_t validate_jwt(const char* token) {
    jwt_validation_result_t result = {0};

    // Decode token without verification first
    jwt_header_t header = jwt_decode_header(token);
    jwt_claims_t claims = jwt_decode_claims(token);

    // Check expiration
    time_t now = time(NULL);
    if (claims.exp < now) {
        result.valid = false;
        result.error = JWT_ERROR_EXPIRED;
        return result;
    }

    // Check not before
    if (claims.nbf > now) {
        result.valid = false;
        result.error = JWT_ERROR_NOT_YET_VALID;
        return result;
    }

    // Verify signature based on algorithm
    if (strcmp(header.alg, "HS256") == 0) {
        result.valid = jwt_verify_hmac(token, jwt_config.hmac_secret);
    } else if (strcmp(header.alg, "RS256") == 0) {
        result.valid = jwt_verify_rsa(token, jwt_config.rsa_public_key);
    } else {
        result.valid = false;
        result.error = JWT_ERROR_UNSUPPORTED_ALGORITHM;
    }

    // Check if token is revoked (database lookup)
    if (result.valid) {
        char* token_hash = compute_token_hash(token);
        result.valid = !is_token_revoked(token_hash);
        free(token_hash);
    }

    if (result.valid) {
        result.claims = claims;
    }

    return result;
}
```

### Renew Functions

#### parse_renew_request()

```c
char* parse_renew_request(const char* request_body) {
    // Parse JSON: { "token": "jwt_string" }
    json_t* json = json_parse(request_body);
    if (!json) return NULL;

    json_t* token_field = json_object_get(json, "token");
    if (!token_field || !json_is_string(token_field)) {
        json_decref(json);
        return NULL;
    }

    char* token = strdup(json_string_value(token_field));
    json_decref(json);
    return token;
}
```

#### validate_jwt_token()

```c
jwt_validation_result_t validate_jwt_token(const char* token) {
    return validate_jwt(token); // Use shared validation function
}
```

#### generate_new_jwt()

```c
char* generate_new_jwt(jwt_claims_t* old_claims) {
    // Create new claims with updated timestamps
    jwt_claims_t new_claims = *old_claims;
    time_t now = time(NULL);

    new_claims.iat = now;
    new_claims.nbf = now;
    new_claims.exp = now + JWT_LIFETIME;
    new_claims.jti = generate_jti(); // New JWT ID

    return jwt_encode(&new_claims, jwt_secret, JWT_ALG_HMAC_SHA256);
}
```

#### update_jwt_storage()

```c
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                       const char* new_jwt_hash, time_t new_expires) {
    // Delete old JWT: QueryRef #019
    execute_query(19, json_object({
        "jwt_hash": old_jwt_hash
    }));

    // Store new JWT: QueryRef #013
    execute_query(13, json_object({
        "account_id": account_id,
        "jwt_hash": new_jwt_hash,
        "expires": new_expires
    }));
}
```

### Logout Functions

#### parse_logout_request()

```c
char* parse_logout_request(const char* request_body) {
    // Same as parse_renew_request - extract token from JSON
    return parse_renew_request(request_body);
}
```

#### validate_jwt_for_logout()

```c
jwt_validation_result_t validate_jwt_for_logout(const char* token) {
    // For logout, we accept expired tokens but still validate signature
    jwt_validation_result_t result = validate_jwt(token);

    // Override expiration check for logout
    if (result.error == JWT_ERROR_EXPIRED) {
        // Decode claims anyway for token hash computation
        jwt_claims_t claims = jwt_decode_claims(token);
        result.valid = true; // Accept expired token for logout
        result.claims = claims;
        result.error = JWT_ERROR_NONE;
    }

    return result;
}
```

#### delete_jwt_from_storage()

```c
void delete_jwt_from_storage(const char* jwt_hash) {
    // Execute QueryRef #019: Delete JWT
    execute_query(19, json_object({
        "jwt_hash": jwt_hash
    }));
}
```

### Register Functions

#### validate_registration_input()

```c
bool validate_registration_input(const char* username, const char* password,
                                const char* email, const char* full_name) {
    // Username: 3-50 chars, alphanumeric + underscore/hyphen
    if (!username || strlen(username) < 3 || strlen(username) > 50) return false;
    if (!is_alphanumeric_underscore_hyphen(username)) return false;

    // Password: 8-128 chars
    if (!password || strlen(password) < 8 || strlen(password) > 128) return false;

    // Email: valid format, max 255 chars
    if (!email || strlen(email) > 255 || !is_valid_email(email)) return false;

    // Full name: optional, max 255 chars
    if (full_name && strlen(full_name) > 255) return false;

    return true;
}
```

#### check_username_availability()

```c
bool check_username_availability(const char* username) {
    // Need new QueryRef #021: Check username/email availability
    // Parameters: { "username": username }
    // Expected result: count (0 = available)

    query_result_t result = execute_query(21, json_object({
        "username": username
    }));

    return (result.rows[0]["count"] == 0);
}
```

#### hash_password()

```c
char* hash_password(const char* password, int account_id) {
    // Format: SHA256(account_id + password)
    char* combined = malloc(strlen(password) + 32); // account_id max length
    sprintf(combined, "%d%s", account_id, password);

    char* hash = sha256_hash(combined);
    free(combined);

    return hash;
}
```

#### create_account_record()

```c
int create_account_record(const char* username, const char* email,
                         const char* hashed_password, const char* full_name) {
    // Need new QueryRef #022: Create new account
    // Parameters: { "username": username, "email": email, "password_hash": hashed_password, "full_name": full_name }
    // Expected result: new account_id

    query_result_t result = execute_query(22, json_object({
        "username": username,
        "email": email,
        "password_hash": hashed_password,
        "full_name": full_name
    }));

    return result.rows[0]["account_id"];
}
```

## Implementation Architecture

### API Structure

Following the existing Hydrogen API pattern with separate directories for each endpoint:

```structure
src/api/auth/
├── auth_service.c           # Main integration file (lightweight, includes all modules)
├── auth_service.h           # Auth service header with common functions
├── auth_service_jwt.c       # JWT operations implementation (~500 lines)
├── auth_service_jwt.h       # JWT operations internal header
├── auth_service_validation.c # Input validation implementation (~180 lines)
├── auth_service_validation.h # Validation internal header
├── auth_service_database.c  # Database operations implementation (~500 lines)
├── auth_service_database.h  # Database operations internal header
├── login/
│   ├── login.c             # Login endpoint implementation
│   └── login.h             # Login header with Swagger documentation
├── renew/
│   ├── renew.c             # Renew endpoint implementation
│   └── renew.h             # Renew header with Swagger documentation
├── logout/
│   ├── logout.c            # Logout endpoint implementation
│   └── logout.h            # Logout header with Swagger documentation
└── register/
    ├── register.c          # Register endpoint implementation
    └── register.h          # Register header with Swagger documentation
```

**Refactoring Note (2026-01-09)**: The original 1180-line [`auth_service.c`](/elements/001-hydrogen/hydrogen/src/api/auth/auth_service.c) has been refactored into focused modules to improve maintainability and meet the project's goal of keeping source files under 500 lines (max 1,000 lines):

- **auth_service_jwt.c** (~500 lines): All JWT-related operations including:
  - Base64url encoding/decoding
  - JWT generation and validation
  - Token hashing and management
  - JWT configuration

- **auth_service_validation.c** (~180 lines): All validation operations including:
  - Login input validation
  - Registration input validation
  - Email and timezone format validation
  - IP whitelist/blacklist checks
  - Rate limiting logic

- **auth_service_database.c** (~500 lines): All database operations including:
  - Query execution wrappers
  - Account lookup and management
  - Password verification
  - JWT storage and revocation
  - Login attempt logging
  - IP blocking

- **auth_service.c** (~17 lines): Lightweight integration file that includes all three modules

This modular structure provides:

- Better code organization with clear separation of concerns
- Easier testing with focused unit test targets
- Improved maintainability with smaller, more manageable files
- Better build times with focused compilation units

### JWT Handling

- Use existing JWT library integration
- Claims structure matches Delphi implementation
- SHA256 HMAC signing with configurable secrets
- Configurable expiry times from app config
- Revocation via database storage with hash indexing

### Swagger Documentation

Each endpoint header will include comprehensive Swagger documentation for automatic API documentation generation:

```c
//@ swagger:path /api/auth/login
//@ swagger:method POST
//@ swagger:operationId postAuthLogin
//@ swagger:tags "Authentication Service"
//@ swagger:summary User login endpoint
//@ swagger:description Authenticates user credentials and returns JWT token with comprehensive security checks including IP whitelisting, rate limiting, and audit logging.
//@ swagger:requestBody {"required":true,"content":{"application/json":{"schema":{"type":"object","required":["login_id","password","api_key","tz"],"properties":{"login_id":{"type":"string","example":"user@example.com"},"password":{"type":"string","example":"password123"},"api_key":{"type":"string","example":"abc123"},"tz":{"type":"string","example":"America/Vancouver"}}}}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"token":{"type":"string","example":"Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"error":{"type":"string","example":"Invalid credentials"}}}
//@ swagger:response 429 application/json {"type":"object","properties":{"error":{"type":"string","example":"Too many failed attempts"}}}