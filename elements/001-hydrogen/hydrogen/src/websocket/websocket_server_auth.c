/*
 * WebSocket Authentication Handler
 *
 * Implements connection authentication using a key-based scheme:
 * - Validates authentication headers
 * - Manages session authentication state
 * - Provides security logging
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_internal.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_handle_authentication(struct lws *wsi, WebSocketSessionData *session, const char *auth_header)
{
    if (!session || !auth_header || !ws_context) {
        log_this(SR_WEBSOCKET, "Invalid authentication parameters", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    // Already authenticated
    if (session->authenticated) {
        return 0;
    }

    // Check if the authorization scheme is correct
    if (strncmp(auth_header, HYDROGEN_AUTH_SCHEME " ", strlen(HYDROGEN_AUTH_SCHEME) + 1) != 0) {
        log_this(SR_WEBSOCKET, "Invalid authentication scheme", LOG_LEVEL_ALERT, 0);
        return -1;
    }

    // Extract and validate the key
    const char *key = auth_header + strlen(HYDROGEN_AUTH_SCHEME) + 1;
    
    // Update client info before validation for better logging
    ws_update_client_info(wsi, session);
    
    if (!ws_auth_accept_key(wsi, key, true)) {
        log_this(SR_WEBSOCKET, "Authentication failed for client %s (%s)", LOG_LEVEL_ALERT, 2, session->request_ip, session->request_app);
        return -1;
    }

    // Authentication successful
    session->authenticated = true;
    log_this(SR_WEBSOCKET, "Client authenticated successfully: %s (%s)", LOG_LEVEL_STATE, 2 ,session->request_ip, session->request_app);

    return 0;
}

// Helper function to check if a session is authenticated
bool ws_is_authenticated(const WebSocketSessionData *session)
{
    return session && session->authenticated;
}

// Helper function to clear authentication state
void ws_clear_authentication(WebSocketSessionData *session)
{
    if (session) {
        session->authenticated = false;
        if (session->authenticated_key) {
            free(session->authenticated_key);
            session->authenticated_key = NULL;
        }
    }
}

bool ws_extract_query_auth_key(struct lws *wsi, char *out, size_t out_len)
{
    char source[512];
    int pass;

    if (out && out_len > 0U) {
        out[0] = '\0';
    }
    if (!wsi || !out || out_len < 2U) {
        return false;
    }

    for (pass = 0; pass < 2; pass++) {
        const char *query = NULL;
        const char *cursor;
        const char *value = NULL;
        size_t value_len = 0U;
        bool found = false;
        size_t decoded_len = 0U;
        size_t i = 0U;

        if (pass == 0) {
            int args_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_URI_ARGS);
            if (args_len >= (int)sizeof(source)) {
                return false;
            }
            if (args_len > 0 && lws_hdr_copy(wsi, source, (int)sizeof(source), WSI_TOKEN_HTTP_URI_ARGS) > 0) {
                query = source;
            } else {
                continue;
            }
        } else {
            int uri_len = lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI);
            char *qmark;

            if (uri_len <= 0 || uri_len >= (int)sizeof(source)) {
                continue;
            }
            if (lws_hdr_copy(wsi, source, (int)sizeof(source), WSI_TOKEN_GET_URI) <= 0) {
                continue;
            }
            qmark = strchr(source, '?');
            if (!qmark || qmark[1] == '\0') {
                continue;
            }
            query = qmark + 1;
        }

        cursor = query;
        while (cursor[0] != '\0') {
            if (strncmp(cursor, "key=", 4) == 0) {
                value = cursor + 4;
                const char *amp = strchr(value, '&');
                value_len = amp ? (size_t)(amp - value) : strlen(value);
                found = true;
                break;
            }
            const char *amp = strchr(cursor, '&');
            if (!amp) {
                break;
            }
            cursor = amp + 1;
        }

        if (!found) {
            continue;
        }
        if (value_len == 0U) {
            return false;
        }

        while (i < value_len) {
            char ch;

            if (decoded_len >= out_len - 1U) {
                out[0] = '\0';
                return false;
            }

            ch = value[i];
            if (ch == '%' && (i + 2U) < value_len) {
                char hex_hi = value[i + 1U];
                char hex_lo = value[i + 2U];
                int hi = -1;
                int lo = -1;

                if (hex_hi >= '0' && hex_hi <= '9') {
                    hi = hex_hi - '0';
                } else if (hex_hi >= 'a' && hex_hi <= 'f') {
                    hi = hex_hi - 'a' + 10;
                } else if (hex_hi >= 'A' && hex_hi <= 'F') {
                    hi = hex_hi - 'A' + 10;
                }

                if (hex_lo >= '0' && hex_lo <= '9') {
                    lo = hex_lo - '0';
                } else if (hex_lo >= 'a' && hex_lo <= 'f') {
                    lo = hex_lo - 'a' + 10;
                } else if (hex_lo >= 'A' && hex_lo <= 'F') {
                    lo = hex_lo - 'A' + 10;
                }

                if (hi >= 0 && lo >= 0) {
                    int decoded = (hi << 4) | lo;
                    if (decoded == 0) {
                        out[0] = '\0';
                        return false;
                    }
                    out[decoded_len] = (char)decoded;
                    decoded_len++;
                    i += 3U;
                    continue;
                }
            }

            out[decoded_len] = ch;
            decoded_len++;
            i++;
        }

        out[decoded_len] = '\0';
        return decoded_len > 0U;
    }

    return false;
}

bool ws_copy_request_path(struct lws *wsi, char *out, size_t out_len)
{
    int uri_len;
    char *qmark;

    if (!wsi || !out || out_len < 2U) {
        return false;
    }
    out[0] = '\0';

    uri_len = lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI);
    if (uri_len <= 0 || uri_len >= (int)out_len) {
        return false;
    }
    if (lws_hdr_copy(wsi, out, (int)out_len, WSI_TOKEN_GET_URI) <= 0) {
        out[0] = '\0';
        return false;
    }
    qmark = strchr(out, '?');
    if (qmark) {
        *qmark = '\0';
    }
    return out[0] != '\0';
}

int ws_auth_surface_from_path(struct lws *wsi)
{
    char path[256];
    char term_path[256];
    const char *web_path = "/terminal";
    size_t n;

    if (!ws_copy_request_path(wsi, path, sizeof(path))) {
        return 0;
    }
    if (strcmp(path, "/wss") == 0) {
        return 1;
    }

    if (app_config && app_config->terminal.web_path && app_config->terminal.web_path[0]) {
        web_path = app_config->terminal.web_path;
    }
    n = strlen(web_path);
    while (n > 1U && web_path[n - 1U] == '/') {
        n--;
    }
    if (n >= sizeof(term_path) - 4U) {
        return 0;
    }
    memcpy(term_path, web_path, n);
    memcpy(term_path + n, "/ws", 4);
    if (strcmp(path, term_path) == 0) {
        if (ws_context && ws_context->terminal_auth_key[0] != '\0') {
            return 2;
        }
        return 0;
    }
    return 0;
}

int ws_auth_surface_from_protocol(struct lws *wsi)
{
    const struct lws_protocols *protocol;

    if (!wsi || !ws_context) {
        return 0;
    }
    protocol = lws_get_protocol(wsi);
    if (!protocol || !protocol->name) {
        return 0;
    }
    if (ws_context->protocol[0] != '\0' && strcmp(protocol->name, ws_context->protocol) == 0) {
        return 1;
    }
    if (ws_context->terminal_protocol[0] != '\0' &&
        strcmp(protocol->name, ws_context->terminal_protocol) == 0) {
        return 2;
    }
    return 0;
}

bool ws_auth_accept_key(struct lws *wsi, const char *presented, bool require_protocol_match)
{
    int path_surface;
    const char *expected;

    if (!wsi || !presented || presented[0] == '\0' || !ws_context) {
        return false;
    }

    path_surface = ws_auth_surface_from_path(wsi);
    if (path_surface == 0) {
        return false;
    }
    if (require_protocol_match) {
        int proto_surface = ws_auth_surface_from_protocol(wsi);
        if (proto_surface == 0 || proto_surface != path_surface) {
            return false;
        }
    }

    expected = (path_surface == 2) ? ws_context->terminal_auth_key : ws_context->auth_key;
    if (!expected || expected[0] == '\0') {
        return false;
    }
    return strcmp(presented, expected) == 0;
}
