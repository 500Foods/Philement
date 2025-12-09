# Test 41: Conduit Query

## Overview

Test 41 tests the Conduit Service query endpoint that executes pre-defined database queries by reference. This test launches all 5 database engines from the multi-configuration and validates the `/api/conduit/query` endpoint functionality.

## Purpose

- **Query Execution**: Tests execution of pre-defined SQL queries by reference ID
- **Parameter Handling**: Validates typed parameter processing (INTEGER, STRING, BOOLEAN, FLOAT)
- **Multi-Database**: Tests query execution across all supported database engines
- **Error Handling**: Ensures proper error responses for invalid queries
- **Synchronous Operation**: Validates blocking query execution with timeout

## Test Configuration

- **Test Name**: Conduit Query
- **Test Abbreviation**: QRY
- **Test Number**: 41
- **Version**: 1.0.0

## What It Does

1. **Multi-Database Setup**: Launches all 5 database engines (ACZ, CNV, Helium, HTST) simultaneously
2. **DQM Initialization**: Waits for Database Queue Manager initialization across all engines
3. **Query Testing**: Executes various queries against different databases
4. **Parameter Validation**: Tests different parameter types and combinations
5. **Result Verification**: Validates JSON responses and HTTP status codes

## Database Engines Tested

- **ACZ (Acuranzo)**: PostgreSQL-based application database
- **CNV (Canvas)**: PostgreSQL-based canvas database
- **Helium**: SQLite-based system database
- **HTST (HydroTest)**: PostgreSQL-based test database

## Query Test Cases

### Valid Queries

- **User Profile Query** (1234): Tests user data retrieval with INTEGER parameters
- **Product List Query** (5678): Tests product data with STRING parameters
- **Order History Query** (4321): Tests order data across databases

### Invalid Queries

- **Invalid Query Reference** (9999): Tests error handling for non-existent queries

## API Endpoint

**Endpoint**: `POST /api/conduit/query`

**Request Format**:

```json
{
  "query_ref": 1234,
  "database": "ACZ",
  "params": {
    "INTEGER": {
      "userId": 123,
      "limit": 10
    },
    "STRING": {
      "username": "testuser",
      "status": "active"
    }
  }
}
```

**Success Response**:

```json
{
  "success": true,
  "query_ref": 1234,
  "description": "User Profile Query",
  "rows": [...],
  "row_count": 1,
  "column_count": 4,
  "execution_time_ms": 45,
  "queue_used": "fast"
}
```

## Test Architecture

### Parallel Execution

- **Database Launch**: All 5 databases start simultaneously
- **DQM Monitoring**: Tracks initialization across all engines
- **Query Testing**: Executes tests against running databases
- **Graceful Shutdown**: Proper cleanup after testing

### Configuration

- **Base Config**: Uses `hydrogen_test_41_conduit.json` (derived from test_30_multi.json)
- **Port**: 5341 (to avoid conflicts with other tests)
- **Timeout**: 20 seconds for DQM initialization

## Dependencies

- **Database Engines**: PostgreSQL, SQLite (with appropriate environment variables)
- **Conduit Service**: `/api/conduit/query` endpoint implementation
- **Query Table Cache**: Pre-defined queries loaded during bootstrap
- **DQM System**: Database Queue Manager for query execution

## Test Output

The test provides:

- **Database Status**: Initialization status for each of the 5 databases
- **DQM Launch Count**: Number of queue managers started
- **Query Results**: Success/failure for each test query
- **Performance Metrics**: Response times and execution statistics

## Integration

This test is part of the Hydrogen testing framework and runs automatically with:

```bash
# Run this specific test
./tests/test_41_conduit.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](/docs/H/tests/TESTING.md)
- [Database Tests](/docs/H/tests/test_30_database.md)
- [System Endpoint Tests](/docs/H/tests/test_21_system_endpoints.md)
- [Conduit Service](/docs/H/plans/CONDUIT.md)
- [Database Architecture](/docs/H/core/database.md)