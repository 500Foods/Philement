#include <src/hydrogen.h>
#include <src/mcp/mcp_dispatch.h>
#include <src/mcp/mcp_http.h>
#include <src/mcp/mcp_stats.h>
#include <src/scripting/orchestrator.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting_invoke.h>
#include <src/scripting/worker_pool.h>

#include <pthread.h>
#include <string.h>

extern AppConfig *app_config;

pthread_mutex_t mcp_dispatch_suspend_lock = PTHREAD_MUTEX_INITIALIZER;

McpDispatchSubmitFn mcp_dispatch_submit_hook = NULL;
McpDispatchWaitFn mcp_dispatch_wait_hook = NULL;
McpDispatchLoadSourceFn mcp_dispatch_load_source_hook = NULL;
char *mcp_dispatch_protocol_source = NULL;

static const char MCP_DISPATCH_NOT_FOUND_BODY[] = "{\"error\":\"not found\"}";

void mcp_dispatch_set_submit_hook(McpDispatchSubmitFn fn) {
    mcp_dispatch_submit_hook = fn;
}

void mcp_dispatch_set_wait_hook(McpDispatchWaitFn fn) {
    mcp_dispatch_wait_hook = fn;
}

void mcp_dispatch_set_load_source_hook(McpDispatchLoadSourceFn fn) {
    mcp_dispatch_load_source_hook = fn;
}

void mcp_dispatch_set_protocol_source(const char *source) {
    free(mcp_dispatch_protocol_source);
    mcp_dispatch_protocol_source = source ? strdup(source) : NULL;
}

void mcp_dispatch_clear_hooks(void) {
    mcp_dispatch_submit_hook = NULL;
    mcp_dispatch_wait_hook = NULL;
    mcp_dispatch_load_source_hook = NULL;
    free(mcp_dispatch_protocol_source);
    mcp_dispatch_protocol_source = NULL;
}

char *mcp_dispatch_load_protocol_source(const MCPConfig *cfg) {
    const char *script_name;
    const char *database;
    char *group;
    char *script;
    char *source;
    int timeout;

    if (mcp_dispatch_protocol_source) {
        return strdup(mcp_dispatch_protocol_source);
    }

    script_name = (cfg && cfg->Protocol) ? cfg->Protocol : NULL;
    if (mcp_dispatch_load_source_hook) {
        return mcp_dispatch_load_source_hook(script_name);
    }

    if (!script_name || script_name[0] == '\0') {
        return NULL;
    }
    if (!scripting_invoke_parse_script_name(script_name, &group, &script)) {
        return NULL;
    }

    database = NULL;
    if (cfg && cfg->Database && cfg->Database[0] != '\0') {
        database = cfg->Database;
    } else {
        database = orchestrator_resolve_database();
    }
    if (!database || database[0] == '\0') {
        free(group);
        free(script);
        return NULL;
    }

    timeout = (cfg && cfg->RequestTimeoutSeconds > 0) ? cfg->RequestTimeoutSeconds : 30;
    source = scripting_fetch_mcp_script_source(group, script, database, timeout);
    free(group);
    free(script);
    if (source && source[0] == '\0') {
        free(source);
        return NULL;
    }
    return source;
}

const char *mcp_dispatch_auth_kind_name(McpAuthKind kind) {
    switch (kind) {
        case MCP_AUTH_KIND_HYDROGEN_JWT:
            return "hydrogen_jwt";
        case MCP_AUTH_KIND_OIDC_IDP:
            return "oidc_idp";
        case MCP_AUTH_KIND_OIDC_RP:
            return "oidc_rp";
        case MCP_AUTH_KIND_NONE:
        default:
            return "none";
    }
}

int mcp_dispatch_worker_cap(void) {
    if (app_config && app_config->scripting.WorkerCount > 0) {
        return app_config->scripting.WorkerCount;
    }
    return 2;
}

char *mcp_dispatch_build_params(const McpRpcEnvelope *env, const McpAuthResult *auth,
                                const char *session_id) {
    json_t *root;
    json_t *hydrogen;
    json_t *scopes;
    char *out;

    root = json_object();
    hydrogen = json_object();
    if (!root || !hydrogen) {
        if (root) {
            json_decref(root);
        }
        if (hydrogen) {
            json_decref(hydrogen);
        }
        return NULL;
    }

    json_object_set_new(root, "jsonrpc", json_string("2.0"));
    if (env && env->id) {
        json_object_set(root, "id", env->id);
    } else {
        json_object_set_new(root, "id", json_null());
    }
    json_object_set_new(root, "method", json_string(env && env->method ? env->method : ""));
    if (env && env->params) {
        json_object_set(root, "params", env->params);
    }

    json_object_set_new(hydrogen, "sub", json_string(auth && auth->sub ? auth->sub : ""));
    if (auth && auth->iss) {
        json_object_set_new(hydrogen, "iss", json_string(auth->iss));
    }
    json_object_set_new(hydrogen, "roles", json_string(auth && auth->roles ? auth->roles : ""));
    scopes = json_array();
    if (scopes && auth) {
        size_t i;
        for (i = 0; i < auth->scope_count; i++) {
            if (auth->scopes && auth->scopes[i]) {
                json_array_append_new(scopes, json_string(auth->scopes[i]));
            }
        }
    }
    json_object_set_new(hydrogen, "scopes", scopes ? scopes : json_array());
    json_object_set_new(hydrogen, "database",
                        json_string(auth && auth->database ? auth->database : ""));
    json_object_set_new(hydrogen, "session_id", json_string(session_id ? session_id : ""));
    json_object_set_new(hydrogen, "protocol_version",
                        json_string(env && env->protocol_version ? env->protocol_version
                                                                 : MCP_DEFAULT_PROTOCOL_VERSION));
    json_object_set_new(hydrogen, "auth_kind",
                        json_string(mcp_dispatch_auth_kind_name(auth ? auth->kind
                                                                     : MCP_AUTH_KIND_NONE)));
    json_object_set_new(root, "_hydrogen", hydrogen);

    out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

char *mcp_dispatch_submit_job(const char *script_name, const char *params_json) {
    if (mcp_dispatch_submit_hook) {
        return mcp_dispatch_submit_hook(script_name, params_json);
    }
    if (mcp_dispatch_protocol_source) {
        return scripting_submit_job_with_source(script_name, mcp_dispatch_protocol_source,
                                                params_json);
    }
    return scripting_submit_job(script_name, params_json);
}

ScriptingWaitResult mcp_dispatch_wait_job(const char *job_id, int timeout_seconds,
                                          char **result_json_out) {
    ScoreboardEntry *entry;
    ScriptingWaitResult wr;

    if (result_json_out) {
        *result_json_out = NULL;
    }
    if (mcp_dispatch_wait_hook) {
        return mcp_dispatch_wait_hook(job_id, timeout_seconds, result_json_out);
    }
    entry = NULL;
    wr = scripting_wait_job(job_id, timeout_seconds, &entry);
    if (result_json_out && entry && entry->result_json) {
        *result_json_out = strdup(entry->result_json);
    }
    scoreboard_entry_free(entry);
    return wr;
}

enum MHD_Result mcp_dispatch_queue_error(struct MHD_Connection *connection, const json_t *id,
                                         int code, const char *message, const char *session_id) {
    char *err_body;

    err_body = mcp_rpc_make_error(id, code, message);
    if (!err_body) {
        return MHD_NO;
    }
    mcp_stats_inc_rpc_failed();
    mcp_stats_add_bytes_out((unsigned long long)strlen(err_body));
    return mcp_queue_rpc_response(connection, MHD_HTTP_OK, err_body, true, session_id);
}

enum MHD_Result mcp_dispatch_submit_protocol(struct MHD_Connection *connection,
                                             const MCPConfig *cfg,
                                             const McpAuthResult *auth,
                                             const McpRpcEnvelope *env,
                                             const char *session_id) {
    char *params_json;
    char *job_id;
    char *result_json;
    char *source;
    ScriptingWaitResult wr;
    int timeout;
    int cap;
    const char *script_name;
    bool skip_load;

    if (!connection || !env) {
        return MHD_NO;
    }

    cap = mcp_dispatch_worker_cap();
    if ((int)mcp_stats_get_rpc_in_flight() >= cap) {
        return mcp_dispatch_queue_error(connection, env->id, MCP_RPC_OVERLOAD,
                                        "Overloaded", session_id);
    }

    script_name = (cfg && cfg->Protocol) ? cfg->Protocol : NULL;
    if (!script_name || script_name[0] == '\0') {
        return mcp_dispatch_queue_error(connection, env->id, MCP_RPC_INTERNAL_ERROR,
                                        "Internal error", session_id);
    }

    skip_load = mcp_dispatch_submit_hook && !mcp_dispatch_load_source_hook
                && !mcp_dispatch_protocol_source;
    source = NULL;
    if (!skip_load) {
        source = mcp_dispatch_load_protocol_source(cfg);
        if (!source) {
            char *not_found_body = strdup(MCP_DISPATCH_NOT_FOUND_BODY);
            if (!not_found_body) {
                return MHD_NO;
            }
            return mcp_queue_rpc_response(connection, MHD_HTTP_NOT_FOUND, not_found_body, true,
                                          session_id);
        }
    }

    params_json = mcp_dispatch_build_params(env, auth, session_id);
    if (!params_json) {
        free(source);
        return mcp_dispatch_queue_error(connection, env->id, MCP_RPC_INTERNAL_ERROR,
                                        "Internal error", session_id);
    }

    mcp_stats_add_rpc_in_flight(1);
    if (mcp_dispatch_submit_hook) {
        job_id = mcp_dispatch_submit_hook(script_name, params_json);
    } else if (source) {
        job_id = scripting_submit_job_with_source(script_name, source, params_json);
    } else {
        job_id = mcp_dispatch_submit_job(script_name, params_json);
    }
    free(source);
    free(params_json);
    if (!job_id) {
        mcp_stats_add_rpc_in_flight(-1);
        return mcp_dispatch_queue_error(connection, env->id, MCP_RPC_INTERNAL_ERROR,
                                        "Internal error", session_id);
    }

    timeout = (cfg && cfg->RequestTimeoutSeconds > 0) ? cfg->RequestTimeoutSeconds : 30;

    pthread_mutex_lock(&mcp_dispatch_suspend_lock);
    MHD_suspend_connection(connection);
    wr = mcp_dispatch_wait_job(job_id, timeout, &result_json);
    MHD_resume_connection(connection);
    pthread_mutex_unlock(&mcp_dispatch_suspend_lock);
    free(job_id);
    mcp_stats_add_rpc_in_flight(-1);

    if (wr == SCRIPTING_WAIT_TIMEOUT) {
        mcp_stats_inc_dispatch_timeouts();
    }

    if (wr == SCRIPTING_WAIT_COMPLETED && result_json) {
        mcp_stats_inc_rpc_succeeded();
        mcp_stats_add_bytes_out((unsigned long long)strlen(result_json));
        return mcp_queue_rpc_response(connection, MHD_HTTP_OK, result_json, true, session_id);
    }

    free(result_json);
    return mcp_dispatch_queue_error(connection, env->id, MCP_RPC_INTERNAL_ERROR,
                                    "Internal error", session_id);
}
