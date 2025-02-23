/*
 * Safe Shutdown Sequence for 3D Printer Control
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
 * 4. Emergency Handling
 *    Why These Features?
 *    - Immediate stop capability
 *    - Hardware protection
 *    - State preservation
 *    - Error logging
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

// Handle interrupt signals (Ctrl+C) with safe shutdown sequence
// Coordinates emergency stops and graceful termination
void inthandler(int signum);

// Perform coordinated shutdown of all system components
// Ensures safe hardware state and resource cleanup
void graceful_shutdown(void);

#endif // HYDROGEN_SHUTDOWN_H