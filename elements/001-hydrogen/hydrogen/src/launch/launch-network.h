/*
 * Network Subsystem Launch Readiness Check
 * 
 * This module provides the launch readiness check for the network subsystem.
 * It verifies that network interfaces are available and properly configured
 * before allowing the system to proceed with initialization.
 */

#ifndef HYDROGEN_LAUNCH_NETWORK_H
#define HYDROGEN_LAUNCH_NETWORK_H

#include "launch.h"

/*
 * Check network subsystem launch readiness
 *
 * This function performs the following checks:
 * - System state (shutdown in progress, startup/running state)
 * - Configuration loaded
 * - Network interfaces available (Go if > 0)
 * - For each interface:
 *   - Status (Go = up, No-Go = down)
 *   - Availability in configuration
 * - Final decision based on whether > 0 interfaces are up
 *
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_network_launch_readiness(void);

/*
 * Network subsystem shutdown flag
 * This flag is used to signal the network subsystem to shut down
 */
extern volatile int network_system_shutdown;

#endif /* HYDROGEN_LAUNCH_NETWORK_H */