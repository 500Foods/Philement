/*
 * Chat Request Builder - Build JSON Requests for AI APIs
 */

#include <src/hydrogen.h>
#include "chat_request_builder.h"

// Approximate tokens per character (rough estimate: 1 token ~= 4 chars)
#define TOKEN_ESTIMATE_CHARS_PER_TOKEN 4

// Default parameters
ChatRequestParams chat_request_params_default(void) {
    ChatRequestParams params;
    params.model = NULL;
    params.temperature = -1.0;  // Use engine default
    params.max_tokens = -1;     // Use engine default
    params.stream = false;
    params.additional_params = NULL;
    return params;
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

// Token estimation (rough estimate)
int chat_message_estimate_tokens(const char* content) {
    if (!content) return 0;
    return (int)(strlen(content) / TOKEN_ESTIMATE_CHARS_PER_TOKEN) + 1;
}

int chat_request_estimate_tokens(const ChatMessage* messages) {
    int total = 0;
    const ChatMessage* current = messages;
    while (current) {
        total += chat_message_estimate_tokens(current->content);
        total += 4;  // Message overhead
        current = current->next;
    }
    return total + 2;  // Base overhead
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

    // Temperature
    if (params->temperature >= 0.0) {
        json_object_set_new(root, "temperature", json_real(params->temperature));
    } else if (engine->temperature_default >= 0.0) {
        json_object_set_new(root, "temperature", json_real(engine->temperature_default));
    }

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

    // Additional params
    if (params->additional_params) {
        const char* key;
        json_t* value;
        json_object_foreach(params->additional_params, key, value) {
            json_object_set(root, key, value);
        }
    }

    return root;
}

// Build Anthropic request (simplified)
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

    // Temperature
    if (params->temperature >= 0.0) {
        json_object_set_new(root, "temperature", json_real(params->temperature));
    } else if (engine->temperature_default >= 0.0) {
        json_object_set_new(root, "temperature", json_real(engine->temperature_default));
    }

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
    }

    return root;
}

// Build Ollama request
json_t* chat_request_build_ollama(const ChatEngineConfig* engine,
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

    // Options
    json_t* options = json_object();
    if (params->temperature >= 0.0) {
        json_object_set_new(options, "temperature", json_real(params->temperature));
    } else if (engine->temperature_default >= 0.0) {
        json_object_set_new(options, "temperature", json_real(engine->temperature_default));
    }
    json_object_set_new(root, "options", options);

    // Stream
    if (params->stream) {
        json_object_set_new(root, "stream", json_true());
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
            return chat_request_build_ollama(engine, messages, params);
        case CEC_PROVIDER_OPENAI:
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

// Validate request
bool chat_request_validate(const ChatEngineConfig* engine,
                           const ChatMessage* messages,
                           const ChatRequestParams* params,
                           char** error_message) {
    if (!engine) {
        if (error_message) *error_message = strdup("No engine configuration");
        return false;
    }
    if (!messages) {
        if (error_message) *error_message = strdup("No messages provided");
        return false;
    }

    // Check token limit
    int estimated_tokens = chat_request_estimate_tokens(messages);
    if (params->max_tokens > 0) {
        estimated_tokens += params->max_tokens;
    }
    if (engine->max_tokens > 0 && estimated_tokens > engine->max_tokens) {
        if (error_message) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Estimated tokens (%d) exceeds engine limit (%d)",
                     estimated_tokens, engine->max_tokens);
            *error_message = strdup(buf);
        }
        return false;
    }

    (void)params;
    return true;
}
