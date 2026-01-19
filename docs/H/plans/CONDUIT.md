# Conduit Service - Action Plan

## Current Status

**Endpoints** (7 total):

- ✅ `query` - Single query execution (POST-only)
- ✅ `queries` - Multiple parallel queries (POST-only) - **FIXED CRASH AND FULLY OPERATIONAL**
- ✅ `auth_query` - Authenticated single query (POST-only)
- ✅ `auth_queries` - Authenticated multiple queries (POST-only)
- ✅ `alt_query` - Authenticated single query with database override (POST-only)
- ✅ `alt_queries` - Authenticated multiple queries with database override (POST-only)
- ✅ `status` - Database readiness status (GET with optional JWT)

**Core Features**:

- Named parameters (`:userId`) → database-specific formats (`$1`, `?`)
- Synchronous execution with timeout and webserver resource suspension
- Intelligent queue selection with DQM statistics tracking (per-database)
- Brotli compression for cached query results
- JWT authentication for auth/alt endpoints
- Optional JWT authentication for status endpoint (conditional response fields with per-database DQM stats)
- POST-only endpoints for all query operations (security and consistency)

**Rate Limiting Implementation** (COMPLETED):

- ✅ Added `MaxQueriesPerRequest` field to `DatabaseConnection` struct (default: 10, range: 1-100)
- ✅ Updated configuration parsing with validation in `config_databases.c`
- ✅ Implemented query deduplication logic in `queries` and `auth_queries` endpoints
- ✅ Added rate limiting validation with HTTP 429 responses for exceeded limits
- ✅ Graceful handling of missing database connections (skips rate limiting instead of failing)

**Testing Infrastructure** (COMPLETED):

- ✅ Created unified `test_50_conduit.json` with all 7 database engines in single server
- ✅ Updated `test_51_conduit.sh` to use single-server approach instead of 7 parallel servers
- ✅ Comprehensive testing across all endpoints and database engines
- ✅ Fixed test output balance: Status endpoint tests now have 1 TEST line and 1 PASS/FAIL line per database
- ✅ Enhanced DQM statistics validation: Test validates presence of DQM statistics in authenticated status responses
- ✅ Improved test logging: Per-queue statistics displayed in Lead→Fast→Medium→Slow→Cache order with proper formatting

**Next Actions**:

1. Fix database connectivity issues preventing server startup ✅
2. Complete comprehensive testing across available database engines
3. Add unit tests for rate limiting and deduplication ✅
4. Performance benchmarking and optimization
5. Fix JWT validation in conduit endpoints (changed validate_jwt calls to pass NULL instead of "unknown")
6. ✅ **Implement comprehensive queries endpoint testing**: Added `test_conduit_queries_comprehensive()` with deduplication, error handling, and cross-database validation
7. Implement remaining query endpoint tests (auth queries, alt queries)
8. Add parameter validation and error handling tests
9. Performance testing and memory leak validation
10. ✅ **Fixed DQM Statistics Timing**: Resolved critical bug in execution time calculation and implemented microsecond-precision tracking
11. ✅ **Fixed Parameter Passing Bug**: Resolved critical issue where conduit endpoints were pre-converting SQL templates instead of letting database engines handle parameter conversion, causing parameterized queries to fail

**Current Focus: Fix /api/conduit/queries Endpoint Crash (2026-01-19)**:

- **Issue**: The `/api/conduit/queries` endpoint crashes with "Application reported internal error" after processing the request
- **Symptoms**: Request parsing succeeds, Brotli compression occurs, but server crashes during response sending
- **Root Cause Analysis**:
  - Swagger example syntax was incorrect (used "postgresql_demo" instead of "Demo_PG", wrong parameter format)
  - **Primary Issue**: Webserver daemon not configured with `MHD_ALLOW_SUSPEND_RESUME` flag, causing crash when `MHD_suspend_connection()` is called
  - Potential crash due to large response size with DQM statistics duplicated in each individual query result
  - Need to debug the crash in `handle_conduit_queries_request()` function
- **Fixes Applied**:
  1. ✅ Update swagger example in `queries.h` to match working syntax from test_51_conduit.sh
  2. ✅ Remove DQM statistics from ALL query endpoints (`/api/conduit/query`, `/api/conduit/queries`, etc.) - DQM statistics are only included in status endpoints
  3. ✅ **Critical Fix**: Added `MHD_ALLOW_SUSPEND_RESUME` flag to webserver daemon initialization in `web_server_core.c`
  4. ✅ **Critical Fix**: Updated queries endpoint to use proper POST buffering mechanism (`api_buffer_post_data`) instead of raw upload_data parsing
  5. ✅ Added comprehensive diagnostic logging to identify crash location
  6. Build successful - ready for testing
- **Testing Results**:
  1. ✅ Endpoint no longer crashes - processes requests successfully
  2. ✅ Multiple queries endpoint working: 7/7 databases passing tests
  3. ✅ Parallel query execution confirmed working
  4. ✅ Connection suspension for long-running queries operational
- **Next Steps**:
  1. ✅ Test completed successfully - queries endpoint fully functional
  2. Update this plan with completion status

**Recent Implementation Progress (2026-01-19)**:

- ✅ **Per-Database DQM Statistics**: Modified status endpoint to show DQM statistics per database instead of global aggregation. Each database now reports its own query metrics under `databases.{database_name}.dqm_statistics`
- ✅ **Fixed DQM Statistics Data Types**: Corrected `avg_execution_time_ms` field from `unsigned long long` to `double` for proper floating-point averaging of query execution times
- ✅ **Enhanced Test Output**: Updated test logging to display "DQM Statistics for {database_name}:" with per-database breakdowns, providing clearer operational visibility
- ✅ **Statistics Recording Architecture**: Implemented per-database statistics tracking with proper initialization and atomic operations for thread-safe counters
- ✅ **API Response Structure**: Status endpoint now provides granular DQM metrics per database while maintaining backward compatibility
- ✅ **Fixed Critical Timing Bug**: Resolved issue where `submitted_at_ns` timestamp was set after query serialization, causing execution time calculations to use 0 as start time. Moved timestamp setting before JSON serialization in `database_queue_submit_query()`
- ✅ **Unified Microsecond Tracking**: Changed internal statistics to track execution times in microseconds (`avg_execution_time_us`) with integer precision, while maintaining millisecond display in test logs for readability
- ✅ **Corrected JSON Serialization**: Updated `avg_execution_time_us` to serialize as integer instead of float for precise microsecond reporting

**Key Technical Insights**:

- **Statistics Granularity**: Per-database DQM statistics provide better operational visibility than global aggregation, allowing monitoring of individual database performance
- **Type Safety**: Fixed floating-point arithmetic issues in average execution time calculations to prevent precision loss
- **Thread Safety**: Maintained atomic operations for statistics counters across concurrent database operations
- **Average Calculation**: Execution time averaging now correctly handles completed queries only, with proper type conversions for millisecond precision
- **Timing Precision**: Microsecond-level execution time tracking with integer precision provides accurate performance measurements
- **Serialization Order Critical**: Setting timestamps before object serialization prevents data corruption in queued operations
- **Parameter Conversion Architecture**: Database engines must handle their own parameter placeholder conversion (:param → $1, ?) rather than pre-conversion at the API layer

## Implementation Insights - Queries Endpoint Crash Fix (2026-01-19)

**Critical Webserver Configuration Bug Fixed**: Discovered and resolved a fundamental issue where the MHD webserver daemon was not configured with `MHD_ALLOW_SUSPEND_RESUME`, causing immediate crashes when the queries endpoint attempted to suspend connections for long-running parallel queries.

**Root Cause Analysis**:

- Queries endpoint uses `MHD_suspend_connection()` to free webserver threads during parallel query execution
- Webserver daemon must be started with `MHD_ALLOW_SUSPEND_RESUME` flag to enable suspend/resume functionality
- Without this flag, MHD throws a fatal error: "Cannot suspend connections without enabling MHD_ALLOW_SUSPEND_RESUME!"
- This caused the server to abort with SIGABRT, generating core dumps

**Solution Implemented**:

- Added `MHD_ALLOW_SUSPEND_RESUME` flag to webserver daemon initialization in `web_server_core.c`
- Updated queries endpoint to use proper POST buffering mechanism (`api_buffer_post_data`) instead of raw upload_data parsing
- Added comprehensive diagnostic logging to identify crash locations during development
- Ensured all MHD connection suspension operations are properly supported

**Additional Fixes Applied**:

- **POST Buffering Architecture**: Fixed queries endpoint to use the same robust POST body buffering as single query endpoint
- **Memory Management**: Corrected use-after-free issues in error handling paths
- **API Consistency**: Ensured all conduit endpoints use consistent request parsing and error handling patterns

**Testing Validation**:

- Queries endpoint now processes multiple parallel queries successfully
- All 7 database engines pass multiple query tests (7/7)
- Connection suspension works correctly for long-running batch operations
- No more crashes or core dumps during query execution

**Key Architectural Lessons**:

1. **Webserver Flags Matter**: MHD daemon configuration flags are critical for advanced features like connection suspension
2. **Consistent Implementation**: All endpoints using similar functionality (POST buffering, connection suspension) should use identical patterns
3. **Diagnostic Logging**: Comprehensive logging at each processing step enables rapid identification of crash locations
4. **Testing Coverage**: Parallel query execution requires thorough testing across all supported database engines

**Future Investigation Items**:

- **Average Execution Time Accuracy**: If averages continue to show as 0.000s, investigate database engine timing measurement. Current implementation relies on `result->execution_time_ms` from database engines, which may not be set correctly in all cases
- **Query Failure Patterns**: With 7 failures out of 61 total queries, investigate why queries are failing and ensure failures don't skew performance metrics
- **Per-Queue Statistics Validation**: Verify that Lead/Slow/Medium/Fast/Cache queue statistics accurately reflect actual query distribution and performance

## Implementation Insights - Queries Endpoint Comprehensive Testing (2026-01-19)

**Deduplication Architecture Decision**: The queries endpoint now returns truly deduplicated results instead of replicating them to match input order. This provides significant bandwidth savings and cleaner API semantics:

- **Input**: `[53, 53, 54]` (3 requests with duplication)
- **Execution**: Only 2 unique queries executed (53, 54)
- **Output**: `[result53, result54]` (2 deduplicated results)
- **Client Usage**: Results indexed by `query_ref`, eliminating need for 1:1 input/output mapping

**Test Output Formatting Evolution**: Enhanced `validate_conduit_request()` to provide richer information for batch operations:

- **Queries Endpoint**: "X results, Y total rows, Z bytes"
- **Single Query Endpoint**: "X rows, Z bytes"
- **File Path**: Separate line for cleaner log readability
- **Null Handling**: Robust jq queries with fallback defaults

**Error Handling Patterns Established**: All conduit endpoints now follow consistent error response patterns:

- **Invalid Query Refs**: HTTP 200 with `success=false` + error details
- **Parameter Errors**: HTTP 200 with `success=false` + validation messages
- **Empty Arrays**: HTTP 200 with `success=false` + descriptive error
- **Rate Limits**: HTTP 429 with rate limit details

**Cross-Engine Testing Importance**: Comprehensive testing across all 7 database engines revealed subtle differences in error handling and parameter processing that were addressed uniformly.

## Next Target: Authenticated Single Query Endpoint

Following the same methodical approach as the queries endpoint:

1. **Create `test_conduit_auth_query_comprehensive()`** with scenarios:
   - Valid authenticated queries with different queryrefs
   - Invalid JWT tokens
   - Expired JWT tokens
   - JWT from wrong database (cross-database access attempts)
   - Missing authentication headers
   - Malformed JWT tokens

2. **Expected Behavior**:
   - Uses JWT database context for query execution
   - Validates JWT before processing queries
   - Returns appropriate error responses for auth failures
   - Maintains same parameter and result formatting as public endpoints

3. **Implementation Path**:
   - Follow existing `query.c` patterns for JWT validation
   - Use `lookup_database_and_query()` with JWT-extracted database
   - Test against authenticated queryrefs (different from public ones)
   - Ensure proper error responses for authentication failures

This approach will establish the foundation for authenticated query operations before moving to authenticated batch queries and cross-database access patterns.

## Implementation Insights - Parameter Passing Fix (2026-01-19)

**Critical Architecture Bug Fixed**: Discovered and resolved a fundamental design flaw where the Conduit API layer was pre-converting SQL parameter placeholders (:param → $1, ?) instead of letting database engines handle their own parameter conversion. This caused parameterized queries to fail because the database engines expected to perform the conversion themselves.

**Root Cause Analysis**:

- Conduit `process_parameters()` function was converting named parameters to positional format
- Converted SQL (with $1, $2, etc.) was stored in `DatabaseQuery.query_template`
- Database engines attempted to re-convert already-converted SQL, leading to parameter binding failures
- PostgreSQL logged "Executing without parameters" because parameter arrays were NULL or empty

**Solution Implemented**:

- Modified `prepare_and_submit_query()` to accept original SQL template instead of converted SQL
- Updated all conduit endpoints (`query`, `queries`, `auth_query`, `auth_queries`, `alt_query`, `alt_queries`) to pass `cache_entry->sql_template` directly
- Database engines now properly handle parameter conversion from :param format to engine-specific placeholders
- Parameter JSON is correctly passed to database engines for proper binding

**Testing Validation**:

- Single query endpoint tests now pass 21/21 across 7 databases
- Invalid query reference tests pass 7/7 across 7 databases
- Parameter parsing and binding now works correctly for all supported datatypes

**Key Architectural Lesson**:

1. **Separation of Concerns**: API layer should handle request parsing and validation, database engines should handle SQL dialect conversion
2. **Parameter Binding**: Database engines expect original SQL with named parameters (:param) and separate parameter data structures
3. **Engine-Specific Conversion**: Each database engine (PostgreSQL, MySQL, SQLite, etc.) handles its own placeholder conversion ($1 vs ?)
4. **Testing Importance**: Parameterized query functionality must be tested end-to-end across all supported database engines

This fix enables proper parameterized query execution, preventing SQL injection vulnerabilities and ensuring type-safe parameter binding across all supported database engines.

## Implementation Insights - Statistics Timing Fixes (2026-01-19)

**Critical Serialization Order Bug**: The most significant issue discovered was that `submitted_at_ns` timestamps were being set **after** query serialization to JSON. This caused enqueued queries to have `submitted_at_ns = 0`, leading to execution time calculations of `completion_us - 0 = completion_us` (a timestamp instead of duration). The fix required moving timestamp setting before JSON serialization in `database_queue_submit_query()`.

**Microsecond Precision Achieved**: The system now tracks execution times with microsecond precision internally using integer arithmetic, providing accurate performance measurements. The API reports `avg_execution_time_us` as integers, while test logs display converted millisecond values for readability.

**Data Type Evolution**: Started with `unsigned long long` for counters, moved to `double` for averaging, then back to `unsigned long long` for integer microsecond precision. This evolution reflects the need for exact timing measurements over floating-point approximations.

**Thread-Safe Statistics**: All statistics updates use atomic operations (`__sync_fetch_and_add`) ensuring accurate metrics across concurrent database operations without performance penalties.

**Key Lessons for Future Development**:

1. **Serialization Order Matters**: Always set timestamps and computed fields before serializing objects to persistent storage
2. **Integer vs Float Precision**: For timing measurements, integer microseconds provide better precision than floating-point milliseconds
3. **API Consistency**: Maintain consistent data types in JSON responses (integers for counts/timing, floats only when necessary)
4. **Test Display Formatting**: Convert internal units for display while keeping raw data intact for programmatic use

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

### Phase 5: Conduit API Endpoints ✅ **COMPLETED**

- [x] **Create Conduit directory structure**: Set up `src/api/conduit/` with `conduit_service.h`, `conduit_service.c`, and subdirectories for each endpoint
- [x] **Implement `/api/conduit/query` endpoint**: Create `query/query.h` and `query/query.c` with Swagger documentation, JSON parsing, parameter processing, queue selection, and synchronous execution
- [x] **Register `/api/conduit/query`**: Add endpoint registration in `src/api/api_service.c` and update logging
- [x] **Implement `/api/conduit/auth_query` endpoint**: Create `auth_query/auth_query.h` and `auth_query/auth_query.c` with JWT validation, database extraction from claims, and authenticated query execution
- [x] **Implement `/api/conduit/auth_queries` endpoint**: Create `auth_queries/auth_queries.h` and `auth_queries/auth_queries.c` for parallel authenticated queries with JWT validation
- [x] **Register `/api/conduit/queries` endpoint**: Add endpoint registration in `src/api/api_service.c` for the existing multiple query implementation
- [x] **Implement `/api/conduit/alt_query` endpoint**: Create `alt_query/alt_query.h` and `alt_query/alt_query.c` with JWT validation and database override from request body parameter
- [x] **Implement `/api/conduit/alt_queries` endpoint**: Create `alt_queries/alt_queries.h` and `alt_queries/alt_queries.c` for parallel queries with database override
- [x] **Add DQM statistics tracking**: Extend `DatabaseQueueManager` with comprehensive statistics tracking using structure:

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

- [x] **Include DQM stats in responses**: Add DQM statistics to all endpoint JSON responses showing queue distribution and performance metrics
- [ ] **Implement cached query loading**: After QTC bootstrap, identify queries with queue_type="cache" and prepopulate compressed results for improved response times (requires extending QueryCacheEntry with compressed_result fields)
- [x] **Add Brotli compression utilities**: Create `src/utils/utils_compression.c` with `compress_json_result()` and `decompress_cached_result()` functions for memory-efficient caching

### Phase 6: Webserver Resource Suspension ✅ **COMPLETED**

### Phase 6.5: Conduit-Specific Rate Limiting ✅ **COMPLETED**

- [x] **Add MaxQueriesPerRequest parameter**: Added `MaxQueriesPerRequest` (default: 10, range: 1-100) to `DatabaseConnection` struct in `config_databases.h`
- [x] **Update config_databases.c**: Implemented parsing and validation for the new rate limit parameter with proper error handling
- [x] **Implement query deduplication**: Added deduplication logic by `query_ref` within each batch request in `queries.c` and `auth_queries.c`
- [x] **Implement rate limiting in endpoints**: Added validation of unique query count against `MaxQueriesPerRequest` limit in both `queries` and `auth_queries` endpoints
- [x] **Add rate limit error responses**: Implemented HTTP 429 responses with appropriate error messages when limits exceeded
- [x] **Graceful degradation**: When database connection lookup fails, rate limiting is skipped instead of failing the request (allows continued operation)
- [x] **Update Swagger documentation**: Document the rate limiting behavior, deduplication, and default limits
- [x] **Add unit tests**: Test rate limiting validation logic including deduplication scenarios

**Note**: Broader API rate limiting (e.g., 60 requests/minute per client) should be implemented at the webserver/API level as a separate feature, not specific to Conduit endpoints.

- [x] **Implement suspension mechanism**: Added webserver resource suspension for long-running queries in `/api/conduit/queries` and `/api/conduit/auth_queries` endpoints using MHD connection suspension
- [x] **Add suspension to plural endpoints**: Integrated resource suspension into both `/api/conduit/queries` and `/api/conduit/auth_queries` handlers with proper mutex protection
- [x] **Test thread pool protection**: Suspension prevents thread pool exhaustion during batch operations by freeing the webserver thread while queries execute
- [x] **Validate synchronous semantics**: Clients receive synchronous API responses despite internal parallel query execution

### Phase 7: Comprehensive Testing

- [x] **Create unified test_50_conduit.json**: Combined all 7 database engines into a single configuration file with unique database names (postgresql_demo, mysql_demo, sqlite_demo, db2_demo, mariadb_demo, cockroachdb_demo, yugabytedb_demo)
- [x] **Update test_51_conduit.sh**: Restructured to use single server with 7 databases instead of 7 parallel servers (version 1.2.0)
- [x] **Add database readiness checking**: Implemented `check_database_readiness()` function that parses server logs to determine which databases have completed migration and are ready for testing
- [x] **Update test functions**: Modified all test functions to skip databases that are not ready, allowing partial testing when some databases fail to initialize
- [x] **Fixed test script issues**: Updated JSON grep pattern to handle spaces, corrected query_ref format from strings to integers
- [x] **Status endpoint working**: `/api/conduit/status` GET endpoint successfully returns database readiness status for all 7 databases
- [x] **Cleaned up test output**: Implemented proper `print_subtest`/`print_result` pairing for accurate test counting, replaced EXEC with TEST lines, added detailed status logging per database
- [x] **JWT acquisition testing**: Individual tests for JWT token acquisition from each of the 7 database engines (7/7 passed)
- [x] **Authenticated status testing**: Individual tests for authenticated status endpoint access showing detailed migration status, cache entries, and per-database DQM statistics (8/8 passed including 1 public + 7 authenticated)
- [x] **Per-database DQM statistics**: Status endpoint now displays "DQM Statistics for {database_name}:" with individual database metrics instead of global aggregation
- [ ] **Restructure test_51_conduit.sh for methodical testing**:
  - [x] **Step 1: Server startup and migration verification** - Launch unified server and verify all 7 databases complete migrations within MIGRATION_TIMEOUT window ✅
  - [x] **Step 2: Basic status endpoint testing** - Test `/api/conduit/status` GET without JWT to verify all databases report ready status ✅
  - [x] **Step 3: JWT acquisition** - Login to each of the 7 databases individually to obtain JWT tokens for authenticated testing ✅
  - [x] **Step 4: Authenticated status testing** - Test `/api/conduit/status` GET with each JWT to verify detailed status responses with migration info, cache entries, and DQM statistics ✅
  - [x] **Step 5: Single query endpoint testing** - Test `/api/conduit/query` POST with specific query_refs across all ready databases:
    - **Public queries** ✅:
      - #53 - Get Themes (no parameters) - **WORKING**: Returns theme data across all 7 databases
      - #54 - Get Icons (no parameters) - **WORKING**: Returns icon data across all 7 databases
    - **Parameterized queries** ✅:
      - Query #55 with integer parameters - **WORKING**: Successfully executes parameterized queries with proper parameter binding
    - **Not Public queries**:
      - #30 - Get Lookups List (no parameters)
      - #45 - Get Lookup (:LOOKUP id is integer param, value 43 - Tours)
    - Focus on getting public queries working first, then add authenticated versions
  - [x] **Step 6: Multiple queries endpoint testing** - Test `/api/conduit/queries` POST with batch requests across all ready databases ✅
  - [x] **Step 6.5: Comprehensive queries testing** - Added `test_conduit_queries_comprehensive()` with deduplication, error handling, and cross-database validation ✅
  - [ ] **Step 7: Authenticated single query testing** - Test `/api/conduit/auth_query` POST using JWT authentication (NEXT TARGET)
  - [ ] **Step 8: Authenticated multiple queries testing** - Test `/api/conduit/auth_queries` POST with batch authenticated requests
  - [ ] **Step 9: Cross-database query testing** - Test `/api/conduit/alt_query` and `/api/conduit/alt_queries` endpoints for database override functionality
  - [ ] **Step 10: Parameter validation and error handling** - Test parameter types, rate limiting, deduplication, and error scenarios
  - [ ] **Step 11: Performance and memory testing** - Validate resource suspension, timeout handling, and memory leak prevention
- [ ] **Fix query_ref JSON format issue**: Test script sends query_ref as string but endpoints expect integer
- [ ] **Test query endpoint**: Request valid query_refs from each database, validate identical results across engines
- [ ] **Test queries endpoint**: Request multiple query_refs simultaneously from each database
- [ ] **Test auth_query endpoint**: Use JWT authentication for single queries
- [ ] **Test auth_queries endpoint**: Use JWT authentication for batch queries
- [ ] **Test alt_query endpoint**: Cross-database access with JWT from one database but explicit database override
- [ ] **Test alt_queries endpoint**: Cross-database batch queries with database override
- [ ] **Validate cross-database functionality**: Ensure alt endpoints can access any database regardless of JWT origin
- [ ] **Test parameter datatypes**: Validate parameter passing for all supported types (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP) across all engines
- [ ] **Test parallel execution**: Verify queries endpoint executes multiple queries in parallel with proper result aggregation
- [ ] **Test timeout handling**: Validate timeout scenarios for individual queries and batch operations
- [ ] **Test error scenarios**: Verify proper error responses for invalid query_ref, parameter validation failures, and database connection issues
- [ ] **Test authorization failures**: Attempt to access non-public queries from public endpoints - should return 401/403
- [ ] **Test invalid query references**: Request non-existent query_refs - should return 404
- [ ] **Test rate limiting**: Submit more than 10 queries per database in a single request - should be rejected with 429
- [ ] **Test query deduplication**: Submit batch with duplicate query_refs - should execute only unique queries and count correctly against limit
- [ ] **Test malformed requests**: Invalid JSON, missing required fields, wrong parameter types
- [ ] **Performance testing**: Run concurrent request tests to validate queue selection and resource suspension effectiveness
- [ ] **Memory leak testing**: Use Valgrind to ensure no memory leaks in parameter processing and result handling
- [ ] **Integration testing**: Run full test suite with `mka` to ensure no regressions in other components

### Phase 8: Final Integration and Documentation

- [x] **Add database readiness endpoint**: Create `/api/conduit/status` endpoint to check which databases are ready for queries (shows migration/bootstrap completion status per database)
- [x] **Implement optional JWT authentication for status endpoint**: Add conditional response fields based on JWT presence (public access for basic status, authenticated access for detailed operational data)
- [x] **Change all conduit endpoints to POST-only**: Convert query endpoints from GET/POST support to POST-only for security and consistency (status remains GET)
- [ ] **Fix database connectivity issues**: Resolve server startup problems preventing comprehensive testing
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

## Implementation Insights

### Recent Implementation Lessons (2026-01-18)

**Endpoint Registration Pattern**: Simple endpoint registration requires 4 modifications to `api_service.c`:

1. Add include for endpoint header
2. Add routing case in `handle_api_request()`
3. Add to protected endpoints list (if JWT required)
4. Add to endpoint logging in `register_api_endpoints()`

**JWT Authentication Patterns**:

- `auth_*` endpoints: Extract database from JWT claims (user's assigned database)
- `alt_*` endpoints: Accept database override in request body (admin cross-database access)
- Both require valid JWT token for authentication

**Code Reuse**: The existing helper functions in `query.h` (`handle_*` functions) provide excellent abstraction for:

- Method validation
- Request parsing
- Database/queue lookups
- Parameter processing
- Query submission and response building

**Build Verification**: Always run `mkt` after C code changes - provides fast feedback with minimal output, greatly reducing token usage for AI-assisted development.

**Test Framework Integration**: When adding new endpoints, update test scripts comprehensively:

- Add test functions for new endpoints
- Call test functions in parallel execution
- Add result analysis for new test types
- Update expected test counts in reporting

**Code Reuse Patterns**: The existing `query.h` helper functions provide excellent abstraction:

- `execute_single_query()` pattern works for both single and multiple query endpoints
- Parameter processing, queue selection, and response building are fully reusable
- Only request parsing and result aggregation differ between single/multiple variants

**Endpoint Registration Consistency**: New endpoint registration follows a precise 4-step pattern:

1. Add header include to `api_service.c`
2. Add routing case in `handle_api_request()`
3. Add to protected endpoints list (if JWT required)
4. Add logging in `register_api_endpoints()`

**Incremental Development**: Breaking complex features into small, testable steps (register endpoint → implement alt_query → implement alt_queries) allows for steady progress and early detection of integration issues.

## Implementation Insights - DQM Statistics (2026-01-18)

**Statistics Architecture Success**: The DQM statistics implementation demonstrates excellent separation of concerns:

- **Atomic Operations**: Using `__sync_fetch_and_add` for thread-safe counters ensures accurate statistics across concurrent operations
- **JSON Serialization**: Centralized `database_queue_manager_get_stats_json()` function provides consistent API responses
- **Minimal Performance Impact**: Statistics tracking adds negligible overhead while providing valuable operational insights

**Type Safety Lessons**: The implementation revealed important type conversion considerations:

- **Unsigned vs Signed**: Using `unsigned long long` for counters prevents overflow but requires explicit casting for JSON serialization
- **Time Types**: `time_t` (signed) to `unsigned long long` conversions need explicit casting to avoid compiler warnings
- **JSON Library Types**: Jansson's `json_int_t` (long long) requires casting from unsigned counters

**API Response Design**: Adding statistics to all endpoints follows a clean pattern:

- **Consistent Field Name**: `"dqm_statistics"` appears in all conduit responses
- **Optional Inclusion**: Statistics are added only when `global_queue_manager` is available
- **Performance Consideration**: JSON generation is lightweight and provides immediate operational value

**Testing Strategy Validation**: The incremental approach proved effective:

- **Build Verification**: `mkt` catches type conversion issues early
- **Unit Test Pattern**: Statistics functions can be easily unit tested in isolation
- **Integration Testing**: End-to-end testing validates the complete statistics pipeline

**Future Extensibility**: The statistics structure provides foundation for:

- **Performance Monitoring**: Average execution times per queue type
- **Load Balancing**: Queue selection counters inform optimization decisions
- **Operational Alerts**: Timeout and failure rate tracking enables proactive monitoring
- **Capacity Planning**: Usage patterns guide infrastructure scaling decisions

## Implementation Insights - Parallel Query Execution (2026-01-18)

**DQM-Limited Parallelization Model**: The endpoint implementation correctly submits all queries to DQMs and waits for results, with the actual parallelization determined by the DQM threading architecture:

- **Endpoint Role**: Submit requests to appropriate DQMs and collect results synchronously
- **DQM Parallelization**: If 10 queries are submitted to 10 different DQMs (different databases/queue types), they execute in parallel
- **Queue Type Constraints**: If all queries target the same "slow" queue type with only one DQM instance, parallelization is limited by that DQM's threading model
- **Resource Efficiency**: Connection suspension ensures webserver threads aren't held during query execution, regardless of DQM parallelization breadth

**Implementation Correctness**: The "submit and wait" pattern properly separates concerns:

- **Endpoint**: Handles HTTP protocol, authentication, parameter validation, and response formatting
- **DQM Layer**: Manages actual database execution with appropriate threading and resource management
- **Result Collection**: Synchronous API semantics maintained despite internal parallel execution

**Performance Characteristics**:

- **Best Case**: Multiple DQMs enable true parallel execution across different databases/queue types
- **Constrained Case**: Single DQM bottleneck limits concurrent execution but still provides resource suspension benefits
- **Always Beneficial**: Webserver thread release prevents thread pool exhaustion during long-running batches

## Implementation Insights - Brotli Compression Utilities (2026-01-18)

**Compression Architecture Success**: The Brotli compression utilities provide efficient memory compression for cached query results:

- **Streaming API**: Using `BrotliEncoderCompressStream()` allows handling of large JSON payloads without loading everything into memory
- **Quality Optimization**: Quality level 6 provides excellent compression ratio (typically 70-80% size reduction) while maintaining fast compression/decompression speeds
- **Error Handling**: Comprehensive error checking with proper cleanup ensures robustness in production environments
- **Memory Safety**: Buffer size estimation prevents overflow while allowing dynamic sizing for varying payload sizes

**Integration Benefits**: The utilities integrate seamlessly with existing infrastructure:

- **Build System**: Leverages existing Brotli libraries already configured in CMake
- **Logging**: Uses SR_API subsystem for consistent logging with other API components
- **Memory Management**: Follows project patterns with caller responsibility for buffer cleanup
- **Type Safety**: Explicit casting prevents compiler warnings while maintaining portability

**Cached Query Identification**: Bootstrap query already identifies cache-type queries via queue_type="cache" field:

- **Existing Infrastructure**: Database bootstrap loads queries with queue_type mapping (0="cache", 1="slow", 2="medium", 3="fast")
- **No Additional Schema Changes**: Cache eligibility determined by existing queue_type field in queries table
- **Prepopulation Ready**: Queries with queue_type="cache" are ideal candidates for compressed result prepopulation

**Cached Query Prepopulation Strategy**: For queries with queue_type="cache", we can prepopulate compressed results immediately after bootstrap:

- **No Parameters Required**: Cache-type queries typically have no runtime parameters, making them ideal for prepopulation
- **Post-Migration Timing**: Execute cached queries after database migrations complete to ensure data consistency
- **Memory Efficiency**: Store compressed results in extended QTC entries, decompressing only when served
- **Startup Performance**: Prepopulation during application startup avoids first-request latency penalties
- **Fallback Mechanism**: If prepopulation fails, queries can still execute normally with on-demand caching

**Implementation Approach**: Extend `QueryCacheEntry` structure with compressed result fields and implement prepopulation in bootstrap phase:

```c
// Extended QueryCacheEntry structure
typedef struct QueryCacheEntry {
    // ... existing fields ...
    unsigned char* compressed_result;  // Brotli-compressed JSON result
    size_t compressed_size;           // Size of compressed data
    time_t cached_at;                 // When result was cached
} QueryCacheEntry;
```

This approach enables instant response times for frequently accessed, parameter-free queries while maintaining full functionality for dynamic queries.

## Implementation Insights - Rate Limiting Implementation (2026-01-18)

**Configuration-Driven Rate Limiting Success**: The rate limiting implementation demonstrates clean separation between configuration and enforcement:

- **Flexible Configuration**: `MaxQueriesPerRequest` parameter allows per-database customization (default: 10, range: 1-100)
- **Validation at Parse Time**: Configuration validation prevents invalid values from entering the system
- **Graceful Degradation**: When database connection lookup fails, rate limiting is skipped rather than failing the request
- **Query Deduplication**: Automatic deduplication by `query_ref` ensures fair counting of unique queries against limits

**Implementation Patterns Established**:

- **Consistent Error Responses**: HTTP 429 responses with clear error messages for rate limit violations
- **Logging Integration**: Uses appropriate log levels (ALERT for warnings, DEBUG for details) following project conventions
- **Memory Safety**: Proper cleanup of allocated arrays and strings in all error paths
- **Performance Considerations**: Deduplication happens before queue submission, minimizing impact on query execution

**Testing Infrastructure Evolution**: The shift from parallel servers to unified single-server testing provides significant benefits:

- **Resource Efficiency**: Single server with 7 databases vs. 7 separate servers reduces memory and CPU usage
- **Simplified Coordination**: No complex parallel process management or synchronization issues
- **Comprehensive Coverage**: Tests all endpoints against all database engines systematically
- **Easier Debugging**: Single log stream and result file instead of coordinating multiple parallel executions

**Key Lessons for Future Development**:

1. **Start with Working Configuration**: Having `test_50_conduit.json` already prepared saved significant implementation time
2. **Graceful Error Handling**: Skipping rate limiting on configuration errors maintains service availability
3. **Incremental Testing**: Unified server approach allows testing rate limiting across all databases simultaneously
4. **Documentation Synchronization**: Keeping CONDUIT.md updated with actual progress prevents confusion about completion status

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

## Implementation Insights - Swagger Documentation Update (2026-01-18)

**Automated Documentation Generation**: The swagger-generate.sh script provides excellent automation for API documentation:

- **Comprehensive Scanning**: Automatically discovers all API endpoints across services and extracts swagger annotations
- **Validation Integration**: Built-in OpenAPI 3.1.0 validation using swagger-cli ensures specification correctness
- **Consistent Formatting**: Standardized annotation format ensures uniform documentation across all endpoints
- **Build Integration**: Documentation generation is part of the standard build process with clear success/failure feedback

**Rate Limiting Documentation**: Successfully documented HTTP 429 responses for both `/api/conduit/queries` and `/api/conduit/auth_queries` endpoints:

- **Error Schema**: Consistent error response format with success flag and descriptive error message
- **Endpoint Descriptions**: Updated descriptions to clearly indicate rate limiting constraints (default: 10 queries per request)
- **API Specification**: OpenAPI spec now accurately reflects runtime behavior and limits

**Documentation Maintenance**: The approach demonstrates effective documentation practices:

- **Code-First Documentation**: API documentation lives with implementation, ensuring synchronization
- **Incremental Updates**: Small, focused documentation changes allow for easy verification and testing
- **Validation Assurance**: Automated validation prevents documentation drift and specification errors

**Future Documentation Work**: This foundation enables systematic completion of remaining documentation tasks:

- **Parameter Documentation**: All supported parameter types (INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP) need detailed examples
- **Security Annotations**: JWT authentication patterns should be consistently documented across all protected endpoints
- **Response Examples**: Comprehensive response examples for success and error cases will improve API usability

## Implementation Insights - Database Readiness Testing (2026-01-18)

**Partial Database Testing Strategy**: The test framework now intelligently handles environments where not all databases are available:

- **Readiness Detection**: `check_database_readiness()` function parses server logs to identify which databases have successfully completed migration and QTC population
- **Graceful Degradation**: Test functions skip databases that are not ready instead of failing the entire test suite
- **Migration Status Parsing**: Detects successful completion messages like "Migration process completed (success) - QTC populated from bootstrap queries"
- **Flexible Testing**: Allows testing against available databases while database connectivity issues are resolved for others

**Test Infrastructure Improvements**: The unified single-server approach provides significant advantages:

- **Resource Efficiency**: Single server with multiple databases vs. multiple parallel servers reduces system resource usage
- **Simplified Coordination**: No complex process synchronization or inter-process communication issues
- **Comprehensive Coverage**: Tests all endpoints systematically against all available database engines
- **Incremental Progress**: Can proceed with testing available databases while fixing connectivity issues for others

**Key Lessons for Robust Testing**:

1. **Environment Independence**: Tests should work regardless of which databases are available in the current environment
2. **Failure Isolation**: Database-specific failures shouldn't prevent testing other working databases
3. **Status Transparency**: Clear logging of which databases are ready/unavailable helps with debugging
4. **Progressive Validation**: Start with basic connectivity tests before running comprehensive endpoint validation

**Migration Completion Detection**: The implementation correctly identifies different migration outcomes:

- **Success Pattern**: `"Migration process completed (success) - QTC populated from bootstrap queries"` marks database as ready
- **Failure Pattern**: `"Migration process completed (failed but continuing) - QTC populated from bootstrap queries"` still marks database as ready (QTC populated)
- **Timeout Handling**: Extended migration timeouts (300 seconds) accommodate slow database initialization
- **Bootstrap Query Validation**: Ensures QTC is populated before allowing query execution

This approach enables reliable testing in development environments where database availability may vary, while maintaining comprehensive coverage when all databases are operational.

## Implementation Insights - Status Endpoint Authentication (2026-01-18)

**Optional JWT Authentication Pattern**: The status endpoint demonstrates an effective pattern for conditional authentication:

- **Public Access First**: Basic monitoring data available without authentication
- **Progressive Disclosure**: Sensitive operational details revealed only to authenticated users
- **JWT Validation**: Uses existing JWT infrastructure without requiring endpoint registration in protected lists
- **Clean API Design**: Single endpoint serves different audiences with appropriate data granularity

**Benefits Achieved**:

- **Monitoring Compatibility**: External monitoring tools can access basic status without JWT complexity
- **Operational Security**: Migration status, cache metrics, and DQM statistics protected behind authentication
- **API Flexibility**: Clients can choose authentication level based on their monitoring needs
- **Future Extensibility**: Pattern can be applied to other endpoints requiring tiered access

## Implementation Insights - POST-Only Endpoint Design (2026-01-18)

**Unified Method Enforcement**: Converting all conduit query endpoints to POST-only provides significant benefits:

- **Security Enhancement**: Eliminates sensitive data exposure in URLs and server logs
- **Type Safety**: JSON request bodies enable better parameter validation and type checking
- **API Consistency**: All data-modifying operations now use POST method
- **Future-Proofing**: Complex parameter structures (nested objects, arrays) are naturally supported

**Implementation Strategy**:

- **Centralized Validation**: Single `validate_http_method()` function in `query.c` affects all endpoints
- **Shared Infrastructure**: All conduit endpoints use common helper functions for method validation
- **Minimal Code Changes**: Method enforcement required changes in only one location
- **Comprehensive Coverage**: All 6 query endpoints updated simultaneously through shared validation

**Lessons for Test 51 Integration**:

- **Endpoint Testing**: Test scripts must be updated to use POST requests with JSON bodies
- **Parameter Handling**: GET query parameter parsing can be removed from test utilities
- **Documentation Updates**: Swagger changes ensure API documentation reflects POST-only design
- **Backward Compatibility**: Consider migration path for any existing GET-based integrations

**Key Architectural Decisions**:

1. **Status remains GET**: Monitoring endpoints typically use GET for caching and simplicity
2. **Query operations are POST**: Data operations with complex parameters use POST for security
3. **Optional authentication**: Status provides public access with enhanced authenticated features
4. **Consistent validation**: Shared validation functions reduce maintenance overhead

## Implementation Insights - Test Development Best Practices (2026-01-19)

**Methodical Testing Approach Success**: The restructured test_51_conduit.sh demonstrates the value of incremental, foundation-first testing:

- **Logical Test Sequencing**: Database readiness verification must precede authentication attempts
- **Individual Component Validation**: Testing each database engine separately provides precise failure isolation
- **JSON Parsing Robustness**: Regex patterns for JSON validation must account for formatting variations (spaces, quotes)
- **Data Type Consistency**: API contracts require strict adherence to expected data types (integers vs strings)
- **Test Output Standardization**: Following TESTING.md guidelines with `print_subtest`/`print_result` pairs enables proper test counting and reporting

**Queue Selection Algorithm Fix**: Fixed critical bug in `select_optimal_queue()` where it strictly required matching `queue_type_hint`, causing failures when queries had `queue_type="cache"` but only `"Slow"` queues were configured. Updated to prefer matching queue types but fall back to Lead DQM when no suitable queue exists.

**Test Script Analysis Bug Fix**: Fixed arithmetic syntax error in `analyze_conduit_test_results()` function caused by grep commands matching multiple similar lines (e.g., `*_PASSED=` patterns). Added `^` anchors to ensure exact line matches for result parsing.

**Key Testing Architecture Decisions**:

1. **Readiness-Gated Authentication**: Only attempt JWT acquisition for databases that have completed migrations
2. **Per-Database Test Granularity**: Individual tests for each database provide better debugging and partial success visibility
3. **Infrastructure Validation First**: Confirm server startup, migrations, and authentication before testing business logic
4. **Clean Test Output**: Proper TEST/PASS formatting with detailed INFO logging for operational transparency
5. **Graceful Degradation**: Tests skip unavailable databases rather than failing entirely

**Future Test Development Guidelines**:

- **Ordering Matters**: Always verify prerequisites (database readiness, service availability) before dependent operations
- **Individual Accountability**: Test each component separately to isolate failures and enable partial success
- **Data Validation**: Ensure test data matches API expectations (types, formats, required fields)
- **Logging Standards**: Use consistent INFO/TEST/PASS formatting for automated result processing
- **Incremental Confidence**: Build testing confidence step-by-step rather than attempting comprehensive validation initially

## Implementation Insights - Status Endpoint Testing (2026-01-19)

**DQM Statistics Validation Success**: The status endpoint testing revealed that DQM statistics are properly included in authenticated responses, providing comprehensive operational visibility:

- **Complete Statistics Coverage**: All conduit endpoints include DQM statistics showing total queries submitted/completed/failed, timeouts, queue selection counters, and per-queue performance metrics
- **Test Output Balance**: Fixed critical issue where status endpoint tests had unbalanced TEST/PASS ratios (1 TEST + 2 PASS lines). Consolidated validation into single balanced test per database
- **Enhanced Logging**: Per-queue statistics now display in operational order (Lead→Fast→Medium→Slow→Cache) with proper capitalization and consistent formatting
- **Time Formatting**: Average execution times formatted as seconds with 3 decimal places (e.g., 0.000s) for better readability

**Operational Visibility Improvements**:

- **Queue Performance Monitoring**: Real-time visibility into which queues are handling what volume of queries
- **Failure Pattern Analysis**: Clear tracking of query failures across different queue types
- **Load Distribution Insights**: Queue selection counters show how the intelligent selection algorithm distributes load
- **Performance Trending**: Average execution time tracking enables performance monitoring and optimization

**Test Infrastructure Lessons**:

1. **Output Balance Matters**: Unbalanced TEST/PASS ratios confuse automated test result processing and reporting
2. **Comprehensive Validation**: Single tests should validate all aspects of functionality (success + detailed content)
3. **Display Consistency**: Standardized formatting and ordering improves operational usability
4. **Statistics Integrity**: DQM statistics provide valuable operational insights when properly exposed and validated

**Future Status Endpoint Enhancements**:

- **Execution Time Accuracy**: Investigate why average execution times show as 0.000s - may indicate timing measurement issues in query processing
- **Queue Health Metrics**: Consider adding queue depth and thread utilization metrics to status responses
- **Historical Trending**: Implement time-windowed statistics for performance analysis
- **Alert Thresholds**: Add configurable thresholds for automated monitoring and alerting

## Implementation Insights - Methodical Query Testing (2026-01-19)

**Incremental Testing Strategy Success**: The restructured Test 51 demonstrates the value of methodical, query-focused testing over comprehensive endpoint coverage:

- **Query-Specific Validation**: Testing individual queries (#53 Get Themes, #54 Get Icons) across all databases provides precise validation of data integrity and performance
- **Framework Function Reuse**: Using existing `format_number()` from framework.sh ensures consistent number formatting (thousands separators) across all tests
- **JSON Response Validation**: Comprehensive validation of response structure (`success`, `rows` array, metadata) ensures API contract compliance
- **Database Consistency**: Identical results across all 7 database engines (PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB, YugabyteDB) validates cross-engine compatibility

**Test Output Enhancement Lessons**:

1. **Formatting Functions**: Use framework-provided `format_number()` instead of inline printf for consistent thousands separators (177,793 vs 177793)
2. **JSON Structure Awareness**: API responses follow consistent pattern: `success` boolean, `rows` array, metadata fields (execution_time_ms, queue_used, etc.)
3. **File Size Reporting**: Including formatted file sizes in test output helps identify payload size issues early
4. **Row Count Validation**: Reporting actual row counts enables quick verification of data completeness

**Query Reference Mapping**: Confirmed query references from migration files:

- **#53 Get Themes**: acuranzo_1146.lua - Returns theme configuration data (8 rows across all databases)
- **#54 Get Icons**: acuranzo_1148.lua - Returns icon definitions (12 rows across all databases)
- **#30 Get Lookups List**: acuranzo_1121.lua - Returns available lookup types
- **#45 Get Lookup**: acuranzo_1117.lua - Returns specific lookup entries (requires :LOOKUPID parameter)

**Performance Insights**:

- **Theme Data**: ~177KB responses with 8 theme records each
- **Icon Data**: ~3KB responses with 12 icon records each
- **Consistent Timing**: Sub-1-second execution times across all engines
- **Memory Efficiency**: Compressed responses indicate successful Brotli implementation

**Future Testing Expansions**:

- **Parameter Validation**: Test queries with required parameters (#45 Get Lookup with :LOOKUPID=43)
- **Error Handling**: Invalid query references should return HTTP 404 (currently getting HTTP 000 - connection issue)
- **Batch Operations**: `/api/conduit/queries` endpoint for multiple simultaneous queries
- **Authentication**: JWT-protected endpoints for sensitive operations

See [CONDUIT_DIAGRAMS.md](/docs/H/plans/CONDUIT_DIAGRAMS.md) for detailed architecture diagrams.
