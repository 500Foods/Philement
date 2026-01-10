# AUTH ENDPOINTS - TABLE OF CONTENTS

## Overview

This document serves as the central table of contents for the Hydrogen authentication endpoints implementation. All detailed content has been organized into specialized documents for better maintainability and clarity.

## Review

A thorough review of the authentication system architecture and implementation plan has been completed by Kilo Code (Architect Mode) on 2026-01-08. The review confirms the plan is complete and ready for implementation: [AUTH_PLAN_REVIEW.md](/docs/H/plans/AUTH_PLAN_REVIEW.md).

## Quick Reference

### Endpoints to be Implemented

- `POST /api/auth/login` - User login with username/password, returns JWT
- `POST /api/auth/renew` - Renew JWT token
- `POST /api/auth/logout` - Logout, invalidate JWT
- `POST /api/auth/register` - User registration

### Key Implementation Points

- **Database**: Uses Acuranzo schema with QueryRefs #001-#023
- **JWT**: HMAC-SHA256 signing with configurable secrets and RSA support
- **Security**: IP whitelisting/blacklisting, rate limiting, comprehensive audit logging
- **Integration**: Follows existing Hydrogen patterns for API, logging, and configuration
- **Testing**: Unity unit tests + comprehensive blackbox integration tests

## Documentation Structure

### 📚 Core Documents

| Document | Purpose | Status |
| ---------- | --------- | -------- |
| **[AUTH_PLAN_DIAGRAMS.md](/docs/H/plans/AUTH_PLAN_DIAGRAMS.md)** | Architecture and flow diagrams | ✅ Complete |
| **[AUTH_PLAN_API.md](/docs/H/plans/AUTH_PLAN_API.md)** | API specifications and schemas | ✅ Complete |
| **[AUTH_PLAN_IMPLEMENTATION.md](/docs/H/plans/AUTH_PLAN_IMPLEMENTATION.md)** | Function implementations and pseudo-code | ✅ Complete |
| **[AUTH_PLAN_SECURITY.md](/docs/H/plans/AUTH_PLAN_SECURITY.md)** | Security considerations and hardening | ✅ Complete |
| **[AUTH_PLAN_INTEGRATION.md](/docs/H/plans/AUTH_PLAN_INTEGRATION.md)** | Subsystem integration details | ✅ Complete |
| **[AUTH_PLAN_PERFORMANCE.md](/docs/H/plans/AUTH_PLAN_PERFORMANCE.md)** | Performance and scalability | ✅ Complete |
| **[AUTH_PLAN_ACURANZO.md](/docs/H/plans/AUTH_PLAN_ACURANZO.md)** | Database integration and migrations | ✅ Complete |
| **[AUTH_PLAN_OPERATIONS.md](/docs/H/plans/AUTH_PLAN_OPERATIONS.md)** | Monitoring and operations | ✅ Complete |
| **[AUTH_PLAN_ERRORS.md](/docs/H/plans/AUTH_PLAN_ERRORS.md)** | Error handling and edge cases | ✅ Complete |

### 📈 Planning & Progress

| Document | Purpose | Status |
| ---------- | --------- | -------- |
| **[AUTH_PLAN_ACURANZO.md](/docs/H/plans/AUTH_PLAN_ACURANZO.md)** | Database integration and migrations | ✅ Complete |
| **[AUTH_PLAN_PROGRESS.md](/docs/H/plans/AUTH_PLAN_PROGRESS.md)** | Implementation progress tracking, risk analysis, team assignments, phase checklists | ✅ Consolidated |
| **[AUTH_PLAN_TESTS.md](/docs/H/plans/AUTH_PLAN_TESTS.md)** | Comprehensive testing strategy | ✅ Complete |

## Quick Start Guide

### For Developers

1. **Review the architecture**: Start with [AUTH_PLAN_DIAGRAMS.md](/docs/H/plans/AUTH_PLAN_DIAGRAMS.md)
2. **Understand the API**: See [AUTH_PLAN_API.md](/docs/H/plans/AUTH_PLAN_API.md) for specifications
3. **Implementation details**: Check [AUTH_PLAN_IMPLEMENTATION.md](/docs/H/plans/AUTH_PLAN_IMPLEMENTATION.md)
4. **Database setup**: Follow [AUTH_PLAN_ACURANZO.md](/docs/H/plans/AUTH_PLAN_ACURANZO.md)

### For Testers

1. **Blackbox/Integration Testing**: [TESTING.md](/docs/H/tests/TESTING.md) - Framework and patterns for integration tests
2. **Unity Unit Testing**: [TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) - Framework and patterns for unit tests
3. **Auth Testing Strategy**: [AUTH_PLAN_TESTS.md](/docs/H/plans/AUTH_PLAN_TESTS.md) - Comprehensive auth-specific test plan
4. **Test cases**: See integration test section in AUTH_PLAN_TESTS.md
5. **Environment setup**: Check test data requirements in AUTH_PLAN_ACURANZO.md

### For Operations

1. **Deployment & Progress**: [AUTH_PLAN_PROGRESS.md](/docs/H/plans/AUTH_PLAN_PROGRESS.md) (includes deployment checklist)
2. **Monitoring setup**: [AUTH_PLAN_OPERATIONS.md](/docs/H/plans/AUTH_PLAN_OPERATIONS.md)
3. **Performance targets**: [AUTH_PLAN_PERFORMANCE.md](/docs/H/plans/AUTH_PLAN_PERFORMANCE.md)

## Developer Reference Guide

### 📁 Key File Locations

| Component | Location | Purpose |
| ----------- | ---------- | --------- |
| **Auth Service** | `src/api/auth/` | Main auth implementation |
| **Database Types** | `src/database/database.h` | `QueryResult` structure |
| **OIDC Structures** | `src/oidc/oidc_tokens.h` | JWT claims reference |
| **Config System** | `src/config/config.h` | Configuration access |
| **Logging** | `src/logging/` | `log_this()` function |
| **API Framework** | `src/api/api_service.h` | API request handling |

### 🔧 Required Includes

```c
// Standard libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Project libraries
#include <src/config/config.h>        // Configuration access
#include <src/database/database.h>    // QueryResult structure
#include <src/hydrogen.h>             // Main project header
```

### 📊 Key Data Structures

| Structure | Location | Purpose |
| ----------- | ---------- | --------- |
| `QueryResult` | `database.h:89` | Database query results |
| `OIDCTokenClaims` | `oidc_tokens.h:40` | JWT claims reference |
| `account_info_t` | `auth_service.h` | User account data |
| `system_info_t` | `auth_service.h` | API key system data |
| `jwt_claims_t` | `auth_service.h` | Auth JWT claims |
| `jwt_config_t` | `auth_service.h` | JWT configuration |

### 🛠️ Common Patterns

**Database Queries:**

```c
// Use conduit service for QueryRef execution
// QueryRef #001-#023 are pre-defined in payload
QueryResult* result = execute_query(ref_number, json_params);
```

**Logging:**

```c
// Use SR_AUTH subsystem for auth logging
log_this("AUTH", "Login attempt: %s from %s", username, ip);
```

**Memory Management:**

```c
// Always check for NULL after allocations
account_info_t* account = calloc(1, sizeof(account_info_t));
// ... use account ...
free_account_info(account); // Use cleanup functions
```

**Error Handling:**

```c
if (!result || !result->success) {
    log_this("AUTH", "Database error: %s", result->error_message);
    return false;
}
```

### 🔍 Implementation Search Paths

When implementing auth functions, check these locations in order:

1. **Existing OIDC code** (`src/oidc/`) - JWT handling patterns
2. **Database examples** (`src/api/conduit/`) - Query execution
3. **API endpoints** (`src/api/system/`) - Request/response patterns
4. **Config usage** (`src/config/`) - Configuration access

## Environment Variables Required

```bash
export HYDROGEN_JWT_SECRET="your-256-bit-secret-here"
export HYDROGEN_AUTH_ENABLED="true"
export HYDROGEN_AUTH_MAX_FAILED_ATTEMPTS="5"
export HYDROGEN_AUTH_RATE_LIMIT_WINDOW="15"
export HYDROGEN_AUTH_IP_BLOCK_DURATION="15"
export HYDROGEN_AUTH_ENABLE_REGISTRATION="true"
```
