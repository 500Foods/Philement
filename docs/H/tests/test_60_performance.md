# Test 60: Performance Testing

## Overview

The [`test_60_performance.sh`](/elements/001-hydrogen/hydrogen/tests/test_60_performance.sh) script measures API performance across all 8 database engines, running 5 iterations. The comparison is the median query time of the warm, successful runs.

## Purpose

This test validates:

- Response time performance for database queries
- Caching effectiveness (repeated queries should be faster)
- Data transfer consistency across databases
- Error rate under sustained load
- Comparative performance between database engines

## Test Configuration

- **Test Name**: Performance Test
- **Test Abbreviation**: PRF
- **Test Number**: 60
- **Version**: 1.0.4

## Key Features

### Performance Iterations

Runs 5 iterations of the same query sequence. Iteration 1 is warmup. The median is taken from iterations 2 through 5, and only from runs that returned HTTP 200. A run that returned an error is marked with a star and cannot win. An engine needs two such clean warm runs before it has a median.

### Timing Metrics

Measures elapsed time in milliseconds for:

- Sign-in, reported in the Login column and left out of the comparison
- QueryRef #25, Get Queries List, three times per iteration
- QueryRef #30, Get Lookups List, once per iteration

QueryRef #25 reads every stored query and computes the length of its name, summary, and code. After migration that table is the largest body of text each engine holds, so the timing reflects reading it rather than the cost of opening a connection. Themes, icons, and a short number range mostly timed the HTTP round trip, and the fastest of five samples was often only a few milliseconds apart.

### Data Collection

- **Response time**: Millisecond precision using `date +%s%N`
- **Data transferred**: Bytes via curl's `size_download`
- **Errors**: Counted per iteration

## Query Sequence Tested

Each iteration executes:

| Query | Endpoint | Description | Auth Required |
|-------|----------|-------------|---------------|
| #25 × 3 | `/api/conduit/auth_query` | Get Queries List | Yes (JWT) |
| #30 | `/api/conduit/auth_query` | Lookup List | Yes (JWT) |

## Test Flow

1. **Locate Hydrogen Binary**: Find appropriate build
2. **Validate Environment**: Check demo credentials
3. **Validate Configuration**: Parse performance config
4. **Server Startup**: Launch with unified config (port 5600)
5. **Migration Wait**: Wait for all databases to be ready
6. **5 Iterations**:
   - Acquire JWT tokens for ready databases
   - Execute query sequence
   - Record timing and data transfer
7. **Summary Report**: Display results table
8. **Server Shutdown**: Graceful termination

## Performance Summary Output

The test generates a summary table:

```table
Database        Run1         Run2         Run3         Run4         Run5       Median      Login
Demo_PG:      0.420s       0.310s       0.298s       0.305s       0.301s      0.303s     0.041s
Demo_MY:      0.880s       0.640s       0.655s       0.648s       0.660s      0.652s     0.090s
...
Winner: Demo_PG median 0.303s
```

`Run1` is warmup. `Median` ignores it, and ignores any starred run. `Login` is the median sign-in time.

Also reports:

- Total data transferred per database
- Data transfer variance across all tests

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle (`run_conduit_server`, `shutdown_conduit_server`)
- **`framework.sh`**: Test framework, logging utilities

## Configuration File

**`hydrogen_test_60_performance.json`**:

- Port: 5600
- All 8 database engines configured
- Standard demo Acuranzo schema

## Response Files

Saved to `${DIAG_TEST_DIR}/responses/iter{1-5}/{db_name}/`:

- `login.json` - JWT acquisition response
- `q25_queries_1.json`, `q25_queries_2.json`, `q25_queries_3.json` - Query catalog scans
- `q30_lookups.json` - Authenticated lookup query

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_50_conduit_query.md](/docs/H/tests/test_50_conduit_query.md) - Single query tests
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Comprehensive conduit tests