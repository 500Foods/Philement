/*
 * Print Queue Buffers Configuration
 *
 * Defines the configuration structure and defaults for print queue
 * buffer sizes. This includes settings for various message types
 * and queue operations.
 */

#ifndef HYDROGEN_CONFIG_PRINT_BUFFERS_H
#define HYDROGEN_CONFIG_PRINT_BUFFERS_H

#include <stddef.h>

// Project headers
#include "../config_forward.h"

// Default buffer sizes (in bytes)
#define DEFAULT_JOB_MESSAGE_SIZE (32 * 1024)     // 32KB
#define DEFAULT_STATUS_MESSAGE_SIZE (8 * 1024)    // 8KB
#define DEFAULT_QUEUE_MESSAGE_SIZE (16 * 1024)    // 16KB
#define DEFAULT_COMMAND_BUFFER_SIZE (4 * 1024)    // 4KB
#define DEFAULT_RESPONSE_BUFFER_SIZE (16 * 1024)  // 16KB

// Validation limits
#define MIN_MESSAGE_SIZE (1 * 1024)      // 1KB minimum
#define MAX_MESSAGE_SIZE (1024 * 1024)   // 1MB maximum
#define MIN_BUFFER_SIZE (512)            // 512B minimum
#define MAX_BUFFER_SIZE (512 * 1024)     // 512KB maximum

// Print queue buffers configuration structure
struct PrintQueueBuffersConfig {
    // Message sizes
    size_t job_message_size;      // Size for job-related messages
    size_t status_message_size;   // Size for status updates
    size_t queue_message_size;    // Size for queue operations
    
    // Operation buffers
    size_t command_buffer_size;   // Size for command processing
    size_t response_buffer_size;  // Size for response data
};

/*
 * Initialize print queue buffers configuration with default values
 *
 * This function initializes a new PrintQueueBuffersConfig structure
 * with default values that provide reasonable buffer sizes for
 * typical operations.
 *
 * @param config Pointer to PrintQueueBuffersConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_print_buffers_init(PrintQueueBuffersConfig* config);

/*
 * Free resources allocated for print queue buffers configuration
 *
 * This function cleans up any resources allocated by config_print_buffers_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to PrintQueueBuffersConfig structure to cleanup
 */
void config_print_buffers_cleanup(PrintQueueBuffersConfig* config);

/*
 * Validate print queue buffers configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all buffer sizes are within acceptable ranges
 * - Ensures proper relationships between buffer sizes
 * - Validates memory usage constraints
 *
 * @param config Pointer to PrintQueueBuffersConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any buffer size is outside valid range
 * - If buffer size relationships are invalid
 * - If total memory usage exceeds system constraints
 */
int config_print_buffers_validate(const PrintQueueBuffersConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_BUFFERS_H */