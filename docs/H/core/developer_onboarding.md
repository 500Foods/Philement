# Developer Onboarding Guide

This guide is designed to help new developers quickly understand the Hydrogen project structure, architecture, and codebase organization. It complements the existing documentation by providing navigation assistance, visualization, and reference materials to accelerate the onboarding process.

## Visual Architecture Overview

```diagram
┌────────────────────────────────────────────────────────────────┐
│                     Hydrogen Server Core                       │
├─────────────┬─────────────┬─────────────────┬──────────────────┤
│  Web Server │  WebSocket  │  mDNS Discovery │  Service Manager │
│    (HTTP)   │   Server    │     Service     │                  │
├─────────────┴─────────────┴─────────────────┴──────────────────┤
│                      Queue System                              │
├─────────────┬─────────────┬─────────────────┬──────────────────┤
│    Print    │  Logging    │    Network      │ State Management │
│   Service   │   System    │     Layer       │                  │
├─────────────┴─────────────┴─────────────────┴──────────────────┤
│                Configuration & Utility Layer                   │
└────────────────────────────────────────────────────────────────┘
```

### Key Component Relationships

- **Core Services** (Web Server, WebSocket Server, mDNS Service) handle external communications
- All components use the **Queue System** for thread-safe data exchange
- **Configuration** provides centralized settings management
- **State Management** coordinates startup/shutdown sequences
- **Logging System** provides structured event recording across all components

## Code Navigation Guide

The Hydrogen codebase follows a modular structure with consistent patterns:

### Main Directories

- `src/` - Main source code
  - `api/` - RESTful API endpoints
  - `config/` - Configuration management
  - `logging/` - Logging system
  - `mdns/` - Service discovery
  - `network/` - Network interface layer
  - `oidc/` - OpenID Connect implementation
  - `print/` - 3D printing functionality
  - `queue/` - Thread-safe queue system
  - `state/` - System state management
  - `utils/` - Utility functions
  - `webserver/` - HTTP server implementation
  - `websocket/` - WebSocket server implementation
- `tests/` - Test framework and scripts
- `docs/` - Documentation
- `oidc-client-examples/` - Sample OIDC client implementations

### Code Entry Points

- `src/hydrogen.c` - Main program entry point
- `src/state/startup.c` - System initialization sequence
- `src/state/shutdown.c` - Graceful shutdown sequence

### Finding Your Way Around

1. **Understanding a Feature**: Start with the corresponding directory in `src/`
2. **API Implementation**: Check `src/api/` for endpoint handlers
3. **Configuration Options**: See `src/config/configuration.c` and `hydrogen.json`
4. **Thread Management**: Review `src/utils/utils_threads.c` for threading patterns
5. **Error Handling**: See `src/logging/logging.c` for logging conventions

## Component Dependency Map

```diagram
       ┌────────────┐
       │ hydrogen.c │
       └─────┬──────┘
             │
     ┌───────▼────────┐
     │  startup.c     │
     └───────┬────────┘
             │ initializes
┌────────────┼────────────┬────────────────┬───────────────┐
▼            ▼            ▼                ▼               ▼
┌──────────┐ ┌──────────┐ ┌──────────────┐ ┌─────────────┐ ┌─────────────┐
│ logging  │ │ config   │ │ web_server   │ │ websocket   │ │ mdns_server │
└──────────┘ └──────────┘ └──────┬───────┘ └─────┬───────┘ └─────────────┘
                                 │               │
                                 ▼               ▼
                          ┌─────────────┐ ┌─────────────┐
                          │ API Modules │ │ Connections │
                          └─────────────┘ └─────────────┘
```

### Initialization Order (Critical Path)

1. Configuration Loading
2. Logging System
3. State Management
4. Queue System
5. Network Services
6. Web Server
7. WebSocket Server
8. mDNS Service
9. Print Services

### Shutdown Order

The shutdown sequence is the reverse of the initialization order to ensure proper resource cleanup.

## Security and Encryption

The Hydrogen project uses several security mechanisms that developers need to be aware of:

### Encryption Systems

1. **Payload Encryption**
   - RSA+AES hybrid encryption for embedded assets
   - See [payload/README.md](/elements/001-hydrogen/hydrogen/payloads/README.md) for implementation
   - Key management documented in [SECRETS.md](/docs/H/SECRETS.md)

2. **OIDC Security**
   - RSA key pairs for token signing
   - AES encryption for sensitive data
   - See [docs/oidc_integration.md](/docs/H/core/subsystems/oidc/oidc.md)

3. **Transport Security**
   - TLS for HTTP/WebSocket connections
   - Certificate management
   - See [docs/reference/network_architecture.md](/docs/H/core/reference/network_architecture.md)

### Secrets Management

- Environment variable-based configuration
- Secure key storage guidelines
- See [SECRETS.md](/docs/H/SECRETS.md) for comprehensive documentation

## Project Glossary

| Term | Definition |
|------|------------|
| Queue | Thread-safe data structure for inter-component communication |
| Service | A standalone component with its own thread(s) |
| Handler | Function that processes a specific request type |
| Context | Data structure holding the state of a service |
| Message | Data packet exchanged between components |
| Endpoint | API access point exposed via HTTP |
| Resource | System component managed by the service layer |
| Payload | Encrypted asset embedded in executable |
| Secret | Sensitive data managed via environment variables |

## Quick-Reference Implementation Patterns

### Thread Management Pattern

```c
// Thread creation pattern
pthread_t thread_id;
pthread_create(&thread_id, NULL, thread_function, context);

// Add to tracking system for monitoring
add_service_thread(&service_threads, thread_id);
```

### Queue Operations Pattern

```c
// Adding to queue
pthread_mutex_lock(&queue->mutex);
enqueue(queue, item);
pthread_cond_signal(&queue->cond);
pthread_mutex_unlock(&queue->mutex);

// Processing queue
pthread_mutex_lock(&queue->mutex);
while (queue_empty(queue) && !shutdown_flag) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
}
item = dequeue(queue);
pthread_mutex_unlock(&queue->mutex);
```

### Error Handling Pattern

```c
if (operation_failed) {
    log_this("Component", "Operation failed: specific reason", LOG_LEVEL_ERROR, 
             true, false, true);
    cleanup_resources();
    return false;
}
```

## Development Environment Setup

### Prerequisites

- GCC compiler toolchain
- GNU Make
- Required libraries:
  - POSIX threads (`pthreads`)
  - JSON parsing (`jansson`)
  - HTTP server (`microhttpd`)
  - WebSockets (`libwebsockets`)
  - Cryptography (`openssl`)

### Building the Project

```bash
# Basic build
make

# Debug build with symbols
make debug

# Build with memory checking
make valgrind

# Clean build artifacts
make clean
```

### Testing Changes

```bash
# Run the test suite
./tests/run_tests.sh

# Test startup and shutdown
./tests/test_startup_shutdown.sh

# Monitor resource usage
./tests/monitor_resources.sh
```

## Getting Started with Code Changes

1. **Understand the Context**: Review relevant documentation in `docs/`
2. **Locate the Code**: Use the [Code Navigation Guide](#code-navigation-guide)
3. **Check Dependencies**: Refer to the [Component Dependency Map](#component-dependency-map)
4. **Follow Patterns**: Use the established [Implementation Patterns](#quick-reference-implementation-patterns)
5. **Test Changes**: Run appropriate tests from the test suite
6. **Review Coding Guidelines**: Ensure your changes follow the [Coding Guidelines](/docs/H/core/coding_guidelines.md)

## Common Development Tasks

### Adding a New API Endpoint

1. Define the endpoint in the appropriate header file (e.g., `src/api/system/system_service.h`)
2. Implement the handler function in the corresponding C file
3. Register the endpoint in the web server initialization (`src/webserver/web_server_core.c`)
4. Add documentation in `docs/api.md`

### Adding a New Configuration Option

1. Add the option definition in `src/config/configuration.h`
2. Implement parsing in `src/config/configuration.c`
3. Document the option in `docs/reference/configuration.md`
4. Update sample configuration files

### Debugging Tips

- Use log messages with appropriate severity levels
- Check thread status with `./tests/analyze_stuck_threads.sh`
- Monitor resource usage with `./tests/monitor_resources.sh`
- Review log files in the `results/` directory

## Additional Resources

- [Coding Guidelines](/docs/H/core/coding_guidelines.md) - Detailed coding standards
- [API Documentation](/docs/H/core/subsystems/api/api.md) - Complete API reference
- [System Architecture](/docs/H/core/reference/system_architecture.md) - Detailed system design
- [Testing System](/docs/H/core/testing.md) - Test framework documentation
