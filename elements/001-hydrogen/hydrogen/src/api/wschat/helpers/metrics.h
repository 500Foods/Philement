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

// Initialize chat metrics subsystem
bool chat_metrics_init(void);
void chat_metrics_cleanup(void);

// Engine health gauge: hydrogen_chat_engine_health{database="x", engine="y", provider="z"}
// Value: 1 = healthy, 0 = unhealthy
void chat_metrics_engine_health(const char* database, const char* engine,
                                const char* provider, bool healthy);

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

// Request duration histogram: hydrogen_chat_request_duration_seconds_bucket{engine="x", le="y"}
void chat_metrics_request_duration(const char* engine, double duration_seconds);

// Update all metrics for an engine from its current state
void chat_metrics_update_from_engine(const char* database, ChatEngineConfig* engine);

// Generate Prometheus-format metrics output for all engines
// Writes metrics to buffer, returns number of bytes written (or needed if buffer too small)
size_t chat_metrics_generate_prometheus(char* buffer, size_t buffer_size);

#endif // METRICS_H
