/*
 * Server Configuration Implementation
 *
 * Implements the configuration handlers for the server subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_server.h"

// Load server configuration from JSON
// Note: Validation moved to launch readiness checks
bool load_server_config(json_t* root, AppConfig* config, const char* config_path) {
    ServerConfig* server = &config->server;

    // Initialize with defaults
    *server = (ServerConfig){
        .server_name = strdup("Philement/hydrogen"),
        .exec_file = NULL,    
        .config_file = NULL,  
        .log_file = strdup("/var/log/hydrogen/hydrogen.log"),
        .payload_key = strdup("${env.PAYLOAD_KEY}"),
        .startup_delay = 5
    };

    // Process all config items in sequence
    bool success = true;

    // Get executable path
    char exec_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path)-1);
    if (len != -1) {
        exec_path[len] = '\0';
    } else {
        // Fallback to a reasonable default if readlink fails
        strncpy(exec_path, "hydrogen", sizeof(exec_path)-1);
        exec_path[sizeof(exec_path)-1] = '\0';
    }

    // One line per key/value using macros
    success = PROCESS_SECTION(root, "Server");
    success = success && PROCESS_STRING(root, server, server_name, "Server.ServerName", "Server");
    success = success && PROCESS_STRING_DIRECT(server, exec_file, "Server.ExecFile", "Server", exec_path);
    success = success && PROCESS_STRING_DIRECT(server, config_file, "Server.ConfigFile", "Server", config_path);
    success = success && PROCESS_STRING(root, server, log_file, "Server.LogFile", "Server");
    success = success && PROCESS_SENSITIVE(root, server, payload_key, "Server.PayloadKey", "Server");
    success = success && PROCESS_INT(root, server, startup_delay, "Server.StartupDelay", "Server");

    return success;
}

// Free resources allocated for server configuration
void cleanup_server_config(ServerConfig* config) {
    if (!config) {
        return;
    }
    if (config->exec_file) {
        free(config->exec_file);
        config->exec_file = NULL;
    }
    // Free allocated strings
    if (config->server_name) {
        free(config->server_name);
        config->server_name = NULL;
    }

    if (config->log_file) {
        free(config->log_file);
        config->log_file = NULL;
    }

    if (config->config_file) {
        free(config->config_file);
        config->config_file = NULL;
    }

    if (config->payload_key) {
        free(config->payload_key);
        config->payload_key = NULL;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ServerConfig));
}

void dump_server_config(const ServerConfig* config) {
    if (!config) {
        log_this(SR_CONFIG, "Cannot dump NULL server config", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Use DUMP_ macros to show actual AppConfig contents
    DUMP_STRING("―― server_name", config->server_name);
    DUMP_STRING("―― exec_file", config->exec_file);
    DUMP_STRING("―― config_file", config->config_file);
    DUMP_STRING("―― log_file", config->log_file);
    DUMP_SECRET("―― payload_key", config->payload_key);
    DUMP_INT("―― startup_delay", config->startup_delay);
}
