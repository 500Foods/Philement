# Test 37: Firebird Migration Test

## Overview

Test 37 launches the Hydrogen server with Firebird migration configuration and measures the time required to complete database migration operations. Firebird replaces CockroachDB as the seventh engine in the Acuranzo migration matrix.

## Purpose

This test validates that Firebird database migrations execute successfully and provides performance metrics for migration completion time. It runs the full Acuranzo design against Firebird SuperServer 4.0.

## Test Flow

1. **Binary Validation**: Ensures hydrogen binary is available and executable
2. **Configuration Check**: Validates Firebird test configuration file
3. **Server Launch**: Starts hydrogen with migration-enabled Firebird config
4. **Migration Monitoring**: Waits for "Migration test completed in X.XXXs" message
5. **Performance Capture**: Extracts and reports migration completion time
6. **Failure Detection**: Scans server log for migration APPLY/REVERSE/transaction errors
7. **Cleanup**: Gracefully shuts down server

## Configuration

- **Config File**: `tests/configs/hydrogen_test_37_firebird.json`
- **Database**: Firebird 4.0 SuperServer on port 3050
- **Server Port**: 5376
- **Migration Trigger**: `"AutoMigration": true, "TestMigration": false`
- **Database File**: `/var/lib/firebird/data/testfb.fdb` (via `${env.FIREBIRD_DB_PATH}`)
- **Schema**: empty (Firebird uses file-based isolation, no schema prefix)

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

- Firebird 4.0 SuperServer running on port 3050 (started via `extras/firebird/start.sh`)
- `FIREBIRD_SYSDBA_PASSWORD` environment variable set
- `FIREBIRD_DB_PATH` environment variable set (defaults to `/var/lib/firebird/data/testfb.fdb`)
- Test database `testfb.fdb` created separately via `extras/firebird/create_test_db.sh` — the test treats the database as pre-existing
- Hydrogen binary built and available
- Config file: `tests/configs/hydrogen_test_37_firebird.json`

## Performance Metrics

- **Migration Time**: Time from startup to migration completion
- **Total Runtime**: Complete server execution time
- **Startup Time**: Time to reach "STARTUP COMPLETE"
- **Reversed Migrations**: Count of successful REVERSE (TestMigration) operations in log

## Error Handling

- **Lifecycle Setup Failure**: Firebird start or database creation fails
- **Startup Failure**: Server fails to start within timeout
- **Migration Timeout**: Migration doesn't complete within 1800 seconds
- **Configuration Issues**: Invalid config file or missing dependencies
- **Database Connection**: Firebird connection failures
- **Migration Failures**: APPLY/REVERSE/transaction errors detected in server log

## Related Tests

- **Test 31**: Migration validation (static SQL generation analysis)
- **Test 32**: PostgreSQL migration performance (reference baseline)
- **Test 40**: Auth endpoint testing across engines (includes Firebird)
- **[FIREBIRD.md](/docs/H/plans/FIREBIRD.md)**: Full Firebird engine plan
