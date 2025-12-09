# Hydrogen src

This is the C source for the hydrogen project.

## Common

- [`globals.c`](globals.c) - global functions
- [`globals.h`](globals.h) - global variables
- [`hydrogen.c`](hydrogen.c) - This is where you'll find main()
- [`hydrogen.h`](hydrogen.h) - Included in virtually every source file
- [`hydrogen_not.c`](hydrogen_not.c) - Dummy file used for testing

## Support

- [`config`](config/) - Configuration file and defaults management
- [`handlers`](handlers/) - Generates custom core files, handles SIGINT, SIGURSR1, etc.
- [`landing`](landing/) - Subsystem cleanup
- [`launch`](launch/) - Subsystem initialization
- [`mutex`](mutex/) - Lock handling for safe thread handling
- [`queue`](queue/) - Implements subsystems communication
- [`state`](state/) - Global state like running, starting, stopping status, and uptime tracking
- [`status`](status/) - Comprehensive metrics for CPU, memory, network, filesystems, and service performance in both JSON and Prometheus formats
- [`utils`](utils/) - Frequently used functions

## Subsystems

- [`api`](api/) - API endpoints
- [`database`](database/) - Database management - IBM DB2 (LUW), MySQL/MariaDB, PostgreSQL, SQLite
- [`logging`](logging/) - Logging system
- [`mailrelay`](mailrelay/) - SMTP services
- [`mdns`](mdns/) - Service discovery (client) and advertising (server)
- [`mirage`](mirage/) - Proxy services
- [`network`](network/) - Network interfaces
- [`notify`](notify/) - Notification services
- [`oidc`](oidc/) - OpenID Connect
- [`payload`](payload/) - Payload management
- [`print`](print/) - Print subsystem
- [`registry`](registry/) - Subsystem registry
- [`resources`](resources/) - Shared resources like queues, configs, etc.
- [`swagger`](swagger/) - Swagger documentation via OpenAPI/SwaggerUI
- [`terminal`](terminal/) - Terminal interface via xterm.js
- [`threads`](threads/) - Thread management via pthreads
- [`webserver`](webserver/) - HTTP server via microhttpd
- [`websocket`](websocket/) - WebSocket server via libwebsockets
