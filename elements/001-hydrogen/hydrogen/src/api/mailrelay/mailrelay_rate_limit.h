/*
 * Mail Relay API Rate Limiting
 *
 * Phase 14.3: Per-key fixed-window rate limiter for the REST send/preview
 * endpoints. Mirrors the chat_rate_limit precedent (src/api/wschat/helpers/
 * chat_rate_limit.c) and the mailrelay_event_check_rate_limit precedent
 * (src/mailrelay/mailrelay_events.c).
 *
 * Scope is configurable via MailRelayConfig.RateLimit.Scope:
 *   0 = global  (single bucket for all API callers)
 *   1 = user    (per JWT sub / user_id)
 *   2 = ip      (per client IP from JWT claims)
 *   3 = template (per template_key from the request body)
 *
 * Window semantics: per-key fixed window. When wall-clock now exceeds
 * window_start + interval_seconds, the counter resets.
 *
 * Fail-open: when rate limiting is disabled (Enabled=false), when app_config
 * is unavailable, or on allocation failure, the check always returns
 * MAIL_RELAY_RATE_ALLOWED. A production fault under rate limiting must never
 * lock out legitimate callers.
 */

#ifndef MAILRELAY_RATE_LIMIT_H
#define MAILRELAY_RATE_LIMIT_H

#include <src/globals.h>
#include <src/config/config.h>

#include <stdbool.h>
#include <time.h>

typedef enum MailRelayRateLimitResult {
    MAIL_RELAY_RATE_ALLOWED = 0,
    MAIL_RELAY_RATE_THROTTLED = 1
} MailRelayRateLimitResult;

typedef struct MailRelayApiRateLimitEntry {
    char* key;                  // Bucket key (scope-dependent string)
    time_t window_start;        // Wall-clock start of the current window
    int count;                  // Requests in the current window
    struct MailRelayApiRateLimitEntry* next;
} MailRelayApiRateLimitEntry;

/*
 * Initialize the rate-limit module. Idempotent. Safe to call before
 * app_config is available.
 */
void mailrelay_rate_limit_init(void);

/*
 * Free all buckets and release module state. Safe to call multiple times.
 * Pairs with mailrelay_rate_limit_init().
 */
void mailrelay_rate_limit_shutdown(void);

/*

 * Build the rate-limit bucket key for a request based on the configured
 * scope. Returns a malloc'd string the caller must free. Returns NULL when
 * the scope is GLOBAL (single bucket, key is NULL).
 *
 *   sub          - JWT subject (user_id), may be NULL
 *   client_ip    - client IP from JWT claims, may be NULL
 *   template_key - template_key from the request body, may be NULL
 */
char* mailrelay_rate_limit_build_key(const char* sub,
                                     const char* client_ip,
                                     const char* template_key);

/*
 * Check and record a request against the rate limit.
 *
 * Returns MAIL_RELAY_RATE_ALLOWED if the request may proceed (and the
 * counter is advanced), or MAIL_RELAY_RATE_THROTTLED if the caller must
 * return a MAIL_RATE_LIMITED error.
 *
 * Fails open (returns ALLOWED) when:
 *   - app_config is NULL or mail_relay.RateLimit.Enabled is false
 *   - interval_seconds <= 0 or max_requests <= 0
 *   - allocation fails
 */
MailRelayRateLimitResult mailrelay_rate_limit_check_and_record(
    const char* sub,
    const char* client_ip,
    const char* template_key);

/*
 * Look up the bucket for key. Caller must hold g_rate_limit_mutex.
 * Exposed for direct unit testing. Returns NULL if not found.
 */
MailRelayApiRateLimitEntry* mailrelay_rate_limit_find_locked(const char* key);

/*
 * Allocate a new bucket for key at time now and link it to the head.
 * Caller must hold g_rate_limit_mutex. Exposed for direct unit testing.
 * Returns NULL on allocation failure.
 */
MailRelayApiRateLimitEntry* mailrelay_rate_limit_new_bucket_locked(const char* key,
                                                                 time_t now);

#endif /* MAILRELAY_RATE_LIMIT_H */
