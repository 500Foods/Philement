/*
 * Chat Configuration Implementation
 */

#include <src/hydrogen.h>

#include "config_chat.h"
#include "config_utils.h"

void chat_config_apply_defaults(ChatConfig *config) {
    if (!config) {
        return;
    }

    config->RateLimit.Enabled = false;
    config->RateLimit.MaxRequestsPerInterval = 60;
    config->RateLimit.IntervalSeconds = 60;
    config->RateLimit.MaxTokensPerInterval = 100000;
}

bool load_chat_config(json_t *root, AppConfig *config) {
    bool success = true;
    ChatConfig *chat;

    if (!config) {
        return false;
    }

    chat = &config->chat;
    cleanup_chat_config(chat);
    chat_config_apply_defaults(chat);

    success = PROCESS_SECTION(root, "Chat");
    success = success && PROCESS_BOOL(root, chat, RateLimit.Enabled,
                                      "Chat.RateLimit.Enabled", "Chat");
    success = success && PROCESS_INT(root, chat, RateLimit.MaxRequestsPerInterval,
                                      "Chat.RateLimit.MaxRequestsPerInterval", "Chat");
    success = success && PROCESS_INT(root, chat, RateLimit.IntervalSeconds,
                                      "Chat.RateLimit.IntervalSeconds", "Chat");
    success = success && PROCESS_INT(root, chat, RateLimit.MaxTokensPerInterval,
                                      "Chat.RateLimit.MaxTokensPerInterval", "Chat");

    if (success) {
        if (chat->RateLimit.MaxRequestsPerInterval < 0) {
            log_this(SR_CONFIG,
                     "Chat.RateLimit.MaxRequestsPerInterval must be >= 0 (got %d)",
                     LOG_LEVEL_ERROR, 1, chat->RateLimit.MaxRequestsPerInterval);
            success = false;
        }
        if (chat->RateLimit.IntervalSeconds <= 0) {
            log_this(SR_CONFIG,
                     "Chat.RateLimit.IntervalSeconds must be > 0 (got %d)",
                     LOG_LEVEL_ERROR, 1, chat->RateLimit.IntervalSeconds);
            success = false;
        }
        if (chat->RateLimit.MaxTokensPerInterval < 0) {
            log_this(SR_CONFIG,
                     "Chat.RateLimit.MaxTokensPerInterval must be >= 0 (got %d)",
                     LOG_LEVEL_ERROR, 1, chat->RateLimit.MaxTokensPerInterval);
            success = false;
        }
    }

    if (success) {
        log_this(SR_CONFIG, "― Chat configuration loaded successfully", LOG_LEVEL_DEBUG, 0);
    }

    return success;
}

void dump_chat_config(const ChatConfig *config) {
    if (!config) {
        return;
    }

    log_this(SR_CONFIG_CURRENT, "Chat Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_CONFIG_CURRENT, "  RateLimit.Enabled: %s", LOG_LEVEL_DEBUG, 1,
             config->RateLimit.Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  RateLimit.MaxRequestsPerInterval: %d", LOG_LEVEL_DEBUG, 1,
             config->RateLimit.MaxRequestsPerInterval);
    log_this(SR_CONFIG_CURRENT, "  RateLimit.IntervalSeconds: %d", LOG_LEVEL_DEBUG, 1,
             config->RateLimit.IntervalSeconds);
    log_this(SR_CONFIG_CURRENT, "  RateLimit.MaxTokensPerInterval: %d", LOG_LEVEL_DEBUG, 1,
             config->RateLimit.MaxTokensPerInterval);
}

void cleanup_chat_config(ChatConfig *config) {
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(ChatConfig));
}