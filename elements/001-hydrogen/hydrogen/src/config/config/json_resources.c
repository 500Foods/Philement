/*
 * System Resources configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include "../config.h"
#include "../config_utils.h"
#include "../resources/config_resources.h"
#include "json_resources.h"

bool load_json_resources(json_t* root, AppConfig* config) {
    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    bool using_defaults = !json_is_object(resources);
    
    log_config_section("SystemResources", using_defaults);
    
    if (!using_defaults) {
        // Memory limits
        json_t* memory = json_object_get(resources, "Memory");
        if (json_is_object(memory)) {
            log_config_item("Memory", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(memory, "MaxMemoryMB");
            config->resources.max_memory_mb = get_config_size(val, DEFAULT_MAX_MEMORY_MB);
            log_config_item("MaxMemoryMB", format_int_buffer(config->resources.max_memory_mb), !val, 1);
            
            val = json_object_get(memory, "MaxBufferSize");
            config->resources.max_buffer_size = get_config_size(val, DEFAULT_MAX_BUFFER_SIZE);
            char max_buffer[64];
            snprintf(max_buffer, sizeof(max_buffer), "%sKB", 
                    format_int_buffer(config->resources.max_buffer_size / 1024));
            log_config_item("MaxBufferSize", max_buffer, !val, 1);
            
            val = json_object_get(memory, "MinBufferSize");
            config->resources.min_buffer_size = get_config_size(val, DEFAULT_MIN_BUFFER_SIZE);
            char min_buffer[64];
            snprintf(min_buffer, sizeof(min_buffer), "%sKB", 
                    format_int_buffer(config->resources.min_buffer_size / 1024));
            log_config_item("MinBufferSize", min_buffer, !val, 1);
        }

        // Queue settings
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_item("Queues", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(queues, "MaxQueueSize");
            config->resources.max_queue_size = get_config_size(val, DEFAULT_MAX_QUEUE_SIZE);
            log_config_item("MaxQueueSize", format_int_buffer(config->resources.max_queue_size), !val, 1);
            
            val = json_object_get(queues, "MaxQueueMemoryMB");
            config->resources.max_queue_memory_mb = get_config_size(val, DEFAULT_MAX_QUEUE_MEMORY_MB);
            char queue_mem[64];
            snprintf(queue_mem, sizeof(queue_mem), "%sMB", 
                    format_int_buffer(config->resources.max_queue_memory_mb));
            log_config_item("MaxQueueMemoryMB", queue_mem, !val, 1);
            
            val = json_object_get(queues, "QueueTimeoutMS");
            config->resources.queue_timeout_ms = get_config_int(val, DEFAULT_QUEUE_TIMEOUT_MS);
            char timeout_buffer[64];
            snprintf(timeout_buffer, sizeof(timeout_buffer), "%sms", 
                    format_int_buffer(config->resources.queue_timeout_ms));
            log_config_item("QueueTimeoutMS", timeout_buffer, !val, 1);
        }

        // Thread limits
        json_t* threads = json_object_get(resources, "Threads");
        if (json_is_object(threads)) {
            log_config_item("Threads", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(threads, "MinThreads");
            config->resources.min_threads = get_config_int(val, DEFAULT_MIN_THREADS);
            log_config_item("MinThreads", format_int_buffer(config->resources.min_threads), !val, 1);
            
            val = json_object_get(threads, "MaxThreads");
            config->resources.max_threads = get_config_int(val, DEFAULT_MAX_THREADS);
            log_config_item("MaxThreads", format_int_buffer(config->resources.max_threads), !val, 1);
            
            val = json_object_get(threads, "ThreadStackSize");
            config->resources.thread_stack_size = get_config_size(val, DEFAULT_THREAD_STACK_SIZE);
            char stack_buffer[64];
            snprintf(stack_buffer, sizeof(stack_buffer), "%sKB", 
                    format_int_buffer(config->resources.thread_stack_size / 1024));
            log_config_item("ThreadStackSize", stack_buffer, !val, 1);
        }

        // File limits
        json_t* files = json_object_get(resources, "Files");
        if (json_is_object(files)) {
            log_config_item("Files", "Configured", false, 0);
            
            json_t* val;
            val = json_object_get(files, "MaxOpenFiles");
            config->resources.max_open_files = get_config_int(val, DEFAULT_MAX_OPEN_FILES);
            log_config_item("MaxOpenFiles", format_int_buffer(config->resources.max_open_files), !val, 1);
            
            val = json_object_get(files, "MaxFileSizeMB");
            config->resources.max_file_size_mb = get_config_size(val, DEFAULT_MAX_FILE_SIZE_MB);
            char file_size[64];
            snprintf(file_size, sizeof(file_size), "%sMB", 
                    format_int_buffer(config->resources.max_file_size_mb));
            log_config_item("MaxFileSizeMB", file_size, !val, 1);
            
            val = json_object_get(files, "MaxLogSizeMB");
            config->resources.max_log_size_mb = get_config_size(val, DEFAULT_MAX_LOG_SIZE_MB);
            char log_size[64];
            snprintf(log_size, sizeof(log_size), "%sMB", 
                    format_int_buffer(config->resources.max_log_size_mb));
            log_config_item("MaxLogSizeMB", log_size, !val, 1);
        }

        // Resource monitoring
        json_t* monitoring = json_object_get(resources, "Monitoring");
        if (json_is_object(monitoring)) {
            log_config_item("Monitoring", "Configured", false, 0);
            
            json_t* enforce = json_object_get(monitoring, "EnforceLimits");
            config->resources.enforce_limits = get_config_bool(enforce, true);
            log_config_item("EnforceLimits", config->resources.enforce_limits ? "true" : "false", !enforce, 1);
            
            json_t* log_usage = json_object_get(monitoring, "LogUsage");
            config->resources.log_usage = get_config_bool(log_usage, true);
            log_config_item("LogUsage", config->resources.log_usage ? "true" : "false", !log_usage, 1);
            
            json_t* val = json_object_get(monitoring, "CheckIntervalMS");
            config->resources.check_interval_ms = get_config_int(val, 5000);  // Default 5 seconds
            char interval_buffer[64];
            snprintf(interval_buffer, sizeof(interval_buffer), "%sms", 
                    format_int_buffer(config->resources.check_interval_ms));
            log_config_item("CheckIntervalMS", interval_buffer, !val, 1);
        }

        // Validate configuration
        if (config_resources_validate(&config->resources) != 0) {
            log_config_item("Status", "Invalid configuration, using defaults", true, 0);
            config_resources_init(&config->resources);
        }

        return true;
    } else {
        // Set defaults if no resources configuration is provided
        config_resources_init(&config->resources);
        
        log_config_item("Status", "Section missing, using defaults", true, 0);
        return true;
    }
}