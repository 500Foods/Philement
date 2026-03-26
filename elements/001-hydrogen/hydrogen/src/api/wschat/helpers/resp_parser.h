/*
 * Chat Response Parser - Parse AI API Responses
 *
 * Parses OpenAI-compatible responses into standard format.
 * Extracts content, tokens, model, finish_reason.
 * Handles error responses appropriately.
 */

#ifndef RESP_PARSER_H
#define RESP_PARSER_H

// Project includes
#include <src/hydrogen.h>
#include <jansson.h>

// Local includes
#include "engine_cache.h"

// Parsed response structure
typedef struct ChatParsedResponse {
    bool success;               // Whether parsing succeeded
    char* content;              // Response content (assistant message)
    char* model;                // Model used
    char* finish_reason;        // Finish reason (e.g., "stop", "length")
    int prompt_tokens;          // Tokens in prompt
    int completion_tokens;      // Tokens in completion
    int total_tokens;           // Total tokens used
    char* error_message;        // Error message if success is false
    json_t* raw_response;       // Full raw response (for advanced use)
} ChatParsedResponse;

// Stream chunk structure (for streaming responses)
typedef struct ChatStreamChunk {
    char* id;                   // Chunk ID
    char* content;              // Delta content
    char* reasoning_content;    // Reasoning/thinking content (e.g., Kimi K2.5)
    char* finish_reason;        // Finish reason (if complete)
    char* model;                // Model name
    bool is_done;               // Whether this is the final chunk
} ChatStreamChunk;

// Function prototypes

// Create and destroy parsed response
ChatParsedResponse* chat_parsed_response_create(void);
void chat_parsed_response_destroy(ChatParsedResponse* response);

// Parse OpenAI-compatible response
ChatParsedResponse* chat_response_parse_openai(const char* json_response);

// Parse Anthropic response
ChatParsedResponse* chat_response_parse_anthropic(const char* json_response);

// Parse Ollama response
ChatParsedResponse* chat_response_parse_ollama(const char* json_response);

// Generic parser (auto-detects format based on structure)
ChatParsedResponse* chat_response_parse(const char* json_response, ChatEngineProvider provider);

// Stream chunk parsing
ChatStreamChunk* chat_stream_chunk_parse(const char* json_line);
void chat_stream_chunk_destroy(ChatStreamChunk* chunk);

// Error extraction from failed responses
char* chat_response_extract_error(const char* json_response);

// Token extraction helpers
bool chat_response_extract_tokens(json_t* root, int* prompt_tokens, int* completion_tokens, int* total_tokens);

// Content extraction from various response formats
char* chat_response_extract_content(json_t* root, ChatEngineProvider provider);

#endif // RESP_PARSER_H
