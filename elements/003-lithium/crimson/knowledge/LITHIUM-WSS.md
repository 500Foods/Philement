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
| `src/shared/radar-controller.js` | Radar status icon controller (sweep, targets, colors) |
| `src/shared/conduit.js` | Conduit API wrappers (add/remove radar targets per request) |
| `src/managers/main/main.js` | Status indicator UI and initialization |
| `src/managers/main/main.css` | Radar icon styles (sidebar header) |

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

The connection status is displayed as an animated **radar icon** in the sidebar header gradient. The radar serves double duty: it visualizes both WebSocket connection health and REST API request activity.

### Radar Icon Architecture

The radar is an inline SVG (`#radar-icon`) driven entirely by JavaScript via `src/shared/radar-controller.js`. The animation is **time-based** (uses `performance.now()` timestamps, not frame counting), so rotation speed is deterministic and independent of frame rate.

```
┌──────────────────────────────────────┐
│  1. Sweep trail  (25 fading paths)  │  ← Rotates 0.108°/ms (3.333s period)
│  2. Targets      (dynamic blips)    │  ← 60% radius, flash on sweep pass
│  3. Outer rim    (static circle)    │  ← Color = WS health (currentColor)
│  4. Sweep line   (single arm)       │  ← Rotates with trail, white
│  5. Center hub   (dot)              │  ← Uses currentColor (--accent-primary)
└──────────────────────────────────────┘
```

### Timing Model

All timing derives from a single constant:

| Constant | Value | Meaning |
|----------|-------|---------|
| `SWEEP_PERIOD_MS` | 3333 | Full 360° rotation period |
| `SWEEP_DPS` | 0.108°/ms | Angular velocity (360 / 3333) |
| `HALF_PERIOD_MS` | 1667 | Half rotation — fade-out duration |
| `TARGET_FLASH_MS` | 300 | Initial bright flash on target add |
| `PING_DURATION_MS` | 400 | Bright flash when sweep passes target |
| `PING_THRESHOLD_DEG` | 5° | Angular proximity for sweep-pass detection |

Because the animation loop computes `angle += SWEEP_DPS * (now - lastTimestamp)`, the sweep position is always accurate to wall-clock time regardless of frame drops.

### Connection States

The outer rim and center hub use `currentColor`, which resolves to `--accent-primary` via CSS on `#radar-icon`. State classes override this:

| State | Class | Color | Meaning |
|-------|-------|-------|---------|
| Connected | `.radar-connected` | `#86efac` (light green) | WebSocket is live |
| Flaky | `.radar-flaky` | `#fde68a` (light amber) | Connection is unstable |
| Disconnected | `.radar-disconnected` | `#fca5a5` (light red) | No active connection |
| Connecting | `.radar-flaky` | `#fde68a` (light amber) | Connection in progress |
| Error | `.radar-disconnected` | `#fca5a5` (light red) | Connection failed |

On disconnect, the icon plays a brief shake animation (±8° rotation, 420ms).

### Target Blips (REST API + WebSocket Tracking)

When a request starts, a **target blip** appears at the **current sweep arm position on the 60% radius ring**. The target's angular position is captured at creation time for sweep-pass detection.

| Target Type | Shape | Triggered By |
|-------------|-------|--------------|
| `triangle` | Triangle | Single queries (`authQuery`, `query`), WS messages |
| `square` | Square | Batch queries (`authQueries`, `queries`), WS keepalive |

**Target lifecycle:**

1. **Appear** — Bright white at the sweep arm's current position (60% radius)
2. **Settle** — After 300ms, fades to normal color: green (`#10b981`) for WS, red (`#ef4444`) for REST
3. **Sweep pass** — Each time the sweep arm passes within 5° of the target, it flashes bright white with a glow for 400ms, then returns to normal color
4. **Remove** — Flash sequence (white → color → white → color), then fade to transparent over ~1.67s (half a sweep cycle)

**REST API tracking:** The conduit wrappers (`authQuery`, `authQueries`, `query`, `queries`) call `addTarget(type, 'rest')` before the request and `removeTarget(id)` in a `finally` block.

**WebSocket tracking:** `app-ws.js` calls `addTarget(type, 'ws')` on `send()` and `removeTarget()` (no ID, FIFO) when a response arrives (`keepalive_ok`, `chat_done`, `chat_error`).

### Sweep Animation

The sweep is driven by `requestAnimationFrame` with **time-based delta**:

```javascript
const dt = timestamp - lastTimestamp;
angle = (angle + SWEEP_DPS * dt) % 360;
```

This means a full rotation always takes exactly `SWEEP_PERIOD_MS` (3.333s), regardless of whether the browser drops frames. The animation loop runs only when:

- The WebSocket is connected, **or**
- At least one target blip exists

### Heartbeat Flash

On each WebSocket keepalive (`onHeartbeat()`), the sweep arm briefly brightens to `#a5f3fc` (140ms), giving visual confirmation that the connection is alive.

### Color Scheme

- **Sweep elements** (trail paths, arm stroke): white (`#ffffff`)
- **Targets**: start white → settle to green (WS) / red (REST) → flash white on sweep pass → fade out on removal
- **Rim/center hub**: `currentColor`, set by CSS state classes based on `--accent-primary`

### CSS Classes

```css
.radar-icon               /* 32×32 SVG container, positioned in sidebar header gradient */
#targets-group g.ping     /* Target glow effect when sweep passes over */
```

### API

**Module:** `src/shared/radar-controller.js`

| Function | Purpose |
|----------|---------|
| `initRadar()` | Cache DOM refs, inject ping CSS, set initial state |
| `wsConnected()` | Set rim color to green, start sweep |
| `wsFlaky()` | Set rim color to amber, start sweep |
| `wsDisconnected()` | Set rim color to red, stop sweep, shake animation |
| `onHeartbeat()` | Flash sweep arm bright for 140ms |
| `addTarget(type, category)` | Add blip (`'triangle'`/`'square'`) at sweep position; `category` is `'ws'` or `'rest'`; returns unique ID |
| `removeTarget(id?)` | Flash sequence then fade out over ~1.67s; if no ID, removes oldest (FIFO) |
| `destroyRadar()` | Clean up animation, remove injected CSS |

### Integration Points

- **`main.js`** — Calls `initRadar()` on render, maps `ConnectionState` changes to `wsConnected()`/`wsFlaky()`/`wsDisconnected()`
- **`app-ws.js`** — Calls `addTarget()` on `send()`, `removeTarget()` on response (`keepalive_ok`, `chat_done`, `chat_error`), calls `onHeartbeat()` via `onSendActivity`
- **`conduit.js`** — All four API wrappers (`authQuery`, `authQueries`, `query`, `queries`) call `addTarget()` before the request and `removeTarget()` in a `finally` block

### Initialization Sequence

```
1. User logs in
2. MainManager.init() called
3. render() caches #radar-icon element
4. initRadar() called (sets red, starts no animation)
5. initWebSocket() called
6. ws.connect() establishes connection
7. onStateChange callback → wsConnected() (green, sweep starts)
8. Keepalive interval starts
9. Each keepalive → onHeartbeat() (arm flash)
10. Each conduit API call → addTarget()/removeTarget() (blips appear/disappear)
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

### Chunk Message Structure

Each `chat_chunk` message contains:

```json
{
  "type": "chat_chunk",
  "id": "crimson_1234567890_1",
  "chunk": {
    "content": "The answer is...",
    "reasoning_content": "Let me think about this...",
    "model": "kimi-k2.5",
    "index": 0,
    "finish_reason": null
  }
}
```

| Field | Type | Description |
|-------|------|-------------|
| `content` | string | Main response text (may be empty during reasoning phase) |
| `reasoning_content` | string | Model's thinking/reasoning process (e.g., Kimi K2.5) |
| `model` | string | Model name |
| `index` | number | Chunk sequence number |
| `finish_reason` | string | Present on final chunk (e.g., "stop") |

**Note**: Models like Kimi K2.5 send `reasoning_content` chunks first, then `content` chunks. The reasoning panel displays `reasoning_content` when toggled visible.

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

Last updated: March 29, 2026
