# Conduit Service - Action Plan

## Current Status

**Endpoints** (6 total):

- ✅ `query` - Single query execution
- ✅ `queries` - Multiple parallel queries (implemented but not registered)
- ✅ `auth_query` - Authenticated single query
- ✅ `auth_queries` - Authenticated multiple queries
- ❌ `alt_query` - Authenticated single query with database override
- ❌ `alt_queries` - Authenticated multiple queries with database override

**Core Features**:

- Named parameters (`:userId`) → database-specific formats (`$1`, `?`)
- Synchronous execution with timeout and webserver resource suspension
- Intelligent queue selection with DQM statistics tracking
- Brotli compression for cached query results
- JWT authentication for auth/alt endpoints

**Next Actions**:

1. Register `/api/conduit/queries` endpoint in `api_service.c`
2. Implement `alt_query` and `alt_queries` endpoints
3. Add DQM statistics to all responses
4. Complete test_51_conduit.sh validation

## Conduit Implementation Checklist

### Phase 1: Query Table Cache (QTC) Foundation ✅ **COMPLETED**

- [x] **Create QTC structures**: Implement `QueryCacheEntry` and `QueryTableCache` in `src/database/database_cache.h` with query_ref, sql_template, description, queue_type, timeout_seconds, last_used, and usage_count fields
- [x] **Implement cache management functions**: Create `query_cache_create()`, `query_cache_add_entry()`, `query_cache_lookup()`, `query_cache_update_usage()`, and `query_cache_destroy()` in `src/database/database_cache.c`
- [x] **Add QTC to DatabaseQueue**: Integrate `query_cache` field into `DatabaseQueue` structure in `src/database/dbqueue/dbqueue.h`
- [x] **Bootstrap QTC loading**: Modify `database_queue_execute_bootstrap_query()` in `src/database/database_bootstrap.c` to populate QTC from bootstrap query results (query_ref, code, name, query_queue_lu_58, query_timeout)
- [x] **Add QTC unit tests**: Create comprehensive tests in `tests/unity/src/database/database_cache_test.c` for all cache operations
- [x] **Verify QTC integration**: Run `mkt` to ensure compilation and `mku database_cache_test` to validate functionality

### Phase 2: Enhanced Queue Selection ✅ **COMPLETED**

- [x] **Add last_request_time field**: Extend `DatabaseQueue` structure in `src/database/dbqueue/dbqueue.h` with `volatile time_t last_request_time` for LRU-style selection
- [x] **Implement selection algorithm**: Create `select_optimal_queue()` in `src/database/database_queue_select.c` that filters by database name, prefers recommended queue type, selects minimum depth queue, and uses earliest `last_request_time` as tie-breaker
- [x] **Update queue submission**: Modify `database_queue_submit_query()` in `src/database/dbqueue/submit.c` to atomically update `last_request_time` when queries are submitted
- [x] **Initialize timestamps**: Set `last_request_time` to queue creation time in queue initialization functions
- [x] **Add selection unit tests**: Create comprehensive tests in `tests/unity/src/database/database_queue_select_test.c` covering all selection criteria and edge cases
- [x] **Verify queue selection**: Run `mkt` for compilation and `mku database_queue_select_test` for validation

### Phase 3: Parameter Processing ✅ **COMPLETED**

- [x] **Create parameter structures**: Implement `ParameterType` enum and `TypedParameter` union in `src/database/database_params.h` supporting INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP
- [x] **Implement parsing functions**: Create `parse_typed_parameters()` in `src/database/database_params.c` to parse JSON parameter format into `ParameterList` structure
- [x] **Add named-to-positional conversion**: Implement `convert_named_to_positional()` to replace `:paramName` with database-specific placeholders (`$1` for PostgreSQL, `?` for MySQL/SQLite/DB2)
- [x] **Handle parameter ordering**: Build ordered parameter arrays maintaining the order of appearance in SQL templates for correct binding
- [x] **Add parameter unit tests**: Create comprehensive tests in `tests/unity/src/database/database_params_test.c` for all parameter types and conversion scenarios
- [x] **Verify parameter processing**: Run `mkt` for compilation and `mku database_params_test` for validation

### Phase 4: Synchronous Query Execution ✅ **COMPLETED**

- [x] **Create pending result structures**: Implement `PendingQueryResult` and `PendingResultManager` in `src/database/database_pending.h` with mutex, condition variable, and timeout support
- [x] **Implement wait mechanism**: Create `pending_result_register()`, `pending_result_wait()`, and `pending_result_signal_ready()` functions in `src/database/database_pending.c`
- [x] **Integrate with DQM worker**: Modify DQM worker thread to call `pending_result_signal_ready()` when query execution completes
- [x] **Add timeout handling**: Implement proper condition variable waiting with timeout support and spurious wakeup protection
- [x] **Add cleanup management**: Implement periodic cleanup of expired pending results to prevent memory leaks
- [x] **Add pending result unit tests**: Create comprehensive tests in `tests/unity/src/database/database_pending_test.c` for wait, signal, and timeout scenarios
- [x] **Verify synchronous execution**: Run `mkt` for compilation and `mku database_pending_test` for validation

### Phase 5: Conduit API Endpoints - **PARTIALLY COMPLETE**

- [x] **Create Conduit directory structure**: Set up `src/api/conduit/` with `conduit_service.h`, `conduit_service.c`, and subdirectories for each endpoint
- [x] **Implement `/api/conduit/query` endpoint**: Create `query/query.h` and `query/query.c` with Swagger documentation, JSON parsing, parameter processing, queue selection, and synchronous execution
- [x] **Register `/api/conduit/query`**: Add endpoint registration in `src/api/api_service.c` and update logging
- [x] **Implement `/api/conduit/auth_query` endpoint**: Create `auth_query/auth_query.h` and `auth_query/auth_query.c` with JWT validation, database extraction from claims, and authenticated query execution
- [x] **Implement `/api/conduit/auth_queries` endpoint**: Create `auth_queries/auth_queries.h` and `auth_queries/auth_queries.c` for parallel authenticated queries with JWT validation
- [ ] **Register `/api/conduit/queries` endpoint**: Add endpoint registration in `src/api/api_service.c` for the existing multiple query implementation
- [ ] **Implement `/api/conduit/alt_query` endpoint**: Create `alt_query/alt_query.h` and `alt_query/alt_query.c` with JWT validation and database override from request body parameter
- [ ] **Implement `/api/conduit/alt_queries` endpoint**: Create `alt_queries/alt_queries.h` and `alt_queries/alt_queries.c` for parallel queries with database override
- [ ] **Add DQM statistics tracking**: Extend `DatabaseQueueManager` with comprehensive statistics tracking using structure:

  ```c
  typedef struct DQMStatistics {
      volatile unsigned long long queue_selection_counters[5];  // Indexed by QUEUE_TYPE_*
      volatile unsigned long long total_queries_submitted;
      volatile unsigned long long total_queries_completed;
      volatile unsigned long long total_queries_failed;
      volatile unsigned long long total_timeouts;
      struct {
          volatile unsigned long long submitted;
          volatile unsigned long long completed;
          volatile unsigned long long failed;
          volatile unsigned long long avg_execution_time_ms;
          volatile time_t last_used;
      } per_queue_stats[5];
  } DQMStatistics;
  ```

- [ ] **Include DQM stats in responses**: Add DQM statistics to all endpoint JSON responses showing queue distribution and performance metrics
- [ ] **Implement cached query loading**: After QTC bootstrap, load queries marked as "cache" type and compress results with Brotli for improved response times
- [ ] **Add Brotli compression utilities**: Create `src/utils/utils_compression.c` with `compress_json_result()` and `decompress_cached_result()` functions for memory-efficient caching

### Phase 6: Webserver Resource Suspension

- [ ] **Implement suspension mechanism**: Add webserver resource suspension for long-running queries in `/api/conduit/queries` and `/api/conduit/auth_queries` endpoints using:

  ```c
  // In endpoint handler after submitting queries
  pthread_mutex_lock(&webserver_suspend_lock);
  webserver_thread_suspended = true;
  MHD_suspend_connection(connection);
  // Wait for all queries to complete
  wait_for_all_pending_results(pending_results_array);
  // Resume connection processing
  MHD_resume_connection(connection);
  webserver_thread_suspended = false;
  pthread_mutex_unlock(&webserver_suspend_lock);
  ```

- [ ] **Add suspension to plural endpoints**: Integrate resource suspension into both `/api/conduit/queries` and `/api/conduit/auth_queries` handlers
- [ ] **Test thread pool protection**: Verify that suspension prevents thread pool exhaustion during batch operations
- [ ] **Validate synchronous semantics**: Ensure clients still receive synchronous API responses despite internal parallel execution

### Phase 7: Comprehensive Testing

- [ ] **Complete test_51_conduit.sh**: Update test script to validate all 6 endpoints across all 7 database engines (PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, YugabyteDB)
- [ ] **Test parameter datatypes**: Validate parameter passing for all supported types (INTEGER, STRING, BOOLEAN, FLOAT, JSON, ARRAY) across all engines
- [ ] **Test JWT authentication**: Verify JWT validation and database extraction for auth endpoints
- [ ] **Test database override**: Validate alt_query and alt_queries endpoints properly override database from JWT claims
- [ ] **Test parallel execution**: Verify queries endpoint executes multiple queries in parallel with proper result aggregation
- [ ] **Test timeout handling**: Validate timeout scenarios for individual queries and batch operations
- [ ] **Test error scenarios**: Verify proper error responses for invalid query_ref, parameter validation failures, and database connection issues
- [ ] **Performance testing**: Run concurrent request tests to validate queue selection and resource suspension effectiveness
- [ ] **Memory leak testing**: Use Valgrind to ensure no memory leaks in parameter processing and result handling
- [ ] **Integration testing**: Run full test suite with `mka` to ensure no regressions in other components

### Phase 8: Final Integration and Documentation

- [ ] **Update Swagger documentation**: Ensure all endpoints have complete OpenAPI specifications with security annotations
- [ ] **Add code documentation**: Update function comments in all conduit implementation files
- [ ] **Create parameter type documentation**: Document all supported parameter types and JSON formats
- [ ] **Update testing documentation**: Document test_51_conduit.sh coverage and validation procedures
- [ ] **Performance documentation**: Document DQM statistics, caching benefits, and resource suspension advantages
- [ ] **Security documentation**: Document JWT validation, database override security, and authentication flows
- [ ] **Run final test suite**: Execute complete test suite including test_51_conduit.sh across all engines
- [ ] **Verify production readiness**: Confirm all endpoints functional, tested, and documented

## Architecture Overview

### Query Table Cache (QTC)

In-memory cache per database, loaded during bootstrap, protected by read-write lock. Contains query_ref, sql_template, description, queue_type, timeout_seconds, last_used, and usage_count fields.

### Parameter Types

Supports INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP with JSON format:

```json
{
  "INTEGER": {"userId": 123, "limit": 50},
  "STRING": {"username": "johndoe", "email": "user@example.com"},
  "BOOLEAN": {"isActive": true, "sendEmail": false},
  "FLOAT": {"price": 19.99, "discount": 0.15},
  "TEXT": {"description": "Large text content"},
  "DATE": {"birth_date": "1990-01-01"},
  "TIME": {"login_time": "14:30:00"},
  "DATETIME": {"created_at": "2023-12-01 10:00:00"},
  "TIMESTAMP": {"updated_at": "2023-12-01 10:00:00.123"}
}
```

### Named Parameter Conversion

Converts `:paramName` to positional parameters at runtime:

- PostgreSQL: `:userId` → `$1`, `:email` → `$2`
- MySQL/SQLite/DB2: `:userId` → `?`, `:email` → `?`

### Queue Selection Algorithm

Selects optimal queue by: filtering by database name → preferring recommended queue type → selecting minimum depth queue → using earliest `last_request_time` as tie-breaker.

### Synchronous Execution with Timeouts

Blocks calling thread on condition variable with timeout. For long-running queries, webserver resources are suspended to prevent thread exhaustion.

### Parallel Query Execution

For plural endpoints (`/api/conduit/queries`, `/api/conduit/auth_queries`), submits individual queries to DQMs for parallel execution with collective timeout and result aggregation.

### DQM Statistics Tracking

Lead DQM maintains comprehensive statistics on query distribution across queue types for operational visibility and performance monitoring.

## Common Implementation Notes

### Bootstrap Query Requirements

Bootstrap query must return exactly: `query_ref` (INTEGER), `code` (TEXT), `name` (TEXT), `query_queue_lu_58` (INTEGER), `query_timeout` (INTEGER). Test manually before implementing parser.

### Named Parameter Parsing

Use regex with word boundaries `:\w+\b` to avoid false matches in strings/comments. Include edge cases in unit tests.

### Thread Safety Considerations

Use atomic operations for `last_request_time` updates. Selection is advisory - minor races acceptable.

### Memory Management

Pair every `malloc` with `free`. Use consistent cleanup patterns. Test with Valgrind (`tests/test_11_leaks_like_a_sieve.sh`).

### Timeout Handling

Check completion flag before timeout flag. Use proper condition variable wait loop with spurious wakeup protection.

### Database-Specific Syntax

Check `DatabaseEngine` type before parameter conversion. PostgreSQL uses `$1`, others use `?`.

## Resuming Work Checklist

- [ ] Read this CONDUIT.md document top to bottom
- [ ] Review current database implementation status
- [ ] Check existing `src/api/conduit/` code
- [ ] Identify current phase progress
- [ ] Run existing tests: `./tests/test_30_database.sh`
- [ ] Verify bootstrap query configuration
- [ ] Check for blocking dependencies

## Implementation Dependencies

**Critical Path**: QTC → Queue Selection → Parameters → Pending Results → API Endpoints

**Parallel Work**: Unit tests, documentation, and Swagger annotations can be developed alongside implementation.

**Integration Points**:

- Phase 1: Bootstrap execution
- Phase 2: Query submission
- Phase 4: DQM worker thread
- Phase 5: API subsystem

## Service Overview

Conduit Service provides synchronous database query execution via REST API. Clients reference pre-defined queries by ID, pass typed JSON parameters, and receive results with intelligent queue selection and timeout handling.

**Key Features**:

- Query Table Cache (QTC) with in-memory query storage
- Typed parameter support (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP)
- Named-to-positional parameter conversion
- Intelligent DQM selection with statistics tracking
- Synchronous execution with configurable timeouts
- JWT authentication for secure endpoints
- Multi-database support across 7 engines
- Parallel batch query execution
- Brotli compression for cached results

See [CONDUIT_DIAGRAMS.md](/docs/H/plans/CONDUIT_DIAGRAMS.md) for detailed architecture diagrams.
