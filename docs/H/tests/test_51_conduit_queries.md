# Test 51: Conduit Endpoints

## Overview

Test 51 comprehensively tests all Conduit Service endpoints that execute pre-defined database queries by reference. This test launches all 7 database engines and validates the four conduit endpoints with both public and JWT-authenticated access patterns.

## Purpose

- **Query Execution**: Tests execution of pre-defined SQL queries by reference ID across all parameter types
- **Parameter Handling**: Validates typed parameter processing (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP)
- **Multi-Database**: Tests query execution across all 7 supported database engines
- **Authentication**: Tests both public and JWT-authenticated endpoint access
- **Endpoint Coverage**: Tests all 4 conduit endpoints (single/multiple queries, public/authenticated)
- **Error Handling**: Ensures proper error responses for invalid queries and authentication failures
- **Synchronous Operation**: Validates blocking query execution with timeout

## Test Configuration

- **Test Name**: Conduit Endpoints
- **Test Abbreviation**: CDT
- **Test Number**: 51
- **Version**: 1.1.0

## What It Does

1. **Multi-Database Setup**: Launches all 7 database engines simultaneously
2. **JWT Authentication**: Obtains JWT tokens for authenticated endpoint testing
3. **Endpoint Testing**: Tests all 4 conduit endpoints with various query types
4. **Parameter Validation**: Tests all 9 parameter types with real Acuranzo queries
5. **Result Verification**: Validates JSON responses and HTTP status codes
6. **Authentication Flow**: Tests login → JWT extraction → authenticated queries

## Database Engines Tested

- **PostgreSQL**: Full parameter support with PQexecParams()
- **MySQL**: Full parameter support with prepared statements
- **SQLite**: Full parameter support with prepared statements
- **DB2**: Full parameter support with SQLBindParameter()
- **MariaDB**: Full parameter support with prepared statements
- **CockroachDB**: Full parameter support with prepared statements
- **YugabyteDB**: Full parameter support with prepared statements

## Query Test Cases

### Real Acuranzo Queries Tested

- **Query 001** (System Information): Tests basic system info retrieval
- **Query 008** (Account ID): Tests user account lookup with authentication
- **Query 025** (Query List): Tests system query listing
- **Query 026** (Lookup): Tests lookup table access
- **Query 044** (Filtered Lookup): Tests AI model filtering with complex parameters
- **Query 053** (Themes): Tests theme configuration retrieval

### Endpoint Coverage

#### Public Endpoints (No Authentication Required)

**1. Single Query**: `POST /api/conduit/query`

```json
{
  "query_ref": "001",
  "database": "Acuranzo",
  "params": {
    "INTEGER": { "limit": 10 },
    "STRING": { "status": "active" }
  }
}
```

**2. Multiple Queries**: `POST /api/conduit/queries`

```json
{
  "database": "Acuranzo",
  "queries": [
    {
      "query_ref": "001",
      "params": {}
    },
    {
      "query_ref": "025",
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
```

#### Authenticated Endpoints (JWT Required)

**3. Auth Single Query**: `POST /api/conduit/auth_query`

```json
{
  "query_ref": "008",
  "params": {
    "STRING": { "username": "testuser" }
  }
}
```

**4. Auth Multiple Queries**: `POST /api/conduit/auth_queries`

```json
{
  "queries": [
    {
      "query_ref": "001",
      "params": {}
    },
    {
      "query_ref": "025",
      "params": {
        "STRING": { "status": "active" }
      }
    }
  ]
}
```

### Parameter Types Tested

- **INTEGER**: 64-bit integer values
- **STRING**: Variable-length string values
- **BOOLEAN**: True/false values
- **FLOAT**: Double-precision floating point
- **TEXT**: Large text fields (CLOB/TEXT)
- **DATE**: Date values (YYYY-MM-DD)
- **TIME**: Time values (HH:MM:SS)
- **DATETIME**: Combined date/time (YYYY-MM-DD HH:MM:SS)
- **TIMESTAMP**: Date/time with milliseconds

## Authentication Flow

1. **Login**: Obtains JWT token using demo credentials
2. **Token Extraction**: Parses JWT from login response
3. **Authenticated Requests**: Uses `Authorization: Bearer <token>` header
4. **Token Validation**: Tests both valid and invalid authentication scenarios

## Test Architecture

### Parallel Execution

- **Database Launch**: All 7 databases start simultaneously with different ports (5510-5516)
- **JWT Authentication**: Automatic login flow to obtain tokens for authenticated endpoints
- **Endpoint Testing**: Comprehensive testing of all 4 conduit endpoints
- **Parameter Validation**: Tests all 9 parameter types with real queries
- **Graceful Shutdown**: Proper cleanup after testing

### Configuration

- **Base Configs**: Uses `hydrogen_test_51_*.json` files for each database engine
- **Ports**: 5510-5516 (to avoid conflicts with other tests)
- **Timeout**: 15 seconds for server startup, 10 seconds for shutdown
- **Credentials**: Uses environment variables (HYDROGEN_DEMO_USER_NAME, etc.)

## Dependencies

- **Database Engines**: All 7 supported engines (PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, YugabyteDB)
- **Conduit Service**: All 4 conduit endpoints fully implemented
- **Authentication Service**: JWT token generation and validation
- **Query Table Cache**: Pre-defined Acuranzo queries loaded during bootstrap
- **Parameter Support**: Full typed parameter binding across all engines

## Test Output

The test provides:

- **Database Status**: Initialization status for each of the 7 databases
- **Authentication Status**: JWT token acquisition results
- **Endpoint Results**: Success/failure for each of the 4 conduit endpoints
- **Parameter Validation**: Coverage of all 9 parameter types
- **Performance Metrics**: Response times and execution statistics

## Integration

This test is part of the Hydrogen testing framework and runs automatically with:

```bash
# Run this specific test
./tests/test_51_conduit.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](/docs/H/tests/TESTING.md)
- [Database Tests](/docs/H/tests/test_30_database.md)
- [Authentication Tests](/docs/H/tests/test_40_auth.md)
- [Conduit Service](/docs/H/plans/CONDUIT.md)
- [Database Parameter Support](/docs/H/plans/DATABASE_UPDATE_PLAN.md)
- [Database Architecture](/docs/H/core/subsystems/database/database.md)