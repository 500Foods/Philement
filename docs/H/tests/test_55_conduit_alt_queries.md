# Test 55: Conduit Alt Multiple Queries Endpoint

## Overview

The [`test_55_conduit_alt_queries.sh`](/elements/001-hydrogen/hydrogen/tests/test_55_conduit_alt_queries.sh) script tests the `/api/conduit/alt_queries` endpoint which allows authenticated batch queries with database override capability.

## Purpose

This test validates:

- Cross-database batch query execution
- JWT token validation while querying different databases
- Query deduplication in batch requests
- Rate limiting across database overrides
- DQM statistics reporting

## Test Configuration

- **Test Name**: Conduit Alt Queries
- **Test Abbreviation**: CFM
- **Test Number**: 55
- **Version**: 1.1.1

## Key Features

### Cross-Database Testing Matrix

Tests a 7x2 matrix where each database's JWT token is used to query two other databases. Uses different query combinations than test_54 (offset patterns 3,4 vs 1,2).

### Batch Query Format

```json
POST /api/conduit/alt_queries
{
  "queries": [
    {"query_ref": 30, "database": "Demo_MY", "params": {}},
    {"query_ref": 45, "database": "Demo_CR", "params": {"STRING": {"TOP": "1"}}
  ]
}
```

Each query in the batch can target a different database via the `database` override field.

## Test Flow

1. **Startup**: Launch Hydrogen with unified conduit configuration (port 5550)
2. **Migration Wait**: Wait for all database migrations to complete
3. **JWT Acquisition**: Obtain tokens for all ready databases
4. **Cross-Batch Tests**: Execute alt batch queries using cross-database matrix
5. **Shutdown**: Graceful server termination

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle, JWT acquisition, request validation
- **`framework.sh`**: Test framework

## Configuration File

**`hydrogen_test_55_conduit_alt_queries.json`**:

- Port: 5550
- All 7 database engines configured

## Skipped Scenarios

If no JWT tokens available:

- Test logs `ALT_MULTIPLE_QUERIES_SKIPPED_NO_TOKEN` to result file
- Test reports as passed (graceful skip)

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_54_conduit_alt_query.md](/docs/H/tests/test_54_conduit_alt_query.md) - Single alt query tests
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Main conduit tests