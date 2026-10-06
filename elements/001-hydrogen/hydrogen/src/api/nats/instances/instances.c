/*
 * GET /api/nats/instances.
 *
 * The peer list is the same snapshot H.nats.instances returns.
 * Presence disabled yields self and an empty peers array.
 * Ids are copied from nats_registry_copy and are not logged.
 */

#include <src/hydrogen.h>

#include <string.h>

#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif

#include "instances.h"

json_t *nats_instances_build_json(void) {
    bool presence = app_config && app_config->nats.Presence.Enabled;
    NatsRegistryCopy copied[NATS_REGISTRY_COPY_CAP];
    int count = 0;
    json_t *response;
    json_t *peers;
    const char *self = nats_registry_self_state();

    memset(copied, 0, sizeof(copied));
    if (presence) {
        count = nats_registry_copy(copied, NATS_REGISTRY_COPY_CAP);
    }
    response = json_object();
    peers = json_array();
    if (!response || !peers) {
        json_decref(response);
        json_decref(peers);
        return NULL;
    }
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "singleton",
                        json_boolean(nats_registry_is_singleton()));
    json_object_set_new(response, "self", json_string(self ? self : "none"));
    {
        int i;

        for (i = 0; i < count; i++) {
            json_t *peer = json_object();

            if (!peer) {
                json_decref(response);
                json_decref(peers);
                return NULL;
            }
            json_object_set_new(peer, "id", json_string(copied[i].id));
            json_object_set_new(peer, "state", json_string(copied[i].state));
            json_object_set_new(peer, "websocket_connections",
                                json_integer(copied[i].websocket_connections));
            json_array_append_new(peers, peer);
        }
    }
    json_object_set_new(response, "peers", peers);
    return response;
}

enum MHD_Result handle_nats_instances_request(
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

        response = nats_instances_build_json();
        if (!response) {
            return api_send_error_and_cleanup(connection, NULL,
                "Failed to build instances response", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        log_this(SR_NATS, "Instances request", LOG_LEVEL_DEBUG, 0);
        return api_send_json_response(connection, response, MHD_HTTP_OK);
    }
}
