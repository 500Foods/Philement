# Chat Service - Phase 9: Local Disk Cache and LRU

## Objective
Add a local disk cache (LRU) for hot segments to avoid repeated decompression and improve performance for active conversations.

## Prerequisites
- Phase 8 completed and verified (context hashing working)

## Testable Gate
Before proceeding to Phase 10, the following must be verified:
- Local disk cache implemented with 1GB limit
- LRU eviction policy working correctly
- Cache stores uncompressed segments (avoiding repeated decompression)
- Background sync from cache to database functioning
- Performance improvement measured for conversations active within last hour
- Unit tests pass for cache operations
- No regression in existing functionality

## Tasks

### 1. Design LRU disk cache
- 1GB storage limit for hot segments
- Store uncompressed segments to avoid repeated decompression
- Use LRU (Least Recently Used) eviction policy
- Persist cache across restarts (metadata tracking)

### 2. Implement cache storage structure
- Cache directory structure organized by hash prefixes
- Metadata file tracking access times and LRU ordering
- Segment files stored as uncompressed JSON for fast access
- Reference counting for shared segments

### 3. Cache integration points
- Check local cache before database for segment retrieval
- Store newly created segments in both cache and database
- Update access time on cache hits
- Background thread to sync dirty cache entries to database

### 4. LRU eviction implementation
- Track access timestamps for all cached segments
- When cache exceeds limit, evict least recently used segments
- Remove evicted segments from cache storage and metadata
- Log evictions for monitoring and tuning

### 5. Background synchronization
- Periodically flush dirty cache entries to database
- Handle cache-database consistency
- Recovery mechanism for cache corruption
- Performance tuning for sync frequency

### 6. Performance monitoring
- Track cache hit/miss ratios
- Measure decompression time savings
- Monitor cache size and eviction rates
- Log performance metrics for optimization

### 7. Unit tests
- Test LRU eviction under various access patterns
- Verify cache-database consistency
- Test background sync functionality
- Measure performance improvement scenarios
- Test cache recovery after unexpected shutdown

## Verification Steps
1. Verify cache directory structure and metadata files created
2. Test storing and retrieving segments from local cache
3. Confirm LRU eviction works when cache limit exceeded
4. Measure performance improvement for repeated segment access
5. Test background sync to database
6. Verify cache recovers correctly after restart
7. Run unit tests for all cache components
8. Ensure existing chat functionality unaffected
9. Test integration with context hashing (Phase 9)