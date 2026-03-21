# Chat Service - Phase 2: Chat Engine Cache (CEC) and Proxy Foundation

## Objective

Implement the Chat Engine Cache (CEC), proxy foundation, and core chat components.

## Prerequisites

- Phase 1 completed and verified (libcurl integration, internal query enforcement, config fields)

## Testable Gate

Before proceeding to Phase 3, the following must be verified:

- Chat Engine Cache loads successfully during database bootstrap
- Engine lookup functions work correctly (by name, default engine, list all)
- Chat proxy can successfully make HTTP requests to a test endpoint
- Request builder creates valid JSON for OpenAI-compatible APIs
- Response parser correctly extracts data from OpenAI-compatible responses
- Unit tests for CEC operations, proxy, builder, and parser all pass

## Tasks

### 1. Create ChatEngineConfig and ChatEngineCache structures

- Define data structures in `src/api/conduit/chat_common/chat_engine_cache.h`
- Implement lookup functions mirroring QTC pattern

### 2. Implement CEC bootstrap

- Execute the internal QueryRef during database bootstrap (same flow as QTC)
- Populate cache per database

### 3. CEC refresh mechanism

- Implement periodic refresh (1 hour)
- Implement on-demand admin endpoint for refresh

### 4. Implement chat_proxy.c

- Use libcurl to send HTTP POST requests to external AI APIs
- Handle timing, response collection, and error conditions
- Return structured ChatProxyResult

### 5. Implement chat_request_builder.c

- Build request for OpenAI-compatible APIs (OpenAI, xAI, Gradient, Ollama)
- Support temperature, max_tokens, stream parameters
- Auto-detect format based on provider

### 6. Implement chat_response_parser.c

- Parse OpenAI-compatible responses into standard format
- Extract content, tokens, model, finish_reason
- Handle error responses appropriately

### 7. Token pre-counting

- Implement quick estimation (`strlen/4`) to reject over-limit requests before proxying

### 8. Retry mechanism

- Implement exponential backoff with jitter (1-2 retries on 429/5xx errors)

### 9. Unit tests

- Create test suite for cache operations
- Test proxy with mock server
- Validate request builder output
- Verify response parser accuracy

## Verification Steps

1. Verify CEC loads during database bootstrap (check logs)
2. Test engine lookup by name returns correct configuration
3. Test default engine selection works
4. Test listing all engines returns sanitized list (no API keys)
5. Verify proxy can reach httpbin.org/post or similar test endpoint
6. Confirm request builder creates valid OpenAI-compatible JSON
7. Validate parser extracts correct fields from sample OpenAI response
8. Run unit tests for all new components
9. Ensure existing functionality remains unaffected

---

## Phase 2 Completion Summary

**Status: COMPLETE** (2026-03-20)

### Completed Tasks

| Task | File(s) | Status |
|------|---------|--------|
| CEC Structures | `chat_engine_cache.h/c` | Thread-safe cache with rwlock, lookup by name/ID/default |
| CEC Bootstrap | `database_bootstrap.c` | Auto-populates from QueryRef #061 on startup |
| CEC Refresh | `chat_engine_cache.c` | Manual refresh + interval checking |
| Chat Proxy | `chat_proxy.h/c` | libcurl HTTP client with retry + exponential backoff |
| Request Builder | `chat_request_builder.h/c` | OpenAI, Anthropic, Ollama format support |
| Response Parser | `chat_response_parser.h/c` | Parses all provider formats, streaming chunk support |
| Token Pre-counting | `chat_request_builder.c` | `strlen/4` estimation with validation |
| Unit Tests | `chat_engine_cache_test.c` | 10 tests, all passing |
| Build Verification | All targets | Compiles without errors |

### Files Created

```directory
src/api/conduit/chat_common/
├── chat_engine_cache.h/c    # CEC data structures & operations
├── chat_proxy.h/c           # HTTP client with retry
├── chat_request_builder.h/c # JSON request building
└── chat_response_parser.h/c # Response parsing

tests/unity/src/chat/
└── chat_engine_cache_test.c # Unit tests (10 tests, all pass)
```

### Integration Complete

- `chat_engine_cache_bootstrap_for_database()` called automatically after QTC population
- Only runs for databases with `"Chat": true` in config
- CEC stored in `DatabaseQueue->chat_engine_cache`
- Refresh functions: `chat_engine_cache_refresh()`, `chat_engine_cache_should_refresh()`

### Test Results

```test
10 Tests 0 Failures 0 Ignored
OK
```

### Next Phase Ready

Phase 2.5 can proceed: Chat Status and Monitoring with Liveliness configuration