/*
 * Safe Shutdown and Signal Handling for 3D Printer Control
 * 
 * Why Careful Shutdown Matters:
 * 1. Hardware Protection
 *    - Safe temperature reduction
 *    - Motor power management
 *    - Head parking sequence
 *    - Power system shutdown
 * 
 * 2. Print Preservation
 *    Why This Matters?
 *    - Save print progress
 *    - Enable job recovery
 *    - Preserve calibration
 *    - Maintain statistics
 * 
 * 3. Resource Cleanup
 *    Why So Thorough?
 *    - Free system memory
 *    - Close network connections
 *    - Release file handles
 *    - Clear queued operations
 * 
 * 4. Signal Handling
 *    Why These Features?
 *    - SIGINT: Clean shutdown (Ctrl+C)
 *    - SIGTERM: Clean shutdown (kill)
 *    - SIGHUP: Restart with config reload
 *    - State preservation
 * 
 * 5. System Integrity
 *    Why This Approach?
 *    - Configuration persistence
 *    - State consistency
 *    - Data integrity
 *    - Recovery readiness
 */

#ifndef HYDROGEN_SHUTDOWN_H
#define HYDROGEN_SHUTDOWN_H

#include <signal.h>

// Flag indicating if a restart was requested (e.g., via SIGHUP)
extern volatile sig_atomic_t restart_requested;

// Handle various signals (SIGINT, SIGTERM, SIGHUP)
// Coordinates emergency stops, graceful termination, and restart
void signal_handler(int signum);

// Restart the application after graceful shutdown
// Re-reads configuration and reinitializes all subsystems
int restart_hydrogen(const char* config_path);

// Perform coordinated shutdown of all system components
// Ensures safe hardware state and resource cleanup
void graceful_shutdown(void);

#endif // HYDROGEN_SHUTDOWN_H