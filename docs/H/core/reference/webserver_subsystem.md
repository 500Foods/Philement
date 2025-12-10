# WebServer Subsystem

This document provides a detailed explanation of the WebServer subsystem within Hydrogen, including its architecture, implementation details, and integration with the Launch System.

## Overview

The WebServer subsystem is a core component of Hydrogen that provides HTTP-based communication capabilities. It handles REST API endpoints, serves static content, and provides the foundation for other subsystems like WebSocket and Swagger UI. The WebServer is built on a lightweight, custom HTTP server implementation optimized for embedded systems.

## System Architecture

The WebServer follows a multi-threaded architecture with specialized components:

```diagram
┌───────────────────────────────────────────────────────────────────┐
│                         WebServer Subsystem                       │
│                                                                   │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │  Main Listener  │           │ Worker Threads  │                │
│  │     Thread      │◄─────────►│     Pool        │                │
│  └─────────────────┘           └─────────────────┘                │
│          │                              │                         │
│          ▼                              ▼                         │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │  Socket Accept  │           │ Request Handler │                │
│  │     Queue       │───────────┤     Logic       │                │
│  └─────────────────┘           └─────────────────┘                │
│                                        │                          │
│                      ┌─────────────────┼──────────────────┐       │
│                      │                 │                  │       │
│               ┌──────▼─────┐    ┌──────▼─────┐     ┌──────▼─────┐ │
│               │   Static   │    │  REST API  │     │   Upload   │ │
│               │  Content   │    │  Handlers  │     │  Handlers  │ │
│               └────────────┘    └────────────┘     └────────────┘ │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Main Listener Thread**: Manages the server socket and accepts incoming connections
2. **Worker Thread Pool**: Processes HTTP requests in parallel
3. **Request Handler Logic**: Parses HTTP requests and routes them to appropriate handlers
4. **API Handlers**: Process REST API endpoints and generate responses
5. **Static Content**: Serves files from configured directories
6. **Upload Handlers**: Manage file uploads and multipart form data

## Implementation Details

The WebServer is implemented with these primary components:

### Threading Model

The WebServer uses a thread pool architecture to handle multiple concurrent connections:

```c
// Web server thread pool
static ServiceThreads webserver_threads = {0};

// Main thread handle
static pthread_t webserver_thread;

// Shutdown flag
volatile sig_atomic_t web_server_shutdown = 0;
```

The server creates a configurable number of worker threads to process requests in parallel, while the main thread handles connection acceptance.

### Request Processing Flow

```diagram
┌─────────────────┐
│ Accept Connection│
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Parse Headers  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Route Request   │
└────────┬────────┘
         │
         ├─────────────────┬─────────────────┐
         │                 │                 │
         ▼                 ▼                 ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│ Process API     │ │ Serve Static    │ │ Handle Upload   │
│ Request         │ │ Content         │ │ Request         │
└────────┬────────┘ └────────┬────────┘ └────────┬────────┘
         │                   │                   │
         ├───────────────────┴───────────────────┘
         │
         ▼
┌─────────────────┐
│ Send Response   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Close Connection│
└─────────────────┘
```

### Compression Support

The WebServer implements on-the-fly compression for text-based content:

```c
// Compression settings
typedef struct {
    bool enabled;
    int level;  // 1-9, where 9 is maximum compression
    size_t min_size;  // Minimum size to compress
    char* content_types;  // Content types to compress
} CompressionConfig;
```

Compression is automatically applied based on the client's `Accept-Encoding` header and the configured compression settings.

### Request Routing

The WebServer uses a prefix-based routing system to direct requests to the appropriate handlers:

```c
// API handler registration
typedef struct {
    const char* prefix;
    http_handler_func handler;
    void* user_data;
} APIHandler;

// Register a new API handler
bool register_api_handler(const char* prefix, http_handler_func handler, void* user_data);
```

Handlers are matched based on the URL path prefix, with the most specific match taking precedence.

## Integration with Launch System

The WebServer integrates with the Hydrogen Launch System through a dedicated launch readiness check implementation:

```c
bool check_webserver_launch_readiness(LaunchReadiness* readiness) {
    // Check configuration
    readiness->go_config = app_config && 
                         app_config->webserver.enabled && 
                         app_config->webserver.port > 0 && 
                         app_config->webserver.port <= 65535;
    
    // Check dependencies
    readiness->go_dependencies = is_subsystem_running_by_name("Logging");
    
    // Check resources (ports, file descriptors, etc.)
    readiness->go_resources = true;  // Resource checks
    
    // Check system conditions
    readiness->go_system = true;  // System checks
    
    // Check state (not in shutdown)
    readiness->go_state = !web_server_shutdown;
    
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

The WebServer has the following launch requirements:

1. **Configuration**:
   - Server must be enabled in configuration
   - Port must be valid (1-65535)
   - At least one thread must be configured

2. **Dependencies**:
   - Logging subsystem must be running

3. **Resources**:
   - Configured port must be available
   - Sufficient file descriptors must be available

4. **State**:
   - Not already in shutdown state

## Initialization Process

The WebServer initialization follows this sequence:

```c
bool init_webserver_subsystem(void) {
    // Block initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        return false;
    }
    
    // Apply configuration
    int port = app_config->webserver.port;
    int thread_count = app_config->webserver.threads;
    
    // Initialize socket
    server_socket = create_server_socket(port, app_config->webserver.backlog);
    if (server_socket < 0) {
        log_this("WebServer", "Failed to create server socket", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Initialize thread pool
    if (!init_webserver_thread_pool(thread_count)) {
        log_this("WebServer", "Failed to initialize thread pool", LOG_LEVEL_ERROR);
        close(server_socket);
        return false;
    }
    
    // Start main listener thread
    if (pthread_create(&webserver_thread, NULL, web_server_main, NULL) != 0) {
        log_this("WebServer", "Failed to create main thread", LOG_LEVEL_ERROR);
        shutdown_webserver_thread_pool();
        close(server_socket);
        return false;
    }
    
    // Register with thread tracking
    register_thread(&webserver_threads, webserver_thread, "webserver_main");
    
    log_this("WebServer", "Initialized on port %d with %d threads", 
            LOG_LEVEL_STATE, port, thread_count);
    return true;
}
```

## Shutdown Process

The WebServer implements a clean shutdown sequence:

```c
void shutdown_web_server(void) {
    // Signal shutdown
    web_server_shutdown = 1;
    
    // Close server socket to interrupt accept() calls
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
    
    // Wait for main thread to exit
    if (webserver_thread != 0) {
        pthread_join(webserver_thread, NULL);
        webserver_thread = 0;
    }
    
    // Shutdown worker thread pool
    shutdown_webserver_thread_pool();
    
    log_this("WebServer", "Shutdown complete", LOG_LEVEL_STATE);
}
```

## Configuration Options

The WebServer is configured through the `WebServer` section in the Hydrogen configuration:

```json
{
  "WebServer": {
    "Enabled": true,
    "Port": 8080,
    "Threads": 4,
    "Backlog": 10,
    "ReadTimeout": 30,
    "WriteTimeout": 30,
    "MaxRequestSize": 1048576,
    "DefaultContentType": "text/html; charset=utf-8",
    "DocumentRoot": "/var/lib/hydrogen/www",
    "Compression": {
      "Enabled": true,
      "Level": 6,
      "MinSize": 1024,
      "ContentTypes": "text/html,text/css,text/javascript,application/json"
    }
  }
}
```

### Key Configuration Parameters

| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `Enabled` | Enable/disable the web server | `true` | `true`/`false` |
| `Port` | Server listening port | `8080` | 1-65535 |
| `Threads` | Worker thread count | `4` | 1-32 |
| `Backlog` | Connection backlog size | `10` | 1-128 |
| `ReadTimeout` | Socket read timeout (seconds) | `30` | 1-3600 |
| `WriteTimeout` | Socket write timeout (seconds) | `30` | 1-3600 |
| `MaxRequestSize` | Maximum request size (bytes) | `1048576` | 1024-1073741824 |
| `DocumentRoot` | Static content directory | `/var/lib/hydrogen/www` | Valid path |

## Performance Considerations

The WebServer is designed for efficient operation with these optimizations:

1. **Connection Pooling**: Reuses connections when possible to reduce overhead
2. **Zero-Copy Techniques**: Uses sendfile() for efficient static file serving
3. **Thread Pool**: Balances parallelism with resource usage
4. **Buffer Management**: Uses efficient buffer allocation strategies
5. **Compression**: Reduces bandwidth with on-the-fly compression

## Implementation Files

The WebServer implementation is spread across these files:

| File | Purpose |
|------|---------|
| `src/webserver/web_server_core.c` | Core server implementation |
| `src/webserver/web_server_core.h` | Public interface |
| `src/webserver/web_server_request.c` | Request parsing and handling |
| `src/webserver/web_server_compression.c` | Compression support |
| `src/webserver/web_server_upload.c` | File upload handling |
| `src/webserver/web_server_payload.c` | Request/response body handling |
| `src/state/startup_webserver.c` | Initialization integration |
| `src/config/launch/webserver.c` | Launch readiness check |

## API Example

Example of registering an API handler with the WebServer:

```c
// Handler function
bool handle_example_api(HttpRequest* request, HttpResponse* response, void* user_data) {
    // Set content type
    http_response_set_header(response, "Content-Type", "application/json");
    
    // Create JSON response
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", "success");
    cJSON_AddStringToObject(json, "message", "Example API response");
    
    // Convert to string and set as response
    char* json_str = cJSON_PrintUnformatted(json);
    http_response_set_body(response, json_str, strlen(json_str));
    
    // Clean up
    free(json_str);
    cJSON_Delete(json);
    
    return true;
}

// Register the handler
register_api_handler("/api/example", handle_example_api, NULL);
```

## Error Handling

The WebServer implements comprehensive error handling:

1. **HTTP Status Codes**: Appropriate status codes for different error conditions
2. **Detailed Logging**: Error conditions are logged with context information
3. **Graceful Degradation**: Continues serving other requests when one fails
4. **Timeout Handling**: Properly manages slow or stalled connections

## Related Documentation

For more information on WebServer-related topics, see:

- [WebServer Configuration](/docs/H/core/reference/webserver_configuration.md) - Configuration details
- [Launch System Architecture](/docs/H/core/reference/launch_system_architecture.md) - Launch system integration
- [API Documentation](/docs/H/core/subsystems/api/api.md) - REST API details
- [Subsystem Registry Architecture](/docs/H/core/reference/subsystem_registry_architecture.md) - Subsystem management