# Hydrogen Project Structure

This document provides a comprehensive overview of the Hydrogen project's file organization and architecture.

## Root Files

<details>
<summary><b>Configuration & Documentation</b></summary>

- [README.md](README.md) - Project overview and quick start guide
- [RECIPE.md](RECIPE.md) - Development requirements and coding standards
- [SETUP.md](SETUP.md) - Installation and setup instructions
- [RELEASES.md](RELEASES.md) - Release notes and version history
- [SECRETS.md](SECRETS.md) - Security configuration guide
- [SITEMAP.md](SITEMAP.md) - Project navigation and file index
- [STRUCTURE.md](STRUCTURE.md) - This file - project structure overview
- [COVERAGE.svg](COVERAGE.svg) - Visual coverage analysis report (auto-generated)
- [COMPLETE.svg](COMPLETE.svg) - Complete test suite results visualization (auto-generated)

</details>

## Core Application

<details>
<summary><b>Main Entry Point</b></summary>

- [src/hydrogen.c](src/hydrogen.c) - Main entry point and core system initialization
- [src/not_hydrogen.c](src/not_hydrogen.c) - Error handler test file

</details>

## Source Code Organization

<details>
<summary><b>API Components</b></summary>

- [src/api/api_service.c](src/api/api_service.c) - Core API service implementation
- [src/api/api_service.h](src/api/api_service.h) - API service interface definitions
- [src/api/api_utils.c](src/api/api_utils.c) - API utility functions
- [src/api/api_utils.h](src/api/api_utils.h) - API utility interfaces
- [src/api/README.md](src/api/README.md) - API documentation
- [src/api/oidc/](src/api/oidc/) - OIDC-specific API endpoints
- [src/api/system/](src/api/system/) - System information API endpoints

</details>

<details>
<summary><b>Configuration Management</b></summary>

- [src/config/config.c](src/config/config.c) - Core configuration system
- [src/config/config.h](src/config/config.h) - Configuration structures and constants
- [src/config/config_forward.h](src/config/config_forward.h) - Forward declarations
- [src/config/config_api.c](src/config/config_api.c) - API configuration implementation
- [src/config/config_api.h](src/config/config_api.h) - API configuration interface definitions
- [src/config/config_databases.c](src/config/config_databases.c) - Database configuration implementation
- [src/config/config_databases.h](src/config/config_databases.h) - Database configuration interface definitions
- [src/config/config_logging.c](src/config/config_logging.c) - Logging configuration implementation
- [src/config/config_logging.h](src/config/config_logging.h) - Logging configuration interface definitions
- [src/config/config_mail_relay.c](src/config/config_mail_relay.c) - Mail relay configuration implementation
- [src/config/config_mail_relay.h](src/config/config_mail_relay.h) - Mail relay configuration interface definitions
- [src/config/config_mdns_client.c](src/config/config_mdns_client.c) - mDNS client configuration implementation
- [src/config/config_mdns_client.h](src/config/config_mdns_client.h) - mDNS client configuration interface definitions
- [src/config/config_mdns_server.c](src/config/config_mdns_server.c) - mDNS server configuration implementation
- [src/config/config_mdns_server.h](src/config/config_mdns_server.h) - mDNS server configuration interface definitions
- [src/config/config_network.c](src/config/config_network.c) - Network configuration implementation
- [src/config/config_network.h](src/config/config_network.h) - Network configuration interface definitions
- [src/config/config_notify.c](src/config/config_notify.c) - Notification configuration implementation
- [src/config/config_notify.h](src/config/config_notify.h) - Notification configuration interface definitions
- [src/config/config_oidc.c](src/config/config_oidc.c) - OIDC configuration implementation
- [src/config/config_oidc.h](src/config/config_oidc.h) - OIDC configuration interface definitions
- [src/config/config_print.c](src/config/config_print.c) - Print subsystem configuration implementation
- [src/config/config_print.h](src/config/config_print.h) - Print subsystem configuration interface definitions
- [src/config/config_print_priorities.h](src/config/config_print_priorities.h) - Print priority definitions
- [src/config/config_resources.c](src/config/config_resources.c) - Resource configuration implementation
- [src/config/config_resources.h](src/config/config_resources.h) - Resource configuration interface definitions
- [src/config/config_server.c](src/config/config_server.c) - Server configuration implementation
- [src/config/config_server.h](src/config/config_server.h) - Server configuration interface definitions
- [src/config/config_swagger.c](src/config/config_swagger.c) - Swagger configuration implementation
- [src/config/config_swagger.h](src/config/config_swagger.h) - Swagger configuration interface definitions
- [src/config/config_terminal.c](src/config/config_terminal.c) - Terminal configuration implementation
- [src/config/config_terminal.h](src/config/config_terminal.h) - Terminal configuration interface definitions
- [src/config/config_utils.c](src/config/config_utils.c) - Configuration utilities implementation
- [src/config/config_utils.h](src/config/config_utils.h) - Configuration utilities interface definitions
- [src/config/config_webserver.c](src/config/config_webserver.c) - Web server configuration implementation
- [src/config/config_webserver.h](src/config/config_webserver.h) - Web server configuration interface definitions
- [src/config/config_websocket.c](src/config/config_websocket.c) - WebSocket configuration implementation
- [src/config/config_websocket.h](src/config/config_websocket.h) - WebSocket configuration interface definitions

</details>

<details>
<summary><b>Launch System</b></summary>

- [src/launch/launch.c](src/launch/launch.c) - Core launch system coordination
- [src/launch/launch.h](src/launch/launch.h) - Launch system interface definitions
- [src/launch/launch_api.c](src/launch/launch_api.c) - API subsystem launch
- [src/launch/launch_database.c](src/launch/launch_database.c) - Database subsystem launch
- [src/launch/launch_logging.c](src/launch/launch_logging.c) - Logging subsystem launch
- [src/launch/launch_mail_relay.c](src/launch/launch_mail_relay.c) - Mail relay launch
- [src/launch/launch_mdns_client.c](src/launch/launch_mdns_client.c) - mDNS client launch
- [src/launch/launch_mdns_server.c](src/launch/launch_mdns_server.c) - mDNS server launch
- [src/launch/launch_network.c](src/launch/launch_network.c) - Network subsystem launch
- [src/launch/launch_notify.c](src/launch/launch_notify.c) - Notification subsystem launch
- [src/launch/launch_oidc.c](src/launch/launch_oidc.c) - OIDC subsystem launch
- [src/launch/launch_payload.c](src/launch/launch_payload.c) - Payload subsystem launch
- [src/launch/launch_plan.c](src/launch/launch_plan.c) - Launch planning system
- [src/launch/launch_print.c](src/launch/launch_print.c) - Print subsystem launch
- [src/launch/launch_readiness.c](src/launch/launch_readiness.c) - Readiness checks
- [src/launch/launch_registry.c](src/launch/launch_registry.c) - Registry subsystem launch
- [src/launch/launch_resources.c](src/launch/launch_resources.c) - Resource subsystem launch
- [src/launch/launch_review.c](src/launch/launch_review.c) - Launch review system
- [src/launch/launch_swagger.c](src/launch/launch_swagger.c) - Swagger subsystem launch
- [src/launch/launch_terminal.c](src/launch/launch_terminal.c) - Terminal subsystem launch
- [src/launch/launch_threads.c](src/launch/launch_threads.c) - Thread subsystem launch
- [src/launch/launch_webserver.c](src/launch/launch_webserver.c) - Web server launch
- [src/launch/launch_websocket.c](src/launch/launch_websocket.c) - WebSocket subsystem launch

</details>

<details>
<summary><b>Landing System (Shutdown)</b></summary>

- [src/landing/landing.c](src/landing/landing.c) - Core landing system coordination
- [src/landing/landing.h](src/landing/landing.h) - Landing system interface definitions
- [src/landing/landing_api.c](src/landing/landing_api.c) - API subsystem shutdown
- [src/landing/landing_database.c](src/landing/landing_database.c) - Database subsystem shutdown
- [src/landing/landing_logging.c](src/landing/landing_logging.c) - Logging subsystem shutdown
- [src/landing/landing_mail_relay.c](src/landing/landing_mail_relay.c) - Mail relay shutdown
- [src/landing/landing_mdns_client.c](src/landing/landing_mdns_client.c) - mDNS client shutdown
- [src/landing/landing_mdns_server.c](src/landing/landing_mdns_server.c) - mDNS server shutdown
- [src/landing/landing_network.c](src/landing/landing_network.c) - Network subsystem shutdown
- [src/landing/landing_payload.c](src/landing/landing_payload.c) - Payload subsystem shutdown
- [src/landing/landing_plan.c](src/landing/landing_plan.c) - Landing planning system
- [src/landing/landing_print.c](src/landing/landing_print.c) - Print subsystem shutdown
- [src/landing/landing_readiness.c](src/landing/landing_readiness.c) - Shutdown readiness checks
- [src/landing/landing_registry.c](src/landing/landing_registry.c) - Registry subsystem shutdown
- [src/landing/landing_review.c](src/landing/landing_review.c) - Landing review system
- [src/landing/landing_swagger.c](src/landing/landing_swagger.c) - Swagger subsystem shutdown
- [src/landing/landing_terminal.c](src/landing/landing_terminal.c) - Terminal subsystem shutdown
- [src/landing/landing_threads.c](src/landing/landing_threads.c) - Thread subsystem shutdown
- [src/landing/landing_webserver.c](src/landing/landing_webserver.c) - Web server shutdown
- [src/landing/landing_websocket.c](src/landing/landing_websocket.c) - WebSocket subsystem shutdown

</details>

<details>
<summary><b>Logging System</b></summary>

- [src/logging/logging.c](src/logging/logging.c) - Core logging system implementation
- [src/logging/logging.h](src/logging/logging.h) - Logging system interface definitions
- [src/logging/log_queue_manager.c](src/logging/log_queue_manager.c) - Thread-safe log message queue handler
- [src/logging/log_queue_manager.h](src/logging/log_queue_manager.h) - Log queue manager interface definitions

</details>

<details>
<summary><b>Network Services</b></summary>

- [src/network/network.h](src/network/network.h) - Network interface abstractions
- [src/network/network_linux.c](src/network/network_linux.c) - Linux network stack implementation
- [src/mdns/mdns_server.h](src/mdns/mdns_server.h) - Service discovery interface definitions
- [src/mdns/mdns_linux.c](src/mdns/mdns_linux.c) - Linux-specific mDNS implementation
- [src/mdns/keys.c](src/mdns/mdns_keys.c) - mDNS cryptographic key management implementation
- [src/mdns/keys.h](src/mdns/mdns_keys.h) - mDNS cryptographic key management interface definitions

</details>

<details>
<summary><b>OIDC Authentication</b></summary>

- [src/oidc/oidc_service.c](src/oidc/oidc_service.c) - Core OIDC service implementation
- [src/oidc/oidc_service.h](src/oidc/oidc_service.h) - OIDC service interface definitions
- [src/oidc/oidc_clients.c](src/oidc/oidc_clients.c) - OIDC client management implementation
- [src/oidc/oidc_clients.h](src/oidc/oidc_clients.h) - OIDC client management interface definitions
- [src/oidc/oidc_keys.c](src/oidc/oidc_keys.c) - OIDC cryptographic key handling implementation
- [src/oidc/oidc_keys.h](src/oidc/oidc_keys.h) - OIDC cryptographic key handling interface definitions
- [src/oidc/oidc_tokens.c](src/oidc/oidc_tokens.c) - OIDC token management implementation
- [src/oidc/oidc_tokens.h](src/oidc/oidc_tokens.h) - OIDC token management interface definitions
- [src/oidc/oidc_users.c](src/oidc/oidc_users.c) - OIDC user management implementation
- [src/oidc/oidc_users.h](src/oidc/oidc_users.h) - OIDC user management interface definitions

</details>

<details>
<summary><b>Payload Management</b></summary>

- [src/payload/payload.c](src/payload/payload.c) - Payload system implementation
- [src/payload/payload.h](src/payload/payload.h) - Payload system interface definitions

</details>

<details>
<summary><b>Print Management</b></summary>

- [src/print/print_queue_manager.c](src/print/print_queue_manager.c) - 3D print job scheduling and management implementation
- [src/print/print_queue_manager.h](src/print/print_queue_manager.h) - 3D print job scheduling and management interface definitions
- [src/print/beryllium.c](src/print/beryllium.c) - G-code analysis functionality implementation
- [src/print/beryllium.h](src/print/beryllium.h) - G-code analysis functionality interface definitions

</details>

<details>
<summary><b>Queue System</b></summary>

- [src/queue/queue.c](src/queue/queue.c) - Generic thread-safe queue implementation
- [src/queue/queue.h](src/queue/queue.h) - Generic thread-safe queue interface definitions

</details>

<details>
<summary><b>Registry System</b></summary>

- [src/registry/registry.c](src/registry/registry.c) - Subsystem registry implementation
- [src/registry/registry.h](src/registry/registry.h) - Subsystem registry interface definitions
- [src/registry/registry_integration.c](src/registry/registry_integration.c) - Registry integration utilities implementation
- [src/registry/registry_integration.h](src/registry/registry_integration.h) - Registry integration utilities interface definitions

</details>

<details>
<summary><b>State Management</b></summary>

- [src/state/state.c](src/state/state.c) - Global state management implementation
- [src/state/state.h](src/state/state.h) - Global state management interface definitions
- [src/state/state_types.h](src/state/state_types.h) - State type definitions

</details>

<details>
<summary><b>Status System</b></summary>

- [src/status/status.c](src/status/status.c) - Core status system implementation
- [src/status/status.h](src/status/status.h) - Core status system interface definitions
- [src/status/status_core.c](src/status/status_core.c) - Status core functionality implementation
- [src/status/status_core.h](src/status/status_core.h) - Status core functionality interface definitions
- [src/status/status_formatters.c](src/status/status_formatters.c) - Status output formatters implementation
- [src/status/status_formatters.h](src/status/status_formatters.h) - Status output formatters interface definitions
- [src/status/status_process.c](src/status/status_process.c) - Process status monitoring implementation
- [src/status/status_process.h](src/status/status_process.h) - Process status monitoring interface definitions
- [src/status/status_system.c](src/status/status_system.c) - System status monitoring implementation
- [src/status/status_system.h](src/status/status_system.h) - System status monitoring interface definitions

</details>

<details>
<summary><b>Swagger Documentation</b></summary>

- [src/swagger/swagger.c](src/swagger/swagger.c) - Swagger API documentation system implementation
- [src/swagger/swagger.h](src/swagger/swagger.h) - Swagger API documentation system interface definitions

</details>

<details>
<summary><b>Thread Management</b></summary>

- [src/threads/threads.c](src/threads/threads.c) - Thread management system implementation
- [src/threads/threads.h](src/threads/threads.h) - Thread management system interface definitions

</details>

<details>
<summary><b>Utility Functions</b></summary>

- [src/utils/utils.c](src/utils/utils.c) - Common utility functions implementation
- [src/utils/utils.h](src/utils/utils.h) - Common utility functions interface definitions
- [src/utils/utils_dependency.c](src/utils/utils_dependency.c) - Dependency management utilities implementation
- [src/utils/utils_dependency.h](src/utils/utils_dependency.h) - Dependency management utilities interface definitions
- [src/utils/utils_logging.c](src/utils/utils_logging.c) - Extended logging utilities implementation
- [src/utils/utils_logging.h](src/utils/utils_logging.h) - Extended logging utilities interface definitions
- [src/utils/utils_queue.c](src/utils/utils_queue.c) - Queue manipulation utilities implementation
- [src/utils/utils_queue.h](src/utils/utils_queue.h) - Queue manipulation utilities interface definitions
- [src/utils/utils_time.c](src/utils/utils_time.c) - Time handling utilities implementation
- [src/utils/utils_time.h](src/utils/utils_time.h) - Time handling utilities interface definitions

</details>

<details>
<summary><b>Web Server</b></summary>

- [src/webserver/web_server_core.c](src/webserver/web_server_core.c) - Core HTTP server implementation
- [src/webserver/web_server_core.h](src/webserver/web_server_core.h) - Core HTTP server interface definitions
- [src/webserver/web_server_compression.c](src/webserver/web_server_compression.c) - HTTP compression support implementation
- [src/webserver/web_server_compression.h](src/webserver/web_server_compression.h) - HTTP compression support interface definitions
- [src/webserver/web_server.h](src/webserver/web_server.h) - Web server interface

</details>

<details>
<summary><b>WebSocket Server</b></summary>

- [src/websocket/websocket_server.c](src/websocket/websocket_server.c) - WebSocket server core implementation
- [src/websocket/websocket_server.h](src/websocket/websocket_server.h) - WebSocket server core interface definitions
- [src/websocket/websocket_server_internal.h](src/websocket/websocket_server_internal.h) - Internal WebSocket definitions
- [src/websocket/websocket_server_auth.c](src/websocket/websocket_server_auth.c) - WebSocket authentication system
- [src/websocket/websocket_server_connection.c](src/websocket/websocket_server_connection.c) - Connection lifecycle handler
- [src/websocket/websocket_server_context.c](src/websocket/websocket_server_context.c) - Server context management
- [src/websocket/websocket_server_dispatch.c](src/websocket/websocket_server_dispatch.c) - Message routing system
- [src/websocket/websocket_server_message.c](src/websocket/websocket_server_message.c) - Message processing engine
- [src/websocket/websocket_server_status.c](src/websocket/websocket_server_status.c) - Status reporting implementation

</details>

## Configuration

<details>
<summary><b>Configuration Files</b></summary>

- [configs/hydrogen.json](configs/hydrogen.json) - Main configuration file for the Hydrogen server
- [configs/hydrogen-env.json](configs/hydrogen-env.json) - Environment-specific configuration

</details>

## Documentation

<details>
<summary><b>Core Documentation</b></summary>

- [docs/README.md](docs/README.md) - Documentation index
- [docs/developer_onboarding.md](docs/developer_onboarding.md) - Setup and onboarding guide
- [docs/coding_guidelines.md](docs/coding_guidelines.md) - Coding standards and practices
- [docs/api.md](docs/api.md) - API reference documentation
- [docs/testing.md](docs/testing.md) - Testing guide and procedures
- [docs/configuration.md](docs/configuration.md) - Configuration system documentation
- [docs/data_structures.md](docs/data_structures.md) - Data structure documentation
- [docs/service.md](docs/service.md) - Service architecture documentation
- [docs/system_info.md](docs/system_info.md) - System information documentation

</details>

<details>
<summary><b>Subsystem Documentation</b></summary>

- [docs/ai_integration.md](docs/ai_integration.md) - AI integration documentation
- [docs/mdns_server.md](docs/mdns_server.md) - mDNS server documentation
- [docs/oidc_integration.md](docs/oidc_integration.md) - OIDC integration guide
- [docs/print_queue.md](docs/print_queue.md) - Print queue system documentation
- [docs/shutdown_architecture.md](docs/shutdown_architecture.md) - Shutdown system architecture
- [docs/thread_monitoring.md](docs/thread_monitoring.md) - Thread monitoring documentation
- [docs/web_socket.md](docs/web_socket.md) - WebSocket system documentation

</details>

<details>
<summary><b>API Documentation</b></summary>

- [docs/api/system/system_health.md](docs/api/system/system_health.md) - System health API
- [docs/api/system/system_info.md](docs/api/system/system_info.md) - System information API
- [docs/api/system/system_version.md](docs/api/system/system_version.md) - System version API

</details>

<details>
<summary><b>Reference Documentation</b></summary>

- [docs/reference/api.md](docs/reference/api.md) - API reference
- [docs/reference/configuration.md](docs/reference/configuration.md) - Configuration reference
- [docs/reference/data_structures.md](docs/reference/data_structures.md) - Data structures reference
- [docs/reference/database_architecture.md](docs/reference/database_architecture.md) - Database architecture
- [docs/reference/database_configuration.md](docs/reference/database_configuration.md) - Database configuration
- [docs/reference/launch_system_architecture.md](docs/reference/launch_system_architecture.md) - Launch system architecture
- [docs/reference/logging_configuration.md](docs/reference/logging_configuration.md) - Logging configuration
- [docs/reference/mdns_client_architecture.md](docs/reference/mdns_client_architecture.md) - mDNS client architecture
- [docs/reference/mdns_configuration.md](docs/reference/mdns_configuration.md) - mDNS configuration
- [docs/reference/network_architecture.md](docs/reference/network_architecture.md) - Network architecture
- [docs/reference/oidc_architecture.md](docs/reference/oidc_architecture.md) - OIDC architecture
- [docs/reference/print_queue_architecture.md](docs/reference/print_queue_architecture.md) - Print queue architecture
- [docs/reference/print_subsystem.md](docs/reference/print_subsystem.md) - Print subsystem reference
- [docs/reference/printqueue_configuration.md](docs/reference/printqueue_configuration.md) - Print queue configuration
- [docs/reference/resources_configuration.md](docs/reference/resources_configuration.md) - Resources configuration
- [docs/reference/smtp_configuration.md](docs/reference/smtp_configuration.md) - SMTP configuration
- [docs/reference/smtp_relay_architecture.md](docs/reference/smtp_relay_architecture.md) - SMTP relay architecture
- [docs/reference/subsystem_registry_architecture.md](docs/reference/subsystem_registry_architecture.md) - Subsystem registry architecture
- [docs/reference/swagger_architecture.md](docs/reference/swagger_architecture.md) - Swagger architecture
- [docs/reference/swagger_configuration.md](docs/reference/swagger_configuration.md) - Swagger configuration
- [docs/reference/system_architecture.md](docs/reference/system_architecture.md) - System architecture
- [docs/reference/system_info.md](docs/reference/system_info.md) - System information reference
- [docs/reference/terminal_architecture.md](docs/reference/terminal_architecture.md) - Terminal architecture
- [docs/reference/web_socket.md](docs/reference/web_socket.md) - WebSocket reference
- [docs/reference/webserver_configuration.md](docs/reference/webserver_configuration.md) - Web server configuration
- [docs/reference/webserver_subsystem.md](docs/reference/webserver_subsystem.md) - Web server subsystem
- [docs/reference/websocket_architecture.md](docs/reference/websocket_architecture.md) - WebSocket architecture
- [docs/reference/websocket_configuration.md](docs/reference/websocket_configuration.md) - WebSocket configuration
- [docs/reference/websocket_subsystem.md](docs/reference/websocket_subsystem.md) - WebSocket subsystem

</details>

<details>
<summary><b>Launch System Reference</b></summary>

- [docs/reference/launch/payload_subsystem.md](docs/reference/launch/payload_subsystem.md) - Payload subsystem reference
- [docs/reference/launch/threads_subsystem.md](docs/reference/launch/threads_subsystem.md) - Threads subsystem reference
- [docs/reference/launch/webserver_subsystem.md](docs/reference/launch/webserver_subsystem.md) - Web server subsystem reference

</details>

<details>
<summary><b>Deployment & Development</b></summary>

- [docs/deployment/docker.md](docs/deployment/docker.md) - Docker deployment guide
- [docs/development/ai_development.md](docs/development/ai_development.md) - AI-assisted development guide
- [docs/development/coding_guidelines.md](docs/development/coding_guidelines.md) - Development coding guidelines

</details>

<details>
<summary><b>User Guides</b></summary>

- [docs/guides/quick-start.md](docs/guides/quick-start.md) - Quick start guide
- [docs/guides/print-queue.md](docs/guides/print-queue.md) - Print queue user guide
- [docs/guides/use-cases/home-workshop.md](docs/guides/use-cases/home-workshop.md) - Home workshop use case
- [docs/guides/use-cases/print-farm.md](docs/guides/use-cases/print-farm.md) - Print farm use case

</details>

## Examples

<details>
<summary><b>OIDC Client Examples</b></summary>

- [examples/README.md](examples/README.md) - Examples documentation
- [examples/C/auth_code_flow.c](examples/C/auth_code_flow.c) - Authorization Code flow example in C
- [examples/C/client_credentials.c](examples/C/client_credentials.c) - Client Credentials flow example in C
- [examples/C/password_flow.c](examples/C/password_flow.c) - Resource Owner Password flow example in C
- [examples/JavaScript/auth_code_flow.html](examples/JavaScript/auth_code_flow.html) - Authorization Code flow example in JavaScript

</details>

## Payloads

<details>
<summary><b>Payload System</b></summary>

- [payloads/README.md](payloads/README.md) - Payload system documentation
- [payloads/payload-generate.sh](payloads/payload-generate.sh) - Payload generation script
- [payloads/swagger-generate.sh](payloads/swagger-generate.sh) - Swagger generation script
- [payloads/swagger.json](payloads/swagger.json) - Swagger API specification

</details>

## Testing Framework

- [tests/README.md](tests/README.md) - Testing documentation and procedures
- [tests/test_00_all.sh](tests/test_00_all.sh) - Orchestration script

<details>
<summary><b>Test Scripts</b></summary>

### Compilation and Static Analysis

- [tests/test_01_compilation.sh](tests/test_01_compilation.sh) - Compilation tests
- [tests/test_02_secrets.sh](tests/test_02_secrets.sh) - Checks validity of key environment variables
- [tests/test_04_check_links.sh](tests/test_04_check_links.sh) - Link validation tests

### Core Functional Tests

- [tests/test_10_unity.sh](tests/test_10_unity.sh) - Unity framework tests
- [tests/test_11_leaks_like_a_sieve.sh](tests/test_11_leaks_like_a_sieve.sh) - Memory leak tests
- [tests/test_12_env_variables.sh](tests/test_12_env_variables.sh) - Environment variable tests
- [tests/test_13_crash_handler.sh](tests/test_13_crash_handler.sh) - Crash handler tests
- [tests/test_14_library_dependencies.sh](tests/test_14_library_dependencies.sh) - Library dependency tests
- [tests/test_15_json_error_handling.sh](tests/test_15_json_error_handling.sh) - JSON error handling tests
- [tests/test_16_shutdown.sh](tests/test_16_shutdown.sh) - Shutdown-specific tests
- [tests/test_17_startup_shutdown.sh](tests/test_17_startup_shutdown.sh) - Startup/shutdown tests
- [tests/test_18_signals.sh](tests/test_18_signals.sh) - Signal handling tests
- [tests/test_19_socket_rebind.sh](tests/test_19_socket_rebind.sh) - Socket rebinding tests

### Server Tests

- [tests/test_20_api_prefix.sh](tests/test_20_api_prefix.sh) - API prefix tests
- [tests/test_21_system_endpoints.sh](tests/test_21_system_endpoints.sh) - System endpoint tests
- [tests/test_22_swagger.sh](tests/test_22_swagger.sh) - Swagger functionality tests
- [tests/test_23_websockets.sh](tests/test_23_websockets.sh) - Swagger functionality tests

### Linting Tests

- [tests/test_90_markdownlint.sh](tests/test_90_markdownlint.sh) - Markdown linting (markdownlint)
- [tests/test_91_cppcheck.sh](tests/test_91_cppcheck.sh) - C/C++ static analysis (cppcheck)
- [tests/test_92_shellcheck.sh](tests/test_92_shellcheck.sh) - Shell script linting (shellcheck)
- [tests/test_93_jsonlint.sh](tests/test_93_jsonlint.sh) - JSON validation and linting
- [tests/test_94_eslint.sh](tests/test_94_eslint.sh) - JavaScript linting (eslint)
- [tests/test_95_stylelint.sh](tests/test_95_stylelint.sh) - CSS linting (stylelint)
- [tests/test_96_htmlhint.sh](tests/test_96_htmlhint.sh) - HTML validation (htmlhint)
- [tests/test_98_code_size.sh](tests/test_98_code_size.sh) - Code size analysis and metrics
- [tests/test_99_coverage.sh](tests/test_99_coverage.sh) - Build system coverage

</details>

<details>
<summary><b>Test Libraries</b></summary>

- [tests/lib/](tests/lib/) - Test library functions
  - [cloc.sh](tests/lib/cloc.sh) - Code line counting utilities
  - [coverage.sh](tests/lib/coverage.sh) - Main coverage orchestration and API functions
  - [coverage-combinedn.sh](tests/lib/coverage-combined.sh) - Deals with Blackbox and Unity data aggregation
  - [coverage-common.sh](tests/lib/coverage-common.sh) - Shared coverage utilities and variables
  - [coverage_table.sh](tests/lib/coverage_table.sh) - Advanced tabular coverage reporting with visual formatting
  - [env_utils.sh](tests/lib/env_utils.sh) - Environment variable utilities
  - [file_utils.sh](tests/lib/file_utils.sh) - File manipulation utilities
  - [framework.sh](tests/lib/framework.sh) - Core testing framework
  - [github-sitemap.sh](tests/lib/github-sitemap.sh) - GitHub sitemap utilities
  - [lifecycle.sh](tests/lib/lifecycle.sh) - Test lifecycle management
  - [log_output.sh](tests/lib/log_output.sh) - Log output formatting
  - [network_utils.sh](tests/lib/network_utils.sh) - Network testing utilities
  
</details>

## Build Tools & Utilities

<details>
<summary><b>Build Scripts</b></summary>

- [extras/README.md](extras/README.md) - Build scripts and diagnostic tools documentation
- [extras/make-all.sh](extras/make-all.sh) - Compilation test script
- [extras/make-clean.sh](extras/make-clean.sh) - Comprehensive build cleanup script
- [extras/make-trial.sh](extras/make-trial.sh) - Quick trial build and diagnostics script
- [extras/filter-log.sh](extras/filter-log.sh) - Log output filtering utility

</details>

<details>
<summary><b>Diagnostic Tools</b></summary>

- [extras/one-offs/debug_payload.c](extras/one-offs/debug_payload.c) - Payload debug analysis tool
- [extras/one-offs/find_all_markers.c](extras/one-offs/find_all_markers.c) - Multiple marker detection tool
- [extras/one-offs/test_payload_detection.c](extras/one-offs/test_payload_detection.c) - Payload validation testing tool

</details>

## Directory Structure Overview

Based on the RECIPE.md structure and current implementation:

```directory
hydrogen/
├── README.md, RECIPE.md, SETUP.md, RELEASES.md, SECRETS.md, SITEMAP.md, STRUCTURE.md
├── configs/                    # Configuration files
│   ├── hydrogen.json          # Main configuration
│   └── hydrogen-env.json      # Environment configuration
├── src/                       # Source code
│   ├── hydrogen.c             # Main entry point
│   ├── not_hydrogen.c         # Alternative entry point
│   ├── Makefile              # Build system
│   ├── api/                  # API endpoints
│   ├── config/               # Config management
│   ├── landing/              # Shutdown handlers
│   ├── launch/               # Startup handlers
│   ├── logging/              # Logging system
│   ├── mdns/                 # Service discovery
│   ├── network/              # Network interfaces
│   ├── oidc/                 # OpenID Connect
│   ├── payload/              # Payload management
│   ├── print/                # Print subsystem
│   ├── queue/                # Thread-safe queues
│   ├── registry/             # Subsystem registry
│   ├── state/                # System state
│   ├── status/               # Status system
│   ├── swagger/              # API documentation
│   ├── threads/              # Thread management
│   ├── utils/                # Utilities
│   ├── webserver/            # HTTP server
│   └── websocket/            # WebSocket server
├── docs/                     # Documentation
│   ├── api/                  # API documentation
│   ├── deployment/           # Deployment guides
│   ├── development/          # Development guides
│   ├── guides/               # User guides
│   ├── reference/            # Reference documentation
│   └── releases/             # Release notes
├── examples/                 # Example code
│   ├── C/                    # C examples
│   └── JavaScript/           # JavaScript examples
├── extras/                   # Extra utilities
├── payloads/                 # Payload definitions
└── tests/                    # Test framework
    ├── configs/              # Test configurations
    ├── diagnostics/          # Test diagnostics
    ├── lib/                  # Test libraries
    ├── logs/                 # Test logs
    └── results/              # Test results
```

## Architecture Overview

The Hydrogen project follows a modular architecture with clear separation of concerns:

### Launch/Landing System

- **Launch**: Systematic startup of all subsystems in dependency order
- **Landing**: Graceful shutdown in reverse order
- **Registry**: Central coordination of all subsystems
- **Phases**: Readiness → Plan → Execute → Review

### Subsystem Order (from RECIPE.md)

1. Registry - Central subsystem coordination
2. Payload - Payload management system
3. Threads - Thread management and monitoring
4. Network - Network interface management
5. Database - Database connectivity (when configured)
6. Logging - Centralized logging system
7. WebServer - HTTP server functionality
8. API - REST API endpoints
9. Swagger - API documentation system
10. WebSocket - Real-time communication
11. Terminal - Terminal interface
12. mDNS Server - Service discovery server
13. mDNS Client - Service discovery client
14. MailRelay - Email notification system
15. Print - 3D printing subsystem
16. Resources - Resource management
17. OIDC - Authentication system
18. Notify - Notification system

### Key Features

- **Thread Safety**: All components designed for multi-threaded operation
- **Configuration**: JSON-based configuration with robust fallback handling
- **Logging**: Comprehensive logging with multiple levels and thread-safe queues
- **Testing**: Extensive test suite with 20+ test scripts covering all aspects
- **Documentation**: Comprehensive documentation covering architecture, APIs, and usage
- **Security**: OIDC authentication, encrypted payloads, and secure key management

### Build Targets

- `trial`: Required clean build with full test suite (must pass)
- `debug`: Debug build with symbols
- `perf`: Performance-optimized build
- `release`: Production build
- `valgrind`: Memory debugging build
- `default`: Standard development build
