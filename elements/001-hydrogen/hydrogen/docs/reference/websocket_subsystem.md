# WebSocket Subsystem

This document provides a comprehensive overview of the WebSocket subsystem in Hydrogen, detailing its architecture, implementation, and integration with the Launch System.

## Overview

The WebSocket subsystem is a key component of Hydrogen that provides real-time bidirectional communication between clients and the server. It enables push notifications, live updates, and interactive control of 3D printers. The WebSocket implementation follows the RFC 6455 standard and provides a reliable, secure channel for real-time data exchange.

## System Architecture

The WebSocket subsystem implements a multi-threaded architecture with specialized components for connection management, message processing, and dispatch:

```diagram
┌───────────────────────────────────────────────────────────────────┐
│                      WebSocket Subsystem                          │
│                                                                   │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │  Listener       │           │ Connection      │                │
│  │     Thread      │◄─────────►│   Manager       │                │
│  └─────────────────┘           └─────────────────┘                │
│          │                              │                         │
│          ▼                              ▼                         │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │ Authentication  │           │ Client Session  │                │
│  │    Handler      │           │   Tracking      │                │
│  └─────────────────┘           └─────────────────┘                │
│          │                              │                         │
│          ▼                              ▼                         │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │ Message         │           │ Message         │                │
│  │ Processing      │◄─────────►│ Dispatch        │                │
│  └─────────────────┘           └─────────────────┘                │
│                                        │                          │
│                      ┌─────────────────┼──────────────────┐       │
│                      │                 │                  │       │
│               ┌──────▼─────┐    ┌──────▼─────┐     ┌──────▼─────┐ │
│               │   Status   │    │   Control  │     │   Event    │ │
│               │  Channel   │    │  Channel   │     │  Channel   │ │
│               └────────────┘    └────────────┘     └────────────┘ │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Listener Thread**: Accepts WebSocket connection upgrades from HTTP connections
2. **Connection Manager**: Tracks and manages active WebSocket connections
3. **Authentication Handler**: Validates client credentials and permissions
4. **Client Session Tracking**: Maintains client state and session information
5. **Message Processing**: Handles incoming WebSocket messages (text and binary)
6. **Message Dispatch**: Routes messages to appropriate handlers and channels
7. **Channels**: Logical groupings for different message types (status, control, events)

## Implementation Details

The WebSocket subsystem is implemented with these major components:

### Threading Model

The WebSocket server uses a dedicated thread pool architecture:

```c
// WebSocket thread pool
static ServiceThreads websocket_threads = {0};

// Main thread handle
static pthread_t websocket_thread;

// Shutdown flag
volatile sig_atomic_t websocket_shutdown = 0;
```

The server creates:

- A main listener thread that manages the upgrade from HTTP to WebSocket
- Worker threads to handle client connections
- A dispatch thread for message routing

### WebSocket Protocol Implementation

The implementation follows the WebSocket RFC 6455 protocol, handling:

1. **Handshake Process**:
   - HTTP Upgrade verification
   - Sec-WebSocket-Key validation
   - Protocol negotiation

2. **Frame Processing**:
   - Frame header parsing
   - Masking/unmasking
   - Fragmentation handling
   - Control frames (Ping/Pong/Close)

3. **Message Types**:
   - Text messages (UTF-8)
   - Binary messages
   - Control messages

### Connection States

```c
typedef enum {
    WS_STATE_CONNECTING,  // Initial handshake
    WS_STATE_OPEN,        // Active connection
    WS_STATE_CLOSING,     // Closing handshake
    WS_STATE_CLOSED       // Connection terminated
} WebSocketState;
```

### Message Handling

The WebSocket subsystem processes messages using a staged approach:

```diagram
┌─────────────────┐
│ Receive Message │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Parse Frame     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Assemble Message│
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│ Handle Control  │─Yes─► Process Control  │
│ Frame?          │     │ Frame           │
└────────┬────────┘     └─────────────────┘
         │ No
         ▼
┌─────────────────┐
│ Parse Message   │
│ Content         │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Dispatch to     │
│ Handler         │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Send Response   │
└─────────────────┘
```

## Integration with Launch System

The WebSocket subsystem integrates with the Hydrogen Launch System through a dedicated launch readiness check:

```c
bool check_websocket_launch_readiness(LaunchReadiness* readiness) {
    // Check configuration
    readiness->go_config = app_config && 
                         app_config->websocket.enabled && 
                         app_config->websocket.port > 0 && 
                         app_config->websocket.port <= 65535;
    
    // Check dependencies
    readiness->go_dependencies = is_subsystem_running_by_name("Logging") &&
                               is_subsystem_running_by_name("WebServer");
    
    // Check resources (ports, file descriptors, etc.)
    readiness->go_resources = true;  // Resource checks
    
    // Check system conditions
    readiness->go_system = true;  // System checks
    
    // Check state (not in shutdown)
    readiness->go_state = !websocket_shutdown;
    
    // Final decision
    readiness->go_final = readiness->go_config && 
                        readiness->go_dependencies && 
                        readiness->go_resources && 
                        readiness->go_system && 
                        readiness->go_state;
    
    // Provide status messages
    // (Status message generation code)
    
    return readiness->go_final;
}
```

### Launch Prerequisites

The WebSocket subsystem has the following launch requirements:

1. **Configuration**:
   - Server must be enabled in configuration
   - Port must be valid (1-65535)
   - Protocol settings must be valid

2. **Dependencies**:
   - Logging subsystem must be running
   - WebServer subsystem must be running

3. **Resources**:
   - Configured port must be available
   - Sufficient file descriptors must be available

4. **State**:
   - Not already in shutdown state

## Initialization Process

The WebSocket subsystem initialization follows this sequence:

```c
bool init_websocket_subsystem(void) {
    // Block initialization during shutdown
    if (server_stopping || websocket_shutdown) {
        return false;
    }
    
    // Apply configuration
    int port = app_config->websocket.port;
    int max_connections = app_config->websocket.max_connections;
    
    // Initialize data structures
    if (!init_websocket_connection_manager(max_connections)) {
        log_this("WebSocket", "Failed to initialize connection manager", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Initialize message handlers
    if (!init_websocket_message_handlers()) {
        log_this("WebSocket", "Failed to initialize message handlers", LOG_LEVEL_ERROR);
        cleanup_websocket_connection_manager();
        return false;
    }
    
    // Start main thread
    if (pthread_create(&websocket_thread, NULL, websocket_main, NULL) != 0) {
        log_this("WebSocket", "Failed to create main thread", LOG_LEVEL_ERROR);
        cleanup_websocket_message_handlers();
        cleanup_websocket_connection_manager();
        return false;
    }
    
    // Register with thread tracking
    register_thread(&websocket_threads, websocket_thread, "websocket_main");
    
    log_this("WebSocket", "Initialized on port %d with max %d connections", 
            LOG_LEVEL_STATE, port, max_connections);
    return true;
}
```

## Shutdown Process

The WebSocket subsystem implements a clean shutdown sequence:

```c
void stop_websocket_server(void) {
    // Signal shutdown
    websocket_shutdown = 1;
    
    // Close all active connections
    websocket_close_all_connections(1001, "Server Shutdown");
    
    // Wait for main thread to exit
    if (websocket_thread != 0) {
        pthread_join(websocket_thread, NULL);
        websocket_thread = 0;
    }
    
    // Clean up resources
    cleanup_websocket_message_handlers();
    cleanup_websocket_connection_manager();
    
    log_this("WebSocket", "Shutdown complete", LOG_LEVEL_STATE);
}
```

## Configuration Options

The WebSocket subsystem is configured through the `WebSocket` section in the Hydrogen configuration:

```json
{
  "WebSocket": {
    "Enabled": true,
    "Port": 8081,
    "MaxConnections": 100,
    "IdleTimeout": 300,
    "MaxMessageSize": 65536,
    "HandshakeTimeout": 10,
    "PingInterval": 30,
    "Protocols": "hydrogen-v1",
    "AllowOrigins": "*",
    "Authentication": {
      "Required": true,
      "Methods": "token,basic,oidc"
    }
  }
}
```

### Key Configuration Parameters

| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `Enabled` | Enable/disable the WebSocket server | `true` | `true`/`false` |
| `Port` | Server listening port | `8081` | 1-65535 |
| `MaxConnections` | Maximum simultaneous connections | `100` | 1-10000 |
| `IdleTimeout` | Connection idle timeout (seconds) | `300` | 1-3600 |
| `MaxMessageSize` | Maximum message size (bytes) | `65536` | 1024-1048576 |
| `HandshakeTimeout` | Handshake timeout (seconds) | `10` | 1-60 |
| `PingInterval` | Ping message interval (seconds) | `30` | 1-300 |
| `Protocols` | Supported subprotocols | `hydrogen-v1` | Comma-separated list |
| `AllowOrigins` | Allowed origin domains | `*` | Domain list or `*` |

## Channel System

The WebSocket subsystem organizes communication through logical channels:

```c
// Register a message handler for a specific channel
bool websocket_register_channel_handler(
    const char* channel,
    websocket_message_handler handler,
    void* user_data
);

// Send a message on a specific channel
bool websocket_broadcast_to_channel(
    const char* channel,
    const char* message,
    size_t message_len,
    websocket_client_filter filter
);
```

### Standard Channels

The WebSocket subsystem provides several standard channels:

| Channel | Purpose | Message Format |
|---------|---------|----------------|
| `system` | System status updates | JSON |
| `print` | Print job updates | JSON |
| `control` | Printer control commands | JSON |
| `log` | Log streaming | Text |
| `terminal` | Terminal emulation | Text |
| `notification` | User notifications | JSON |

## Security Features

The WebSocket implementation includes several security features:

1. **Origin Checking**: Validates the Origin header against allowed domains
2. **Authentication**: Supports multiple authentication methods
3. **Permission Checking**: Enforces access controls on channels
4. **Message Validation**: Validates message format and content
5. **Rate Limiting**: Prevents abuse with connection and message rate limits

## Implementation Files

The WebSocket implementation is spread across these files:

| File | Purpose |
|------|---------|
| `src/websocket/websocket_server.c` | Core server implementation |
| `src/websocket/websocket_server.h` | Public interface |
| `src/websocket/websocket_server_connection.c` | Connection management |
| `src/websocket/websocket_server_message.c` | Message processing |
| `src/websocket/websocket_server_dispatch.c` | Message routing |
| `src/websocket/websocket_server_auth.c` | Authentication |
| `src/websocket/websocket_server_context.c` | Client context tracking |
| `src/websocket/websocket_dynamic.c` | Dynamic module loading |
| `src/state/startup_websocket.c` | Initialization integration |
| `src/config/launch/websocket.c` | Launch readiness check |

## Integration Example

Example of registering a message handler:

```c
// Handler function
void handle_control_messages(
    websocket_connection_t* connection,
    const char* message,
    size_t message_len,
    void* user_data
) {
    // Parse JSON message
    cJSON* json = cJSON_ParseWithLength(message, message_len);
    if (!json) {
        websocket_send_text(connection, "{\"error\":\"Invalid JSON\"}", 23);
        return;
    }
    
    // Extract command
    cJSON* cmd = cJSON_GetObjectItem(json, "command");
    if (!cmd || !cJSON_IsString(cmd)) {
        websocket_send_text(connection, "{\"error\":\"Missing command\"}", 26);
        cJSON_Delete(json);
        return;
    }
    
    // Handle command
    if (strcmp(cmd->valuestring, "status") == 0) {
        // Send status response
        websocket_send_text(connection, "{\"status\":\"ready\"}", 19);
    } else {
        // Unknown command
        websocket_send_text(connection, "{\"error\":\"Unknown command\"}", 28);
    }
    
    // Clean up
    cJSON_Delete(json);
}

// Register handler during initialization
websocket_register_channel_handler("control", handle_control_messages, NULL);
```

## Error Handling

The WebSocket subsystem implements comprehensive error handling:

1. **Protocol Errors**: Properly handles malformed frames
2. **Connection Issues**: Manages connection drops gracefully
3. **Resource Exhaustion**: Handles resource limits appropriately
4. **Authentication Failures**: Provides clear error messages
5. **Message Processing Errors**: Isolates errors to prevent system-wide issues

## Related Documentation

For more information on WebSocket-related topics, see:

- [WebSocket Configuration](websocket_configuration.md) - Configuration details
- [Launch System Architecture](launch_system_architecture.md) - Launch system integration
- [WebServer Subsystem](webserver_subsystem.md) - Related HTTP server
- [Subsystem Registry Architecture](subsystem_registry_architecture.md) - Subsystem management