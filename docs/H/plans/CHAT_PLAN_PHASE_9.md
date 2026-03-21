# Chat Service - Phase 9: Context Hashing for Client-Server Optimization

## Objective
Reduce bandwidth usage by implementing context hashing where clients send hashes instead of full context for repeated messages.

## Prerequisites
- Phase 8 completed and verified (existing chat queries updated for hash-based storage)

## Testable Gate
Before proceeding to Phase 10, the following must be verified:
- Context hashing implementation works (SHA-256 of message content)
- Clients can send context_hashes field instead of full messages
- Server reconstructs context from hashes using QueryRef A (Get Segments)
- Bandwidth reduction of 50-90% achieved for long conversations
- Fallback to full content when hashes not found in cache/DB
- Unit tests pass for context hashing functionality
- No regression in existing chat endpoints

## Tasks

### 1. Implement SHA-256 hashing of message content
- Add function to compute SHA-256 hash of JSON message content
- Use existing cryptographic libraries or implement securely
- Ensure consistent hashing (same content always produces same hash)

### 2. Extend chat request format
- Add optional `context_hashes` field to chat requests
- When present, contains array of SHA-256 hashes of previous messages
- Server uses hashes to retrieve segments instead of receiving full content
- Maintain backward compatibility with full message format

### 3. Server-side reconstruction from hashes
- When context_hashes provided, call QueryRef A to get segments
- Decompress and parse each segment to reconstruct conversation
- Validate that reconstructed context matches expected format
- Handle missing hashes gracefully (request full content for those)

### 4. Client-side implementation guidance
- Document how clients should compute and send context hashes
- Recommend sending hashes for all but the newest message
- Explain trade-offs (bandwidth vs. computational cost)
- Provide sample implementation pseudocode

### 5. Cache integration
- Check local LRU disk cache first for segment hashes
- Fallback to database query if not in local cache
- Update access statistics on cache hits
- Populate local cache from database misses

### 6. Performance optimization
- Implement efficient hash lookup (avoid O(n^2) behavior)
- Batch segment retrievals when possible
- Consider predictive prefetching based on conversation patterns

### 7. Unit tests
- Test SHA-256 hash generation consistency
- Test context reconstruction from hashes
- Test fallback behavior when hashes not found
- Test bandwidth savings calculations
- Test integration with local and database caches
- Verify no data loss or corruption in reconstruction

## Verification Steps
1. Verify SHA-256 hashing produces consistent results
2. Test sending context_hashes in chat request instead of full content
3. Confirm server correctly reconstructs context from hashes
4. Measure bandwidth reduction for sample conversations
5. Test fallback to full content when hashes unavailable
6. Verify local cache integration works correctly
7. Run unit tests for context hashing functionality
8. Ensure existing chat endpoints still work with full content
9. Test mixed scenarios (some hashes, some full content)