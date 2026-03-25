# Chat Service - Phase 11: Streaming Support (WebSocket-based)

## Objective
Add streaming response support via WebSocket for real-time chat interactions, leveraging existing libwebsockets infrastructure.

## Prerequisites
- Phase 10 completed and verified (cross-server segment recovery working)
- WebSocket server already running (port 5001, used for terminal)

## Current Status (2026-03-23)
- ✅ Housekeeping: Fixed failing Unity test `chat_lru_cache_test` (12/12 passing)
- ✅ HTTP streaming endpoint removed (was `conduit/auth_chat_stream`) - not needed with WebSocket approach
- ✅ WebSocket chat handler: Implemented with direct streaming writes (no queues)
- ✅ Thread-safety fix: Direct writes from streaming callback with connection_valid guard
- ✅ Terminal subsystem: Direct writes for PTY data (no queue needed)
- ✅ Provider streaming formats researched: OpenAI (SSE), Anthropic (SSE with events), Ollama (NDJSON)
- ✅ Provider-specific streaming parsers: Implemented in `chat_stream_chunk_parse()` with auto-detection
- ⏳ Unit tests for streaming: Not yet written.
- ⏳ Integration tests for WebSocket chat: Not yet written.

## Variances from Original Plan
1. **Switched from HTTP SSE to WebSocket streaming** - Original plan was HTTP Server-Sent Events via microhttpd. Decision made to use WebSocket because:
   - Existing libwebsockets infrastructure already in place (WebSocket server for terminal)
   - Simpler implementation - no need for complex microhttpd streaming callbacks
   - Better performance and bidirectional communication
   - Reduces dependency on microhttpd for streaming

2. **Thread-safety solution** - Original plan was separate thread + pipe. Implemented direct writes with connection validity checks:
    - Data written directly from streaming callback to WebSocket
    - `connection_valid` flag prevents writes to closing connections
    - Simpler than queue-based approach, no synchronization needed
    - Eliminated heap corruption issues caused by queue cleanup race conditions

3. **Provider-specific parsing** - Original plan was to implement separate parsers per provider. Implemented auto-detection within single parser function:
   - Simplifies integration (no provider context needed)
   - Easier to maintain and extend
   - Handles mixed provider scenarios

4. **Terminal subsystem fix** - Added write-queuing for terminal streaming to address thread-safety issue discovered during review. This was not in original Phase 11 plan but was identified as necessary.

## Lessons Learned
1. **Direct writes work**: Writing directly from streaming callbacks (with `connection_valid` guards) is simpler and more reliable than queue-based approaches.
2. **Queue complexity is dangerous**: Queue lifecycle management with refcounting across multiple threads caused heap corruption and double-free crashes.
3. **libwebsockets callback 38**: `LWS_CALLBACK_WS_PEER_INITIATED_CLOSE` (reason 38) fires when client sends close frame - must be handled the same as `LWS_CALLBACK_CLOSED`.
4. **Double-cleanup guard**: Use `connection_valid` flag to prevent cleanup from running twice when both reason 38 and reason 4 fire.
5. **Provider streaming formats differ significantly** - Same as before.

## Testable Gate
Before proceeding to Phase 12, the following must be verified:
- WebSocket chat message type "chat" handles both streaming and non-streaming requests
- JWT authentication per connection (existing WebSocket auth extended)
- Streaming works with all supported AI providers (OpenAI, Anthropic, Ollama)
- Proper JSON message format for requests and responses
- Unit and integration tests pass for WebSocket chat
- No regression in existing WebSocket terminal or HTTP chat endpoints

## Tasks

### 1. Extend WebSocket message handling ✅
- Added new message type "chat" to `handle_message_type()` in `websocket_server_message.c`
- Created `handle_chat_message()` function to process chat requests
- Reused existing request parsing from `auth_chat_parse_request()`

### 2. JWT authentication per connection ✅
- JWT validation per message (database stored in session)
- Extract database from JWT claims (reuse `auth_jwt_helper.h`)
- Store authenticated database in session data

### 3. Chat streaming via WebSocket ✅
- Used existing `chat_proxy_send_stream()` for backend streaming
- Send chunks as "chat_chunk" JSON messages via `ws_write_json_response()`
- Send final "chat_done" message with complete response
- Handle non-streaming mode (single "chat_done" response)

### 4. Message format definition ✅
- JSON schema defined for "chat" request (similar to HTTP auth_chat)
- "chat_chunk" response format defined (content delta, model, index)
- "chat_done" response format defined (full content, tokens, finish_reason)
- "chat_error" response format defined

### 5. Integration with existing chat infrastructure ✅
- Reuse `chat_engine_cache` for engine lookup
- Reuse `chat_metrics` for timing and token counting
- Reuse `chat_storage` for conversation persistence (after stream ends)
- Reuse `chat_context_hashing` for optimization

### 6. Error handling in WebSocket chat ✅
- Send "chat_error" messages on failures
- Handle client disconnections gracefully
- Clean up proxy resources when stream ends
- Log errors appropriately

### 7. Thread-safety fix for chat streaming ✅
- Implemented direct writes from streaming callback with `connection_valid` guards
- Connection close handling via `LWS_CALLBACK_WS_PEER_INITIATED_CLOSE` (reason 38)
- Double-cleanup guard prevents crashes on browser reload
- Removed all queue infrastructure - data flows directly to client

### 8. Provider-specific streaming formats research ✅
- OpenAI: SSE with `data: {...}` lines, `[DONE]` sentinel (parser exists)
- Anthropic: SSE with `event:` and `data:` lines (different event types)
- Ollama: Newline-delimited JSON (`application/x-ndjson`)

### 9. Provider-specific streaming parsers ✅
- Extended `chat_stream_chunk_parse()` to handle Anthropic events (content_block_delta, message_stop)
- Added parser for Ollama NDJSON format (response, done)
- Auto-detection based on JSON structure (type field, response+done fields)

### 10. Unit and integration tests ⏳
- Test WebSocket chat with mock streaming server
- Verify proper JSON message format and parsing
- Test streaming and non-streaming modes
- Test error conditions and client disconnections
- Ensure existing WebSocket terminal functionality unaffected

### 11. Documentation and cleanup ⏳
- Update Phase 11 plan with WebSocket approach
- Document WebSocket chat protocol for clients
- Consider deprecating HTTP streaming endpoint (optional)

### 12. Review of libwebsockets implementation and terminal subsystem ✅
- Reviewed WebSocket server architecture
- Identified thread-safety issue in terminal PTY bridge (low risk)
- Recommendations: Consider write queuing for terminal subsystem as well

## Verification Steps
1. Verify WebSocket server handles "chat" message type
2. Test chat request with JWT authentication
3. Confirm streaming chunks arrive as "chat_chunk" messages via write queue (thread-safe)
4. Verify non-streaming request returns single "chat_done" message
5. Test with OpenAI, Anthropic, and Ollama providers (parsers implemented)
6. Run unit and integration tests for WebSocket chat
7. Ensure existing terminal WebSocket and HTTP chat endpoints still work
8. Verify no crashes or data corruption under concurrent streaming connections
8. Review terminal subsystem integration and ensure no regressions