#include <src/hydrogen.h>
#include <src/mcp/mcp_client.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include <src/globals.h>

#include <curl/curl.h>
#include <strings.h>

static int mcp_client_rpc_seq = 1;

int mcp_client_next_id(void) {
    int id = mcp_client_rpc_seq;
    mcp_client_rpc_seq += 1;
    if (mcp_client_rpc_seq <= 0) {
        mcp_client_rpc_seq = 1;
    }
    return id;
}

char *mcp_client_rpc_request(int id, const char *method, json_t *params) {
    json_t *root;
    char *out;

    if (!method || method[0] == '\0') {
        return NULL;
    }
    root = json_object();
    if (!root) {
        return NULL;
    }
    json_object_set_new(root, "jsonrpc", json_string("2.0"));
    json_object_set_new(root, "id", json_integer(id));
    json_object_set_new(root, "method", json_string(method));
    if (params) {
        json_object_set(root, "params", params);
    } else {
        json_object_set_new(root, "params", json_object());
    }
    out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

char *mcp_client_unwrap_body(const char *body) {
    const char *cursor;
    const char *last;
    const char *nl;
    size_t n;
    char *out;

    if (!body) {
        return NULL;
    }
    if (strncmp(body, "data:", 5) != 0 && strncmp(body, "event:", 6) != 0 &&
        strstr(body, "\ndata:") == NULL) {
        return strdup(body);
    }
    last = NULL;
    cursor = body;
    while (cursor && *cursor) {
        if (strncmp(cursor, "data:", 5) == 0) {
            last = cursor + 5;
            if (*last == ' ') {
                last += 1;
            }
        }
        nl = strchr(cursor, '\n');
        if (!nl) {
            break;
        }
        cursor = nl + 1;
    }
    if (!last) {
        return strdup(body);
    }
    nl = strchr(last, '\n');
    n = nl ? (size_t)(nl - last) : strlen(last);
    while (n > 0 && (last[n - 1] == '\r' || last[n - 1] == '\n')) {
        n -= 1;
    }
    out = malloc(n + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, last, n);
    out[n] = '\0';
    return out;
}

bool mcp_client_rpc_parse_result(const char *body, json_t **out_result, char **out_error) {
    char *unwrapped;
    json_t *root;
    json_t *err;
    json_t *msg;
    json_t *result;

    if (out_result) {
        *out_result = NULL;
    }
    if (out_error) {
        *out_error = NULL;
    }
    if (!body || body[0] == '\0') {
        if (out_error) {
            *out_error = strdup("empty mcp response");
        }
        return false;
    }
    unwrapped = mcp_client_unwrap_body(body);
    if (!unwrapped) {
        if (out_error) {
            *out_error = strdup("unwrap failed");
        }
        return false;
    }
    root = json_loads(unwrapped, 0, NULL);
    free(unwrapped);
    if (!root) {
        if (out_error) {
            *out_error = strdup("invalid mcp json");
        }
        return false;
    }
    err = json_object_get(root, "error");
    if (err && !json_is_null(err)) {
        msg = json_object_get(err, "message");
        if (out_error) {
            *out_error = strdup(msg && json_is_string(msg) ? json_string_value(msg) : "rpc error");
        }
        json_decref(root);
        return false;
    }
    result = json_object_get(root, "result");
    if (!result) {
        json_decref(root);
        if (out_error) {
            *out_error = strdup("missing result");
        }
        return false;
    }
    if (out_result) {
        *out_result = json_incref(result);
    }
    json_decref(root);
    return true;
}

char *mcp_client_http_post(const char *url,
                           const char *authorization,
                           const char *session_id,
                           const char *body,
                           char **out_session_id) {
    struct curl_slist *headers = NULL;
    OidcRpHttpResponse *resp;
    char *auth_hdr = NULL;
    char *sess_hdr = NULL;
    char *copied = NULL;

    if (out_session_id) {
        *out_session_id = NULL;
    }
    if (!url || url[0] == '\0' || !body) {
        return NULL;
    }
    headers = curl_slist_append(headers, "Accept: application/json, text/event-stream");
    headers = curl_slist_append(headers, "MCP-Protocol-Version: " MCP_CLIENT_PROTOCOL_VERSION);
    if (authorization && authorization[0] != '\0') {
        if (strncasecmp(authorization, "Bearer ", 7) == 0 ||
            strncasecmp(authorization, "Authorization:", 14) == 0) {
            if (asprintf(&auth_hdr, "Authorization: %s",
                         strncasecmp(authorization, "Authorization:", 14) == 0
                         ? authorization + 14 : authorization) != -1) {
                headers = curl_slist_append(headers, auth_hdr);
            }
        } else {
            if (asprintf(&auth_hdr, "Authorization: Bearer %s", authorization) != -1) {
                headers = curl_slist_append(headers, auth_hdr);
            }
        }
        free(auth_hdr);
    }
    if (session_id && session_id[0] != '\0') {
        if (asprintf(&sess_hdr, "Mcp-Session-Id: %s", session_id) != -1) {
            headers = curl_slist_append(headers, sess_hdr);
            free(sess_hdr);
        }
    }
    resp = oidc_rp_http_post_with_headers_slist(url, true, body, "application/json",
                                                headers, MCP_CLIENT_MAX_BODY, 30);
    if (!resp) {
        return NULL;
    }
    if (out_session_id && resp->headers) {
        for (size_t i = 0; i < resp->headers_count; i++) {
            if (resp->headers[i].name &&
                strcasecmp(resp->headers[i].name, "mcp-session-id") == 0 &&
                resp->headers[i].value) {
                *out_session_id = strdup(resp->headers[i].value);
                break;
            }
        }
    }
    if (resp->http_status >= 200 && resp->http_status < 300 && resp->body) {
        copied = strdup(resp->body);
    } else if (resp->body) {
        copied = strdup(resp->body);
    }
    oidc_rp_http_response_free(resp);
    return copied;
}

bool mcp_client_initialize(const char *url,
                           const char *authorization,
                           char **out_session_id,
                           char **out_error) {
    json_t *params;
    json_t *caps;
    json_t *info;
    json_t *result = NULL;
    char *req;
    char *resp;
    const char *notify;
    char *notify_resp;
    char *session = NULL;
    bool ok;

    if (out_session_id) {
        *out_session_id = NULL;
    }
    if (out_error) {
        *out_error = NULL;
    }
    params = json_object();
    caps = json_object();
    info = json_object();
    if (!params || !caps || !info) {
        json_decref(params);
        json_decref(caps);
        json_decref(info);
        if (out_error) {
            *out_error = strdup("alloc failed");
        }
        return false;
    }
    json_object_set_new(params, "protocolVersion", json_string(MCP_CLIENT_PROTOCOL_VERSION));
    json_object_set_new(info, "name", json_string("hydrogen"));
    json_object_set_new(info, "version", json_string("1.0"));
    json_object_set_new(params, "capabilities", caps);
    json_object_set_new(params, "clientInfo", info);
    req = mcp_client_rpc_request(mcp_client_next_id(), "initialize", params);
    json_decref(params);
    if (!req) {
        if (out_error) {
            *out_error = strdup("initialize encode failed");
        }
        return false;
    }
    resp = mcp_client_http_post(url, authorization, NULL, req, &session);
    free(req);
    ok = mcp_client_rpc_parse_result(resp, &result, out_error);
    free(resp);
    if (result) {
        json_decref(result);
    }
    if (!ok) {
        free(session);
        return false;
    }
    notify = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    notify_resp = mcp_client_http_post(url, authorization, session, notify, NULL);
    free(notify_resp);
    if (out_session_id) {
        *out_session_id = session;
    } else {
        free(session);
    }
    return true;
}

bool mcp_client_tools_list(const char *url,
                           const char *authorization,
                           const char *session_id,
                           json_t **out_tools,
                           char **out_error) {
    json_t *result = NULL;
    json_t *tools;
    char *req;
    char *resp;
    bool ok;

    if (out_tools) {
        *out_tools = NULL;
    }
    req = mcp_client_rpc_request(mcp_client_next_id(), "tools/list", NULL);
    if (!req) {
        if (out_error) {
            *out_error = strdup("tools/list encode failed");
        }
        return false;
    }
    resp = mcp_client_http_post(url, authorization, session_id, req, NULL);
    free(req);
    ok = mcp_client_rpc_parse_result(resp, &result, out_error);
    free(resp);
    if (!ok) {
        return false;
    }
    tools = json_object_get(result, "tools");
    if (!tools || !json_is_array(tools)) {
        json_decref(result);
        if (out_error) {
            *out_error = strdup("tools/list missing tools array");
        }
        return false;
    }
    if (out_tools) {
        *out_tools = json_incref(tools);
    }
    json_decref(result);
    return true;
}

bool mcp_client_tools_call(const char *url,
                           const char *authorization,
                           const char *session_id,
                           const char *name,
                           json_t *arguments,
                           json_t **out_result,
                           char **out_error) {
    json_t *params;
    char *req;
    char *resp;
    bool ok;

    if (out_result) {
        *out_result = NULL;
    }
    if (!name || name[0] == '\0') {
        if (out_error) {
            *out_error = strdup("tool name required");
        }
        return false;
    }
    params = json_object();
    if (!params) {
        if (out_error) {
            *out_error = strdup("alloc failed");
        }
        return false;
    }
    json_object_set_new(params, "name", json_string(name));
    if (arguments) {
        json_object_set(params, "arguments", arguments);
    } else {
        json_object_set_new(params, "arguments", json_object());
    }
    req = mcp_client_rpc_request(mcp_client_next_id(), "tools/call", params);
    json_decref(params);
    if (!req) {
        if (out_error) {
            *out_error = strdup("tools/call encode failed");
        }
        return false;
    }
    resp = mcp_client_http_post(url, authorization, session_id, req, NULL);
    free(req);
    ok = mcp_client_rpc_parse_result(resp, out_result, out_error);
    free(resp);
    return ok;
}

bool mcp_client_tool_allowed(const char *name,
                             char **allowed_tools,
                             size_t allowed_count) {
    size_t i;

    if (!name || name[0] == '\0' || !allowed_tools || allowed_count == 0) {
        return false;
    }
    for (i = 0; i < allowed_count; i++) {
        if (allowed_tools[i] && strcmp(allowed_tools[i], name) == 0) {
            return true;
        }
    }
    return false;
}

json_t *mcp_client_tools_filter(json_t *tools,
                                char **allowed_tools,
                                size_t allowed_count) {
    json_t *out;
    size_t i;
    size_t n;

    if (!tools || !json_is_array(tools)) {
        return json_array();
    }
    out = json_array();
    if (!out) {
        return NULL;
    }
    n = json_array_size(tools);
    for (i = 0; i < n; i++) {
        json_t *tool = json_array_get(tools, i);
        json_t *name_obj;
        const char *name;

        if (!json_is_object(tool)) {
            continue;
        }
        name_obj = json_object_get(tool, "name");
        if (!name_obj || !json_is_string(name_obj)) {
            continue;
        }
        name = json_string_value(name_obj);
        if (!mcp_client_tool_allowed(name, allowed_tools, allowed_count)) {
            continue;
        }
        json_array_append(out, tool);
    }
    return out;
}

json_t *mcp_client_mcp_schema(json_t *mcp_tool) {
    json_t *schema;

    if (!mcp_tool) {
        return NULL;
    }
    schema = json_object_get(mcp_tool, "inputSchema");
    if (!schema) {
        schema = json_object_get(mcp_tool, "input_schema");
    }
    return schema;
}

json_t *mcp_client_tool_to_openai(json_t *mcp_tool) {
    json_t *out;
    json_t *fn;
    json_t *name;
    json_t *desc;
    json_t *schema;

    if (!mcp_tool || !json_is_object(mcp_tool)) {
        return NULL;
    }
    name = json_object_get(mcp_tool, "name");
    if (!name || !json_is_string(name)) {
        return NULL;
    }
    out = json_object();
    fn = json_object();
    if (!out || !fn) {
        json_decref(out);
        json_decref(fn);
        return NULL;
    }
    json_object_set_new(out, "type", json_string("function"));
    json_object_set(fn, "name", name);
    desc = json_object_get(mcp_tool, "description");
    if (desc) {
        json_object_set(fn, "description", desc);
    }
    schema = mcp_client_mcp_schema(mcp_tool);
    if (schema) {
        json_object_set(fn, "parameters", schema);
    } else {
        json_object_set_new(fn, "parameters", json_object());
    }
    json_object_set_new(out, "function", fn);
    return out;
}

json_t *mcp_client_tool_to_responses(json_t *mcp_tool) {
    json_t *out;
    json_t *name;
    json_t *desc;
    json_t *schema;

    if (!mcp_tool || !json_is_object(mcp_tool)) {
        return NULL;
    }
    name = json_object_get(mcp_tool, "name");
    if (!name || !json_is_string(name)) {
        return NULL;
    }
    out = json_object();
    if (!out) {
        return NULL;
    }
    json_object_set_new(out, "type", json_string("function"));
    json_object_set(out, "name", name);
    desc = json_object_get(mcp_tool, "description");
    if (desc) {
        json_object_set(out, "description", desc);
    }
    schema = mcp_client_mcp_schema(mcp_tool);
    if (schema) {
        json_object_set(out, "parameters", schema);
    } else {
        json_object_set_new(out, "parameters", json_object());
    }
    return out;
}

json_t *mcp_client_tool_to_anthropic(json_t *mcp_tool) {
    json_t *out;
    json_t *name;
    json_t *desc;
    json_t *schema;

    if (!mcp_tool || !json_is_object(mcp_tool)) {
        return NULL;
    }
    name = json_object_get(mcp_tool, "name");
    if (!name || !json_is_string(name)) {
        return NULL;
    }
    out = json_object();
    if (!out) {
        return NULL;
    }
    json_object_set(out, "name", name);
    desc = json_object_get(mcp_tool, "description");
    if (desc) {
        json_object_set(out, "description", desc);
    }
    schema = mcp_client_mcp_schema(mcp_tool);
    if (schema) {
        json_object_set(out, "input_schema", schema);
    } else {
        json_object_set_new(out, "input_schema", json_object());
    }
    return out;
}

json_t *mcp_client_tools_map(json_t *mcp_tools, json_t *(*convert)(json_t *)) {
    json_t *out;
    size_t i;
    size_t n;

    out = json_array();
    if (!out) {
        return NULL;
    }
    if (!mcp_tools || !json_is_array(mcp_tools) || !convert) {
        return out;
    }
    n = json_array_size(mcp_tools);
    for (i = 0; i < n; i++) {
        json_t *converted = convert(json_array_get(mcp_tools, i));
        if (converted) {
            json_array_append_new(out, converted);
        }
    }
    return out;
}

json_t *mcp_client_tools_to_openai(json_t *mcp_tools) {
    return mcp_client_tools_map(mcp_tools, mcp_client_tool_to_openai);
}

json_t *mcp_client_tools_to_responses(json_t *mcp_tools) {
    return mcp_client_tools_map(mcp_tools, mcp_client_tool_to_responses);
}

json_t *mcp_client_tools_to_anthropic(json_t *mcp_tools) {
    return mcp_client_tools_map(mcp_tools, mcp_client_tool_to_anthropic);
}

json_t *mcp_client_fetch_tools(const char *url,
                               const char *authorization,
                               char **allowed_tools,
                               size_t allowed_count,
                               const char *correlation_id) {
    char *session = NULL;
    char *error = NULL;
    json_t *listed = NULL;
    json_t *filtered;
    const char *cid = correlation_id ? correlation_id : "-";

    if (!url || url[0] == '\0') {
        return NULL;
    }
    if (!mcp_client_initialize(url, authorization, &session, &error)) {
        log_this(SR_MCP, "local_mcp initialize failed url=%s err=%s (cid=%s)",
                 LOG_LEVEL_ERROR, 3, url, error ? error : "unknown", cid);
        free(error);
        free(session);
        return NULL;
    }
    if (!mcp_client_tools_list(url, authorization, session, &listed, &error)) {
        log_this(SR_MCP, "local_mcp tools/list failed url=%s err=%s (cid=%s)",
                 LOG_LEVEL_ERROR, 3, url, error ? error : "unknown", cid);
        free(error);
        free(session);
        return NULL;
    }
    free(session);
    filtered = mcp_client_tools_filter(listed, allowed_tools, allowed_count);
    json_decref(listed);
    return filtered;
}
