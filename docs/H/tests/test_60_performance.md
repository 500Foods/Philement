# Test 60: Performance Testing

## Overview

The [`test_60_performance.sh`](/elements/001-hydrogen/hydrogen/tests/test_60_performance.sh) script measures API performance across all 7 database engines, running 5 iterations to assess caching effects and response time consistency.

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
- **Version**: 1.0.0

## Key Features

### Performance Iterations

Runs 5 iterations of the same query sequence to measure:

1. **Warmup effect**: First iteration may be slower due to cache misses
2. **Caching**: Subsequent iterations should show improved performance
3. **Consistency**: Performance should be stable across iterations

### Timing Metrics

Measures elapsed time in milliseconds for:

- JWT acquisition (login endpoint)
- Single queries (QueryRef #53, #54, #55)
- Batched queries (queries #53, #54, #55 in single request)
- Authenticated queries (QueryRef #30)

### Data Collection

- **Response time**: Millisecond precision using `date +%s%N`
- **Data transferred**: Bytes via curl's `size_download`
- **Errors**: Counted per iteration

## Query Sequence Tested

Each iteration executes:

| Query | Endpoint | Description | Auth Required |
|-------|----------|-------------|---------------|
| #53 | `/api/conduit/query` | Get Themes | No |
| #54 | `/api/conduit/query` | Get Icons | No |
| #55 | `/api/conduit/query` | Number Range (params) | No |
| Batch | `/api/conduit/queries` | Queries #53, #54, #55 | No |
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
Database        Run1         Run2         Run3         Run4         Run5         Best
Demo_PG:      2.345s       1.892s       1.456s       1.421s       1.398s    1.398s
Demo_MY:      2.412s       1.934s       1.512s       1.489s       1.467s    1.467s
...
Winner: Demo_SQL with 1.234s
```

Also reports:

- Total data transferred per database
- Data transfer variance across all tests

## Library Dependencies

- **`conduit_utils.sh`**: Server lifecycle (`run_conduit_server`, `shutdown_conduit_server`)
- **`framework.sh`**: Test framework, logging utilities

## Configuration File

**`hydrogen_test_60_performance.json`**:

- Port: 5600
- All 7 database engines configured
- Standard demo Acuranzo schema

## Response Files

Saved to `${DIAG_TEST_DIR}/responses/iter{1-5}/{db_name}/`:

- `login.json` - JWT acquisition response
- `q53_themes.json` - Themes query
- `q54_icons.json` - Icons query
- `q55_numbers.json` - Number range query
- `batch_queries.json` - Batch query response
- `q30_lookups.json` - Authenticated lookup query

## Related Documentation

- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [test_50_conduit_query.md](/docs/H/tests/test_50_conduit_query.md) - Single query tests
- [test_51_conduit_queries.md](/docs/H/tests/test_51_conduit_queries.md) - Comprehensive conduit tests