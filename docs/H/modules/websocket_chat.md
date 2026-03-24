# WebSocket Chat Handler

Handles chat messages via the WebSocket protocol, supporting both streaming and non-streaming responses.

**Module:** `src/websocket/websocket_server_chat.c`  
**Status:** Operational — Multi-turn conversations working with conversation history

---

## Overview

| Aspect | Details |
|--------|---------|
| **Module** | `websocket_server_chat.c` |
| **Purpose** | Process chat requests and stream AI responses |
| **Message Types** | `chat`, `chat_chunk`, `chat_done`, `chat_error` |
| **Modes** | Streaming (chunked) and Non-streaming (single response) |

---

## Message Types

### Client → Server

```json
{
  "type": "chat",
  "id": "crimson_1234567890_1",
  "payload": {
    "engine": "Crimson",
    "messages": [
      {"role": "user", "content": "Hello"}
    ],
    "stream": true,
    "jwt": "Bearer <token>"
  }
}
```

### Server → Client (Streaming)

```json
// Multiple chat_chunk messages
{
  "type": "chat_chunk",
  "id": "crimson_1234567890_1",
  "chunk": {
    "content": "Hello",
    "index": 0,
    "finish_reason": null
  }
}

// Final chat_done message
{
  "type": "chat_done",
  "id": "crimson_1234567890_1",
  "result": {
    "content": "Full response text",
    "model": "Kimi K2.5",
    "tokens": {"prompt": 100, "completion": 50, "total": 150},
    "response_time_ms": 2500.5
  }
}
```

---

## Logging Improvements (March 2026)

Enhanced logging now includes sizes for debugging:

```
Request parsed: 3 messages, stream=false
Prompt sent to Crimson/Kimi K2.5/OpenAI (non-streaming, 963 bytes)
Response received from Crimson/Kimi K2.5/OpenAI (5964ms, 4192 bytes)
Parsed response: content_length=4096, model=Kimi K2.5, finish_reason=stop
Sending response: content=4096 bytes, messages=3, finish=stop
Final response sent to client for request crimson_123 (5966ms total, 4096 bytes content)
```

### Logged Information

| Event | Information Logged |
|-------|-------------------|
| Request received | Message count, streaming mode |
| Prompt sent | Engine, provider, mode, request size (bytes) |
| Response received | Engine, provider, time (ms), response size (bytes) |
| Response parsed | Content length, model, finish reason |
| Response sent | Content size, message count, total time |

---

## Queue Management

### Streaming Queue Lifecycle

The chat queue is used for thread-safe communication between the streaming thread and the WebSocket service thread:

1. **Creation**: Queue created at start of streaming request
2. **Enqueuing**: Chunks enqueued by streaming callback
3. **Dequeuing**: Chunks dequeued and written in `handle_chat_writable()`
4. **Cleanup**: Queue cleaned up in `chat_session_cleanup()` on connection close

### Important: Queue Cleanup Timing

**Do NOT clean up the queue in `handle_chat_writable()`** or immediately after `lws_callback_on_writable()`. This causes a race condition:

```
❌ WRONG:
1. Stream completes
2. lws_callback_on_writable() called
3. Queue cleaned up immediately
4. handle_chat_writable() fires → queue is empty → nothing sent

✅ CORRECT:
1. Stream completes
2. lws_callback_on_writable() called
3. handle_chat_writable() fires → dequeues and writes chunks
4. Connection closes → chat_session_cleanup() cleans up queue
```

---

## Conversation History

### Multi-turn Support

The server accepts a `messages` array containing conversation history:

```json
{
  "messages": [
    {"role": "user", "content": "First question"},
    {"role": "assistant", "content": "First answer"},
    {"role": "user", "content": "Follow-up question"}
  ]
}
```

### History Handling

- History is passed directly to the AI provider
- Client sends last 10 messages for context
- Server logs message count for debugging

---

## Error Handling

### Error Response

```json
{
  "type": "chat_error",
  "id": "crimson_123",
  "error": "Error message"
}
```

### Common Errors

| Error | Cause |
|-------|-------|
| Missing or invalid payload | Request body malformed |
| JWT required for chat authentication | No JWT provided |
| Invalid JWT | Token expired or malformed |
| Missing database claim | JWT missing database scope |
| Engine not found | Invalid engine name |
| Engine is currently unavailable | Engine health check failed |

---

## Related Documentation

- [LITHIUM-WSS.md](/docs/Li/LITHIUM-WSS.md) — Client-side WebSocket documentation
- [LITHIUM-MGR-CRIMSON.md](/docs/Li/LITHIUM-MGR-CRIMSON.md) — Crimson UI implementation

---

Last updated: March 24, 2026
