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
#include "config.h"  // For queue and buffer constants

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
 * This function loads the resource configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_resources_config(json_t* root, AppConfig* config);

/*
 * Clean up resource configuration
 *
 * This function cleans up the resource configuration. It safely handles
 * NULL pointers and partial initialization.
 *
 * @param config Pointer to ResourceConfig structure to cleanup
 */
void cleanup_resources_config(ResourceConfig* config);

/*
 * Dump resource configuration for debugging
 *
 * This function outputs the current state of the resource configuration
 * in a structured format.
 *
 * @param config Pointer to ResourceConfig structure to dump
 */
void dump_resources_config(const ResourceConfig* config);

#endif /* CONFIG_RESOURCES_H */
