/*
 * GET /api/nats/status.
 *
 * Link is down, degraded, or up. state is down only when NATS is
 * disabled. Counters come from nats_stats_collect. Peer ids are
 * not logged.
 */

#include <src/hydrogen.h>

#include <string.h>

#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/nats/nats.h>
#include <src/nats/nats_stats.h>

#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif

#include "status.h"

json_t *nats_status_build_json(const NatsMetrics *metrics) {
    json_t *response;

    if (!metrics || !metrics->self) {
        return NULL;
    }
    response = json_object();
    if (!response) {
        return NULL;
    }
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "enabled", json_boolean(metrics->enabled));
    json_object_set_new(response, "link", json_string(nats_link_name(metrics->link)));
    json_object_set_new(response, "state",
                        json_string(nats_status_state(metrics->enabled, metrics->link)));
    json_object_set_new(response, "presence", json_boolean(metrics->presence));
    json_object_set_new(response, "singleton", json_boolean(metrics->singleton));
    json_object_set_new(response, "peers", json_integer(metrics->peers));
    json_object_set_new(response, "alive", json_integer(metrics->alive));
    json_object_set_new(response, "self", json_string(metrics->self));
    json_object_set_new(response, "published",
                        json_integer((json_int_t)metrics->published));
    json_object_set_new(response, "received",
                        json_integer((json_int_t)metrics->received));
    json_object_set_new(response, "reconnects",
                        json_integer((json_int_t)metrics->reconnects));
    return response;
}

enum MHD_Result handle_nats_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {
    (void)url;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    if (!method || strcmp(method, "GET") != 0) {
        json_t *error_response = json_object();

        if (error_response) {
            json_object_set_new(error_response, "success", json_false());
            json_object_set_new(error_response, "error", json_string("Method not allowed"));
            json_object_set_new(error_response, "message",
                                json_string("Only GET requests are supported"));
        }
        return api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    {
        jwt_validation_result_t jwt_result;
        const char *auth_header;
        NatsMetrics snap;
        json_t *response;

        auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        if (!extract_and_validate_jwt(auth_header, &jwt_result)) {
            const char *error_msg = get_jwt_error_message(jwt_result.error);

            if (jwt_result.claims) {
                free_jwt_claims(jwt_result.claims);
            }
            (void)send_jwt_error_response(connection, error_msg, MHD_HTTP_UNAUTHORIZED);
            return MHD_YES;
        }
        if (!validate_jwt_claims(&jwt_result, connection)) {
            if (jwt_result.claims) {
                free_jwt_claims(jwt_result.claims);
            }
            return MHD_YES;
        }
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
        }

        nats_stats_collect(&snap);
        response = nats_status_build_json(&snap);
        if (!response) {
            return api_send_error_and_cleanup(connection, NULL,
                "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        log_this(SR_NATS, "Status request: enabled=%s link=%s", LOG_LEVEL_DEBUG, 2,
                 snap.enabled ? "true" : "false", nats_link_name(snap.link));
        return api_send_json_response(connection, response, MHD_HTTP_OK);
    }
}
