# hydrogen

## Overview

Hydrogen is a high-performance, multithreaded server framework that began as a modern replacement for Mainsail and Moonraker in the 3D printing ecosystem. While it excels at providing a powerful front-end for Klipper-based 3D printers, it has evolved into a versatile platform capable of serving many roles beyond 3D printing.

At its heart, Hydrogen features a robust queue system and a sophisticated service management architecture. Core services include:

- Web Server: Delivers static content and exposes a comprehensive REST API
- WebSocket Server: Enables real-time bidirectional communication with guaranteed message delivery
- mDNS Client/Server: Handles service discovery and auto-configuration
- Queue System: Provides thread-safe data management across all components
- Print Server: Offers complete 3D printer control and monitoring capabilities
- OIDC Provider: Implements OpenID Connect protocol for secure authentication and authorization

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

Complete, detailed documentation is available in the `docs` directory, organized by topic:

- [**Main Documentation**](docs/README.md) - Start here for a comprehensive overview
- [**Developer Onboarding Guide**](docs/developer_onboarding.md) - Visual architecture overview and code navigation to expedite project understanding
- [**Quick Start Guide**](docs/guides/quick-start.md) - Get up and running quickly
- [**Release Notes**](RELEASES.md) - Detailed version history
- [**AI Integration**](docs/ai_integration.md) - AI capabilities and implementations

## Additional Documentation

The following documentation files are available outside of the main docs/ directory:

- [README.md](README.md) - This file, providing project overview and navigation
- [RECIPE.md](RECIPE.md) - Development guide optimized for AI assistance
- [RELEASES.md](RELEASES.md) - Version history and release notes
- [SECRETS.md](SECRETS.md) - Secrets management using environment variables
- [src/api/README.md](src/api/README.md) - API implementation details
- [payload/README.md](payload/README.md) - Payload system with encryption
- [tests/README.md](tests/README.md) - Testing framework documentation
- [oidc-client-examples/README.md](oidc-client-examples/README.md) - OIDC example implementations

## Files

<details>
<summary><b>Project Folder</b></summary>

- [hydrogen.json](hydrogen.json) Configuration file for the Hydrogen server
- [Makefile](Makefile) Build instructions for compiling the Hydrogen program
- [src/hydrogen.c](src/hydrogen.c) Main entry point and core system initialization

</details>
<details>
<summary><b>API Components</b></summary>

- [src/api/system/system_service.c](src/api/system/system_service.c) System service implementation
- [src/api/system/system_service.h](src/api/system/system_service.h) System service interface

</details>
<details>
<summary><b>Configuration Management</b></summary>

- [src/config/configuration.c](src/config/configuration.c) Handles loading and managing configuration settings
- [src/config/configuration.h](src/config/configuration.h) Defines configuration-related structures and constants
- [src/config/keys.c](src/config/keys.c) Implements cryptographic key generation and management
- [src/config/keys.h](src/config/keys.h) Header file for cryptographic operations

</details>
<details>
<summary><b>Logging System</b></summary>

- [src/logging/logging.c](src/logging/logging.c) Core logging system implementation
- [src/logging/logging.h](src/logging/logging.h) Logging interface and configuration
- [src/logging/log_queue_manager.c](src/logging/log_queue_manager.c) Thread-safe log message queue handler
- [src/logging/log_queue_manager.h](src/logging/log_queue_manager.h) Log queue management interface

</details>
<details>
<summary><b>Network Services</b></summary>

- [src/mdns/mdns_server.h](src/mdns/mdns_server.h) Service discovery interface definitions
- [src/mdns/mdns_linux.c](src/mdns/mdns_linux.c) Linux-specific mDNS implementation
- [src/network/network.h](src/network/network.h) Network interface abstractions
- [src/network/network_linux.c](src/network/network_linux.c) Linux network stack implementation

</details>
<details>
<summary><b>Print Management</b></summary>

- [src/print/beryllium.c](src/print/beryllium.c) Implements G-code analysis functionality
- [src/print/beryllium.h](src/print/beryllium.h) Header file for G-code analysis declarations
- [src/print/print_queue_manager.c](src/print/print_queue_manager.c) 3D print job scheduling and management
- [src/print/print_queue_manager.h](src/print/print_queue_manager.h) Print queue interface

</details>
<details>
<summary><b>Queue System</b></summary>

- [src/queue/queue.c](src/queue/queue.c) Generic thread-safe queue implementation
- [src/queue/queue.h](src/queue/queue.h) Queue data structure interface

</details>
<details>
<summary><b>State Management</b></summary>

- [src/state/shutdown.c](src/state/shutdown.c) Graceful system shutdown coordination
- [src/state/shutdown.h](src/state/shutdown.h) Shutdown procedure definitions
- [src/state/startup.c](src/state/startup.c) System initialization and service startup
- [src/state/startup.h](src/state/startup.h) Startup sequence definitions
- [src/state/state.c](src/state/state.c) Global state management implementation
- [src/state/state.h](src/state/state.h) State tracking interface

</details>
<details>
<summary><b>Tests</b></summary>

- [tests/analyze_stuck_threads.sh](tests/analyze_stuck_threads.sh) Script for analyzing thread deadlocks
- [tests/run_tests.sh](tests/run_tests.sh) Script for executing test suites
- [tests/test_startup_shutdown.sh](tests/test_startup_shutdown.sh) Tests for startup/shutdown procedures
- [tests/monitor_resources.sh](tests/monitor_resources.sh) Script for monitoring system resource usage
- [tests/README.md](tests/README.md) Testing documentation

</details>
<details>
<summary><b>Utility Functions</b></summary>

- [src/utils/utils.c](src/utils/utils.c) Common utility functions
- [src/utils/utils.h](src/utils/utils.h) Utility function declarations
- [src/utils/utils_logging.c](src/utils/utils_logging.c) Extended logging utilities
- [src/utils/utils_logging.h](src/utils/utils_logging.h) Logging utility interfaces
- [src/utils/utils_queue.c](src/utils/utils_queue.c) Queue manipulation utilities
- [src/utils/utils_queue.h](src/utils/utils_queue.h) Queue utility interfaces
- [src/utils/utils_status.c](src/utils/utils_status.c) Status reporting utilities
- [src/utils/utils_status.h](src/utils/utils_status.h) Status utility interfaces
- [src/utils/utils_threads.c](src/utils/utils_threads.c) Thread management utilities
- [src/utils/utils_threads.h](src/utils/utils_threads.h) Threading utility interfaces
- [src/utils/utils_time.c](src/utils/utils_time.c) Time handling utilities
- [src/utils/utils_time.h](src/utils/utils_time.h) Time utility interfaces

</details>
<details>
<summary><b>Web Server</b></summary>

- [src/webserver/web_server_core.c](src/webserver/web_server_core.c) Core HTTP server implementation
- [src/webserver/web_server_core.h](src/webserver/web_server_core.h) HTTP server interface
- [src/webserver/web_server_print.h](src/webserver/web_server_print.h) 3D printing endpoint definitions
- [src/webserver/web_server_request.c](src/webserver/web_server_request.c) HTTP request handling
- [src/webserver/web_server_request.h](src/webserver/web_server_request.h) Request processing interface
- [src/webserver/web_server_upload.c](src/webserver/web_server_upload.c) File upload handling
- [src/webserver/web_server_upload.h](src/webserver/web_server_upload.h) Upload processing interface

</details>
<details>
<summary><b>WebSocket Server</b></summary>

- [src/websocket/websocket_server.c](src/websocket/websocket_server.c) WebSocket server core implementation
- [src/websocket/websocket_server.h](src/websocket/websocket_server.h) WebSocket server public interface
- [src/websocket/websocket_server_internal.h](src/websocket/websocket_server_internal.h) Internal WebSocket definitions
- [src/websocket/websocket_server_auth.c](src/websocket/websocket_server_auth.c) WebSocket authentication system
- [src/websocket/websocket_server_connection.c](src/websocket/websocket_server_connection.c) Connection lifecycle handler
- [src/websocket/websocket_server_context.c](src/websocket/websocket_server_context.c) Server context management
- [src/websocket/websocket_server_dispatch.c](src/websocket/websocket_server_dispatch.c) Message routing system
- [src/websocket/websocket_server_message.c](src/websocket/websocket_server_message.c) Message processing engine
- [src/websocket/websocket_server_status.c](src/websocket/websocket_server_status.c) Status reporting implementation

</details>
<details>
<!-- AI-ASSISTANT-ATTENTION: OIDC client examples require careful review of docs/oidc_integration.md and docs/reference/oidc_architecture.md first -->
<summary><b>OIDC Client Examples</b></summary>

- [oidc-client-examples/README.md](oidc-client-examples/README.md) Overview of OIDC example implementations
- [oidc-client-examples/C/auth_code_flow.c](oidc-client-examples/C/auth_code_flow.c) Authorization Code flow example in C
- [oidc-client-examples/C/client_credentials.c](oidc-client-examples/C/client_credentials.c) Client Credentials flow example in C
- [oidc-client-examples/C/password_flow.c](oidc-client-examples/C/password_flow.c) Resource Owner Password flow example in C
- [oidc-client-examples/C/Makefile](oidc-client-examples/C/Makefile) Makefile for building C examples
- [oidc-client-examples/JavaScript/auth_code_flow.html](oidc-client-examples/JavaScript/auth_code_flow.html) Authorization Code flow example in JavaScript

</details>

## System Dependencies

### Build-time Dependencies

These tools are required when building or developing with the Hydrogen codebase:

- C compiler and build tools (gcc and friends)
- [curl](https://curl.se/) - Data transfer tool for downloading dependencies
- [tar](https://www.gnu.org/software/tar/) - Archiving utility (typically part of core utils)
- [wget](https://www.gnu.org/software/wget/) - Alternative network downloader
- [brotli](https://github.com/google/brotli) - Modern compression algorithm
- [jq](https://stedolan.github.io/jq/) - Command-line JSON processor
- [upx](https://upx.github.io/) - Executable compressor for release builds
- [markdownlint](https://github.com/DavidAnson/markdownlint) - Markdown file linting and style checking
- [jsonlint](https://github.com/zaach/jsonlint) - JSON file validation and formatting
- [cppcheck](https://cppcheck.sourceforge.io/) - Static analysis for C/C++ code
- [eslint](https://eslint.org/) - JavaScript code linting and style checking
- [stylelint](https://stylelint.io/) - CSS/SCSS linting and style checking
- [htmlhint](https://htmlhint.com/) - HTML code linting and validation

### Runtime Dependencies

These libraries are required for Hydrogen to run:

- [pthreads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) - POSIX thread support for concurrent operations
- [jansson](https://github.com/akheron/jansson) - Efficient and memory-safe JSON parsing/generation
- [microhttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight embedded HTTP server
- [libwebsockets](https://github.com/warmcat/libwebsockets) - Full-duplex WebSocket communication
- [OpenSSL](https://www.openssl.org/) (libssl/libcrypto) - Industry-standard encryption and .security
- [libm](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html) - Mathematical operations support
- [libbrotlidec](https://github.com/google/brotli) - Brotli decompression library
- [libtar](https://github.com/tklauser/libtar) - TAR file manipulation

### Environment Variables

Environment variables provide a flexible way to configure Hydrogen without modifying its configuration files. Any configuration value can be substituted with an environment variable using the `${env.VARIABLE_NAME}` syntax, making it easy to adapt settings across different environments. The following variables are used in the default configuration:

**Configuration File Location:**

- `HYDROGEN_CONFIG` - Override the default configuration file location

**Server Settings:**

- `PAYLOAD_KEY` - Key used for encrypting payload data

**Database Connections:**
For each database (Log, OIDC, Acuranzo, Helium, Canvas), the following variables are used:

- `*_DB_HOST` - Database server hostname
- `*_DB_PORT` - Database server port
- `*_DB_NAME` - Database name
- `*_DB_USER` - Database username
- `*_DB_PASS` - Database password

Example for Log database:

- `LOG_DB_HOST` - Log database server hostname
- `LOG_DB_PORT` - Log database server port
- `LOG_DB_NAME` - Log database name
- `LOG_DB_USER` - Log database username
- `LOG_DB_PASS` - Log database password

**Mail Relay Configuration:**
For each SMTP server (up to 2 servers supported):

- `SMTP_SERVER1_HOST` - Primary SMTP server hostname
- `SMTP_SERVER1_PORT` - Primary SMTP server port
- `SMTP_SERVER1_USER` - Primary SMTP server username
- `SMTP_SERVER1_PASS` - Primary SMTP server password
- `SMTP_SERVER2_*` - Same variables for secondary SMTP server

To use these variables in the configuration file, use the format `${env.VARIABLE_NAME}`. For example:

```json
{
    "Server": {
        "PayloadKey": "${env.PAYLOAD_KEY}"
    },
    "Databases": {
        "Log": {
            "Host": "${env.LOG_DB_HOST}",
            "Port": "${env.LOG_DB_PORT}"
        }
    }
}
```

## Latest Test Results

Generated on: Mon Mar 17 17:30:48 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 13 |
| Passed | 8 |
| Failed | 5 |
| Skipped | 0 |
| Total Subtests | 87 |
| Passed Subtests | 35 |
| Failed Subtests | 52 |
| Runtime | 4m 25s |

### Individual Test Results

| Test | Status | Details |
| ---- | ------ | ------- |
| env_payload | ✅ Passed | Test completed without errors (2 of 2 subtests passed) |
| compilation | ✅ Passed | Test completed without errors (7 of 7 subtests passed) |
| startup_shutdown | ✅ Passed | Both min and max configuration tests passed (4 of 6 subtests passed) |
| api_prefixes | ❌ Failed | Test failed with errors (0 of 10 subtests passed) |
| dynamic_loading | ❌ Failed | Test failed with errors (1 of 7 subtests passed) |
| env_payload | ✅ Passed | Test completed without errors (2 of 2 subtests passed) |
| env_variables | ✅ Passed | Test completed without errors (1 of 1 subtests passed) |
| json_error_handling | ✅ Passed | Test completed without errors (2 of 2 subtests passed) |
| library_dependencies | ❌ Failed | Test failed with errors (0 of 16 subtests passed) |
| socket_rebind | ✅ Passed | Test completed without errors (1 of 1 subtests passed) |
| swagger_ui | ❌ Failed | Test failed with errors (2 of 10 subtests passed) |
| system_endpoints | ❌ Failed | Test failed with errors (1 of 11 subtests passed) |
| z_codebase | ✅ Passed | Test completed without errors (12 of 12 subtests passed) |

## Repository Information

Generated via cloc: Mon Mar 17 17:30:48 PDT 2025

| Language | Files | Blank Lines | Comment Lines | Code Lines |
| -------- | ----- | ----------- | ------------- | ---------- |
| C | 118 | 3374 | 5036 | 14822 |
| Markdown | 95 | 3518 | 27 | 12629 |
| JSON | 17 | 0 | 0 | 4798 |
| Bourne | Shell | 21 | 1183 | 1230 |
| C/C++ | Header | 96 | 894 | 4197 |
| Text | 11 | 2 | 0 | 1194 |
| HTML | 1 | 74 | 0 | 493 |
| make | 2 | 50 | 211 | 201 |
| **Total** | **361** | **9095** | **10701** | **41019** |
