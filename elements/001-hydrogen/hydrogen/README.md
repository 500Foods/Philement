# hydrogen

## Overview

Hydrogen is a high-performance, multithreaded server framework that began as a modern replacement for Mainsail and Moonraker in the 3D printing ecosystem. While it excels at providing a powerful front-end for Klipper-based 3D printers, it has evolved into a versatile platform capable of serving many roles beyond 3D printing.

At its heart, Hydrogen features a robust queue system and a sophisticated service management architecture. Core services include:

- Web Server: Delivers static content and exposes a comprehensive REST API
- WebSocket Server: Enables real-time bidirectional communication with guaranteed message delivery
- mDNS Client/Server: Handles service discovery and auto-configuration
- Queue System: Provides thread-safe data management across all components
- Print Server: Offers complete 3D printer control and monitoring capabilities

Whether you're managing a single 3D printer, orchestrating a large print farm, or building a high-performance web application, Hydrogen provides the foundation you need. Its modular architecture and emphasis on performance make it a versatile tool for both Philement projects and broader applications.

## Intended Audience & Requirements

Hydrogen is currently designed for technical users who:

- Are comfortable working with Linux-based systems
- Have experience with command-line interfaces and system configuration
- Understand basic networking concepts and server administration
- Are familiar with development tools and building software from source

**Platform Support:**

- Primary Platform: Linux-based systems
- Future Support: While we plan to expand platform support in the future, Hydrogen is currently optimized for and tested primarily on Linux systems

**Technical Prerequisites:**

- Linux operating system
- Familiarity with building and running server applications
- Basic understanding of WebSocket and REST API concepts
- Command-line proficiency for configuration and maintenance

## Documentation

Comprehensive documentation is available in the `docs` directory:

### Core Documentation

- [Main Documentation](docs/README.md) - Overview and getting started
- [Configuration Guide](docs/configuration.md) - Server configuration and settings
- [Release Notes](docs/release_notes.md) - Detailed version history and changes

### API & Interfaces

- [REST API Documentation](docs/api.md) - REST API endpoints and usage
- [WebSocket Interface](docs/web_socket.md) - Real-time communication interface
- [System API Reference](docs/api/system/system_info.md) - System-level API documentation

### Features & Components

- [Print Queue Management](docs/print_queue.md) - Print job scheduling and management
- [Data Structures](docs/data_structures.md) - Core data structure documentation
- [Thread Monitoring](docs/thread_monitoring.md) - Thread management and monitoring
- [Service Management](docs/service.md) - Service lifecycle and configuration

### Guides & Use Cases

- [Quick Start Guide](docs/guides/quick-start.md) - Get up and running quickly
- [Print Farm Setup](docs/guides/use-cases/print-farm.md) - Setting up a print farm
- [Home Workshop](docs/guides/use-cases/home-workshop.md) - Single printer setup

### Development & Deployment

- [Coding Guidelines](docs/coding_guidelines.md) - Development standards and practices
- [Docker Deployment](docs/deployment/docker.md) - Container-based deployment guide

### Technical Reference

- [System Information](docs/reference/system_info.md) - Detailed system architecture
- [API Reference](docs/reference/api.md) - Complete API documentation
- [Configuration Reference](docs/reference/configuration.md) - Detailed configuration options
- [WebSocket Reference](docs/reference/web_socket.md) - WebSocket protocol details

## Files

### Root Directory

- `hydrogen.json` Configuration file for the Hydrogen server
- `Makefile` Build instructions for compiling the Hydrogen program
- `src/hydrogen.c` Main entry point and core system initialization

### API Components

Located in `src/api/`:

- `system/system_service.c` System service implementation
- `system/system_service.h` System service interface

### Configuration Management

Located in `src/config/`:

- `configuration.c` Handles loading and managing configuration settings
- `configuration.h` Defines configuration-related structures and constants
- `keys.c` Implements cryptographic key generation and management
- `keys.h` Header file for cryptographic operations

### Logging System

Located in `src/logging/`:

- `logging.c` Core logging system implementation
- `logging.h` Logging interface and configuration
- `log_queue_manager.c` Thread-safe log message queue handler
- `log_queue_manager.h` Log queue management interface

### Network Services

Located in `src/mdns/`:

- `mdns_server.h` Service discovery interface definitions
- `mdns_linux.c` Linux-specific mDNS implementation

Located in `src/network/`:

- `network.h` Network interface abstractions
- `network_linux.c` Linux network stack implementation

### Print Management

Located in `src/print/`:

- `beryllium.c` Implements G-code analysis functionality
- `beryllium.h` Header file for G-code analysis declarations
- `print_queue_manager.c` 3D print job scheduling and management
- `print_queue_manager.h` Print queue interface

### Queue System

Located in `src/queue/`:

- `queue.c` Generic thread-safe queue implementation
- `queue.h` Queue data structure interface

### State Management

Located in `src/state/`:

- `shutdown.c` Graceful system shutdown coordination
- `shutdown.h` Shutdown procedure definitions
- `startup.c` System initialization and service startup
- `startup.h` Startup sequence definitions
- `state.c` Global state management implementation
- `state.h` State tracking interface

### Utility Functions

Located in `src/utils/`:

- `utils.c` Common utility functions
- `utils.h` Utility function declarations
- `utils_logging.c` Extended logging utilities
- `utils_logging.h` Logging utility interfaces
- `utils_queue.c` Queue manipulation utilities
- `utils_queue.h` Queue utility interfaces
- `utils_status.c` Status reporting utilities
- `utils_status.h` Status utility interfaces
- `utils_threads.c` Thread management utilities
- `utils_threads.h` Threading utility interfaces
- `utils_time.c` Time handling utilities
- `utils_time.h` Time utility interfaces

### Web Server

Located in `src/webserver/`:

- `web_server_core.c` Core HTTP server implementation
- `web_server_core.h` HTTP server interface
- `web_server_print.h` 3D printing endpoint definitions
- `web_server_request.c` HTTP request handling
- `web_server_request.h` Request processing interface
- `web_server_upload.c` File upload handling
- `web_server_upload.h` Upload processing interface

### WebSocket Server

Located in `src/websocket/`:

- `websocket_server.c` WebSocket server core implementation
- `websocket_server.h` WebSocket server public interface
- `websocket_server_internal.h` Internal WebSocket definitions
- `websocket_server_auth.c` WebSocket authentication system
- `websocket_server_connection.c` Connection lifecycle handler
- `websocket_server_context.c` Server context management
- `websocket_server_dispatch.c` Message routing system
- `websocket_server_message.c` Message processing engine
- `websocket_server_status.c` Status reporting implementation

## System Dependencies

Core Libraries:

- [pthreads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) - POSIX thread support for concurrent operations
- [jansson](https://github.com/akheron/jansson) - Efficient and memory-safe JSON parsing/generation
- [microhttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight embedded HTTP server
- [libwebsockets](https://github.com/warmcat/libwebsockets) - Full-duplex WebSocket communication
- [OpenSSL](https://www.openssl.org/) (libssl/libcrypto) - Industry-standard encryption and security
- [libm](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html) - Mathematical operations support
