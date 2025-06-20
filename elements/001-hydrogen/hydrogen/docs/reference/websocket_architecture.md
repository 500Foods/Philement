# WebSocket Server Architecture

This document provides a detailed overview of the WebSocket server component in Hydrogen, explaining its internal architecture, communication patterns, and implementation details.

## Overview

The WebSocket server is a core component of Hydrogen that provides real-time bidirectional communication between clients and the server. It enables:

- Real-time status updates from the server to clients
- Command and control messages from clients to the server
- Event-driven notifications for system state changes
- Authenticated and secure communication channels
- Guaranteed message delivery with acknowledgment

## System Architecture

The WebSocket server follows a layered architecture with clear separation of concerns:

```diagram
┌──────────────────────────────────────────────────┐
│               WebSocket Server                   │
│                                                  │
│   ┌────────────┐    ┌────────────┐    ┌───────┐  │
│   │ Connection │    │  Message   │    │ Auth  │  │
│   │  Manager   │    │ Processor  │    │Service│  │
│   └────────────┘    └────────────┘    └───────┘  │
│          │                 │               │     │
│          ▼                 ▼               ▼     │
│   ┌────────────┐    ┌────────────┐    ┌───────┐  │
│   │  Context   │    │ Dispatcher │    │Status │  │
│   │  Handler   │    │            │    │Service│  │
│   └────────────┘    └────────────┘    └───────┘  │
│                                                  │
└──────────────────────────────────────────────────┘
           │                │               │
           ▼                ▼               ▼
┌───────────────┐    ┌─────────────┐    ┌──────────┐
│ libwebsockets │    │ Application │    │  System  │
│     Core      │    │  Services   │    │  Status  │
└───────────────┘    └─────────────┘    └──────────┘
```

### Key Components

1. **Connection Manager**: Handles client connections, disconnections, and session tracking
2. **Message Processor**: Parses, validates, and processes incoming messages
3. **Auth Service**: Manages authentication and authorization
4. **Context Handler**: Maintains connection context and state
5. **Dispatcher**: Routes messages to appropriate application services
6. **Status Service**: Broadcasts system status updates to clients

## Data Flow

```diagram
┌───────────┐         ┌───────────────┐         ┌─────────────┐
│  Client   │         │   WebSocket   │         │ Application │
│  Browser  │◄────────┤    Server     ├────────►│  Services   │
└─────┬─────┘         └───────┬───────┘         └─────────────┘
      │                       │                        │
      │                       │                        │
      │                       ▼                        │
      │               ┌───────────────┐                │
      │               │  Connection   │                │
      │◄──────────────┤   Context     ├───────────────►│
      │               └───────────────┘                │
      │                                                │
      │               ┌───────────────┐                │
      │               │    System     │                │
      └──────────────►│    Events     │◄───────────────┘
                      └───────────────┘
```

### Key Data Flows

1. **Client to Server (Command Flow)**:
   - Client establishes WebSocket connection
   - Authentication handshake occurs
   - Client sends JSON-formatted commands
   - Server validates message format and authentication
   - Command is dispatched to appropriate service
   - Response or acknowledgment sent back to client

2. **Server to Client (Event Flow)**:
   - System event occurs (state change, error, update)
   - Event is formatted as JSON message
   - Connection contexts are queried for interested clients
   - Event is broadcast to relevant clients
   - Optional acknowledgment tracked for delivery guarantee

3. **Connection Management Flow**:
   - New connections tracked in connection registry
   - Session information maintained with connection context
   - Heartbeat mechanism verifies connection health
   - Disconnections trigger resource cleanup
   - Reconnection logic handles transient network issues

## Message Structure

WebSocket messages follow a consistent structure to ensure proper routing and processing:

```json
{
  "type": "command|event|response|ack",
  "id": "unique-message-identifier",
  "timestamp": "2025-02-26T18:45:00.000Z",
  "channel": "system|print|network|file",
  "action": "specific-command-or-event",
  "data": {
    // Command-specific or event-specific payload
  },
  "auth": {
    "token": "authentication-token",
    "signature": "message-signature-if-required"
  }
}
```

### Message Types

1. **Command Messages** (Client → Server):
   - Requests for action or information
   - Require authentication
   - Always receive a response

2. **Event Messages** (Server → Client):
   - Notifications of state changes
   - System status updates
   - May require acknowledgment

3. **Response Messages** (Server → Client):
   - Direct replies to commands
   - Include success/failure status
   - Contain requested data if applicable

4. **Acknowledgment Messages** (Client → Server):
   - Confirm receipt of an event
   - Simple format with event ID
   - Used for guaranteed delivery

## Authentication System

The WebSocket server implements a multi-layer authentication system:

```diagram
┌─────────────┐    ┌────────────────┐    ┌─────────────┐
│  Initial    │    │   Session      │    │  Message    │
│ Handshake   │───►│ Authentication │───►│ Validation  │
└─────────────┘    └────────────────┘    └─────────────┘
       ▲                   ▲                    ▲
       │                   │                    │
┌─────────────┐    ┌────────────────┐    ┌─────────────┐
│ HTTP Header │    │  Auth Token    │    │  Message    │
│ Validation  │    │  Verification  │    │ Signatures  │
└─────────────┘    └────────────────┘    └─────────────┘
```

1. **Connection Authentication**:
   - HTTP header contains `Authorization: Key <token>`
   - Protocol must be `hydrogen-protocol`
   - Initial validation occurs during handshake

2. **Session Authentication**:
   - After connection, session token verified
   - Token mapped to user identity and permissions
   - Session context created and maintained

3. **Message Authentication**:
   - Each message includes authentication token
   - Optional message signing for sensitive commands
   - Authorization checks for specific actions

## Connection State Machine

WebSocket connections follow a defined state machine:

```diagram
┌───────────┐      ┌───────────┐      ┌───────────┐
│ Connecting│─────►│ Verifying │─────►│Established│
└─────┬─────┘      └─────┬─────┘      └─────┬─────┘
      │                  │                  │
      │                  │                  │
      ▼                  ▼                  ▼
┌───────────┐      ┌───────────┐      ┌───────────┐
│  Failed   │      │  Rejected │      │ Connected │◄───┐
└───────────┘      └───────────┘      └─────┬─────┘    │
                                            │          │
                                            │          │
                                            ▼          │
                                      ┌───────────┐    │
                                      │ Heartbeat │────┘
                                      └─────┬─────┘
                                            │
                                            │
                                            ▼
                                      ┌───────────┐      ┌───────────┐
                                      │ Inactive  │─────►│ Closing   │
                                      └───────────┘      └─────┬─────┘
                                                               │
                                                               │
                                                               ▼
                                                         ┌───────────┐
                                                         │   Closed  │
                                                         └───────────┘
```

Each state transition triggers specific actions:

- **Connecting → Verifying**: Initial connection validation
- **Verifying → Established**: Authentication succeeded
- **Established → Connected**: Session context created
- **Connected → Heartbeat**: Regular keepalive cycle
- **Heartbeat → Inactive**: No recent activity detected
- **Inactive → Closing**: Closing connection due to inactivity
- **Connected → Closing**: Client or server initiated close
- **Closing → Closed**: Connection terminated and resources released

## Thread Safety Model

The WebSocket server employs a sophisticated thread safety model:

```diagram
┌──────────────────────────────────────────────────────┐
│ WebSocket Server Main Thread                         │
│ ┌─────────────────────────────────────────────────┐  │
│ │ Connection Management / Initial Authentication  │  │
│ └─────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
                          │
                          │ (libwebsockets event loop)
                          ▼
┌──────────────────────────────────────────────────────┐
│ Service Thread Pool                                  │
│ ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│ │   Message   │  │ Connection  │  │    Auth     │    │
│ │  Processing │  │  Handling   │  │  Validation │    │
│ └─────────────┘  └─────────────┘  └─────────────┘    │
└──────────────────────────────────────────────────────┘
                          │
                          │ (thread-safe queues)
                          ▼
┌──────────────────────────────────────────────────────┐
│ Application Service Threads                          │
│ ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│ │    Print    │  │   System    │  │    File     │    │
│ │   Service   │  │   Service   │  │   Service   │    │
│ └─────────────┘  └─────────────┘  └─────────────┘    │
└──────────────────────────────────────────────────────┘
```

Key thread safety mechanisms:

1. **Mutex Protection**:
   - Connection registry protected by connection_mutex
   - Context data protected by context_mutex
   - Message queue protected by queue_mutex

2. **Thread-Local Storage**:
   - Connection-specific data stored in thread-local storage
   - Prevents race conditions during connection handling

3. **Atomic Operations**:
   - Status flags use atomic operations
   - Connection counters use atomic increments/decrements

4. **Lock-Free Queues**:
   - Message passing uses lock-free queues where possible
   - Minimizes contention in high-throughput scenarios

## Memory Management

The WebSocket server implements careful memory management:

1. **Connection Context Lifecycle**:

   ```diagram
   ┌───────────┐     ┌───────────┐     ┌───────────┐
   │ Allocate  │────►│  In Use   │────►│ Marked for│
   │ Context   │     │           │     │ Deletion  │
   └───────────┘     └───────────┘     └─────┬─────┘
                                             │
                                             │
                                             ▼
                                       ┌───────────┐
                                       │  Zombie   │
                                       │  State    │
                                       └─────┬─────┘
                                             │
                                             │
                                             ▼
                                       ┌───────────┐
                                       │  Freed    │
                                       │           │
                                       └───────────┘
   ```

2. **Two-Phase Destruction**:
   - References marked inactive before memory release
   - Final release only when all references gone
   - Prevents use-after-free scenarios

3. **Resource Tracking**:
   - Each connection tracks allocated resources
   - Automatic cleanup on connection close
   - Periodic garbage collection for orphaned resources

## Configuration Options

The WebSocket server has multiple configuration options:

```json
{
  "WebSocketServer": {
    "Enabled": true,
    "Port": 8080,
    "Interface": "0.0.0.0",
    "EnableIPv6": true,
    "SecureMode": "tls",  // "none", "tls"
    "CertificatePath": "/etc/hydrogen/cert.pem",
    "KeyPath": "/etc/hydrogen/key.pem",
    "AuthRequired": true,
    "IdleTimeout": 300,
    "MaxConnections": 100,
    "ThreadPoolSize": 4
  }
}
```

## Integration With libwebsockets

The WebSocket server is built on top of libwebsockets, with careful integration:

```diagram
┌─────────────────────────────────────────────────┐
│             Hydrogen WebSocket Server           │
│                                                 │
│  ┌───────────────┐        ┌──────────────────┐  │
│  │ Hydrogen API  │        │ Connection Mgmt  │  │
│  └───────┬───────┘        └────────┬─────────┘  │
│          │                         │            │
│          ▼                         ▼            │
│  ┌───────────────────────────────────────────┐  │
│  │           Integration Layer               │  │
│  └───────────────────┬───────────────────────┘  │
│                      │                          │
└──────────────────────┼──────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────┐
│              libwebsockets Library               │
│                                                  │
│   ┌────────────┐   ┌────────────┐   ┌─────────┐  │
│   │ Connection │   │   Event    │   │  TLS    │  │
│   │   API      │   │   Loop     │   │ Support │  │
│   └────────────┘   └────────────┘   └─────────┘  │
│                                                  │
└──────────────────────────────────────────────────┘
```

Key integration points:

1. **Protocol Handlers**:
   - Custom hydrogen-protocol implementation
   - Handler functions for connection events
   - Message processing callbacks

2. **Event Loop Integration**:
   - libwebsockets event-driven model
   - Non-blocking I/O operations
   - Custom service intervals

3. **Context Management**:
   - vhost and context creation
   - Protocol registration
   - TLS certificate configuration

## Event Dispatching System

The WebSocket server implements a sophisticated event dispatching system:

```diagram
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Event Source   │───►│  Event Router   │───►│ Channel Filter  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
                                                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Connection     │◄───│ Message Encoder │◄───│  Subscription   │
│   Iterator      │    │                 │    │    Matcher      │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │
         ▼
┌─────────────────┐    ┌─────────────────┐
│  Send Queue     │───►│  Delivery       │
│                 │    │  Confirmation   │
└─────────────────┘    └─────────────────┘
```

This system enables:

- Topic-based subscriptions
- Client-specific filtering
- Guaranteed delivery
- Efficient broadcast to many clients
- Prioritized message delivery

## Error Handling Strategy

The WebSocket server implements comprehensive error handling:

1. **Categorized Errors**:
   - Connection errors (network, TLS)
   - Protocol errors (malformed messages)
   - Authentication errors (invalid credentials)
   - Application errors (service issues)

2. **Error Recovery**:
   - Automatic reconnection for transient issues
   - Session recovery after disconnection
   - Message retry for failed deliveries

3. **Error Reporting**:
   - Structured error responses to clients
   - Detailed server-side logging
   - Metrics for error tracking and analysis

## Security Considerations

The WebSocket server implements several security measures:

1. **Transport Security**:
   - TLS encryption for all connections
   - Modern cipher suites only
   - Certificate validation

2. **Authentication**:
   - Token-based authentication
   - Session validation
   - Permission checking

3. **Input Validation**:
   - Message format verification
   - Size limits on messages
   - Content sanitization

4. **Rate Limiting**:
   - Connection rate limiting
   - Message rate limiting
   - Automatic blocking of abusive clients

## Implementation Files

The WebSocket server is implemented across these files:

| File | Purpose |
|------|---------|
| `src/websocket/websocket_server.c` | Main implementation file |
| `src/websocket/websocket_server.h` | Public interface |
| `src/websocket/websocket_server_internal.h` | Internal definitions |
| `src/websocket/websocket_server_auth.c` | Authentication system |
| `src/websocket/websocket_server_connection.c` | Connection management |
| `src/websocket/websocket_server_context.c` | Context handling |
| `src/websocket/websocket_server_dispatch.c` | Message dispatching |
| `src/websocket/websocket_server_message.c` | Message processing |
| `src/websocket/websocket_server_status.c` | Status reporting |

## Usage Examples

### Server Initialization

```c
// Initialize WebSocket server configuration
websocket_config_t config;
memset(&config, 0, sizeof(websocket_config_t));
config.port = 8080;
config.interface = "0.0.0.0";
config.secure_mode = WEBSOCKET_SECURE_TLS;
config.cert_path = "/etc/hydrogen/cert.pem";
config.key_path = "/etc/hydrogen/key.pem";
config.auth_required = true;
config.idle_timeout = 300;
config.max_connections = 100;

// Register event handlers
websocket_register_handler("system", system_event_handler);
websocket_register_handler("print", print_event_handler);
websocket_register_handler("file", file_event_handler);

// Start WebSocket server
websocket_server_context_t* context = websocket_server_start(&config);
if (!context) {
    log_this("WebSocket", "Failed to start WebSocket server", LOG_LEVEL_ERROR, true, true, true);
    return false;
}

return true;
```

### Sending Events

```c
// Create a system status event
json_t* event = json_object();
json_object_set_new(event, "type", json_string("event"));
json_object_set_new(event, "id", json_string(generate_uuid()));
json_object_set_new(event, "timestamp", json_string(get_iso_timestamp()));
json_object_set_new(event, "channel", json_string("system"));
json_object_set_new(event, "action", json_string("status_update"));

// Add event data
json_t* data = json_object();
json_object_set_new(data, "server_running", json_boolean(server_running));
json_object_set_new(data, "memory_usage", json_integer(get_memory_usage()));
json_object_set_new(data, "cpu_load", json_real(get_cpu_load()));
json_object_set_new(data, "thread_count", json_integer(get_thread_count()));
json_object_set_new(event, "data", data);

// Broadcast to all authenticated clients
websocket_broadcast_message(context, event, "system");

// Free JSON object (copied during send)
json_decref(event);
```

### Processing Client Messages

```c
// WebSocket message handler
int process_websocket_message(websocket_server_context_t* context, 
                             websocket_connection_t* connection,
                             const char* message, size_t length) {
    // Parse JSON message
    json_error_t error;
    json_t* root = json_loads(message, 0, &error);
    if (!root) {
        log_this("WebSocket", "Invalid JSON: %s", LOG_LEVEL_ALERTING, error.text);
        return -1;
    }
    
    // Extract message details
    const char* type = json_string_value(json_object_get(root, "type"));
    const char* channel = json_string_value(json_object_get(root, "channel"));
    const char* action = json_string_value(json_object_get(root, "action"));
    json_t* data = json_object_get(root, "data");
    
    // Verify authentication
    if (!websocket_verify_auth(connection, root)) {
        log_this("WebSocket", "Authentication failed for message", LOG_LEVEL_ALERTING, true, true, true);
        json_decref(root);
        return -1;
    }
    
    // Route to appropriate handler
    int result = -1;
    if (strcmp(type, "command") == 0 && channel && action) {
        // Find registered handler for this channel
        websocket_handler_t* handler = websocket_find_handler(context, channel);
        if (handler) {
            result = handler->process_command(connection, action, data);
        } else {
            log_this("WebSocket", "No handler for channel: %s", LOG_LEVEL_ALERTING, channel);
        }
    } else if (strcmp(type, "ack") == 0) {
        // Process acknowledgment
        const char* message_id = json_string_value(json_object_get(root, "id"));
        if (message_id) {
            websocket_mark_delivered(context, message_id, connection);
            result = 0;
        }
    }
    
    // Free JSON object
    json_decref(root);
    return result;
}
```

## Related Documentation

- [WebSocket Interface](../web_socket.md) - General WebSocket documentation
- [API Documentation](../reference/api.md) - API endpoints that integrate with WebSockets
- [Configuration Guide](../reference/configuration.md) - Configuration options
- [Security Overview](/docs/reference/security.md) - Security considerations
