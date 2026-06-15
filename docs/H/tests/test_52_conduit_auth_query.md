# Test 52: Conduit Authenticated Single Query Endpoint

## Overview

The [`test_52_conduit_auth_query.sh`](/elements/001-hydrogen/hydrogen/tests/test_52_conduit_auth_query.sh) script tests the `/api/conduit/auth_query` endpoint for authenticated query execution across all 7 database engines.

## Purpose

This test validates:

- Authenticated access to QueryRef-protected queries
- JWT token validation and authorization
- Proper error handling for expired/revoked tokens
- JWT claims verification (database claim presence)
- Response format consistency across all database engines

## Test Configuration

- **Test Name**: Conduit Auth Query
- **Test Abbreviation**: CA1
- **Test Number**: 52
- **Version**: 1.1.0

## Key Features

### Authentication Testing

Tests various JWT-related scenarios:

1. **Valid JWT**: Authenticated query succeeds
2. **No JWT**: Missing authorization returns error
3. **Malformed JWT**: Invalid token format rejected
4. **Expired JWT**: Expired token returns error
5. **Revoked JWT**: Token not in hash cache returns error
6. **Missing database claim**: JWT without database claim rejected

### Status Endpoint Testing

Tests `/api/conduit/status` public endpoint to verify:

- Server reports ready databases
- Status matches migration completion logs

## Test Flow

1. **Startup**: Launch Hydrogen with unified conduit configuration (port 5520)
2. **Migration Wait**: Wait for all database migrations to complete
3. **JWT Acquisition**: Obtain tokens for all ready databases via `/api/auth/login`
4. **Status Test**: Verify `/api/conduit/status` reports correct databases
5. **Auth Query Tests**: Test `/api/conduit/auth_query` across ready databases
6. **Shutdown**: Graceful server termination

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle, JWT acquisition, request validation
- **`framework.sh`**: Test framework, result tracking

## Configuration File

**`hydrogen_test_52_conduit_auth_query.json`**:

- Port: 5520
- API Prefix: `/api`
- All 7 database engines configured

## Authenticated Query Tests

Tests QueryRef #30 (Get Lookups List) and #45 (Get Lookup) which require authentication:

| QueryRef | Description | Expected |
|----------|-------------|----------|
| 30 | Get Lookups List | Returns lookup data |
| 45 | Get Lookup | Returns single lookup |

## Error Handling

### JWT Validation Errors

| Condition | Expected Response |
|-----------|-----------------|
| No Authorization header | HTTP 401 or 400 with error |
| Malformed JWT format | HTTP 401 with parse error |
| Expired token | HTTP 401 with expiration message |
| Revoked token | HTTP 401 with revocation message |
| Missing database claim | HTTP 401 or 400 |

## Request Format

```json
POST /api/conduit/auth_query
{
  "query_ref": 30,
  "params": {
    "STRING": {"status": "active"}
  }
}
```

Note: No `database` field required; uses database claim from JWT.

## Response Format

```json
{
  "success": true,
  "row_count": 15,
  "results": [...]
}
```

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Main conduit endpoint tests
- [test_50_conduit_query.md](/docs/H/tests/test_50_conduit_query.md) - Public query endpoint tests