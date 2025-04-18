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
 * Get the registry subsystem's readiness status
 * 
 * This function provides a cached readiness check for the registry subsystem.
 * The result is cached until launch_registry_subsystem() is called, which
 * clears the cache.
 * 
 * Memory Management:
 * - The returned messages are managed by the caching system
 * - Do not free the messages in the returned structure
 * - The cache is automatically cleared during launch
 * 
 * @return LaunchReadiness struct with cached readiness status and messages
 */
LaunchReadiness get_registry_readiness(void);

/**
 * Check if the registry subsystem is ready to launch
 * 
 * Note: Prefer using get_registry_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
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