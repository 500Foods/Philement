# Chat Service Implementation Plan - Phased Approach

## Overview
This document summarizes the 13-phase implementation plan for the Chat Proxy Service within the Conduit API. Each phase has a specific testable gate to ensure quality and prevent scope creep.

## Phase Files

1. **[CHAT_PLAN_PHASE_1.md](CHAT_PLAN_PHASE_1.md)** - Prerequisites and Infrastructure
   - libcurl integration
   - Internal CEC bootstrap QueryRef creation
   - Internal query enforcement
   - Chat configuration fields
   - **Gate**: libcurl functional, tests pass, internal query enforcement working

2. **[CHAT_PLAN_PHASE_2.md](CHAT_PLAN_PHASE_2.md)** - Chat Engine Cache and Proxy Foundation
   - CEC data structures and lookup functions
   - CEC bootstrap and refresh mechanisms
   - chat_proxy.c, chat_request_builder.c, chat_response_parser.c
   - Token pre-counting and retry mechanisms
   - **Gate**: CEC loads successfully, proxy/builder/parser unit tests pass

2.5. **[CHAT_PLAN_PHASE_2_5.md](CHAT_PLAN_PHASE_2_5.md)** - Chat Status and Monitoring
   - Health tracking with configurable Liveliness interval
   - Augmented `/api/conduit/status` endpoint with model information
   - Prometheus metrics for chat operations
   - **Gate**: Status endpoint returns model health, Prometheus metrics accessible

3. **[CHAT_PLAN_PHASE_3.md](CHAT_PLAN_PHASE_3.md)** - Authenticated Endpoints (auth_chat, auth_chats)
   - JWT authentication integration
   - Database extraction from JWT claims
   - Single and batch chat endpoints
   - Image/multimodal support (Day One)
   - **Gate**: POST /api/conduit/auth_chat and /api/conduit/auth_chats working

4. **[CHAT_PLAN_PHASE_4.md](CHAT_PLAN_PHASE_4.md)** - Cross-Database Endpoints (alt_chat, alt_chats)
   - Database override via request body (not JWT claims)
   - Cross-database routing for centralized auth patterns
   - **Gate**: POST /api/conduit/alt_chat and /api/conduit/alt_chats working

5. **[CHAT_PLAN_PHASE_5.md](CHAT_PLAN_PHASE_5.md)** - Additional Provider Support
   - Anthropic native format support
   - Ollama native API support
   - Provider-specific configuration options
   - **Gate**: Non-OpenAI providers working correctly

6. **[CHAT_PLAN_PHASE_6.md](CHAT_PLAN_PHASE_6.md)** - Conversation History with Content-Addressable Storage + Brotli
   - conversation_segments table with Brotli compression
   - Extended convos table with segment_refs
   - Storage and retrieval pipelines
   - **Gate**: Content-addressable storage working with compression benefits

7. **[CHAT_PLAN_PHASE_7.md](CHAT_PLAN_PHASE_7.md)** - Update Existing Chat Queries
   - New QueryRefs #067, #068, #069 replaces #036, #039, #041
   - QueryRefs D, E, F already implemented as #064, #065, #066
   - Backwards compatibility maintained
   - 42 Unity tests pass, cppcheck clean
   - **Gate**: ✅ New queries work with hash-based storage, backwards compatibility maintained

8. **[CHAT_PLAN_PHASE_8.md](CHAT_PLAN_PHASE_8.md)** - Context Hashing for Client-Server Optimization
    - SHA-256 hashing of message content (43-char base64url)
    - Optional `context_hashes` field in chat requests
    - Hash validation and segment existence checking
    - Bandwidth statistics in response (hashes_used, bandwidth_saved)
    - Graceful fallback when hashes not found
    - **Gate**: ✅ Context hashing framework complete, 23 unit tests passing

9. **[CHAT_PLAN_PHASE_9.md](CHAT_PLAN_PHASE_9.md)** - Local Disk Cache and LRU
   - 1GB LRU disk cache for hot segments
   - Uncompressed segment storage to avoid repeated decompression
   - Background sync to database
   - **Gate**: ✅ Cache implemented, 12 unit tests passing, LRU eviction working

10. **[CHAT_PLAN_PHASE_10.md](CHAT_PLAN_PHASE_10.md)** - Cross-Server Segment Recovery
    - Batch query for missing segments (QueryRef #070)
    - Automatic fallback to database (from Phase 9)
    - Conservative pre-fetching
    - Cache location changed to `./cache/` with `CHAT_CACHE_DIR` env var override
    - **Gate**: ✅ Batch retrieval working, 9 unit tests passing, cppcheck clean

11. **[CHAT_PLAN_PHASE_11.md](CHAT_PLAN_PHASE_11.md)** - Streaming Support
    - POST /api/conduit/auth_chat/stream endpoint
    - Server-Sent Events implementation
    - Streaming integration with proxy
    - **Gate**: Streaming endpoint working with proper SSE format

12. **[CHAT_PLAN_PHASE_12.md](CHAT_PLAN_PHASE_12.md)** - Advanced Multi-modal Features
    - Media upload endpoint (/api/conduit/upload)
    - media_assets table
    - Extended conversation_segments with metadata
    - Provider-specific multi-modal handling
    - Engine limits enforcement
    - **Gate**: Advanced multi-modal features working with proper provider translation

13. **[CHAT_PLAN_PHASE_13.md](CHAT_PLAN_PHASE_13.md)** - Advanced Features
    - Function calling support
    - Response caching
    - Load balancing across API keys
    - Fallback engines
    - Usage analytics dashboard
    - Conversation management APIs
    - Real-time cost tracking
    - A/B testing framework
    - **Gate**: All advanced features working without regressions

## Implementation Philosophy

This phased approach ensures:
1. **Early Value Delivery**: Core chat functionality available after Phase 3
2. **Risk Mitigation**: Each phase builds on verified previous work
3. **Testability**: Clear gates prevent accumulating technical debt
4. **Flexibility**: Phases can be adjusted based on feedback and changing requirements
5. **Quality Focus**: Unit and integration tests required at each step

## Dependencies Between Phases

Each phase depends on the successful completion of the previous phase's testable gate. This creates a solid foundation where:
- Phases 1-2 establish infrastructure and core components (libcurl, CEC, proxy)
- Phase 2.5 adds monitoring and observability before public endpoints
- Phases 3-5 deliver progressively more sophisticated API endpoints (public, auth, cross-db, multi-provider)
- Phases 6-9 enhance performance and storage (content-addressable storage, Brotli, context hashing, LRU cache)
- Phases 10-13 add advanced features (batch recovery, streaming, multi-modal, enterprise features)

## Current Progress

| Phase | Status | Completion Date |
|-------|--------|-----------------|
| Phase 1 | ✅ COMPLETE | 2026-03-20 |
| Phase 2 | ✅ COMPLETE | 2026-03-20 |
| Phase 2.5 | ✅ COMPLETE | 2026-03-20 |
| Phase 3 | ✅ COMPLETE | 2026-03-20 |
| Phase 4 | ✅ COMPLETE | 2026-03-20 |
| Phase 5 | ✅ COMPLETE | 2026-03-21 |
| Phase 6 | ✅ COMPLETE | 2026-03-22 |
| Phase 7 | ✅ COMPLETE | 2026-03-22 |
| Phase 8 | ✅ COMPLETE | 2026-03-23 |
| Phase 9 | ✅ COMPLETE | 2026-03-23 |
| Phase 10 | ✅ COMPLETE | 2026-03-23 |
| Phase 11 | ✅ COMPLETE | 2026-03-23 |
| Phase 12 | ✅ COMPLETE | 2026-03-23 |
| Phase 13 | ⏳ PENDING | - |

## Next Steps

Phase 11 (Streaming) and Phase 12 (Advanced Multi-modal) are now complete. The next phase is **Phase 13: Advanced Features** which will implement:
- Function calling support
- Response caching
- Load balancing across API keys
- Fallback engines
- Usage analytics dashboard
- Conversation management APIs
- Real-time cost tracking
- A/B testing framework

Client development can now proceed with the completed multi-modal infrastructure.

---

## WebSocket Chat Endpoint

In addition to the HTTP endpoints, Hydrogen provides a WebSocket-based chat interface for real-time streaming communication.

### WebSocket Server Configuration

| Setting | Default | Description |
|---------|---------|-------------|
| `WebSocketServer.Port` | `7001` | WebSocket listening port |
| `WebSocketServer.Protocol` | `hydrogen` | Protocol name for client identification |
| `WebSocketServer.Key` | (from env) | Authentication key for connections |

### Connection Authentication

WebSocket connections authenticate via query parameter:

```
ws://host:port/path?key=<auth_key>
```

The server validates the key in `LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION` before allowing the upgrade.

**Note**: libwebsockets strips query strings during WebSocket upgrade. The server uses a fallback authentication mechanism for JavaScript clients that relies on matching the configured key against a hardcoded value.

### Message Protocol

#### Client → Server: Chat Request

```json
{
  "type": "chat",
  "id": "request-123",
  "payload": {
    "engine": "Crimson",
    "messages": [
      {"role": "user", "content": "Hello, Crimson!"}
    ],
    "stream": true,
    "jwt": "Bearer <jwt-token>"
  }
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `type` | string | Yes | Message type: `"chat"` |
| `id` | string | No | Request ID for tracking |
| `payload.engine` | string | No | Engine name (defaults to config default) |
| `payload.messages` | array | Yes | Chat messages array |
| `payload.stream` | boolean | No | Enable streaming (default: true) |
| `payload.jwt` | string | Yes | JWT token with `Bearer ` prefix |

#### Server → Client: Streaming Chunk

```json
{
  "type": "chat_chunk",
  "id": "request-123",
  "chunk": {
    "content": "Hello",
    "model": "Crimson",
    "index": 0,
    "finish_reason": null
  }
}
```

#### Server → Client: Completion

```json
{
  "type": "chat_done",
  "id": "request-123",
  "result": {
    "content": "Full response text",
    "model": "Crimson",
    "finish_reason": "stop",
    "tokens": {"prompt": 10, "completion": 20, "total": 30},
    "response_time_ms": 1234.5
  }
}
```

#### Server → Client: Error

```json
{
  "type": "chat_error",
  "id": "request-123",
  "error": "Error message"
}
```

### Structured Response Format

The AI is prompted to return structured responses with a delimiter:

```
[Conversation text here]
[LITHIUM-CRIMSON-JSON]
{"followUpQuestions": [...], "suggestions": {...}, "metadata": {...}}
```

This allows clients to:
1. Stream the conversation text immediately
2. Parse structured data after the delimiter
3. Display follow-up questions as interactive buttons
4. Process suggestions for UI actions

### Authentication Flow

1. **Connection**: Client connects with `?key=<auth_key>` query parameter
2. **Validation**: Server validates key during protocol filtering
3. **Chat**: Each chat message includes JWT in payload for database context
4. **Session**: Database claim from JWT is cached in session for subsequent messages

### Error Handling

| Error | HTTP/WS Code | Description |
|-------|--------------|-------------|
| Missing JWT | `chat_error` | No JWT provided in payload |
| Invalid JWT | `chat_error` | JWT validation failed |
| Missing database claim | `chat_error` | JWT lacks database claim |
| Engine not found | `chat_error` | Requested engine not configured |
| Engine unhealthy | `chat_error` | Engine health check failed |

### Known Issues

1. **Query String Stripping**: libwebsockets strips query strings during upgrade. Server uses fallback authentication for JavaScript clients.

2. **Connection Lifecycle**: Connections are persistent and remain open for the lifetime of the client. Clients establish a connection once and reuse it for multiple chat requests. The connection will only close if the client disconnects, goes to sleep, or fails to respond to heartbeat pings.

3. **Key Consistency**: The server's hardcoded fallback key must match the configured key for JavaScript clients to authenticate.

### Heartbeat and Connection Health

The WebSocket server implements a heartbeat mechanism to maintain connections through proxies and load balancers (like Traefik in DOKS clusters):

- **Ping/Pong**: Server sends ping frames periodically (default: every 30 seconds)
- **TCP Keepalive**: Enabled at the socket level (ka_time=30s, ka_interval=10s, ka_probes=3)
- **Stale Detection**: Connections that don't respond to pings within the timeout (default: 60 seconds) are closed

Configuration options in `WebSocketServer.Heartbeat`:
- `Enabled`: Enable/disable heartbeat (default: true)
- `PingIntervalSeconds`: How often to send pings (default: 30)
- `PongTimeoutSeconds`: How long to wait for pong before closing (default: 60)
- `StaleConnectionSeconds`: Consider connection stale after this time (default: 90)

### Testing

WebSocket chat can be tested with the terminal client:

```bash
# Using websocat
websocat ws://localhost:7001/?key=ABCDEFGHIJKLMNOP --protocol=hydrogen
```

Then send a JSON message:
```json
{"type":"chat","id":"test-1","payload":{"engine":"Crimson","messages":[{"role":"user","content":"Hello"}],"stream":true,"jwt":"Bearer <token>"}}
```