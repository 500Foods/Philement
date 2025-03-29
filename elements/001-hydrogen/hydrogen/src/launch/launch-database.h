/*
 * Database Subsystem Launch Interface
 */

#ifndef LAUNCH_DATABASE_H
#define LAUNCH_DATABASE_H

#include "launch.h"

/**
 * @brief Check if the Database subsystem is ready to launch
 * @returns LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_database_launch_readiness(void);

/**
 * @brief Initialize the Database subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_database_subsystem(void);

/**
 * @brief Shutdown the Database subsystem
 */
void shutdown_database(void);

#endif /* LAUNCH_DATABASE_H */