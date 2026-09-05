/*
 * Chat Metrics Implementation
 *
 * Lightweight Prometheus-compatible metrics for chat operations.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "metrics.h"
#include "health.h"

// Simple metric storage (per-database, per-engine)
#define MAX_METRIC_ENTRIES 256
static ChatMetricEntry metric_entries[MAX_METRIC_ENTRIES];
static size_t metric_entry_count = 0;
static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

// Find or create metric entry
 ChatMetricEntry* chat_metrics_get_metric_entry(const char* database, const char* engine) {
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

// Response time gauge
void chat_metrics_response_time(const char* database, const char* engine,
                                double response_time_ms) {
    ChatMetricEntry* entry = chat_metrics_get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->response_time_ms = response_time_ms;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Conversation counter
void chat_metrics_conversation(const char* database, const char* engine) {
    ChatMetricEntry* entry = chat_metrics_get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->conversations_total++;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}

// Token counter
void chat_metrics_tokens(const char* database, const char* engine,
                         const char* token_type, int token_count) {
    ChatMetricEntry* entry = chat_metrics_get_metric_entry(database, engine);
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
    
    ChatMetricEntry* entry = chat_metrics_get_metric_entry(database, engine);
    if (!entry) return;
    
    pthread_mutex_lock(&metrics_mutex);
    entry->errors_total++;
    entry->last_update = time(NULL);
    pthread_mutex_unlock(&metrics_mutex);
}


// Helper to write metric line to buffer
size_t chat_metrics_write_metric(char* buffer, size_t offset, size_t buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_tokens_total", labels,
                              (double)entry->tokens_prompt_total);

        // Completion tokens
        if (offset < buffer_size) {
            snprintf(labels, sizeof(labels),
                     "database=\"%s\",engine=\"%s\",type=\"completion\"",
                     entry->database, entry->engine);
            offset = chat_metrics_write_metric(buffer, offset, buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
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
        offset = chat_metrics_write_metric(buffer, offset, buffer_size,
                              "hydrogen_chat_request_duration_seconds_sum", labels,
                              entry->request_duration_sum);
        
        // Count
        if (offset < buffer_size) {
            offset = chat_metrics_write_metric(buffer, offset, buffer_size,
                                  "hydrogen_chat_request_duration_seconds_count", labels,
                                  (double)entry->request_duration_count);
        }
    }
    
    pthread_mutex_unlock(&metrics_mutex);
    
    return offset;
}
