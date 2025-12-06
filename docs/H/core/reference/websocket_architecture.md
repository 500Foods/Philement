# WebSocket Architecture

## Overview

The WebSocket server in the Hydrogen project facilitates real-time communication between clients and the server, enabling features such as live updates and interactive control. It is built on top of the libwebsockets library, which provides the underlying WebSocket protocol implementation.

## Components

- **WebSocket Server Context**: Manages the overall state and configuration of the WebSocket server, including the libwebsockets context, connection tracking, and server metrics.
- **Connection Management**: Handles the lifecycle of client connections, including establishment, authentication, and closure.
- **Shutdown Process**: Manages the graceful termination of connections and cleanup of resources during server shutdown.

## Shutdown Optimization (2025-Jul-18)

Efforts were made to optimize the WebSocket server shutdown process to reduce exit time to milliseconds, particularly for test 19. Key changes include:

- Implemented custom connection tracking with a linked list to manage active connections.
- Developed a force-close mechanism using `lws_close_reason` and `lws_callback_on_writable` to avoid compilation errors with undefined constants like `PENDING_TIMEOUT_KILLABLE`.
- Integrated the force-close mechanism into the shutdown flow in `stop_websocket_server` and `cleanup_websocket_server`.
- Added detailed timing logging to measure shutdown duration in milliseconds, identifying bottlenecks.
- Attempted multiple optimization strategies to mitigate delays in `lws_context_destroy`, including extended service cancellations and thread termination adjustments.
- Explored custom context destruction to bypass `lws_context_destroy`, but this failed due to inaccessible internal structures of libwebsockets.
- Despite optimizations, the delay in `lws_context_destroy` persists, indicating a limitation within the libwebsockets library that may require further investigation or upstream fixes.

## Key Files

- [src/websocket/websocket_server_shutdown.c](../../src/websocket/websocket_server_shutdown.c)
- [src/websocket/websocket_server_context.c](../../src/websocket/websocket_server_context.c)
- [src/websocket/websocket_server_internal.h](../../src/websocket/websocket_server_internal.h)

## Challenges

- The primary challenge in shutdown optimization is the significant delay caused by `lws_context_destroy`, which appears to be an internal limitation of libwebsockets. This delay impacts the overall exit time, preventing achievement of millisecond-level shutdown for test 19.
- Direct manipulation of libwebsockets internal structures for custom cleanup is not feasible due to their inaccessibility in the public API.

## Future Considerations

- Investigate potential patches or updates to libwebsockets that address shutdown performance.
- Explore alternative WebSocket libraries if shutdown performance remains a critical issue.
- Consider asynchronous or background cleanup processes to minimize perceived shutdown time in test scenarios.
