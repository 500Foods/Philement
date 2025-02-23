/*
 * Real-time Communication for 3D Printer Control
 * 
 * Why WebSockets Matter:
 * 1. Real-time Control
 *    - Immediate command execution
 *    - Status synchronization
 *    - Emergency stop capability
 *    - Motion coordination
 * 
 * 2. Bidirectional Updates
 *    Why This Design?
 *    - Temperature monitoring
 *    - Position tracking
 *    - Error reporting
 *    - Progress updates
 * 
 * 3. Security Features
 *    Why These Measures?
 *    - Authentication system
 *    - Command validation
 *    - Connection control
 *    - Data protection
 * 
 * 4. Connection Management
 *    Why So Robust?
 *    - Auto-reconnection
 *    - State preservation
 *    - Error recovery
 *    - Load balancing
 * 
 * 5. Status Monitoring
 *    Why These Capabilities?
 *    - Print progress tracking
 *    - Hardware health checks
 *    - Resource monitoring
 *    - Performance metrics
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key);

// Start the WebSocket server
int start_websocket_server();

// Stop the WebSocket server
void stop_websocket_server();

// Cleanup WebSocket server resources
void cleanup_websocket_server();

// Get the actual port the WebSocket server is bound to
int get_websocket_port(void);

// Function prototypes for endpoint handlers
void handle_status_request(struct lws *wsi);

#endif // WEBSOCKET_SERVER_H
