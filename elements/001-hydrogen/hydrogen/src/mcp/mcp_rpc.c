#include <src/hydrogen.h>
#include <src/mcp/mcp_rpc.h>

#include <string.h>

void mcp_rpc_envelope_cleanup(McpRpcEnvelope *env) {
    if (!env) {
        return;
    }
    if (env->root) {
        json_decref(env->root);
    }
    free(env->method);
    free(env->protocol_version);
    memset(env, 0, sizeof(*env));
}

const char *mcp_rpc_status_message(McpRpcStatus status) {
    switch (status) {
        case MCP_RPC_OK:
            return "OK";
        case MCP_RPC_ERR_PARSE:
            return "Parse error";
        case MCP_RPC_ERR_OVERSIZE:
            return "Parse error";
        case MCP_RPC_ERR_INVALID:
            return "Invalid Request";
        default:
            return "Invalid Request";
    }
}

int mcp_rpc_status_code(McpRpcStatus status) {
    switch (status) {
        case MCP_RPC_OK:
            return 0;
        case MCP_RPC_ERR_PARSE:
        case MCP_RPC_ERR_OVERSIZE:
            return MCP_RPC_PARSE_ERROR;
        case MCP_RPC_ERR_INVALID:
            return MCP_RPC_INVALID_REQUEST;
        default:
            return MCP_RPC_INVALID_REQUEST;
    }
}

bool mcp_rpc_is_initialize(const McpRpcEnvelope *env) {
    return env && env->method && strcmp(env->method, "initialize") == 0;
}

char *mcp_rpc_make_error(const json_t *id, int code, const char *message) {
    json_t *root;
    json_t *err;
    char *out;

    root = json_object();
    err = json_object();
    if (!root || !err) {
        if (root) {
            json_decref(root);
        }
        if (err) {
            json_decref(err);
        }
        return NULL;
    }
    json_object_set_new(root, "jsonrpc", json_string("2.0"));
    if (id) {
        json_object_set(root, "id", (json_t *)id);
    } else {
        json_object_set_new(root, "id", json_null());
    }
    json_object_set_new(err, "code", json_integer(code));
    json_object_set_new(err, "message", json_string(message ? message : "Error"));
    json_object_set_new(root, "error", err);
    out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

McpRpcStatus mcp_rpc_parse(const char *body, size_t body_len, int max_body_bytes,
                           const char *protocol_version_header, McpRpcEnvelope *out) {
    json_error_t err;
    json_t *root;
    json_t *jsonrpc;
    json_t *method;
    json_t *id;
    const char *method_str;
    const char *jsonrpc_str;
    int cap;

    if (out) {
        memset(out, 0, sizeof(*out));
    }
    cap = max_body_bytes > 0 ? max_body_bytes : 1048576;
    if (!out) {
        return MCP_RPC_ERR_INVALID;
    }
    if (!body || body_len == 0) {
        return MCP_RPC_ERR_PARSE;
    }
    if ((int)body_len > cap) {
        return MCP_RPC_ERR_OVERSIZE;
    }

    root = json_loadb(body, body_len, JSON_REJECT_DUPLICATES, &err);
    if (!root) {
        return MCP_RPC_ERR_PARSE;
    }
    if (json_is_array(root)) {
        json_decref(root);
        return MCP_RPC_ERR_INVALID;
    }
    if (!json_is_object(root)) {
        json_decref(root);
        return MCP_RPC_ERR_PARSE;
    }

    jsonrpc = json_object_get(root, "jsonrpc");
    jsonrpc_str = json_is_string(jsonrpc) ? json_string_value(jsonrpc) : NULL;
    if (!jsonrpc_str || strcmp(jsonrpc_str, "2.0") != 0) {
        json_decref(root);
        return MCP_RPC_ERR_INVALID;
    }

    method = json_object_get(root, "method");
    method_str = json_is_string(method) ? json_string_value(method) : NULL;
    if (!method_str || method_str[0] == '\0') {
        json_decref(root);
        return MCP_RPC_ERR_INVALID;
    }

    id = json_object_get(root, "id");
    if (id) {
        if (!json_is_string(id) && !json_is_number(id) && !json_is_null(id)) {
            json_decref(root);
            return MCP_RPC_ERR_INVALID;
        }
        out->is_notification = false;
        out->id = id;
    } else {
        out->is_notification = true;
        out->id = NULL;
    }

    out->root = root;
    out->params = json_object_get(root, "params");
    out->method = strdup(method_str);
    if (protocol_version_header && protocol_version_header[0] != '\0') {
        out->protocol_version = strdup(protocol_version_header);
    } else {
        out->protocol_version = strdup(MCP_DEFAULT_PROTOCOL_VERSION);
    }
    if (!out->method || !out->protocol_version) {
        mcp_rpc_envelope_cleanup(out);
        return MCP_RPC_ERR_PARSE;
    }
    return MCP_RPC_OK;
}
