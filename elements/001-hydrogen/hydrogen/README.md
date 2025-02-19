# hydrogen

## Overview

Initially, this works very much like Moonraker, providing a WebSocket proxy to the Klipper API. Later, this will also be the component that replaces Klipper itself, but that's going to be a little bit further down the road.

This is a C program that is essentially a JSON queue manager. Most of what this program does involves moving elements around in different multithreaded and thread-safe queues. Even logging activities use a queue - the first queue created in fact.

Each queue has its own queue manager that determines what happens when something arrives in its queue. Logging, for example, will look at the queued element and either print it to the console, write it out to a log file, or add it to the database. This is handled separately from whatever generated the log event so *that* process doesn't have to wait for any of these actions to complete before continuing on.

## Documentation

Comprehensive documentation is available in the `docs` directory:

- [API Documentation](docs/api.md) - REST API endpoints and usage
- [Main Documentation](docs/README.md) - Overview and getting started
- [Configuration Guide](docs/configuration.md) - Server configuration and settings
- [WebSocket Interface](docs/web_socket.md) - Real-time communication interface
- [Print Queue Management](docs/print_queue.md) - Print job scheduling and management
- [Release Notes](docs/release_notes.md) - Detailed version history and changes

## Files

- `hydrogen.json` Configuration file for the Hydrogen server.
- `Makefile` Build instructions for compiling the Hydrogen program.
- `src/beryllium.c` Implements G-code analysis functionality.
- `src/beryllium.h` Header file for G-code analysis declarations.
- `src/configuration.c` Handles loading and managing configuration settings.
- `src/configuration.h` Defines configuration-related structures and constants.
- `src/hydrogen.c` Main entry point of the program, initializes components.
- `src/keys.c` Implements secret key generation functionality.
- `src/keys.h` Header file for secret key generation.
- `src/logging.c` Implements logging functionality.
- `src/logging.h` Defines logging-related functions and macros.
- `src/log_queue_manager.c` Manages the log message queue.
- `src/log_queue_manager.h` Header file for log queue management.
- `src/mdns_server.h` Defines mDNS-related structures and functions.
- `src/mdns_linux.c` Implements mDNS functionality for Linux.
- `src/network.h` Defines network-related structures and functions.
- `src/network_linux.c` Implements network functionality for Linux.
- `src/print_queue_manager.c` Manages the print job queue.
- `src/print_queue_manager.h` Header file for print queue management.
- `src/queue.c` Implements a generic queue data structure.
- `src/queue.h` Defines queue-related structures and functions.
- `src/shutdown.c` Implements graceful shutdown procedures.
- `src/shutdown.h` Header file for shutdown management.
- `src/startup.c` Implements system initialization procedures.
- `src/startup.h` Header file for startup management.
- `src/state.c` Implements system state management.
- `src/state.h` Defines state management structures and functions.
- `src/utils.c` Implements utility functions.
- `src/utils.h` Defines utility functions and constants.
- `src/web_server.c` Implements the web server interface.
- `src/web_server.h` Header file for web server interface.
- `src/websocket_server.c` Implements the websocket server interface.
- `src/websocket_server.h` Header file for websocket server interface.
- `src/websocket_server_status.c` Implementation of WebSocket "status" endpoint

## System Dependencies

Core Libraries:
- [pthreads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html) - POSIX thread support for concurrent operations
- [jansson](https://github.com/akheron/jansson) - Efficient and memory-safe JSON parsing/generation
- [microhttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight embedded HTTP server
- [libwebsockets](https://github.com/warmcat/libwebsockets) - Full-duplex WebSocket communication
- [OpenSSL](https://www.openssl.org/) (libssl/libcrypto) - Industry-standard encryption and security
- [libm](https://www.gnu.org/software/libc/manual/html_node/Mathematics.html) - Mathematical operations support
