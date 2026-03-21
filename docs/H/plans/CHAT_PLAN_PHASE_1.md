# Chat Service - Phase 1: Prerequisites and Infrastructure

## Objective
Establish foundational infrastructure and dependencies required for chat service implementation.

## Testable Gate
Before proceeding to Phase 2, the following must be verified:
- libcurl is properly integrated and functional in Hydrogen build
- All existing tests still pass (no regressions)
- Library dependencies test passes (test_14_library_dependencies.sh)

## Tasks

### 1. Add libcurl as a core dependency
- Update `cmake/CMakeLists-init.cmake` to add libcurl to `HYDROGEN_BASE_LIBS`
- Add curl initialization (`curl_global_init`/`curl_global_cleanup`) to Hydrogen startup/shutdown
- Add libcurl to `tests/test_14_library_dependencies.sh`

### 2. Create internal CEC bootstrap QueryRef in Helium
- Add new migration with `query_type = 0` (`internal_sql`) for engine retrieval with API keys
- Ensure existing QueryRef #044 (client-facing, strips API keys) remains unchanged

### 3. Implement internal query enforcement in Hydrogen
- Update `lookup_database_and_query()` in `database_operations.c` to block `query_type` 0-3 from conduit access
- Audit existing client usage to verify no breakage (see [Internal Query Enforcement](#internal-query-enforcement-helium--hydrogen))

### 4. Add chat configuration fields to `DatabaseConnection`
- Update `src/config/config_databases.h` with chat-specific fields
- Update `src/config/config_databases.c` parsing logic
- Update `src/config/config_defaults.c` with default values

## Verification Steps
1. Build Hydrogen successfully with new libcurl dependency
2. Run `mkt` alias to verify test build passes
3. Run `mka` alias to verify full build passes
4. Execute `tests/test_14_library_dependencies.sh` to verify libcurl is detected
5. Verify internal query enforcement blocks unauthorized access to query_type 0-3
6. Confirm existing auth service functionality remains unaffected