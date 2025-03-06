/*
 * Terminal Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the terminal subsystem.
 * The terminal system provides console-based interaction and I/O management.
 * 
 * Dependencies:
 * - Requires web server for HTTP-based terminal access
 * - Requires WebSocket server for real-time updates
 */

#ifndef HYDROGEN_STARTUP_TERMINAL_H
#define HYDROGEN_STARTUP_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize terminal subsystem
 * 
 * This function initializes the terminal interface system.
 * It provides console interaction capabilities:
 * - Command-line interface
 * - Real-time status display
 * - Interactive debugging
 * - System monitoring
 * 
 * Features:
 * - Terminal I/O management
 * - Signal handling
 * - Input buffering
 * - Output formatting
 * 
 * @return 1 on success, 0 on failure
 */
int init_terminal_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_TERMINAL_H */