# WebSocket Interface

This document describes the WebSocket interface provided by the Hydrogen server for real-time status updates and interactions.

## Connection Details

- **Port:** 5001 (default, with automatic fallback to next available port)
- **Protocol:** hydrogen-protocol
- **Authorization:** Required, use `Authorization: Key abc` in header
- **Initialization:** Robust startup with port fallback and session validation
- **State Management:** Thread-safe context handling throughout connection lifecycle

The server will automatically try alternative ports (5001-5010) if the default port is unavailable. Check the server logs or use the system info endpoint to determine the actual bound port.

For complete configuration options including IPv6 support and logging settings, see the [Configuration Guide](./configuration.md#websocket).

## Status Updates

The WebSocket server provides real-time status information including:

- Server uptime
- Active connections
- Total connections
- Request metrics

### Example Connection

```javascript
const ws = new WebSocket('ws://localhost:5001', 'hydrogen-protocol');
ws.onopen = () => {
    console.log('Connected to Hydrogen WebSocket server');
};
ws.onmessage = (event) => {
    const status = JSON.parse(event.data);
    console.log('Received status update:', status);
};
```

## Message Format

Status messages are sent as JSON objects with the following structure:

```json
{
    "server_start_time": "ISO8601 timestamp",
    "active_connections": number,
    "total_connections": number,
    "total_requests": number,
    "system": {
        "sysname": "string",
        "nodename": "string",
        "release": "string",
        "version": "string",
        "machine": "string"
    },
    "status": {
        "running": boolean,
        "shutting_down": boolean,
        "totalThreads": number,
        "totalVirtualMemoryBytes": number,
        "totalResidentMemoryBytes": number
    }
}
```

### Message Fields

- **server_start_time**: ISO8601 formatted timestamp of server start
- **active_connections**: Current number of WebSocket connections
- **total_connections**: Total connections since server start
- **total_requests**: Total WebSocket requests processed
- **system**: Operating system and hardware information
- **status**: Current server status and resource usage

### Error Messages

Error responses follow this format:

```json
{
    "error": {
        "code": "string",
        "message": "string",
        "details": object
    }
}
```

Common error codes:

- `auth_failed`: Authentication failure
- `invalid_message`: Malformed message
- `server_error`: Internal server error

## Security

The WebSocket server implements multiple layers of security as documented in [SECRETS.md](SECRETS.md):

### Transport Security

- TLS encryption with modern cipher suites (see [network_architecture.md](./reference/network_architecture.md))
- Certificate validation using system CA store
- Secure WebSocket protocol (wss://) required in production

### Authentication

- API key or OIDC token required (see [oidc_architecture.md](./reference/oidc_architecture.md))
- RSA-signed session tokens with configurable expiration
- Secure token storage using environment variables

Example connection with API key:

```javascript
const ws = new WebSocket('wss://localhost:5001', 'hydrogen-protocol', {
    headers: {
        'Authorization': 'Key ${env.HYDROGEN_API_KEY}'
    }
});
```

Example connection with OIDC token:

```javascript
const ws = new WebSocket('wss://localhost:5001', 'hydrogen-protocol', {
    headers: {
        'Authorization': 'Bearer ${oidc_token}'
    }
});
```

### Message Security

- All messages encrypted in transit
- Input validation and sanitization
- Protection against replay attacks
- Rate limiting and abuse prevention

## Additional Features

More WebSocket features will be documented here as they are implemented, including:

- Print job status updates
- Temperature monitoring
- Command interface
- Error notifications

## Implementation Details

The WebSocket server uses a modular architecture designed for reliability, maintainability, and performance:

### Component Architecture

- **Core Server (websocket_server.c)**
  - Public API and initialization
  - Server lifecycle management
  - Configuration handling

- **Context Management (websocket_server_context.c)**
  - Centralized state management
  - Thread-safe metrics tracking
  - Resource lifecycle handling
  - Clean shutdown coordination

- **Connection Handling (websocket_server_connection.c)**
  - Connection lifecycle management
  - Thread registration/deregistration
  - Client information tracking
  - Connection metrics

- **Authentication (websocket_server_auth.c)**
  - Key-based authentication
  - Session state management
  - Security validation
  - Access control

- **Message Processing (websocket_server_message.c)**
  - Message validation and routing
  - JSON parsing/generation
  - Buffer management
  - Error handling

- **Event Dispatching (websocket_server_dispatch.c)**
  - Callback routing
  - Event state management
  - Error recovery
  - Shutdown coordination

### State Management

- Centralized WebSocketServerContext structure for all server state
- Thread-safe metric collection with mutex protection
- Clean state transitions with validation
- Resource tracking and proper cleanup
- Initialization state management:
  - Safe vhost creation sequence
  - Session validation during startup
  - Port binding with fallback mechanism
  - Context propagation to all components

### Shutdown Sequence

The WebSocket server implements a robust, multi-phase shutdown process:

1. **Signaling Phase**
   - Set `websocket_server_shutdown = 1` flag
   - Broadcast on condition variable to wake waiting threads
   - Allow threads to detect shutdown state and begin cleanup

2. **Connection Closure Phase**
   - Allow existing connections to complete normally (2s timeout)
   - Process remaining messages in queue
   - Clients receive proper closure frames

3. **Thread Termination Phase**
   - Call `stop_websocket_server()` to initiate thread termination
   - Join server thread with extended timeout (5s)
   - Use mutex protection for thread state access

4. **Resource Cleanup Phase**
   - Two-phase pointer handling to prevent use-after-free
   - Wait for callbacks to complete before context destruction
   - Increased delay (250ms) before final resource deallocation
   - Use local copy of context before nullifying global pointer

5. **Fallback Mechanisms**
   - Progressive timeouts with shorter intervals for forced cleanup
   - Thread cancellation if normal termination fails
   - Forced context destruction with proper synchronization
   - Explicit thread status verification

This approach ensures that even under heavy load or in error conditions, the WebSocket server will shut down cleanly without resource leaks, thread hangs, or race conditions.

For a complete overview of the shutdown architecture across all components, see the [Shutdown Architecture](./shutdown_architecture.md) document.

### Memory Management

- Zero-copy message passing where possible
- Memory pooling for frequent operations
- Efficient buffer management
- Minimal dynamic allocations
- Resource cleanup on disconnect

### Concurrency and Safety

- Mutex-protected shared state
- Atomic metric updates
- Thread-safe connection handling
- Proper shutdown synchronization
- Race condition prevention

### Error Recovery

- Automatic reconnection support
- Graceful error handling
- Resource cleanup on disconnect
- Partial message recovery

### Performance Optimizations

- Efficient JSON generation
- Message batching
- Connection pooling
- Low-latency delivery

## Contributing

When adding new WebSocket features:

1. Document the message format
2. Include connection examples
3. Update security requirements if changed
4. Add error handling information
5. Follow memory management best practices
6. Ensure thread safety in implementations
