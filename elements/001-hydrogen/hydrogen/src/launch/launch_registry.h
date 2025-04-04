/*
 * Launch Registry Header
 * 
 * Declares functions for launching the registry subsystem.
 * Note that registry initialization happens before the launch sequence.
 */

#ifndef LAUNCH_REGISTRY_H
#define LAUNCH_REGISTRY_H

#include "launch.h"

/**
 * Check if the registry subsystem is ready to launch
 * 
 * @return LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_registry_launch_readiness(void);

/**
 * Launch the registry subsystem
 * 
 * Note: Registry is pre-initialized before the launch sequence.
 * This primarily handles proper status tracking.
 * 
 * @return 1 if launch successful, 0 on failure
 */
int launch_registry_subsystem(void);

#endif /* LAUNCH_REGISTRY_H */