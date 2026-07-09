/*
 * Mail Relay Send API Endpoint Implementation
 *
 * Implements POST /api/mailrelay/send for authenticated, templated mail
 * submission through the internal Mail Relay producer.
 */

// Standard includes
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/api/mailrelay/mailrelay_api_auth.h>
#include <src/mailrelay/mailrelay.h>
#include <src/utils/utils_time.h>

// Local includes
#include "send.h"

// Maximum supported recipients in a single API request.
#define MAILRELAY_SEND_MAX_RECIPIENTS 256

// Maximum supported parameters in a single API request.
#define MAILRELAY_SEND_MAX_PARAMS 256

// Maximum idempotency key length.
#define MAILRELAY_SEND_MAX_IDEMPOTENCY_LEN 512

// Maximum template key length.
#define MAILRELAY_SEND_MAX_TEMPLATE_KEY_LEN 256

// Maximum rendered error code length.
#define MAILRELAY_SEND_MAX_ERROR_CODE_LEN 64

// Maximum rendered error message length.
#define MAILRELAY_SEND_MAX_ERROR_MESSAGE_LEN 512

// Forward declarations for internal helpers
static bool parse_string_array(json_t* arr, const char* field_name,
                                const char* const** out_items, int* out_count,
                                char* err, size_t err_cap);
static bool parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                                   char* err, size_t err_cap);
static bool parse_send_request_json(json_t* request_json,
                                       MailRelaySendTemplateRequest* req,
                                       MailRelayTemplateParams* params,
                                       const char* const** to_arr,
                                       const char* const** cc_arr,
                                       const char* const** bcc_arr,
                                       char* err, size_t err_cap);
static void parse_producer_error(const char* producer_err,
                                  char* code, size_t code_cap,
                                  char* message, size_t message_cap,
                                  unsigned int* http_status);
static enum MHD_Result send_mailrelay_error(struct MHD_Connection* connection,
                                             const char* code,
                                             const char* message,
                                             unsigned int http_status);

/*
 * Parse a JSON array of strings into a const char* array. The returned pointers
 * reference the JSON string values and are valid only while the json_t object
 * is alive. NULL or non-array JSON is treated as an empty array.
 */
static bool parse_string_array(json_t* arr, const char* field_name,
                                const char* const** out_items, int* out_count,
                                char* err, size_t err_cap) {
    *out_items = NULL;
    *out_count = 0;

    if (!arr) {
        return true;
    }

    if (!json_is_array(arr)) {
        snprintf(err, err_cap, "Field '%s' must be an array of strings", field_name);
        return false;
    }

    size_t count = json_array_size(arr);
    if (count == 0) {
        return true;
    }

    if (count > (size_t)MAILRELAY_SEND_MAX_RECIPIENTS) {
        snprintf(err, err_cap, "Field '%s' exceeds maximum of %d items",
                 field_name, MAILRELAY_SEND_MAX_RECIPIENTS);
        return false;
    }

    const char** items = calloc(count, sizeof(const char*));
    if (!items) {
        snprintf(err, err_cap, "Memory allocation failed for '%s'", field_name);
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        json_t* item = json_array_get(arr, i);
        if (!json_is_string(item)) {
            free(items);
            snprintf(err, err_cap, "Field '%s' must contain only strings", field_name);
            return false;
        }
        items[i] = json_string_value(item);
    }

    *out_items = (const char* const*)items;
    *out_count = (int)count;
    return true;
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
    size_t count = 0;

    json_object_foreach(obj, key, value) {
        (void)count;
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
 * Parse the request JSON into a MailRelaySendTemplateRequest.
 */
static bool parse_send_request_json(json_t* request_json,
                                       MailRelaySendTemplateRequest* req,
                                       MailRelayTemplateParams* params,
                                       const char* const** to_arr,
                                       const char* const** cc_arr,
                                       const char* const** bcc_arr,
                                       char* err, size_t err_cap) {
    if (!request_json || !req || !params || !to_arr || !cc_arr || !bcc_arr || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "Invalid arguments");
        }
        return false;
    }

    memset(req, 0, sizeof(*req));
    mailrelay_template_params_init(params);
    *to_arr = NULL;
    *cc_arr = NULL;
    *bcc_arr = NULL;

    json_t* template_key_json = json_object_get(request_json, "template_key");
    if (!template_key_json || !json_is_string(template_key_json)) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: template_key is required");
        return false;
    }
    req->template_key = json_string_value(template_key_json);
    if (strlen(req->template_key) > MAILRELAY_SEND_MAX_TEMPLATE_KEY_LEN) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: template_key is too long");
        return false;
    }

    json_t* idempotency_key_json = json_object_get(request_json, "idempotency_key");
    if (!idempotency_key_json || !json_is_string(idempotency_key_json)) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: idempotency_key is required");
        return false;
    }
    req->idempotency_key = json_string_value(idempotency_key_json);
    if (strlen(req->idempotency_key) > MAILRELAY_SEND_MAX_IDEMPOTENCY_LEN) {
        snprintf(err, err_cap, "MAIL_PARAM_MISSING: idempotency_key is too long");
        return false;
    }

    if (!parse_string_array(json_object_get(request_json, "to"), "to",
                             to_arr, &req->to_count, err, err_cap)) {
        return false;
    }
    req->to = *to_arr;

    if (!parse_string_array(json_object_get(request_json, "cc"), "cc",
                             cc_arr, &req->cc_count, err, err_cap)) {
        return false;
    }
    req->cc = *cc_arr;

    if (!parse_string_array(json_object_get(request_json, "bcc"), "bcc",
                             bcc_arr, &req->bcc_count, err, err_cap)) {
        return false;
    }
    req->bcc = *bcc_arr;

    if (req->to_count == 0 && req->cc_count == 0 && req->bcc_count == 0) {
        snprintf(err, err_cap, "MAIL_RECIPIENT_INVALID: at least one recipient is required");
        return false;
    }

    json_t* params_json = json_object_get(request_json, "params");
    if (!parse_template_params(params_json, params, err, err_cap)) {
        return false;
    }
    req->params = params;

    json_t* priority_json = json_object_get(request_json, "priority");
    if (priority_json) {
        if (!json_is_integer(priority_json)) {
            snprintf(err, err_cap, "Field 'priority' must be an integer");
            return false;
        }
        req->priority = (int)json_integer_value(priority_json);
    } else {
        req->priority = 0;
    }

    // from/reply_to are forced from configuration; client overrides are not accepted.
    req->from = NULL;
    req->reply_to = NULL;

    return true;
}

/*
 * Map a producer error string to a stable API error code, message, and HTTP
 * status. The producer prefixes errors with a code like "MAIL_QUEUE_FULL:".
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
 * Send a standardized Mail Relay error response.
 */
static enum MHD_Result send_mailrelay_error(struct MHD_Connection* connection,
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
 * Handle POST /api/mailrelay/send requests.
 */
enum MHD_Result handle_mailrelay_send_request(
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
        return send_mailrelay_error(connection, "MAIL_METHOD_NOT_ALLOWED",
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
        return send_mailrelay_error(connection, "MAIL_METHOD_NOT_ALLOWED",
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
        return send_mailrelay_error(connection, "MAIL_INVALID_JSON",
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
        (void)send_jwt_error_response(connection, error_msg, MHD_HTTP_UNAUTHORIZED);
        return MHD_YES;  // Error response already queued
    }

    if (!validate_jwt_claims(&jwt_result, connection)) {
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
            jwt_result.claims = NULL;
        }
        return MHD_YES;  // Error response already sent by validate_jwt_claims
    }

    // Require mail_send role
    if (!mailrelay_api_has_role(jwt_result.claims, "mail_send")) {
        free_jwt_claims(jwt_result.claims);
        jwt_result.claims = NULL;
        return send_mailrelay_error(connection, "MAIL_AUTH_REQUIRED",
                                     "JWT token missing required 'mail_send' role",
                                     MHD_HTTP_UNAUTHORIZED);
    }

    // Parse request body
    MailRelaySendTemplateRequest req = {0};
    MailRelayTemplateParams params = {0};
    const char* const* to_arr = NULL;
    const char* const* cc_arr = NULL;
    const char* const* bcc_arr = NULL;
    char parse_err[256] = {0};

    if (!parse_send_request_json(request_json, &req, &params, &to_arr, &cc_arr, &bcc_arr,
                                  parse_err, sizeof(parse_err))) {
        char code[MAILRELAY_SEND_MAX_ERROR_CODE_LEN] = {0};
        char message[MAILRELAY_SEND_MAX_ERROR_MESSAGE_LEN] = {0};
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;
        parse_producer_error(parse_err, code, sizeof(code), message, sizeof(message), &http_status);
        free_jwt_claims(jwt_result.claims);
        jwt_result.claims = NULL;
        json_decref(request_json);
        free((void*)to_arr);
        free((void*)cc_arr);
        free((void*)bcc_arr);
        mailrelay_template_params_free(&params);
        return send_mailrelay_error(connection, code, message, http_status);
    }

    // Fill built-in macro sources from request context
    req.app_name = "Hydrogen";
    req.server_name = (app_config && app_config->server.server_name) ? app_config->server.server_name : "Hydrogen";

    char timestamp_buffer[32] = {0};
    format_iso_time(time(NULL), timestamp_buffer, sizeof(timestamp_buffer));
    req.timestamp = timestamp_buffer;

    req.request_id = (jwt_result.claims && jwt_result.claims->jti) ? jwt_result.claims->jti : "";
    req.user_email = (jwt_result.claims && jwt_result.claims->email) ? jwt_result.claims->email : "";
    req.otp_code = NULL;

    // Call the internal templated-send producer
    MailRelaySendTemplateResponse resp = {0};
    char producer_err[256] = {0};
    MailRelayStatus status = mailrelay_send_template(&req, &resp, producer_err, sizeof(producer_err));

    free_jwt_claims(jwt_result.claims);
    jwt_result.claims = NULL;

    enum MHD_Result result = MHD_YES;

    if (status == MAILRELAY_OK) {
        json_t* response = json_object();
        if (!response) {
            result = api_send_error_and_cleanup(connection, NULL,
                                                 "Failed to build response",
                                                 MHD_HTTP_INTERNAL_SERVER_ERROR);
        } else {
            json_object_set_new(response, "success", json_true());
            json_object_set_new(response, "message_id", json_string(resp.message_id ? resp.message_id : ""));
            json_object_set_new(response, "status", json_string(resp.status ? resp.status : "queued"));
            result = api_send_json_response(connection, response, MHD_HTTP_OK);
            json_decref(response);
        }
    } else {
        char code[MAILRELAY_SEND_MAX_ERROR_CODE_LEN] = {0};
        char message[MAILRELAY_SEND_MAX_ERROR_MESSAGE_LEN] = {0};
        unsigned int http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
        parse_producer_error(producer_err, code, sizeof(code), message, sizeof(message), &http_status);
        result = send_mailrelay_error(connection, code, message, http_status);
    }

    mailrelay_send_template_response_free(&resp);
    mailrelay_template_params_free(&params);
    free((void*)to_arr);
    free((void*)cc_arr);
    free((void*)bcc_arr);
    json_decref(request_json);

    return result;
}
