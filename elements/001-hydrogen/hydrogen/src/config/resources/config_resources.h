/*
 * Resources Configuration
 *
 * Defines the configuration structure and defaults for system resources.
 * This includes settings for memory limits, buffer sizes, queue capacities,
 * and other system boundaries.
 */

#ifndef HYDROGEN_CONFIG_RESOURCES_H
#define HYDROGEN_CONFIG_RESOURCES_H

#include <stddef.h>
#include <stdbool.h>

// Default memory limits (in bytes)
#define DEFAULT_MAX_MEMORY_MB 1024       // 1GB maximum memory usage
#define DEFAULT_MAX_BUFFER_SIZE 65536    // 64KB maximum buffer size
#define DEFAULT_MIN_BUFFER_SIZE 1024     // 1KB minimum buffer size

// Default buffer sizes
#define DEFAULT_LINE_BUFFER_SIZE 4096    // 4KB line buffer size
#define DEFAULT_LOG_BUFFER_SIZE 8192     // 8KB log buffer size
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE 32768  // 32KB post processor buffer

// Default queue settings
#define DEFAULT_MAX_QUEUE_SIZE 1000      // Maximum items in a queue
#define DEFAULT_MAX_QUEUE_MEMORY_MB 256  // 256MB maximum queue memory
#define DEFAULT_QUEUE_TIMEOUT_MS 30000   // 30 seconds queue timeout
#define DEFAULT_MAX_QUEUE_BLOCKS 1024    // Maximum memory blocks per queue

// Default thread limits
#define DEFAULT_MIN_THREADS 2            // Minimum threads per subsystem
#define DEFAULT_MAX_THREADS 32           // Maximum threads per subsystem
#define DEFAULT_THREAD_STACK_SIZE 65536  // 64KB thread stack size

// Default file limits
#define DEFAULT_MAX_OPEN_FILES 1024      // Maximum open file descriptors
#define DEFAULT_MAX_FILE_SIZE_MB 100     // 100MB maximum file size
#define DEFAULT_MAX_LOG_SIZE_MB 500      // 500MB maximum log file size

// Print job limits
#define DEFAULT_MAX_LAYERS 1000          // Maximum number of layers in a print job

typedef struct ResourceConfig {
    // Memory limits
    size_t max_memory_mb;          // Maximum total memory usage
    size_t max_buffer_size;        // Maximum single buffer size
    size_t min_buffer_size;        // Minimum buffer size

    // Queue settings
    size_t max_queue_size;         // Maximum items per queue
    size_t max_queue_memory_mb;    // Maximum memory for all queues
    size_t max_queue_blocks;       // Maximum memory blocks per queue
    int queue_timeout_ms;          // Queue operation timeout

    // Thread limits
    int min_threads;               // Minimum threads per subsystem
    int max_threads;               // Maximum threads per subsystem
    size_t thread_stack_size;      // Thread stack size

    // File limits
    int max_open_files;           // Maximum open file descriptors
    size_t max_file_size_mb;      // Maximum single file size
    size_t max_log_size_mb;       // Maximum log file size
    size_t post_processor_buffer_size;  // Size of post processor buffer

    // Resource monitoring
    bool enforce_limits;           // Whether to enforce resource limits
    bool log_usage;               // Whether to log resource usage
    int check_interval_ms;        // Resource check interval
} ResourceConfig;

int config_resources_init(ResourceConfig* config);
void config_resources_cleanup(ResourceConfig* config);
int config_resources_validate(const ResourceConfig* config);

#endif /* HYDROGEN_CONFIG_RESOURCES_H */