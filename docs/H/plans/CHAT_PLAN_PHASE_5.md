# Chat Service - Phase 5: Additional Provider Support

**Status: COMPLETED** (2026-03-21)

## Objective

Add support for additional AI providers beyond OpenAI-compatible APIs, specifically Anthropic native format and Ollama native API.

## Prerequisites

- Phase 4 completed and verified (cross-database endpoints working)

## Testable Gate

Before proceeding to Phase 6, the following must be verified:

- ✅ Anthropic native format requests are properly built and responses parsed
- ✅ Ollama native API support works (when use_native_api: true)
- ✅ Provider-specific configuration options are correctly processed
- ✅ Unit tests pass for each provider's specific request/response handling
- ✅ Existing OpenAI-compatible provider support remains unaffected

## Completed Tasks

### 1. Anthropic Native Format Support ✅

**Implementation:**

- Enhanced `chat_request_build_anthropic()` to properly format Anthropic native API requests
- System messages are extracted to a separate `system` field (not in messages array)
- Messages array only contains `user` and `assistant` roles
- Added `x-api-key` header support in `chat_proxy.c` (instead of Bearer token)
- Added `anthropic-version: 2023-06-01` header for Anthropic requests
- Response parser already handled Anthropic format (`content[0].text`)

**Files Modified:**

- `src/api/conduit/chat_common/chat_request_builder.c`
- `src/api/conduit/chat_common/chat_proxy.c`

### 2. Ollama Native API Support ✅

**Implementation:**

- Added `use_native_api` flag to `ChatEngineConfig` structure
- Enhanced `chat_request_build_ollama()` to use native Ollama format
- Native format uses `num_predict` in options (instead of `max_tokens`)
- Generic builder switches between native and OpenAI-compatible based on `use_native_api`
- Response parser already handled Ollama format

**Files Modified:**

- `src/api/conduit/chat_common/chat_engine_cache.h`
- `src/api/conduit/chat_common/chat_engine_cache.c`
- `src/api/conduit/chat_common/chat_request_builder.c`

### 3. Provider-Specific Configuration ✅

**Implementation:**

- `use_native_api` field added to engine configuration
- Field is parsed from QueryRef #061 collection JSON (`use_native_api` key)
- Engine limits (`max_images_per_message`, `max_payload_mb`) are validated
- Added image counting for multimodal messages in `chat_request_validate()`

**Files Modified:**

- `src/api/conduit/chat_common/chat_engine_cache.h`
- `src/api/conduit/chat_common/chat_engine_cache.c`
- `src/api/conduit/chat_common/chat_request_builder.c`

### 4. Unit Tests ✅

**Created:** `tests/unity/src/chat/chat_provider_test.c`

Tests implemented:

- OpenAI request building
- Anthropic native format with system extraction
- Ollama native vs OpenAI-compatible modes
- Response parsing for all three providers
- Image count validation
- use_native_api flag functionality

**Test Results:**

```test
10 Tests 0 Failures 0 Ignored
OK
```

## Files Created/Modified

### New Files

- `tests/unity/src/chat/chat_provider_test.c` - Provider-specific unit tests

### Modified Files

```directory
src/api/conduit/chat_common/
├── chat_engine_cache.h        # Added use_native_api field
├── chat_engine_cache.c        # Parse use_native_api from config
├── chat_request_builder.c     # Anthropic native format, Ollama native mode
└── chat_proxy.c               # Provider-specific headers (x-api-key, anthropic-version)

tests/unity/src/chat/
├── chat_engine_cache_test.c   # Updated for new function signature
tests/unity/src/api/conduit/status/
└── status_test_handle_conduit_status_request.c  # Updated for new signature
```

## API Changes

### ChatEngineConfig Structure

Added field:

```c
bool use_native_api;  // Use provider's native API (not OpenAI-compatible)
```

### chat_engine_config_create()

Updated signature:

```c
ChatEngineConfig* chat_engine_config_create(
    int engine_id, const char* name, ChatEngineProvider provider,
    const char* model, const char* api_url, const char* api_key,
    int max_tokens, double temperature_default, bool is_default,
    int liveliness_seconds, int max_images_per_message,
    int max_payload_mb, int max_concurrent_requests,
    bool use_native_api  // NEW PARAMETER
);
```

### QueryRef #061 Collection JSON

New optional field:

```json
{
  "use_native_api": true,  // Enable native API mode for Ollama
  ...
}
```

## Variances from Original Plan

| Item | Original Plan | Actual Implementation | Rationale |
|------|---------------|----------------------|-----------|
| **Task 3: supported_modalities array** | Add array to engine config for supported content types | Not implemented - deferred to Phase 12 (Advanced Multi-modal) | Modalities not yet needed; will be implemented alongside full multimodal support |
| **Task 4: Auto-detection improvements** | Refine auto-detection for provider format | Not explicitly implemented | Provider is explicitly configured via QueryRef #061; auto-detection not required |
| **Task 5: Provider-specific error handling** | Map provider error codes to standard format | Partially implemented - error parsing exists but no provider-specific code mapping | Current error extraction is sufficient; provider-specific codes can be added when needed |
| **Anthropic header version** | Documented as "anthropic-version" | Implemented "anthropic-version: 2023-06-01" | Uses current stable Anthropic API version |
| **Ollama native endpoint** | Plan mentioned `/api/chat` endpoint | Configurable via QueryRef #061 `endpoint` field | Endpoint URL is configuration-driven, not hardcoded |

## Verification Results

- ✅ Build completes without errors (mkt)
- ✅ All 10 new provider tests pass
- ✅ All 10 existing cache tests pass
- ✅ Anthropic native format correctly extracts system messages
- ✅ Anthropic uses `x-api-key` header instead of Bearer
- ✅ Ollama native mode uses `num_predict` in options
- ✅ Ollama OpenAI-compatible mode uses standard format
- ✅ Image count validation enforces `max_images_per_message`
- ✅ Backward compatibility maintained

## Next Phase

After Phase 5 completion, proceed to Phase 6 (Conversation History with Content-Addressable Storage + Brotli):

- conversation_segments table with Brotli compression
- Extended convos table with segment_refs
- Storage and retrieval pipelines
