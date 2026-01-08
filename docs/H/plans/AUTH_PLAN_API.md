# AUTH ENDPOINTS - API SPECIFICATIONS

## Overview

This document details the API specifications for the authentication endpoints to be implemented in the Hydrogen project.

## Detailed API Specifications

### Login Endpoint (`POST /api/auth/login`)

**Request Body Schema:**

```json
{
  "type": "object",
  "required": ["login_id", "password", "api_key", "tz"],
  "properties": {
    "login_id": {
      "type": "string",
      "minLength": 1,
      "maxLength": 255,
      "description": "Username or email address for authentication"
    },
    "password": {
      "type": "string",
      "minLength": 8,
      "maxLength": 128,
      "description": "User password"
    },
    "api_key": {
      "type": "string",
      "minLength": 1,
      "maxLength": 255,
      "description": "API key for application identification"
    },
    "tz": {
      "type": "string",
      "minLength": 1,
      "maxLength": 50,
      "description": "Client timezone (e.g., 'America/Vancouver')"
    }
  }
}
```

**Response Schemas:**

*Success (200):*

```json
{
  "type": "object",
  "required": ["token", "expires_at", "user_id"],
  "properties": {
    "token": {
      "type": "string",
      "description": "JWT token with Bearer prefix"
    },
    "expires_at": {
      "type": "integer",
      "description": "Token expiration timestamp (Unix epoch)"
    },
    "user_id": {
      "type": "integer",
      "description": "Authenticated user ID"
    },
    "roles": {
      "type": "array",
      "items": {"type": "string"},
      "description": "User role permissions"
    }
  }
}
```

*Rate Limited (429):*

```json
{
  "type": "object",
  "required": ["error", "retry_after"],
  "properties": {
    "error": {"type": "string", "example": "Too many failed attempts"},
    "retry_after": {"type": "integer", "description": "Seconds until retry allowed"}
  }
}
```

*Invalid Credentials (401):*

```json
{
  "type": "object",
  "required": ["error"],
  "properties": {
    "error": {"type": "string", "example": "Invalid credentials"}
  }
}
```

### Renew Endpoint (`POST /api/auth/renew`)

**Request Body Schema:**

```json
{
  "type": "object",
  "required": ["token"],
  "properties": {
    "token": {
      "type": "string",
      "description": "Current valid JWT token"
    }
  }
}
```

**Response Schema (200):**

```json
{
  "type": "object",
  "required": ["token", "expires_at"],
  "properties": {
    "token": {"type": "string", "description": "New JWT token"},
    "expires_at": {"type": "integer", "description": "New token expiration timestamp"}
  }
}
```

### Logout Endpoint (`POST /api/auth/logout`)

**Request Body Schema:**

```json
{
  "type": "object",
  "required": ["token"],
  "properties": {
    "token": {
      "type": "string",
      "description": "JWT token to invalidate"
    }
  }
}
```

**Response Schema (200):**

```json
{
  "type": "object",
  "required": ["success"],
  "properties": {
    "success": {"type": "boolean", "example": true}
  }
}
```

### Register Endpoint (`POST /api/auth/register`)

**Request Body Schema:**

```json
{
  "type": "object",
  "required": ["username", "password", "email"],
  "properties": {
    "username": {
      "type": "string",
      "minLength": 3,
      "maxLength": 50,
      "pattern": "^[a-zA-Z0-9_-]+$"
    },
    "password": {
      "type": "string",
      "minLength": 8,
      "maxLength": 128
    },
    "email": {
      "type": "string",
      "format": "email",
      "maxLength": 255
    },
    "full_name": {
      "type": "string",
      "maxLength": 255
    }
  }
}
```

**Response Schema (201):**

```json
{
  "type": "object",
  "required": ["user_id", "message"],
  "properties": {
    "user_id": {"type": "integer"},
    "message": {"type": "string", "example": "User registered successfully"}
  }
}
```

## Authentication Flow

### Login Endpoint (`/api/auth/login`)

The login process follows the same comprehensive security checks as the Delphi implementation. Instead of one monolithic function, the logic will be broken into separate, testable functions:

1. **validate_login_input()**: Validate required parameters (Login_ID, Password, API_Key, TZ) against minimum length requirements
2. **validate_timezone()**: Verify the provided timezone string is valid using timezone library
3. **verify_api_key()**: Query APP.Licenses table to validate API key exists and retrieve system/application details
4. **check_license_expiry()**: Adjust JWT expiry if license has impending expiration
5. **check_ip_whitelist()**: Query APP.Lists #1 to check if client IP is whitelisted
6. **check_ip_blacklist()**: If not whitelisted, query APP.Lists #0 to check if IP is blacklisted
7. **log_login_attempt()**: Log initial login attempt with timing and IP information
8. **check_failed_attempts()**: Query recent failed attempts within retry window
9. **handle_rate_limiting()**: If attempts exceed threshold and IP not whitelisted, block IP temporarily by adding to blacklist
10. **lookup_account()**: Search for Login_ID in accounts table
11. **verify_login_enabled()**: Verify account is enabled for authentication
12. **verify_account_authorized()**: Check account has proper access permissions
13. **get_account_email()**: Get primary email address for account
14. **get_user_roles()**: Get user roles for JWT claims
15. **verify_password()**: Hash password and compare with stored hash
16. **generate_jwt()**: Create JWT with claims (user ID, app ID, system ID, roles, email, names, IP, timestamps)
17. **store_jwt()**: Store JWT hash in database for revocation tracking
18. **log_successful_login()**: Log successful login with all details
19. **cleanup_login_records()**: Mark successful login attempts as passed
20. **log_endpoint_access()**: Log API endpoint access for auditing

### Renew Endpoint (`/api/auth/renew`)

Step-by-step implementation:

1. **parse_renew_request()**: Extract and validate JWT from request body
2. **validate_jwt_token()**: Verify JWT signature and check if not expired or revoked
3. **check_token_permissions()**: Ensure token has renewal permissions
4. **generate_new_jwt()**: Create new JWT with updated expiry timestamp
5. **update_jwt_storage()**: Replace old JWT hash with new one in database
6. **log_token_renewal()**: Record renewal event for audit trail
7. **return_new_token()**: Send new JWT back to client

### Logout Endpoint (`/api/auth/logout`)

Step-by-step implementation:

1. **parse_logout_request()**: Extract JWT from request body or Authorization header
2. **validate_jwt_for_logout()**: Verify JWT is valid (but allow expired tokens for logout)
3. **delete_jwt_from_storage()**: Remove JWT hash from database
4. **log_logout_event()**: Record logout for audit purposes
5. **return_logout_confirmation()**: Send success response

### Register Endpoint (`/api/auth/register`)

Step-by-step implementation:

1. **validate_registration_input()**: Check username, password, email format and requirements
2. **check_username_availability()**: Query database to ensure username isn't taken
3. **check_email_availability()**: Query database to ensure email isn't already registered
4. **hash_password()**: Generate salted password hash
5. **create_account_record()**: Insert new account into database
6. **set_default_roles()**: Assign default user roles and permissions
7. **send_confirmation_email()**: Queue email verification (if configured)
8. **log_registration()**: Record account creation for audit
9. **return_registration_result()**: Send success response or verification requirements