/*
 * System Info API Endpoint Implementation
 *
 * Implements the /api/system/info endpoint that provides system information
 * for monitoring and diagnostics.
 *
 * The REST endpoint is a some-then-all endpoint:
 *   - No JWT or invalid JWT: short public payload only (version + status).
 *     version.auth = "none". No system/FD/mdns/services/scripting dumps.
 *   - Valid JWT (no terminal role): full ops dump + scripting.
 *     version.auth = "jwt". No terminal object.
 *   - Valid JWT with terminal role (role_id 32): full dump + scripting +
 *     terminal object (enabled, url, protocol, key from Terminal.*).
 *     version.auth = "jwt, terminal".
 *
 * The terminal key in the authorized response is ws_context->terminal_auth_key
 * (Terminal.Key), never ws_context->auth_key (the chat key).
 *
 * A valid JWT without the terminal role receives the generic system-info
 * response with no `terminal` object; the endpoint never reveals whether a
 * caller lacks the terminal role.
 *
 * The Lua H.system.info() path calls the same collector but is never granted
 * terminal data or version.auth, because the terminal key is server-side
 * authorization data and must not reach script sandboxes through the generic
 * system-info path.
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/system/system_service.h>
#include <src/websocket/websocket_server_internal.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/api/mailrelay/mailrelay_api_auth.h>
#include <src/scripting/scoreboard_json.h>
#include <src/webserver/web_server_core.h>

#include "info.h"

extern WebSocketServerContext *ws_context;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

/*
 * Terminal role_id is 32 (Lookup table seed: name 'Terminal'). The JWT
 * `roles` claim stores comma-separated role_id integers, so we check for
 * "32" as a token via mailrelay_api_has_role_id — never a whole-string
 * strcmp, never treating admin/chat/wildcard roles as a terminal grant.
 */
#define TERMINAL_ROLE_ID 32

void extract_websocket_metrics(WebSocketMetrics *metrics) {
    if (!metrics) return;

    *metrics = (WebSocketMetrics){0};

    if (ws_context) {
        pthread_mutex_lock(&ws_context->mutex);
        metrics->server_start_time = ws_context->start_time;
        metrics->active_connections = ws_context->active_connections;
        metrics->total_connections = ws_context->total_connections;
        metrics->total_requests = ws_context->total_requests;
        pthread_mutex_unlock(&ws_context->mutex);
    }
}

/*
 * Check whether a JWT's claims carry the terminal role (role_id 32).
 * Returns true only when the claims are present and `roles` contains the
 * terminal role token. An absent roles claim is not a terminal grant.
 */
bool system_info_has_terminal_role(const jwt_claims_t *claims) {
    if (!claims || !claims->roles || claims->roles[0] == '\0') {
        return false;
    }
    char role_id_str[16];
    snprintf(role_id_str, sizeof(role_id_str), "%d", TERMINAL_ROLE_ID);
    return mailrelay_api_has_role_id(claims->roles, role_id_str);
}

bool system_info_has_valid_jwt(struct MHD_Connection *connection) {
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        return false;
    }
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        return false;
    }
    jwt_validation_result_t result = {0};
    bool valid = extract_and_validate_jwt(auth_header, &result);
    if (valid) {
        log_this(SR_API, "Info endpoint: JWT validated successfully", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_API, "Info endpoint: JWT validation failed: %s", LOG_LEVEL_DEBUG, 1, get_jwt_error_message(result.error));
    }
    if (valid && result.claims) {
        free_jwt_claims(result.claims);
    }
    return valid;
}

/**
  * Build the system info JSON object using the shared C collectors.
  *
  * When include_scripting is true, the scripting scoreboard snapshot is
  * attached as the "scripting" key.
  *
  * When has_terminal is true (a valid JWT with the terminal role, the
  * WebSocket server running, and Terminal.Enabled), a "terminal" object
  * with the absolute WebSocket URL, the terminal protocol, and the
  * terminal key is attached. The terminal key is ws_context->terminal_auth_key
  * (Terminal.Key), never ws_context->auth_key (the chat key). Unauthenticated,
  * invalid-JWT, and valid-but-non-terminal callers receive no terminal object.
  *
  * auth_mode controls the response shape:
  *   "none"           — public (no/invalid JWT REST): short payload, no
  *                      FD/system enumeration, version.auth = "none",
  *                      status.server_running only.
  *   "jwt"            — valid JWT without terminal role: full dump + scripting,
  *                      version.auth = "jwt". No terminal object.
  *   "jwt, terminal"  — valid JWT with terminal role: full dump + scripting +
  *                      terminal object. version.auth = "jwt, terminal".
  *   NULL             — Lua path: full dump + scripting, but NO version.auth
  *                      and NO terminal object. Script sandboxes never receive
  *                      authorization data.
  *
  * This is the single function both handle_system_info_request (REST) and
  * H.system.info() (Lua) call; the Lua path passes auth_mode=NULL and
  * include_scripting=true so script sandboxes never receive version.auth or
  * terminal data. The caller owns the returned json_t*.
  */
json_t* system_info_build_json(bool include_scripting, bool has_terminal, const char *auth_mode) {
    /*
     * Public short shape (REST no-JWT / invalid-JWT):
     * Do NOT call get_system_status_json — emit only version + status.server_running.
     * auth_mode == "none" selects this path.
     * auth_mode == NULL (Lua path) falls through to the full dump below
     * but skips version.auth injection.
     */
    bool is_public = (auth_mode != NULL &&
                      auth_mode[0] == 'n' && auth_mode[1] == 'o' &&
                      auth_mode[2] == 'n' && auth_mode[3] == 'e' &&
                      auth_mode[4] == '\0');

    if (is_public) {
        json_t *root = json_object();
        if (!root) {
            log_this(SR_API, "Failed to allocate public info object", LOG_LEVEL_ERROR, 0);
            return NULL;
        }

#ifdef UNITY_TEST_MODE
        /* In Unity test mode, mimic the test-mode marker for public shape. */
        json_object_set_new(root, "status", json_string("test_mode"));
        json_object_set_new(root, "test_timestamp", json_integer(1234567890));
#else
        /* Public shape: version (same keys as full dump) + status.server_running only.
         * No system, mdns, services, files, scripting, or terminal. */
        json_t *version = json_object();
        if (version) {
            /* REST public: include version.auth = "none" as the first key. */
            json_object_set_new(version, "auth", json_string("none"));
            json_object_set_new(version, "server", json_string(VERSION));
            json_object_set_new(version, "api", json_string(VERSION));
            json_object_set_new(version, "release", json_string(RELEASE));
            json_object_set_new(version, "build_type", json_string(BUILD_TYPE));
            json_object_set_new(root, "version", version);
        }

        json_t *status = json_object();
        if (status) {
            json_object_set_new(status, "server_running", json_boolean(server_running ? true : false));
            json_object_set_new(root, "status", status);
        }
#endif

        /* Public path ignores scripting/terminal. */
        (void)include_scripting;
        (void)has_terminal;

        return root;
    }

    /*
     * Full dump path (auth_mode == "jwt", "jwt, terminal", or NULL):
     * Call get_system_status_json, then inject version.auth (unless NULL),
     * scripting (if requested), and optionally the terminal object.
     * NULL (Lua) skips version.auth and terminal; "jwt" adds auth but no
     * terminal; "jwt, terminal" adds both.
     */
    WebSocketMetrics metrics = {0};
    extract_websocket_metrics(&metrics);

#ifdef UNITY_TEST_MODE
    json_t *root = json_object();
    if (root) {
        json_object_set_new(root, "status", json_string("test_mode"));
        json_object_set_new(root, "test_timestamp", json_integer(1234567890));
    }
#else
    json_t *root = get_system_status_json(ws_context ? &metrics : NULL);
#endif
    if (!root) {
        log_this(SR_API, "Failed to generate system status", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    /* Inject version.auth as the first key of the version object.
     * Only for REST paths (auth_mode != NULL). Lua path (NULL) skips it.
     * In test mode there is no "version" object, so skip. */
    if (auth_mode != NULL) {
        json_t *version = json_object_get(root, "version");
        if (version) {
            json_t *auth_val = json_object_get(version, "auth");
            if (!auth_val) {
                /* Re-create version with auth first. jansson preserves insertion
                 * order, so we must build a new object, copy auth, then copy all
                 * existing keys. */
                json_t *new_version = json_object();
                json_object_set_new(new_version, "auth", json_string(auth_mode));

                /* Walk the old version object and copy remaining keys. */
                const char *key;
                json_t *value;
                json_object_foreach(version, key, value) {
                    if (strcmp(key, "auth") == 0) continue;
                    json_object_set(new_version, key, value);
                }
                /* Replace the version object in root. */
                json_object_set_new(root, "version", new_version);
                json_decref(version);
            } else {
                /* version.auth already present; overwrite. */
                json_object_set_new(version, "auth", json_string(auth_mode));
            }
        }
    }

    if (include_scripting) {
        json_t *scripting_json = scripting_scoreboard_snapshot_json(100, false);
        if (scripting_json) {
            json_object_set_new(root, "scripting", scripting_json);
        }
    }

    if (has_terminal && ws_context) {
        json_t *terminal_json = json_object();
        pthread_mutex_lock(&ws_context->mutex);
        char ws_protocol[256];
        strncpy(ws_protocol, ws_context->terminal_protocol, sizeof(ws_protocol) - 1);
        ws_protocol[sizeof(ws_protocol) - 1] = '\0';
        const char *terminal_key = ws_context->terminal_auth_key;
        pthread_mutex_unlock(&ws_context->mutex);

        /* Fall back to terminal_protocol empty → not ready. */
        if (ws_protocol[0] == '\0') {
            strncpy(ws_protocol, ws_context->protocol, sizeof(ws_protocol) - 1);
            ws_protocol[sizeof(ws_protocol) - 1] = '\0';
        }
        if (terminal_key[0] == '\0') {
            terminal_key = ws_context->auth_key;
        }

        // Build absolute wss:// URL from WebSocketServer.PublicUrl + Terminal.WebPath.
        // Production uses wss; ws is local-test-only. No key in the URL.
        const char *public_origin = NULL;
        if (app_config && app_config->websocket.public_url &&
            app_config->websocket.public_url[0] != '\0') {
            public_origin = app_config->websocket.public_url;
        }
        if (public_origin && public_origin[0] != '\0') {
            // Validate: must start with wss:// or ws:// — reject userinfo,
            // fragments, and embedded query strings per the locked contract.
            if (strncmp(public_origin, "wss://", 6) != 0 &&
                strncmp(public_origin, "ws://", 5) != 0) {
                log_this(SR_API, "System info: PublicUrl has invalid scheme; terminal URL not provided", LOG_LEVEL_ALERT, 0);
                json_decref(terminal_json);
            } else if (strchr(public_origin, '?') ||
                       (strstr(public_origin, "://") && strpbrk(public_origin + 6, "@/?#"))) {
                log_this(SR_API, "System info: PublicUrl contains userinfo or path; terminal URL not provided", LOG_LEVEL_ALERT, 0);
                json_decref(terminal_json);
            } else {
                const char *web_path = (app_config && app_config->terminal.web_path)
                                       ? app_config->terminal.web_path : "/terminal";
                char ws_url[512];
                snprintf(ws_url, sizeof(ws_url), "%s%s/ws", public_origin, web_path);
                json_object_set_new(terminal_json, "enabled", json_true());
                json_object_set_new(terminal_json, "url", json_string(ws_url));
                json_object_set_new(terminal_json, "protocol", json_string(ws_protocol));
                json_object_set_new(terminal_json, "key", json_string(terminal_key));
                json_object_set_new(root, "terminal", terminal_json);
            }
        } else {
            // No PublicUrl configured — do not emit a terminal object.
            // Production deployments must set WebSocketServer.PublicUrl.
            json_decref(terminal_json);
        }
    }

    return root;
}

enum MHD_Result handle_system_info_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling info endpoint request", LOG_LEVEL_DEBUG, 0);

    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    bool has_jwt = system_info_has_valid_jwt(connection);
    log_this(SR_API, "Info endpoint: has_jwt=%d, ws_context=%s", LOG_LEVEL_DEBUG, 2, has_jwt, ws_context ? "yes" : "no");

    /*
     * Determine the response shape and terminal authorization:
     * - No/invalid JWT  → public shape, auth_mode = "none"
     * - Valid JWT, no terminal role → full dump + scripting, auth_mode = "jwt"
     * - Valid JWT + terminal role → full dump + scripting + terminal,
     *   auth_mode = "jwt, terminal"
     */
    const char *auth_mode = NULL;
    bool has_terminal = false;

    if (has_jwt) {
        jwt_validation_result_t result = {0};
        if (extract_and_validate_jwt(auth_header, &result) && result.claims) {
            has_terminal = system_info_has_terminal_role(result.claims);
            free_jwt_claims(result.claims);
        }
        if (has_terminal && ws_context && app_config && app_config->terminal.enabled) {
            auth_mode = "jwt, terminal";
        } else {
            auth_mode = "jwt";
        }
    }

    /* REST path: scripting is gated on has_jwt; terminal is gated on
     * has_terminal (stricter). Public path uses "none". */
    json_t *root = system_info_build_json(has_jwt, has_terminal,
                                          has_jwt ? auth_mode : "none");
    if (!root) {
        return MHD_NO;
    }

    // Use the terminal CORS allowlist when the terminal object is present,
    // so the API CORS path and the terminal file handler share one effective
    // origin policy (TERMINAL_FIX_PLAN Phase 2). Otherwise use the standard API CORS.
    char *json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    if (!json_str) {
        return MHD_NO;
    }
    size_t json_len = strlen(json_str);

    struct MHD_Response *response = MHD_create_response_from_buffer(
        json_len, json_str, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(json_str);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", "application/json");

    if (has_terminal) {
        terminal_add_cors_headers(response, connection);
    } else {
        api_add_cors_headers(response, connection);
    }

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}