/**
 * @file landing_registry.h
 * @brief Registry landing (shutdown) readiness checks and cleanup
 *
 * @note AI-guidance: This is the last subsystem to shut down.
 *       It must verify all other subsystems are inactive before
 *       proceeding with its own shutdown.
 */

#ifndef LANDING_REGISTRY_H
#define LANDING_REGISTRY_H

#include "../config/config.h"
#include "landing.h"
#include "../registry/registry.h"
#include "../state/state_types.h"

// Forward declarations
struct AppConfig;  // Forward declaration of AppConfig

/**
 * @brief Check if the Registry is ready to land
 * @returns LaunchReadiness struct with readiness status and messages
 * @note AI-guidance: Must verify all other subsystems are inactive
 *       before allowing registry shutdown
 */
LaunchReadiness check_registry_landing_readiness(void);

/**
 * @brief Land the Registry subsystem
 * 
 * This function handles cleanup of the registry subsystem. Its behavior
 * differs based on whether this is a full shutdown or part of a restart:
 * 
 * - During restart (is_restart=true): Preserves the Registry's state while
 *   resetting other subsystems. This ensures the Registry remains functional
 *   for the subsequent relaunch.
 * 
 * - During shutdown (is_restart=false): Performs complete cleanup, freeing
 *   all resources and resetting all subsystem states.
 * 
 * @param is_restart true if this is part of a restart sequence, false for shutdown
 * @return int 1 on success, 0 on failure
 */
int land_registry_subsystem(bool is_restart);

/**
 * @brief Report final registry status during landing
 * @note Provides detailed information about subsystem states
 *       during the final shutdown phase
 */
void report_registry_landing_status(void);

#endif /* LANDING_REGISTRY_H */