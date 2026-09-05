/*
 * Chat Metrics for Prometheus
 *
 * Provides Prometheus-compatible metrics for chat operations.
 * Gauges, counters, and histograms for monitoring chat engine health and usage.
 */

#ifndef METRICS_H
#define METRICS_H

// Project includes
#include <src/hydrogen.h>

// Chat includes
#include "engine_cache.h"

// Metric value types for Prometheus
typedef enum {
    CHAT_METRIC_GAUGE = 0,      // Can go up or down
    CHAT_METRIC_COUNTER,        // Only increases
    CHAT_METRIC_HISTOGRAM       // Distribution of values
} ChatMetricType;



// Response time gauge: hydrogen_chat_engine_response_time_ms{database="x", engine="y"}
void chat_metrics_response_time(const char* database, const char* engine,
                                double response_time_ms);

// Conversation counter: hydrogen_chat_conversations_total{database="x", engine="y"}
void chat_metrics_conversation(const char* database, const char* engine);

// Token counter: hydrogen_chat_tokens_total{database="x", engine="y", type="prompt|completion"}
void chat_metrics_tokens(const char* database, const char* engine,
                         const char* token_type, int token_count);

// Error counter: hydrogen_chat_errors_total{database="x", engine="y", error_type="timeout|http|network"}
void chat_metrics_error(const char* database, const char* engine,
                        const char* error_type);



// Generate Prometheus-format metrics output for all engines
// Writes metrics to buffer, returns number of bytes written (or needed if buffer too small)
size_t chat_metrics_generate_prometheus(char* buffer, size_t buffer_size);

/* ----------------------------------------------------------------------------
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 * The full struct is defined here (not opaque) so tests can verify the
 * internal fields after metric calls. */
typedef struct ChatMetricEntry ChatMetricEntry;
ChatMetricEntry* chat_metrics_get_metric_entry(const char* database, const char* engine);

/* Writes a single Prometheus metric line ("name{labels} value") into buffer at
 * the given offset. Returns the new offset, or buffer_size when the buffer is
 * full / the write is truncated. Exposed (non-static) for Unity testing. */
size_t chat_metrics_write_metric(char* buffer, size_t offset, size_t buffer_size,
                                 const char* name, const char* labels, double value);

struct ChatMetricEntry {
    char database[64];
    char engine[64];
    char provider[32];

    /* Gauges */
    double health;              // 1.0 = healthy, 0.0 = unhealthy
    double response_time_ms;

    /* Counters */
    unsigned long long conversations_total;
    unsigned long long tokens_prompt_total;
    unsigned long long tokens_completion_total;
    unsigned long long errors_total;

    /* Request duration histogram (simplified: just track sum and count) */
    double request_duration_sum;
    unsigned long long request_duration_count;

    /* Last update time */
    time_t last_update;
};

#endif // METRICS_H
