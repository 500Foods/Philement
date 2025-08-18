/*
 * WebSocket Subsystem Launch Interface
 */

#ifndef LAUNCH_WEBSOCKET_H
#define LAUNCH_WEBSOCKET_H

#include "launch.h"

/**
 * @brief Get websocket subsystem readiness status (cached)
 * @returns LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness get_websocket_readiness(void);

/**
 * @brief Check if websocket server is running
 * @returns 1 if running, 0 if not running
 */
int is_websocket_server_running(void);

/**
 * @brief Initialize the WebSocket subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_websocket_subsystem(void);

#endif /* LAUNCH_WEBSOCKET_H */
