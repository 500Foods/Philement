/*
 * Launch OIDC Subsystem Header
 * 
 * This header defines the interface for the OIDC subsystem launch functionality.
 */

#ifndef LAUNCH_OIDC_H
#define LAUNCH_OIDC_H

#include <stdbool.h>
#include "launch.h"

// OIDC configuration limits
#define MIN_OIDC_PORT 1024
#define MAX_OIDC_PORT 65535
#define MIN_TOKEN_LIFETIME 300        // 5 minutes
#define MAX_TOKEN_LIFETIME 86400      // 24 hours
#define MIN_REFRESH_LIFETIME 3600     // 1 hour
#define MAX_REFRESH_LIFETIME 2592000  // 30 days
#define MIN_KEY_ROTATION_DAYS 1
#define MAX_KEY_ROTATION_DAYS 90

// Function declarations
LaunchReadiness check_oidc_launch_readiness(void);
int launch_oidc_subsystem(void);

#endif /* LAUNCH_OIDC_H */
