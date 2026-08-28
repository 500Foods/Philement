/*
 * Conduit Script Invoke API (LUA_CLIENT Phases 4–8).
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting_invoke.h>
#include <src/api/auth/auth_service_jwt.h>

#ifdef UNITY_TEST_MODE
#define USE_MOCK_API_UTILS
#include <unity/mocks/mock_api_utils.h>
#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif
#endif

#include "script.h"

/* --- test seams ----------------------------------------------------------- */

static conduit_script_submit_fn g_submit_hook = NULL;
static conduit_script_wait_fn g_wait_hook = NULL;
static conduit_script_jwt_fn g_jwt_hook = NULL;

void conduit_script_set_submit_hook(conduit_script_submit_fn fn) {
    g_submit_hook = fn;
}

void conduit_script_set_wait_hook(conduit_script_wait_fn fn) {
    g_wait_hook = fn;
}

int conduit_script_params_max_bytes(void) {
    if (app_config && app_config->scripting.ClientInvokeMaxParamsBytes > 0) {
        return app_config->scripting.ClientInvokeMaxParamsBytes;
    }
    return CONDUIT_SCRIPT_PARAMS_MAX_BYTES;
}

int conduit_script_timeout_default_s(void) {
    if (app_config && app_config->scripting.ClientInvokeDefaultTimeout > 0) {
        return app_config->scripting.ClientInvokeDefaultTimeout;
    }
    return CONDUIT_SCRIPT_TIMEOUT_DEFAULT_S;
}

int conduit_script_timeout_max_s(void) {
    if (app_config && app_config->scripting.ClientInvokeMaxTimeout > 0) {
        return app_config->scripting.ClientInvokeMaxTimeout;
    }
    return CONDUIT_SCRIPT_TIMEOUT_MAX_S;
}

int conduit_script_result_max_bytes(void) {
    if (app_config && app_config->scripting.ClientInvokeMaxResultBytes > 0) {
        return app_config->scripting.ClientInvokeMaxResultBytes;
    }
    return CONDUIT_SCRIPT_RESULT_MAX_BYTES;
}

void conduit_script_set_jwt_hook(conduit_script_jwt_fn fn) {
    g_jwt_hook = fn;
}

/* --- helpers -------------------------------------------------------------- */

json_t *conduit_script_error_json(const char *error_code, const char *message) {
    json_t *obj = json_object();
    if (!obj) {
        return NULL;
    }
    json_object_set_new(obj, "success", json_false());
    json_object_set_new(obj, "error",
                        json_string(error_code ? error_code : "internal_error"));
    json_object_set_new(obj, "message",
                        json_string(message ? message : "Unknown error"));
    return obj;
}

static enum MHD_Result send_script_error(struct MHD_Connection *connection,
                                         const char *error_code,
                                         const char *message,
                                         unsigned int http_status) {
    json_t *body = conduit_script_error_json(error_code, message);
    if (!body) {
        return api_send_error_and_cleanup(connection, NULL,
                                          "Failed to build error response",
                                          http_status);
    }
    return api_send_json_response(connection, body, http_status);
}

bool conduit_script_is_enabled(void) {
    return app_config && app_config->scripting.Enabled;
}

char *conduit_script_extract_job_id(const char *path) {
    const char *prefix = "conduit/script/";
    size_t prefix_len = strlen(prefix);

    if (!path || strncmp(path, prefix, prefix_len) != 0) {
        return NULL;
    }

    const char *job = path + prefix_len;
    if (*job == '\0') {
        return NULL;
    }

    if (strchr(job, '/') != NULL) {
        return NULL;
    }

    return strdup(job);
}

bool conduit_script_parse_post_json(json_t *request_json,
                                    ConduitScriptRequest *out,
                                    const char **error_code,
                                    const char **error_message) {
    if (!out || !error_code || !error_message) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->wait = true;
    out->timeout_seconds = conduit_script_timeout_default_s();

    if (!request_json || !json_is_object(request_json)) {
        *error_code = "invalid_json";
        *error_message = "Request body must be a JSON object";
        return false;
    }

    json_t *script_j = json_object_get(request_json, "script");
    if (!script_j || !json_is_string(script_j)) {
        *error_code = "missing_script";
        *error_message = "Field 'script' is required and must be a string";
        return false;
    }

    const char *script = json_string_value(script_j);
    if (!script || script[0] == '\0') {
        *error_code = "missing_script";
        *error_message = "Field 'script' must be a non-empty string";
        return false;
    }
    out->script = script;

    json_t *params_j = json_object_get(request_json, "params");
    if (params_j && !json_is_null(params_j)) {
        if (!json_is_object(params_j)) {
            *error_code = "invalid_params";
            *error_message = "Field 'params' must be a JSON object when present";
            return false;
        }
        if (json_object_get(params_j, "_hydrogen") != NULL) {
            *error_code = "reserved_params";
            *error_message = "Field 'params._hydrogen' is reserved and must not be supplied by the client";
            return false;
        }
        char *dumped = json_dumps(params_j, JSON_COMPACT);
        if (!dumped) {
            *error_code = "invalid_params";
            *error_message = "Failed to serialize params";
            return false;
        }
        size_t plen = strlen(dumped);
        free(dumped);
        if (plen > (size_t)conduit_script_params_max_bytes()) {
            *error_code = "params_too_large";
            *error_message = "Field 'params' exceeds configured size limit";
            return false;
        }
        out->params = params_j;
    }

    json_t *wait_j = json_object_get(request_json, "wait");
    if (wait_j && !json_is_null(wait_j)) {
        if (!json_is_boolean(wait_j)) {
            *error_code = "invalid_wait";
            *error_message = "Field 'wait' must be a boolean when present";
            return false;
        }
        out->wait = json_is_true(wait_j);
    }

    json_t *to_j = json_object_get(request_json, "timeout_seconds");
    if (to_j && !json_is_null(to_j)) {
        if (!json_is_integer(to_j)) {
            *error_code = "invalid_timeout";
            *error_message = "Field 'timeout_seconds' must be an integer when present";
            return false;
        }
        json_int_t t = json_integer_value(to_j);
        if (t < 1) {
            t = 1;
        }
        int tmax = conduit_script_timeout_max_s();
        if (t > tmax) {
            t = tmax;
        }
        out->timeout_seconds = (int)t;
    }

    *error_code = NULL;
    *error_message = NULL;
    return true;
}

void conduit_script_set_string_if(json_t *obj, const char *key, const char *val) {
    if (val && val[0] != '\0') {
        json_object_set_new(obj, key, json_string(val));
    }
}

json_t *conduit_script_claims_to_hydrogen(const jwt_claims_t *claims) {
    json_t *h = json_object();
    if (!h || !claims) {
        if (h) {
            json_decref(h);
        }
        return NULL;
    }

    conduit_script_set_string_if(h, "sub", claims->sub);
    conduit_script_set_string_if(h, "iss", claims->iss);
    conduit_script_set_string_if(h, "aud", claims->aud);
    conduit_script_set_string_if(h, "jti", claims->jti);
    conduit_script_set_string_if(h, "username", claims->username);
    conduit_script_set_string_if(h, "email", claims->email);
    conduit_script_set_string_if(h, "roles", claims->roles);
    conduit_script_set_string_if(h, "ip", claims->ip);
    conduit_script_set_string_if(h, "tz", claims->tz);
    conduit_script_set_string_if(h, "database", claims->database);
    conduit_script_set_string_if(h, "idp_provider", claims->idp_provider);

    if (claims->user_id != 0) {
        json_object_set_new(h, "user_id", json_integer(claims->user_id));
    }
    if (claims->system_id != 0) {
        json_object_set_new(h, "system_id", json_integer(claims->system_id));
    }
    if (claims->app_id != 0) {
        json_object_set_new(h, "app_id", json_integer(claims->app_id));
    }
    if (claims->exp != 0) {
        json_object_set_new(h, "exp", json_integer((json_int_t)claims->exp));
    }
    if (claims->iat != 0) {
        json_object_set_new(h, "iat", json_integer((json_int_t)claims->iat));
    }
    json_object_set_new(h, "tzoffset", json_integer(claims->tzoffset));

    /* Intentionally omit id_token (sensitive logout material). */
    return h;
}

char *conduit_script_build_params_json(const ConduitScriptRequest *req,
                                       const jwt_claims_t *claims,
                                       const char **error_code,
                                       const char **error_message) {
    if (!req || !claims || !error_code || !error_message) {
        return NULL;
    }

    json_t *root = json_object();
    if (!root) {
        *error_code = "internal_error";
        *error_message = "Failed to allocate params object";
        return NULL;
    }

    if (req->params && json_is_object(req->params)) {
        const char *key = NULL;
        json_t *val = NULL;
        json_object_foreach(req->params, key, val) {
            json_object_set(root, key, val);
        }
    }

    json_t *hydrogen = conduit_script_claims_to_hydrogen(claims);
    if (!hydrogen) {
        json_decref(root);
        *error_code = "internal_error";
        *error_message = "Failed to build _hydrogen claims";
        return NULL;
    }
    json_object_set_new(root, "_hydrogen", hydrogen);

    char *dumped = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    if (!dumped) {
        *error_code = "internal_error";
        *error_message = "Failed to serialize merged params";
        return NULL;
    }

    if (strlen(dumped) > (size_t)conduit_script_params_max_bytes()) {
        free(dumped);
        *error_code = "params_too_large";
        *error_message = "Merged params (including _hydrogen) exceed size limit";
        return NULL;
    }

    *error_code = NULL;
    *error_message = NULL;
    return dumped;
}

const char *conduit_script_job_status_string(ScoreboardJobStatus st) {
    switch (st) {
    case SCOREBOARD_JOB_PENDING:
        return "pending";
    case SCOREBOARD_JOB_RUNNING:
        return "running";
    case SCOREBOARD_JOB_COMPLETED:
        return "completed";
    case SCOREBOARD_JOB_FAILED:
        return "failed";
    case SCOREBOARD_JOB_KILLED:
        return "killed";
    default:
        return "unknown";
    }
}

const char *conduit_script_wait_status_name(ScriptingWaitResult wr) {
    switch (wr) {
    case SCRIPTING_WAIT_COMPLETED:
        return "completed";
    case SCRIPTING_WAIT_FAILED:
        return "failed";
    case SCRIPTING_WAIT_KILLED:
        return "killed";
    case SCRIPTING_WAIT_TIMEOUT:
        return "timeout";
    case SCRIPTING_WAIT_NOT_FOUND:
        return "not_found";
    case SCRIPTING_WAIT_SHUTDOWN:
        return "shutdown";
    case SCRIPTING_WAIT_INTERNAL:
    default:
        return "internal_error";
    }
}

void conduit_script_map_invoke_error(ScriptingInvokeError err,
                                     unsigned int *http_status_out,
                                     const char **error_code_out,
                                     const char **message_out) {
    unsigned int st = MHD_HTTP_INTERNAL_SERVER_ERROR;
    const char *code = "internal_error";
    const char *msg = "Script submit failed";

    switch (err) {
    case SCRIPTING_INVOKE_OK:
        st = MHD_HTTP_OK;
        code = "ok";
        msg = "ok";
        break;
    case SCRIPTING_INVOKE_ERR_DISABLED:
        st = MHD_HTTP_SERVICE_UNAVAILABLE;
        code = "scripting_disabled";
        msg = "Scripting subsystem is not enabled";
        break;
    case SCRIPTING_INVOKE_ERR_INVALID_NAME:
        st = MHD_HTTP_BAD_REQUEST;
        code = "invalid_script_name";
        msg = "Script name must be Group.Name";
        break;
    case SCRIPTING_INVOKE_ERR_NO_DATABASE:
        st = MHD_HTTP_SERVICE_UNAVAILABLE;
        code = "no_database";
        msg = "No scripting database available";
        break;
    case SCRIPTING_INVOKE_ERR_NOT_FOUND:
        st = MHD_HTTP_NOT_FOUND;
        code = "script_not_found";
        msg = "Script not found";
        break;
    case SCRIPTING_INVOKE_ERR_DB_TIMEOUT:
        st = MHD_HTTP_GATEWAY_TIMEOUT;
        code = "script_fetch_timeout";
        msg = "Timed out loading script source";
        break;
    case SCRIPTING_INVOKE_ERR_SUBMIT_FAILED:
        st = MHD_HTTP_INTERNAL_SERVER_ERROR;
        code = "submit_failed";
        msg = "Failed to enqueue script job";
        break;
    case SCRIPTING_INVOKE_ERR_INTERNAL:
    default:
        st = MHD_HTTP_INTERNAL_SERVER_ERROR;
        code = "internal_error";
        msg = "Internal script invoke error";
        break;
    }

    if (http_status_out) {
        *http_status_out = st;
    }
    if (error_code_out) {
        *error_code_out = code;
    }
    if (message_out) {
        *message_out = msg;
    }
}

json_t *conduit_script_build_job_response(const char *status_str,
                                          const char *job_id,
                                          const char *script_name,
                                          const ScoreboardEntry *entry,
                                          long elapsed_ms) {
    json_t *resp = json_object();
    if (!resp) {
        return NULL;
    }

    json_object_set_new(resp, "status",
                        json_string(status_str ? status_str : "unknown"));
    if (job_id) {
        json_object_set_new(resp, "job_id", json_string(job_id));
    }
    if (script_name) {
        json_object_set_new(resp, "script", json_string(script_name));
    } else if (entry && entry->script_name) {
        json_object_set_new(resp, "script", json_string(entry->script_name));
    }

    json_t *result_obj = NULL;
    if (entry && entry->result_json && entry->result_json[0] != '\0') {
        json_error_t jerr;
        result_obj = json_loads(entry->result_json, 0, &jerr);
    }
    if (!result_obj) {
        result_obj = json_object();
    }
    json_object_set_new(resp, "result", result_obj);

    if (entry && entry->result_type) {
        json_object_set_new(resp, "result_type",
                            json_string(entry->result_type));
    } else {
        json_object_set_new(resp, "result_type", json_null());
    }
    if (entry && entry->result_location) {
        json_object_set_new(resp, "result_location",
                            json_string(entry->result_location));
    } else {
        json_object_set_new(resp, "result_location", json_null());
    }

    /* Phase 8: surface error_message only — never error_traceback by default. */
    if (entry && entry->error_message) {
        json_object_set_new(resp, "error", json_string(entry->error_message));
    } else {
        json_object_set_new(resp, "error", json_null());
    }

    json_object_set_new(resp, "elapsed_ms", json_integer(elapsed_ms));
    return resp;
}

long conduit_script_elapsed_ms_from_entry(const ScoreboardEntry *entry) {
    if (!entry) {
        return 0;
    }
    struct timespec end = entry->finished_at;
    if (end.tv_sec == 0 && end.tv_nsec == 0) {
        clock_gettime(CLOCK_REALTIME, &end);
    }
    struct timespec start = entry->started_at;
    if (start.tv_sec == 0 && start.tv_nsec == 0) {
        start = entry->created_at;
    }
    if (start.tv_sec == 0 && start.tv_nsec == 0) {
        return 0;
    }
    long sec = end.tv_sec - start.tv_sec;
    long nsec = end.tv_nsec - start.tv_nsec;
    return sec * 1000L + nsec / 1000000L;
}

/* --- JWT ------------------------------------------------------------------ */

static bool validate_script_jwt(struct MHD_Connection *connection,
                                jwt_validation_result_t *jwt_out) {
    memset(jwt_out, 0, sizeof(*jwt_out));

    if (g_jwt_hook) {
        if (!g_jwt_hook(connection, jwt_out)) {
            if (!jwt_out->valid) {
                (void)send_jwt_error_response(
                    connection, "Invalid or expired JWT token",
                    MHD_HTTP_UNAUTHORIZED);
            }
            return false;
        }
        return true;
    }

    const char *auth_header = MHD_lookup_connection_value(
        connection, MHD_HEADER_KIND, "Authorization");

    if (!extract_and_validate_jwt(auth_header, jwt_out)) {
        const char *msg = get_jwt_error_message(jwt_out->error);
        if (!auth_header) {
            (void)send_missing_authorization_response(connection);
        } else if (strncmp(auth_header, "Bearer ", 7) != 0) {
            (void)send_invalid_authorization_format_response(connection);
        } else {
            unsigned int jwt_http = (jwt_out->error == JWT_ERROR_UNAVAILABLE)
                ? MHD_HTTP_SERVICE_UNAVAILABLE : MHD_HTTP_UNAUTHORIZED;
            (void)send_jwt_error_response(connection, msg, jwt_http);
        }
        return false;
    }

    if (!validate_jwt_claims(jwt_out, connection)) {
        return false;
    }
    return true;
}

/* --- GET ------------------------------------------------------------------ */

static enum MHD_Result handle_script_get(struct MHD_Connection *connection,
                                         const char *job_id) {
    if (!conduit_script_is_enabled()) {
        return send_script_error(connection, "scripting_disabled",
                                 "Scripting subsystem is not enabled",
                                 MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    if (!job_id || job_id[0] == '\0') {
        return send_script_error(connection, "missing_job_id",
                                 "Job id path segment is required",
                                 MHD_HTTP_BAD_REQUEST);
    }

    jwt_validation_result_t jwt;
    if (!validate_script_jwt(connection, &jwt)) {
        return MHD_YES;
    }

    ScoreboardEntry *entry = NULL;
    if (scripting_scoreboard) {
        entry = scoreboard_find(scripting_scoreboard, job_id);
    }

    if (!entry) {
        free_jwt_claims(jwt.claims);
        return send_script_error(connection, "job_not_found",
                                 "Job not found",
                                 MHD_HTTP_NOT_FOUND);
    }

    if (entry->submitted_by) {
        const char *sub = jwt.claims->sub;
        if (!sub || strcmp(entry->submitted_by, sub) != 0) {
            scoreboard_entry_free(entry);
            free_jwt_claims(jwt.claims);
            return send_script_error(connection, "forbidden",
                                     "Not authorized to view this job",
                                     MHD_HTTP_FORBIDDEN);
        }
    }

    const char *st = conduit_script_job_status_string(entry->status);
    long elapsed = conduit_script_elapsed_ms_from_entry(entry);
    json_t *resp = conduit_script_build_job_response(
        st, job_id, entry->script_name, entry, elapsed);
    scoreboard_entry_free(entry);
    free_jwt_claims(jwt.claims);

    if (!resp) {
        return send_script_error(connection, "internal_error",
                                 "Failed to build response",
                                 MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    return api_send_json_response(connection, resp, MHD_HTTP_OK);
}

/* --- POST ----------------------------------------------------------------- */

static enum MHD_Result handle_script_post(struct MHD_Connection *connection,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls) {
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data("POST", upload_data,
                                                      upload_data_size,
                                                      con_cls, &buffer);

    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;
        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                                              "Request processing error",
                                              MHD_HTTP_INTERNAL_SERVER_ERROR);
        case API_BUFFER_METHOD_ERROR:
            api_free_post_buffer(con_cls);
            return send_script_error(connection, "method_not_allowed",
                                     "Use POST for /api/conduit/script",
                                     MHD_HTTP_METHOD_NOT_ALLOWED);
        case API_BUFFER_COMPLETE:
            break;
        default:
            api_free_post_buffer(con_cls);
            return send_script_error(connection, "internal_error",
                                     "Unexpected buffer state",
                                     MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (!conduit_script_is_enabled()) {
        api_free_post_buffer(con_cls);
        return send_script_error(connection, "scripting_disabled",
                                 "Scripting subsystem is not enabled",
                                 MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    if (!buffer || !buffer->data || buffer->size == 0) {
        api_free_post_buffer(con_cls);
        return send_script_error(connection, "invalid_json",
                                 "Request body is empty",
                                 MHD_HTTP_BAD_REQUEST);
    }

    json_error_t jerr;
    json_t *request_json = json_loadb(buffer->data, buffer->size, 0, &jerr);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        return send_script_error(connection, "invalid_json",
                                 "Request body contains invalid JSON",
                                 MHD_HTTP_BAD_REQUEST);
    }

    ConduitScriptRequest req;
    const char *err_code = NULL;
    const char *err_msg = NULL;
    if (!conduit_script_parse_post_json(request_json, &req, &err_code, &err_msg)) {
        unsigned int http_st = MHD_HTTP_BAD_REQUEST;
        if (err_code && strcmp(err_code, "params_too_large") == 0) {
            http_st = MHD_HTTP_CONTENT_TOO_LARGE; /* 413 */
        }
        enum MHD_Result r = send_script_error(connection, err_code, err_msg,
                                              http_st);
        json_decref(request_json);
        return r;
    }

    jwt_validation_result_t jwt;
    if (!validate_script_jwt(connection, &jwt)) {
        json_decref(request_json);
        return MHD_YES;
    }

    char *params_json = conduit_script_build_params_json(
        &req, jwt.claims, &err_code, &err_msg);
    if (!params_json) {
        free_jwt_claims(jwt.claims);
        unsigned int http_st = MHD_HTTP_BAD_REQUEST;
        if (err_code && strcmp(err_code, "params_too_large") == 0) {
            http_st = MHD_HTTP_CONTENT_TOO_LARGE;
        } else if (err_code && strcmp(err_code, "internal_error") == 0) {
            http_st = MHD_HTTP_INTERNAL_SERVER_ERROR;
        }
        enum MHD_Result r = send_script_error(connection, err_code, err_msg,
                                              http_st);
        json_decref(request_json);
        return r;
    }

    struct timespec t0;
    clock_gettime(CLOCK_REALTIME, &t0);

    log_this(SR_API,
             "conduit/script invoke start script=%s wait=%s timeout=%d",
             LOG_LEVEL_STATE, 3, req.script,
             req.wait ? "true" : "false", req.timeout_seconds);

    char *job_id = NULL;
    ScriptingInvokeError inv_err;
    if (g_submit_hook) {
        inv_err = g_submit_hook(req.script, params_json, NULL,
                                req.timeout_seconds, &job_id);
    } else {
        inv_err = scripting_submit_job_from_db(req.script, params_json, NULL,
                                               req.timeout_seconds, &job_id);
    }
    free(params_json);

    if (inv_err != SCRIPTING_INVOKE_OK || !job_id) {
        unsigned int http_st = MHD_HTTP_INTERNAL_SERVER_ERROR;
        const char *code = NULL;
        const char *msg = NULL;
        conduit_script_map_invoke_error(inv_err, &http_st, &code, &msg);
        log_this(SR_API,
                 "conduit/script invoke end script=%s status=error code=%s",
                 LOG_LEVEL_STATE, 2, req.script, code ? code : "unknown");
        free_jwt_claims(jwt.claims);
        free(job_id);
        enum MHD_Result r = send_script_error(connection, code, msg, http_st);
        json_decref(request_json);
        return r;
    }

    if (jwt.claims->sub && scripting_scoreboard) {
        (void)scoreboard_set_submitted_by(scripting_scoreboard, job_id,
                                          jwt.claims->sub);
    }

    if (!req.wait) {
        log_this(SR_API,
                 "conduit/script invoke end job_id=%s script=%s status=pending elapsed_ms=0",
                 LOG_LEVEL_STATE, 2, job_id, req.script);
        json_t *resp = conduit_script_build_job_response(
            "pending", job_id, req.script, NULL, 0);
        free(job_id);
        free_jwt_claims(jwt.claims);
        json_decref(request_json);
        if (!resp) {
            return send_script_error(connection, "internal_error",
                                     "Failed to build response",
                                     MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        return api_send_json_response(connection, resp, MHD_HTTP_ACCEPTED);
    }

    ScoreboardEntry *entry = NULL;
    ScriptingWaitResult wr;
    if (g_wait_hook) {
        wr = g_wait_hook(job_id, req.timeout_seconds, &entry);
    } else {
        wr = scripting_wait_job(job_id, req.timeout_seconds, &entry);
    }

    struct timespec t1;
    clock_gettime(CLOCK_REALTIME, &t1);
    long elapsed = (t1.tv_sec - t0.tv_sec) * 1000L
                 + (t1.tv_nsec - t0.tv_nsec) / 1000000L;
    if (entry) {
        long from_entry = conduit_script_elapsed_ms_from_entry(entry);
        if (from_entry > 0) {
            elapsed = from_entry;
        }
    }

    const char *status_str = conduit_script_wait_status_name(wr);
    if (wr == SCRIPTING_WAIT_NOT_FOUND || wr == SCRIPTING_WAIT_INTERNAL
        || wr == SCRIPTING_WAIT_SHUTDOWN) {
        log_this(SR_API,
                 "conduit/script invoke end job_id=%s script=%s status=%s elapsed_ms=%ld",
                 LOG_LEVEL_STATE, 4, job_id, req.script, status_str, elapsed);
        scoreboard_entry_free(entry);
        free(job_id);
        free_jwt_claims(jwt.claims);
        json_decref(request_json);
        if (wr == SCRIPTING_WAIT_SHUTDOWN) {
            return send_script_error(connection, "scripting_shutdown",
                                     "Scripting subsystem is shutting down",
                                     MHD_HTTP_SERVICE_UNAVAILABLE);
        }
        return send_script_error(
            connection,
            wr == SCRIPTING_WAIT_NOT_FOUND ? "job_not_found" : "internal_error",
            wr == SCRIPTING_WAIT_NOT_FOUND ? "Job not found after submit"
                                           : "Wait failed",
            wr == SCRIPTING_WAIT_NOT_FOUND ? MHD_HTTP_NOT_FOUND
                                           : MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    log_this(SR_API,
             "conduit/script invoke end job_id=%s script=%s status=%s elapsed_ms=%ld",
             LOG_LEVEL_STATE, 4, job_id, req.script, status_str, elapsed);

    json_t *resp = conduit_script_build_job_response(
        status_str, job_id, req.script, entry, elapsed);
    scoreboard_entry_free(entry);
    free(job_id);
    free_jwt_claims(jwt.claims);
    json_decref(request_json);

    if (!resp) {
        return send_script_error(connection, "internal_error",
                                 "Failed to build response",
                                 MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    /* Phase 0: job outcomes including failed/timeout use HTTP 200. */
    return api_send_json_response(connection, resp, MHD_HTTP_OK);
}

enum MHD_Result handle_conduit_script_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    const char *path) {
    (void)url;

    if (!connection || !method || !path) {
        return MHD_NO;
    }

    char *job_id = conduit_script_extract_job_id(path);
    if (job_id || strcmp(path, "conduit/script/") == 0) {
        if (strcmp(method, "GET") != 0) {
            free(job_id);
            return send_script_error(connection, "method_not_allowed",
                                     "Use GET for /api/conduit/script/{job_id}",
                                     MHD_HTTP_METHOD_NOT_ALLOWED);
        }
        /* job_id is NULL only for the exact "conduit/script/" path (no id
         * segment); handle_script_get maps that to a 400 missing_job_id
         * rather than a misleading 404. */
        enum MHD_Result r = handle_script_get(connection, job_id);
        free(job_id);
        return r;
    }

    if (strcmp(path, "conduit/script") != 0) {
        return send_script_error(connection, "not_found",
                                 "Endpoint not found",
                                 MHD_HTTP_NOT_FOUND);
    }

    if (strcmp(method, "POST") != 0) {
        return send_script_error(connection, "method_not_allowed",
                                 "Use POST for /api/conduit/script",
                                 MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    if (!upload_data_size || !con_cls) {
        return MHD_NO;
    }

    return handle_script_post(connection, upload_data, upload_data_size, con_cls);
}
