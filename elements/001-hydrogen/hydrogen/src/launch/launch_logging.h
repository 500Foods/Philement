/*
 * Logging Subsystem Launch Interface
 */

#ifndef LAUNCH_LOGGING_H
#define LAUNCH_LOGGING_H

#include "launch.h"

/**
 * @brief Check if the Logging subsystem is ready to launch
 * @returns LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_logging_launch_readiness(void);

/**
 * @brief Initialize the Logging subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_logging_subsystem(void);

/**
 * @brief Shutdown the Logging subsystem
 */
void shutdown_logging(void);

#endif /* LAUNCH_LOGGING_H */
