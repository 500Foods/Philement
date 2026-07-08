/*
 * Mail Relay Status API Endpoint Implementation
 *
 * Implements GET /api/mailrelay/status returning subsystem counters and health.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/mailrelay/mailrelay.h>

// Local includes
#include "status.h"

// JWT helper for bearer token validation
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Forward declaration for internal helper
static enum MHD_Result mailrelay_status_send_response(
    struct MHD_Connection *connection,
    const MailRelayStatusCounters *counters);

/*
 * Send the JSON status response.
 */
static enum MHD_Result mailrelay_status_send_response(
    struct MHD_Connection *connection,
    const MailRelayStatusCounters *counters) {
    json_t *response = json_object();
    if (!response) {
        return api_send_error_and_cleanup(connection, NULL,
            "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "enabled", json_boolean(counters->enabled));
    json_object_set_new(response, "initialized", json_boolean(counters->initialized));
    json_object_set_new(response, "queued", json_integer((json_int_t)counters->queued));
    json_object_set_new(response, "sending", json_integer((json_int_t)counters->sending));
    json_object_set_new(response, "sent", json_integer((json_int_t)counters->sent));
    json_object_set_new(response, "failed", json_integer((json_int_t)counters->failed));
    json_object_set_new(response, "retrying", json_integer((json_int_t)counters->retrying));
    json_object_set_new(response, "permanent_failures", json_integer((json_int_t)counters->permanent_failures));
    json_object_set_new(response, "last_success", json_integer((json_int_t)counters->last_success));
    json_object_set_new(response, "last_failure", json_integer((json_int_t)counters->last_failure));
    json_object_set_new(response, "worker_count", json_integer(counters->worker_count));
    json_object_set_new(response, "queue_depth", json_integer(counters->queue_depth));

    enum MHD_Result result = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);
    return result;
}

/*
 * Handle GET /api/mailrelay/status requests.
 */
enum MHD_Result handle_mailrelay_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {
    (void)url;              // Unused parameter
    (void)upload_data;      // Unused parameter
    (void)upload_data_size; // Unused parameter
    (void)con_cls;          // Status endpoint has no POST body state

    // Only GET is supported
    if (!method || strcmp(method, "GET") != 0) {
        json_t *error_response = json_object();
        if (error_response) {
            json_object_set_new(error_response, "success", json_false());
            json_object_set_new(error_response, "error", json_string("Method not allowed"));
            json_object_set_new(error_response, "message", json_string("Only GET requests are supported"));
        }
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
        if (error_response) {
            json_decref(error_response);
        }
        return ret;
    }

    // Validate JWT Bearer token
    const char *auth_header = MHD_lookup_connection_value(connection,
                                                          MHD_HEADER_KIND,
                                                          "Authorization");
    jwt_validation_result_t jwt_result;
    if (!extract_and_validate_jwt(auth_header, &jwt_result)) {
        const char *error_msg = get_jwt_error_message(jwt_result.error);
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
        }
        return send_jwt_error_response(connection, error_msg, MHD_HTTP_UNAUTHORIZED);
    }

    if (!validate_jwt_claims(&jwt_result, connection)) {
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
        }
        return MHD_YES; // Error response already sent
    }

    if (jwt_result.claims) {
        free_jwt_claims(jwt_result.claims);
    }

    MailRelayStatusCounters counters;
    mailrelay_get_status(&counters);

    log_this(SR_MAIL_RELAY, "Status request: queue_depth=%d workers=%d",
             LOG_LEVEL_DEBUG, 2, counters.queue_depth, counters.worker_count);

    return mailrelay_status_send_response(connection, &counters);
}
