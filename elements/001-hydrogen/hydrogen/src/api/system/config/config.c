/*
 * System Configuration API Endpoint Implementation
 * 
 * Returns the complete server configuration file as JSON.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Network headers
#include <microhttpd.h>
#include <sys/utsname.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Project headers
#include "config.h"
#include "../../../api/api_utils.h"
#include "../../../logging/logging.h"
#include "../../../config/config.h"

enum MHD_Result handle_system_config_request(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    (void)upload_data;      // Unused
    (void)upload_data_size; // Unused
    
    log_this("Config", "Handling config endpoint request", LOG_LEVEL_STATE);
    
    // Start timing
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Initialize connection context if needed
    if (*con_cls == NULL) {
        *con_cls = (void *)1;
        return MHD_YES;
    }
    
    // Only allow GET method
    if (strcmp(method, "GET") != 0) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Only GET method is allowed"));
        
        log_this("Config", "Method not allowed: %s", LOG_LEVEL_DEBUG, method);
        *con_cls = NULL; // Reset connection context
        return api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Get the application configuration
    const AppConfig *app_config = get_app_config();
    if (!app_config || !app_config->server.config_file) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Configuration not available"));
        
        log_this("Config", "Application configuration not available", LOG_LEVEL_ERROR);
        *con_cls = NULL; // Reset connection context
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Log that we're loading the config file
    log_this("Config", "Loading configuration from file: %s", LOG_LEVEL_DEBUG, app_config->server.config_file);
    
    // Load the raw JSON configuration file
    json_error_t error;
    json_t *config_json = json_load_file(app_config->server.config_file, 0, &error);
    
    if (!config_json) {
        json_t *error_obj = json_object();
        json_object_set_new(error_obj, "error", json_string("Failed to load configuration file"));
        json_object_set_new(error_obj, "details", json_string(error.text));
        
        log_this("Config", "Failed to load config file: %s (line %d, column %d)",
                 LOG_LEVEL_ERROR, error.text, error.line, error.column);
        
        *con_cls = NULL; // Reset connection context
        return api_send_json_response(connection, error_obj, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Create a wrapper response with metadata
    json_t *response_obj = json_object();
    
    // Add the full configuration file as loaded
    json_object_set(response_obj, "config", config_json);
    
    // Add metadata about the configuration
    json_object_set_new(response_obj, "config_file", json_string(app_config->server.config_file));
    
    // Add timing information
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double processing_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                           (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    
    json_t *timing = json_object();
    json_object_set_new(timing, "processing_time_ms", json_real(processing_time));
    json_object_set_new(timing, "timestamp", json_integer(time(NULL)));
    json_object_set_new(response_obj, "timing", timing);
    
    // Decref the config_json since we've added it to the response with json_object_set
    // This ensures proper reference counting without double-free issues
    json_decref(config_json);
    
    // Reset connection context before returning
    *con_cls = NULL;
    
    log_this("Config", "Completed building configuration response in %.3f ms", LOG_LEVEL_DEBUG, processing_time);
    
    // Send the response - this will free the response_obj
    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}