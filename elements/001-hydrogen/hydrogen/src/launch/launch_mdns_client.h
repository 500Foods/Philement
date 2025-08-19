/*
 * mDNS Client Subsystem Launch Interface
 */

#ifndef LAUNCH_MDNS_CLIENT_H
#define LAUNCH_MDNS_CLIENT_H

#include "launch.h"

/**
 * @brief Initialize the mDNS Client subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_mdns_client_subsystem(void);
LaunchReadiness check_mdns_client_launch_readiness(void);

#endif /* LAUNCH_MDNS_CLIENT_H */
