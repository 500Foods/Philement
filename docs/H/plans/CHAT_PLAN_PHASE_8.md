# Chat Service - Phase 8: Update Existing Chat Queries

## Objective
Modify existing chat queries to work with the new content-addressable storage model while maintaining backwards compatibility.

## Prerequisites
- Phase 7 completed and verified (content-addressable storage with Brotli working)

## Testable Gate
Before proceeding to Phase 9, the following must be verified:
- Updated QueryRef #036 (Store Chat) works with hash-based storage
- Updated QueryRef #041 (Get Chat) reconstructs conversations from hash references
- Updated QueryRef #039 (Get Chats List) includes segment count and storage metrics
- New QueryRefs D, E, F work correctly for their respective purposes
- Backwards compatibility maintained - legacy conversations still readable
- All existing chat-related tests pass
- No regression in related functionality

## Tasks

### 1. Update QueryRef #036 (Store Chat)
- Modify to store only hash references in convos.segment_refs instead of full text
- Insert actual segment content via QueryRef B (Store Conversation Segment)
- Store engine_name, model, tokens, cost, session_id, etc. in convos table
- Maintain same interface for callers

### 2. Update QueryRef #041 (Get Chat)
- Major update to return reconstructed conversation from hash references
- Get segment_refs hash array from convos
- Call QueryRef A to get actual segment content
- Decompress and reassemble conversation in correct order
- Return same format as before for backward compatibility

### 3. Update QueryRef #039 (Get Chats List)
- Add segment count (length of segment_refs array)
- Add total storage size calculation (sum of uncompressed sizes)
- Optionally add compression ratio averages
- Maintain existing fields for backward compatibility

### 4. Create New QueryRef D: Reconstruct Conversation
- Get all segment hashes for a conversation
- Return metadata plus instructions to fetch segments
- Used for audit trails and administrative functions

### 5. Create New QueryRef E: Find Conversations by Segment Content
- Accept segment hash (pre-computed from search content)
- Find all convos containing that hash in their segment_refs
- Return convos_id, convos_ref, session_id, created_at
- Useful for content search and compliance

### 6. Create New QueryRef F: Get Conversation Statistics
- Analytics query for storage metrics
- Count total conversations, unique segments
- Calculate total uncompressed/compressed bytes
- Average compression ratio and space saved
- Important for capacity planning and optimization verification

### 7. Backwards Compatibility Strategy
- Hybrid read: Check if segment_refs exists, fallback to legacy columns
- During transition period, support both storage types
- Automatic migration of legacy content optional (background job)
- Clear deprecation path for legacy storage

### 8. Unit and Integration Tests
- Test storing and retrieving chats with new hash-based system
- Verify backwards compatibility with legacy format
- Test reconstruction accuracy (no data loss)
- Test statistics queries return correct values
- Test search by segment content works
- Ensure all existing chat-related tests still pass

## Verification Steps
1. Verify updated QueryRef #036 stores hashes, not full content
2. Test storing a chat and retrieving it via updated #041
3. Confirm reconstructed chat matches original content exactly
4. Verify QueryRef #039 returns segment count and size info
5. Test new QueryRefs D, E, F return expected results
6. Verify backwards compatibility - legacy chats still readable
7. Run all existing chat-related tests (test_23_websockets.sh, etc.)
8. Test hybrid read functionality
9. Verify no regression in related API endpoints