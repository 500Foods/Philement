# Test 40: Authentication Endpoints - Multi-Engine Parallel Testing

## Overview

The [`test_40_auth.sh`](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh) script is a comprehensive authentication testing tool within the Hydrogen test suite, focused on validating JWT-based authentication endpoints across multiple database engines in parallel. This test ensures that authentication functionality works correctly with PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, and YugabyteDB databases.

## Purpose

This script validates that the Hydrogen server correctly handles JWT-based authentication operations including login, logout, token renewal, and user registration. It verifies authentication functionality against multiple database engines simultaneously, ensuring consistent behavior across different database platforms.

## Script Details

- **Script Name**: `test_40_auth.sh`
- **Test Name**: Auth (engines: 7)
- **Test Abbreviation**: JWT
- **Version**: 1.5.0
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **Multi-Engine Parallel Testing**: Tests authentication against PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, and YugabyteDB simultaneously
- **JWT Token Management**: Validates JWT token generation, renewal, and invalidation
- **User Registration**: Tests new user registration endpoint
- **Invalid Credentials Handling**: Validates proper rejection of incorrect credentials
- **Environment Variable Integration**: Uses environment variables for demo credentials (aligned with migrations 1144/1145)

### Authentication Endpoints Tested

- **`/api/auth/login`**: Tests user login with username/password, returns JWT token
- **`/api/auth/renew`**: Tests JWT token renewal functionality
- **`/api/auth/logout`**: Tests logout and JWT token invalidation
- **`/api/auth/register`**: Tests new user registration with unique credentials

### Advanced Testing

- **Parallel Execution**: Runs multiple database engine instances simultaneously using job control
- **Configuration Validation**: Verifies all test configuration files exist and are valid
- **Port Management**: Extracts and manages different ports for each database configuration
- **Response Validation**: Validates HTTP status codes and JSON response content
- **Server Lifecycle Management**: Handles startup, testing, and graceful shutdown for each instance
- **Environment Variable Validation**: Ensures required demo credentials are properly configured

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `framework.sh` - Test framework and result tracking
- `file_utils.sh` - File operations and path management
- `lifecycle.sh` - Process lifecycle management
- `network_utils.sh` - Network utilities and port management

### Configuration Files

Test configurations are stored in `configs/` directory:

- **`hydrogen_test_40_postgres.json`**: PostgreSQL configuration with demo schema
- **`hydrogen_test_40_mysql.json`**: MySQL configuration with demo schema
- **`hydrogen_test_40_sqlite.json`**: SQLite configuration with demo schema
- **`hydrogen_test_40_db2.json`**: DB2 configuration with demo schema
- **`hydrogen_test_40_mariadb.json`**: MariaDB configuration with demo schema
- **`hydrogen_test_40_cockroachdb.json`**: CockroachDB configuration with demo schema
- **`hydrogen_test_40_yugabytedb.json`**: YugabyteDB configuration with demo schema

### Environment Variables Required

The test requires these environment variables to be set (matching migration 1144/1145):

- **`HYDROGEN_DEMO_USER_NAME`**: Demo user login name
- **`HYDROGEN_DEMO_USER_PASS`**: Demo user password
- **`HYDROGEN_DEMO_API_KEY`**: API key for authentication requests
- **`HYDROGEN_DEMO_ADMIN_NAME`**: Demo admin username (reserved for future use)
- **`HYDROGEN_DEMO_ADMIN_PASS`**: Demo admin password (reserved for future use)
- **`HYDROGEN_DEMO_EMAIL`**: Demo user email (reserved for future use)

### Test Sequence (Per Database Engine)

Each database engine runs through the following test sequence:

1. **Configuration Validation**: Verifies configuration file exists and is valid
2. **Server Startup**: Launches Hydrogen server with specific database configuration
3. **Migration Wait**: Waits for database migrations to complete (if running) before proceeding with auth tests
4. **Login Test**: Tests successful login with valid credentials, extracts JWT token
5. **Invalid Login Test**: Tests rejection of invalid credentials (expects HTTP 401)
6. **Token Renewal Test**: Tests JWT token renewal using obtained token
7. **Logout Test**: Tests logout and token invalidation
8. **Registration Test**: Tests new user registration with unique credentials
9. **Server Shutdown**: Gracefully stops the server instance

### Response Validation Criteria

- **Login Success**: HTTP 200, valid JWT token returned in response
- **Invalid Login**: HTTP 401, proper error response for incorrect credentials
- **Token Renewal**: HTTP 200, new token returned
- **Logout**: HTTP 200, token invalidated successfully
- **Registration**: HTTP 201, new user created successfully

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_40_auth.sh
```

## Expected Results

### Successful Test Criteria

- All configuration files validated successfully
- Required environment variables are set
- All seven database engine servers start successfully
- Login succeeds with valid credentials for all engines
- Invalid credentials correctly rejected with HTTP 401 for all engines
- JWT token renewal works correctly for all engines
- Logout successfully invalidates tokens for all engines
- User registration creates new users successfully for all engines
- All servers shutdown gracefully

### Key Validation Points

- **Multi-Engine Consistency**: Authentication works identically across all database engines
- **JWT Token Handling**: Tokens are properly generated, renewed, and invalidated
- **Security Validation**: Invalid credentials are properly rejected
- **Database Integration**: Each database engine correctly stores and retrieves authentication data
- **Parallel Execution**: All instances run concurrently without conflicts
- **Configuration Flexibility**: Each engine uses separate ports and configurations

## Troubleshooting

### Common Issues

- **Missing Environment Variables**: Ensure all `HYDROGEN_DEMO_*` environment variables are set
- **Configuration Files Missing**: Verify all four database configuration files exist in `configs/` directory
- **Database Connection Failures**: Check that database engines are running and accessible
- **Port Conflicts**: Each configuration uses a unique port (extracted automatically from configs)
- **Startup Timeouts**: Default 15-second timeout; increase if databases are slow to initialize
- **Demo Schema Issues**: Ensure demo schema exists and migrations have run (1144/1145)

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Log Files**: Separate logs for each database engine (postgres, mysql, sqlite, db2)
- **Result Files**: Individual result files track test progress and outcomes
- **Response Files**: JSON responses saved for each endpoint test
- **Timestamped Output**: All results include timestamps for tracking

## Configuration Requirements

### Database Configuration Common Elements

All configuration files must include:

- **Database Engine**: Specified engine type (postgresql, mysql, sqlite, db2)
- **Demo Schema**: Connection to "demo" database/schema
- **Automigration**: Enabled to ensure schema is up-to-date
- **WebServer Port**: Unique port for each configuration
- **API Endpoints**: Authentication endpoints enabled

### Security Considerations

- Demo credentials are environment-based for security
- JWT tokens are properly invalidated on logout
- Invalid credentials are rejected with appropriate HTTP status codes
- Passwords are hashed and never stored in plain text
- API key validation ensures authorized access

## Parallel Execution Details

The test uses parallel execution patterns similar to [`test_30_database.sh`](/docs/H/tests/test_30_database.md):

- **Job Control**: Limits concurrent jobs to available CPU cores
- **Background Processes**: Each database engine test runs in background
- **Result Collection**: Results are analyzed sequentially after parallel completion
- **Clean Output**: Sequential result processing ensures readable output
- **PID Tracking**: Each server process is tracked for proper cleanup

## Version History

- **1.5.0** (2026-01-18): Added migration completion wait before auth tests to prevent failures when databases are cleared
- **1.4.0** (2026-01-14): Expanded to cover all 7 database engines (PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, YugabyteDB)
- **1.3.0** (2026-01-13): Fixed JWT token passing for renew/logout endpoints
- **1.2.0** (2026-01-10): Updated to use environment variables for demo credentials
- **1.1.0** (2026-01-10): Restructured for parallel execution across multiple database engines
- **1.0.0** (2026-01-10): Initial implementation of auth endpoint tests

## Related Documentation

- [test_00_all.md](/docs/H/tests/test_00_all.md) - Main test suite documentation
- [test_30_database.md](/docs/H/tests/test_30_database.md) - Multi-engine parallel testing pattern
- [test_21_system_endpoints.md](/docs/H/tests/test_21_system_endpoints.md) - System endpoint testing
- [test_22_swagger.md](/docs/H/tests/test_22_swagger.md) - Similar API testing pattern
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Modular library documentation
- [TESTING.md](/docs/H/tests/TESTING.md) - Overall testing framework documentation
- [configuration.md](/docs/H/core/configuration.md) - Configuration file documentation
- [testing.md](/docs/H/core/testing.md) - Overall testing strategy
