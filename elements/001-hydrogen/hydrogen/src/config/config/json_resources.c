/*
 * System Resources configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"
#include "../config_utils.h"
#include "../resources/config_resources.h"
#include "json_resources.h"
#include "../logging/config_logging_utils.h"

bool load_json_resources(json_t* root, AppConfig* config) {
    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        log_config_section_header("SystemResources");
        
        // Memory limits
        json_t* memory = json_object_get(resources, "Memory");
        if (json_is_object(memory)) {
            log_config_section_item("Memory", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(memory, "MaxMemoryMB");
            config->resources.max_memory_mb = get_config_size(val, DEFAULT_MAX_MEMORY_MB);
            log_config_section_item("MaxMemoryMB", "%zu", LOG_LEVEL_STATE, !val, 1, "MB", "MB", "Config",
                    config->resources.max_memory_mb);
            
            val = json_object_get(memory, "MaxBufferSize");
            config->resources.max_buffer_size = get_config_size(val, DEFAULT_MAX_BUFFER_SIZE);
            log_config_section_item("MaxBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "KB", "Config",
                    config->resources.max_buffer_size);
            
            val = json_object_get(memory, "MinBufferSize");
            config->resources.min_buffer_size = get_config_size(val, DEFAULT_MIN_BUFFER_SIZE);
            log_config_section_item("MinBufferSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "KB", "Config",
                    config->resources.min_buffer_size);
        }

        // Queue settings
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_section_item("Queues", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(queues, "MaxQueueSize");
            config->resources.max_queue_size = get_config_size(val, DEFAULT_MAX_QUEUE_SIZE);
            log_config_section_item("MaxQueueSize", "%zu", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->resources.max_queue_size);
            
            val = json_object_get(queues, "MaxQueueMemoryMB");
            config->resources.max_queue_memory_mb = get_config_size(val, DEFAULT_MAX_QUEUE_MEMORY_MB);
            log_config_section_item("MaxQueueMemoryMB", "%zu", LOG_LEVEL_STATE, !val, 1, "MB", "MB", "Config",
                    config->resources.max_queue_memory_mb);
            
            val = json_object_get(queues, "QueueTimeoutMS");
            config->resources.queue_timeout_ms = get_config_int(val, DEFAULT_QUEUE_TIMEOUT_MS);
            log_config_section_item("QueueTimeoutMS", "%d", LOG_LEVEL_STATE, !val, 1, "ms", "ms", "Config",
                    config->resources.queue_timeout_ms);
        }

        // Thread limits
        json_t* threads = json_object_get(resources, "Threads");
        if (json_is_object(threads)) {
            log_config_section_item("Threads", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(threads, "MinThreads");
            config->resources.min_threads = get_config_int(val, DEFAULT_MIN_THREADS);
            log_config_section_item("MinThreads", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->resources.min_threads);
            
            val = json_object_get(threads, "MaxThreads");
            config->resources.max_threads = get_config_int(val, DEFAULT_MAX_THREADS);
            log_config_section_item("MaxThreads", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->resources.max_threads);
            
            val = json_object_get(threads, "ThreadStackSize");
            config->resources.thread_stack_size = get_config_size(val, DEFAULT_THREAD_STACK_SIZE);
            log_config_section_item("ThreadStackSize", "%zu", LOG_LEVEL_STATE, !val, 1, "B", "KB", "Config",
                    config->resources.thread_stack_size);
        }

        // File limits
        json_t* files = json_object_get(resources, "Files");
        if (json_is_object(files)) {
            log_config_section_item("Files", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* val;
            val = json_object_get(files, "MaxOpenFiles");
            config->resources.max_open_files = get_config_int(val, DEFAULT_MAX_OPEN_FILES);
            log_config_section_item("MaxOpenFiles", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config",
                    config->resources.max_open_files);
            
            val = json_object_get(files, "MaxFileSizeMB");
            config->resources.max_file_size_mb = get_config_size(val, DEFAULT_MAX_FILE_SIZE_MB);
            log_config_section_item("MaxFileSizeMB", "%zu", LOG_LEVEL_STATE, !val, 1, "MB", "MB", "Config",
                    config->resources.max_file_size_mb);
            
            val = json_object_get(files, "MaxLogSizeMB");
            config->resources.max_log_size_mb = get_config_size(val, DEFAULT_MAX_LOG_SIZE_MB);
            log_config_section_item("MaxLogSizeMB", "%zu", LOG_LEVEL_STATE, !val, 1, "MB", "MB", "Config",
                    config->resources.max_log_size_mb);
        }

        // Resource monitoring
        json_t* monitoring = json_object_get(resources, "Monitoring");
        if (json_is_object(monitoring)) {
            log_config_section_item("Monitoring", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* enforce = json_object_get(monitoring, "EnforceLimits");
            config->resources.enforce_limits = get_config_bool(enforce, true);
            log_config_section_item("EnforceLimits", "%s", LOG_LEVEL_STATE, !enforce, 1, NULL, NULL, "Config",
                    config->resources.enforce_limits ? "true" : "false");
            
            json_t* log_usage = json_object_get(monitoring, "LogUsage");
            config->resources.log_usage = get_config_bool(log_usage, true);
            log_config_section_item("LogUsage", "%s", LOG_LEVEL_STATE, !log_usage, 1, NULL, NULL, "Config",
                    config->resources.log_usage ? "true" : "false");
            
            json_t* val = json_object_get(monitoring, "CheckIntervalMS");
            config->resources.check_interval_ms = get_config_int(val, 5000);  // Default 5 seconds
            log_config_section_item("CheckIntervalMS", "%d", LOG_LEVEL_STATE, !val, 1, "ms", "ms", "Config",
                    config->resources.check_interval_ms);
        }

        // Validate configuration
        if (config_resources_validate(&config->resources) != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            config_resources_init(&config->resources);
        }

        return true;
    } else {
        // Set defaults if no resources configuration is provided
        config_resources_init(&config->resources);
        
        log_config_section_header("SystemResources");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        return true;
    }
}