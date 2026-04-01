/*
 * Chat Response Parser - Parse AI API Responses
 */

#include <src/hydrogen.h>
#include "resp_parser.h"

// Create parsed response
ChatParsedResponse* chat_parsed_response_create(void) {
    ChatParsedResponse* response = (ChatParsedResponse*)calloc(1, sizeof(ChatParsedResponse));
    if (!response) return NULL;
    response->success = false;
    response->content = NULL;
    response->model = NULL;
    response->finish_reason = NULL;
    response->prompt_tokens = 0;
    response->completion_tokens = 0;
    response->total_tokens = 0;
    response->error_message = NULL;
    response->raw_response = NULL;
    return response;
}

// Destroy parsed response
void chat_parsed_response_destroy(ChatParsedResponse* response) {
    if (!response) return;
    free(response->content);
    free(response->model);
    free(response->finish_reason);
    free(response->error_message);
    if (response->raw_response) {
        json_decref(response->raw_response);
    }
    free(response);
}

// Extract content from various formats
char* chat_response_extract_content(json_t* root, ChatEngineProvider provider) {
    if (!root) return NULL;

    const char* content = NULL;

    if (provider == CEC_PROVIDER_ANTHROPIC) {
        // Anthropic format: content[0].text
        json_t* content_array = json_object_get(root, "content");
        if (content_array && json_is_array(content_array)) {
            json_t* first = json_array_get(content_array, 0);
            if (first) {
                json_t* text = json_object_get(first, "text");
                if (text && json_is_string(text)) {
                    content = json_string_value(text);
                }
            }
        }
    } else if (provider == CEC_PROVIDER_OLLAMA) {
        // Ollama format: message.content
        json_t* message = json_object_get(root, "message");
        if (message) {
            json_t* msg_content = json_object_get(message, "content");
            if (msg_content && json_is_string(msg_content)) {
                content = json_string_value(msg_content);
            }
        }
        // Or direct response: response
        if (!content) {
            json_t* direct = json_object_get(root, "response");
            if (direct && json_is_string(direct)) {
                content = json_string_value(direct);
            }
        }
    } else {
        // OpenAI format: choices[0].message.content
        json_t* choices = json_object_get(root, "choices");
        if (choices && json_is_array(choices)) {
            json_t* first = json_array_get(choices, 0);
            if (first) {
                json_t* message = json_object_get(first, "message");
                if (message) {
                    json_t* msg_content = json_object_get(message, "content");
                    if (msg_content && json_is_string(msg_content)) {
                        content = json_string_value(msg_content);
                    }
                }
                // Also check for delta (streaming)
                json_t* delta = json_object_get(first, "delta");
                if (delta) {
                    json_t* delta_content = json_object_get(delta, "content");
                    if (delta_content && json_is_string(delta_content)) {
                        content = json_string_value(delta_content);
                    }
                }
            }
        }
    }

    return content ? strdup(content) : NULL;
}

// Extract tokens
bool chat_response_extract_tokens(json_t* root, int* prompt_tokens, int* completion_tokens, int* total_tokens) {
    if (!root) return false;

    bool found = false;

    // OpenAI format: usage.prompt_tokens, etc.
    json_t* usage = json_object_get(root, "usage");
    if (usage) {
        json_t* pt = json_object_get(usage, "prompt_tokens");
        json_t* ct = json_object_get(usage, "completion_tokens");
        json_t* tt = json_object_get(usage, "total_tokens");

        if (pt && json_is_integer(pt)) {
            *prompt_tokens = (int)json_integer_value(pt);
            found = true;
        }
        if (ct && json_is_integer(ct)) {
            *completion_tokens = (int)json_integer_value(ct);
            found = true;
        }
        if (tt && json_is_integer(tt)) {
            *total_tokens = (int)json_integer_value(tt);
            found = true;
        }
    }

    // Anthropic format: usage.input_tokens, usage.output_tokens
    json_t* pt = json_object_get(usage, "input_tokens");
    json_t* ct = json_object_get(usage, "output_tokens");
    if (pt && json_is_integer(pt)) {
        *prompt_tokens = (int)json_integer_value(pt);
        found = true;
    }
    if (ct && json_is_integer(ct)) {
        *completion_tokens = (int)json_integer_value(ct);
        found = true;
    }

    return found;
}

// Parse OpenAI response
ChatParsedResponse* chat_response_parse_openai(const char* json_response) {
    if (!json_response) return NULL;

    ChatParsedResponse* response = chat_parsed_response_create();
    if (!response) return NULL;

    json_error_t error;
    json_t* root = json_loads(json_response, 0, &error);
    if (!root) {
        response->error_message = strdup("Failed to parse JSON response");
        return response;
    }

    response->raw_response = root;

    // Check for error
    json_t* error_obj = json_object_get(root, "error");
    if (error_obj) {
        response->success = false;
        json_t* msg = json_object_get(error_obj, "message");
        if (msg && json_is_string(msg)) {
            response->error_message = strdup(json_string_value(msg));
        } else {
            response->error_message = strdup("API returned error");
        }
        return response;
    }

    // Extract content
    response->content = chat_response_extract_content(root, CEC_PROVIDER_OPENAI);

    // Extract model
    json_t* model = json_object_get(root, "model");
    if (model && json_is_string(model)) {
        response->model = strdup(json_string_value(model));
    }

    // Extract finish reason
    json_t* choices = json_object_get(root, "choices");
    if (choices && json_is_array(choices)) {
        json_t* first = json_array_get(choices, 0);
        if (first) {
            json_t* finish = json_object_get(first, "finish_reason");
            if (finish && json_is_string(finish)) {
                response->finish_reason = strdup(json_string_value(finish));
            }
        }
    }

    // Extract tokens
    chat_response_extract_tokens(root, &response->prompt_tokens,
                                  &response->completion_tokens,
                                  &response->total_tokens);

    response->success = (response->content != NULL);

    return response;
}

// Parse Anthropic response
ChatParsedResponse* chat_response_parse_anthropic(const char* json_response) {
    if (!json_response) return NULL;

    ChatParsedResponse* response = chat_parsed_response_create();
    if (!response) return NULL;

    json_error_t error;
    json_t* root = json_loads(json_response, 0, &error);
    if (!root) {
        response->error_message = strdup("Failed to parse JSON response");
        return response;
    }

    response->raw_response = root;

    // Check for error
    json_t* error_obj = json_object_get(root, "error");
    if (error_obj) {
        response->success = false;
        json_t* msg = json_object_get(error_obj, "message");
        if (msg && json_is_string(msg)) {
            response->error_message = strdup(json_string_value(msg));
        } else {
            response->error_message = strdup("API returned error");
        }
        return response;
    }

    // Extract content
    response->content = chat_response_extract_content(root, CEC_PROVIDER_ANTHROPIC);

    // Extract model
    json_t* model = json_object_get(root, "model");
    if (model && json_is_string(model)) {
        response->model = strdup(json_string_value(model));
    }

    // Extract stop reason (Anthropic's finish reason)
    json_t* stop = json_object_get(root, "stop_reason");
    if (stop && json_is_string(stop)) {
        response->finish_reason = strdup(json_string_value(stop));
    }

    // Extract tokens
    chat_response_extract_tokens(root, &response->prompt_tokens,
                                  &response->completion_tokens,
                                  &response->total_tokens);

    response->success = (response->content != NULL);

    return response;
}

// Parse Ollama response
ChatParsedResponse* chat_response_parse_ollama(const char* json_response) {
    if (!json_response) return NULL;

    ChatParsedResponse* response = chat_parsed_response_create();
    if (!response) return NULL;

    json_error_t error;
    json_t* root = json_loads(json_response, 0, &error);
    if (!root) {
        response->error_message = strdup("Failed to parse JSON response");
        return response;
    }

    response->raw_response = root;

    // Check for error
    json_t* error_obj = json_object_get(root, "error");
    if (error_obj && json_is_string(error_obj)) {
        response->success = false;
        response->error_message = strdup(json_string_value(error_obj));
        return response;
    }

    // Extract content
    response->content = chat_response_extract_content(root, CEC_PROVIDER_OLLAMA);

    // Extract model
    json_t* model = json_object_get(root, "model");
    if (model && json_is_string(model)) {
        response->model = strdup(json_string_value(model));
    }

    // Ollama doesn't have a standard finish_reason
    response->finish_reason = strdup("stop");

    response->success = (response->content != NULL);

    return response;
}

// Generic parser
ChatParsedResponse* chat_response_parse(const char* json_response, ChatEngineProvider provider) {
    switch (provider) {
        case CEC_PROVIDER_ANTHROPIC:
            return chat_response_parse_anthropic(json_response);
        case CEC_PROVIDER_OLLAMA:
            return chat_response_parse_ollama(json_response);
        case CEC_PROVIDER_OPENAI:
        case CEC_PROVIDER_UNKNOWN:
        default:
            return chat_response_parse_openai(json_response);
    }
}

// Destroy stream chunk
void chat_stream_chunk_destroy(ChatStreamChunk* chunk) {
    if (!chunk) return;
    free(chunk->id);
    free(chunk->content);
    free(chunk->reasoning_content);
    free(chunk->finish_reason);
    free(chunk->model);
    if (chunk->extra_fields) {
        json_decref(chunk->extra_fields);
    }
    free(chunk);
}

// Parse stream chunk
ChatStreamChunk* chat_stream_chunk_parse(const char* json_line) {
    if (!json_line || strlen(json_line) == 0) return NULL;

    // Skip "data: " prefix if present
    const char* data = json_line;
    if (strncmp(json_line, "data: ", 6) == 0) {
        data = json_line + 6;
    }
    // Skip "event: " prefix if present (Anthropic SSE)
    else if (strncmp(json_line, "event: ", 7) == 0) {
        // This is an event line, not data; skip it
        return NULL;
    }

    // Check for [DONE]
    if (strcmp(data, "[DONE]") == 0) {
        ChatStreamChunk* chunk = (ChatStreamChunk*)calloc(1, sizeof(ChatStreamChunk));
        if (chunk) {
            chunk->is_done = true;
        }
        return chunk;
    }

    json_error_t error;
    json_t* root = json_loads(data, 0, &error);
    if (!root) return NULL;

    ChatStreamChunk* chunk = (ChatStreamChunk*)calloc(1, sizeof(ChatStreamChunk));
    if (!chunk) {
        json_decref(root);
        return NULL;
    }

    // Extract ID
    json_t* id = json_object_get(root, "id");
    if (id && json_is_string(id)) {
        chunk->id = strdup(json_string_value(id));
    }

    // Extract model
    json_t* model = json_object_get(root, "model");
    if (model && json_is_string(model)) {
        chunk->model = strdup(json_string_value(model));
    }

    // Detect provider by JSON structure
    bool is_anthropic = false;
    bool is_ollama = false;
    
    // Check for Anthropic: has "type" field
    json_t* type = json_object_get(root, "type");
    if (type && json_is_string(type)) {
        const char* type_str = json_string_value(type);
        if (strcmp(type_str, "content_block_delta") == 0 ||
            strcmp(type_str, "message_start") == 0 ||
            strcmp(type_str, "message_stop") == 0) {
            is_anthropic = true;
        }
    }
    
    // Check for Ollama: has "response" field and "done" field
    json_t* response = json_object_get(root, "response");
    json_t* done = json_object_get(root, "done");
    if (response && json_is_string(response) && done && json_is_boolean(done)) {
        is_ollama = true;
    }

    if (is_anthropic) {
        // Anthropic streaming format
        if (type) {
            const char* type_str = json_string_value(type);
            if (strcmp(type_str, "content_block_delta") == 0) {
                json_t* delta = json_object_get(root, "delta");
                if (delta) {
                    json_t* text = json_object_get(delta, "text");
                    if (text && json_is_string(text)) {
                        chunk->content = strdup(json_string_value(text));
                    }
                }
            } else if (strcmp(type_str, "message_stop") == 0) {
                chunk->is_done = true;
            }
            // For other event types, content remains NULL
        }
    } else if (is_ollama) {
        // Ollama streaming format
        if (response && json_is_string(response)) {
            chunk->content = strdup(json_string_value(response));
        }
        if (done && json_is_boolean(done) && json_boolean_value(done)) {
            chunk->is_done = true;
            // Extract finish_reason if present
            json_t* done_reason = json_object_get(root, "done_reason");
            if (done_reason && json_is_string(done_reason)) {
                chunk->finish_reason = strdup(json_string_value(done_reason));
            }
        }
    } else {
        // OpenAI format (original)
        json_t* choices = json_object_get(root, "choices");
        if (choices && json_is_array(choices)) {
            json_t* first = json_array_get(choices, 0);
            if (first) {
                // Check for delta content
                json_t* delta = json_object_get(first, "delta");
                if (delta) {
                    json_t* content = json_object_get(delta, "content");
                    if (content && json_is_string(content)) {
                        chunk->content = strdup(json_string_value(content));
                    }
                    // Check for reasoning_content (e.g., Kimi K2.5)
                    json_t* reasoning = json_object_get(delta, "reasoning_content");
                    if (reasoning && json_is_string(reasoning)) {
                        chunk->reasoning_content = strdup(json_string_value(reasoning));
                    }
                }

                // Check for finish reason
                json_t* finish = json_object_get(first, "finish_reason");
                if (finish && json_is_string(finish)) {
                    chunk->finish_reason = strdup(json_string_value(finish));
                }
            }
        }

        // Check for provider-specific extra fields (e.g., retrieval, citations)
        // These are typically at the root level, not in choices
        json_t* retrieval = json_object_get(root, "retrieval");
        if (retrieval) {
            if (!chunk->extra_fields) {
                chunk->extra_fields = json_object();
            }
            json_object_set(chunk->extra_fields, "retrieval", retrieval);
        }

        // Also check for citations at root level
        json_t* citations = json_object_get(root, "citations");
        if (citations) {
            if (!chunk->extra_fields) {
                chunk->extra_fields = json_object();
            }
            json_object_set(chunk->extra_fields, "citations", citations);
        }
    }

    json_decref(root);
    return chunk;
}

// Extract error from response
char* chat_response_extract_error(const char* json_response) {
    if (!json_response) return NULL;

    json_error_t error;
    json_t* root = json_loads(json_response, 0, &error);
    if (!root) return NULL;

    char* result = NULL;

    json_t* error_obj = json_object_get(root, "error");
    if (error_obj) {
        json_t* msg = json_object_get(error_obj, "message");
        if (msg && json_is_string(msg)) {
            result = strdup(json_string_value(msg));
        } else if (json_is_string(error_obj)) {
            result = strdup(json_string_value(error_obj));
        }
    }

    json_decref(root);
    return result;
}
