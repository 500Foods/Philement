/**
 * @file landing-registry.h
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

// Forward declarations
struct AppConfig;  // Forward declaration of AppConfig

/**
 * @brief Check if the Registry is ready to land
 * @returns LandingReadiness struct with readiness status and messages
 * @note AI-guidance: Must verify all other subsystems are inactive
 *       before allowing registry shutdown
 */
LandingReadiness check_registry_landing_readiness(void);

/**
 * @brief Free resources allocated by the Registry
 * 
 * This function frees any resources allocated by the registry.
 * It should be called only after all other subsystems have been
 * shut down and marked as inactive.
 * 
 * @note This is the final cleanup step in the landing sequence
 */
void shutdown_registry(void);

/**
 * @brief Report final registry status during landing
 * @note Provides detailed information about subsystem states
 *       during the final shutdown phase
 */
void report_registry_landing_status(void);

#endif /* LANDING_REGISTRY_H */