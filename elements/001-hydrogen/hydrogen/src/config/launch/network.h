/*
 * Network Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the network subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the network interface discovery logic in src/network/network_linux.c
 * to ensure the network interfaces can be successfully discovered and used later.
 */

#ifndef HYDROGEN_LAUNCH_NETWORK_H
#define HYDROGEN_LAUNCH_NETWORK_H

#include "launch.h"

/*
 * Check if the network subsystem is ready to launch
 * 
 * This function performs the following checks:
 * 1. System state (shutdown in progress, startup/running state)
 * 2. Configuration loaded
 * 3. Network interfaces available
 * 4. Interface status (up/down)
 * 5. Interface availability in configuration
 * 
 * @return LaunchReadiness struct with subsystem name, ready status, and message array
 */
LaunchReadiness check_network_launch_readiness(void);

#endif /* HYDROGEN_LAUNCH_NETWORK_H */