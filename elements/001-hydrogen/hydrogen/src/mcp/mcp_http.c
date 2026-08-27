#include <src/hydrogen.h>
#include <src/mcp/mcp_http.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_prm.h>
#include <src/mcp/mcp_dispatch.h>
#include <src/mcp/mcp_rpc.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

extern AppConfig *app_config;

struct MHD_Daemon *mcp_daemon = NULL;

static const MCPConfig *mcp_http_cfg = NULL;
static int mcp_listen_pool_size = 0;
static struct sockaddr_storage mcp_bind_addr;
static char mcp_con_seen;

static const char MCP_HEALTHZ_BODY[] = "{\"status\":\"ok\"}";

static void mcp_request_completed(void *cls, struct MHD_Connection *connection,
                                  void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls;
    (void)connection;
    (void)toe;
    mcp_http_upload_free(con_cls);
}

static enum MHD_Result mcp_queue_static(struct MHD_Connection *connection,
                                        unsigned int status,
                                        const char *body,
                                        const char *content_type) {
    struct MHD_Response *response;
    enum MHD_Result queued;
    size_t len = body ? strlen(body) : 0;

    response = MHD_create_response_from_buffer(len, (void *)(body ? body : ""),
                                               MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }
    if (content_type) {
        MHD_add_response_header(response, "Content-Type", content_type);
    }
    queued = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return queued;
}

static enum MHD_Result mcp_queue_owned(struct MHD_Connection *connection,
                                       unsigned int status,
                                       char *body) {
    struct MHD_Response *response;
    enum MHD_Result queued;
    size_t len = body ? strlen(body) : 0;

    response = MHD_create_response_from_buffer(len, body, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(body);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", "application/json");
    queued = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return queued;
}

bool mcp_http_upload_is(const void *con_cls) {
    const McpHttpUpload *upload = (const McpHttpUpload *)con_cls;
    return upload && upload->magic == MCP_UPLOAD_MAGIC;
}

McpHttpUpload *mcp_http_upload_new(void) {
    McpHttpUpload *upload = calloc(1, sizeof(*upload));
    if (upload) {
        upload->magic = MCP_UPLOAD_MAGIC;
    }
    return upload;
}

bool mcp_http_upload_append(McpHttpUpload *upload, const char *data, size_t len, int max_body) {
    char *grown;
    size_t need;
    int cap;

    if (!upload) {
        return false;
    }
    if (len == 0) {
        return true;
    }
    cap = max_body > 0 ? max_body : 1048576;
    if (upload->size + len > (size_t)cap) {
        upload->overflow = true;
        return false;
    }
    need = upload->size + len + 1;
    if (need > upload->capacity) {
        size_t next = upload->capacity == 0 ? 4096 : upload->capacity * 2;
        while (next < need) {
            next *= 2;
        }
        grown = realloc(upload->data, next);
        if (!grown) {
            upload->overflow = true;
            return false;
        }
        upload->data = grown;
        upload->capacity = next;
    }
    memcpy(upload->data + upload->size, data, len);
    upload->size += len;
    upload->data[upload->size] = '\0';
    return true;
}

void mcp_http_upload_free(void **con_cls) {
    McpHttpUpload *upload;

    if (!con_cls || !mcp_http_upload_is(*con_cls)) {
        return;
    }
    upload = (McpHttpUpload *)*con_cls;
    free(upload->data);
    free(upload);
    *con_cls = NULL;
}

enum MHD_Result mcp_queue_rpc_response(struct MHD_Connection *connection,
                                       unsigned int status,
                                       char *body,
                                       bool take_ownership,
                                       const char *session_id) {
    struct MHD_Response *response;
    enum MHD_Result queued;
    size_t len = body ? strlen(body) : 0;
    enum MHD_ResponseMemoryMode mode;

    mode = take_ownership ? MHD_RESPMEM_MUST_FREE : MHD_RESPMEM_PERSISTENT;
    if (take_ownership && !body) {
        body = strdup("");
        if (!body) {
            return MHD_NO;
        }
        len = 0;
    }
    response = MHD_create_response_from_buffer(len, take_ownership ? body : (void *)(body ? body : ""),
                                               mode);
    if (!response) {
        if (take_ownership) {
            free(body);
        }
        return MHD_NO;
    }
    if (len > 0) {
        MHD_add_response_header(response, "Content-Type", "application/json");
    }
    if (session_id && session_id[0] != '\0') {
        MHD_add_response_header(response, "Mcp-Session-Id", session_id);
    }
    queued = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return queued;
}

enum MHD_Result mcp_http_handle_delete(struct MHD_Connection *connection,
                                       const MCPConfig *cfg,
                                       const McpAuthResult *auth) {
    const char *session_hdr;
    McpSessionResult result;

    (void)cfg;
    session_hdr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Mcp-Session-Id");
    result = mcp_session_delete(session_hdr, auth ? auth->sub : NULL);
    if (result == MCP_SESSION_HIJACK) {
        return mcp_send_unauthorized(connection, cfg);
    }
    if (result != MCP_SESSION_DELETED) {
        return mcp_queue_static(connection, MHD_HTTP_NOT_FOUND, "{\"error\":\"session not found\"}",
                                "application/json");
    }
    return mcp_queue_rpc_response(connection, MHD_HTTP_NO_CONTENT, NULL, false, NULL);
}

enum MHD_Result mcp_http_handle_post(struct MHD_Connection *connection,
                                     const MCPConfig *cfg,
                                     const McpAuthResult *auth,
                                     const char *body,
                                     size_t body_len) {
    McpRpcEnvelope env;
    McpRpcStatus parsed;
    McpSessionResult session_st;
    const char *proto_hdr;
    const char *session_hdr;
    char *session_id = NULL;
    char *err_body;
    enum MHD_Result queued;
    bool allow_create;

    mcp_stats_inc_rpc_received();
    mcp_stats_add_bytes_in((unsigned long long)body_len);
    mcp_stats_touch_rpc();

    proto_hdr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "MCP-Protocol-Version");
    parsed = mcp_rpc_parse(body, body_len, cfg ? cfg->MaxBodyBytes : 1048576, proto_hdr, &env);
    if (parsed != MCP_RPC_OK) {
        mcp_stats_inc_rpc_failed();
        err_body = mcp_rpc_make_error(NULL, mcp_rpc_status_code(parsed),
                                      mcp_rpc_status_message(parsed));
        if (!err_body) {
            return MHD_NO;
        }
        mcp_stats_add_bytes_out((unsigned long long)strlen(err_body));
        return mcp_queue_rpc_response(connection, MHD_HTTP_BAD_REQUEST, err_body, true, NULL);
    }

    session_hdr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Mcp-Session-Id");
    allow_create = (session_hdr == NULL || session_hdr[0] == '\0') || mcp_rpc_is_initialize(&env);
    session_st = mcp_session_resolve(session_hdr, auth ? auth->sub : NULL, allow_create,
                                     cfg ? cfg->MaxSessions : 256,
                                     cfg ? cfg->SessionIdleTimeoutSeconds : 900,
                                     &session_id);
    if (session_st == MCP_SESSION_HIJACK) {
        mcp_rpc_envelope_cleanup(&env);
        free(session_id);
        mcp_stats_inc_rpc_failed();
        return mcp_send_unauthorized(connection, cfg);
    }
    if (session_st == MCP_SESSION_UNKNOWN) {
        mcp_rpc_envelope_cleanup(&env);
        free(session_id);
        mcp_stats_inc_rpc_failed();
        return mcp_queue_static(connection, MHD_HTTP_NOT_FOUND, "{\"error\":\"session not found\"}",
                                "application/json");
    }
    if (session_st == MCP_SESSION_LIMIT) {
        mcp_stats_inc_rpc_failed();
        err_body = mcp_rpc_make_error(env.id, MCP_RPC_SESSION_LIMIT, "Session limit exceeded");
        mcp_rpc_envelope_cleanup(&env);
        free(session_id);
        if (!err_body) {
            return MHD_NO;
        }
        mcp_stats_add_bytes_out((unsigned long long)strlen(err_body));
        return mcp_queue_rpc_response(connection, MHD_HTTP_OK, err_body, true, NULL);
    }

    if (env.is_notification) {
        mcp_stats_inc_rpc_succeeded();
        queued = mcp_queue_rpc_response(connection, MHD_HTTP_ACCEPTED, NULL, false, session_id);
        mcp_rpc_envelope_cleanup(&env);
        free(session_id);
        return queued;
    }

    queued = mcp_dispatch_submit_protocol(connection, cfg, auth, &env, session_id);
    mcp_rpc_envelope_cleanup(&env);
    free(session_id);
    return queued;
}

bool mcp_url_is_path(const char *url, const char *path) {
    return url && path && strcmp(url, path) == 0;
}

bool mcp_url_is_healthz(const char *url, const char *path) {
    char expected[512];

    if (!url || !path) {
        return false;
    }
    if (snprintf(expected, sizeof(expected), "%s/healthz", path) >= (int)sizeof(expected)) {
        return false;
    }
    return strcmp(url, expected) == 0;
}

bool mcp_url_is_prm(const char *url, const char *path) {
    char with_path[512];

    if (!url) {
        return false;
    }
    if (strcmp(url, "/.well-known/oauth-protected-resource") == 0) {
        return true;
    }
    if (!path) {
        return false;
    }
    if (snprintf(with_path, sizeof(with_path),
                 "/.well-known/oauth-protected-resource%s", path) >= (int)sizeof(with_path)) {
        return false;
    }
    return strcmp(url, with_path) == 0;
}

bool mcp_origin_allowed(const char *origin, const MCPConfig *cfg) {
    size_t i;

    if (!origin || origin[0] == '\0') {
        return true;
    }
    if (!cfg) {
        return false;
    }
    for (i = 0; i < cfg->AllowedOriginCount; i++) {
        if (cfg->AllowedOrigins[i] && strcmp(cfg->AllowedOrigins[i], origin) == 0) {
            return true;
        }
    }
    return false;
}

enum MHD_Result mcp_send_prm(struct MHD_Connection *connection, const MCPConfig *cfg) {
    char *body = mcp_prm_build(cfg, app_config);
    if (!body) {
        return MHD_NO;
    }
    return mcp_queue_owned(connection, MHD_HTTP_OK, body);
}

bool mcp_fill_bind_addr(const MCPConfig *cfg, struct sockaddr_storage *out) {
    struct sockaddr_in6 *in6;
    struct sockaddr_in *in4;

    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    if (!cfg || !cfg->Interface) {
        return false;
    }

    if (strchr(cfg->Interface, ':')) {
        in6 = (struct sockaddr_in6 *)out;
        in6->sin6_family = AF_INET6;
        in6->sin6_port = htons((uint16_t)cfg->Port);
        return inet_pton(AF_INET6, cfg->Interface, &in6->sin6_addr) == 1;
    }

    in4 = (struct sockaddr_in *)out;
    in4->sin_family = AF_INET;
    in4->sin_port = htons((uint16_t)cfg->Port);
    return inet_pton(AF_INET, cfg->Interface, &in4->sin_addr) == 1;
}

bool mcp_start_listen(const MCPConfig *cfg) {
    unsigned int flags;
    unsigned int pool;
    const union MHD_DaemonInfo *info;

    if (!cfg) {
        log_this(SR_MCP, "Cannot listen: NULL MCP config", LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (mcp_daemon != NULL) {
        log_this(SR_MCP, "MCP daemon already listening", LOG_LEVEL_ALERT, 0);
        return false;
    }
    if (!mcp_fill_bind_addr(cfg, &mcp_bind_addr)) {
        log_this(SR_MCP, "Invalid MCP.Interface %s", LOG_LEVEL_ERROR, 1, cfg->Interface ? cfg->Interface : "(null)");
        return false;
    }

    pool = cfg->ThreadPoolSize > 0 ? (unsigned int)cfg->ThreadPoolSize : 1U;
    flags = MHD_USE_INTERNAL_POLLING_THREAD;
    flags |= MHD_USE_SELECT_INTERNALLY;
    flags |= MHD_ALLOW_SUSPEND_RESUME;
    if (mcp_bind_addr.ss_family == AF_INET6) {
        flags |= MHD_USE_DUAL_STACK;
    }

    mcp_http_cfg = cfg;
    mcp_daemon = MHD_start_daemon(flags | MHD_USE_DEBUG | MHD_USE_ERROR_LOG,
                                  (uint16_t)cfg->Port,
                                  NULL, NULL,
                                  &mcp_handle_request, NULL,
                                  MHD_OPTION_SOCK_ADDR, &mcp_bind_addr,
                                  MHD_OPTION_THREAD_POOL_SIZE, pool,
                                  MHD_OPTION_NOTIFY_COMPLETED, mcp_request_completed, NULL,
                                  MHD_OPTION_LISTENING_ADDRESS_REUSE, 1,
                                  MHD_OPTION_END);
    if (mcp_daemon == NULL) {
        log_this(SR_MCP, "Failed to start MCP daemon on %s:%d (port may be in use)",
                 LOG_LEVEL_ERROR, 2, cfg->Interface, cfg->Port);
        mcp_http_cfg = NULL;
        return false;
    }

    info = MHD_get_daemon_info(mcp_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL || info->port == 0) {
        log_this(SR_MCP, "MCP daemon failed to bind %s:%d", LOG_LEVEL_ERROR, 2,
                 cfg->Interface, cfg->Port);
        MHD_stop_daemon(mcp_daemon);
        mcp_daemon = NULL;
        mcp_http_cfg = NULL;
        return false;
    }

    mcp_listen_pool_size = (int)pool;
    log_this(SR_MCP, "MCP listening on %s:%d path %s (pool %d)", LOG_LEVEL_STATE, 4,
             cfg->Interface, cfg->Port, cfg->Path ? cfg->Path : "/mcp", mcp_listen_pool_size);
    return true;
}

void mcp_stop_listen(void) {
    if (mcp_daemon != NULL) {
        MHD_stop_daemon(mcp_daemon);
        mcp_daemon = NULL;
        log_this(SR_MCP, "MCP daemon stopped", LOG_LEVEL_STATE, 0);
    }
    mcp_http_cfg = NULL;
    mcp_listen_pool_size = 0;
}

bool mcp_is_listening(void) {
    return mcp_daemon != NULL;
}

int mcp_http_thread_pool_size(void) {
    return mcp_listen_pool_size;
}

enum MHD_Result mcp_handle_request(void *cls,
                                   struct MHD_Connection *connection,
                                   const char *url,
                                   const char *method,
                                   const char *version,
                                   const char *upload_data,
                                   size_t *upload_data_size,
                                   void **con_cls) {
    const MCPConfig *cfg;
    bool is_post;
    bool is_delete;
    bool is_get;

    (void)cls;
    (void)version;

    if (con_cls && *con_cls == NULL) {
        McpHttpUpload *created = mcp_http_upload_new();
        *con_cls = created ? (void *)created : (void *)&mcp_con_seen;
        return MHD_YES;
    }
    if (upload_data_size && *upload_data_size > 0) {
        const MCPConfig *append_cfg = mcp_http_cfg;
        if (!append_cfg && app_config) {
            append_cfg = &app_config->mcp;
        }
        if (mcp_http_upload_is(con_cls ? *con_cls : NULL)) {
            mcp_http_upload_append((McpHttpUpload *)*con_cls, upload_data, *upload_data_size,
                                   append_cfg ? append_cfg->MaxBodyBytes : 1048576);
        }
        *upload_data_size = 0;
        return MHD_YES;
    }

    cfg = mcp_http_cfg;
    if (!cfg && app_config) {
        cfg = &app_config->mcp;
    }

    is_get = method && strcmp(method, "GET") == 0;
    is_post = method && strcmp(method, "POST") == 0;
    is_delete = method && strcmp(method, "DELETE") == 0;

    if (cfg && mcp_url_is_healthz(url, cfg->Path) && is_get) {
        return mcp_queue_static(connection, MHD_HTTP_OK, MCP_HEALTHZ_BODY, "application/json");
    }

    if (is_get && mcp_url_is_prm(url, cfg ? cfg->Path : NULL)) {
        return mcp_send_prm(connection, cfg);
    }

    if (!cfg || !mcp_url_is_path(url, cfg->Path)) {
        return mcp_queue_static(connection, MHD_HTTP_NOT_FOUND, "{\"error\":\"not found\"}",
                                "application/json");
    }

    if (is_post || is_delete) {
        McpAuthResult auth;
        const char *origin = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Origin");
        const char *authorization;
        const char *body = NULL;
        size_t body_len = 0;
        enum MHD_Result result;

        if (mcp_http_upload_is(con_cls ? *con_cls : NULL)) {
            McpHttpUpload *upload = (McpHttpUpload *)*con_cls;
            if (upload->overflow) {
                char *err_body = mcp_rpc_make_error(NULL, MCP_RPC_PARSE_ERROR, "Parse error");
                mcp_stats_inc_rpc_received();
                mcp_stats_inc_rpc_failed();
                if (!err_body) {
                    return MHD_NO;
                }
                return mcp_queue_rpc_response(connection, MHD_HTTP_BAD_REQUEST, err_body, true, NULL);
            }
            body = upload->data;
            body_len = upload->size;
        }

        if (!mcp_origin_allowed(origin, cfg)) {
            mcp_stats_inc_origin_rejected();
            return mcp_queue_static(connection, MHD_HTTP_FORBIDDEN, "{\"error\":\"origin rejected\"}",
                                    "application/json");
        }
        authorization = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
        memset(&auth, 0, sizeof(auth));
        if (!mcp_validate_bearer(authorization, body, body_len, cfg, app_config, &auth)) {
            mcp_auth_result_cleanup(&auth);
            return mcp_send_unauthorized(connection, cfg);
        }
        if (is_delete) {
            result = mcp_http_handle_delete(connection, cfg, &auth);
            mcp_auth_result_cleanup(&auth);
            return result;
        }
        result = mcp_http_handle_post(connection, cfg, &auth, body, body_len);
        mcp_auth_result_cleanup(&auth);
        return result;
    }

    return mcp_queue_static(connection, MHD_HTTP_METHOD_NOT_ALLOWED, "{\"error\":\"method not allowed\"}",
                            "application/json");
}
