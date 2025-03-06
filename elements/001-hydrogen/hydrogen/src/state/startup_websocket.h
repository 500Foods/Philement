/*
 * WebSocket Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the WebSocket subsystem.
 * The WebSocket server is completely independent from the HTTP/REST API server to:
 * 1. Allow independent scaling
 * 2. Enhance reliability through isolation
 * 3. Support flexible deployment
 * 4. Enable different security policies
 */

#ifndef HYDROGEN_STARTUP_WEBSOCKET_H
#define HYDROGEN_STARTUP_WEBSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WebSocket server subsystem
 * 
 * This function initializes the WebSocket server for real-time
 * bidirectional communication. It is completely independent from
 * the HTTP/REST API server.
 * 
 * The WebSocket server provides:
 * - Real-time status updates
 * - Live monitoring capabilities
 * - Bidirectional communication
 * - Event-driven updates
 * 
 * @return 1 on success, 0 on failure
 */
int init_websocket_subsystem(void);

/**
 * Check if WebSocket server is running
 * 
 * This function checks if the WebSocket server is currently running
 * and available to handle real-time connections.
 * 
 * @return 1 if running, 0 if not running
 */
int is_websocket_server_running(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_WEBSOCKET_H */