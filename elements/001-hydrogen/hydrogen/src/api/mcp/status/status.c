/*
 * MCP Status API Endpoint Implementation
 *
 * Implements GET /api/mcp/status returning listen config, accept flags, and counters.
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/mcp/mcp.h>

#include "status.h"

#include <src/api/conduit/helpers/auth_jwt_helper.h>

json_t *mcp_status_json_string_or_null(const char *value) {
    if (!value) {
        return json_null();
    }
    return json_string(value);
}

json_t *mcp_status_build_counters(const McpMetrics *metrics) {
    json_t *counters;
    json_t *reasons;

    counters = json_object();
    if (!counters) {
        return NULL;
    }

    json_object_set_new(counters, "sessions_active",
                        json_integer((json_int_t)metrics->sessions_active));
    json_object_set_new(counters, "sessions_total",
                        json_integer((json_int_t)metrics->sessions_total));
    json_object_set_new(counters, "sessions_expired",
                        json_integer((json_int_t)metrics->sessions_expired));
    json_object_set_new(counters, "rpc_received",
                        json_integer((json_int_t)metrics->rpc_received));
    json_object_set_new(counters, "rpc_succeeded",
                        json_integer((json_int_t)metrics->rpc_succeeded));
    json_object_set_new(counters, "rpc_failed",
                        json_integer((json_int_t)metrics->rpc_failed));
    json_object_set_new(counters, "rpc_in_flight",
                        json_integer((json_int_t)metrics->rpc_in_flight));
    json_object_set_new(counters, "auth_rejected",
                        json_integer((json_int_t)metrics->auth_rejected));

    reasons = json_object();
    if (!reasons) {
        json_decref(counters);
        return NULL;
    }
    json_object_set_new(reasons, "missing",
                        json_integer((json_int_t)metrics->auth_rejected_missing));
    json_object_set_new(reasons, "malformed",
                        json_integer((json_int_t)metrics->auth_rejected_malformed));
    json_object_set_new(reasons, "hydrogen_jwt",
                        json_integer((json_int_t)metrics->auth_rejected_hydrogen_jwt));
    json_object_set_new(reasons, "oidc_idp",
                        json_integer((json_int_t)metrics->auth_rejected_oidc_idp));
    json_object_set_new(reasons, "oidc_rp",
                        json_integer((json_int_t)metrics->auth_rejected_oidc_rp));
    json_object_set_new(reasons, "aud",
                        json_integer((json_int_t)metrics->auth_rejected_aud));
    json_object_set_new(reasons, "scope",
                        json_integer((json_int_t)metrics->auth_rejected_scope));
    json_object_set_new(counters, "auth_rejected_reasons", reasons);

    json_object_set_new(counters, "origin_rejected",
                        json_integer((json_int_t)metrics->origin_rejected));
    json_object_set_new(counters, "dispatch_timeouts",
                        json_integer((json_int_t)metrics->dispatch_timeouts));
    json_object_set_new(counters, "bytes_in",
                        json_integer((json_int_t)metrics->bytes_in));
    json_object_set_new(counters, "bytes_out",
                        json_integer((json_int_t)metrics->bytes_out));
    json_object_set_new(counters, "last_rpc_at",
                        json_integer((json_int_t)metrics->last_rpc_at));
    return counters;
}

enum MHD_Result mcp_status_send_response(
    struct MHD_Connection *connection,
    const McpStatusSnapshot *snap) {
    json_t *response;
    json_t *listen;
    json_t *counters;

    response = json_object();
    if (!response) {
        return api_send_error_and_cleanup(connection, NULL,
            "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "enabled", json_boolean(snap->enabled));
    json_object_set_new(response, "initialized", json_boolean(snap->initialized));

    listen = json_object();
    if (!listen) {
        json_decref(response);
        return api_send_error_and_cleanup(connection, NULL,
            "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    json_object_set_new(listen, "interface",
                        mcp_status_json_string_or_null(snap->listen_interface));
    json_object_set_new(listen, "port", json_integer(snap->listen_port));
    json_object_set_new(listen, "path",
                        mcp_status_json_string_or_null(snap->listen_path));
    json_object_set_new(response, "listen", listen);

    json_object_set_new(response, "protocol",
                        mcp_status_json_string_or_null(snap->protocol));
    json_object_set_new(response, "accept_hydrogen_jwt",
                        json_boolean(snap->accept_hydrogen_jwt));
    json_object_set_new(response, "accept_oidc_idp",
                        json_boolean(snap->accept_oidc_idp));
    json_object_set_new(response, "accept_oidc_rp",
                        json_boolean(snap->accept_oidc_rp));
    json_object_set_new(response, "resource",
                        mcp_status_json_string_or_null(snap->resource));
    json_object_set_new(response, "thread_count", json_integer(snap->thread_count));
    json_object_set_new(response, "thread_pool_size",
                        json_integer(snap->thread_pool_size));

    counters = mcp_status_build_counters(&snap->metrics);
    if (!counters) {
        json_decref(response);
        return api_send_error_and_cleanup(connection, NULL,
            "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    json_object_set_new(response, "counters", counters);

    return api_send_json_response(connection, response, MHD_HTTP_OK);
}

enum MHD_Result handle_mcp_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {
    jwt_validation_result_t jwt_result;
    const char *auth_header;
    McpStatusSnapshot snap;

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

    if (!mcp_get_status(&snap)) {
        return api_send_error_and_cleanup(connection, NULL,
            "Failed to build status response", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_MCP, "Status request: enabled=%s rpc_in_flight=%llu sessions_active=%llu",
             LOG_LEVEL_DEBUG, 3, snap.enabled ? "true" : "false",
             snap.metrics.rpc_in_flight, snap.metrics.sessions_active);

    return mcp_status_send_response(connection, &snap);
}
