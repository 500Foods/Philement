/*
 * System Configuration API Endpoint Implementation
 * 
 * Returns the server's configuration file in JSON format.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Project headers
#include "config.h"
#include "../../../api/api_utils.h"
#include "../../../logging/logging.h"
#include "../../../config/configuration.h"

enum MHD_Result handle_system_config_request(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    (void)upload_data;      // Unused
    (void)upload_data_size; // Unused
    
    // Initialize connection context if needed
    if (*con_cls == NULL) {
        *con_cls = (void *)1;
        return MHD_YES;
    }
    
    // Only allow GET method
    if (strcmp(method, "GET") != 0) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Only GET method is allowed"));
        return api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Get the configuration file path
    const char *config_path = get_app_config()->config_file;
    if (!config_path) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Configuration file path not available"));
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Load and parse the configuration file
    json_error_t json_error;
    json_t *config = json_load_file(config_path, 0, &json_error);
    if (!config) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Failed to load configuration file"));
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Send the configuration as JSON response (compression handled by api_send_json_response)
    enum MHD_Result ret = api_send_json_response(connection, config, MHD_HTTP_OK);
    json_decref(config);
    return ret;
}