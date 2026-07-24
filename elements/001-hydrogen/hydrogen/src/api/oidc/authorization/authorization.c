/*
 * OIDC Authorization Endpoint Implementation
 *
 * GET  /oauth/authorize — validate params, show login form
 * POST /oauth/authorize — authenticate resource owner, issue code, redirect
 */

#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/api/api_utils.h>

#include "authorization.h"

#include <stdio.h>
#include <string.h>

enum MHD_Result oidc_auth_send_html(struct MHD_Connection *connection,
                                    const char *html, unsigned int status);
enum MHD_Result oidc_auth_send_redirect(struct MHD_Connection *connection,
                                        const char *location);
char* oidc_auth_build_login_html(const char *client_id, const char *redirect_uri,
                                 const char *response_type, const char *scope,
                                 const char *state, const char *nonce,
                                 const char *code_challenge,
                                 const char *code_challenge_method,
                                 const char *error_msg);
void oidc_auth_free_params(char *client_id, char *redirect_uri, char *response_type,
                           char *scope, char *state, char *nonce,
                           char *code_challenge, char *code_challenge_method);
bool oidc_auth_params_valid_for_code(const char *client_id, const char *redirect_uri,
                                     const char *response_type,
                                     const char *code_challenge,
                                     const char *code_challenge_method,
                                     const char **error_out);

enum MHD_Result oidc_auth_send_html(struct MHD_Connection *connection,
                                    const char *html, unsigned int status) {
    if (!html) {
        return MHD_NO;
    }
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(html), (void*)html, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "text/html; charset=utf-8");
    add_oidc_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return ret;
}

enum MHD_Result oidc_auth_send_redirect(struct MHD_Connection *connection,
                                        const char *location) {
    if (!location) {
        return MHD_NO;
    }
    struct MHD_Response *response = MHD_create_response_from_buffer(0, (void*)"", MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Location", location);
    add_oidc_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

void oidc_auth_free_params(char *client_id, char *redirect_uri, char *response_type,
                           char *scope, char *state, char *nonce,
                           char *code_challenge, char *code_challenge_method) {
    free(client_id);
    free(redirect_uri);
    free(response_type);
    free(scope);
    free(state);
    free(nonce);
    free(code_challenge);
    free(code_challenge_method);
}

bool oidc_auth_params_valid_for_code(const char *client_id, const char *redirect_uri,
                                     const char *response_type,
                                     const char *code_challenge,
                                     const char *code_challenge_method,
                                     const char **error_out) {
    if (!client_id || !redirect_uri || !response_type) {
        if (error_out) {
            *error_out = "invalid_request";
        }
        return false;
    }
    if (strcmp(response_type, "code") != 0) {
        if (error_out) {
            *error_out = "unsupported_response_type";
        }
        return false;
    }
    if (!code_challenge || !oidc_pkce_method_is_s256(code_challenge_method)) {
        if (error_out) {
            *error_out = "invalid_request";
        }
        return false;
    }

    OIDCContext *ctx = get_oidc_context();
    if (!ctx || !ctx->client_context) {
        if (error_out) {
            *error_out = "server_error";
        }
        return false;
    }
    if (!oidc_validate_client((OIDCClientContext*)ctx->client_context, client_id, redirect_uri)) {
        if (error_out) {
            *error_out = "unauthorized_client";
        }
        return false;
    }
    return true;
}

char* oidc_auth_build_login_html(const char *client_id, const char *redirect_uri,
                                 const char *response_type, const char *scope,
                                 const char *state, const char *nonce,
                                 const char *code_challenge,
                                 const char *code_challenge_method,
                                 const char *error_msg) {
    char *html = NULL;
    const char *err = error_msg ? error_msg : "";
    if (asprintf(&html,
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        "<title>Sign in</title>"
        "<style>body{font-family:sans-serif;max-width:28rem;margin:2rem auto;padding:1rem}"
        "label{display:block;margin-top:.75rem}input{width:100%%;padding:.4rem}"
        "button{margin-top:1rem;padding:.5rem 1rem}.err{color:#a00}</style></head><body>"
        "<h1>Sign in</h1>"
        "<p class=\"err\">%s</p>"
        "<form method=\"POST\" action=\"/oauth/authorize\">"
        "<input type=\"hidden\" name=\"client_id\" value=\"%s\">"
        "<input type=\"hidden\" name=\"redirect_uri\" value=\"%s\">"
        "<input type=\"hidden\" name=\"response_type\" value=\"%s\">"
        "<input type=\"hidden\" name=\"scope\" value=\"%s\">"
        "<input type=\"hidden\" name=\"state\" value=\"%s\">"
        "<input type=\"hidden\" name=\"nonce\" value=\"%s\">"
        "<input type=\"hidden\" name=\"code_challenge\" value=\"%s\">"
        "<input type=\"hidden\" name=\"code_challenge_method\" value=\"%s\">"
        "<label>Username or email"
        "<input name=\"username\" type=\"text\" autocomplete=\"username\" required></label>"
        "<label>Password"
        "<input name=\"password\" type=\"password\" autocomplete=\"current-password\" required></label>"
        "<button type=\"submit\">Continue</button>"
        "</form></body></html>",
        err,
        client_id ? client_id : "",
        redirect_uri ? redirect_uri : "",
        response_type ? response_type : "code",
        scope ? scope : "openid",
        state ? state : "",
        nonce ? nonce : "",
        code_challenge ? code_challenge : "",
        code_challenge_method ? code_challenge_method : "S256") < 0) {
        return NULL;
    }
    return html;
}

char* oidc_auth_form_get(const char *body, const char *key);
void oidc_auth_merge_strdup(char **dest, const char *src);

char* oidc_auth_form_get(const char *body, const char *key) {
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

void oidc_auth_merge_strdup(char **dest, const char *src) {
    if (dest && !*dest && src) {
        *dest = strdup(src);
    }
}

enum MHD_Result handle_oidc_authorization_endpoint(struct MHD_Connection *connection,
                                             const char *method,
                                             const char *upload_data,
                                             size_t *upload_data_size,
                                             void **con_cls) {
    log_this(SR_OIDC, "Handling authorization endpoint request", LOG_LEVEL_DEBUG, 0);

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
        return send_oauth_error(connection, "invalid_request", "Method not allowed", NULL, NULL);
    }

    char *client_id = NULL;
    char *redirect_uri = NULL;
    char *response_type = NULL;
    char *scope = NULL;
    char *state = NULL;
    char *nonce = NULL;
    char *code_challenge = NULL;
    char *code_challenge_method = NULL;
    char *username = NULL;
    char *password = NULL;

    (void)extract_oauth_params(connection, &client_id, &redirect_uri, &response_type,
                               &scope, &state, &nonce, &code_challenge, &code_challenge_method);

    if (buffer && buffer->data && buffer->size > 0U) {
        char *body = (char*)malloc(buffer->size + 1U);
        if (body) {
            memcpy(body, buffer->data, buffer->size);
            body[buffer->size] = '\0';
            char *v;
            v = oidc_auth_form_get(body, "client_id");
            oidc_auth_merge_strdup(&client_id, v);
            free(v);
            v = oidc_auth_form_get(body, "redirect_uri");
            oidc_auth_merge_strdup(&redirect_uri, v);
            free(v);
            v = oidc_auth_form_get(body, "response_type");
            oidc_auth_merge_strdup(&response_type, v);
            free(v);
            v = oidc_auth_form_get(body, "scope");
            oidc_auth_merge_strdup(&scope, v);
            free(v);
            v = oidc_auth_form_get(body, "state");
            oidc_auth_merge_strdup(&state, v);
            free(v);
            v = oidc_auth_form_get(body, "nonce");
            oidc_auth_merge_strdup(&nonce, v);
            free(v);
            v = oidc_auth_form_get(body, "code_challenge");
            oidc_auth_merge_strdup(&code_challenge, v);
            free(v);
            v = oidc_auth_form_get(body, "code_challenge_method");
            oidc_auth_merge_strdup(&code_challenge_method, v);
            free(v);
            username = oidc_auth_form_get(body, "username");
            if (!username) {
                username = oidc_auth_form_get(body, "login_id");
            }
            password = oidc_auth_form_get(body, "password");
            free(body);
        }
    }

    api_free_post_buffer(con_cls);

    const char *param_error = NULL;
    if (!oidc_auth_params_valid_for_code(client_id, redirect_uri, response_type,
                                         code_challenge, code_challenge_method,
                                         &param_error)) {
        enum MHD_Result ret = send_oauth_error(connection,
                                               param_error ? param_error : "invalid_request",
                                               "Invalid authorization request",
                                               redirect_uri, state);
        oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                              nonce, code_challenge, code_challenge_method);
        free(username);
        free(password);
        return ret;
    }

    if (method && strcmp(method, "GET") == 0) {
        char *html = oidc_auth_build_login_html(client_id, redirect_uri, response_type,
                                                scope, state, nonce, code_challenge,
                                                code_challenge_method, NULL);
        oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                              nonce, code_challenge, code_challenge_method);
        free(username);
        free(password);
        if (!html) {
            return MHD_NO;
        }
        enum MHD_Result ret = oidc_auth_send_html(connection, html, MHD_HTTP_OK);
        free(html);
        return ret;
    }

    if (!username || !password || username[0] == '\0' || password[0] == '\0') {
        char *html = oidc_auth_build_login_html(client_id, redirect_uri, response_type,
                                                scope, state, nonce, code_challenge,
                                                code_challenge_method,
                                                "Username and password are required.");
        oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                              nonce, code_challenge, code_challenge_method);
        free(username);
        free(password);
        if (!html) {
            return MHD_NO;
        }
        enum MHD_Result ret = oidc_auth_send_html(connection, html, MHD_HTTP_OK);
        free(html);
        return ret;
    }

    int account_id = 0;
    if (!oidc_authenticate_resource_owner(username, password, &account_id)) {
        char *html = oidc_auth_build_login_html(client_id, redirect_uri, response_type,
                                                scope, state, nonce, code_challenge,
                                                code_challenge_method,
                                                "Invalid credentials.");
        oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                              nonce, code_challenge, code_challenge_method);
        free(username);
        free(password);
        if (!html) {
            return MHD_NO;
        }
        enum MHD_Result ret = oidc_auth_send_html(connection, html, MHD_HTTP_OK);
        free(html);
        return ret;
    }
    free(username);
    free(password);

    const char *issue_error = NULL;
    char *code = oidc_issue_authorization_code(client_id, redirect_uri, scope, nonce,
                                               code_challenge, code_challenge_method,
                                               account_id, &issue_error);
    if (!code) {
        enum MHD_Result ret = send_oauth_error(connection,
                                               issue_error ? issue_error : "server_error",
                                               "Failed to issue authorization code",
                                               redirect_uri, state);
        oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                              nonce, code_challenge, code_challenge_method);
        return ret;
    }

    char *location = NULL;
    if (state && state[0] != '\0') {
        if (asprintf(&location, "%s?code=%s&state=%s", redirect_uri, code, state) < 0) {
            location = NULL;
        }
    } else {
        if (asprintf(&location, "%s?code=%s", redirect_uri, code) < 0) {
            location = NULL;
        }
    }
    free(code);
    oidc_auth_free_params(client_id, redirect_uri, response_type, scope, state,
                          nonce, code_challenge, code_challenge_method);

    if (!location) {
        return MHD_NO;
    }
    enum MHD_Result ret = oidc_auth_send_redirect(connection, location);
    free(location);
    return ret;
}
