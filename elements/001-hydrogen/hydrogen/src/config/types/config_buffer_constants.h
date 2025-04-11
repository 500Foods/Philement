/*
 * Buffer and Layer Configuration Constants
 *
 * Shared constants for buffer sizes and layer limits.
 * These constants are used throughout the system for
 * consistent buffer allocation and layer handling.
 */

#ifndef CONFIG_BUFFER_CONSTANTS_H
#define CONFIG_BUFFER_CONSTANTS_H

// Default buffer sizes
#define DEFAULT_LINE_BUFFER_SIZE 4096           // 4KB for line buffers
#define DEFAULT_LOG_BUFFER_SIZE 8192            // 8KB for log messages
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE 8192 // 8KB for post processing
#define DEFAULT_COMMAND_BUFFER_SIZE 4096        // 4KB for commands (increased for print operations)
#define DEFAULT_RESPONSE_BUFFER_SIZE 16384      // 16KB for responses

// Layer limits
#define DEFAULT_MAX_LAYERS 1000                 // Maximum number of layers
#define MIN_LAYERS 1                            // Minimum number of layers
#define MAX_LAYERS 10000                        // Absolute maximum layers

#endif /* CONFIG_BUFFER_CONSTANTS_H */