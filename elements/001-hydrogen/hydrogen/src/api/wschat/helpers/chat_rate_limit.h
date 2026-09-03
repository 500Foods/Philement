/*
 * Chat Rate Limiting
 *
 * Phase 10b (CHAT_FINALE). Per-sub fixed-window rate limiting for the
 * chat subsystem (REST auth_chat, REST auth_chats, WebSocket chat).
 * Mirrors the mailrelay_event_check_rate_limit precedent
 * (src/mailrelay/mailrelay_events.c) — linked list of per-key buckets
 * guarded by a single mutex, fail-open on allocation error so an
 * internal fault never locks out legitimate users.
 *
 * Bucket counters track both request count AND estimated token budget
 * (Q4: bundle both). Input tokens are estimated at request entry from
 * the concatenated message content using a chars/4 heuristic. Output
 * tokens are recorded by the streaming layer when it observes the
 * chat_done chunk's usage.completion_tokens; non-streaming and local
 * providers emit no usage, so output is recorded as zero.
 *
 * Window semantics: per-sub fixed window. When the wall clock exceeds
 * window_start + interval_seconds, the counters reset and the next
 * request starts a fresh window.
 *
 * Configuration: Chat.RateLimit.{Enabled, MaxRequestsPerInterval,
 * IntervalSeconds, MaxTokensPerInterval}. Disabled by default; when
 * Enabled=false chat_rate_limit_check_and_record short-circuits and
 * always returns CHAT_RATE_LIMIT_ALLOWED.
 */

#ifndef HYDROGEN_CHAT_RATE_LIMIT_H
#define HYDROGEN_CHAT_RATE_LIMIT_H

#include <src/globals.h>

#include <stddef.h>
#include <stdbool.h>

typedef enum ChatRateLimitResult {
    CHAT_RATE_LIMIT_ALLOWED = 0,
    CHAT_RATE_LIMIT_THROTTLED_REQUESTS = 1,
    CHAT_RATE_LIMIT_THROTTLED_TOKENS = 2
} ChatRateLimitResult;

typedef struct ChatRateLimitEntry {
    char *sub;
    time_t window_start;
    int request_count;
    long long token_count;
    struct ChatRateLimitEntry *next;
} ChatRateLimitEntry;

/*
 * Initialize the rate limit module. Idempotent. No-op if already
 * initialized. Safe to call before app_config is available.
 */
void chat_rate_limit_init(void);

/*
 * Free all buckets and release module state. Safe to call multiple
 * times. Pairs with chat_rate_limit_init().
 */
void chat_rate_limit_shutdown(void);

/*
 * Reset all buckets. Test seam for Unity. Not for production use.
 */
void chat_rate_limit_reset_all(void);

/*
 * Check whether the given sub is allowed to make a chat request that
 * will consume approximately est_input_tokens (chars/4 heuristic
 * already applied by the caller). On ALLOWED, the request is
 * recorded against the sub's bucket. On THROTTLED_*, no counters are
 * advanced and the caller must return an error envelope to the user.
 *
 * Fails open (returns ALLOWED) on allocation error or when rate
 * limiting is disabled in config.
 *
 * Must be called from a thread that does not already hold the
 * module's mutex.
 */
ChatRateLimitResult chat_rate_limit_check_and_record(const char *sub,
                                                     long long est_input_tokens);

/*
 * Record actual output tokens for a previously-allowed request.
 * Called from the streaming layer when it observes the chat_done
 * chunk's usage.completion_tokens. Silently ignores unknown subs,
 * disabled config, allocation errors, and non-positive token counts.
 *
 * Must be called from a thread that does not already hold the
 * module's mutex.
 */
void chat_rate_limit_record_output(const char *sub, long long output_tokens);

/*
 * Return the current request count for sub. Returns 0 if sub has no
 * bucket or the module is uninitialized. Test seam.
 */
int chat_rate_limit_request_count(const char *sub);

/*
 * Return the current estimated token count for sub. Returns 0 if sub
 * has no bucket or the module is uninitialized. Test seam.
 */
long long chat_rate_limit_token_count(const char *sub);

/*
 * Estimate input tokens for a chat request from the messages JSON
 * array. Walks every string field under every message and applies the
 * chars/4 heuristic from Q4.1 of the Phase 10b design. Returns 0 for
 * NULL or non-array input.
 *
 * Public so chokepoint call sites in auth_chat.c, auth_chats.c, and
 * websocket_server_chat.c can share one implementation.
 */
long long chat_rate_limit_estimate_input_tokens(json_t *messages);

/*
 * Count UTF-8 characters in a string. Exposed for direct unit
 * testing. Returns 0 for NULL.
 */
size_t chat_rate_limit_utf8_chars(const char *s);

/*
 * Walk a JSON value and sum its string leaf lengths (UTF-8
 * characters). Exposed for direct unit testing.
 */
long long chat_rate_limit_walk_json(json_t *value);

/*
 * Look up the bucket for sub. Caller must hold the module's mutex.
 * Exposed for direct unit testing. Returns NULL if not found.
 */
ChatRateLimitEntry *chat_rate_limit_find_locked(const char *sub);

/*
 * Allocate a new bucket for sub at time now and link it to the
 * head. Caller must hold the module's mutex. Exposed for direct
 * unit testing. Returns NULL on allocation failure.
 */
ChatRateLimitEntry *chat_rate_limit_new_bucket_locked(const char *sub,
                                                       time_t now);

/*
 * Build a rate-limited error envelope matching the chat REST
 * subsystem's error shape (success=false, error, message, error_code).
 * `error_code` is 4291 for request-count throttle and 4292 for
 * token-budget throttle. `message` is a human-readable hint. The
 * returned json_t* is owned by the caller (decref with
 * json_decref). Returns NULL on allocation failure.
 *
 * Used by REST chokepoints (auth_chat.c, auth_chats.c).
 */
json_t *chat_rate_limit_build_error_response(int error_code,
                                            const char *message);

#endif /* HYDROGEN_CHAT_RATE_LIMIT_H */