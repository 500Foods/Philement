/*
 * Chat Metrics Implementation
 *
 * Lightweight Prometheus-compatible metrics for chat operations.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "chat_metrics.h"
#include "chat_health.h"

// Simple metric storage (per-database, per-engine)
typedef struct ChatMetricEntry {
    char database[64];
    char engine[64];
    char provider[32];
    
    // Gauges
    double health;              // 1.0 = healthy, 0.0 = unhealthy
    double response_time_ms;
    
    // Counters
    unsigned long long conversations_total;
    unsigned long long tokens_prompt_total;
    unsigned long long tokens_completion_total;
    unsigned long long errors_total;
    
    // Request duration histogram (simplified: just track sum and count)
    double request_duration_sum;
    unsigned long long request_duration_count;
    
    // Last update time
    time_t last_update;
} ChatMetricEntry;

// Simple fixed-size metric storage
#define MAX_METRIC_ENTRIES 256
static ChatMetricEntry metric_entries[MAX_METRIC_ENTRIES];
static size_t metric_entry_count = 0;
static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool metrics_initialized = false;

// Find or create metric entry
static ChatMetricEntry* get_metric_entry(const char* database, const char* engine) {
    if (!database || !engine) return NULL;
    
    pthread_mutex_lock(&metrics_mutex);
    
    // Look for existing entry
    for (size_t i = 0; i < metric_entry_count; i++) {
        if (strcmp(metric_entries[i].database, database) == 0 &&
            strcmp(metric_entries[i].engine, engine) == 0) {
            pthread_mutex_unlock(&metrics_mutex);
            return &metric_entries[i];
        }
    }
    
    // Create new entry if space available
    if (metric_entry_count < MAX_METRIC_ENTRIES) {
        ChatMetricEntry* entry = &metric_entries[metric_entry_count++];
        strncpy(entry->database, database, sizeof(entry->database) - 1);
        entry->database[sizeof(entry->database) - 1] = '\0';
        strncpy(entry->engine, engine, sizeof(entry->engine) - 1);
        entry->engine[sizeof(entry->engine) - 1] = '\0';
        entry->provider[0] = '\0';
        entry->health = 1.0;
        entry->response_time_ms = 0.0;
        entry->conversations_total = 0;
        entry->tokens_prompt_total = 0;
        entry->tokens_completion_total = 0;
        entry->errors_total = 0;
        entry->request_duration_sum = 0.0;
        entry->request_duration_count = 0;
        entry->last_update = time(NULL);
        pthread_mutex_unlock(&metrics_mutex);
        return entry;
    }
    
    pthread_mutex_unlock(&metrics_mutex);
    return NULL;  // No space available
}

// Initialize chat metrics subsystem
bool chat_metrics_init(void) {
    pthread_mutex_lock(&metrics_mutex);
    if (metrics_initialized) {
        pthread_mutex_unlock(&metrics_mutex);
        return true;
    }
    
    memset(metric_entries, 0, sizeof(metric_entries));
    metric_entry_count = 0;
    metrics_initialized = true;
    
    pthread_mutex_unlock(&metrics_mutex);
    
    log_this(SR_CHAT, "Chat metrics initialized", LOG_LEVEL_DEBUG, 0);
    return true;
}

// Cleanup chat metrics
void chat_metrics_cleanup(void) {
    pthread_mutex_lock(&metrics_mutex);
    metrics_initialized = false;
    metric_entry_count = 0;
    pthread_mutex_unlock(&metrics_mutex);
}

// Engine health gauge
void chat_metrics_engine_health(const char* database, const char* engine,
                                const char* provider, bool healthy) {
    ChatMetricEntry* entry = get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->health = healthy ? 1.0 : 0.0;
    if (provider && !entry->provider[0]) {
        strncpy(entry->provider, provider, sizeof(entry->provider) - 1);
        entry->provider[sizeof(entry->provider) - 1] = '\0';
    }
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Response time gauge
void chat_metrics_response_time(const char* database, const char* engine,
                                double response_time_ms) {
    ChatMetricEntry* entry = get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->response_time_ms = response_time_ms;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Conversation counter
void chat_metrics_conversation(const char* database, const char* engine) {
    ChatMetricEntry* entry = get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->conversations_total++;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Token counter
void chat_metrics_tokens(const char* database, const char* engine,
                         const char* token_type, int token_count) {
    ChatMetricEntry* entry = get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    if (token_type && strcmp(token_type, "prompt") == 0) {
        entry->tokens_prompt_total += (unsigned long long)token_count;
    } else if (token_type && strcmp(token_type, "completion") == 0) {
        entry->tokens_completion_total += (unsigned long long)token_count;
    } else {
        // Unknown type - add to both (or could be ignored)
        entry->tokens_prompt_total += (unsigned long long)token_count;
    }
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Error counter
void chat_metrics_error(const char* database, const char* engine,
                        const char* error_type) {
    (void)error_type;  // Could be used for error type breakdown in future
    
    ChatMetricEntry* entry = get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->errors_total++;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Request duration histogram
void chat_metrics_request_duration(const char* engine, double duration_seconds) {
    // Find any entry for this engine (across all databases)
    pthread_mutex_lock(&metrics_mutex);
    for (size_t i = 0; i < metric_entry_count; i++) {
        if (strcmp(metric_entries[i].engine, engine) == 0) {
            metric_entries[i].request_duration_sum += duration_seconds;
            metric_entries[i].request_duration_count++;
            metric_entries[i].last_update = time(NULL);
            break;
        }
    }
    pthread_mutex_unlock(&metrics_mutex);
}

// Update all metrics for an engine from its current state
void chat_metrics_update_from_engine(const char* database, ChatEngineConfig* engine) {
    if (!database || !engine) return;
    
    const char* provider_str = chat_engine_provider_to_string(engine->provider);
    
    pthread_mutex_lock(&engine->health_mutex);
    bool is_healthy = engine->is_healthy;
    double avg_response = engine->avg_response_time_ms;
    unsigned long long conversations = engine->conversations_24h;
    unsigned long long tokens = engine->tokens_24h;
    pthread_mutex_unlock(&engine->health_mutex);
    
    ChatMetricEntry* entry = get_metric_entry(database, engine->name);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->health = is_healthy ? 1.0 : 0.0;
    strncpy(entry->provider, provider_str, sizeof(entry->provider) - 1);
    entry->provider[sizeof(entry->provider) - 1] = '\0';
    entry->response_time_ms = avg_response;
    entry->conversations_total = conversations;
    entry->tokens_completion_total = tokens;  // Simplified: store all tokens here
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Helper to write metric line to buffer
static size_t write_metric(char* buffer, size_t offset, size_t buffer_size,
                           const char* name, const char* labels, double value) {
    if (offset >= buffer_size) return offset;

    size_t remaining = buffer_size - offset;
    int written = snprintf(buffer + offset, remaining,
                           "%s{%s} %.3f\n",
                           name, labels, value);

    if (written < 0 || (size_t)written >= remaining) {
        return buffer_size;  // Buffer full
    }

    return offset + (size_t)written;
}

// Generate Prometheus-format metrics output
size_t chat_metrics_generate_prometheus(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return 0;

    size_t offset = 0;

    // Write header comments
    offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                               "# HELP hydrogen_chat_engine_health Health status of chat engines (1=healthy, 0=unhealthy)\n"
                               "# TYPE hydrogen_chat_engine_health gauge\n");

    pthread_mutex_lock(&metrics_mutex);

    // Buffer for label formatting - sized to handle max field lengths
    // database[64] + engine[64] + provider[32] + extra formatting = ~256+ bytes
    // Using larger buffer to avoid compiler warnings about truncation
    char labels[65536];

    // Engine health gauges
    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];
        snprintf(labels, sizeof(labels),
                 "database=\"%s\",engine=\"%s\",provider=\"%s\"",
                 entry->database, entry->engine,
                 entry->provider[0] ? entry->provider : "unknown");
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_engine_health", labels, entry->health);
    }

    // Response time gauges
    if (offset < buffer_size) {
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                   "# HELP hydrogen_chat_engine_response_time_ms Average response time in milliseconds\n"
                                   "# TYPE hydrogen_chat_engine_response_time_ms gauge\n");
    }

    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];
        snprintf(labels, sizeof(labels),
                 "database=\"%s\",engine=\"%s\"",
                 entry->database, entry->engine);
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_engine_response_time_ms", labels,
                              entry->response_time_ms);
    }

    // Conversation counters
    if (offset < buffer_size) {
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                   "# HELP hydrogen_chat_conversations_total Total number of chat conversations\n"
                                   "# TYPE hydrogen_chat_conversations_total counter\n");
    }

    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];
        snprintf(labels, sizeof(labels),
                 "database=\"%s\",engine=\"%s\"",
                 entry->database, entry->engine);
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_conversations_total", labels,
                              (double)entry->conversations_total);
    }

    // Token counters
    if (offset < buffer_size) {
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                   "# HELP hydrogen_chat_tokens_total Total number of tokens used\n"
                                   "# TYPE hydrogen_chat_tokens_total counter\n");
    }

    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];

        // Prompt tokens
        snprintf(labels, sizeof(labels),
                 "database=\"%s\",engine=\"%s\",type=\"prompt\"",
                 entry->database, entry->engine);
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_tokens_total", labels,
                              (double)entry->tokens_prompt_total);

        // Completion tokens
        if (offset < buffer_size) {
            snprintf(labels, sizeof(labels),
                     "database=\"%s\",engine=\"%s\",type=\"completion\"",
                     entry->database, entry->engine);
            offset = write_metric(buffer, offset, buffer_size,
                                  "hydrogen_chat_tokens_total", labels,
                                  (double)entry->tokens_completion_total);
        }
    }

    // Error counters
    if (offset < buffer_size) {
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                   "# HELP hydrogen_chat_errors_total Total number of chat errors\n"
                                   "# TYPE hydrogen_chat_errors_total counter\n");
    }

    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];
        snprintf(labels, sizeof(labels),
                 "database=\"%s\",engine=\"%s\",error_type=\"total\"",
                 entry->database, entry->engine);
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_errors_total", labels,
                              (double)entry->errors_total);
    }
    
    // Request duration histogram (simplified)
    if (offset < buffer_size) {
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                   "# HELP hydrogen_chat_request_duration_seconds Chat request duration in seconds\n"
                                   "# TYPE hydrogen_chat_request_duration_seconds histogram\n");
    }
    
    for (size_t i = 0; i < metric_entry_count && offset < buffer_size; i++) {
        const ChatMetricEntry* entry = &metric_entries[i];

        // Sum
        snprintf(labels, sizeof(labels), "engine=\"%s\"", entry->engine);
        offset = write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_request_duration_seconds_sum", labels,
                              entry->request_duration_sum);
        
        // Count
        if (offset < buffer_size) {
            offset = write_metric(buffer, offset, buffer_size,
                                  "hydrogen_chat_request_duration_seconds_count", labels,
                                  (double)entry->request_duration_count);
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
    
    return offset;
}
