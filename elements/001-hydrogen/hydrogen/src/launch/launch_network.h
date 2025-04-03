/*
 * Network Subsystem Launch Readiness Check
 * 
 * This module provides the launch readiness check for the network subsystem.
 * It verifies that network interfaces are available and properly configured
 * before allowing the system to proceed with initialization.
 * 
 * Key Features:
 * - Network interface discovery and testing
 * - IPv4/IPv6 ping response time measurement
 * - Registry integration for subsystem state management
 * - Dependency tracking (requires Threads subsystem)
 */

#ifndef HYDROGEN_LAUNCH_NETWORK_H
#define HYDROGEN_LAUNCH_NETWORK_H

#include "launch.h"
#include "../registry/registry.h"
#include "../logging/logging.h"

#define LOG_LINE_BREAK "――――――――――――――――――――――――――――――――――――――――"

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
 *   - Ping response time for IPv4/IPv6 addresses
 * - Registry state and dependencies
 * - Final decision based on whether > 0 interfaces are up
 *
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_network_launch_readiness(void);

/**
 * Launch the network subsystem
 * 
 * This function performs the following steps:
 * 1. Verify system state
 * 2. Initialize network subsystem
 * 3. Enumerate network interfaces
 * 4. Test interfaces with ping
 * 5. Update registry and verify state
 * 
 * @return 1 on success, 0 on failure
 */
int launch_network_subsystem(void);

/*
 * Network subsystem shutdown flag
 * This flag is used to signal the network subsystem to shut down
 */
extern volatile int network_system_shutdown;

#endif /* HYDROGEN_LAUNCH_NETWORK_H */