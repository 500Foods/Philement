/*
 * Chat Request Builder - Build JSON Requests for AI APIs
 *
 * Builds request payloads for OpenAI-compatible APIs (OpenAI, xAI, Gradient, Ollama).
 * Supports temperature, max_tokens, stream parameters.
 * Auto-detects format based on provider.
 */

#ifndef REQ_BUILDER_H
#define REQ_BUILDER_H

// Project includes
#include <src/hydrogen.h>
#include <jansson.h>

// Local includes
#include "engine_cache.h"

// Message role types
typedef enum {
    CHAT_ROLE_SYSTEM = 0,
    CHAT_ROLE_USER,
    CHAT_ROLE_ASSISTANT,
    CHAT_ROLE_TOOL,
    CHAT_ROLE_UNKNOWN
} ChatMessageRole;

// Chat message structure
typedef struct ChatMessage {
    ChatMessageRole role;
    char* content;
    char* name;              // Optional name for tool messages
    char* tool_call_id;      // Optional tool call ID
    struct ChatMessage* next; // Linked list for conversation history
} ChatMessage;

// Chat request parameters
typedef struct ChatRequestParams {
    const char* model;           // Model override (NULL to use engine default)
    double temperature;          // Temperature (0.0 - 2.0, -1 for default)
    int max_tokens;              // Max tokens (-1 for default)
    bool stream;                 // Enable streaming
    json_t* additional_params;   // Additional provider-specific parameters
} ChatRequestParams;

// Function prototypes

// Message management
ChatMessage* chat_message_create(ChatMessageRole role, const char* content, const char* name);
void chat_message_destroy(ChatMessage* message);
void chat_message_list_destroy(ChatMessage* head);
ChatMessage* chat_message_list_append(ChatMessage* head, ChatMessage* new_message);

// Role helpers
const char* chat_message_role_to_string(ChatMessageRole role);
ChatMessageRole chat_message_role_from_string(const char* role_str);

// Request building
json_t* chat_request_build_openai(const ChatEngineConfig* engine,
                                   const ChatMessage* messages,
                                   const ChatRequestParams* params);

json_t* chat_request_build_anthropic(const ChatEngineConfig* engine,
                                      const ChatMessage* messages,
                                      const ChatRequestParams* params);

json_t* chat_request_build_ollama(const ChatEngineConfig* engine,
                                   const ChatMessage* messages,
                                   const ChatRequestParams* params);

// Generic request builder (auto-detects provider)
json_t* chat_request_build(const ChatEngineConfig* engine,
                           const ChatMessage* messages,
                           const ChatRequestParams* params);

// Request to JSON string
char* chat_request_to_json_string(json_t* request, bool compact);

// Token estimation
int chat_request_estimate_tokens(const ChatMessage* messages);
int chat_message_estimate_tokens(const char* content);

// Default parameters
ChatRequestParams chat_request_params_default(void);

// Validation
bool chat_request_validate(const ChatEngineConfig* engine,
                           const ChatMessage* messages,
                           const ChatRequestParams* params,
                           char** error_message);

#endif // REQ_BUILDER_H
