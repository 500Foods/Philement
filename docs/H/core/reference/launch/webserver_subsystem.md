# WebServer Subsystem Launch Process

This document details the complete launch and landing process for the WebServer subsystem in Hydrogen.

## Overview

The WebServer subsystem provides HTTP services and serves as a foundation for other web-based features. It depends on the Network subsystem and is required by several other subsystems like Terminal and Swagger.

## Launch Process

```diagram
┌─────────────────────┐
│  Begin WebServer    │
│  Launch Process     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  System State Check │
│  - Not stopping     │
│  - Not shutdown     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Config Check       │
│  - Enabled          │
│  - Valid port       │
│  - Valid paths      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Dependency Check   │
│  - Network ready    │
│  - Logging ready    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Resource Check     │
│  - Port available   │
│  - File limits      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Launch Decision    │───────┐
└──────────┬──────────┘       │
           │                  │
           ▼                  ▼
    [Success Path]     [Failure Path]
           │                  │
           ▼                  ▼
┌─────────────────┐  ┌───────────────┐
│ Start Server &  │  │ Log Error &   │
│ Register        │  │ Exit Launch   │
└─────────────────┘  └───────────────┘
```

### Launch Readiness Checks

The WebServer subsystem performs these checks in sequence:

1. **System State Check**

   ```c
   if (web_server_shutdown) {
       readiness->status_messages[4] = strdup("No-Go: Subsystem is in shutdown state");
       return false;
   }
   ```

   - Verifies not in shutdown state
   - Checks system running status

2. **Configuration Check**

   ```c
   if (!app_config->webserver.enabled) {
       readiness->status_messages[0] = strdup("No-Go: WebServer is disabled");
       return false;
   }
   if (app_config->webserver.port < 1 || app_config->webserver.port > 65535) {
       readiness->status_messages[0] = strdup("No-Go: Invalid port configuration");
       return false;
   }
   ```

   - Checks if enabled in configuration
   - Validates port number
   - Verifies path configurations

3. **Dependency Check**

   ```c
   if (!is_subsystem_running_by_name("Network")) {
       readiness->status_messages[1] = strdup("No-Go: Required dependency 'Network' not running");
       return false;
   }
   ```

   - Verifies Network subsystem is running
   - Checks Logging subsystem status

4. **Resource Check**

   ```c
   // Check port availability
   if (!is_port_available(app_config->webserver.port)) {
       readiness->status_messages[2] = strdup("No-Go: Port already in use");
       return false;
   }
   ```

   - Verifies port availability
   - Checks file descriptor limits

### Launch Subsystem Process

After passing readiness checks, the WebServer subsystem:

1. **Initialize Server**

   ```c
   bool launch_webserver_subsystem(void) {
       // Initialize core server
       if (!init_web_server()) {
           log_this("WebServer", "Failed to initialize", LOG_LEVEL_ERROR);
           return false;
       }
       
       // Start server thread
       if (pthread_create(&webserver_thread, NULL, web_server_main, NULL) != 0) {
           log_this("WebServer", "Failed to start thread", LOG_LEVEL_ERROR);
           return false;
       }
       
       return true;
   }
   ```

2. **Register with Registry**

   ```c
   int webserver_id = register_subsystem_from_launch(
       "WebServer",
       &webserver_threads,
       &webserver_thread,
       &web_server_shutdown,
       init_webserver_subsystem,
       shutdown_web_server
   );
   
   // Add dependencies
   if (webserver_id >= 0) {
       add_dependency_from_launch(webserver_id, "Network");
   }
   ```

## Landing Process

```diagram
┌─────────────────────┐
│  Begin WebServer    │
│  Landing Process    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Set Shutdown Flag  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Close Connections  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Stop Server Thread │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Free Resources     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Landing Complete   │
└─────────────────────┘
```

The landing process is handled by `shutdown_web_server()`:

```c
void shutdown_web_server(void) {
    // Signal shutdown
    web_server_shutdown = true;
    
    // Close all active connections
    close_all_connections();
    
    // Wait for server thread
    if (webserver_thread) {
        pthread_join(webserver_thread, NULL);
    }
    
    // Cleanup resources
    cleanup_web_server();
    
    log_this("WebServer", "Shutdown complete", LOG_LEVEL_STATE);
}
```

## Error Handling

The WebServer implements comprehensive error handling:

1. **Connection Management**
   - Graceful connection closure
   - Resource cleanup on errors
   - Connection limit enforcement

2. **Thread Safety**
   - Mutex protection for shared resources
   - Thread synchronization during shutdown
   - Safe thread termination

3. **Resource Management**
   - Socket cleanup
   - Memory deallocation
   - File descriptor management

## Logging

The subsystem provides detailed logging:

```c
// Launch logging
log_this("WebServer", "Starting on port %d", LOG_LEVEL_STATE, port);
log_this("WebServer", "Server thread started", LOG_LEVEL_STATE);

// Operation logging
log_this("WebServer", "Connection from %s", LOG_LEVEL_DEBUG, client_ip);
log_this("WebServer", "Request: %s %s", LOG_LEVEL_DEBUG, method, path);

// Error logging
log_this("WebServer", "Failed to bind: %s", LOG_LEVEL_ERROR, strerror(errno));
log_this("WebServer", "Connection error: %s", LOG_LEVEL_ERROR, error_msg);

// Shutdown logging
log_this("WebServer", "Beginning shutdown", LOG_LEVEL_STATE);
log_this("WebServer", "All connections closed", LOG_LEVEL_STATE);
```

## Integration Points

The WebServer subsystem integrates with:

1. **Network Subsystem**
   - Socket operations
   - Network interface binding
   - Connection management

2. **Logging Subsystem**
   - Request logging
   - Error reporting
   - Status updates

3. **Configuration System**
   - Port configuration
   - Server settings
   - Resource limits

4. **Subsystem Registry**
   - State management
   - Dependency tracking
   - Lifecycle control

## Related Documentation

- [Launch System Architecture](/docs/H/core/reference/launch_system_architecture.md)
- [Network Subsystem](../network_architecture.md)
- [Configuration System](../configuration.md)
- [Subsystem Registry](../subsystem_registry_architecture.md)
