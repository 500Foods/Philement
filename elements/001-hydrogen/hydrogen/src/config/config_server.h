/*
 * Server Configuration
 *
 * Defines the configuration structure and handlers for the server subsystem.
 * This is section A of the configuration system, handling core server settings:
 * - Server identification
 * - Log file location
 * - Payload encryption
 * - Startup behavior
 */

#ifndef HYDROGEN_CONFIG_SERVER_H
#define HYDROGEN_CONFIG_SERVER_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// Server configuration structure (Section A)
struct ServerConfig {
    char* server_name;      // Server identification
    char* log_file;         // Log file path
    char* config_file;      // Configuration file path
    char* payload_key;      // Key for payload encryption
    int startup_delay;      // Delay before starting services (seconds)
};
typedef struct ServerConfig ServerConfig;

/*
 * Load server configuration from JSON
 *
 * This function loads the server configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @param config_path Path to the configuration file being loaded
 * @return true if successful, false on error
 */
bool load_server_config(json_t* root, AppConfig* config, const char* config_path);


/*
 * Free resources allocated for server configuration
 *
 * This function frees all resources allocated for server configuration.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to ServerConfig structure to cleanup
 */
void config_server_cleanup(ServerConfig* config);


#endif /* HYDROGEN_CONFIG_SERVER_H */