#include <src/hydrogen.h>
#include <src/mcp/mcp_client.h>
#include <src/globals.h>
#include "local_mcp.h"
#include "proxy.h"
#include "proxy_multi.h"

void chat_local_mcp_server_cleanup(ChatLocalMcpServer *server) {
    if (!server) {
        return;
    }
    free(server->url);
    free(server->authorization);
    if (server->allowed_tools) {
        for (size_t i = 0; i < server->allowed_tool_count; i++) {
            free(server->allowed_tools[i]);
        }
        free(server->allowed_tools);
    }
    server->url = NULL;
    server->authorization = NULL;
    server->allowed_tools = NULL;
    server->allowed_tool_count = 0;
}

void chat_local_mcp_config_cleanup(ChatLocalMcpConfig *cfg) {
    if (!cfg) {
        return;
    }
    if (cfg->servers) {
        for (size_t i = 0; i < cfg->server_count; i++) {
            chat_local_mcp_server_cleanup(&cfg->servers[i]);
        }
        free(cfg->servers);
    }
    cfg->servers = NULL;
    cfg->server_count = 0;
    cfg->enabled = false;
}

bool chat_local_mcp_load_allowed(json_t *allowed_obj, ChatLocalMcpServer *server) {
    size_t i;
    size_t n;

    if (!allowed_obj || !json_is_array(allowed_obj) || !server) {
        return false;
    }
    n = json_array_size(allowed_obj);
    if (n == 0) {
        return false;
    }
    server->allowed_tools = calloc(n, sizeof(char *));
    if (!server->allowed_tools) {
        return false;
    }
    server->allowed_tool_count = 0;
    for (i = 0; i < n; i++) {
        json_t *item = json_array_get(allowed_obj, i);
        if (!json_is_string(item)) {
            continue;
        }
        server->allowed_tools[server->allowed_tool_count] = strdup(json_string_value(item));
        if (server->allowed_tools[server->allowed_tool_count]) {
            server->allowed_tool_count += 1;
        }
    }
    return server->allowed_tool_count > 0;
}

bool chat_local_mcp_config_load(json_t *collection, ChatLocalMcpConfig *out) {
    json_t *obj;
    json_t *enabled;
    json_t *servers;
    size_t i;
    size_t n;
    size_t loaded = 0;

    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    if (!collection || !json_is_object(collection)) {
        return true;
    }
    obj = json_object_get(collection, "local_mcp");
    if (!obj || !json_is_object(obj)) {
        return true;
    }
    enabled = json_object_get(obj, "enabled");
    out->enabled = enabled && json_is_boolean(enabled) && json_boolean_value(enabled);
    servers = json_object_get(obj, "servers");
    if (!servers || !json_is_array(servers)) {
        return true;
    }
    n = json_array_size(servers);
    if (n == 0) {
        return true;
    }
    out->servers = calloc(n, sizeof(ChatLocalMcpServer));
    if (!out->servers) {
        out->enabled = false;
        return false;
    }
    for (i = 0; i < n; i++) {
        json_t *row = json_array_get(servers, i);
        json_t *url;
        json_t *auth;
        json_t *allowed;
        ChatLocalMcpServer *dst;

        if (!json_is_object(row)) {
            continue;
        }
        url = json_object_get(row, "url");
        allowed = json_object_get(row, "allowed_tools");
        if (!url || !json_is_string(url) || json_string_value(url)[0] == '\0') {
            continue;
        }
        dst = &out->servers[loaded];
        dst->url = strdup(json_string_value(url));
        auth = json_object_get(row, "authorization");
        if (auth && json_is_string(auth)) {
            dst->authorization = strdup(json_string_value(auth));
        }
        if (!chat_local_mcp_load_allowed(allowed, dst)) {
            chat_local_mcp_server_cleanup(dst);
            continue;
        }
        loaded += 1;
    }
    out->server_count = loaded;
    if (loaded == 0) {
        free(out->servers);
        out->servers = NULL;
    }
    return true;
}

json_t *chat_local_mcp_list_tools(const ChatEngineConfig *engine, const char *correlation_id) {
    json_t *all;
    size_t i;

    if (!engine || !engine->local_mcp.enabled || engine->local_mcp.server_count == 0) {
        return NULL;
    }
    all = json_array();
    if (!all) {
        return NULL;
    }
    for (i = 0; i < engine->local_mcp.server_count; i++) {
        const ChatLocalMcpServer *server = &engine->local_mcp.servers[i];
        json_t *tools = mcp_client_fetch_tools(server->url,
                                               server->authorization,
                                               server->allowed_tools,
                                               server->allowed_tool_count,
                                               correlation_id);
        if (!tools) {
            continue;
        }
        size_t t;
        size_t n = json_array_size(tools);
        for (t = 0; t < n; t++) {
            json_array_append(all, json_array_get(tools, t));
        }
        json_decref(tools);
    }
    if (json_array_size(all) == 0) {
        json_decref(all);
        return NULL;
    }
    return all;
}

void chat_request_append_local_mcp_tools(json_t *root, json_t *mcp_tools, ChatEngineProvider provider,
                                         bool use_responses_api) {
    json_t *converted;
    json_t *tools;

    if (!root || !mcp_tools || !json_is_array(mcp_tools) || json_array_size(mcp_tools) == 0) {
        return;
    }
    if (provider == CEC_PROVIDER_ANTHROPIC) {
        converted = mcp_client_tools_to_anthropic(mcp_tools);
    } else if (use_responses_api) {
        converted = mcp_client_tools_to_responses(mcp_tools);
    } else {
        converted = mcp_client_tools_to_openai(mcp_tools);
    }
    if (!converted) {
        return;
    }
    tools = json_object_get(root, "tools");
    if (!tools || !json_is_array(tools)) {
        json_object_set_new(root, "tools", converted);
        return;
    }
    size_t i;
    size_t n = json_array_size(converted);
    for (i = 0; i < n; i++) {
        json_array_append(tools, json_array_get(converted, i));
    }
    json_decref(converted);
}

json_t *chat_local_mcp_normalize_call(const char *id, const char *name, json_t *arguments) {
    json_t *out;

    if (!name || name[0] == '\0') {
        return NULL;
    }
    out = json_object();
    if (!out) {
        return NULL;
    }
    json_object_set_new(out, "id", json_string(id ? id : ""));
    json_object_set_new(out, "name", json_string(name));
    if (arguments) {
        json_object_set(out, "arguments", arguments);
    } else {
        json_object_set_new(out, "arguments", json_object());
    }
    return out;
}

json_t *chat_local_mcp_extract_openai(json_t *root) {
    json_t *out;
    json_t *choices;
    json_t *first;
    json_t *message;
    json_t *calls;
    size_t i;
    size_t n;

    out = json_array();
    if (!out || !root) {
        json_decref(out);
        return json_array();
    }
    choices = json_object_get(root, "choices");
    if (!choices || !json_is_array(choices)) {
        return out;
    }
    first = json_array_get(choices, 0);
    if (!first) {
        return out;
    }
    message = json_object_get(first, "message");
    if (!message) {
        message = json_object_get(first, "delta");
    }
    if (!message) {
        return out;
    }
    calls = json_object_get(message, "tool_calls");
    if (!calls || !json_is_array(calls)) {
        return out;
    }
    n = json_array_size(calls);
    for (i = 0; i < n; i++) {
        json_t *call = json_array_get(calls, i);
        json_t *fn;
        json_t *id;
        json_t *name;
        json_t *args;
        json_t *parsed_args = NULL;
        json_t *normalized;

        if (!json_is_object(call)) {
            continue;
        }
        id = json_object_get(call, "id");
        fn = json_object_get(call, "function");
        if (!fn) {
            continue;
        }
        name = json_object_get(fn, "name");
        args = json_object_get(fn, "arguments");
        if (args && json_is_string(args)) {
            const char *args_str = json_string_value(args);
            parsed_args = json_loads(args_str ? args_str : "{}", 0, NULL);
            if (!parsed_args) {
                parsed_args = json_object();
            }
            args = parsed_args;
        }
        normalized = chat_local_mcp_normalize_call(
            id && json_is_string(id) ? json_string_value(id) : "",
            name && json_is_string(name) ? json_string_value(name) : NULL,
            args);
        if (parsed_args) {
            json_decref(parsed_args);
        }
        if (normalized) {
            json_array_append_new(out, normalized);
        }
    }
    return out;
}

json_t *chat_local_mcp_extract_anthropic(json_t *root) {
    json_t *out;
    json_t *content;
    size_t i;
    size_t n;

    out = json_array();
    if (!out || !root) {
        json_decref(out);
        return json_array();
    }
    content = json_object_get(root, "content");
    if (!content || !json_is_array(content)) {
        return out;
    }
    n = json_array_size(content);
    for (i = 0; i < n; i++) {
        json_t *block = json_array_get(content, i);
        json_t *type;
        json_t *normalized;

        if (!json_is_object(block)) {
            continue;
        }
        type = json_object_get(block, "type");
        if (!type || !json_is_string(type) || strcmp(json_string_value(type), "tool_use") != 0) {
            continue;
        }
        normalized = chat_local_mcp_normalize_call(
            json_is_string(json_object_get(block, "id")) ? json_string_value(json_object_get(block, "id")) : "",
            json_is_string(json_object_get(block, "name")) ? json_string_value(json_object_get(block, "name")) : NULL,
            json_object_get(block, "input"));
        if (normalized) {
            json_array_append_new(out, normalized);
        }
    }
    return out;
}

json_t *chat_local_mcp_extract_responses(json_t *root) {
    json_t *out;
    json_t *output;
    size_t i;
    size_t n;

    out = json_array();
    if (!out || !root) {
        json_decref(out);
        return json_array();
    }
    output = json_object_get(root, "output");
    if (!output || !json_is_array(output)) {
        json_t *response = json_object_get(root, "response");
        if (response) {
            output = json_object_get(response, "output");
        }
    }
    if (!output || !json_is_array(output)) {
        return out;
    }
    n = json_array_size(output);
    for (i = 0; i < n; i++) {
        json_t *item = json_array_get(output, i);
        json_t *type;
        json_t *args;
        json_t *parsed_args = NULL;
        json_t *normalized;
        const char *id;
        json_t *id_obj;

        if (!json_is_object(item)) {
            continue;
        }
        type = json_object_get(item, "type");
        if (!type || !json_is_string(type) || strcmp(json_string_value(type), "function_call") != 0) {
            continue;
        }
        id_obj = json_object_get(item, "call_id");
        if (!id_obj) {
            id_obj = json_object_get(item, "id");
        }
        id = id_obj && json_is_string(id_obj) ? json_string_value(id_obj) : "";
        args = json_object_get(item, "arguments");
        if (args && json_is_string(args)) {
            parsed_args = json_loads(json_string_value(args), 0, NULL);
            if (!parsed_args) {
                parsed_args = json_object();
            }
            args = parsed_args;
        }
        normalized = chat_local_mcp_normalize_call(
            id,
            json_is_string(json_object_get(item, "name")) ? json_string_value(json_object_get(item, "name")) : NULL,
            args);
        if (parsed_args) {
            json_decref(parsed_args);
        }
        if (normalized) {
            json_array_append_new(out, normalized);
        }
    }
    return out;
}

json_t *chat_local_mcp_extract_tool_calls_json(json_t *root, ChatEngineProvider provider) {
    json_t *calls;

    if (!root) {
        return json_array();
    }
    if (provider == CEC_PROVIDER_ANTHROPIC) {
        calls = chat_local_mcp_extract_anthropic(root);
    } else {
        calls = chat_local_mcp_extract_openai(root);
        if (calls && json_array_size(calls) == 0) {
            json_decref(calls);
            calls = chat_local_mcp_extract_responses(root);
        }
    }
    return calls;
}

json_t *chat_local_mcp_extract_tool_calls(const char *response_body, ChatEngineProvider provider) {
    json_t *root;
    json_t *calls;

    if (!response_body) {
        return NULL;
    }
    root = json_loads(response_body, 0, NULL);
    if (!root) {
        return NULL;
    }
    calls = chat_local_mcp_extract_tool_calls_json(root, provider);
    json_decref(root);
    if (calls && json_array_size(calls) == 0) {
        json_decref(calls);
        return NULL;
    }
    return calls;
}

char *chat_local_mcp_tool_result_text(json_t *mcp_result) {
    json_t *content;
    json_t *is_error;
    char *dumped;

    if (!mcp_result) {
        return strdup("{\"error\":\"empty tool result\"}");
    }
    is_error = json_object_get(mcp_result, "isError");
    content = json_object_get(mcp_result, "content");
    if (content && json_is_array(content) && json_array_size(content) > 0) {
        size_t n = json_array_size(content);
        for (size_t i = 0; i < n; i++) {
            json_t *block = json_array_get(content, i);
            json_t *text = json_object_get(block, "text");
            if (text && json_is_string(text)) {
                return strdup(json_string_value(text));
            }
        }
    }
    dumped = json_dumps(mcp_result, JSON_COMPACT);
    if (dumped) {
        return dumped;
    }
    if (is_error && json_is_true(is_error)) {
        return strdup("{\"isError\":true}");
    }
    return strdup("{}");
}

const ChatLocalMcpServer *chat_local_mcp_find_server(const ChatEngineConfig *engine, const char *name) {
    size_t i;

    if (!engine || !name) {
        return NULL;
    }
    for (i = 0; i < engine->local_mcp.server_count; i++) {
        const ChatLocalMcpServer *server = &engine->local_mcp.servers[i];
        if (mcp_client_tool_allowed(name, server->allowed_tools, server->allowed_tool_count)) {
            return server;
        }
    }
    return NULL;
}

json_t *chat_local_mcp_proxy_tool_calls(const ChatEngineConfig *engine,
                                        json_t *tool_calls,
                                        const char *correlation_id) {
    json_t *results;
    size_t i;
    size_t n;
    const char *cid = correlation_id ? correlation_id : "-";

    results = json_array();
    if (!results) {
        return NULL;
    }
    if (!engine || !tool_calls || !json_is_array(tool_calls)) {
        return results;
    }
    n = json_array_size(tool_calls);
    for (i = 0; i < n; i++) {
        json_t *call = json_array_get(tool_calls, i);
        json_t *name_obj;
        json_t *id_obj;
        json_t *args;
        json_t *row;
        json_t *mcp_result = NULL;
        char *error = NULL;
        char *session = NULL;
        char *text;
        const char *name;
        const char *id;
        const ChatLocalMcpServer *server;

        row = json_object();
        if (!row) {
            continue;
        }
        name_obj = json_object_get(call, "name");
        id_obj = json_object_get(call, "id");
        args = json_object_get(call, "arguments");
        name = name_obj && json_is_string(name_obj) ? json_string_value(name_obj) : "";
        id = id_obj && json_is_string(id_obj) ? json_string_value(id_obj) : "";
        json_object_set_new(row, "id", json_string(id));
        json_object_set_new(row, "name", json_string(name));
        server = chat_local_mcp_find_server(engine, name);
        if (!server) {
            json_object_set_new(row, "content", json_string("{\"error\":\"tool not allowlisted\"}"));
            json_array_append_new(results, row);
            continue;
        }
        if (!mcp_client_initialize(server->url, server->authorization, &session, &error)) {
            log_this(SR_CHAT, "local_mcp tools/call initialize failed name=%s err=%s (cid=%s)",
                     LOG_LEVEL_ERROR, 3, name, error ? error : "unknown", cid);
            json_object_set_new(row, "content", json_string(error ? error : "initialize failed"));
            free(error);
            free(session);
            json_array_append_new(results, row);
            continue;
        }
        if (!mcp_client_tools_call(server->url, server->authorization, session, name, args,
                                   &mcp_result, &error)) {
            log_this(SR_CHAT, "local_mcp tools/call failed name=%s err=%s (cid=%s)",
                     LOG_LEVEL_ERROR, 3, name, error ? error : "unknown", cid);
            json_object_set_new(row, "content", json_string(error ? error : "tools/call failed"));
            free(error);
            free(session);
            if (mcp_result) {
                json_decref(mcp_result);
            }
            json_array_append_new(results, row);
            continue;
        }
        free(session);
        text = chat_local_mcp_tool_result_text(mcp_result);
        json_decref(mcp_result);
        json_object_set_new(row, "content", json_string(text ? text : "{}"));
        free(text);
        json_array_append_new(results, row);
    }
    return results;
}

void chat_local_mcp_append_openai_results(json_t *root, json_t *tool_calls, json_t *results) {
    json_t *messages;
    json_t *assistant;
    json_t *calls;
    size_t i;
    size_t n;

    messages = json_object_get(root, "messages");
    if (!messages || !json_is_array(messages)) {
        return;
    }
    assistant = json_object();
    calls = json_array();
    json_object_set_new(assistant, "role", json_string("assistant"));
    json_object_set_new(assistant, "content", json_null());
    n = json_array_size(tool_calls);
    for (i = 0; i < n; i++) {
        json_t *call = json_array_get(tool_calls, i);
        json_t *tc = json_object();
        json_t *fn = json_object();
        json_t *args = json_object_get(call, "arguments");
        char *args_str = args ? json_dumps(args, JSON_COMPACT) : strdup("{}");

        json_object_set(tc, "id", json_object_get(call, "id"));
        json_object_set_new(tc, "type", json_string("function"));
        json_object_set(fn, "name", json_object_get(call, "name"));
        json_object_set_new(fn, "arguments", json_string(args_str ? args_str : "{}"));
        free(args_str);
        json_object_set_new(tc, "function", fn);
        json_array_append_new(calls, tc);
    }
    json_object_set_new(assistant, "tool_calls", calls);
    json_array_append_new(messages, assistant);
    n = json_array_size(results);
    for (i = 0; i < n; i++) {
        json_t *row = json_array_get(results, i);
        json_t *msg = json_object();
        json_object_set_new(msg, "role", json_string("tool"));
        json_object_set(msg, "tool_call_id", json_object_get(row, "id"));
        json_object_set(msg, "content", json_object_get(row, "content"));
        json_array_append_new(messages, msg);
    }
}

void chat_local_mcp_append_anthropic_results(json_t *root, json_t *tool_calls, json_t *results) {
    json_t *messages;
    json_t *assistant;
    json_t *user;
    json_t *asst_content;
    json_t *user_content;
    size_t i;
    size_t n;

    messages = json_object_get(root, "messages");
    if (!messages || !json_is_array(messages)) {
        return;
    }
    assistant = json_object();
    user = json_object();
    asst_content = json_array();
    user_content = json_array();
    json_object_set_new(assistant, "role", json_string("assistant"));
    json_object_set_new(user, "role", json_string("user"));
    n = json_array_size(tool_calls);
    for (i = 0; i < n; i++) {
        json_t *call = json_array_get(tool_calls, i);
        json_t *block = json_object();
        json_object_set_new(block, "type", json_string("tool_use"));
        json_object_set(block, "id", json_object_get(call, "id"));
        json_object_set(block, "name", json_object_get(call, "name"));
        json_object_set(block, "input", json_object_get(call, "arguments"));
        json_array_append_new(asst_content, block);
    }
    n = json_array_size(results);
    for (i = 0; i < n; i++) {
        json_t *row = json_array_get(results, i);
        json_t *block = json_object();
        json_object_set_new(block, "type", json_string("tool_result"));
        json_object_set(block, "tool_use_id", json_object_get(row, "id"));
        json_object_set(block, "content", json_object_get(row, "content"));
        json_array_append_new(user_content, block);
    }
    json_object_set_new(assistant, "content", asst_content);
    json_object_set_new(user, "content", user_content);
    json_array_append_new(messages, assistant);
    json_array_append_new(messages, user);
}

void chat_local_mcp_append_responses_results(json_t *root, json_t *tool_calls, json_t *results) {
    json_t *input;
    size_t i;
    size_t n;

    input = json_object_get(root, "input");
    if (!input || !json_is_array(input)) {
        return;
    }
    n = json_array_size(tool_calls);
    for (i = 0; i < n; i++) {
        json_t *call = json_array_get(tool_calls, i);
        json_t *item = json_object();
        json_t *args = json_object_get(call, "arguments");
        char *args_str = args ? json_dumps(args, JSON_COMPACT) : strdup("{}");
        json_object_set_new(item, "type", json_string("function_call"));
        json_object_set(item, "call_id", json_object_get(call, "id"));
        json_object_set(item, "name", json_object_get(call, "name"));
        json_object_set_new(item, "arguments", json_string(args_str ? args_str : "{}"));
        free(args_str);
        json_array_append_new(input, item);
    }
    n = json_array_size(results);
    for (i = 0; i < n; i++) {
        json_t *row = json_array_get(results, i);
        json_t *item = json_object();
        json_object_set_new(item, "type", json_string("function_call_output"));
        json_object_set(item, "call_id", json_object_get(row, "id"));
        json_object_set(item, "output", json_object_get(row, "content"));
        json_array_append_new(input, item);
    }
}

char *chat_local_mcp_append_tool_results(const char *request_json,
                                         json_t *tool_calls,
                                         json_t *results,
                                         ChatEngineProvider provider,
                                         bool use_responses_api) {
    json_t *root;
    char *out;

    if (!request_json || !tool_calls || !results) {
        return NULL;
    }
    root = json_loads(request_json, 0, NULL);
    if (!root) {
        return NULL;
    }
    if (provider == CEC_PROVIDER_ANTHROPIC) {
        chat_local_mcp_append_anthropic_results(root, tool_calls, results);
    } else if (use_responses_api) {
        chat_local_mcp_append_responses_results(root, tool_calls, results);
    } else {
        chat_local_mcp_append_openai_results(root, tool_calls, results);
    }
    out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

void chat_local_mcp_accumulate_stream_tool_calls(json_t **acc, json_t *delta_tool_calls) {
    size_t i;
    size_t n;

    if (!acc || !delta_tool_calls || !json_is_array(delta_tool_calls)) {
        return;
    }
    if (!*acc) {
        *acc = json_array();
        if (!*acc) {
            return;
        }
    }
    n = json_array_size(delta_tool_calls);
    for (i = 0; i < n; i++) {
        json_t *delta = json_array_get(delta_tool_calls, i);
        json_t *index_obj;
        json_int_t index = (json_int_t)i;
        json_t *existing;
        json_t *fn;
        json_t *d_fn;

        if (!json_is_object(delta)) {
            continue;
        }
        index_obj = json_object_get(delta, "index");
        if (index_obj && json_is_integer(index_obj)) {
            index = json_integer_value(index_obj);
        }
        while (json_array_size(*acc) <= (size_t)index) {
            json_t *blank = json_object();
            json_object_set_new(blank, "id", json_string(""));
            json_object_set_new(blank, "name", json_string(""));
            json_object_set_new(blank, "arguments", json_string(""));
            json_array_append_new(*acc, blank);
        }
        existing = json_array_get(*acc, (size_t)index);
        if (json_object_get(delta, "id") && json_is_string(json_object_get(delta, "id"))) {
            json_object_set(existing, "id", json_object_get(delta, "id"));
        }
        d_fn = json_object_get(delta, "function");
        fn = json_object_get(delta, "name");
        if (d_fn && json_is_object(d_fn)) {
            json_t *d_name = json_object_get(d_fn, "name");
            if (d_name && json_is_string(d_name) && json_string_value(d_name)[0] != '\0') {
                json_object_set(existing, "name", d_name);
            }
            json_t *d_args = json_object_get(d_fn, "arguments");
            if (d_args && json_is_string(d_args)) {
                json_t *args = json_object_get(existing, "arguments");
                if (args && json_is_string(args)) {
                    const char *prev = json_string_value(args);
                    const char *add = json_string_value(d_args);
                    char *joined = NULL;
                    if (asprintf(&joined, "%s%s", prev ? prev : "", add ? add : "") != -1) {
                        json_object_set_new(existing, "arguments", json_string(joined));
                        free(joined);
                    }
                }
            }
        } else if (fn && json_is_string(fn)) {
            json_object_set(existing, "name", fn);
        }
    }
}

json_t *chat_local_mcp_finalize_accumulated(json_t *acc) {
    json_t *out;
    size_t i;
    size_t n;

    if (!acc || !json_is_array(acc)) {
        return NULL;
    }
    out = json_array();
    n = json_array_size(acc);
    for (i = 0; i < n; i++) {
        json_t *row = json_array_get(acc, i);
        json_t *args = json_object_get(row, "arguments");
        json_t *parsed = NULL;
        json_t *normalized;

        if (args && json_is_string(args)) {
            parsed = json_loads(json_string_value(args), 0, NULL);
            if (!parsed) {
                parsed = json_object();
            }
        }
        normalized = chat_local_mcp_normalize_call(
            json_is_string(json_object_get(row, "id")) ? json_string_value(json_object_get(row, "id")) : "",
            json_is_string(json_object_get(row, "name")) ? json_string_value(json_object_get(row, "name")) : NULL,
            parsed ? parsed : args);
        if (parsed) {
            json_decref(parsed);
        }
        if (normalized) {
            json_array_append_new(out, normalized);
        }
    }
    if (json_array_size(out) == 0) {
        json_decref(out);
        return NULL;
    }
    return out;
}

struct ChatProxyResult *chat_local_mcp_complete_request(const ChatEngineConfig *engine,
                                                        const char *request_body,
                                                        const char *correlation_id) {
    ChatProxyConfig proxy_config = chat_proxy_get_default_config();
    ChatProxyResult *result;
    char *current;
    int round;
    const char *cid = correlation_id ? correlation_id : "-";

    if (!engine || !request_body) {
        return NULL;
    }
    current = strdup(request_body);
    if (!current) {
        return NULL;
    }
    result = chat_proxy_send_with_retry(engine, current, &proxy_config);
    if (!engine->local_mcp.enabled) {
        free(current);
        return result;
    }
    for (round = 0; round < CHAT_LOCAL_MCP_MAX_ROUNDS; round++) {
        json_t *calls;
        json_t *results;
        char *next_body;

        if (!chat_proxy_result_is_success(result) || !result->response_body) {
            break;
        }
        calls = chat_local_mcp_extract_tool_calls(result->response_body, engine->provider);
        if (!calls) {
            break;
        }
        log_this(SR_CHAT, "local_mcp proxy round=%d calls=%zu (cid=%s)",
                 LOG_LEVEL_STATE, 3, round + 1, json_array_size(calls), cid);
        results = chat_local_mcp_proxy_tool_calls(engine, calls, cid);
        next_body = chat_local_mcp_append_tool_results(current, calls, results,
                                                       engine->provider, engine->use_responses_api);
        json_decref(calls);
        json_decref(results);
        if (!next_body) {
            break;
        }
        free(current);
        current = next_body;
        chat_proxy_result_destroy(result);
        result = chat_proxy_send_with_retry(engine, current, &proxy_config);
    }
    free(current);
    return result;
}

char *chat_local_mcp_stream_next_body(struct MultiStreamContext *stream_ctx) {
    json_t *calls;
    json_t *results;
    char *next_body;
    const ChatEngineConfig *engine;
    const char *cid;

    if (!stream_ctx || !stream_ctx->engine || !stream_ctx->request_body) {
        return NULL;
    }
    engine = stream_ctx->engine;
    if (!engine->local_mcp.enabled) {
        return NULL;
    }
    if (stream_ctx->local_mcp_round >= CHAT_LOCAL_MCP_MAX_ROUNDS) {
        return NULL;
    }
    cid = stream_ctx->local_mcp_cid ? stream_ctx->local_mcp_cid : "-";
    calls = chat_local_mcp_finalize_accumulated(stream_ctx->tool_call_acc);
    if (!calls) {
        return NULL;
    }
    results = chat_local_mcp_proxy_tool_calls(engine, calls, cid);
    next_body = chat_local_mcp_append_tool_results(stream_ctx->request_body, calls, results,
                                                   engine->provider, engine->use_responses_api);
    json_decref(calls);
    json_decref(results);
    if (!next_body) {
        return NULL;
    }
    stream_ctx->local_mcp_round += 1;
    if (stream_ctx->tool_call_acc) {
        json_decref(stream_ctx->tool_call_acc);
        stream_ctx->tool_call_acc = NULL;
    }
    return next_body;
}
