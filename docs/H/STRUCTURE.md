# Hydrogen Project Structure

This document provides a comprehensive overview of the Hydrogen project's file organization and architecture.

## Table of Contents

- [Root Files](#root-files)
- [Core Application](#core-application)
- [Source Code Organization](#source-code-organization)
- [Documentation](#documentation)
- [Examples](#examples)
- [Payloads](#payloads)
- [Testing Framework](#testing-framework)
- [Build Tools & Utilities](#build-tools--utilities)
- [Directory Structure Overview](#directory-structure-overview)
- [Architecture Overview](#architecture-overview)

## Root Files

<details>
<summary><b>Configuration & Documentation</b></summary>

- [README.md](/docs/H/README.md) - Project overview and quick start guide
- [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md) - Development requirements and coding standards
- [PROMPTS.md](/docs/H/PROMPTS.md) - More about prompts
- [CURIOSITIES.md](/docs/H/CURIOSITIES.md) - More about the project
- [SETUP.md](/docs/H/SETUP.md) - Installation and setup instructions
- [SECRETS.md](/docs/H/SECRETS.md) - Security configuration guide
- [SITEMAP.md](/docs/H/SITEMAP.md) - Project navigation and file index
- [STRUCTURE.md](/docs/H/STRUCTURE.md) - This file - project structure overview

</details>

<details>
<summary><b>Statistics</b></summary>

- [README.md](/elements/001-hydrogen/hydrogen/images/README.md) - Documentation for generated SVG visualization files
- [CLOC_CODE.svg](/elements/001-hydrogen/hydrogen/images/CLOC_CODE.svg) - Reformatted output from cloc (auto-generated)
- [CLOC_STAT.svg](/elements/001-hydrogen/hydrogen/images/CLOC_STAT.svg) - Extended statistics generated from cloc data (auto-generated)
- [COVERAGE.svg](/elements/001-hydrogen/hydrogen/images/COVERAGE.svg) - Visual coverage analysis report (auto-generated)
- [COMPLETE.svg](/elements/001-hydrogen/hydrogen/images/COMPLETE.svg) - Complete test suite results visualization (auto-generated)

</details>

## Core Application

<details>
<summary><b>Main Entry Point</b></summary>

- [src/hydrogen.c](/elements/001-hydrogen/hydrogen/src/hydrogen.c) - Main entry point and core system initialization

</details>

## Source Code Organization

<details>
<summary><b>API Components</b></summary>

- [src/api/api_service.c](/elements/001-hydrogen/hydrogen/src/api/api_service.c) - Core API service implementation
- [src/api/api_service.h](/elements/001-hydrogen/hydrogen/src/api/api_service.h) - API service interface definitions
- [src/api/api_utils.c](/elements/001-hydrogen/hydrogen/src/api/api_utils.c) - API utility functions
- [src/api/api_utils.h](/elements/001-hydrogen/hydrogen/src/api/api_utils.h) - API utility interfaces
- [src/api/README.md](/elements/001-hydrogen/hydrogen/src/api/README.md) - API documentation
- [src/api/oidc/](/elements/001-hydrogen/hydrogen/src/api/oidc/) - OIDC-specific API endpoints
- [src/api/system/](/elements/001-hydrogen/hydrogen/src/api/system/) - System information API endpoints

</details>

<details>
<summary><b>Configuration Management</b></summary>

- [src/config/config.c](/elements/001-hydrogen/hydrogen/src/config/config.c) - Core configuration system
- [src/config/config.h](/elements/001-hydrogen/hydrogen/src/config/config.h) - Configuration structures and constants
- [src/config/config_forward.h](/elements/001-hydrogen/hydrogen/src/config/config_forward.h) - Forward declarations
- [src/config/config_api.c](/elements/001-hydrogen/hydrogen/src/config/config_api.c) - API configuration implementation
- [src/config/config_api.h](/elements/001-hydrogen/hydrogen/src/config/config_api.h) - API configuration interface definitions
- [src/config/config_databases.c](/elements/001-hydrogen/hydrogen/src/config/config_databases.c) - Database configuration implementation
- [src/config/config_databases.h](/elements/001-hydrogen/hydrogen/src/config/config_databases.h) - Database configuration interface definitions
- [src/config/config_logging.c](/elements/001-hydrogen/hydrogen/src/config/config_logging.c) - Logging configuration implementation
- [src/config/config_logging.h](/elements/001-hydrogen/hydrogen/src/config/config_logging.h) - Logging configuration interface definitions
- [src/config/config_mail_relay.c](/elements/001-hydrogen/hydrogen/src/config/config_mail_relay.c) - Mail relay configuration implementation
- [src/config/config_mail_relay.h](/elements/001-hydrogen/hydrogen/src/config/config_mail_relay.h) - Mail relay configuration interface definitions
- [src/config/config_mdns_client.c](/elements/001-hydrogen/hydrogen/src/config/config_mdns_client.c) - mDNS client configuration implementation
- [src/config/config_mdns_client.h](/elements/001-hydrogen/hydrogen/src/config/config_mdns_client.h) - mDNS client configuration interface definitions
- [src/config/config_mdns_server.c](/elements/001-hydrogen/hydrogen/src/config/config_mdns_server.c) - mDNS server configuration implementation
- [src/config/config_mdns_server.h](/elements/001-hydrogen/hydrogen/src/config/config_mdns_server.h) - mDNS server configuration interface definitions
- [src/config/config_network.c](/elements/001-hydrogen/hydrogen/src/config/config_network.c) - Network configuration implementation
- [src/config/config_network.h](/elements/001-hydrogen/hydrogen/src/config/config_network.h) - Network configuration interface definitions
- [src/config/config_notify.c](/elements/001-hydrogen/hydrogen/src/config/config_notify.c) - Notification configuration implementation
- [src/config/config_notify.h](/elements/001-hydrogen/hydrogen/src/config/config_notify.h) - Notification configuration interface definitions
- [src/config/config_oidc.c](/elements/001-hydrogen/hydrogen/src/config/config_oidc.c) - OIDC configuration implementation
- [src/config/config_oidc.h](/elements/001-hydrogen/hydrogen/src/config/config_oidc.h) - OIDC configuration interface definitions
- [src/config/config_print.c](/elements/001-hydrogen/hydrogen/src/config/config_print.c) - Print subsystem configuration implementation
- [src/config/config_print.h](/elements/001-hydrogen/hydrogen/src/config/config_print.h) - Print subsystem configuration interface definitions
- [src/config/config_print_priorities.h](/elements/001-hydrogen/hydrogen/src/config/config_print_priorities.h) - Print priority definitions
- [src/config/config_resources.c](/elements/001-hydrogen/hydrogen/src/config/config_resources.c) - Resource configuration implementation
- [src/config/config_resources.h](/elements/001-hydrogen/hydrogen/src/config/config_resources.h) - Resource configuration interface definitions
- [src/config/config_server.c](/elements/001-hydrogen/hydrogen/src/config/config_server.c) - Server configuration implementation
- [src/config/config_server.h](/elements/001-hydrogen/hydrogen/src/config/config_server.h) - Server configuration interface definitions
- [src/config/config_swagger.c](/elements/001-hydrogen/hydrogen/src/config/config_swagger.c) - Swagger configuration implementation
- [src/config/config_swagger.h](/elements/001-hydrogen/hydrogen/src/config/config_swagger.h) - Swagger configuration interface definitions
- [src/config/config_terminal.c](/elements/001-hydrogen/hydrogen/src/config/config_terminal.c) - Terminal configuration implementation
- [src/config/config_terminal.h](/elements/001-hydrogen/hydrogen/src/config/config_terminal.h) - Terminal configuration interface definitions
- [src/config/config_utils.c](/elements/001-hydrogen/hydrogen/src/config/config_utils.c) - Configuration utilities implementation
- [src/config/config_utils.h](/elements/001-hydrogen/hydrogen/src/config/config_utils.h) - Configuration utilities interface definitions
- [src/config/config_webserver.c](/elements/001-hydrogen/hydrogen/src/config/config_webserver.c) - Web server configuration implementation
- [src/config/config_webserver.h](/elements/001-hydrogen/hydrogen/src/config/config_webserver.h) - Web server configuration interface definitions
- [src/config/config_websocket.c](/elements/001-hydrogen/hydrogen/src/config/config_websocket.c) - WebSocket configuration implementation
- [src/config/config_websocket.h](/elements/001-hydrogen/hydrogen/src/config/config_websocket.h) - WebSocket configuration interface definitions

</details>

<details>
<summary><b>Database Management</b></summary>

- [src/database/database.c](/elements/001-hydrogen/hydrogen/src/database/database.c) - Core database system implementation
- [src/database/database.h](/elements/001-hydrogen/hydrogen/src/database/database.h) - Database system interface definitions
- [src/database/database_bootstrap.c](/elements/001-hydrogen/hydrogen/src/database/database_bootstrap.c) - Database bootstrap functionality
- [src/database/database_bootstrap.h](/elements/001-hydrogen/hydrogen/src/database/database_bootstrap.h) - Database bootstrap interface
- [src/database/database_cache.c](/elements/001-hydrogen/hydrogen/src/database/database_cache.c) - Database query caching
- [src/database/database_cache.h](/elements/001-hydrogen/hydrogen/src/database/database_cache.h) - Database cache interface
- [src/database/database_connstring.c](/elements/001-hydrogen/hydrogen/src/database/database_connstring.c) - Connection string handling
- [src/database/database_connstring.h](/elements/001-hydrogen/hydrogen/src/database/database_connstring.h) - Connection string interface
- [src/database/database_engine.c](/elements/001-hydrogen/hydrogen/src/database/database_engine.c) - Database engine abstraction
- [src/database/database_engine.h](/elements/001-hydrogen/hydrogen/src/database/database_engine.h) - Database engine interface
- [src/database/database_execute.c](/elements/001-hydrogen/hydrogen/src/database/database_execute.c) - Query execution system
- [src/database/database_execute.h](/elements/001-hydrogen/hydrogen/src/database/database_execute.h) - Query execution interface
- [src/database/database_manage.c](/elements/001-hydrogen/hydrogen/src/database/database_manage.c) - Database management operations
- [src/database/database_manage.h](/elements/001-hydrogen/hydrogen/src/database/database_manage.h) - Database management interface
- [src/database/database_params.c](/elements/001-hydrogen/hydrogen/src/database/database_params.c) - Parameter binding system
- [src/database/database_params.h](/elements/001-hydrogen/hydrogen/src/database/database_params.h) - Parameter binding interface
- [src/database/database_pending.c](/elements/001-hydrogen/hydrogen/src/database/database_pending.c) - Pending operations handling
- [src/database/database_pending.h](/elements/001-hydrogen/hydrogen/src/database/database_pending.h) - Pending operations interface
- [src/database/database_queue_select.c](/elements/001-hydrogen/hydrogen/src/database/database_queue_select.c) - Queue-based select operations
- [src/database/database_queue_select.h](/elements/001-hydrogen/hydrogen/src/database/database_queue_select.h) - Queue select interface
- [src/database/database_serialize.c](/elements/001-hydrogen/hydrogen/src/database/database_serialize.c) - Data serialization
- [src/database/database_serialize.h](/elements/001-hydrogen/hydrogen/src/database/database_serialize.h) - Serialization interface
- [src/database/database_types.h](/elements/001-hydrogen/hydrogen/src/database/database_types.h) - Database type definitions
- [src/database/prepared_cache.h](/elements/001-hydrogen/hydrogen/src/database/prepared_cache.h) - Prepared statement cache
- [src/database/db2/](/elements/001-hydrogen/hydrogen/src/database/db2/) - DB2-specific database implementation
- [src/database/dbqueue/](/elements/001-hydrogen/hydrogen/src/database/dbqueue/) - Database queue system
- [src/database/migration/](/elements/001-hydrogen/hydrogen/src/database/migration/) - Database migration system
- [src/database/mysql/](/elements/001-hydrogen/hydrogen/src/database/mysql/) - MySQL-specific database implementation
- [src/database/postgresql/](/elements/001-hydrogen/hydrogen/src/database/postgresql/) - PostgreSQL-specific database implementation
- [src/database/sqlite/](/elements/001-hydrogen/hydrogen/src/database/sqlite/) - SQLite-specific database implementation

</details>

<details>
<summary><b>Global State Management</b></summary>

- [src/globals.c](/elements/001-hydrogen/hydrogen/src/globals.c) - Global variable definitions
- [src/globals.h](/elements/001-hydrogen/hydrogen/src/globals.h) - Global variable declarations

</details>

<details>
<summary><b>Request Handlers</b></summary>

- [src/handlers/handlers.c](/elements/001-hydrogen/hydrogen/src/handlers/handlers.c) - HTTP request handler implementations
- [src/handlers/handlers.h](/elements/001-hydrogen/hydrogen/src/handlers/handlers.h) - HTTP request handler interface definitions

</details>

<details>
<summary><b>Mutex Management</b></summary>

- [src/mutex/mutex.c](/elements/001-hydrogen/hydrogen/src/mutex/mutex.c) - Mutex utility functions implementation
- [src/mutex/mutex.h](/elements/001-hydrogen/hydrogen/src/mutex/mutex.h) - Mutex utility functions interface definitions

</details>

<details>
<summary><b>Launch System</b></summary>

- [src/launch/launch.c](/elements/001-hydrogen/hydrogen/src/launch/launch.c) - Core launch system coordination
- [src/launch/launch.h](/elements/001-hydrogen/hydrogen/src/launch/launch.h) - Launch system interface definitions
- [src/launch/launch_api.c](/elements/001-hydrogen/hydrogen/src/launch/launch_api.c) - API subsystem launch
- [src/launch/launch_database.c](/elements/001-hydrogen/hydrogen/src/launch/launch_database.c) - Database subsystem launch
- [src/launch/launch_logging.c](/elements/001-hydrogen/hydrogen/src/launch/launch_logging.c) - Logging subsystem launch
- [src/launch/launch_mail_relay.c](/elements/001-hydrogen/hydrogen/src/launch/launch_mail_relay.c) - Mail relay launch
- [src/launch/launch_mdns_client.c](/elements/001-hydrogen/hydrogen/src/launch/launch_mdns_client.c) - mDNS client launch
- [src/launch/launch_mdns_server.c](/elements/001-hydrogen/hydrogen/src/launch/launch_mdns_server.c) - mDNS server launch
- [src/launch/launch_network.c](/elements/001-hydrogen/hydrogen/src/launch/launch_network.c) - Network subsystem launch
- [src/launch/launch_notify.c](/elements/001-hydrogen/hydrogen/src/launch/launch_notify.c) - Notification subsystem launch
- [src/launch/launch_oidc.c](/elements/001-hydrogen/hydrogen/src/launch/launch_oidc.c) - OIDC subsystem launch
- [src/launch/launch_payload.c](/elements/001-hydrogen/hydrogen/src/launch/launch_payload.c) - Payload subsystem launch
- [src/launch/launch_plan.c](/elements/001-hydrogen/hydrogen/src/launch/launch_plan.c) - Launch planning system
- [src/launch/launch_print.c](/elements/001-hydrogen/hydrogen/src/launch/launch_print.c) - Print subsystem launch
- [src/launch/launch_readiness.c](/elements/001-hydrogen/hydrogen/src/launch/launch_readiness.c) - Readiness checks
- [src/launch/launch_registry.c](/elements/001-hydrogen/hydrogen/src/launch/launch_registry.c) - Registry subsystem launch
- [src/launch/launch_resources.c](/elements/001-hydrogen/hydrogen/src/launch/launch_resources.c) - Resource subsystem launch
- [src/launch/launch_review.c](/elements/001-hydrogen/hydrogen/src/launch/launch_review.c) - Launch review system
- [src/launch/launch_swagger.c](/elements/001-hydrogen/hydrogen/src/launch/launch_swagger.c) - Swagger subsystem launch
- [src/launch/launch_terminal.c](/elements/001-hydrogen/hydrogen/src/launch/launch_terminal.c) - Terminal subsystem launch
- [src/launch/launch_threads.c](/elements/001-hydrogen/hydrogen/src/launch/launch_threads.c) - Thread subsystem launch
- [src/launch/launch_webserver.c](/elements/001-hydrogen/hydrogen/src/launch/launch_webserver.c) - Web server launch
- [src/launch/launch_websocket.c](/elements/001-hydrogen/hydrogen/src/launch/launch_websocket.c) - WebSocket subsystem launch

</details>

<details>
<summary><b>Landing System (Shutdown)</b></summary>

- [src/landing/landing.c](/elements/001-hydrogen/hydrogen/src/landing/landing.c) - Core landing system coordination
- [src/landing/landing.h](/elements/001-hydrogen/hydrogen/src/landing/landing.h) - Landing system interface definitions
- [src/landing/landing_api.c](/elements/001-hydrogen/hydrogen/src/landing/landing_api.c) - API subsystem shutdown
- [src/landing/landing_database.c](/elements/001-hydrogen/hydrogen/src/landing/landing_database.c) - Database subsystem shutdown
- [src/landing/landing_logging.c](/elements/001-hydrogen/hydrogen/src/landing/landing_logging.c) - Logging subsystem shutdown
- [src/landing/landing_mail_relay.c](/elements/001-hydrogen/hydrogen/src/landing/landing_mail_relay.c) - Mail relay shutdown
- [src/landing/landing_mdns_client.c](/elements/001-hydrogen/hydrogen/src/landing/landing_mdns_client.c) - mDNS client shutdown
- [src/landing/landing_mdns_server.c](/elements/001-hydrogen/hydrogen/src/landing/landing_mdns_server.c) - mDNS server shutdown
- [src/landing/landing_network.c](/elements/001-hydrogen/hydrogen/src/landing/landing_network.c) - Network subsystem shutdown
- [src/landing/landing_payload.c](/elements/001-hydrogen/hydrogen/src/landing/landing_payload.c) - Payload subsystem shutdown
- [src/landing/landing_plan.c](/elements/001-hydrogen/hydrogen/src/landing/landing_plan.c) - Landing planning system
- [src/landing/landing_print.c](/elements/001-hydrogen/hydrogen/src/landing/landing_print.c) - Print subsystem shutdown
- [src/landing/landing_readiness.c](/elements/001-hydrogen/hydrogen/src/landing/landing_readiness.c) - Shutdown readiness checks
- [src/landing/landing_registry.c](/elements/001-hydrogen/hydrogen/src/landing/landing_registry.c) - Registry subsystem shutdown
- [src/landing/landing_review.c](/elements/001-hydrogen/hydrogen/src/landing/landing_review.c) - Landing review system
- [src/landing/landing_swagger.c](/elements/001-hydrogen/hydrogen/src/landing/landing_swagger.c) - Swagger subsystem shutdown
- [src/landing/landing_terminal.c](/elements/001-hydrogen/hydrogen/src/landing/landing_terminal.c) - Terminal subsystem shutdown
- [src/landing/landing_threads.c](/elements/001-hydrogen/hydrogen/src/landing/landing_threads.c) - Thread subsystem shutdown
- [src/landing/landing_webserver.c](/elements/001-hydrogen/hydrogen/src/landing/landing_webserver.c) - Web server shutdown
- [src/landing/landing_websocket.c](/elements/001-hydrogen/hydrogen/src/landing/landing_websocket.c) - WebSocket subsystem shutdown

</details>

<details>
<summary><b>Logging System</b></summary>

- [src/logging/logging.c](/elements/001-hydrogen/hydrogen/src/logging/logging.c) - Core logging system implementation
- [src/logging/logging.h](/elements/001-hydrogen/hydrogen/src/logging/logging.h) - Logging system interface definitions
- [src/logging/log_queue_manager.c](/elements/001-hydrogen/hydrogen/src/logging/log_queue_manager.c) - Thread-safe log message queue handler
- [src/logging/log_queue_manager.h](/elements/001-hydrogen/hydrogen/src/logging/log_queue_manager.h) - Log queue manager interface definitions

</details>

<details>
<summary><b>Network Services</b></summary>

- [src/network/network.h](/elements/001-hydrogen/hydrogen/src/network/network.h) - Network interface abstractions
- [src/network/network_linux.c](/elements/001-hydrogen/hydrogen/src/network/network_linux.c) - Linux network stack implementation
- [src/mdns/mdns_server.h](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server.h) - Service discovery interface definitions
- [src/mdns/mdns_server.c](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server.c) - mDNS implementation
- [src/mdns/mdns_server_announce.c](/elements/001-hydrogen/hydrogen/src/mdns/mdns_server_announce.c) - announcement aspects of mdns implementation
- [src/mdns/keys.c](/elements/001-hydrogen/hydrogen/src/mdns/mdns_keys.c) - mDNS cryptographic key management implementation
- [src/mdns/keys.h](/elements/001-hydrogen/hydrogen/src/mdns/mdns_keys.h) - mDNS cryptographic key management interface definitions

</details>

<details>
<summary><b>OIDC Authentication</b></summary>

- [src/oidc/oidc_service.c](/elements/001-hydrogen/hydrogen/src/oidc/oidc_service.c) - Core OIDC service implementation
- [src/oidc/oidc_service.h](/elements/001-hydrogen/hydrogen/src/oidc/oidc_service.h) - OIDC service interface definitions
- [src/oidc/oidc_clients.c](/elements/001-hydrogen/hydrogen/src/oidc/oidc_clients.c) - OIDC client management implementation
- [src/oidc/oidc_clients.h](/elements/001-hydrogen/hydrogen/src/oidc/oidc_clients.h) - OIDC client management interface definitions
- [src/oidc/oidc_keys.c](/elements/001-hydrogen/hydrogen/src/oidc/oidc_keys.c) - OIDC cryptographic key handling implementation
- [src/oidc/oidc_keys.h](/elements/001-hydrogen/hydrogen/src/oidc/oidc_keys.h) - OIDC cryptographic key handling interface definitions
- [src/oidc/oidc_tokens.c](/elements/001-hydrogen/hydrogen/src/oidc/oidc_tokens.c) - OIDC token management implementation
- [src/oidc/oidc_tokens.h](/elements/001-hydrogen/hydrogen/src/oidc/oidc_tokens.h) - OIDC token management interface definitions
- [src/oidc/oidc_users.c](/elements/001-hydrogen/hydrogen/src/oidc/oidc_users.c) - OIDC user management implementation
- [src/oidc/oidc_users.h](/elements/001-hydrogen/hydrogen/src/oidc/oidc_users.h) - OIDC user management interface definitions

</details>

<details>
<summary><b>Payload Management</b></summary>

- [src/payload/payload.c](/elements/001-hydrogen/hydrogen/src/payload/payload.c) - Payload system implementation
- [src/payload/payload.h](/elements/001-hydrogen/hydrogen/src/payload/payload.h) - Payload system interface definitions

</details>

<details>
<summary><b>Print Management</b></summary>

- [src/print/print_queue_manager.c](/elements/001-hydrogen/hydrogen/src/print/print_queue_manager.c) - 3D print job scheduling and management implementation
- [src/print/print_queue_manager.h](/elements/001-hydrogen/hydrogen/src/print/print_queue_manager.h) - 3D print job scheduling and management interface definitions
- [src/print/beryllium.c](/elements/001-hydrogen/hydrogen/src/print/beryllium.c) - G-code analysis functionality implementation
- [src/print/beryllium.h](/elements/001-hydrogen/hydrogen/src/print/beryllium.h) - G-code analysis functionality interface definitions

</details>

<details>
<summary><b>Queue System</b></summary>

- [src/queue/queue.c](/elements/001-hydrogen/hydrogen/src/queue/queue.c) - Generic thread-safe queue implementation
- [src/queue/queue.h](/elements/001-hydrogen/hydrogen/src/queue/queue.h) - Generic thread-safe queue interface definitions

</details>

<details>
<summary><b>Registry System</b></summary>

- [src/registry/registry.c](/elements/001-hydrogen/hydrogen/src/registry/registry.c) - Subsystem registry implementation
- [src/registry/registry.h](/elements/001-hydrogen/hydrogen/src/registry/registry.h) - Subsystem registry interface definitions
- [src/registry/registry_integration.c](/elements/001-hydrogen/hydrogen/src/registry/registry_integration.c) - Registry integration utilities implementation
- [src/registry/registry_integration.h](/elements/001-hydrogen/hydrogen/src/registry/registry_integration.h) - Registry integration utilities interface definitions

</details>

<details>
<summary><b>State Management</b></summary>

- [src/state/state.c](/elements/001-hydrogen/hydrogen/src/state/state.c) - Global state management implementation
- [src/state/state.h](/elements/001-hydrogen/hydrogen/src/state/state.h) - Global state management interface definitions
- [src/state/state_types.h](/elements/001-hydrogen/hydrogen/src/state/state_types.h) - State type definitions

</details>

<details>
<summary><b>Status System</b></summary>

- [src/status/status.c](/elements/001-hydrogen/hydrogen/src/status/status.c) - Core status system implementation
- [src/status/status.h](/elements/001-hydrogen/hydrogen/src/status/status.h) - Core status system interface definitions
- [src/status/status_core.c](/elements/001-hydrogen/hydrogen/src/status/status_core.c) - Status core functionality implementation
- [src/status/status_core.h](/elements/001-hydrogen/hydrogen/src/status/status_core.h) - Status core functionality interface definitions
- [src/status/status_formatters.c](/elements/001-hydrogen/hydrogen/src/status/status_formatters.c) - Status output formatters implementation
- [src/status/status_formatters.h](/elements/001-hydrogen/hydrogen/src/status/status_formatters.h) - Status output formatters interface definitions
- [src/status/status_process.c](/elements/001-hydrogen/hydrogen/src/status/status_process.c) - Process status monitoring implementation
- [src/status/status_process.h](/elements/001-hydrogen/hydrogen/src/status/status_process.h) - Process status monitoring interface definitions
- [src/status/status_system.c](/elements/001-hydrogen/hydrogen/src/status/status_system.c) - System status monitoring implementation
- [src/status/status_system.h](/elements/001-hydrogen/hydrogen/src/status/status_system.h) - System status monitoring interface definitions

</details>

<details>
<summary><b>Swagger Documentation</b></summary>

- [src/swagger/swagger.c](/elements/001-hydrogen/hydrogen/src/swagger/swagger.c) - Swagger API documentation system implementation
- [src/swagger/swagger.h](/elements/001-hydrogen/hydrogen/src/swagger/swagger.h) - Swagger API documentation system interface definitions

</details>

<details>
<summary><b>Thread Management</b></summary>

- [src/threads/threads.c](/elements/001-hydrogen/hydrogen/src/threads/threads.c) - Thread management system implementation
- [src/threads/threads.h](/elements/001-hydrogen/hydrogen/src/threads/threads.h) - Thread management system interface definitions

</details>

<details>
<summary><b>Utility Functions</b></summary>

- [src/utils/utils.c](/elements/001-hydrogen/hydrogen/src/utils/utils.c) - Common utility functions implementation
- [src/utils/utils.h](/elements/001-hydrogen/hydrogen/src/utils/utils.h) - Common utility functions interface definitions
- [src/utils/utils_dependency.c](/elements/001-hydrogen/hydrogen/src/utils/utils_dependency.c) - Dependency management utilities implementation
- [src/utils/utils_dependency.h](/elements/001-hydrogen/hydrogen/src/utils/utils_dependency.h) - Dependency management utilities interface definitions
- [src/utils/utils_logging.c](/elements/001-hydrogen/hydrogen/src/utils/utils_logging.c) - Extended logging utilities implementation
- [src/utils/utils_logging.h](/elements/001-hydrogen/hydrogen/src/utils/utils_logging.h) - Extended logging utilities interface definitions
- [src/utils/utils_queue.c](/elements/001-hydrogen/hydrogen/src/utils/utils_queue.c) - Queue manipulation utilities implementation
- [src/utils/utils_queue.h](/elements/001-hydrogen/hydrogen/src/utils/utils_queue.h) - Queue manipulation utilities interface definitions
- [src/utils/utils_time.c](/elements/001-hydrogen/hydrogen/src/utils/utils_time.c) - Time handling utilities implementation
- [src/utils/utils_time.h](/elements/001-hydrogen/hydrogen/src/utils/utils_time.h) - Time handling utilities interface definitions

</details>

<details>
<summary><b>Web Server</b></summary>

- [src/webserver/web_server_core.c](/elements/001-hydrogen/hydrogen/src/webserver/web_server_core.c) - Core HTTP server implementation
- [src/webserver/web_server_core.h](/elements/001-hydrogen/hydrogen/src/webserver/web_server_core.h) - Core HTTP server interface definitions
- [src/webserver/web_server_compression.c](/elements/001-hydrogen/hydrogen/src/webserver/web_server_compression.c) - HTTP compression support implementation
- [src/webserver/web_server_compression.h](/elements/001-hydrogen/hydrogen/src/webserver/web_server_compression.h) - HTTP compression support interface definitions
- [src/webserver/web_server.h](/elements/001-hydrogen/hydrogen/src/webserver/web_server.h) - Web server interface

</details>

<details>
<summary><b>WebSocket Server</b></summary>

- [src/websocket/websocket_server.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server.c) - WebSocket server core implementation
- [src/websocket/websocket_server.h](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server.h) - WebSocket server core interface definitions
- [src/websocket/websocket_server_internal.h](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_internal.h) - Internal WebSocket definitions
- [src/websocket/websocket_server_auth.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_auth.c) - WebSocket authentication system
- [src/websocket/websocket_server_connection.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_connection.c) - Connection lifecycle handler
- [src/websocket/websocket_server_context.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_context.c) - Server context management
- [src/websocket/websocket_server_dispatch.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_dispatch.c) - Message routing system
- [src/websocket/websocket_server_message.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_message.c) - Message processing engine
- [src/websocket/websocket_server_status.c](/elements/001-hydrogen/hydrogen/src/websocket/websocket_server_status.c) - Status reporting implementation

</details>

## Documentation

<details>
<summary><b>Core Documentation</b></summary>

- [docs/README.md](/docs/H/core/README.md) - Documentation index
- [docs/developer_onboarding.md](/docs/H/core/developer_onboarding.md) - Setup and onboarding guide
- [docs/coding_guidelines.md](/docs/H/core/coding_guidelines.md) - Coding standards and practices
- [docs/api.md](/docs/H/core/subsystems/api/api.md) - API reference documentation
- [docs/testing.md](/docs/H/core/testing.md) - Testing guide and procedures
- [docs/metrics/README.md](/docs/H/metrics/README.md) - Build metrics documentation
- [docs/configuration.md](/docs/H/core/configuration.md) - Configuration system documentation
- [docs/data_structures.md](/docs/H/core/data_structures.md) - Data structure documentation
- [docs/service.md](/docs/H/core/service.md) - Service architecture documentation
- [docs/system_info.md](/docs/H/core/system_info.md) - System information documentation

</details>

<details>
<summary><b>Subsystem Documentation</b></summary>

- [Subsystems Documentation](/docs/H/core/subsystems/README.md) - Table of contents for all subsystem documentation

- [docs/ai_integration.md](/docs/H/core/ai_integration.md) - AI integration documentation
- [docs/mdns_server.md](/docs/H/core/subsystems/mdnsserver/mdnsserver.md) - mDNS server documentation
- [docs/oidc_integration.md](/docs/H/core/subsystems/oidc/oidc.md) - OIDC integration guide
- [docs/print_queue.md](/docs/H/core/subsystems/print/print.md) - Print queue system documentation
- [docs/shutdown_architecture.md](/docs/H/core/shutdown_architecture.md) - Shutdown system architecture
- [docs/thread_monitoring.md](/docs/H/core/subsystems/threads/threads.md) - Thread monitoring documentation
- [docs/web_socket.md](/docs/H/core/subsystems/websocket/websocket.md) - WebSocket system documentation

</details>

<details>
<summary><b>API Documentation</b></summary>

- [docs/api/system/system_health.md](/docs/H/api/system/system_health.md) - System health API
- [docs/api/system/system_info.md](/docs/H/api/system/system_info.md) - System information API
- [docs/api/system/system_version.md](/docs/H/api/system/system_version.md) - System version API

</details>

<details>
<summary><b>Reference Documentation</b></summary>

- [docs/reference/api.md](/docs/H/core/reference/api.md) - API reference
- [docs/reference/configuration.md](/docs/H/core/reference/configuration.md) - Configuration reference
- [docs/reference/data_structures.md](/docs/H/core/reference/data_structures.md) - Data structures reference
- [docs/reference/database_architecture.md](/docs/H/core/reference/database_architecture.md) - Database architecture
- [docs/reference/database_configuration.md](/docs/H/core/reference/database_configuration.md) - Database configuration
- [docs/reference/launch_system_architecture.md](/docs/H/core/reference/launch_system_architecture.md) - Launch system architecture
- [docs/reference/logging_configuration.md](/docs/H/core/reference/logging_configuration.md) - Logging configuration
- [docs/reference/mdns_client_architecture.md](/docs/H/core/reference/mdns_client_architecture.md) - mDNS client architecture
- [docs/reference/mdns_configuration.md](/docs/H/core/reference/mdns_configuration.md) - mDNS configuration
- [docs/reference/network_architecture.md](/docs/H/core/reference/network_architecture.md) - Network architecture
- [docs/reference/oidc_architecture.md](/docs/H/core/reference/oidc_architecture.md) - OIDC architecture
- [docs/reference/print_queue_architecture.md](/docs/H/core/reference/print_queue_architecture.md) - Print queue architecture
- [docs/reference/print_subsystem.md](/docs/H/core/reference/print_subsystem.md) - Print subsystem reference
- [docs/reference/printqueue_configuration.md](/docs/H/core/reference/printqueue_configuration.md) - Print queue configuration
- [docs/reference/resources_configuration.md](/docs/H/core/reference/resources_configuration.md) - Resources configuration
- [docs/reference/smtp_configuration.md](/docs/H/core/reference/smtp_configuration.md) - SMTP configuration
- [docs/reference/smtp_relay_architecture.md](/docs/H/core/reference/smtp_relay_architecture.md) - SMTP relay architecture
- [docs/reference/subsystem_registry_architecture.md](/docs/H/core/reference/subsystem_registry_architecture.md) - Subsystem registry architecture
- [docs/reference/swagger_architecture.md](/docs/H/core/reference/swagger_architecture.md) - Swagger architecture
- [docs/reference/swagger_configuration.md](/docs/H/core/reference/swagger_configuration.md) - Swagger configuration
- [docs/reference/system_architecture.md](/docs/H/core/reference/system_architecture.md) - System architecture
- [docs/reference/system_info.md](/docs/H/core/reference/system_info.md) - System information reference
- [docs/reference/terminal_architecture.md](/docs/H/core/reference/terminal_architecture.md) - Terminal architecture
- [docs/reference/web_socket.md](/docs/H/core/reference/web_socket.md) - WebSocket reference
- [docs/reference/webserver_configuration.md](/docs/H/core/reference/webserver_configuration.md) - Web server configuration
- [docs/reference/webserver_subsystem.md](/docs/H/core/reference/webserver_subsystem.md) - Web server subsystem
- [docs/reference/websocket_architecture.md](/docs/H/core/reference/websocket_architecture.md) - WebSocket architecture
- [docs/reference/websocket_configuration.md](/docs/H/core/reference/websocket_configuration.md) - WebSocket configuration
- [docs/reference/websocket_subsystem.md](/docs/H/core/reference/websocket_subsystem.md) - WebSocket subsystem

</details>

<details>
<summary><b>Launch System Reference</b></summary>

- [docs/reference/launch/payload_subsystem.md](/docs/H/core/reference/launch/payload_subsystem.md) - Payload subsystem reference
- [docs/reference/launch/threads_subsystem.md](/docs/H/core/reference/launch/threads_subsystem.md) - Threads subsystem reference
- [docs/reference/launch/webserver_subsystem.md](/docs/H/core/reference/launch/webserver_subsystem.md) - Web server subsystem reference

</details>

<details>
<summary><b>Deployment & Development</b></summary>

- [docs/deployment/docker.md](/docs/H/core/deployment/docker.md) - Docker deployment guide
- [docs/development/ai_development.md](/docs/H/core/development/ai_development.md) - AI-assisted development guide
- [docs/development/coding_guidelines.md](/docs/H/core/development/coding_guidelines.md) - Development coding guidelines

</details>

<details>
<summary><b>User Guides</b></summary>

- [docs/guides/quick-start.md](/docs/H/core/guides/quick-start.md) - Quick start guide
- [docs/guides/print-queue.md](/docs/H/core/guides/print-queue.md) - Print queue user guide
- [docs/guides/use-cases/home-workshop.md](/docs/H/core/guides/use-cases/home-workshop.md) - Home workshop use case
- [docs/guides/use-cases/print-farm.md](/docs/H/core/guides/use-cases/print-farm.md) - Print farm use case

</details>

## Examples

<details>
<summary><b>OIDC Client Examples</b></summary>

- [examples/README.md](/elements/001-hydrogen/hydrogen/examples/README.md) - Examples documentation
- [examples/C/auth_code_flow.c](/elements/001-hydrogen/hydrogen/examples/C/auth_code_flow.c) - Authorization Code flow example in C
- [examples/C/client_credentials.c](/elements/001-hydrogen/hydrogen/examples/C/client_credentials.c) - Client Credentials flow example in C
- [examples/C/password_flow.c](/elements/001-hydrogen/hydrogen/examples/C/password_flow.c) - Resource Owner Password flow example in C
- [examples/JavaScript/auth_code_flow.html](/elements/001-hydrogen/hydrogen/examples/JavaScript/auth_code_flow.html) - Authorization Code flow example in JavaScript

</details>

## Payloads

<details>
<summary><b>Payload System</b></summary>

- [payloads/README.md](/elements/001-hydrogen/hydrogen/payloads/README.md) - Payload system documentation
- [payloads/payload-generate.sh](/elements/001-hydrogen/hydrogen/payloads/payload-generate.sh) - Payload generation script
- [payloads/swagger-generate.sh](/elements/001-hydrogen/hydrogen/payloads/swagger-generate.sh) - Swagger generation script
- [payloads/swagger.json](/elements/001-hydrogen/hydrogen/payloads/swagger.json) - Swagger API specification

</details>

## Testing Framework

- [tests/TESTING.md](/docs/H/tests/TESTING.md) - Testing documentation and procedures
- [tests/TESTING_UNITY.md](/docs/H/tests/TESTING_UNITY.md) - Unity unit testing framework documentation
- [tests/test_00_all.sh](/elements/001-hydrogen/hydrogen/tests/test_00_all.sh) - Orchestration script

<details>
<summary><b>Test Scripts</b></summary>

### Compilation and Static Analysis

- [tests/test_01_compilation.sh](/elements/001-hydrogen/hydrogen/tests/test_01_compilation.sh) - Compilation tests
- [tests/test_02_secrets.sh](/elements/001-hydrogen/hydrogen/tests/test_02_secrets.sh) - Checks validity of key environment variables
- [tests/test_03_shell.sh](/elements/001-hydrogen/hydrogen/tests/test_03_shell.sh) - Checks existence of key environment variables
- [tests/test_04_check_links.sh](/elements/001-hydrogen/hydrogen/tests/test_04_check_links.sh) - Link validation tests

### Core Functional Tests

- [tests/test_10_unity.sh](/elements/001-hydrogen/hydrogen/tests/test_10_unity.sh) - Unity framework tests
- [tests/test_11_leaks_like_a_sieve.sh](/elements/001-hydrogen/hydrogen/tests/test_11_leaks_like_a_sieve.sh) - Memory leak tests
- [tests/test_12_env_variables.sh](/elements/001-hydrogen/hydrogen/tests/test_12_env_variables.sh) - Environment variable tests
- [tests/test_13_crash_handler.sh](/elements/001-hydrogen/hydrogen/tests/test_13_crash_handler.sh) - Crash handler tests
- [tests/test_14_library_dependencies.sh](/elements/001-hydrogen/hydrogen/tests/test_14_library_dependencies.sh) - Library dependency tests
- [tests/test_15_json_error_handling.sh](/elements/001-hydrogen/hydrogen/tests/test_15_json_error_handling.sh) - JSON error handling tests
- [tests/test_16_shutdown.sh](/elements/001-hydrogen/hydrogen/tests/test_16_shutdown.sh) - Shutdown-specific tests
- [tests/test_17_startup_shutdown.sh](/elements/001-hydrogen/hydrogen/tests/test_17_startup_shutdown.sh) - Startup/shutdown tests
- [tests/test_18_signals.sh](/elements/001-hydrogen/hydrogen/tests/test_18_signals.sh) - Signal handling tests
- [tests/test_19_socket_rebind.sh](/elements/001-hydrogen/hydrogen/tests/test_19_socket_rebind.sh) - Socket rebinding tests

### Server Tests

- [tests/test_20_api_prefix.sh](/elements/001-hydrogen/hydrogen/tests/test_20_api_prefix.sh) - API prefix tests
- [tests/test_21_system_endpoints.sh](/elements/001-hydrogen/hydrogen/tests/test_21_system_endpoints.sh) - System endpoint tests
- [tests/test_22_swagger.sh](/elements/001-hydrogen/hydrogen/tests/test_22_swagger.sh) - Swagger functionality tests
- [tests/test_23_websockets.sh](/elements/001-hydrogen/hydrogen/tests/test_23_websockets.sh) - WebSocket functionality tests
- [tests/test_24_uploads.sh](/elements/001-hydrogen/hydrogen/tests/test_24_uploads.sh) - File upload functionality tests
- [tests/test_25_mdns.sh](/elements/001-hydrogen/hydrogen/tests/test_25_mdns.sh) - mDNS service discovery tests
- [tests/test_26_terminal.sh](/elements/001-hydrogen/hydrogen/tests/test_26_terminal.sh) - Terminal interface tests
- [tests/test_41_conduit.sh](/elements/001-hydrogen/hydrogen/tests/test_41_conduit.sh) - Conduit Query endpoint test

### Database Tests

- [tests/test_30_database.sh](/elements/001-hydrogen/hydrogen/tests/test_30_database.sh) - All Engines Parallel Operational Test
- [tests/test_31_migrations.sh](/elements/001-hydrogen/hydrogen/tests/test_31_migrations.sh) - Database migration validation
- [tests/test_32_postgres_migrations.sh](/elements/001-hydrogen/hydrogen/tests/test_32_postgres_migrations.sh) - PostgreSQL migration performance test
- [tests/test_33_mysql_migrations.sh](/elements/001-hydrogen/hydrogen/tests/test_33_mysql_migrations.sh) - MySQL migration performance test
- [tests/test_34_sqlite_migrations.sh](/elements/001-hydrogen/hydrogen/tests/test_34_sqlite_migrations.sh) - SQLite migration performance test
- [tests/test_35_db2_migrations.sh](/elements/001-hydrogen/hydrogen/tests/test_35_db2_migrations.sh) - DB2 migration performance test
- [tests/test_40_auth.sh](/elements/001-hydrogen/hydrogen/tests/test_40_auth.sh) - Authentication endpoints testing (JWT tokens across multiple database engines)

### Deliverables

- [tests/test_70_installer.sh](/elements/001-hydrogen/hydrogen/tests/test_70_installer.sh) - Standalone installer building test
- [tests/test_71_database_diagrams.sh](/elements/001-hydrogen/hydrogen/tests/test_71_database_diagrams.sh) - Database diagram generation

### Linting Tests

- [tests/test_89_coverage.sh](/elements/001-hydrogen/hydrogen/tests/test_89_coverage.sh) - Build system coverage
- [tests/test_90_markdownlint.sh](/elements/001-hydrogen/hydrogen/tests/test_90_markdownlint.sh) - Markdown linting (markdownlint)
- [tests/test_91_cppcheck.sh](/elements/001-hydrogen/hydrogen/tests/test_91_cppcheck.sh) - C/C++ static analysis (cppcheck)
- [tests/test_92_shellcheck.sh](/elements/001-hydrogen/hydrogen/tests/test_92_shellcheck.sh) - Shell script linting (shellcheck)
- [tests/test_93_jsonlint.sh](/elements/001-hydrogen/hydrogen/tests/test_93_jsonlint.sh) - JSON validation and linting
- [tests/test_94_eslint.sh](/elements/001-hydrogen/hydrogen/tests/test_94_eslint.sh) - JavaScript linting (eslint)
- [tests/test_95_stylelint.sh](/elements/001-hydrogen/hydrogen/tests/test_95_stylelint.sh) - CSS linting (stylelint)
- [tests/test_96_htmlhint.sh](/elements/001-hydrogen/hydrogen/tests/test_96_htmlhint.sh) - HTML validation (htmlhint)
- [tests/test_97_xmlstarlet.sh](/elements/001-hydrogen/hydrogen/tests/test_97_xmlstarlet.sh) - XML/SVG validation (xmlstarlet)
- [tests/test_98_luacheck.sh](/elements/001-hydrogen/hydrogen/tests/test_98_luacheck.sh) - Lua code analysis with luacheck
- [tests/test_99_code_size.sh](/elements/001-hydrogen/hydrogen/tests/test_99_code_size.sh) - Code size analysis and metrics

</details>

<details>
<summary><b>Test Libraries</b></summary>

- [tests/lib/](/elements/001-hydrogen/hydrogen/tests/lib/) - Test library functions
  - [cloc.sh](/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh) - Code line counting utilities
  - [discrepancy.sh](/elements/001-hydrogen/hydrogen/tests/lib/discrepancy.sh) - Coverage discrepancy analysis utilities
  - [coverage.sh](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh) - Main coverage orchestration and API functions
  - [coverage-combinedn.sh](/elements/001-hydrogen/hydrogen/tests/lib/coverage-combined.sh) - Deals with Blackbox and Unity data aggregation
  - [coverage-common.sh](/elements/001-hydrogen/hydrogen/tests/lib/coverage-common.sh) - Shared coverage utilities and variables
  - [coverage_table.sh](/elements/001-hydrogen/hydrogen/tests/lib/coverage_table.sh) - Advanced tabular coverage reporting with visual formatting
  - [env_utils.sh](/elements/001-hydrogen/hydrogen/tests/lib/env_utils.sh) - Environment variable utilities
  - [file_utils.sh](/elements/001-hydrogen/hydrogen/tests/lib/file_utils.sh) - File manipulation utilities
  - [framework.sh](/elements/001-hydrogen/hydrogen/tests/lib/framework.sh) - Core testing framework
  - [github-sitemap.sh](/elements/001-hydrogen/hydrogen/tests/lib/github-sitemap.sh) - GitHub sitemap utilities
  - [lifecycle.sh](/elements/001-hydrogen/hydrogen/tests/lib/lifecycle.sh) - Test lifecycle management
  - [log_output.sh](/elements/001-hydrogen/hydrogen/tests/lib/log_output.sh) - Log output formatting
  - [network_utils.sh](/elements/001-hydrogen/hydrogen/tests/lib/network_utils.sh) - Network testing utilities
  
</details>

## Build Tools & Utilities

<details>
<summary><b>Build Scripts</b></summary>

- [extras/README.md](/elements/001-hydrogen/hydrogen/extras/README.md) - Build scripts and diagnostic tools documentation
- [extras/make-all.sh](/elements/001-hydrogen/hydrogen/extras/make-all.sh) - Compilation test script
- [extras/make-clean.sh](/elements/001-hydrogen/hydrogen/extras/make-clean.sh) - Comprehensive build cleanup script
- [extras/make-trial.sh](/elements/001-hydrogen/hydrogen/extras/make-trial.sh) - Quick trial build and diagnostics script
- [extras/filter-log.sh]/elements/001-hydrogen/hydrogen/(extras/filter-log.sh) - Log output filtering utility

</details>

<details>
<summary><b>Installer System</b></summary>

- [installer/README.md](/elements/001-hydrogen/hydrogen/installer/README.md) - Hydrogen installer system documentation
- [installer/installer_wrapper.sh](/elements/001-hydrogen/hydrogen/installer/installer_wrapper.sh) - Installer template script

</details>

<details>
<summary><b>Diagnostic Tools</b></summary>

- [extras/payload/debug_payload.c](/elements/001-hydrogen/hydrogen/extras/payload/debug_payload.c) - Payload debug analysis tool
- [extras/payload/find_all_markers.c](/elements/001-hydrogen/hydrogen/extras/payload/find_all_markers.c) - Multiple marker detection tool
- [extras/payload/test_payload_detection.c](/elements/001-hydrogen/hydrogen/extras/payload/test_payload_detection.c) - Payload validation testing tool

</details>

<details>
<summary><b>Database UDFs</b></summary>

Database User-Defined Functions (UDFs) for extending database capabilities:

- [extras/base64decode_udf_db2/](/elements/001-hydrogen/hydrogen/extras/base64decode_udf_db2/) - Base64 decoding UDF for DB2
- [extras/brotli_udf_db2/](/elements/001-hydrogen/hydrogen/extras/brotli_udf_db2/) - Brotli compression UDF for DB2
- [extras/brotli_udf_mysql/](/elements/001-hydrogen/hydrogen/extras/brotli_udf_mysql/) - Brotli compression UDF for MySQL/MariaDB
- [extras/brotli_udf_postgresql/](/elements/001-hydrogen/hydrogen/extras/brotli_udf_postgresql/) - Brotli compression UDF for PostgreSQL
- [extras/brotli_udf_sqlite/](/elements/001-hydrogen/hydrogen/extras/brotli_udf_sqlite/) - Brotli compression UDF for SQLite

Each UDF directory contains C source code, compiled shared libraries, Makefiles, and test scripts for the respective database engine.

</details>

## Directory Structure Overview

Based on the INSTRUCTIONS.md structure and current implementation:

```directory
hydrogen/
├── README.md, INSTRUCTIONS.md, SETUP.md, RELEASES.md, SECRETS.md, SITEMAP.md, STRUCTURE.md
├── configs/                    # Configuration files
│   ├── hydrogen.json          # Main configuration
│   └── hydrogen-env.json      # Environment configuration
├── src/                       # Source code
│   ├── hydrogen.c             # Main entry point
│   ├── hydrogen_not.c         # Alternative entry point
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
├── installer/                # Installer wrapper and scripts
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

### Subsystem Order (from INSTRUCTIONS.md)

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
19. Mirage - Proxy services

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
- `coverage`: Build with coverage instrumentation for testing
- `valgrind`: Memory debugging build
- `default`: Standard development build

### CMake Configuration

The build system uses modular CMake configuration files:

- [cmake/CMakeLists.txt]/elements/001-hydrogen/hydrogen/(cmake/CMakeLists.txt) - Main CMake configuration
- [cmake/CMakeLists-base.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-base.cmake) - Base build configuration
- [cmake/CMakeLists-cache.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-cache.cmake) - Cache configuration
- [cmake/CMakeLists-coverage.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-coverage.cmake) - Coverage build configuration
- [cmake/CMakeLists-debug.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-debug.cmake) - Debug build configuration
- [cmake/CMakeLists-examples.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-examples.cmake) - Examples build configuration
- [cmake/CMakeLists-init.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-init.cmake) - Initialization configuration
- [cmake/CMakeLists-ninja.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-ninja.cmake) - Ninja build system configuration
- [cmake/CMakeLists-output.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-output.cmake) - Output configuration
- [cmake/CMakeLists-package.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-package.cmake) - Packaging configuration
- [cmake/CMakeLists-perf.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-perf.cmake) - Performance build configuration
- [cmake/CMakeLists-regular.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-regular.cmake) - Regular build configuration
- [cmake/CMakeLists-release.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-release.cmake) - Release build configuration
- [cmake/CMakeLists-targets.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-targets.cmake) - Build targets configuration
- [cmake/CMakeLists-unity.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-unity.cmake) - Unity testing configuration
- [cmake/CMakeLists-valgrind.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-valgrind.cmake) - Valgrind build configuration
- [cmake/CMakeLists-version.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-version.cmake) - Version configuration
- [cmake/CMakePresets.json](/elements/001-hydrogen/hydrogen/cmake/CMakePresets.json) - CMake presets configuration
