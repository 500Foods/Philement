/*
 * Resources Configuration Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_resources.h"

bool load_resources_config(json_t* root, AppConfig* config) {
    bool success = true;
    ResourceConfig* resources_config = &config->resources;

    // Zero out the config structure
    memset(resources_config, 0, sizeof(ResourceConfig));

    // Initialize with secure defaults
    resources_config->max_memory_mb = 1024;              // 1GB default
    resources_config->max_buffer_size = 1048576;         // 1MB default
    resources_config->min_buffer_size = 4096;            // 4KB default

    resources_config->max_queue_size = DEFAULT_MAX_QUEUE_SIZE;
    resources_config->max_queue_memory_mb = DEFAULT_MAX_QUEUE_MEMORY_MB;
    resources_config->max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
    resources_config->queue_timeout_ms = DEFAULT_QUEUE_TIMEOUT_MS;

    resources_config->post_processor_buffer_size = 65536; // 64KB default

    resources_config->min_threads = 4;
    resources_config->max_threads = 32;
    resources_config->thread_stack_size = 65536;         // 64KB default

    resources_config->max_open_files = 1024;
    resources_config->max_file_size_mb = 1024;          // 1GB default
    resources_config->max_log_size_mb = 100;            // 100MB default

    resources_config->enforce_limits = true;
    resources_config->log_usage = true;
    resources_config->check_interval_ms = 5000;         // 5 seconds default

    // Process main resources section
    success = PROCESS_SECTION(root, "Resources");

    // Process memory section
    if (success) {
        success = PROCESS_SECTION(root, "Resources.Memory");
        success = success && PROCESS_SIZE(root, resources_config, max_memory_mb,
                                        "Resources.Memory.MaxMemoryMB", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, max_buffer_size,
                                        "Resources.Memory.MaxBufferSize", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, min_buffer_size,
                                        "Resources.Memory.MinBufferSize", "Resources");
    }

    // Process queue settings
    if (success) {
        success = PROCESS_SECTION(root, "Resources.Queues");
        success = success && PROCESS_SIZE(root, resources_config, max_queue_size,
                                        "Resources.Queues.MaxQueueSize", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, max_queue_memory_mb,
                                        "Resources.Queues.MaxQueueMemoryMB", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, max_queue_blocks,
                                        "Resources.Queues.MaxQueueBlocks", "Resources");
        success = success && PROCESS_INT(root, resources_config, queue_timeout_ms,
                                       "Resources.Queues.QueueTimeoutMS", "Resources");
    }

    // Process thread limits
    if (success) {
        success = PROCESS_SECTION(root, "Resources.Threads");
        success = success && PROCESS_INT(root, resources_config, min_threads,
                                       "Resources.Threads.MinThreads", "Resources");
        success = success && PROCESS_INT(root, resources_config, max_threads,
                                       "Resources.Threads.MaxThreads", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, thread_stack_size,
                                        "Resources.Threads.ThreadStackSize", "Resources");
    }

    // Process file limits
    if (success) {
        success = PROCESS_SECTION(root, "Resources.Files");
        success = success && PROCESS_INT(root, resources_config, max_open_files,
                                       "Resources.Files.MaxOpenFiles", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, max_file_size_mb,
                                        "Resources.Files.MaxFileSizeMB", "Resources");
        success = success && PROCESS_SIZE(root, resources_config, max_log_size_mb,
                                        "Resources.Files.MaxLogSizeMB", "Resources");
    }

    // Process monitoring settings
    if (success) {
        success = PROCESS_SECTION(root, "Resources.Monitoring");
        success = success && PROCESS_BOOL(root, resources_config, enforce_limits,
                                        "Resources.Monitoring.EnforceLimits", "Resources");
        success = success && PROCESS_BOOL(root, resources_config, log_usage,
                                        "Resources.Monitoring.LogUsage", "Resources");
        success = success && PROCESS_INT(root, resources_config, check_interval_ms,
                                       "Resources.Monitoring.CheckIntervalMS", "Resources");
    }

    return success;
}

void cleanup_resources_config(ResourceConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ResourceConfig));
}

void dump_resources_config(const ResourceConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL resources config");
        return;
    }

    // Memory limits
    DUMP_TEXT("――", "Memory Limits");
    DUMP_SIZE("――――Max Memory (MB)", config->max_memory_mb);
    DUMP_SIZE("――――Max Buffer Size", config->max_buffer_size);
    DUMP_SIZE("――――Min Buffer Size", config->min_buffer_size);

    // Queue settings
    DUMP_TEXT("――", "Queue Settings");
    DUMP_SIZE("――――Max Queue Size", config->max_queue_size);
    DUMP_SIZE("――――Max Queue Memory (MB)", config->max_queue_memory_mb);
    DUMP_SIZE("――――Max Queue Blocks", config->max_queue_blocks);
    DUMP_INT("――――Queue Timeout (ms)", config->queue_timeout_ms);

    // Buffer sizes
    DUMP_TEXT("――", "Buffer Settings");
    DUMP_SIZE("――――Post Processor Buffer", config->post_processor_buffer_size);

    // Thread limits
    DUMP_TEXT("――", "Thread Limits");
    DUMP_INT("――――Min Threads", config->min_threads);
    DUMP_INT("――――Max Threads", config->max_threads);
    DUMP_SIZE("――――Thread Stack Size", config->thread_stack_size);

    // File limits
    DUMP_TEXT("――", "File Limits");
    DUMP_INT("――――Max Open Files", config->max_open_files);
    DUMP_SIZE("――――Max File Size (MB)", config->max_file_size_mb);
    DUMP_SIZE("――――Max Log Size (MB)", config->max_log_size_mb);

    // Monitoring settings
    DUMP_TEXT("――", "Monitoring Settings");
    DUMP_BOOL("――――Enforce Limits", config->enforce_limits);
    DUMP_BOOL("――――Log Usage", config->log_usage);
    DUMP_INT("――――Check Interval (ms)", config->check_interval_ms);
}
