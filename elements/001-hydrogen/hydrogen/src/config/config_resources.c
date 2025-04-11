/*
 * Resources Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "config.h"
#include "config_utils.h"
#include "config_resources.h"
#include "types/config_queue_constants.h"  // For queue-related constants

// Forward declarations for validation helpers
static int validate_memory_limits(const ResourceConfig* config);
static int validate_queue_settings(const ResourceConfig* config);
static int validate_thread_limits(const ResourceConfig* config);
static int validate_file_limits(const ResourceConfig* config);

bool load_resources_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    ResourceConfig defaults = {
        // Memory limits
        .max_memory_mb = 1024,              // 1GB default
        .max_buffer_size = 1048576,         // 1MB default
        .min_buffer_size = 4096,            // 4KB default

        // Queue settings
        .max_queue_size = DEFAULT_MAX_QUEUE_SIZE,
        .max_queue_memory_mb = DEFAULT_MAX_QUEUE_MEMORY_MB,
        .max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS,
        .queue_timeout_ms = DEFAULT_QUEUE_TIMEOUT_MS,

        // Buffer sizes
        .post_processor_buffer_size = 65536, // 64KB default

        // Thread limits
        .min_threads = 4,
        .max_threads = 32,
        .thread_stack_size = 65536,         // 64KB default

        // File limits
        .max_open_files = 1024,
        .max_file_size_mb = 1024,          // 1GB default
        .max_log_size_mb = 100,            // 100MB default

        // Monitoring settings
        .enforce_limits = true,
        .log_usage = true,
        .check_interval_ms = 5000          // 5 seconds default
    };

    // Copy defaults
    config->resources = defaults;

    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    bool using_defaults = !json_is_object(resources);
    
    log_config_section("SystemResources", using_defaults);
    
    if (!using_defaults) {
        // Memory limits
        json_t* memory = json_object_get(resources, "Memory");
        if (json_is_object(memory)) {
            log_config_item("Memory", "Configured", false);
            
            PROCESS_SIZE(memory, config, resources.max_memory_mb, "MaxMemoryMB", "Memory");
            PROCESS_SIZE(memory, config, resources.max_buffer_size, "MaxBufferSize", "Memory");
            PROCESS_SIZE(memory, config, resources.min_buffer_size, "MinBufferSize", "Memory");
        }

        // Queue settings
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_item("Queues", "Configured", false);
            
            PROCESS_SIZE(queues, config, resources.max_queue_size, "MaxQueueSize", "Queues");
            PROCESS_SIZE(queues, config, resources.max_queue_memory_mb, "MaxQueueMemoryMB", "Queues");
            PROCESS_SIZE(queues, config, resources.max_queue_blocks, "MaxQueueBlocks", "Queues");
            PROCESS_INT(queues, config, resources.queue_timeout_ms, "QueueTimeoutMS", "Queues");
        }

        // Thread limits
        json_t* threads = json_object_get(resources, "Threads");
        if (json_is_object(threads)) {
            log_config_item("Threads", "Configured", false);
            
            PROCESS_INT(threads, config, resources.min_threads, "MinThreads", "Threads");
            PROCESS_INT(threads, config, resources.max_threads, "MaxThreads", "Threads");
            PROCESS_SIZE(threads, config, resources.thread_stack_size, "ThreadStackSize", "Threads");
        }

        // File limits
        json_t* files = json_object_get(resources, "Files");
        if (json_is_object(files)) {
            log_config_item("Files", "Configured", false);
            
            PROCESS_INT(files, config, resources.max_open_files, "MaxOpenFiles", "Files");
            PROCESS_SIZE(files, config, resources.max_file_size_mb, "MaxFileSizeMB", "Files");
            PROCESS_SIZE(files, config, resources.max_log_size_mb, "MaxLogSizeMB", "Files");
        }

        // Resource monitoring
        json_t* monitoring = json_object_get(resources, "Monitoring");
        if (json_is_object(monitoring)) {
            log_config_item("Monitoring", "Configured", false);
            
            PROCESS_BOOL(monitoring, config, resources.enforce_limits, "EnforceLimits", "Monitoring");
            PROCESS_BOOL(monitoring, config, resources.log_usage, "LogUsage", "Monitoring");
            PROCESS_INT(monitoring, config, resources.check_interval_ms, "CheckIntervalMS", "Monitoring");
        }

        // Validate configuration
        if (config_resources_validate(&config->resources) != 0) {
            log_config_item("Status", "Invalid configuration, using defaults", true);
            config->resources = defaults;
            return false;
        }
    } else {
        log_config_item("Status", "Section missing, using defaults", true);
    }

    return true;
}

int config_resources_init(ResourceConfig* config) {
    if (!config) {
        return -1;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ResourceConfig));
    return 0;
}

void config_resources_cleanup(ResourceConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ResourceConfig));
}

static int validate_memory_limits(const ResourceConfig* config) {
    // Basic range checks
    if (config->max_memory_mb < 64 || config->max_memory_mb > 16384) {  // 64MB to 16GB
        return -1;
    }

    if (config->max_buffer_size < config->min_buffer_size) {
        return -1;
    }

    if (config->max_buffer_size > (config->max_memory_mb * 1024 * 1024) / 4) {  // Max 1/4 of total memory
        return -1;
    }

    return 0;
}

static int validate_queue_settings(const ResourceConfig* config) {
    // Queue size and memory checks
    if (config->max_queue_size < MIN_QUEUE_SIZE || config->max_queue_size > MAX_QUEUE_SIZE) {
        return -1;
    }

    if (config->max_queue_memory_mb < MIN_QUEUE_MEMORY_MB || 
        config->max_queue_memory_mb > MAX_QUEUE_MEMORY_MB ||
        config->max_queue_memory_mb > config->max_memory_mb / 2) {  // Max 1/2 of total memory
        return -1;
    }

    if (config->queue_timeout_ms < MIN_QUEUE_TIMEOUT_MS || 
        config->queue_timeout_ms > MAX_QUEUE_TIMEOUT_MS) {
        return -1;
    }

    if (config->max_queue_blocks < MIN_QUEUE_BLOCKS || 
        config->max_queue_blocks > MAX_QUEUE_BLOCKS) {
        return -1;
    }

    return 0;
}

static int validate_thread_limits(const ResourceConfig* config) {
    // Thread count and stack size checks
    if (config->min_threads < 1 || config->min_threads > config->max_threads) {
        return -1;
    }

    if (config->max_threads > 256) {  // Reasonable upper limit
        return -1;
    }

    if (config->thread_stack_size < 16384 || config->thread_stack_size > 1048576) {  // 16KB to 1MB
        return -1;
    }

    return 0;
}

static int validate_file_limits(const ResourceConfig* config) {
    // File descriptor and size checks
    if (config->max_open_files < 64 || config->max_open_files > 65535) {
        return -1;
    }

    if (config->max_file_size_mb > config->max_memory_mb * 2) {  // Max 2x memory size
        return -1;
    }

    if (config->max_log_size_mb < 10 || config->max_log_size_mb > 10240) {  // 10MB to 10GB
        return -1;
    }

    return 0;
}

int config_resources_validate(const ResourceConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate each section
    if (validate_memory_limits(config) != 0 ||
        validate_queue_settings(config) != 0 ||
        validate_thread_limits(config) != 0 ||
        validate_file_limits(config) != 0) {
        return -1;
    }

    // Validate monitoring settings
    if (config->check_interval_ms < 1000 || config->check_interval_ms > 60000) {  // 1s to 1min
        return -1;
    }

    return 0;
}