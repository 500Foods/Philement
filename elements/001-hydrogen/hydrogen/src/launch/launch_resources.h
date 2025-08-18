/*
 * Launch Resources Subsystem Header
 * 
 * This header defines the interface for the resources subsystem launch functionality.
 */

#ifndef LAUNCH_RESOURCES_H
#define LAUNCH_RESOURCES_H

#include <stdbool.h>
#include "launch.h"
#include "../config/config.h"  // For queue and buffer constants

// Resource limits
#define MIN_MEMORY_MB 64
#define MAX_MEMORY_MB 16384
#define MIN_RESOURCE_BUFFER_SIZE 1024
#define MAX_RESOURCE_BUFFER_SIZE (1024 * 1024 * 1024)
#define MIN_THREADS 2
#define MAX_THREADS 1024
#define MIN_STACK_SIZE (16 * 1024)
#define MAX_STACK_SIZE (8 * 1024 * 1024)
#define MIN_OPEN_FILES 64
#define MAX_OPEN_FILES 65536
#define MIN_LOG_SIZE_MB 1
#define MAX_LOG_SIZE_MB 10240
#define MIN_CHECK_INTERVAL_MS 100
#define MAX_CHECK_INTERVAL_MS 60000

// Function declarations
LaunchReadiness check_resources_launch_readiness(void);
int launch_resources_subsystem(void);

#endif /* LAUNCH_RESOURCES_H */
