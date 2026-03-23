# Chat Service - Phase 7: Update Existing Chat Queries

**Status: COMPLETE** (2026-03-22)

## Objective
Modify existing chat queries to work with the new content-addressable storage model while maintaining backwards compatibility.

## Prerequisites
- Phase 6 completed and verified (content-addressable storage with Brotli working)

## Testable Gate
Before proceeding to Phase 8, the following must be verified:
- [x] QueryRef #067 (Store Chat with Hashes) created - replaces #036
- [x] QueryRef #068 (Get Chat Reconstruct) created - replaces #041
- [x] QueryRef #069 (Get Chats List with Metrics) created - replaces #039
- [x] QueryRefs D, E, F work correctly for their respective purposes
- [x] Backwards compatibility maintained - legacy conversations still readable
- [x] All existing chat-related tests pass
- [x] No regression in related functionality

## Verification Results (2026-03-22)

| Test | Tests | Pass | Fail | Status |
|------|-------|------|------|--------|
| chat_engine_cache_test | 10 | 10 | 0 | PASS |
| chat_provider_test | 10 | 10 | 0 | PASS |
| chat_storage_test | 17 | 17 | 0 | PASS |
| status_test_handle_conduit_status_request | 5 | 5 | 0 | PASS |
| Compilation (mka) | 18 | 18 | 0 | PASS |
| cppcheck (lint) | 1,267 files | - | 0 | PASS |

**Total: 42 Unity tests passed, 0 failures**

## New QueryRefs Created

| New QueryRef | Migration | Name | Replaces | Purpose |
|--------------|-----------|------|----------|---------|
| #067 | acuranzo_1167.lua | Store Chat (Hash-based) | #036 | Stores chat with segment_refs instead of legacy prompt/response |
| #068 | acuranzo_1168.lua | Get Chat (Reconstruct) | #041 | Reconstructs conversation from hash references |
| #069 | acuranzo_1169.lua | Get Chats List (with Metrics) | #039 | Returns chat list with segment_count and storage metrics |

Note: QueryRefs D, E, F from the original plan were already implemented in Phase 6 as:
- QueryRef #064 (P6D) - Reconstruct Conversation - same as D
- QueryRef #065 (P6E) - Find Conversations by Segment Content - same as E
- QueryRef #066 (P6F) - Get Conversation Statistics - same as F

## Tasks

### 1. Update QueryRef #036 -> #067 (Store Chat) ✅ COMPLETE
- **Migration**: acuranzo_1167.lua
- **Status**: Complete
- Stores segment_refs array instead of legacy prompt/response columns
- Includes engine_name, model, tokens, cost, session_id, etc.

### 2. Update QueryRef #041 -> #068 (Get Chat) ✅ COMPLETE
- **Migration**: acuranzo_1168.lua
- **Status**: Complete
- Reconstructs conversation from hash references in segment_refs
- Uses LATERAL join to fetch segments from convo_segs table
- Returns same format as legacy for backward compatibility

### 3. Update QueryRef #039 -> #069 (Get Chats List) ✅ COMPLETE
- **Migration**: acuranzo_1169.lua
- **Status**: Complete
- Adds segment_count (from segment_refs array length)
- Adds total_uncompressed_bytes, total_compressed_bytes
- Adds avg_compression_ratio
- Maintains backward compatibility with legacy columns

### 4. QueryRefs D, E, F (Already implemented in Phase 6)
- QueryRef #064 (P6D) - Reconstruct Conversation - equivalent to D
- QueryRef #065 (P6E) - Find Conversations by Segment Content - equivalent to E
- QueryRef #066 (P6F) - Get Conversation Statistics - equivalent to F

### 5. Backwards Compatibility Strategy ✅ COMPLETE
- Hybrid read: Check if segment_refs exists, fallback to legacy columns
- During transition period, support both storage types
- Legacy column queries (#036, #039, #041) remain available for existing data

### 6. Unit and Integration Tests ✅ COMPLETE
- Test storing and retrieving chats with new hash-based system
- Verify backwards compatibility with legacy format
- Test reconstruction accuracy (no data loss)
- Test statistics queries return correct values
- Test search by segment content works
- Ensure all existing chat-related tests still pass

### 7. Mark Legacy QueryRefs as Obsolete ✅ COMPLETE
Legacy QueryRefs have been marked as obsolete in the database:
- QueryRef #036 - Store Chat (replaced by #067)
- QueryRef #039 - Get Chats List (replaced by #069)
- QueryRef #041 - Get Chat (replaced by #068)

## Variances from Original Plan

| Item | Original Plan | Actual Implementation | Rationale |
|------|---------------|----------------------|-----------|
| **QueryRef naming** | A, B, C, D, E, F | 062-069 sequential integers | Consistent with project conventions |
| **chat_storage.c** | Stub functions | Fully integrated with QueryRefs | Direct implementation vs placeholder approach |
| **QueryRefs D, E, F** | Created in Phase 7 | Already created in Phase 6 as #064-066 | Completed early during Phase 6 development |
| **Status document** | PENDING during implementation | Marked COMPLETE from start | Document updated as implementation progressed |

## Files Created/Modified

### Helium Migrations
- `elements/002-helium/acuranzo/migrations/acuranzo_1167.lua` - QueryRef #067
- `elements/002-helium/acuranzo/migrations/acuranzo_1168.lua` - QueryRef #068
- `elements/002-helium/acuranzo/migrations/acuranzo_1169.lua` - QueryRef #069

### Hydrogen Source
- `src/api/conduit/chat_common/chat_storage.c` - Integrated with QueryRefs #062, #063, #067, #068
- `src/api/conduit/auth_chat/auth_chat.c` - Updated to use chat_storage functions

## Next Phase Ready

Phase 8 (Context Hashing for Client-Server Optimization) can proceed:
- SHA-256 hashing of message content for client requests
- Context hashes in chat requests to reduce bandwidth
- Server-side reconstruction from hashes
- Target: 50-90% bandwidth reduction for long conversations