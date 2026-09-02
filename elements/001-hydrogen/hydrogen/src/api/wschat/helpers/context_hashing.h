/*
 * Chat Context Hashing - Client-Server Optimization (Phase 8)
 */

#ifndef CONTEXT_HASHING_H
#define CONTEXT_HASHING_H

#include <src/hydrogen.h>
#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>

#define CHAT_CONTEXT_MAX_HASH_LEN    64
#define CHAT_CONTEXT_MAX_HASHES      100

bool chat_context_validate_hash(const char* hash);

char** chat_context_parse_request_hashes(json_t* request_json, char** error_message);

void chat_context_free_hash_array(char** hashes, size_t count);

#endif // CONTEXT_HASHING_H
