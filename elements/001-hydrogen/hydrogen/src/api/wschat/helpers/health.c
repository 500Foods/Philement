/*
 * Chat Health Monitoring Implementation
 *
 * Background health checks for chat engines with configurable intervals.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "health.h"
#include "proxy.h"

// Health check implementation for OpenAI-compatible APIs
// Uses a minimal valid request to test connectivity
bool chat_health_check_openai(const ChatEngineConfig* engine) {
    if (!engine || !engine->api_url[0]) return false;

    // Build a minimal valid request body
    // Use max_tokens: 1 to minimize cost/time
    char minimal_body[512];
    snprintf(minimal_body, sizeof(minimal_body),
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"max_tokens\":1}",
        engine->model);

    ChatProxyConfig config = chat_proxy_get_default_config();
    config.request_timeout_seconds = HEALTH_TIMEOUT_SECONDS;
    config.connect_timeout_seconds = 5;
    config.max_retries = 0;  // Don't retry health checks

    ChatProxyResult* result = chat_proxy_send_request(engine, minimal_body, &config);

    bool is_healthy = false;
    if (result) {
        // Consider 2xx responses as healthy, even if we get a content filter
        // or other "soft" error
        is_healthy = (result->http_status >= 200 && result->http_status < 300) ||
                     (result->http_status == 400);  // Bad request still means server is up
        chat_proxy_result_destroy(result);
    }

    return is_healthy;
}

// Health check implementation for Anthropic
bool chat_health_check_anthropic(const ChatEngineConfig* engine) {
    if (!engine || !engine->api_url[0]) return false;

    // Anthropic uses a different request format
    char minimal_body[512];
    snprintf(minimal_body, sizeof(minimal_body),
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"max_tokens\":1}",
        engine->model);

    ChatProxyConfig config = chat_proxy_get_default_config();
    config.request_timeout_seconds = HEALTH_TIMEOUT_SECONDS;
    config.connect_timeout_seconds = 5;
    config.max_retries = 0;

    ChatProxyResult* result = chat_proxy_send_request(engine, minimal_body, &config);

    bool is_healthy = false;
    if (result) {
        is_healthy = (result->http_status >= 200 && result->http_status < 300) ||
                     (result->http_status == 400);
        chat_proxy_result_destroy(result);
    }

    return is_healthy;
}

// Health check implementation for Ollama
bool chat_health_check_ollama(const ChatEngineConfig* engine) {
    if (!engine || !engine->api_url[0]) return false;

    // Ollama uses OpenAI-compatible format now
    char minimal_body[512];
    snprintf(minimal_body, sizeof(minimal_body),
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"stream\":false,\"options\":{\"num_predict\":1}}",
        engine->model);

    ChatProxyConfig config = chat_proxy_get_default_config();
    config.request_timeout_seconds = HEALTH_TIMEOUT_SECONDS;
    config.connect_timeout_seconds = 5;
    config.max_retries = 0;

    ChatProxyResult* result = chat_proxy_send_request(engine, minimal_body, &config);

    bool is_healthy = false;
    if (result) {
        is_healthy = (result->http_status >= 200 && result->http_status < 300) ||
                     (result->http_status == 400);
        chat_proxy_result_destroy(result);
    }

    return is_healthy;
}

// Perform health check on a single engine
bool chat_health_check_engine(ChatEngineConfig* engine) {
    if (!engine) return false;

    bool is_healthy = false;

    switch (engine->provider) {
        case CEC_PROVIDER_OPENAI:
            is_healthy = chat_health_check_openai(engine);
            break;
        case CEC_PROVIDER_ANTHROPIC:
            is_healthy = chat_health_check_anthropic(engine);
            break;
        case CEC_PROVIDER_OLLAMA:
            is_healthy = chat_health_check_ollama(engine);
            break;
        case CEC_PROVIDER_UNKNOWN:
        default:
            log_this(SR_CHAT, "Unknown provider type for health check: %s",
                     LOG_LEVEL_ERROR, 1, engine->name);
            is_healthy = false;
    }

    // Update engine health status
    chat_engine_config_mark_health_checked(engine, is_healthy);

    log_this(SR_CHAT, "Health check for %s: %s", LOG_LEVEL_DEBUG, 2,
             engine->name, is_healthy ? "healthy" : "unhealthy");

    return is_healthy;
}

// Background health check thread function
void* chat_health_monitor_thread(void* arg) {
    if (!arg) return NULL;

    ChatEngineCache* cache = (ChatEngineCache*)arg;

    log_this(SR_CHAT, "Health monitor thread started for database: %s",
             LOG_LEVEL_STATE, 1, cache->database_name ? cache->database_name : "unknown");

    while (cache->health_monitor_running) {
        // Check all engines in the cache
        pthread_rwlock_rdlock(&cache->cache_lock);

        size_t engine_count = cache->engine_count;
        ChatEngineConfig** engines = NULL;

        if (engine_count > 0) {
            engines = malloc(engine_count * sizeof(ChatEngineConfig*));
            if (engines) {
                for (size_t i = 0; i < engine_count; i++) {
                    engines[i] = cache->engines[i];
                }
            }
        }

        pthread_rwlock_unlock(&cache->cache_lock);

        // Perform health checks outside the lock
        if (engines) {
            for (size_t i = 0; i < engine_count; i++) {
                if (!cache->health_monitor_running) break;

                ChatEngineConfig* engine = engines[i];
                if (engine && engine->liveliness_seconds > 0) {
                    // Check if enough time has passed since last check
                    time_t now = time(NULL);
                    pthread_mutex_lock(&engine->health_mutex);
                    time_t last_check = engine->last_health_check;
                    int interval = engine->liveliness_seconds;
                    pthread_mutex_unlock(&engine->health_mutex);

                    if ((now - last_check) >= interval) {
                        chat_health_check_engine(engine);
                    }
                }
            }
            free(engines);
        }

        // Sleep for a short interval before next check cycle
        // We check every 10 seconds to see if any engines need checking
        for (int i = 0; i < 10 && cache->health_monitor_running; i++) {
            sleep(1);
        }
    }

    log_this(SR_CHAT, "Health monitor thread stopped for database: %s",
             LOG_LEVEL_STATE, 1, cache->database_name ? cache->database_name : "unknown");

    return NULL;
}

// Start health monitoring for a database
bool chat_health_monitor_start(ChatEngineCache* cache) {
    if (!cache) return false;

    // Check if already running
    if (cache->health_monitor_running && cache->health_monitor_thread) {
        log_this(SR_CHAT, "Health monitor already running for: %s",
                 LOG_LEVEL_DEBUG, 1, cache->database_name);
        return true;
    }

    // Check if any engines have liveliness enabled
    bool has_liveliness = false;
    pthread_rwlock_rdlock(&cache->cache_lock);
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i] && cache->engines[i]->liveliness_seconds > 0) {
            has_liveliness = true;
            break;
        }
    }
    pthread_rwlock_unlock(&cache->cache_lock);

    if (!has_liveliness) {
        log_this(SR_CHAT, "No engines with liveliness enabled for: %s",
                 LOG_LEVEL_DEBUG, 1, cache->database_name);
        return true;  // Not an error, just nothing to monitor
    }

    // Start the monitor thread
    cache->health_monitor_running = true;

    int result = pthread_create(&cache->health_monitor_thread, NULL,
                                chat_health_monitor_thread, cache);

    if (result != 0) {
        cache->health_monitor_running = false;
        log_this(SR_CHAT, "Failed to start health monitor thread for: %s (error: %d)",
                 LOG_LEVEL_ERROR, 2, cache->database_name, result);
        return false;
    }

    log_this(SR_CHAT, "Health monitor started for database: %s",
             LOG_LEVEL_STATE, 1, cache->database_name);

    return true;
}



