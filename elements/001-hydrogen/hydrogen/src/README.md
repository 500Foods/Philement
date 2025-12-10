# Hydrogen src

This is the C source for the hydrogen project.

## Common

- [`globals.c`](/elements/001-hydrogen/hydrogen/src/globals.c) - global functions
- [`globals.h`](/elements/001-hydrogen/hydrogen/src/globals.h) - global variables
- [`hydrogen.c`](/elements/001-hydrogen/hydrogen/src/hydrogen.c) - This is where you'll find main()
- [`hydrogen.h`](/elements/001-hydrogen/hydrogen/src/hydrogen.h) - Included in virtually every source file
- [`hydrogen_not.c`](/elements/001-hydrogen/hydrogen/src/hydrogen_not.c) - Dummy file used for testing

## Support

- [`config`](/elements/001-hydrogen/hydrogen/src/config/) - Configuration file and defaults management
- [`handlers`](/elements/001-hydrogen/hydrogen/src/handlers/) - Generates custom core files, handles SIGINT, SIGURSR1, etc.
- [`landing`](/elements/001-hydrogen/hydrogen/src/landing/) - Subsystem cleanup
- [`launch`](/elements/001-hydrogen/hydrogen/src/launch/) - Subsystem initialization
- [`mutex`]/elements/001-hydrogen/hydrogen/src/mutex/) - Lock handling for safe thread handling
- [`queue`](/elements/001-hydrogen/hydrogen/src/queue/) - Implements subsystems communication
- [`state`](/elements/001-hydrogen/hydrogen/src/state/) - Global state like running, starting, stopping status, and uptime tracking
- [`status`](/elements/001-hydrogen/hydrogen/src/status/) - Comprehensive metrics for CPU, memory, network, filesystems, and service performance in both JSON and Prometheus formats
- [`utils`](/elements/001-hydrogen/hydrogen/src/utils/) - Frequently used functions

## Subsystems

- [`api`](/elements/001-hydrogen/hydrogen/src/api/) - API endpoints
- [`database`](/elements/001-hydrogen/hydrogen/src/database/) - Database management - IBM DB2 (LUW), MySQL/MariaDB, PostgreSQL, SQLite
- [`logging`](/elements/001-hydrogen/hydrogen/src/logging/) - Logging system
- [`mailrelay`](/elements/001-hydrogen/hydrogen/src/mailrelay/) - SMTP services
- [`mdns`](/elements/001-hydrogen/hydrogen/src/mdns/) - Service discovery (client) and advertising (server)
- [`mirage`](/elements/001-hydrogen/hydrogen/src/mirage/) - Proxy services
- [`network`](/elements/001-hydrogen/hydrogen/src/network/) - Network interfaces
- [`notify`](/elements/001-hydrogen/hydrogen/src/notify/) - Notification services
- [`oidc`](/elements/001-hydrogen/hydrogen/src/oidc/) - OpenID Connect
- [`payload`](/elements/001-hydrogen/hydrogen/src/payload/) - Payload management
- [`print`](/elements/001-hydrogen/hydrogen/src/print/) - Print subsystem
- [`registry`](/elements/001-hydrogen/hydrogen/src/registry/) - Subsystem registry
- [`resources`](/elements/001-hydrogen/hydrogen/src/resources/) - Shared resources like queues, configs, etc.
- [`swagger`](/elements/001-hydrogen/hydrogen/src/swagger/) - Swagger documentation via OpenAPI/SwaggerUI
- [`terminal`](/elements/001-hydrogen/hydrogen/src/terminal/) - Terminal interface via xterm.js
- [`threads`](/elements/001-hydrogen/hydrogen/src/threads/) - Thread management via pthreads
- [`webserver`](/elements/001-hydrogen/hydrogen/src/webserver/) - HTTP server via microhttpd
- [`websocket`](/elements/001-hydrogen/hydrogen/src/websocket/) - WebSocket server via libwebsockets
