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
 * Behavior differs during restart vs initial launch:
 * - During restart (is_restart=true): More lenient state verification,
 *   preserves existing registry state, and won't trigger shutdown on
 *   unexpected states.
 * - During initial launch (is_restart=false): Strict state verification,
 *   requires transition to RUNNING state.
 * 
 * @param is_restart true if this is part of a restart sequence
 * @return 1 if launch successful, 0 on failure
 */
int launch_registry_subsystem(bool is_restart);

#endif /* LAUNCH_REGISTRY_H */