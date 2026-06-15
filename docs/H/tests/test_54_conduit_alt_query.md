# Test 54: Conduit Alt Single Query Endpoint

## Overview

The [`test_54_conduit_alt_query.sh`](/elements/001-hydrogen/hydrogen/tests/test_54_conduit_alt_query.sh) script tests the `/api/conduit/alt_query` endpoint which allows authenticated queries with database override capability.

## Purpose

This test validates:

- Cross-database query execution using authenticated tokens
- JWT token validation while querying different databases
- Database Queue Manager (DQM) statistics in responses
- 7x2 matrix testing: each database's JWT queries two other databases

## Test Configuration

- **Test Name**: Conduit Alt Query
- **Test Abbreviation**: CF1
- **Test Number**: 54
- **Version**: 1.1.1

## Key Features

### Cross-Database Testing Matrix

Tests a 7x2 matrix where each database's JWT token is used to query two other databases:

| Token Source | Query Targets |
|--------------|---------------|
| PostgreSQL | MySQL, SQLite |
| MySQL | DB2, MariaDB |
| SQLite | CockroachDB, YugabyteDB |
| DB2 | PostgreSQL, MySQL |
| MariaDB | SQLite, CockroachDB |
| CockroachDB | DB2, YugabyteDB |
| YugabyteDB | PostgreSQL, SQLite |

This validates that JWT tokens can be used to query databases different from the one used during login.

## Request Format

```json
POST /api/conduit/alt_query
{
  "query_ref": 30,
  "database": "Demo_MY",  // Override - token may be from Demo_PG
  "params": {}
}
```

The `database` field explicitly specifies which database to query, regardless of JWT source.

## Response Format

Includes DQM statistics:

```json
{
  "success": true,
  "row_count": 15,
  "results": [...],
  "dqm_stats": {
    "queue_name": "Demo_MY",
    "pending_queries": 0,
    "avg_execute_time_ms": 12
  }
}
```

## Test Flow

1. **Startup**: Launch Hydrogen with unified conduit configuration (port 5540)
2. **Migration Wait**: Wait for all database migrations to complete
3. **JWT Acquisition**: Obtain tokens for all ready databases
4. **Cross-Query Tests**: Execute alt queries using cross-database matrix
5. **Shutdown**: Graceful server termination

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle, JWT acquisition, request validation
- **`framework.sh`**: Test framework

## Configuration File

**`hydrogen_test_54_conduit_alt_query.json`**:

- Port: 5540
- All 7 database engines configured

## Skipped Scenarios

If no JWT tokens available:

- Test logs `ALT_SINGLE_QUERY_SKIPPED_NO_TOKEN` to result file
- Test reports as passed (graceful skip)

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_55_conduit_alt_queries.md](/docs/H/tests/test_55_conduit_alt_queries.md) - Batch alt query tests
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Main conduit tests