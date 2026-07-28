/*
 * OIDC Introspection Endpoint Implementation
 *
 * Implements the token introspection endpoint (/oauth/introspect)
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/api/oidc/token/token.h>
#include <src/api/api_utils.h>
#include <src/oidc/oidc_service.h>

// Local includes
#include "introspection.h"

/**
 * Token introspection endpoint
 *
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_introspection_endpoint(struct MHD_Connection *connection,
                                             const char *method,
                                             const char *upload_data,
                                             size_t *upload_data_size,
                                             void **con_cls) {
    log_this(SR_OIDC, "Handling introspection endpoint request", LOG_LEVEL_DEBUG, 0);

    if (!method || strcmp(method, "POST") != 0) {
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size,
                                                      con_cls, &buffer);
    if (buf_result == API_BUFFER_CONTINUE) {
        return MHD_YES;
    }
    if (buf_result == API_BUFFER_ERROR) {
        return MHD_NO;
    }
    if (buf_result == API_BUFFER_METHOD_ERROR) {
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    char *token = NULL;
    char *token_type_hint = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;

    if (buffer && buffer->data && buffer->size > 0U) {
        char *body = (char*)malloc(buffer->size + 1U);
        if (body) {
            memcpy(body, buffer->data, buffer->size);
            body[buffer->size] = '\0';
            token = oidc_token_form_get(body, "token");
            token_type_hint = oidc_token_form_get(body, "token_type_hint");
            client_id = oidc_token_form_get(body, "client_id");
            client_secret = oidc_token_form_get(body, "client_secret");
            free(body);
        }
    }

    if (!client_id || !client_secret) {
        char *basic_id = NULL;
        char *basic_secret = NULL;
        if (extract_client_credentials(connection, &basic_id, &basic_secret)) {
            if (!client_id) {
                client_id = basic_id;
                basic_id = NULL;
            }
            if (!client_secret) {
                client_secret = basic_secret;
                basic_secret = NULL;
            }
        }
        free(basic_id);
        free(basic_secret);
    }

    api_free_post_buffer(con_cls);

    if (!token) {
        free(token);
        free(token_type_hint);
        free(client_id);
        free(client_secret);
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Token parameter required\"}",
                                   MHD_HTTP_BAD_REQUEST);
    }

    if (!client_id) {
        free(token);
        free(token_type_hint);
        free(client_id);
        free(client_secret);
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_client\",\"error_description\":\"Invalid client credentials\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }

    char *introspection_response = oidc_process_introspection_request(
        token, token_type_hint, client_id, client_secret);

    free(token);
    free(token_type_hint);
    free(client_id);
    free(client_secret);

    if (!introspection_response) {
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_client\",\"error_description\":\"Client authentication failed\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }

    enum MHD_Result ret = send_oidc_json_response(connection, introspection_response, MHD_HTTP_OK);
    free(introspection_response);

    return ret;
}
