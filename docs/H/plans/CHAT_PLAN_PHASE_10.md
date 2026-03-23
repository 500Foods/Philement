# Chat Service - Phase 10: Cross-Server Segment Recovery

## Status: ✅ COMPLETE (2026-03-23)

## Objective
Implement mechanisms for recovering missing segments in a shared-nothing server architecture where each server maintains its own local cache but shares a common database backend.

## Architecture Notes
- **Shared Storage**: Database serves as the shared backend between servers
- **Cache Location**: `./cache/` relative to server cwd (configurable via `CHAT_CACHE_DIR` env var)
- **Two-Tier Structure**: `{cache_dir}/{XX}/{XXXXXX}.json` (first 2 hash chars as subfolder)
- **No Topology Awareness**: Servers don't know about each other; all sync through database
- **Server Affinity**: Same user directed to same server (for WebSockets), reducing cross-server segment needs
- **Pre-fetching**: Conservative approach - pre-fetch next segment in conversation when accessing current

## Prerequisites
- Phase 9 completed and verified (local disk cache and LRU working)

## Testable Gate Results
All gates verified:

| Gate | Status | Notes |
|------|--------|-------|
| Cache location changed to `./cache/` | ✅ | Default is `./cache`, override with `CHAT_CACHE_DIR` env var |
| Batch query mechanism (QueryRef #070) | ✅ | New migration `acuranzo_1170.lua` |
| Automatic fallback to database | ✅ | Already existed in Phase 9 |
| Conservative pre-fetching | ✅ | `chat_storage_prefetch_segment()` implemented |
| Unit tests pass | ✅ | 9 tests in `chat_phase10_test.c` |
| No regression | ✅ | Existing tests unaffected |

## Tasks Completed

### 1. Update cache directory configuration ✅
- Changed default cache location from `~/.philement/chat_lru_cache/` to `./cache/`
- Added `CHAT_CACHE_DIR` environment variable for override
- Removed database name from cache path (no topology = single cache)
- New structure: `./cache/{XX}/{XXXXXX}.json`
- **Files modified**: `chat_lru_cache.h`, `chat_lru_cache.c`

### 2. Create QueryRef #070 for batch segment retrieval ✅
- Created migration `acuranzo_1170.lua`
- Accepts comma-separated list of hashes via `:SEGMENT_HASHES` parameter
- Returns all found segments in single query
- Internal query type (query_type = 0) for security

### 3. Implement batch retrieval API ✅
- Added `chat_storage_retrieve_segments_batch()` function
- Accepts array of hashes, returns JSON array of found segments
- Single database round-trip for multiple segments
- Integrates with existing cache layer (checks cache first, populates on DB hit)
- **Files modified**: `chat_storage.h`, `chat_storage.c`

### 4. Implement conservative pre-fetching ✅
- Added `chat_storage_prefetch_segment()` function
- Conservative: ensures segment is in cache without full conversation context
- Skip pre-fetch if already in cache
- **Files modified**: `chat_storage.h`, `chat_storage.c`

### 5. Segment availability monitoring ✅
- Leverages existing `ChatLRUCacheStats` structure
- Cache hits, misses, evictions already tracked
- Hit ratio calculation available

### 6. Unit tests ✅
- Created `tests/unity/src/chat/chat_phase10_test.c` with 9 tests:
  - Cache directory configuration tests (4)
  - Batch retrieval API parameter validation tests (3)
  - Pre-fetch API parameter validation tests (2)

## Verification Steps
1. Verify cache uses `./cache/` directory structure
2. Test QueryRef #070 returns multiple segments in single query
3. Test automatic fallback when segments missing from cache
4. Confirm conservative pre-fetching triggers on segment access
5. Run unit tests for all new functionality
6. Ensure existing chat_storage and LRU cache tests still pass
7. Verify cppcheck passes with no issues

## Files Created

| File | Purpose |
|------|---------|
| `elements/002-helium/acuranzo/migrations/acuranzo_1170.lua` | QueryRef #070 - Batch segment retrieval |
| `tests/unity/src/chat/chat_phase10_test.c` | Unit tests (9 tests) |

## Files Modified

| File | Changes |
|------|---------|
| `src/api/conduit/chat_common/chat_lru_cache.h` | Updated default dir name, added env var constant |
| `src/api/conduit/chat_common/chat_lru_cache.c` | Changed cache dir logic, removed database from path |
| `src/api/conduit/chat_common/chat_storage.h` | Added batch retrieval and pre-fetch function declarations |
| `src/api/conduit/chat_common/chat_storage.c` | Implemented `chat_storage_retrieve_segments_batch()` and `chat_storage_prefetch_segment()` |

## Variances from Original Plan

| Item | Original Plan | Actual Implementation | Rationale |
|------|---------------|----------------------|-----------|
| **Pre-fetching approach** | "Optimistic pre-fetching" with conversation pattern analysis | Conservative approach - just ensure segment is in cache | Simpler implementation, server affinity means patterns are less useful |
| **Multi-server continuity** | "Implement segment replication or shared cache strategy" | Not implemented - database is the shared backend | No server-to-server communication needed; database provides continuity |
| **Cache location** | Not specified in detail | `./cache/` relative to cwd with env var override | Simplifies deployment; no hardcoded user home paths |
| **Topology awareness** | "Ensure segments available regardless of which server" | No topology awareness; servers don't know each other | Simplifies architecture; server affinity (for WebSockets) handles most cases |

## Lessons Learned

### 1. Cache Location Flexibility
Using `CHAT_CACHE_DIR` environment variable allows deployment flexibility without code changes. This is important for:
- Docker containers (can mount volume at any path)
- Development vs production environments
- Testing scenarios with temporary directories

### 2. Batch Retrieval Benefits
Single query for multiple segments reduces database round-trips significantly for long conversations. The batch API:
- Uses PostgreSQL `ANY(string_to_array(...))` for efficient batch lookup
- Returns JSON array directly from QueryRef
- Integrates with cache layer to populate on DB hit

### 3. Conservative Pre-fetching
Simple approach of ensuring segment is in cache (rather than predicting next segment) provides good benefits with minimal complexity. Future enhancement could analyze conversation patterns if needed.

### 4. No Topology Awareness
Removing database name from cache path simplifies the cache structure since servers don't need to know about each other. The cache is now:
- Shared across all databases on the same server
- Two-tier structure: `./cache/{XX}/{hash}.json`
- 256 subdirectories for filesystem distribution

### 5. LSP vs Build System
LSP errors in development environment are false positives - the actual build works because CMake handles include paths. Don't rely on LSP for validation.

### 6. QueryRef Numbering
Phase 10 added QueryRef #070. Current QueryRef range for chat: #061-#070.

## Dependencies for Phase 11

Phase 11 (Streaming Support) will need:
- ✅ Existing chat proxy infrastructure (Phase 2)
- ✅ auth_chat endpoint (Phase 4)
- ✅ Response parser supporting streaming chunks (Phase 2)
- ⚠️ New SSE endpoint registration in `api_service.c`
- ⚠️ libcurl multi interface for streaming (already have libcurl)
- ⚠️ Chunk processing in proxy for real-time forwarding

## How Phase 10 Enables Phase 11

Phase 10 provides infrastructure that Phase 11 will build upon:

| Phase 10 Component | How Phase 11 Uses It |
|--------------------|---------------------|
| `chat_storage_retrieve_segments_batch()` | Efficiently fetch conversation context for streaming requests |
| `chat_lru_cache_get()` | Quick access to recent conversation history during streaming |
| `CHAT_CACHE_DIR` env var | Streaming may need separate temp storage for chunked responses |
| QueryRef #070 | Batch context retrieval before streaming begins |
| Cache statistics | Monitor streaming performance impact on cache |

### Key Integration Points for Phase 11

1. **`chat_proxy.c`**: Needs streaming mode using libcurl multi interface
2. **`auth_chat.c`**: Pattern to follow for SSE endpoint structure
3. **`api_service.c`**: Add streaming endpoint registration
4. **Response format**: Current parser (`chat_response_parser.c`) handles chunks

## Next Phase

After Phase 10 completion, proceed to Phase 11 (Streaming Support):
- POST /api/conduit/auth_chat/stream endpoint
- Server-Sent Events implementation
- Streaming integration with proxy
- Key integration point: `chat_proxy.c` needs streaming mode