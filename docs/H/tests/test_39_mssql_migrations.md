# Test 39: MSSQL Migration Test

## Overview

Test 39 launches the Hydrogen server with Microsoft SQL Server migration configuration and measures the time required to complete database migration operations. MSSQL is the sixth engine in the Acuranzo migration matrix.

## Purpose

This test validates that SQL Server database migrations execute successfully and provides performance metrics for migration completion time. It runs the full Acuranzo design against SQL Server 2022 Linux via an ODBC 18 connection.

## Test Flow

1. **Binary Validation**: Ensures hydrogen binary is available and executable
2. **Configuration Check**: Validates MSSQL test configuration file
3. **Server Launch**: Starts hydrogen with migration-enabled MSSQL config
4. **Migration Monitoring**: Waits for "Migration test completed in X.XXXs" message
5. **Performance Capture**: Extracts and reports migration completion time
6. **Failure Detection**: Scans server log for migration APPLY/REVERSE/transaction errors
7. **Cleanup**: Gracefully shuts down server

## Configuration

- **Config File**: `tests/configs/hydrogen_test_39_mssql.json`
- **Database**: SQL Server 2022 Linux (ODBC Driver 18) on port 1433
- **Server Port**: 5390
- **Migration Trigger**: `"AutoMigration": true, "TestMigration": false`
- **Schema**: `testms`
- **Bootstrap Query**: Selects from `testms.queries` (schema-qualified)

## Output Format

- **Migration Time**: Displayed in test name as `(cycle: X.XXXs, mig: N)`
- **Server Logs**: Paths to log and result files for debugging
- **Total Runtime**: Complete server execution time from shutdown logs

## Success Criteria

- Hydrogen server starts successfully
- Migration completion message appears within 1800-second timeout
- Server shuts down cleanly
- Migration time is captured and reported
- No migration failures detected in the LOAD/APPLY cycle

## Dependencies

- SQL Server 2022 Linux container running on port 1433 (start via `extras/mssql_server/start.sh`)
- `MSSQL_SA_PASSWORD` environment variable set (must meet SQL Server complexity: 8+ chars, upper/lower/digits/symbols)
- `MSSQL_DB_NAME` environment variable set (defaults to `hydrotst`)
- `MSSQL_DB_HOST` environment variable set (defaults to `127.0.0.1`)
- `MSSQL_DB_PORT` environment variable set (defaults to `1433`)
- `MSSQL_DB_USER` environment variable set (defaults to `sa`)
- Test database created via `extras/mssql_server/create_test_db.sh` (creates `hydrotst` database + `testms` schema)
- Hydrogen binary built and available
- Config file: `tests/configs/hydrogen_test_39_mssql.json`

## Container Setup

Start the SQL Server container before running this test:

```bash
export MSSQL_SA_PASSWORD="your_strong_password_here"
./extras/mssql_server/start.sh
./extras/mssql_server/create_test_db.sh
```

Stop the container after testing:

```bash
./extras/mssql_server/stop.sh
```

## Performance Metrics

- **Migration Time**: Time from startup to migration completion
- **Total Runtime**: Complete server execution time
- **Startup Time**: Time to reach "STARTUP COMPLETE"
- **Reversed Migrations**: Count of successful REVERSE (TestMigration) operations in log

## Error Handling

- **Startup Failure**: Server fails to start within timeout
- **Migration Timeout**: Migration doesn't complete within 1800 seconds
- **Configuration Issues**: Invalid config file or missing dependencies
- **Database Connection**: SQL Server connection failures
- **Migration Failures**: APPLY/REVERSE/transaction errors detected in server log

## Related Tests

- **Test 31**: Migration validation (static SQL generation analysis) — includes mssql
- **Test 32-38**: Migration performance for other database engines
- **Test 40**: Auth endpoint testing across engines (includes MSSQL)
- **[MSSQL_COMPLETE.md](/docs/H/plans/complete/MSSQL_COMPLETE.md)**: Full MSSQL engine plan
