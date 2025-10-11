/*
 * System Configuration API Endpoint Implementation
 * 
 * Returns the complete server configuration file as JSON.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include <sys/utsname.h>
#include "config.h"
#include "../../../api/api_utils.h"

enum MHD_Result handle_system_config_request(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    (void)upload_data;      // Unused
    (void)upload_data_size; // Unused
    
    log_this(SR_API, "Handling config endpoint request", LOG_LEVEL_STATE, 0);
    
    // Start timing
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Initialize connection context if needed
    if (*con_cls == NULL) {
        static int connection_initialized = 1;
        *con_cls = &connection_initialized;
        return MHD_YES;
    }
    
    // Only allow GET method
    if (strcmp(method, "GET") != 0) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Only GET method is allowed"));
        
        log_this(SR_API, "Method not allowed: %s", LOG_LEVEL_DEBUG, 1, method);
        *con_cls = NULL; // Reset connection context
        return api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Get the application configuration
    if (!app_config || !app_config->server.config_file) {
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Configuration not available"));
        
        log_this(SR_API, "Application configuration not available", LOG_LEVEL_ERROR, 0);
        *con_cls = NULL; // Reset connection context
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Log that we're loading the config file
    log_this(SR_API, "Loading configuration from file: %s", LOG_LEVEL_DEBUG, 1, app_config->server.config_file);
    
    // Load the raw JSON configuration file
    json_error_t error;
    json_t *config_json = json_load_file(app_config->server.config_file, 0, &error);
    
    if (!config_json) {
        json_t *error_obj = json_object();
        json_object_set_new(error_obj, "error", json_string("Failed to load configuration file"));
        json_object_set_new(error_obj, "details", json_string(error.text));
        
        log_this(SR_API, "Failed to load config file: %s (line %d, column %d)", LOG_LEVEL_ERROR, 3, error.text, error.line, error.column);
        
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
    double processing_time = ((double)(end_time.tv_sec - start_time.tv_sec)) * 1000.0 + 
                           ((double)(end_time.tv_nsec - start_time.tv_nsec)) / 1000000.0;
    
    json_t *timing = json_object();
    json_object_set_new(timing, "processing_time_ms", json_real(processing_time));
    json_object_set_new(timing, "timestamp", json_integer(time(NULL)));
    json_object_set_new(response_obj, "timing", timing);
    
    // Decref the config_json since we've added it to the response with json_object_set
    // This ensures proper reference counting without double-free issues
    json_decref(config_json);
    
    // Reset connection context before returning
    *con_cls = NULL;
    
    log_this(SR_API, "Completed building configuration response in %.3f ms", LOG_LEVEL_DEBUG, 1, processing_time);
    
    // Send the response - this will free the response_obj
    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}
