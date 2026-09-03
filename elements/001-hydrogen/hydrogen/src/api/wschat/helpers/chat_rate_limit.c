/*
 * Chat Rate Limiting Implementation
 *
 * See chat_rate_limit.h for design notes.
 */

#include <src/hydrogen.h>

#include "chat_rate_limit.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Count UTF-8 characters in a string. Used by the token estimator to
 * avoid over-counting multi-byte sequences.
 */
size_t chat_rate_limit_utf8_chars(const char *s) {
    if (!s) {
        return 0;
    }
    size_t n = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) {
            n++;
        }
        s++;
    }
    return n;
}

/*
 * Walk a JSON value and accumulate estimated characters. Strings
 * contribute their UTF-8 character count; objects and arrays
 * recurse; everything else contributes zero.
 */
long long chat_rate_limit_walk_json(json_t *value) {
    if (!value) {
        return 0;
    }
    if (json_is_string(value)) {
        return (long long)chat_rate_limit_utf8_chars(json_string_value(value));
    }
    if (json_is_array(value)) {
        size_t i;
        long long total = 0;
        for (i = 0; i < json_array_size(value); i++) {
            total += chat_rate_limit_walk_json(json_array_get(value, i));
        }
        return total;
    }
    if (json_is_object(value)) {
        const char *key;
        json_t *child;
        long long total = 0;
        json_object_foreach(value, key, child) {
            (void)key;
            total += chat_rate_limit_walk_json(child);
        }
        return total;
    }
    return 0;
}

long long chat_rate_limit_estimate_input_tokens(json_t *messages) {
    if (!messages || !json_is_array(messages)) {
        return 0;
    }
    long long chars = chat_rate_limit_walk_json(messages);
    long long tokens = (chars + 3) / 4;
    return tokens > 0 ? tokens : (chars > 0 ? 1 : 0);
}

/* Stable error codes for the throttle envelope. Distinct from any
 * existing conduit error_code (1002 = database not found). Documented
 * in docs/H/api/chat/auth_chat.md (Phase 10b). */
#define CHAT_RATE_LIMIT_ERROR_REQUESTS 4291
#define CHAT_RATE_LIMIT_ERROR_TOKENS   4292

json_t *chat_rate_limit_build_error_response(int error_code,
                                            const char *message) {
    json_t *response = json_object();
    if (!response) {
        return NULL;
    }
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string("rate_limited"));
    json_object_set_new(response, "message",
                        json_string(message ? message : "Request rate limit exceeded"));
    json_object_set_new(response, "error_code", json_integer(error_code));
    return response;
}

/* Single mutex protects the linked list of buckets. */
static pthread_mutex_t g_rate_limit_mutex = PTHREAD_MUTEX_INITIALIZER;
static ChatRateLimitEntry *g_rate_limit_head = NULL;
static bool g_rate_limit_initialized = false;

void chat_rate_limit_init(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    if (!g_rate_limit_initialized) {
        g_rate_limit_initialized = true;
        g_rate_limit_head = NULL;
    }
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

void chat_rate_limit_shutdown(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    ChatRateLimitEntry *entry = g_rate_limit_head;
    while (entry != NULL) {
        ChatRateLimitEntry *next = entry->next;
        free(entry->sub);
        free(entry);
        entry = next;
    }
    g_rate_limit_head = NULL;
    g_rate_limit_initialized = false;
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

void chat_rate_limit_reset_all(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    ChatRateLimitEntry *entry = g_rate_limit_head;
    while (entry != NULL) {
        ChatRateLimitEntry *next = entry->next;
        free(entry->sub);
        free(entry);
        entry = next;
    }
    g_rate_limit_head = NULL;
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

/*
 * Internal helper: look up the bucket for sub. Caller must hold
 * g_rate_limit_mutex. Returns NULL if not found.
 */
/* Look up the bucket for sub. Caller must hold g_rate_limit_mutex.
 * Returns NULL if not found.
 */
ChatRateLimitEntry *chat_rate_limit_find_locked(const char *sub) {
    if (!sub) {
        return NULL;
    }
    ChatRateLimitEntry *entry = g_rate_limit_head;
    while (entry != NULL) {
        if (entry->sub && strcmp(entry->sub, sub) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/*
 * Internal helper: allocate a new bucket for sub and link it to the
 * head. Caller must hold g_rate_limit_mutex. Returns NULL on
 * allocation failure.
 */
ChatRateLimitEntry *chat_rate_limit_new_bucket_locked(const char *sub,
                                                       time_t now) {
    ChatRateLimitEntry *entry = calloc(1, sizeof(ChatRateLimitEntry));
    if (!entry) {
        return NULL;
    }
    entry->sub = strdup(sub);
    if (!entry->sub) {
        free(entry);
        return NULL;
    }
    entry->window_start = now;
    entry->request_count = 0;
    entry->token_count = 0;
    entry->next = g_rate_limit_head;
    g_rate_limit_head = entry;
    return entry;
}

ChatRateLimitResult chat_rate_limit_check_and_record(const char *sub,
                                                     long long est_input_tokens) {
    if (!sub || sub[0] == '\0') {
        return CHAT_RATE_LIMIT_ALLOWED;
    }

    if (!app_config || !app_config->chat.RateLimit.Enabled) {
        return CHAT_RATE_LIMIT_ALLOWED;
    }

    const int max_requests = app_config->chat.RateLimit.MaxRequestsPerInterval;
    const int interval_seconds = app_config->chat.RateLimit.IntervalSeconds;
    const int max_tokens = app_config->chat.RateLimit.MaxTokensPerInterval;

    if (interval_seconds <= 0) {
        return CHAT_RATE_LIMIT_ALLOWED;
    }

    if (est_input_tokens < 0) {
        est_input_tokens = 0;
    }

    time_t now = time(NULL);

    ChatRateLimitResult result = CHAT_RATE_LIMIT_ALLOWED;

    pthread_mutex_lock(&g_rate_limit_mutex);
    if (!g_rate_limit_initialized) {
        g_rate_limit_initialized = true;
        g_rate_limit_head = NULL;
    }

    ChatRateLimitEntry *entry = chat_rate_limit_find_locked(sub);
    if (!entry) {
        entry = chat_rate_limit_new_bucket_locked(sub, now);
        if (!entry) {
            pthread_mutex_unlock(&g_rate_limit_mutex);
            return CHAT_RATE_LIMIT_ALLOWED;
        }
    }

    if ((now - entry->window_start) >= interval_seconds) {
        entry->window_start = now;
        entry->request_count = 0;
        entry->token_count = 0;
    }

    if (max_requests > 0 && entry->request_count >= max_requests) {
        result = CHAT_RATE_LIMIT_THROTTLED_REQUESTS;
    } else if (max_tokens > 0 &&
               (entry->token_count + est_input_tokens) > max_tokens) {
        result = CHAT_RATE_LIMIT_THROTTLED_TOKENS;
    } else {
        entry->request_count++;
        entry->token_count += est_input_tokens;
        result = CHAT_RATE_LIMIT_ALLOWED;
    }

    pthread_mutex_unlock(&g_rate_limit_mutex);
    return result;
}

void chat_rate_limit_record_output(const char *sub, long long output_tokens) {
    if (!sub || sub[0] == '\0' || output_tokens <= 0) {
        return;
    }

    if (!app_config || !app_config->chat.RateLimit.Enabled) {
        return;
    }

    pthread_mutex_lock(&g_rate_limit_mutex);
    if (!g_rate_limit_initialized) {
        pthread_mutex_unlock(&g_rate_limit_mutex);
        return;
    }

    ChatRateLimitEntry *entry = chat_rate_limit_find_locked(sub);
    if (entry) {
        entry->token_count += output_tokens;
    }

    pthread_mutex_unlock(&g_rate_limit_mutex);
}

int chat_rate_limit_request_count(const char *sub) {
    int count = 0;
    pthread_mutex_lock(&g_rate_limit_mutex);
    ChatRateLimitEntry *entry = chat_rate_limit_find_locked(sub);
    if (entry) {
        count = entry->request_count;
    }
    pthread_mutex_unlock(&g_rate_limit_mutex);
    return count;
}

long long chat_rate_limit_token_count(const char *sub) {
    long long count = 0;
    pthread_mutex_lock(&g_rate_limit_mutex);
    ChatRateLimitEntry *entry = chat_rate_limit_find_locked(sub);
    if (entry) {
        count = entry->token_count;
    }
    pthread_mutex_unlock(&g_rate_limit_mutex);
    return count;
}