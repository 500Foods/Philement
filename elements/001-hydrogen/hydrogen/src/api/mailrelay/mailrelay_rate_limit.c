/*
 * Mail Relay API Rate Limiting Implementation
 *
 * See mailrelay_rate_limit.h for design notes.
 */

#include <src/hydrogen.h>

#include "mailrelay_rate_limit.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Single mutex protects the linked list of buckets. */
static pthread_mutex_t g_rate_limit_mutex = PTHREAD_MUTEX_INITIALIZER;
static MailRelayApiRateLimitEntry* g_rate_limit_head = NULL;
static bool g_rate_limit_initialized = false;

void mailrelay_rate_limit_init(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    if (!g_rate_limit_initialized) {
        g_rate_limit_initialized = true;
        g_rate_limit_head = NULL;
    }
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

void mailrelay_rate_limit_shutdown(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    MailRelayApiRateLimitEntry* entry = g_rate_limit_head;
    while (entry != NULL) {
        MailRelayApiRateLimitEntry* next = entry->next;
        free(entry->key);
        free(entry);
        entry = next;
    }
    g_rate_limit_head = NULL;
    g_rate_limit_initialized = false;
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

void mailrelay_rate_limit_reset_all(void) {
    pthread_mutex_lock(&g_rate_limit_mutex);
    MailRelayApiRateLimitEntry* entry = g_rate_limit_head;
    while (entry != NULL) {
        MailRelayApiRateLimitEntry* next = entry->next;
        free(entry->key);
        free(entry);
        entry = next;
    }
    g_rate_limit_head = NULL;
    pthread_mutex_unlock(&g_rate_limit_mutex);
}

/*
 * Build the rate-limit bucket key from the configured scope. Returns NULL
 * for global scope (single bucket). Returns a malloc'd string otherwise.
 */
char* mailrelay_rate_limit_build_key(const char* sub,
                                     const char* client_ip,
                                     const char* template_key) {
    if (!app_config) {
        return NULL;
    }

    int scope = app_config->mail_relay.RateLimit.Scope;

    if (scope == MAIL_RL_SCOPE_GLOBAL) {
        return NULL;
    }

    const char* primary = NULL;
    if (scope == MAIL_RL_SCOPE_USER) {
        primary = sub;
    } else if (scope == MAIL_RL_SCOPE_IP) {
        primary = client_ip;
    } else if (scope == MAIL_RL_SCOPE_TEMPLATE) {
        primary = template_key;
    }

    if (!primary || primary[0] == '\0') {
        return NULL;
    }

    return strdup(primary);
}

MailRelayApiRateLimitEntry* mailrelay_rate_limit_find_locked(const char* key) {
    MailRelayApiRateLimitEntry* entry = g_rate_limit_head;
    while (entry != NULL) {
        if (key == NULL && entry->key == NULL) {
            return entry;
        }
        if (key && entry->key && strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

MailRelayApiRateLimitEntry* mailrelay_rate_limit_new_bucket_locked(const char* key,
                                                                time_t now) {
    MailRelayApiRateLimitEntry* entry = calloc(1, sizeof(MailRelayApiRateLimitEntry));
    if (!entry) {
        return NULL;
    }
    if (key) {
        entry->key = strdup(key);
        if (!entry->key) {
            free(entry);
            return NULL;
        }
    } else {
        entry->key = NULL;
    }
    entry->window_start = now;
    entry->count = 0;
    entry->next = g_rate_limit_head;
    g_rate_limit_head = entry;
    return entry;
}

MailRelayRateLimitResult mailrelay_rate_limit_check_and_record(const char* sub,
                                                               const char* client_ip,
                                                               const char* template_key) {
    if (!app_config || !app_config->mail_relay.RateLimit.Enabled) {
        return MAIL_RELAY_RATE_ALLOWED;
    }

    const int max_requests = app_config->mail_relay.RateLimit.MaxRequestsPerInterval;
    const int interval_seconds = app_config->mail_relay.RateLimit.IntervalSeconds;

    if (interval_seconds <= 0 || max_requests <= 0) {
        return MAIL_RELAY_RATE_ALLOWED;
    }

    char* key = mailrelay_rate_limit_build_key(sub, client_ip, template_key);
    bool is_global = (key == NULL);

    time_t now = time(NULL);

    pthread_mutex_lock(&g_rate_limit_mutex);
    if (!g_rate_limit_initialized) {
        g_rate_limit_initialized = true;
        g_rate_limit_head = NULL;
    }

    MailRelayApiRateLimitEntry* entry = is_global
        ? mailrelay_rate_limit_find_locked(NULL)
        : mailrelay_rate_limit_find_locked(key);

    if (!entry) {
        entry = is_global
            ? mailrelay_rate_limit_new_bucket_locked(NULL, now)
            : mailrelay_rate_limit_new_bucket_locked(key, now);
        if (!entry) {
            pthread_mutex_unlock(&g_rate_limit_mutex);
            free(key);
            return MAIL_RELAY_RATE_ALLOWED; /* Fail open on allocation error */
        }
    }

    if ((now - entry->window_start) >= interval_seconds) {
        entry->window_start = now;
        entry->count = 0;
    }

    MailRelayRateLimitResult result = MAIL_RELAY_RATE_ALLOWED;
    if (entry->count >= max_requests) {
        result = MAIL_RELAY_RATE_THROTTLED;
    } else {
        entry->count++;
    }

    pthread_mutex_unlock(&g_rate_limit_mutex);
    free(key);
    return result;
}

int mailrelay_rate_limit_count_for_key(const char* key) {
    int count = 0;
    pthread_mutex_lock(&g_rate_limit_mutex);
    MailRelayApiRateLimitEntry* entry = mailrelay_rate_limit_find_locked(key);
    if (entry) {
        count = entry->count;
    }
    pthread_mutex_unlock(&g_rate_limit_mutex);
    return count;
}
