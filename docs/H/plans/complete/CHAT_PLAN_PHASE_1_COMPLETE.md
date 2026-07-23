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
- Audit existing client usage to verify no breakage (see [Internal Query Enforcement](#3-implement-internal-query-enforcement-in-hydrogen))

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

---

## Phase 1 Completion Summary

**Status: COMPLETE** (2026-03-20)

### Completed Tasks

| Task | File(s) | Status |
|------|---------|--------|
| libcurl CMake integration | `cmake/CMakeLists-init.cmake` | ✓ Added to libs, includes, cflags |
| libcurl lifecycle | `src/launch/launch.c`, `src/state/state.c` | ✓ Init on startup, cleanup on shutdown |
| libcurl dependency check | `src/utils/utils_dependency.c`, `tests/test_14_library_dependencies.sh` | ✓ Detected as "Good" (8.15.0) |
| Internal QueryRef #061 | `acuranzo/migrations/acuranzo_1158.lua` | ✓ query_type=0 (internal_sql) |
| Query type enforcement | `src/api/conduit/helpers/database_operations.c` | ✓ Blocks 0 and 1000-1010 |
| Chat config fields | `src/config/config_databases.h`, `config_databases.c` | ✓ Single boolean `Chat` field |

### Variances from Original Plan

1. **Query Type Enforcement Scope** (Task 3)
   - **Original**: Block query_type 0-3
   - **Actual**: Block query_type 0 (internal_sql) AND 1000-1010 (migration queries)
   - **Rationale**: Types 1-3 are "system" queries (auth, UI, core schema) used by REST API. Only type 0 (internal) and migration types need blocking.

2. **Chat Configuration Fields** (Task 4)
   - **Original**: Four fields (`ChatEnabled`, `ChatEngine`, `ChatApiKey`, `ChatDefaultModel`)
   - **Actual**: Single boolean field `Chat`
   - **Rationale**: Simplified design - if `Chat: true`, service queries QueryRef #061 for engine config; if false/missing, endpoints return "service not available"

3. **QueryRef #061 Lookup ID**
   - **Original**: Referenced Lookup 044/045
   - **Actual**: Uses Lookup 038 (AI Models with API keys)
   - **Rationale**: Internal query needs raw collection data with API keys; Lookup 038 stores AI engine configurations

### Verification Results

- ✓ Build completes without errors
- ✓ Test 14 passes (libcurl detected: 8.15.0)
- ✓ QueryRef #061 NOT accessible via REST API (returns "not available/not public")
- ✓ Query type enforcement working as designed
- ✓ No regressions in existing tests

### Next Phase Ready

Phase 2 can proceed: Chat Engine Cache (CEC) implementation with internal-only QueryRef #061 access.