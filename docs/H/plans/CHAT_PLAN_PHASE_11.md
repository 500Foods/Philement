# Chat Service - Phase 11: Streaming Support

## Objective
Add streaming response support (Server-Sent Events) for real-time chat interactions.

## Prerequisites
- Phase 10 completed and verified (cross-server segment recovery working)

## Testable Gate
Before proceeding to Phase 12, the following must be verified:
- POST /api/conduit/auth_chat/stream endpoint registered and functional
- Stream: true parameter handled correctly
- Server-Sent Events format implemented properly
- Streaming works with all supported AI providers
- Unit and integration tests pass for streaming functionality
- No regression in existing non-streaming endpoints

## Tasks

### 1. Implement streaming endpoint
- Create `auth_chat_stream/auth_chat_stream.h` and `auth_chat_stream/auth_chat_stream.c`
- Handle POST /api/conduit/auth_chat/stream
- Require JWT authentication (same as auth_chat)
- Extract database from JWT claims
- Support stream: true parameter

### 2. Server-Sent Events implementation
- Set appropriate HTTP headers for SSE:
  - Content-Type: text/event-stream
  - Cache-Control: no-cache
  - Connection: keep-alive
- Implement proper event formatting:
  - id: (optional)
  - event: message
  - data: {JSON payload}
  - \n\n separator
- Handle reconnect mechanisms with Last-Event-ID header

### 3. Streaming integration with proxy
- Modify chat_proxy.c to support streaming responses
- Process incoming chunks as they arrive
- Parse partial JSON if needed (for chunked responses)
- Forward chunks to client as SSE events
- Handle connection timeouts and errors appropriately

### 4. Provider-specific streaming support
- Verify OpenAI, xAI, Ollama support streaming natively
- Handle Anthropic streaming if available
- Implement fallback to non-streaming for providers without streaming
- Maintain consistent event format regardless of provider

### 5. Message accumulation
- Accumulate streaming chunks to form complete message
- Calculate final token counts when stream ends
- Store complete message in conversation_segments (after stream completion)
- Provide intermediate events for typing indicators if desired

### 6. Error handling in streams
- Send error events in SSE format when issues occur
- Properly terminate stream on unrecoverable errors
- Handle client disconnections gracefully
- Clean up resources when stream ends

### 7. Registration in api_service.c
- Add to endpoint_requires_auth(): "conduit/auth_chat/stream"
- Add to endpoint_expects_json(): "conduit/auth_chat/stream" 
- Add routing strcmp() block for the path

### 8. Unit and integration tests
- Test streaming with mock server that sends delayed chunks
- Verify proper SSE format and event parsing
- Test accumulation of chunks into complete message
- Test error handling and stream termination
- Test client disconnection handling
- Ensure existing non-streaming endpoints unaffected

## Verification Steps
1. Verify endpoint registration in api_service.c
2. Test streaming endpoint returns proper SSE headers
3. Confirm stream: true parameter enables streaming mode
4. Verify chunks are sent as proper SSE events
5. Test message accumulation and final storage
6. Test error handling in streaming context
7. Run unit and integration tests for streaming
8. Ensure existing chat endpoints still work normally