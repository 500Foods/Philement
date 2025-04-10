/*
 * Server Configuration
 *
 * Defines the configuration structure and defaults for server settings.
 * This includes both server identification and system-level settings:
 * - Server identification and paths
 * - Process and resource management
 * - Runtime behavior
 *
 * This is section A of the configuration system.
 */

#ifndef HYDROGEN_CONFIG_SERVER_H
#define HYDROGEN_CONFIG_SERVER_H

// Core system headers
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

// Project headers
#include "../config_forward.h"

// Server identification defaults
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_CONFIG_FILE "/etc/hydrogen/hydrogen.json"
#define DEFAULT_STARTUP_DELAY 5

// System resource defaults
#define DEFAULT_SYSTEM_PROCESS_LIMIT 64
#define DEFAULT_SYSTEM_THREAD_LIMIT 32
#define DEFAULT_SYSTEM_FD_LIMIT 1024
#define DEFAULT_SYSTEM_MEMORY_LIMIT (1024 * 1024 * 1024)  // 1GB
#define DEFAULT_SYSTEM_TEMP_DIR "/tmp/hydrogen"
#define DEFAULT_SYSTEM_DATA_DIR "/var/lib/hydrogen"
#define DEFAULT_SYSTEM_CACHE_DIR "/var/cache/hydrogen"

// Server configuration structure (Section A)
struct ServerConfig {
    // Server Identification
    char* server_name;      // Server identification
    char* payload_key;      // Key for payload encryption
    char* config_file;      // Main configuration file path
    char* exec_file;        // Executable file path
    char* log_file;         // Log file path
    int startup_delay;      // Delay before starting services (seconds)

    // Process Management
    size_t process_limit;   // Maximum number of processes
    size_t thread_limit;    // Maximum number of threads
    size_t fd_limit;        // Maximum number of file descriptors
    size_t memory_limit;    // Maximum memory usage

    // System Paths
    char* temp_dir;         // Temporary directory path
    char* data_dir;         // Data directory path
    char* cache_dir;        // Cache directory path

    // Runtime Behavior
    bool enable_core_dumps;
    bool enable_debug_mode;
    bool enable_performance_mode;
};

/*
 * Initialize server configuration with default values
 *
 * This function initializes a new ServerConfig structure with default values.
 * It allocates memory for all string fields and sets conservative defaults
 * for resource limits and runtime behavior.
 *
 * @param config Pointer to ServerConfig structure to initialize
 * @return 0 on success, -1 on failure (memory allocation or null pointer)
 */
int config_server_init(ServerConfig* config);

/*
 * Free resources allocated for server configuration
 *
 * This function frees all resources allocated by config_server_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to ServerConfig structure to cleanup
 */
void config_server_cleanup(ServerConfig* config);

/*
 * Validate server configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all required string fields are present and non-empty
 * - Validates resource limits are within acceptable ranges
 * - Checks for invalid combinations of settings
 *
 * @param config Pointer to ServerConfig structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_server_validate(const ServerConfig* config);

#endif /* HYDROGEN_CONFIG_SERVER_H */