# Test 53: Conduit Authenticated Multiple Queries Endpoint

## Overview

The [`test_53_conduit_auth_queries.sh`](/elements/001-hydrogen/hydrogen/tests/test_53_conduit_auth_queries.sh) script tests the `/api/conduit/auth_queries` endpoint for authenticated batch query execution across all 7 database engines.

## Purpose

This test validates:

- Authenticated batch query execution
- Multiple QueryRef execution in single request
- JWT token validation across all databases
- Response format for batch operations
- Error handling in batch context

## Test Configuration

- **Test Name**: Conduit Auth Queries
- **Test Abbreviation**: CAM
- **Test Number**: 53
- **Version**: 1.0.0

## Key Features

### Batch Query Testing

Tests `/api/conduit/auth_queries` endpoint which allows multiple queries in a single request:

```json
POST /api/conduit/auth_queries
{
  "queries": [
    {"query_ref": 30, "params": {}},
    {"query_ref": 45, "params": {"STRING": {"TOP": "50"}}}
  ]
}
```

Note: No `database` field required; uses database claim from JWT.

## Test Flow

1. **Startup**: Launch Hydrogen with unified conduit configuration (port 5530)
2. **Migration Wait**: Wait for all database migrations to complete
3. **JWT Acquisition**: Obtain tokens for all ready databases
4. **Auth Queries Test**: Test `/api/conduit/auth_queries` across ready databases
5. **Shutdown**: Graceful server termination

## QueryRef Coverage

Tests authenticated QueryRef combinations:

| QueryRef | Description |
|----------|-------------|
| 30 | Get Lookups List |
| 45 | Get Lookup |

## Response Format

```json
{
  "success": true,
  "row_count": 23,
  "results": [
    {"query_ref": 30, "row_count": 15, "data": [...]},
    {"query_ref": 45, "row_count": 8, "data": [...]}
  ]
}
```

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle, JWT acquisition, request validation
- **`framework.sh`**: Test framework

## Configuration File

**`hydrogen_test_53_conduit_auth_queries.json`**:

- Port: 5530
- All 7 database engines configured

## Skipped Scenarios

If no JWT token is available (database login failed):

- Test logs `AUTH_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN` to result file
- Test reports as passed (skipped gracefully)

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Main conduit endpoint tests
- [test_52_conduit_auth_query.md](/docs/H/tests/test_52_conduit_auth_query.md) - Single authenticated query tests