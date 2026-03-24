# WebSocket Connection

Lithium maintains a persistent, app-wide WebSocket connection to the Hydrogen server for real-time communication. This connection is established after login and maintained throughout the user session.

**Status: Operational** — The persistent WebSocket connection with keepalive is now fully functional. Chat streaming and server push messages work correctly over the stable connection.

---

## Overview

| Aspect | Details |
|--------|---------|
| **Module** | `src/shared/app-ws.js` |
| **Purpose** | Persistent real-time connection to Hydrogen server |
| **Lifecycle** | Connects on login, disconnects on logout |
| **Status Indicator** | Sidebar header icon (green/blue/red) |
| **Keepalive** | Application-level ping every 25 seconds |
| **Server Response** | Lightweight `keepalive_ok` (~50 bytes) |

### Architecture

```
┌─────────────────┐                    ┌─────────────────┐
│                 │    WebSocket       │                 │
│  Lithium Client │ ◄────────────────► │ Hydrogen Server │
│   (app-ws.js)   │    Port 7001       │  (websocket_*)  │
└─────────────────┘                    └─────────────────┘
         │
         │ Uses by:
         ▼
┌─────────────────┐     ┌─────────────────┐
│  Crimson Chat   │     │  Notifications  │
│  (crimson-ws.js)│     │    (future)     │
└─────────────────┘     └─────────────────┘
```

---

## Key Files

| File | Purpose |
|------|---------|
| `src/shared/app-ws.js` | App-wide WebSocket client with keepalive |
| `src/shared/crimson-ws.js` | Crimson-specific chat wrapper using app-ws |
| `src/managers/main/main.js` | Status indicator UI and initialization |
| `src/styles/layout.css` | Status indicator styles |

---

## Connection Configuration

The WebSocket connection uses settings from `config/lithium.json`:

```json
{
  "server": {
    "websocket_url": "wss://lithium.philement.com/wss",
    "websocket_key": "ABCDEFGHIJKLMNOP",
    "websocket_protocol": "hydrogen"
  }
}
```

### Configuration Fields

| Field | Description | Default |
|-------|-------------|---------|
| `server.websocket_url` | WebSocket server URL | `wss://lithium.philement.com/wss` |
| `server.websocket_key` | Authentication key (sent as query param) | `ABCDEFGHIJKLMNOP` |
| `server.websocket_protocol` | WebSocket subprotocol | `hydrogen` |

---

## Connection States

```javascript
export const ConnectionState = {
  DISCONNECTED: 'disconnected',  // No active connection
  CONNECTING: 'connecting',      // Connection in progress
  CONNECTED: 'connected',        // Connection established
  ERROR: 'error',                // Connection failed
};
```

---

## Status Indicator

The WebSocket status is displayed as an icon in the sidebar header:

| State | Icon | Color | Description |
|-------|------|-------|-------------|
| Disconnected | `fa-octagon` | Red | Connection not available |
| Connecting | `fa-circle` | Orange (pulsing) | Connection in progress |
| Connected | `fa-circle` | Green | Connected, idle |
| Sending | Flash to blue | Blue (100ms) | Data sent over WebSocket |

### CSS Classes

```css
.ws-status                /* Container */
.ws-status.disconnected   /* Red octagon */
.ws-status.connecting     /* Orange pulsing circle */
.ws-status.connected      /* Green circle */
.ws-status.sending        /* Blue flash state */
.ws-status.flash          /* Active blue flash animation */
```

---

## Keepalive Mechanism

The WebSocket connection uses application-level keepalive messages to maintain the connection through load balancers and proxies.

| Component | Interval | Message | Response |
|-----------|----------|---------|----------|
| **Client** | 25 seconds | `{"type":"keepalive"}` | — |
| **Server** | — | — | `{"type":"keepalive_ok","ts":<timestamp>}` |
| **TCP Keepalive** | 30s idle, 10s interval | OS-level probes | Configured on server |

### Keepalive Implementation

```javascript
// Client sends keepalive every 25 seconds
setInterval(() => {
  if (this.ws && this.ws.readyState === WebSocket.OPEN) {
    this.ws.send(JSON.stringify({ type: 'keepalive' }));
  }
}, 25000);
```

### Why Application-Level Keepalive?

The initial implementation used libwebsockets' protocol-level ping/pong, but this was insufficient because:
1. Load balancers may close idle WebSocket connections
2. Some proxies don't forward LWS ping frames
3. Application-level messages are visible in browser devtools for debugging

The lightweight keepalive response (~50 bytes) minimizes bandwidth overhead while ensuring the connection stays active through network infrastructure.

---

## Retry Behavior

When the connection drops unexpectedly (close code != 1000), the client automatically retries with escalating intervals:

| Phase | Interval | Duration | Attempts |
|-------|----------|----------|----------|
| 1 | 1 minute | 10 minutes | 10 |
| 2 | 2 minutes | 10 minutes | 5 |
| 3 | 5 minutes | 10 minutes | 2 |
| 4 | 10 minutes | Indefinite | ∞ |

### Connection Stability

As of March 2026, the WebSocket connection is stable with the following characteristics:

- **Persistent connections** — Connections remain open indefinitely
- **Chat streaming** — Full response streaming works over the persistent connection
- **Reconnection on drop** — Auto-reconnect with escalating intervals
- **Clean disconnect** — Code 1000 closes without reconnect (e.g., logout)
- **Handler persistence** — Message handlers survive cleanup and reconnection
- **Conversation history** — Multi-turn conversations work in both streaming and non-streaming modes

### Retry Flow

```
Connection lost
    │
    ▼
Phase 1: Try every 1 minute (10 attempts)
    │
    ▼ (after 10 minutes)
Phase 2: Try every 2 minutes (5 attempts)
    │
    ▼ (after 10 minutes)
Phase 3: Try every 5 minutes (2 attempts)
    │
    ▼ (after 10 minutes)
Phase 4: Try every 10 minutes (indefinitely)
```

### User-Initiated Disconnect

When the user logs out (`disconnectAppWS(true)` or `destroyAppWS()`), no retry is scheduled.

---

## API Usage

### Getting the WebSocket Instance

```javascript
import { getAppWS, ConnectionState } from '../shared/app-ws.js';

// Get the singleton instance
const ws = getAppWS();

// Check connection status
if (ws.isConnected()) {
  console.log('Connected');
}
```

### Sending Messages

```javascript
const ws = getAppWS();

// Send a message (returns true if sent successfully)
const sent = ws.send({
  type: 'chat',
  id: 'request_123',
  payload: { /* ... */ }
});
```

### Registering Message Handlers

```javascript
import { registerWSHandler, unregisterWSHandler } from '../shared/app-ws.js';

// Register a handler for a specific message type
registerWSHandler('chat_chunk', (message) => {
  console.log('Received chunk:', message);
});

// Unregister when done
unregisterWSHandler('chat_chunk');
```

### Lifecycle Callbacks

```javascript
const ws = getAppWS({
  onStateChange: (newState, oldState) => {
    console.log(`State changed: ${oldState} -> ${newState}`);
  },
  onSendActivity: () => {
    // Trigger blue flash indicator
  },
  onMessage: (message) => {
    // Fallback handler for unregistered message types
  }
});
```

---

## Crimson Integration

Crimson chat uses the app-wide WebSocket through a wrapper module (`crimson-ws.js`):

```javascript
import { getCrimsonWS } from '../shared/crimson-ws.js';

// Get the Crimson chat client
const crimsonWS = getCrimsonWS({
  onChunk: (content, index, finishReason) => {
    // Handle streaming chunk
  },
  onDone: (content, result) => {
    // Handle completion
  },
  onError: (error) => {
    // Handle error
  }
});

// Send a chat message
await crimsonWS.send('How do I create a query?', {
  history: conversationHistory,
  stream: true
});
```

### Streaming Architecture

The chat streaming flow over WebSocket:

```
Client                    Server                      AI Engine
  │                         │                            │
  │──── chat message ──────►│                            │
  │                         │──── request ──────────────►│
  │                         │                            │
  │◄──── chat_chunk ────────│◄──── chunk stream ─────────│
  │     (multiple)          │     (multiple)             │
  │                         │                            │
  │◄──── chat_done ─────────│◄──── finish ───────────────│
  │                         │                            │
  │◄──── keepalive_ok ──────│                            │
  │                         │                            │
  │──── keepalive ─────────►│                            │
  │◄──── keepalive_ok ──────│                            │
  │                         │                            │
  ... connection stays open ...
```

---

## Logout Handling

The WebSocket connection is gracefully closed during logout:

```javascript
import { disconnectAppWS } from '../shared/app-ws.js';

// Graceful disconnect (sends close code 1000)
disconnectAppWS();
```

This happens automatically when:
- User clicks logout button
- Session expires
- App is unloaded (beforeunload event)

---

## Initialization Sequence

```
1. User logs in
2. MainManager.init() called
3. MainManager.initWebSocket() called
4. getAppWS() creates singleton instance
5. ws.connect() establishes connection
6. Status indicator shows "connecting" (orange)
7. Connection established
8. Status indicator shows "connected" (green)
9. Keepalive interval starts
10. Chat modules can now send messages
```

---

## Error Handling

### Connection Failure

If the initial connection fails:
- Status indicator shows "disconnected" (red octagon)
- Connection will be retried when a module tries to send
- Keepalive will attempt to reconnect on timeout

### Stale Connection Detection

If the server doesn't respond to keepalive pings:
- Connection is considered stale after `KEEPALIVE_TIMEOUT`
- Client forces reconnect by closing the connection
- Status indicator shows "connecting" during reconnect

---

## Related Documentation

- [LITHIUM-API.md](LITHIUM-API.md) — HTTP API endpoints
- [LITHIUM-MGR-CRIMSON.md](LITHIUM-MGR-CRIMSON.md) — Crimson AI Agent (uses WebSocket)
- [LITHIUM-MGR-MAIN.md](LITHIUM-MGR-MAIN.md) — Main Manager (initializes WebSocket)
- [LITHIUM-JWT.md](LITHIUM-JWT.md) — JWT authentication (used for chat auth)

---

Last updated: March 24, 2026
