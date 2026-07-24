/*
 * OIDC Token Endpoint Implementation
 *
 * Implements the OAuth 2.0 token endpoint (/oauth/token)
 */

#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/api/api_utils.h>

#include "token.h"

#include <string.h>

char* oidc_token_form_get(const char *body, const char *key);
void oidc_token_merge_strdup(char **dest, const char *src);
void oidc_token_free_params(char *grant_type, char *code, char *redirect_uri,
                            char *client_id, char *client_secret,
                            char *refresh_token, char *code_verifier);
unsigned int oidc_token_http_status_for_json(const char *json);

char* oidc_token_form_get(const char *body, const char *key) {
    if (!body || !key) {
        return NULL;
    }
    size_t key_len = strlen(key);
    const char *p = body;
    while (p && *p) {
        if ((p == body || p[-1] == '&') &&
            strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            const char *val = p + key_len + 1;
            const char *end = strchr(val, '&');
            size_t len = end ? (size_t)(end - val) : strlen(val);
            char *raw = (char*)malloc(len + 1U);
            if (!raw) {
                return NULL;
            }
            memcpy(raw, val, len);
            raw[len] = '\0';
            char *decoded = api_url_decode(raw);
            free(raw);
            return decoded;
        }
        p = strchr(p, '&');
        if (p) {
            p++;
        }
    }
    return NULL;
}

void oidc_token_merge_strdup(char **dest, const char *src) {
    if (dest && !*dest && src) {
        *dest = strdup(src);
    }
}

void oidc_token_free_params(char *grant_type, char *code, char *redirect_uri,
                            char *client_id, char *client_secret,
                            char *refresh_token, char *code_verifier) {
    free(grant_type);
    free(code);
    free(redirect_uri);
    free(client_id);
    free(client_secret);
    free(refresh_token);
    free(code_verifier);
}

unsigned int oidc_token_http_status_for_json(const char *json) {
    if (!json) {
        return MHD_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (strstr(json, "\"error\":\"invalid_client\"") != NULL) {
        return MHD_HTTP_UNAUTHORIZED;
    }
    if (strstr(json, "\"error\"") != NULL) {
        return MHD_HTTP_BAD_REQUEST;
    }
    return MHD_HTTP_OK;
}

enum MHD_Result handle_oidc_token_endpoint(struct MHD_Connection *connection,
                                     const char *method,
                                     const char *upload_data,
                                     size_t *upload_data_size,
                                     void **con_cls) {
    log_this(SR_OIDC, "Handling token endpoint request", LOG_LEVEL_DEBUG, 0);

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

    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;

    if (buffer && buffer->data && buffer->size > 0U) {
        char *body = (char*)malloc(buffer->size + 1U);
        if (body) {
            memcpy(body, buffer->data, buffer->size);
            body[buffer->size] = '\0';
            grant_type = oidc_token_form_get(body, "grant_type");
            code = oidc_token_form_get(body, "code");
            redirect_uri = oidc_token_form_get(body, "redirect_uri");
            client_id = oidc_token_form_get(body, "client_id");
            client_secret = oidc_token_form_get(body, "client_secret");
            refresh_token = oidc_token_form_get(body, "refresh_token");
            code_verifier = oidc_token_form_get(body, "code_verifier");
            free(body);
        }
    }

    /* Optional Basic auth client credentials when not in body. */
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

    if (!grant_type) {
        oidc_token_free_params(grant_type, code, redirect_uri, client_id,
                               client_secret, refresh_token, code_verifier);
        return send_oidc_json_response(connection,
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Missing grant_type\"}",
                                   MHD_HTTP_BAD_REQUEST);
    }

    char *token_response = oidc_process_token_request(grant_type, code, redirect_uri,
                                                      client_id, client_secret,
                                                      refresh_token, code_verifier);
    oidc_token_free_params(grant_type, code, redirect_uri, client_id,
                           client_secret, refresh_token, code_verifier);

    if (!token_response) {
        return send_oidc_json_response(connection,
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to process token request\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    unsigned int status = oidc_token_http_status_for_json(token_response);
    enum MHD_Result ret = send_oidc_json_response(connection, token_response, status);
    free(token_response);
    return ret;
}
