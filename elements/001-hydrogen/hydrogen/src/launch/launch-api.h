/*
 * API Subsystem Launch Interface
 */

#ifndef LAUNCH_API_H
#define LAUNCH_API_H

#include "launch.h"

/**
 * @brief Check if the API subsystem is ready to launch
 * @returns LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_api_launch_readiness(void);

/**
 * @brief Initialize the API subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_api_subsystem(void);

/**
 * @brief Shutdown the API subsystem
 */
void shutdown_api(void);

#endif /* LAUNCH_API_H */