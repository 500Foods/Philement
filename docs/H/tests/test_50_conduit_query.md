# Test 50: Conduit Single Query Endpoint

## Overview

The [`test_50_conduit_query.sh`](/elements/001-hydrogen/hydrogen/tests/test_50_conduit_query.sh) script tests the `/api/conduit/query` endpoint for single public query execution across all 7 database engines.

## Purpose

This test validates:

- Public (no authentication) single query execution
- All 9 parameter types (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP)
- Cross-database result consistency
- Error handling for invalid queries
- Status endpoint functionality

## Test Configuration

- **Test Name**: Conduit Query
- **Test Abbreviation**: CQ1
- **Test Number**: 50
- **Version**: 1.8.1

## Key Features

### Parameter Type Testing

Tests all 9 supported parameter types via QueryRef #56, #57:

| Type | Description | Test Query |
|------|-------------|----------|
| INTEGER | 64-bit integers | QueryRef 56 with NUMERATOR/DENOMINATOR |
| STRING | Variable strings | QueryRef 53, #54 |
| BOOLEAN | true/false | QueryRef 57 |
| FLOAT | Double precision | QueryRef 57 |
| TEXT | Large text/CLOB | QueryRef 57 |
| DATE | YYYY-MM-DD | QueryRef 57 |
| TIME | HH:MM:SS | QueryRef 57 |
| DATETIME | Combined date/time | QueryRef 57 |
| TIMESTAMP | Date/time with ms | QueryRef 57 |

### Query Types Tested

| QueryRef | Description | Auth Required |
|----------|-------------|---------------|
| 30 | Get Lookups List (`TYPE_PUBLIC`) | No |
| 45 | Get Version History (system SQL; used as non-public negative sample elsewhere) | Yes |
| 53 | Get Themes | No |
| 54 | Get Icons | No |
| 55 | Get Number Range | No |
| 56 | Division (error test) | No |
| 57 | All parameter types | No |

### Status Endpoint Testing

Two status endpoint tests:

1. **Public**: `/api/conduit/status` without JWT - returns basic status
2. **Authenticated**: `/api/conduit/status` with JWT - returns per-database QTC stats

### Error Path Testing

Tests various failure scenarios:

- **Missing parameters**: Query #55 with only START, missing FINISH
- **Unused parameters**: Query #55 with extra unexpected parameters
- **Invalid database**: Non-existent database name in request
- **Parameter type mismatch**: String sent for INTEGER parameter
- **Invalid JSON**: Malformed request body

## Test Flow

1. **Startup**: Launch Hydrogen with unified conduit configuration (port 5500)
2. **Migration Wait**: Wait for all database migrations to complete
3. **JWT Acquisition**: Obtain tokens for ready databases
4. **Status Tests**: Validate `/api/conduit/status` functionality
5. **Single Public Query Tests**: Test valid single queries
6. **Invalid Query Tests**: Test error scenarios
7. **Parameter Tests**: Exercise all 9 parameter types
8. **Shutdown**: Graceful server termination

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle, JWT acquisition, request validation
- **`framework.sh`**: Test framework

## Configuration File

**`hydrogen_test_50_conduit_query.json`**:

- Port: 5500
- All 7 database engines configured
- Demo Acuranzo schema

## Request Format

```json
POST /api/conduit/query
{
  "query_ref": 53,
  "database": "Demo_PG",
  "params": {}
}
```

## Response Format

```json
{
  "success": true,
  "row_count": 5,
  "results": [
    {"theme_key": "...", "theme_value": "..."}
  ]
}
```

## Result Analysis

The test generates per-category pass/total counts:

- `SINGLE_PUBLIC_QUERY_TESTS_PASSED/TOTAL`
- `SINGLE_INVALID_QUERY_TESTS_PASSED/TOTAL`
- `QUERIES_COMPREHENSIVE_TESTS_PASSED/TOTAL`
- `STATUS_PUBLIC_TESTS_PASSED/TOTAL`
- `STATUS_AUTH_TESTS_PASSED/TOTAL`
- `JWT_ACQUISITION_TESTS_PASSED/TOTAL`

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - All conduit endpoints
- [test_52_conduit_auth_query.md](/docs/H/tests/test_52_conduit_auth_query.md) - Authenticated single query