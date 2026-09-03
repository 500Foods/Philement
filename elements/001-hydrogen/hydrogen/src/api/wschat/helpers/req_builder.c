/*
 * Chat Request Builder - Build JSON Requests for AI APIs
 */

#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/mcp/mcp_mint_token.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_resource_url.h>
#include "req_builder.h"
#include "local_mcp.h"

// Default parameters
ChatRequestParams chat_request_params_default(void) {
    ChatRequestParams params;
    params.model = NULL;
    params.temperature = -1.0;  // Use engine default
    params.max_tokens = -1;     // Use engine default
    params.stream = false;
    params.reasoning = NULL;
    params.additional_params = NULL;
    params.hosted_mcp_enabled = false;
    params.hosted_mcp_sub = NULL;
    params.hosted_mcp_database = NULL;
    params.hosted_mcp_roles = NULL;
    params.hosted_mcp_ttl_seconds = 0;
    params.hosted_mcp_correlation_id = NULL;
    params.local_mcp_tools = NULL;
    return params;
}

void chat_correlation_id_generate(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 37) {
        if (buffer && buffer_size > 0) {
            buffer[0] = '\0';
        }
        return;
    }
    const char *hex = "0123456789abcdef";
    size_t idx = 0;
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            buffer[idx++] = '-';
        } else {
            buffer[idx++] = hex[(unsigned)rand() % 16];
        }
    }
    buffer[14] = '4';
    buffer[19] = hex[8 + ((unsigned)rand() % 4)];
    buffer[36] = '\0';
}

void chat_request_params_apply_hosted_mcp(ChatRequestParams *params,
                                          const ChatEngineConfig *engine,
                                          const char *sub,
                                          const char *database,
                                          const char *roles,
                                          const char *correlation_id) {
    if (!params || !engine || !engine->use_responses_api) {
        return;
    }
    params->hosted_mcp_enabled = true;
    params->hosted_mcp_sub = sub;
    params->hosted_mcp_database = database;
    params->hosted_mcp_roles = roles;
    params->hosted_mcp_ttl_seconds = 0;
    params->hosted_mcp_correlation_id = correlation_id;
}

json_t *chat_request_params_apply_local_mcp(ChatRequestParams *params,
                                            const ChatEngineConfig *engine,
                                            const char *correlation_id) {
    json_t *tools;

    if (!params || !engine || !engine->local_mcp.enabled) {
        return NULL;
    }
    tools = chat_local_mcp_list_tools(engine, correlation_id);
    params->local_mcp_tools = tools;
    return tools;
}

// Message management
ChatMessage* chat_message_create(ChatMessageRole role, const char* content, const char* name) {
    ChatMessage* msg = (ChatMessage*)calloc(1, sizeof(ChatMessage));
    if (!msg) return NULL;

    msg->role = role;
    msg->content = content ? strdup(content) : NULL;
    msg->name = name ? strdup(name) : NULL;
    msg->tool_call_id = NULL;
    msg->next = NULL;

    return msg;
}

void chat_message_destroy(ChatMessage* message) {
    if (!message) return;
    free(message->content);
    free(message->name);
    free(message->tool_call_id);
    free(message);
}

void chat_message_list_destroy(ChatMessage* head) {
    while (head) {
        ChatMessage* next = head->next;
        chat_message_destroy(head);
        head = next;
    }
}

ChatMessage* chat_message_list_append(ChatMessage* head, ChatMessage* new_message) {
    if (!new_message) return head;
    if (!head) return new_message;

    ChatMessage* current = head;
    while (current->next) {
        current = current->next;
    }
    current->next = new_message;
    return head;
}

// Role helpers
const char* chat_message_role_to_string(ChatMessageRole role) {
    switch (role) {
        case CHAT_ROLE_SYSTEM: return "system";
        case CHAT_ROLE_USER: return "user";
        case CHAT_ROLE_ASSISTANT: return "assistant";
        case CHAT_ROLE_TOOL: return "tool";
        case CHAT_ROLE_UNKNOWN:
        default: return "user";
    }
}

ChatMessageRole chat_message_role_from_string(const char* role_str) {
    if (!role_str) return CHAT_ROLE_USER;
    if (strcasecmp(role_str, "system") == 0) return CHAT_ROLE_SYSTEM;
    if (strcasecmp(role_str, "user") == 0) return CHAT_ROLE_USER;
    if (strcasecmp(role_str, "assistant") == 0) return CHAT_ROLE_ASSISTANT;
    if (strcasecmp(role_str, "tool") == 0) return CHAT_ROLE_TOOL;
    return CHAT_ROLE_UNKNOWN;
}

// Build OpenAI-compatible request
json_t* chat_request_build_openai(const ChatEngineConfig* engine,
                                   const ChatMessage* messages,
                                   const ChatRequestParams* params) {
    if (!engine || !messages) return NULL;

    json_t* root = json_object();
    if (!root) return NULL;

    // Model
    const char* model = params->model ? params->model : engine->model;
    json_object_set_new(root, "model", json_string(model));

    // Messages array
    json_t* messages_array = json_array();
    const ChatMessage* current = messages;
    while (current) {
        json_t* msg_obj = json_object();
        json_object_set_new(msg_obj, "role", json_string(chat_message_role_to_string(current->role)));
        json_object_set_new(msg_obj, "content", json_string(current->content ? current->content : ""));
        json_array_append_new(messages_array, msg_obj);
        current = current->next;
    }
    json_object_set_new(root, "messages", messages_array);
 
    // Temperature - use client value if provided, otherwise engine default
    double temperature = (params->temperature >= 0.0) ? params->temperature : engine->temperature_default;
    json_object_set_new(root, "temperature", json_real(temperature));

    // Max tokens
    if (params->max_tokens > 0) {
        json_object_set_new(root, "max_tokens", json_integer(params->max_tokens));
    } else if (engine->max_tokens > 0) {
        json_object_set_new(root, "max_tokens", json_integer(engine->max_tokens));
    }

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
    }

    // Request retrieval info for RAG citations (e.g., DigitalOcean GradientAI)
    // This is safe to include for all providers - unused ones will ignore it
    json_object_set_new(root, "include_retrieval_info", json_true());

    chat_request_append_local_mcp_tools(root, params->local_mcp_tools, CEC_PROVIDER_OPENAI, false);

    // Additional params overlay (merges last — explicit fields set the base)
    if (params->additional_params) {
        const char* key;
        json_t* value;
        json_object_foreach(params->additional_params, key, value) {
            json_object_set(root, key, value);
        }
    }

    return root;
}

// Helper: Convert OpenAI image_url format to Anthropic image format
 json_t* chat_request_convert_openai_content_to_anthropic(const char* content_str) {
    if (!content_str) return NULL;
    
    json_error_t error;
    json_t* content_array = json_loads(content_str, 0, &error);
    if (!content_array || !json_is_array(content_array)) {
        if (content_array) json_decref(content_array);
        return NULL;  // Not JSON array, treat as plain text
    }
    
    json_t* new_array = json_array();
    if (!new_array) {
        json_decref(content_array);
        return NULL;
    }
    
    size_t index;
    json_t* item;
    bool success = true;
    
    json_array_foreach(content_array, index, item) {
        if (!json_is_object(item)) {
            json_array_append(new_array, item);
            continue;
        }
        
        json_t* type = json_object_get(item, "type");
        if (!type || !json_is_string(type)) {
            json_array_append(new_array, item);
            continue;
        }
        
        const char* type_str = json_string_value(type);
        json_t* new_item = NULL;
        
        if (strcmp(type_str, "text") == 0) {
            // Text part, keep as is
            new_item = json_deep_copy(item);
        } else if (strcmp(type_str, "image_url") == 0) {
            // Convert to Anthropic image format
            json_t* image_url = json_object_get(item, "image_url");
            if (image_url && json_is_object(image_url)) {
                json_t* url = json_object_get(image_url, "url");
                if (url && json_is_string(url)) {
                    const char* url_str = json_string_value(url);
                    // Determine if it's a data URL
                    if (strncmp(url_str, "data:", 5) == 0) {
                        // Parse data URL: data:<mediatype>;base64,<data>
                        const char* comma = strchr(url_str + 5, ',');
                        if (comma) {
                            // Extract mime type (skip "data:")
                            char* mime_start = strndup(url_str + 5, (size_t)(comma - (url_str + 5)));
                            // Trim any ";base64" (or other parameter) suffix so
                            // only the media type itself remains in mime_start.
                            char* semicolon = strchr(mime_start, ';');
                            if (semicolon) *semicolon = '\0';

                            const char* base64_data = comma + 1;

                            new_item = json_object();
                            json_object_set(new_item, "type", json_string("image"));
                            json_t* source = json_object();
                            json_object_set(source, "type", json_string("base64"));
                            json_object_set(source, "media_type", json_string(mime_start));
                            json_object_set(source, "data", json_string(base64_data));
                            json_object_set(new_item, "source", source);
                            json_decref(source);
                            free(mime_start);
                        }
                    }
                }
            }
            // If conversion failed, keep original
            if (!new_item) {
                new_item = json_deep_copy(item);
            }
        } else {
            // Unknown type, keep as is
            new_item = json_deep_copy(item);
        }
        
        if (new_item) {
            json_array_append(new_array, new_item);
            json_decref(new_item);
        } else {
            success = false;
            break;
        }
    }
    
    json_decref(content_array);
    
    if (!success) {
        json_decref(new_array);
        return NULL;
    }
    
    return new_array;
}

// Build Anthropic native format request
json_t* chat_request_build_anthropic(const ChatEngineConfig* engine,
                                       const ChatMessage* messages,
                                       const ChatRequestParams* params) {
    if (!engine || !messages) return NULL;

    json_t* root = json_object();
    if (!root) return NULL;

    // Model
    const char* model = params->model ? params->model : engine->model;
    json_object_set_new(root, "model", json_string(model));

    // Max tokens (required for Anthropic)
    int max_tokens = params->max_tokens > 0 ? params->max_tokens : engine->max_tokens;
    if (max_tokens <= 0) max_tokens = 4096;  // Default for Anthropic
    json_object_set_new(root, "max_tokens", json_integer(max_tokens));

    // Temperature - use client value if provided, otherwise engine default
    double temperature = (params->temperature >= 0.0) ? params->temperature : engine->temperature_default;
    json_object_set_new(root, "temperature", json_real(temperature));

    // Extract system message and build messages array
    const char* system_content = NULL;
    json_t* messages_array = json_array();
    const ChatMessage* current = messages;
    while (current) {
        if (current->role == CHAT_ROLE_SYSTEM) {
            // Anthropic uses separate system field, not a message
            system_content = current->content;
        } else {
            // Only add user and assistant messages to the array
            json_t* msg_obj = json_object();
            const char* role_str = (current->role == CHAT_ROLE_ASSISTANT) ? "assistant" : "user";
            json_object_set_new(msg_obj, "role", json_string(role_str));
            
            // Process content: convert OpenAI image_url to Anthropic image format if needed
            json_t* content_obj = NULL;
            if (current->content) {
                // Try to parse as JSON array (multimodal)
                content_obj = chat_request_convert_openai_content_to_anthropic(current->content);
            }
            if (content_obj) {
                json_object_set_new(msg_obj, "content", content_obj);
            } else {
                // Plain text content
                json_object_set_new(msg_obj, "content", json_string(current->content ? current->content : ""));
            }
            json_array_append_new(messages_array, msg_obj);
        }
        current = current->next;
    }

    // Add system field if found
    if (system_content) {
        json_object_set_new(root, "system", json_string(system_content));
    }

    json_object_set_new(root, "messages", messages_array);

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
    }

    // Request retrieval info for RAG citations
    json_object_set_new(root, "include_retrieval_info", json_true());

    chat_request_append_local_mcp_tools(root, params->local_mcp_tools, CEC_PROVIDER_ANTHROPIC, false);

    // Additional params overlay (merges last — explicit fields set the base)
    if (params->additional_params) {
        const char* key;
        json_t* value;
        json_object_foreach(params->additional_params, key, value) {
            json_object_set(root, key, value);
        }
    }

    return root;
}

// Build Ollama native format request
// Ollama native API: POST /api/chat
// Uses "num_predict" instead of "max_tokens" in options
json_t* chat_request_build_ollama(const ChatEngineConfig* engine,
                                    const ChatMessage* messages,
                                    const ChatRequestParams* params) {
    if (!engine || !messages) return NULL;

    json_t* root = json_object();
    if (!root) return NULL;

    // Model
    const char* model = params->model ? params->model : engine->model;
    json_object_set_new(root, "model", json_string(model));

    // Messages array (same as OpenAI format)
    json_t* messages_array = json_array();
    const ChatMessage* current = messages;
    while (current) {
        json_t* msg_obj = json_object();
        json_object_set_new(msg_obj, "role", json_string(chat_message_role_to_string(current->role)));
        json_object_set_new(msg_obj, "content", json_string(current->content ? current->content : ""));
        json_array_append_new(messages_array, msg_obj);
        current = current->next;
    }
    json_object_set_new(root, "messages", messages_array);

    // Options (Ollama uses num_predict instead of max_tokens)
    json_t* options = json_object();

    // Temperature - use client value if provided, otherwise engine default
    double temperature = (params->temperature >= 0.0) ? params->temperature : engine->temperature_default;
    json_object_set_new(options, "temperature", json_real(temperature));

    // Ollama uses num_predict for max_tokens
    int max_tokens = params->max_tokens > 0 ? params->max_tokens : engine->max_tokens;
    if (max_tokens > 0) {
        json_object_set_new(options, "num_predict", json_integer(max_tokens));
    }

    json_object_set_new(root, "options", options);

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
    }

    chat_request_append_local_mcp_tools(root, params->local_mcp_tools, CEC_PROVIDER_OLLAMA, false);

    // Additional params overlay (merges last — explicit fields set the base)
    if (params->additional_params) {
        const char* key;
        json_t* value;
        json_object_foreach(params->additional_params, key, value) {
            json_object_set(root, key, value);
        }
    }

    return root;
}

// Resolve ChatRequestParams from optional request fields and engine defaults
ChatRequestParams chat_resolve_request_params(const ChatEngineConfig* engine,
                                               double temperature,
                                               int max_tokens,
                                               bool stream,
                                               const char* reasoning) {
    ChatRequestParams params = chat_request_params_default();
    if (!engine) {
        params.stream = stream;
        return params;
    }
    params.temperature = (temperature >= 0.0) ? temperature : engine->temperature_default;
    params.max_tokens = (max_tokens > 0) ? max_tokens : engine->max_tokens;
    params.stream = stream;
    if (reasoning && strlen(reasoning) > 0) {
        params.reasoning = (char*)reasoning;
    }
    return params;
}

// Build Responses API request (xAI/OpenAI)
// Wire format: input (messages), max_output_tokens, temperature, tools
json_t* chat_request_build_messages_array(const ChatMessage* messages) {
    json_t* messages_array = json_array();
    const ChatMessage* current = messages;
    while (current) {
        json_t* msg_obj = json_object();
        json_object_set_new(msg_obj, "role", json_string(chat_message_role_to_string(current->role)));
        json_object_set_new(msg_obj, "content", json_string(current->content ? current->content : ""));
        json_array_append_new(messages_array, msg_obj);
        current = current->next;
    }
    return messages_array;
}

json_t* chat_request_build_responses(const ChatEngineConfig* engine,
                                      const ChatMessage* messages,
                                      const ChatRequestParams* params) {
    if (!engine || !messages) return NULL;

    json_t* root = json_object();
    if (!root) return NULL;

    // Model
    const char* model = params->model ? params->model : engine->model;
    json_object_set_new(root, "model", json_string(model));

    // Input (Responses API uses "input" instead of "messages")
    json_t* input_array = chat_request_build_messages_array(messages);
    json_object_set_new(root, "input", input_array);

    // Temperature - use client value if provided, otherwise engine default
    double temperature = (params->temperature >= 0.0) ? params->temperature : engine->temperature_default;
    json_object_set_new(root, "temperature", json_real(temperature));

    // Max output tokens (Responses API uses "max_output_tokens")
    int max_tokens = params->max_tokens > 0 ? params->max_tokens : engine->max_tokens;
    if (max_tokens > 0) {
        json_object_set_new(root, "max_output_tokens", json_integer(max_tokens));
    }

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
    }

    // Reasoning effort (Responses API: "reasoning" object with "effort" field)
    if (params->reasoning) {
        json_t* reasoning_obj = json_object();
        json_object_set_new(reasoning_obj, "effort", json_string(params->reasoning));
        json_object_set_new(root, "reasoning", reasoning_obj);
    }

    chat_request_append_local_mcp_tools(root, params->local_mcp_tools, CEC_PROVIDER_OPENAI, true);

    // Hosted MCP (Phase 8b): append `type: mcp` connector so the
    // provider (xAI/OpenAI Responses API) can call public MCP on the
    // chat user's behalf. Mint a fresh short-TTL `aud` token per
    // request, fail-closed on unreachable MCP URL or mint failure.
    if (params->hosted_mcp_enabled) {
        const MCPConfig *mcp_cfg = app_config ? &app_config->mcp : NULL;
        const char *server_url = mcp_cfg ? mcp_auth_resource(mcp_cfg) : "";
        const char *cid = params->hosted_mcp_correlation_id ? params->hosted_mcp_correlation_id : "-";

        if (!mcp_cfg || !server_url || !server_url[0]) {
            log_this("CHAT",
                     "hosted_mcp: cfg has no MCP.Resource (cid=%s) — request fails closed",
                     LOG_LEVEL_ERROR, 1, cid);
            json_decref(root);
            return NULL;
        }
        if (!mcp_mcp_resource_url_is_reachable(server_url)) {
            log_this("CHAT",
                     "hosted_mcp: resource url unreachable (loopback/internal/non-https) url=%s (cid=%s) — request fails closed",
                     LOG_LEVEL_ERROR, 2, server_url, cid);
            json_decref(root);
            return NULL;
        }
        char *jwt = mcp_mint_resource_token(mcp_cfg,
                                           params->hosted_mcp_sub,
                                           params->hosted_mcp_database,
                                           params->hosted_mcp_roles,
                                           params->hosted_mcp_ttl_seconds,
                                           cid);
        if (!jwt) {
            log_this("CHAT",
                     "hosted_mcp: mint failed (cid=%s) — request fails closed",
                     LOG_LEVEL_ERROR, 1, cid);
            json_decref(root);
            return NULL;
        }
        char *authz = NULL;
        if (asprintf(&authz, "Bearer %s", jwt) == -1) {
            authz = NULL;
        }
        free(jwt);
        if (!authz) {
            log_this("CHAT",
                     "hosted_mcp: auth header alloc failed (cid=%s)",
                     LOG_LEVEL_ERROR, 1, cid);
            json_decref(root);
            return NULL;
        }

        json_t *mcp_obj = json_object();
        json_object_set_new(mcp_obj, "type", json_string("mcp"));
        json_object_set_new(mcp_obj, "server_label", json_string("hydrogen"));
        json_object_set_new(mcp_obj, "server_url", json_string(server_url));
        json_object_set_new(mcp_obj, "authorization", json_string(authz));
        json_t *allowed = json_array();
        json_array_append_new(allowed, json_string("System.Info"));
        json_object_set_new(mcp_obj, "allowed_tools", allowed);
        free(authz);

        json_t *tools_array = json_object_get(root, "tools");
        if (!tools_array || !json_is_array(tools_array)) {
            tools_array = json_array();
            json_object_set_new(root, "tools", tools_array);
        }
        json_array_append_new(tools_array, mcp_obj);

        log_this("CHAT",
                 "hosted_mcp: connector injected sub=%s db=%s tools=System.Info (cid=%s)",
                 LOG_LEVEL_STATE, 3,
                 params->hosted_mcp_sub ? params->hosted_mcp_sub : "",
                 params->hosted_mcp_database ? params->hosted_mcp_database : "",
                 cid);
    }

    // Additional params overlay (merges last — explicit fields set the base)
    if (params->additional_params) {
        const char* key;
        json_t* value;
        json_object_foreach(params->additional_params, key, value) {
            json_object_set(root, key, value);
        }
    }

    return root;
}

// Generic request builder
json_t* chat_request_build(const ChatEngineConfig* engine,
                           const ChatMessage* messages,
                           const ChatRequestParams* params) {
    if (!engine || !messages) return NULL;

    switch (engine->provider) {
        case CEC_PROVIDER_ANTHROPIC:
            return chat_request_build_anthropic(engine, messages, params);
        case CEC_PROVIDER_OLLAMA:
            // Ollama can use native format or OpenAI-compatible format
            if (engine->use_native_api) {
                return chat_request_build_ollama(engine, messages, params);
            }
            // Fall through to OpenAI format for Ollama when not using native API
            return chat_request_build_openai(engine, messages, params);
        case CEC_PROVIDER_OPENAI:
            // xAI/OpenAI use Responses API when configured
            if (engine->use_responses_api) {
                return chat_request_build_responses(engine, messages, params);
            }
            return chat_request_build_openai(engine, messages, params);
        case CEC_PROVIDER_UNKNOWN:
        default:
            return chat_request_build_openai(engine, messages, params);
    }
}

// Convert to JSON string
char* chat_request_to_json_string(json_t* request, bool compact) {
    if (!request) return NULL;
    return json_dumps(request, compact ? JSON_COMPACT : JSON_INDENT(2));
}


