/*
 * Resources Configuration
 *
 * Manages system resource limits and monitoring settings
 */

#ifndef CONFIG_RESOURCES_H
#define CONFIG_RESOURCES_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For forward declarations
#include "types/config_queue_constants.h"  // For queue-related constants

/*
 * Resource configuration structure - defines system resource limits
 */
typedef struct ResourceConfig {
    // Memory limits
    size_t max_memory_mb;
    size_t max_buffer_size;
    size_t min_buffer_size;

    // Queue settings
    size_t max_queue_size;
    size_t max_queue_memory_mb;
    size_t max_queue_blocks;
    int queue_timeout_ms;

    // Buffer sizes
    size_t post_processor_buffer_size;

    // Thread limits
    int min_threads;
    int max_threads;
    size_t thread_stack_size;

    // File limits
    int max_open_files;
    size_t max_file_size_mb;
    size_t max_log_size_mb;

    // Monitoring settings
    bool enforce_limits;
    bool log_usage;
    int check_interval_ms;
} ResourceConfig;

/*
 * Load resource configuration from JSON
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_resources_config(json_t* root, AppConfig* config);

/*
 * Initialize resource configuration with defaults
 *
 * @param config The configuration structure to initialize
 * @return 0 on success, -1 on error
 */
int config_resources_init(ResourceConfig* config);

/*
 * Clean up resource configuration
 *
 * @param config The configuration structure to clean up
 */
void config_resources_cleanup(ResourceConfig* config);

/*
 * Validate resource configuration
 *
 * @param config The configuration structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_resources_validate(const ResourceConfig* config);

#endif /* CONFIG_RESOURCES_H */