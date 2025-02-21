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

- [Main Documentation](docs/README.md) - Overview and getting started
- [Configuration Guide](docs/configuration.md) - Server configuration and settings
- [REST API Documentation](docs/api.md) - REST API endpoints and usage
- [WebSocket Interface](docs/web_socket.md) - Real-time communication interface
- [Print Queue Management](docs/print_queue.md) - Print job scheduling and management
- [Release Notes](docs/release_notes.md) - Detailed version history and changes

## Files

- `hydrogen.json` Configuration file for the Hydrogen server
- `Makefile` Build instructions for compiling the Hydrogen program
- `src/beryllium.c` Implements G-code analysis functionality
- `src/beryllium.h` Header file for G-code analysis declarations
- `src/configuration.c` Handles loading and managing configuration settings
- `src/configuration.h` Defines configuration-related structures and constants
- `src/hydrogen.c` Main entry point and core system initialization
- `src/keys.c` Implements cryptographic key generation and management
- `src/keys.h` Header file for cryptographic operations
- `src/logging.c` Core logging system implementation
- `src/logging.h` Logging interface and configuration
- `src/log_queue_manager.c` Thread-safe log message queue handler
- `src/log_queue_manager.h` Log queue management interface
- `src/mdns_server.h` Service discovery interface definitions
- `src/mdns_linux.c` Linux-specific mDNS implementation
- `src/network.h` Network interface abstractions
- `src/network_linux.c` Linux network stack implementation
- `src/print_queue_manager.c` 3D print job scheduling and management
- `src/print_queue_manager.h` Print queue interface
- `src/queue.c` Generic thread-safe queue implementation
- `src/queue.h` Queue data structure interface
- `src/shutdown.c` Graceful system shutdown coordination
- `src/shutdown.h` Shutdown procedure definitions
- `src/startup.c` System initialization and service startup
- `src/startup.h` Startup sequence definitions
- `src/state.c` Global state management implementation
- `src/state.h` State tracking interface
- `src/utils.c` Common utility functions
- `src/utils.h` Utility function declarations
- `src/utils_logging.c` Extended logging utilities
- `src/utils_logging.h` Logging utility interfaces
- `src/utils_queue.c` Queue manipulation utilities
- `src/utils_queue.h` Queue utility interfaces
- `src/utils_status.c` Status reporting utilities
- `src/utils_status.h` Status utility interfaces
- `src/utils_threads.c` Thread management utilities
- `src/utils_threads.h` Threading utility interfaces
- `src/utils_time.c` Time handling utilities
- `src/utils_time.h` Time utility interfaces
- `src/web_server_core.c` Core HTTP server implementation
- `src/web_server_core.h` HTTP server interface
- `src/web_server_print.h` 3D printing endpoint definitions
- `src/web_server_request.c` HTTP request handling
- `src/web_server_request.h` Request processing interface
- `src/web_server_upload.c` File upload handling
- `src/web_server_upload.h` Upload processing interface
- `src/websocket_server.c` WebSocket server core implementation
- `src/websocket_server.h` WebSocket server public interface
- `src/websocket_server_internal.h` Internal WebSocket definitions
- `src/websocket_server_auth.c` WebSocket authentication system
- `src/websocket_server_connection.c` Connection lifecycle handler
- `src/websocket_server_context.c` Server context management
- `src/websocket_server_dispatch.c` Message routing system
- `src/websocket_server_message.c` Message processing engine
- `src/websocket_server_status.c` Status reporting implementation
- `src/api/system/system_service.c` System service implementation
- `src/api/system/system_service.h` System service interface

## System Dependencies

Core Libraries:

- [pthreads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) - POSIX thread support for concurrent operations
- [jansson](https://github.com/akheron/jansson) - Efficient and memory-safe JSON parsing/generation
- [microhttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight embedded HTTP server
- [libwebsockets](https://github.com/warmcat/libwebsockets) - Full-duplex WebSocket communication
- [OpenSSL](https://www.openssl.org/) (libssl/libcrypto) - Industry-standard encryption and security
- [libm](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html) - Mathematical operations support
