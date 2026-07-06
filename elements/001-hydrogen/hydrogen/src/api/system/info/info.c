/*
 * System Info API Endpoint Implementation
 * 
 * Implements the /api/system/info endpoint that provides system information
 * for monitoring and diagnostics.
 * 
 * When a valid JWT is provided in the Authorization header, the response
 * includes scripting subsystem metrics (worker counts, job counts, job list).
 * Without authentication, the endpoint returns basic system info without
 * scripting details.
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/system/system_service.h>
#include <src/websocket/websocket_server_internal.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/scripting/scoreboard_json.h>

#include "info.h"

extern WebSocketServerContext *ws_context;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

void extract_websocket_metrics(WebSocketMetrics *metrics) {
    if (!metrics) return;

    *metrics = (WebSocketMetrics){0};

    if (ws_context) {
        pthread_mutex_lock(&ws_context->mutex);
        metrics->server_start_time = ws_context->start_time;
        metrics->active_connections = ws_context->active_connections;
        metrics->total_connections = ws_context->total_connections;
        metrics->total_requests = ws_context->total_requests;
        pthread_mutex_unlock(&ws_context->mutex);
    }
}

static bool has_valid_jwt(struct MHD_Connection *connection) {
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        return false;
    }
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        return false;
    }
    jwt_validation_result_t result = {0};
    bool valid = extract_and_validate_jwt(auth_header, &result);
    if (valid && result.claims) {
        free_jwt_claims(result.claims);
    }
    return valid;
}

enum MHD_Result handle_system_info_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling info endpoint request", LOG_LEVEL_DEBUG, 0);

    json_t *root = NULL;
    WebSocketMetrics metrics = {0};

    extract_websocket_metrics(&metrics);

#ifdef UNITY_TEST_MODE
    root = json_object();
    if (root) {
        json_object_set_new(root, "status", json_string("test_mode"));
        json_object_set_new(root, "test_timestamp", json_integer(1234567890));
    }
#else
    root = get_system_status_json(ws_context ? &metrics : NULL);
#endif
    if (!root) {
        log_this(SR_API, "Failed to generate system status", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    if (has_valid_jwt(connection)) {
        json_t *scripting_json = scripting_scoreboard_snapshot_json(100, false);
        if (scripting_json) {
            json_object_set_new(root, "scripting", scripting_json);
        }
    }

    return api_send_json_response(connection, root, MHD_HTTP_OK);
}