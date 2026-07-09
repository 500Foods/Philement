/*
 * Mail Relay Preview API Endpoint Implementation
 *
 * Implements POST /api/mailrelay/preview for authenticated, side-effect-free
 * template rendering through the internal Mail Relay macro engine.
 */

// Standard includes
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_template.h>
#include <src/utils/utils_time.h>

// Local includes
#include "preview.h"

// Maximum supported parameters in a single preview request.
#define MAILRELAY_PREVIEW_MAX_PARAMS 256

// Maximum template key length.
#define MAILRELAY_PREVIEW_MAX_TEMPLATE_KEY_LEN 256

// Maximum rendered error code length.
#define MAILRELAY_PREVIEW_MAX_ERROR_CODE_LEN 64

// Maximum rendered error message length.
#define MAILRELAY_PREVIEW_MAX_ERROR_MESSAGE_LEN 512

// Forward declarations for internal helpers
static bool has_role(const char* roles, const char* role);
static bool parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                                  char* err, size_t err_cap);
static bool parse_preview_request_json(json_t* request_json,
                                       const char** template_key_out,
                                       MailRelayTemplateParams* params,
                                       char* err, size_t err_cap);
static void parse_producer_error(const char* producer_err,
                                  char* code, size_t code_cap,
                                  char* message, size_t message_cap,
                                  unsigned int* http_status);
static enum MHD_Result send_mailrelay_preview_error(struct MHD_Connection* connection,
                                                     const char* code,
                                                     const char* message,
                                                     unsigned int http_status);

/*
 * Check whether a comma-separated role list contains a specific role.
 * Whitespace around roles is ignored. Roles are matched case-sensitively.
 */
static bool has_role(const char* roles, const char* role) {
    if (!roles || !role || role[0] == '\0') {
        return false;
    }

    size_t role_len = strlen(role);
    const char* p = roles;

    while (*p != '\0') {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (strncmp(p, role, role_len) == 0) {
            char next = p[role_len];
            if (next == '\0' || next == ',' || next == ' ' || next == '\t') {
                return true;
            }
        }

        // Advance to the next comma
        while (*p != '\0' && *p != ',') {
            p++;
        }
        if (*p == ',') {
            p++;
        }
    }

    return false;
}

/*
 * Parse a JSON object of string values into a MailRelayTemplateParams map.
 * Non-string values are rejected.
 */
static bool parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                                   char* err, size_t err_cap) {
    if (!obj) {
        return true;
    }

    if (!json_is_object(obj)) {
        snprintf(err, err_cap, "Field 'params' must be an object of strings");
        return false;
    }

    const char* key = NULL;
    json_t* value = NULL;

    json_object_foreach(obj, key, value) {
        if (!json_is_string(value)) {
            snprintf(err, err_cap, "Field 'params.%s' must be a string", key);
            return false;
        }
        if (!mailrelay_template_params_add(params, key, json_string_value(value))) {
            snprintf(err, err_cap, "Failed to add parameter '%s'", key);
            return false;
        }
    }

    return true;
}

/*
 * Parse the preview request JSON. Only template_key and optional params are
 * accepted; recipients and idempotency keys are not relevant for a preview.
 */
static bool parse_preview_request_json(json_t* request_json,
                                       const char** template_key_out,
                                       MailRelayTemplateParams* params,
                                       char* err, size_t err_cap) {
    if (!request_json || !template_key_out || !params || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "Invalid arguments");
        }
        return false;
    }

    *template_key_out = NULL;
    mailrelay_template_params_init(params);

    json_t* template_key_json = json_object_get(request_json, "template_key");
    if (!template_key_json || !json_is_string(template_key_json)) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: template_key is required");
        return false;
    }
    *template_key_out = json_string_value(template_key_json);
    if (strlen(*template_key_out) > MAILRELAY_PREVIEW_MAX_TEMPLATE_KEY_LEN) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: template_key is too long");
        return false;
    }

    json_t* params_json = json_object_get(request_json, "params");
    if (!parse_template_params(params_json, params, err, err_cap)) {
        return false;
    }

    return true;
}

/*
 * Map a producer error string to a stable API error code, message, and HTTP
 * status. The producer prefixes errors with a code like "MAIL_PARAM_MISSING:".
 */
static void parse_producer_error(const char* producer_err,
                                  char* code, size_t code_cap,
                                  char* message, size_t message_cap,
                                  unsigned int* http_status) {
    if (!producer_err || producer_err[0] == '\0') {
        snprintf(code, code_cap, "MAIL_INTERNAL_ERROR");
        snprintf(message, message_cap, "Unknown mail relay error");
        *http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    const char* colon = strchr(producer_err, ':');
    if (!colon) {
        snprintf(code, code_cap, "MAIL_INTERNAL_ERROR");
        snprintf(message, message_cap, "%s", producer_err);
        *http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
        return;
    }

    size_t code_len = (size_t)(colon - producer_err);
    if (code_len >= code_cap) {
        code_len = code_cap - 1;
    }
    memcpy(code, producer_err, code_len);
    code[code_len] = '\0';

    const char* msg = colon + 1;
    while (*msg == ' ') {
        msg++;
    }
    snprintf(message, message_cap, "%s", msg);

    if (strcmp(code, "MAIL_TEMPLATE_NOT_FOUND") == 0) {
        *http_status = MHD_HTTP_NOT_FOUND;
    } else if (strcmp(code, "MAIL_PARAM_MISSING") == 0 ||
               strcmp(code, "MAIL_RECIPIENT_INVALID") == 0) {
        *http_status = MHD_HTTP_BAD_REQUEST;
    } else if (strcmp(code, "MAIL_QUEUE_FULL") == 0 ||
               strcmp(code, "MAIL_DISABLED") == 0) {
        *http_status = MHD_HTTP_SERVICE_UNAVAILABLE;
    } else if (strcmp(code, "MAIL_RATE_LIMITED") == 0) {
        *http_status = 429;
    } else {
        *http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
    }
}

/*
 * Send a standardized Mail Relay preview error response.
 */
static enum MHD_Result send_mailrelay_preview_error(struct MHD_Connection* connection,
                                                     const char* code,
                                                     const char* message,
                                                     unsigned int http_status) {
    json_t* error_response = json_object();
    if (!error_response) {
        return api_send_error_and_cleanup(connection, NULL,
                                           "Failed to build error response", http_status);
    }

    json_object_set_new(error_response, "success", json_false());
    json_object_set_new(error_response, "error", json_string(code));
    json_object_set_new(error_response, "message", json_string(message));

    enum MHD_Result result = api_send_json_response(connection, error_response, http_status);
    json_decref(error_response);
    return result;
}

/*
 * Handle POST /api/mailrelay/preview requests.
 */
enum MHD_Result handle_mailrelay_preview_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls) {
    (void)url;  // URL path is validated by dispatch

    if (!connection || !method || !upload_data_size || !con_cls) {
        return MHD_NO;
    }

    // Only POST is supported
    if (strcmp(method, "POST") != 0) {
        return send_mailrelay_preview_error(connection, "MAIL_METHOD_NOT_ALLOWED",
                                             "Only POST requests are supported",
                                             MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    // Buffer POST body
    ApiPostBuffer* buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data,
                                                       upload_data_size, con_cls, &buffer);

    if (buf_result == API_BUFFER_ERROR) {
        // Error response already sent by api_buffer_post_data
        return MHD_YES;
    }
    if (buf_result == API_BUFFER_METHOD_ERROR) {
        api_free_post_buffer(con_cls);
        return send_mailrelay_preview_error(connection, "MAIL_METHOD_NOT_ALLOWED",
                                             "Only POST requests are supported",
                                             MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    if (buf_result == API_BUFFER_CONTINUE) {
        // Need more data; MHD will call again
        return MHD_YES;
    }

    // Parse JSON body
    json_t* request_json = api_parse_json_body(buffer);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        return send_mailrelay_preview_error(connection, "MAIL_INVALID_JSON",
                                             "Request body contains invalid JSON",
                                             MHD_HTTP_BAD_REQUEST);
    }

    // Validate JWT Bearer token
    const char* auth_header = MHD_lookup_connection_value(connection,
                                                           MHD_HEADER_KIND,
                                                           "Authorization");
    jwt_validation_result_t jwt_result;
    if (!extract_and_validate_jwt(auth_header, &jwt_result)) {
        const char* error_msg = get_jwt_error_message(jwt_result.error);
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
            jwt_result.claims = NULL;
        }
        json_decref(request_json);
        return send_jwt_error_response(connection, error_msg, MHD_HTTP_UNAUTHORIZED);
    }

    if (!validate_jwt_claims(&jwt_result, connection)) {
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
            jwt_result.claims = NULL;
        }
        json_decref(request_json);
        return MHD_YES;  // Error response already sent by validate_jwt_claims
    }

    // Require mail_send role
    if (!has_role(jwt_result.claims ? jwt_result.claims->roles : NULL, "mail_send")) {
        free_jwt_claims(jwt_result.claims);
        jwt_result.claims = NULL;
        json_decref(request_json);
        return send_mailrelay_preview_error(connection, "MAIL_AUTH_REQUIRED",
                                             "JWT token missing required 'mail_send' role",
                                             MHD_HTTP_UNAUTHORIZED);
    }

    // Parse request body
    const char* template_key = NULL;
    MailRelayTemplateParams params = {0};
    char parse_err[256] = {0};

    if (!parse_preview_request_json(request_json, &template_key, &params,
                                     parse_err, sizeof(parse_err))) {
        char code[MAILRELAY_PREVIEW_MAX_ERROR_CODE_LEN] = {0};
        char message[MAILRELAY_PREVIEW_MAX_ERROR_MESSAGE_LEN] = {0};
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;
        parse_producer_error(parse_err, code, sizeof(code), message, sizeof(message), &http_status);
        free_jwt_claims(jwt_result.claims);
        jwt_result.claims = NULL;
        json_decref(request_json);
        mailrelay_template_params_free(&params);
        return send_mailrelay_preview_error(connection, code, message, http_status);
    }

    // Mail Relay must be enabled to preview templates
    if (!app_config || !app_config->mail_relay.Enabled) {
        free_jwt_claims(jwt_result.claims);
        jwt_result.claims = NULL;
        json_decref(request_json);
        mailrelay_template_params_free(&params);
        return send_mailrelay_preview_error(connection, "MAIL_DISABLED",
                                             "Mail relay is disabled",
                                             MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    // Fill built-in macro sources from request context
    const char* app_name = "Hydrogen";
    const char* server_name = (app_config && app_config->server.server_name) ?
                               app_config->server.server_name : "Hydrogen";

    char timestamp_buffer[32] = {0};
    format_iso_time(time(NULL), timestamp_buffer, sizeof(timestamp_buffer));
    const char* timestamp = timestamp_buffer;

    const char* request_id = (jwt_result.claims && jwt_result.claims->jti) ?
                              jwt_result.claims->jti : "";
    const char* user_email = (jwt_result.claims && jwt_result.claims->email) ?
                              jwt_result.claims->email : "";

    // Render the template without queue or SMTP side effects
    char* subject = NULL;
    char* text_body = NULL;
    char* html_body = NULL;
    MailRelayMacroList macros_used = {0};
    mailrelay_macro_list_init(&macros_used);

    char render_err[256] = {0};
    bool render_ok = mailrelay_template_preview_with_macros(
        template_key, &params, app_name, server_name, timestamp, request_id, user_email, NULL,
        &subject, &text_body, &html_body, &macros_used,
        render_err, sizeof(render_err));

    free_jwt_claims(jwt_result.claims);
    jwt_result.claims = NULL;
    mailrelay_template_params_free(&params);

    enum MHD_Result result = MHD_YES;

    if (render_ok) {
        json_t* response = json_object();
        if (!response) {
            result = api_send_error_and_cleanup(connection, NULL,
                                                 "Failed to build response",
                                                 MHD_HTTP_INTERNAL_SERVER_ERROR);
        } else {
            json_object_set_new(response, "success", json_true());
            json_object_set_new(response, "template_key", json_string(template_key));

            const char* from = (app_config && app_config->mail_relay.DefaultFrom) ?
                                app_config->mail_relay.DefaultFrom : NULL;
            const char* reply_to = (app_config && app_config->mail_relay.DefaultReplyTo) ?
                                    app_config->mail_relay.DefaultReplyTo : NULL;
            if (from && from[0] != '\0') {
                json_object_set_new(response, "from", json_string(from));
            }
            if (reply_to && reply_to[0] != '\0') {
                json_object_set_new(response, "reply_to", json_string(reply_to));
            }

            json_object_set_new(response, "subject", json_string(subject ? subject : ""));
            if (text_body) {
                json_object_set_new(response, "text_body", json_string(text_body));
            }
            if (html_body) {
                json_object_set_new(response, "html_body", json_string(html_body));
            }

            json_t* macros_json = json_array();
            if (macros_json) {
                for (int i = 0; i < macros_used.count; i++) {
                    json_array_append_new(macros_json, json_string(macros_used.names[i]));
                }
                json_object_set_new(response, "macros_used", macros_json);
            }

            result = api_send_json_response(connection, response, MHD_HTTP_OK);
            json_decref(response);
        }
    } else {
        char code[MAILRELAY_PREVIEW_MAX_ERROR_CODE_LEN] = {0};
        char message[MAILRELAY_PREVIEW_MAX_ERROR_MESSAGE_LEN] = {0};
        unsigned int http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
        parse_producer_error(render_err, code, sizeof(code), message, sizeof(message), &http_status);
        result = send_mailrelay_preview_error(connection, code, message, http_status);
    }

    free(subject);
    free(text_body);
    free(html_body);
    mailrelay_macro_list_free(&macros_used);
    json_decref(request_json);

    return result;
}
