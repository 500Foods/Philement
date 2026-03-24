# WebSocket Server

Hydrogen's WebSocket server provides real-time bidirectional communication for the Lithium client, enabling chat streaming, keepalive, and future push notifications.

**Status: Operational** — Persistent WebSocket connections with keepalive are fully functional. The server maintains stable connections through load balancers.

---

## Overview

| Aspect | Details |
|--------|---------|
| **Source** | `src/websocket/websocket_server*.c` |
| **Protocol** | `hydrogen` (custom subprotocol) |
| **Default Port** | Configurable (typically 7001) |
| **Authentication** | Query parameter or header key |
| **Library** | libwebsockets (lws) |

---

## Architecture

```
┌─────────────────┐                    ┌─────────────────┐
│                 │    WebSocket       │                 │
│  Lithium Client │ ◄────────────────► │ Hydrogen Server │
│   (app-ws.js)   │    Port 7001       │  (websocket_*)  │
└─────────────────┘                    └─────────────────┘
                                          │
                                          │ Uses
                                          ▼
                                 ┌─────────────────┐
                                 │  AI Conduit     │
                                 │  (chat proxy)   │
                                 └─────────────────┘
```

---

## Key Files

| File | Purpose |
|------|---------|
| `websocket_server.c` | Main server thread and service loop |
| `websocket_server_startup.c` | Context and vhost initialization |
| `websocket_server_dispatch.c` | Callback dispatch for all lws events |
| `websocket_server_message.c` | Message parsing, routing, and response writing |
| `websocket_server_chat.c` | Chat streaming with queue-based chunk delivery |
| `websocket_server_heartbeat.c` | Ping/pong keepalive mechanism |
| `websocket_server_connection.c` | Connection lifecycle management |
| `websocket_server_terminal.c` | Terminal protocol message handling |
| `websocket_server_auth.c` | Authentication validation |

---

## Connection Lifecycle

### 1. Connection Establishment

```
Client                           Server
  │                                │
  │─── HTTP Upgrade (with key) ───►│
  │                                │─── Validate auth key
  │                                │─── Create session
  │◄─── 101 Switching Protocols ───│
  │                                │─── LWS_CALLBACK_ESTABLISHED
  │                                │─── Log: "Connection ESTABLISHED"
```

### 2. Message Exchange

```
Client                           Server
  │                                │
  │─── JSON message ──────────────►│
  │                                │─── LWS_CALLBACK_RECEIVE
  │                                │─── parse_and_handle_message()
  │                                │─── Return 0 (success) or -1 (error)
  │◄─── JSON response ─────────────│
```

### 3. Connection Closure

```
Client                           Server
  │                                │
  │◄─── Close frame ───────────────│  (server-initiated)
  │─── Close frame ───────────────►│  (client-initiated)
  │                                │─── LWS_CALLBACK_CLOSED
  │                                │─── ws_handle_connection_closed()
```

---

## Configuration

The vhost is configured with the following settings:

| Setting | Value | Purpose |
|---------|-------|---------|
| `timeout_secs` | 3600 | Idle connection timeout (1 hour) |
| `ka_time` | 30 | TCP keepalive idle time (seconds) |
| `ka_interval` | 10 | TCP keepalive probe interval (seconds) |
| `ka_probes` | 3 | TCP keepalive failed probes before close |

---

## Message Types

### Client → Server

| Type | Description | Handler |
|------|-------------|---------|
| `chat` | Chat request (streaming or non-streaming) | `handle_chat_message()` |
| `keepalive` | Connection keepalive ping | `handle_keepalive_request()` |
| `status` | System status request | `handle_status_request()` |
| `terminal_input` | Terminal keystrokes | `handle_terminal_input()` |
| `terminal_resize` | Terminal resize | `handle_terminal_resize()` |
| `ping` | Heartbeat ping | `handle_heartbeat_ping()` |

### Server → Client

| Type | Description | Size |
|------|-------------|------|
| `chat_chunk` | Streaming response chunk | Variable |
| `chat_done` | Streaming complete marker | ~50 bytes |
| `keepalive_ok` | Keepalive response | ~50 bytes |
| `status_response` | System status | ~6KB |
| `terminal_output` | Terminal output | Variable |
| `pong` | Heartbeat response | Small |

---

## Keepalive Mechanism

### Application-Level Keepalive

The server responds to client keepalive messages with a lightweight response:

```json
{"type":"keepalive_ok","ts":1234567890}
```

**Implementation** (`websocket_server_message.c`):
```c
if (strcmp(type, "keepalive") == 0 || strcmp(type, "k") == 0) {
    log_this(SR_WEBSOCKET, "Handling keepalive request", LOG_LEVEL_DEBUG, 0);
    json_t *response = json_pack("{s:s, s:I}", 
                                  "type", "keepalive_ok",
                                  "ts", (json_int_t)time(NULL));
    int result = ws_write_json_response(wsi, response);
    json_decref(response);
    return result;  // Returns 0 on success
}
```

### Lightweight Response Design

The keepalive response is intentionally minimal (~50 bytes) compared to the status response (~6KB). This reduces bandwidth overhead for the periodic keepalive traffic.

---

## Chat Streaming

### Streaming Architecture

```
1. Client sends chat message with stream: true
2. Server creates queue for response chunks
3. Server forwards to AI engine via chat_proxy
4. AI engine streams chunks via callback
5. Server enqueues each chunk
6. LWS writable callback dequeues and sends chunks
7. Server enqueues chat_done marker
8. Server triggers writable callback
9. Client receives all chunks, then done message
```

### Queue-Based Chunk Delivery

The streaming implementation uses a thread-safe priority queue:

```c
// Create queue for this connection
char* queue_name = create_chat_queue_name(wsi);
session->chat_write_queue_name = queue_name;

// Stream callback enqueues chunks
bool stream_chunk_callback(const char* chunk, size_t chunk_len, void* user_data) {
    queue_enqueue(queue, chunk, chunk_len, priority);
    lws_callback_on_writable(wsi);  // Request writable callback
    return true;
}

// Writable callback dequeues and sends
void handle_chat_writable(struct lws *wsi, const WebSocketSessionData *session) {
    dequeue_and_write_chat_chunks(wsi, session->chat_write_queue_name);
}
```

### Non-Blocking Dequeue

The `dequeue_and_write_chat_chunks()` function uses `queue_dequeue_nonblocking()` to prevent blocking the lws service loop:

```c
while ((data = queue_dequeue_nonblocking(queue, &size, &priority)) != NULL) {
    ws_write_raw_data(wsi, data, size);
    free(data);
}
```

---

## Return Value Protocol

**Critical**: WebSocket callback functions must return `0` on success or `-1` on error. Returning any other value (like response byte count) causes libwebsockets to close the connection.

### Correct Pattern

```c
int handle_keepalive_request(struct lws *wsi) {
    json_t *response = json_pack("{s:s}", "type", "keepalive_ok");
    int result = ws_write_json_response(wsi, response);
    json_decref(response);
    return result;  // Returns 0 on success, -1 on error
}
```

### Incorrect Pattern (Fixed)

```c
// OLD (BROKE CONNECTIONS):
int ws_write_json_response(struct lws *wsi, json_t *json) {
    ...
    return result == 0 ? (int)response_len : result;  // Returns 39!
}

// NEW (CORRECT):
int ws_write_json_response(struct lws *wsi, json_t *json) {
    ...
    return result;  // Returns 0 on success
}
```

---

## Debug Logging

Enable WebSocket debug logging by checking for `[WS]` prefixed messages:

| Log | Meaning |
|-----|---------|
| `[WS] Connection ESTABLISHED` | New client connected |
| `[WS] LWS_CALLBACK_RECEIVE - len=N` | Received N bytes |
| `[WS] parse_and_handle_message returned N` | Message handler returned N |
| `[WS] LWS_CALLBACK_SERVER_WRITEABLE` | Writable callback fired |
| `[WS] lws_write returned N for M bytes` | Wrote M bytes, result N |
| `[WS] Connection CLOSED` | Client disconnected |
| `[WS] PING sent to IP` | Heartbeat ping sent |

---

## Error Handling

| Error | Handling |
|-------|----------|
| Invalid auth key | Connection rejected (return -1) |
| Message too large | Logged and ignored |
| JSON parse error | Logged and ignored |
| Unknown message type | Logged, return 0 (don't close) |
| Terminal message on non-terminal protocol | Ignored gracefully |
| Write failure | Connection closes |

---

## Performance Characteristics

| Metric | Value |
|--------|-------|
| Keepalive response size | ~50 bytes |
| Status response size | ~6KB |
| Keepalive interval | 25 seconds (client) |
| TCP keepalive idle | 30 seconds |
| Connection timeout | 1 hour (idle) |

---

## Related Documentation

- [LITHIUM-WSS.md](../Li/LITHIUM-WSS.md) — Client-side WebSocket implementation
- [LITHIUM-MGR-CRIMSON.md](../Li/LITHIUM-MGR-CRIMSON.md) — Crimson AI chat integration
- [DATABASES.md](DATABASES.md) — Database systems used by chat storage

---

Last updated: March 24, 2026
