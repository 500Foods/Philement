/*
 * Queue Configuration Constants
 *
 * Shared constants for queue configuration and utilities.
 * These constants are used by both the configuration system
 * and the queue utilities.
 */

#ifndef CONFIG_QUEUE_CONSTANTS_H
#define CONFIG_QUEUE_CONSTANTS_H

// Queue size limits
#define DEFAULT_MAX_QUEUE_SIZE 10000            // 10K items
#define DEFAULT_MAX_QUEUE_MEMORY_MB 256         // 256MB default
#define DEFAULT_MAX_QUEUE_BLOCKS 1024           // 1K blocks
#define DEFAULT_QUEUE_TIMEOUT_MS 30000          // 30 seconds

// Queue validation limits
#define MIN_QUEUE_SIZE 10
#define MAX_QUEUE_SIZE 1000000
#define MIN_QUEUE_MEMORY_MB 64
#define MAX_QUEUE_MEMORY_MB 16384
#define MIN_QUEUE_BLOCKS 64
#define MAX_QUEUE_BLOCKS 16384
#define MIN_QUEUE_TIMEOUT_MS 1000
#define MAX_QUEUE_TIMEOUT_MS 300000

#endif /* CONFIG_QUEUE_CONSTANTS_H */