# Chat Service - Phase 6: Conversation History with Content-Addressable Storage + Brotli

## Objective
Implement content-addressable storage for conversation history with Brotli compression to drastically reduce storage requirements.

## Prerequisites
- Phase 5 completed and verified (additional provider support working)

## Testable Gate
Before proceeding to Phase 7, the following must be verified:
- conversation_segments table created with Brotli-compressed storage
- convos table extended with segment_refs and usage tracking columns
- Brotli compression/decompression wrappers functional using existing utils
- Each message stored as compressed segment with SHA-256 hash
- Conversation turns stored as arrays of hash references in convos.segment_refs
- Usage tracking columns (tokens, cost, duration, session_id) working
- Unit tests for compression, hashing, and storage all pass
- Existing chat functionality continues to work (backwards compatibility)

## Tasks

### 1. Create conversation_segments table (Helium migration)
- Table with segment_hash (SHA-256) as PRIMARY KEY
- Brotli-compressed segment_content BLOB
- uncompressed_size and compressed_size tracking
- compression_ratio, created_at, last_accessed, access_count fields
- Indexes on last_accessed and created_at for cache efficiency

### 2. Extend convos table (Helium migration)
- Add segment_refs JSONB column for hash references
- Add engine_name, model columns
- Add tokens_prompt, tokens_completion, cost_total tracking
- Add session_id, user_id, duration_ms for analytics
- Maintain backwards compatibility with legacy columns during transition

### 3. Add Brotli compression/decompression wrappers
- Wrapper functions around existing src/utils/utils_compression.c
- Functions for compressing JSON message to Brotli blob
- Functions for decompressing Brotli blob back to JSON
- Proper error handling and resource cleanup

### 4. Implement storage pipeline
- On message receipt: JSON -> SHA-256 Hash -> Brotli Compress -> Store
- Store only hash reference in convos.segment_refs array
- Check for existing hash before storing (content-addressable deduplication)
- Update access_count and last_accessed on retrieval

### 5. Implement retrieval pipeline
- Get hash references from convos.segment_refs
- Fetch compressed segments from conversation_segments
- Decompress each segment to reconstruct full conversation
- Update access metadata on retrieval

### 6. Create new QueryRefs (Helium migrations)
- QueryRef A: Get Conversation Segments by Hash (batch retrieval)
- QueryRef B: Store Conversation Segment (with deduplication)
- QueryRef C: Store Chat with Hashes (updated #036)
- QueryRef D: Reconstruct Conversation (get hashes + metadata)
- QueryRef E: Find Conversations by Segment Content (audit)
- QueryRef F: Get Conversation Storage Statistics

### 7. Unit tests
- Test SHA-256 hash generation and collision resistance
- Test Brotli compression ratios on sample chat messages
- Test storage deduplication (same content returns same hash)
- Test full storage/retrieval pipeline integrity
- Test access count and last_updated tracking
- Test error handling for compression/decompression failures

## Verification Steps
1. Verify database schema migrations applied correctly
2. Test storing a chat message and retrieving identical content
3. Verify deduplication works (same message stored once)
4. Test Brotli compression achieves expected 3-5x ratios
5. Confirm segment_refs stores only hashes, not full content
6. Verify usage tracking (tokens, cost) is recorded correctly
7. Test reconstruction of multi-message conversations
8. Run unit tests for all storage components
9. Verify existing chat endpoints still work (backwards compatibility)
10. Test hybrid read mode (can read both legacy and hash-based storage)