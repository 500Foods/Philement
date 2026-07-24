/*
 * OIDC UserInfo Endpoint Implementation
 *
 * Implements the OpenID Connect UserInfo endpoint (/oauth/userinfo)
 */

#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>

#include "userinfo.h"

#include <string.h>

enum MHD_Result oidc_userinfo_send_unauthorized(struct MHD_Connection *connection,
                                                const char *description);

enum MHD_Result oidc_userinfo_send_unauthorized(struct MHD_Connection *connection,
                                                const char *description) {
    const char *desc = description ? description : "invalid_token";
    char *body = NULL;
    if (asprintf(&body,
                 "{\"error\":\"invalid_token\",\"error_description\":\"%s\"}",
                 desc) < 0) {
        body = NULL;
    }

    struct MHD_Response *response = NULL;
    if (body) {
        response = MHD_create_response_from_buffer(strlen(body), body, MHD_RESPMEM_MUST_FREE);
    } else {
        response = MHD_create_response_from_buffer(
            strlen("{\"error\":\"invalid_token\"}"),
            (void*)"{\"error\":\"invalid_token\"}",
            MHD_RESPMEM_PERSISTENT);
    }
    if (!response) {
        free(body);
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "WWW-Authenticate",
                            "Bearer error=\"invalid_token\", error_description=\"The access token is invalid\"");
    add_oidc_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
    MHD_destroy_response(response);
    return ret;
}

enum MHD_Result handle_oidc_userinfo_endpoint(struct MHD_Connection *connection,
                                       const char *method) {
    log_this(SR_OIDC, "Handling userinfo endpoint request", LOG_LEVEL_DEBUG, 0);

    if (method && strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0) {
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header || strncasecmp(auth_header, "Bearer ", 7) != 0) {
        return oidc_userinfo_send_unauthorized(connection, "Missing or invalid Authorization header");
    }

    const char *access_token = auth_header + 7;
    while (*access_token == ' ') {
        access_token++;
    }
    if (*access_token == '\0') {
        return oidc_userinfo_send_unauthorized(connection, "Empty access token");
    }

    char *userinfo = oidc_process_userinfo_request(access_token);
    if (!userinfo) {
        return oidc_userinfo_send_unauthorized(connection, "Invalid access token");
    }

    enum MHD_Result ret = send_oidc_json_response(connection, userinfo, MHD_HTTP_OK);
    free(userinfo);
    return ret;
}
