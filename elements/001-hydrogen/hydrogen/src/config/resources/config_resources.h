/*
 * System Resources Configuration
 *
 * Defines the configuration structure and defaults for system-wide resource limits.
 * This includes settings for queue sizes, buffer sizes, and memory allocations.
 *
 * Design Decisions:
 * - Conservative default values to ensure stability
 * - Memory-conscious buffer sizes for embedded systems
 * - Queue sizes optimized for typical workloads
 * - Configurable limits for different deployment scenarios
 */

#ifndef HYDROGEN_CONFIG_RESOURCES_H
#define HYDROGEN_CONFIG_RESOURCES_H

// Core system headers
#include <stddef.h>

// Project headers
#include "../config_forward.h"

// Default queue system values
#define DEFAULT_MAX_QUEUE_BLOCKS 1024
#define DEFAULT_QUEUE_HASH_SIZE 256
#define DEFAULT_QUEUE_CAPACITY 1000

// Default buffer sizes (in bytes)
#define DEFAULT_MESSAGE_BUFFER_SIZE 4096
#define DEFAULT_MAX_LOG_MESSAGE_SIZE 1024
#define DEFAULT_LINE_BUFFER_SIZE 256
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE (64 * 1024)  // 64KB
#define DEFAULT_LOG_BUFFER_SIZE (32 * 1024)  // 32KB
#define DEFAULT_JSON_MESSAGE_SIZE 8192
#define DEFAULT_LOG_ENTRY_SIZE 512

// Resource configuration structure
struct ResourceConfig {
    size_t max_queue_blocks;
    size_t queue_hash_size;
    size_t default_capacity;
    size_t message_buffer_size;
    size_t max_log_message_size;
    size_t line_buffer_size;
    size_t post_processor_buffer_size;
    size_t log_buffer_size;
    size_t json_message_size;
    size_t log_entry_size;
};

/*
 * Initialize resource configuration with default values
 *
 * This function initializes a new ResourceConfig structure with conservative
 * default values suitable for most deployments. These values can be adjusted
 * based on the specific requirements and available system resources.
 *
 * @param config Pointer to ResourceConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_resources_init(ResourceConfig* config);

/*
 * Free resources allocated for resource configuration
 *
 * This function cleans up any resources allocated by config_resources_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to ResourceConfig structure to cleanup
 */
void config_resources_cleanup(ResourceConfig* config);

/*
 * Validate resource configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all buffer sizes are within acceptable ranges
 * - Checks for invalid combinations of settings
 * - Ensures queue-related values are properly sized
 *
 * @param config Pointer to ResourceConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any buffer size is 0 or exceeds system limits
 * - If queue settings are inconsistent
 */
int config_resources_validate(const ResourceConfig* config);

#endif /* HYDROGEN_CONFIG_RESOURCES_H */