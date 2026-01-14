# Test 37: CockroachDB Migration Performance Test

## Overview

Test 37 launches the Hydrogen server with CockroachDB migration configuration and measures the time required to complete database migration operations.

## Purpose

This test validates that CockroachDB database migrations execute successfully and provides performance metrics for migration completion time.

## Test Flow

1. **Binary Validation**: Ensures hydrogen binary is available and executable
2. **Configuration Check**: Validates CockroachDB test configuration file
3. **Server Launch**: Starts hydrogen with migration-enabled CockroachDB config
4. **Migration Monitoring**: Waits for "Migration test completed in X.XXXs" message
5. **Performance Capture**: Extracts and reports migration completion time
6. **Cleanup**: Gracefully shuts down server and reports total runtime

## Configuration

- **Config File**: `tests/configs/hydrogen_test_37_cockroachdb.json`
- **Database**: CockroachDB with ACURANZO environment variables
- **Server Port**: 5370
- **Migration Trigger**: `"AutoMigration": true, "TestMigration": true`

## Output Format

- **Migration Time**: Displayed in test name as `(cycle: X.XXXs)`
- **Server Logs**: Paths to log and result files for debugging
- **Total Runtime**: Complete server execution time from shutdown logs

## Success Criteria

- Hydrogen server starts successfully
- Migration completion message appears within 30-second timeout
- Server shuts down cleanly
- Migration time is captured and reported

## Dependencies

- CockroachDB database server running
- ACURANZO environment variables configured
- Hydrogen binary built and available

## Performance Metrics

- **Migration Time**: Time from startup to migration completion
- **Total Runtime**: Complete server execution time
- **Startup Time**: Time to reach "STARTUP COMPLETE"

## Error Handling

- **Startup Failure**: Server fails to start within timeout
- **Migration Timeout**: Migration doesn't complete within 30 seconds
- **Configuration Issues**: Invalid config file or missing dependencies
- **Database Connection**: CockroachDB connection failures

## Related Tests

- **Test 30**: General database connectivity testing
- **Test 31**: Migration validation (static analysis)
- **Test 32-36**: Migration performance for other database engines