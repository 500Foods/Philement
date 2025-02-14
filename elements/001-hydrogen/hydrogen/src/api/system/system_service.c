/*
 * Implementation of System Service API endpoints.
 * Provides system-level information and operations.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "system_service.h"
#include "../../configuration.h"
#include "../../state.h"
#include "../../logging.h"
#include "../../web_server.h"

extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

enum MHD_Result handle_system_info_request(struct MHD_Connection *connection) {
    struct utsname system_info;
    if (uname(&system_info) < 0) {
        log_this("SystemService", "Failed to get system information", 3, true, false, true);
        const char *error_response = "{\"error\": \"Failed to retrieve system information\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void*)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    json_t *root = json_object();

    // Version Information
    json_t *version = json_object();
    json_object_set_new(version, "server", json_string(VERSION));
    json_object_set_new(version, "api", json_string("1.0"));
    json_object_set_new(root, "version", version);

    // System Information
    json_t *system = json_object();
    json_object_set_new(system, "sysname", json_string(system_info.sysname));
    json_object_set_new(system, "nodename", json_string(system_info.nodename));
    json_object_set_new(system, "release", json_string(system_info.release));
    json_object_set_new(system, "version", json_string(system_info.version));
    json_object_set_new(system, "machine", json_string(system_info.machine));
    json_object_set_new(root, "system", system);

    // Configuration Information
    json_t *config = json_object();
    json_object_set_new(config, "server_name", json_string(app_config->server_name));
    
    json_t *web_config = json_object();
    json_object_set_new(web_config, "port", json_integer(app_config->web.port));
    json_object_set_new(web_config, "upload_path", json_string(app_config->web.upload_path));
    json_object_set_new(web_config, "max_upload_size", json_integer(app_config->web.max_upload_size));
    json_object_set_new(config, "web", web_config);

    json_t *websocket_config = json_object();
    json_object_set_new(websocket_config, "port", json_integer(app_config->websocket.port));
    json_object_set_new(websocket_config, "protocol", json_string(app_config->websocket.protocol));
    json_object_set_new(websocket_config, "max_message_size", json_integer(app_config->websocket.max_message_size));
    json_object_set_new(config, "websocket", websocket_config);

    json_t *device = json_object();
    json_object_set_new(device, "id", json_string(app_config->mdns.device_id));
    json_object_set_new(device, "name", json_string(app_config->mdns.friendly_name));
    json_object_set_new(device, "model", json_string(app_config->mdns.model));
    json_object_set_new(device, "manufacturer", json_string(app_config->mdns.manufacturer));
    json_object_set_new(config, "device", device);
    
    json_object_set_new(root, "configuration", config);

    // Status Information
    json_t *status = json_object();
    json_object_set_new(status, "running", json_boolean(keep_running));
    json_object_set_new(status, "shutting_down", json_boolean(shutting_down));
    
    json_t *services = json_object();
    json_object_set_new(services, "web_server", json_boolean(!web_server_shutdown));
    json_object_set_new(services, "print_queue", json_boolean(!print_queue_shutdown));
    json_object_set_new(services, "log_queue", json_boolean(!log_queue_shutdown));
    json_object_set_new(services, "mdns_server", json_boolean(!mdns_server_shutdown));
    json_object_set_new(services, "websocket_server", json_boolean(!websocket_server_shutdown));
    json_object_set_new(status, "services", services);
    
    json_object_set_new(root, "status", status);

    // Convert to string and create response
    char *response_str = json_dumps(root, JSON_INDENT(2));
    json_decref(root);

    if (!response_str) {
        log_this("SystemService", "Failed to create JSON response", 3, true, false, true);
        const char *error_response = "{\"error\": \"Failed to create response\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void*)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    
    // Add CORS headers using the helper function from web_server
    extern void add_cors_headers(struct MHD_Response *response);
    add_cors_headers(response);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return ret;
}