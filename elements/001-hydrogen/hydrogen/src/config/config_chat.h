/*
 * Chat Configuration
 *
 * JSON section "Chat" (AppConfig letter U). Holds chat subsystem
 * settings that are not engine-specific. Engine settings (model name,
 * API key, use_responses_api, store, etc.) are loaded by the engine
 * cache from per-database rows.
 *
 * Phase 10b (CHAT_FINALE) introduces RateLimit. Disabled by default
 * (fails open). When Enabled=false, chat_rate_limit_check() short-
 * circuits and never blocks a request.
 */

#ifndef HYDROGEN_CONFIG_CHAT_H
#define HYDROGEN_CONFIG_CHAT_H

#include <src/globals.h>

#include <stddef.h>
#include <stdbool.h>

#include <jansson.h>

#include "config_forward.h"

typedef struct ChatRateLimitConfig {
    bool Enabled;
    int MaxRequestsPerInterval;
    int IntervalSeconds;
    int MaxTokensPerInterval;
} ChatRateLimitConfig;

typedef struct ChatConfig {
    ChatRateLimitConfig RateLimit;
} ChatConfig;

bool load_chat_config(json_t *root, AppConfig *config);
void dump_chat_config(const ChatConfig *config);
void cleanup_chat_config(ChatConfig *config);
void chat_config_apply_defaults(ChatConfig *config);

#endif /* HYDROGEN_CONFIG_CHAT_H */