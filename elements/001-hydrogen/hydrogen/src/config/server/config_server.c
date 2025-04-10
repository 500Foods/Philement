/*
 * Server Configuration Implementation
 *
 * Implements the configuration management for server settings,
 * including initialization, validation, and cleanup of server
 * configuration data.
 */

// Core system headers
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Project headers
#include "config_server.h"
#include "../config_utils.h"

/*
 * Free resources allocated for server configuration
 *
 * This function frees all resources allocated by config_server_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to ServerConfig structure to cleanup
 */
void config_server_cleanup(ServerConfig* config) {
    if (!config) return;

    // Free server identification strings
    if (config->server_name) free(config->server_name);
    if (config->payload_key) free(config->payload_key);
    if (config->config_file) free(config->config_file);
    if (config->exec_file) free(config->exec_file);
    if (config->log_file) free(config->log_file);

    // Free system path strings
    if (config->temp_dir) free(config->temp_dir);
    if (config->data_dir) free(config->data_dir);
    if (config->cache_dir) free(config->cache_dir);

    // Zero out the structure to prevent use-after-free
    memset(config, 0, sizeof(ServerConfig));
}