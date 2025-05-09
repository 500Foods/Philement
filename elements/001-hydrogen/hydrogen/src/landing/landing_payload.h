/**
 * @file landing_payload.h
 * @brief Payload subsystem landing (shutdown) readiness checks and cleanup
 *
 * @note AI-guidance: This subsystem must ensure proper cleanup of
 *       encrypted payloads and OpenSSL resources.
 */

#ifndef LANDING_PAYLOAD_H
#define LANDING_PAYLOAD_H

#include "../config/config.h"
#include "landing.h"
#include "../payload/payload.h"
#include "../state/state_types.h"

// Forward declarations
struct AppConfig;  // Forward declaration of AppConfig

/**
 * @brief Check if the Payload subsystem is ready to land
 * @returns LaunchReadiness struct with readiness status and messages
 * @note AI-guidance: Must verify no active payload operations before shutdown
 */
LaunchReadiness check_payload_landing_readiness(void);

/**
 * @brief Free resources allocated during payload launch
 * 
 * This function frees any resources allocated during the payload launch phase.
 * It should be called during the LANDING: PAYLOAD phase of the application.
 * After freeing resources, it unregisters the Payload subsystem to prevent
 * it from being stopped again during the LANDING: SUBSYSTEM REGISTRY phase.
 */
void free_payload_resources(void);

/**
 * @brief Land the payload subsystem
 * 
 * This function handles the complete shutdown sequence for the payload subsystem.
 * It ensures proper cleanup of resources and updates the subsystem state.
 * 
 * @return int 1 on success, 0 on failure
 */
int land_payload_subsystem(void);

#endif /* LANDING_PAYLOAD_H */